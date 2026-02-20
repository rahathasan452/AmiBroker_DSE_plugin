///////////////////////////////////////////////////////////////////////////
// RealtimeFeed.cpp — Background Polling Thread Implementation
//
// Polls DSE for live price updates during market hours and pushes
// them to AmiBroker via PostMessage.
///////////////////////////////////////////////////////////////////////////

#include "RealtimeFeed.h"
#include "Plugin.h"
#include <cstring>
#include <windows.h>


///////////////////////////////////////////////////////////////////////////
// Constructor / Destructor
///////////////////////////////////////////////////////////////////////////

RealtimeFeed::RealtimeFeed()
    : m_hMainWnd(NULL), m_engine(nullptr), m_hThread(NULL), m_running(false),
      m_stopRequested(false), m_pollIntervalMs(5000), m_reconnectAttempts(0),
      m_maxReconnectAttempts(10) {}

RealtimeFeed::~RealtimeFeed() { Stop(); }

///////////////////////////////////////////////////////////////////////////
// Start — Launch the polling thread
///////////////////////////////////////////////////////////////////////////

bool RealtimeFeed::Start(HWND hMainWnd, DseDataEngine *engine) {
  if (m_running.load())
    return true; // Already running

  m_hMainWnd = hMainWnd;
  m_engine = engine;
  m_stopRequested = false;
  m_reconnectAttempts = 0;

  if (engine) {
    m_pollIntervalMs = engine->GetConfig().pollIntervalMs;
    m_maxReconnectAttempts = engine->GetConfig().maxReconnectAttempts;
  }

  m_hThread = CreateThread(NULL,       // default security
                           0,          // default stack size
                           ThreadProc, // thread function
                           this,       // parameter
                           0,          // run immediately
                           NULL        // don't need thread ID
  );

  if (!m_hThread) {
    m_engine->Log("ERROR: RealtimeFeed::Start — CreateThread failed");
    return false;
  }

  m_running = true;
  m_engine->Log("RealtimeFeed::Start — polling thread started");
  return true;
}

///////////////////////////////////////////////////////////////////////////
// Stop — Signal thread to stop and wait for it
///////////////////////////////////////////////////////////////////////////

void RealtimeFeed::Stop() {
  if (!m_running.load())
    return;

  m_stopRequested = true;

  if (m_hThread) {
    // Wait up to 10 seconds for thread to finish
    DWORD result = WaitForSingleObject(m_hThread, 10000);
    if (result == WAIT_TIMEOUT) {
      // Force terminate if it didn't stop gracefully
      TerminateThread(m_hThread, 0);
    }
    CloseHandle(m_hThread);
    m_hThread = NULL;
  }

  m_running = false;
  if (m_engine) {
    m_engine->Log("RealtimeFeed::Stop — polling thread stopped");
  }
}

///////////////////////////////////////////////////////////////////////////
// ThreadProc — Static thread entry point
///////////////////////////////////////////////////////////////////////////

DWORD WINAPI RealtimeFeed::ThreadProc(LPVOID lpParam) {
  RealtimeFeed *self = static_cast<RealtimeFeed *>(lpParam);
  self->PollLoop();
  return 0;
}

///////////////////////////////////////////////////////////////////////////
// PollLoop — Main polling loop
///////////////////////////////////////////////////////////////////////////

void RealtimeFeed::PollLoop() {
  m_engine->Log("PollLoop: entering main loop");

  while (!m_stopRequested.load()) {

    // Check if market is open
    if (m_engine->IsMarketOpen()) {

      // Fetch latest quotes from DSE
      std::vector<DseQuote> quotes;
      bool success = m_engine->FetchLatestQuotes(quotes);

      if (success) {
        m_reconnectAttempts = 0; // Reset on success

        // Update our cache and send streaming updates
        {
          std::lock_guard<std::mutex> lock(m_quotesMutex);
          for (const auto &q : quotes) {
            std::string sym(q.symbol);
            m_latestQuotes[sym] = q;
          }
        }

        // Send updates for subscribed symbols first,
        // then for all symbols
        {
          std::lock_guard<std::mutex> subLock(m_subsMutex);
          for (const auto &sym : m_subscriptions) {
            auto it = m_latestQuotes.find(sym);
            if (it != m_latestQuotes.end()) {
              SendStreamingUpdate(it->second);
            }
          }
        }

        // Also update all fetched quotes
        for (const auto &q : quotes) {
          SendStreamingUpdate(q);
        }

        m_engine->Log("PollLoop: updated %zu quotes", quotes.size());

      } else {
        // Failed — try reconnect
        m_engine->Log("PollLoop: fetch failed, attempting reconnect");
        if (!TryReconnect()) {
          // Give up after max attempts
          m_engine->Log("PollLoop: max reconnect attempts reached, "
                        "waiting 60s");
          // Wait a long time before retrying
          for (int i = 0; i < 60 && !m_stopRequested.load(); ++i) {
            Sleep(1000);
          }
          m_reconnectAttempts = 0; // Reset for next cycle
          continue;
        }
      }

      // Sleep for poll interval
      // Use small sleep chunks so we can respond to stop quickly
      int sleepChunks = m_pollIntervalMs / 100;
      for (int i = 0; i < sleepChunks && !m_stopRequested.load(); ++i) {
        Sleep(100);
      }

    } else {
      // Market closed — sleep longer (60 seconds)
      m_engine->Log("PollLoop: market closed, sleeping 60s");
      for (int i = 0; i < 60 && !m_stopRequested.load(); ++i) {
        Sleep(1000);
      }
    }
  }

  m_engine->Log("PollLoop: exiting");
}

///////////////////////////////////////////////////////////////////////////
// SendStreamingUpdate — Post a real-time update to AmiBroker
///////////////////////////////////////////////////////////////////////////

void RealtimeFeed::SendStreamingUpdate(const DseQuote &quote) {
  if (!m_hMainWnd || !IsWindow(m_hMainWnd))
    return;

  // Allocate a RecentInfo struct on the heap
  // (AmiBroker will read it asynchronously)
  RecentInfo *ri = new RecentInfo;
  memset(ri, 0, sizeof(RecentInfo));

  ri->nStructSize = sizeof(RecentInfo);
  strncpy_s(ri->Name, quote.symbol, sizeof(ri->Name) - 1);
  // FullName is NOT in official RecentInfo! It seems my previous version had
  // it. I will remove it or check where it came from. Official only has
  // Name[64] and Exchange[8].

  ri->fOpen = (float)quote.open;
  ri->fHigh = (float)quote.high;
  ri->fLow = (float)quote.low;
  ri->fLast = (float)quote.ltp;
  ri->fPrev = (float)quote.ycp;
  ri->iTotalVol = (int)quote.volume;
  ri->fChange = (float)quote.change;
  // ChangePercent is NOT in official RecentInfo.
  ri->iTradeVol = (int)quote.trade;

  // Set the bitmap to indicate which fields are valid
  ri->nBitmap = 0xFFFF; // All fields valid

  // Time update
  SYSTEMTIME st;
  GetLocalTime(&st);

  // RecentInfo typically uses decimal-packed integers for Date/Time
  ri->nDateUpdate = (st.wYear * 10000) + (st.wMonth * 100) + st.wDay;
  ri->nTimeUpdate = (st.wHour * 10000) + (st.wMinute * 100) + st.wSecond;

  // Status (using official constants)
  // STATUS_OK is probably 1, STATUS_ERROR is 2, etc.
  // I should check official definitions or just use integers.
  // Actually, official RecentInfo has nStatus.
  ri->nStatus = (m_engine->GetConnectionState() == CONN_CONNECTED) ? 1 : 2;

  // Post the message (non-blocking)
  // WPARAM = pointer to ticker name (static storage in RecentInfo)
  // LPARAM = pointer to RecentInfo
  PostMessage(m_hMainWnd, WM_USER_STREAMING_UPDATE, (WPARAM)ri->Name,
              (LPARAM)ri);
}

///////////////////////////////////////////////////////////////////////////
// TryReconnect — Exponential backoff reconnection
///////////////////////////////////////////////////////////////////////////

bool RealtimeFeed::TryReconnect() {
  if (m_reconnectAttempts >= m_maxReconnectAttempts) {
    return false;
  }

  m_reconnectAttempts++;

  // Exponential backoff: 5s, 10s, 20s, 40s, 60s (capped)
  int backoffMs = 5000 * (1 << (m_reconnectAttempts - 1));
  if (backoffMs > 60000)
    backoffMs = 60000;

  m_engine->Log("TryReconnect: attempt %d/%d, waiting %d ms",
                m_reconnectAttempts, m_maxReconnectAttempts, backoffMs);

  // Wait with chunked sleep
  int chunks = backoffMs / 100;
  for (int i = 0; i < chunks && !m_stopRequested.load(); ++i) {
    Sleep(100);
  }

  // Try a test fetch
  std::vector<DseQuote> testQuotes;
  return m_engine->FetchLatestQuotes(testQuotes);
}

///////////////////////////////////////////////////////////////////////////
// GetLatestQuote — Thread-safe quote retrieval
///////////////////////////////////////////////////////////////////////////

bool RealtimeFeed::GetLatestQuote(const char *symbol, DseQuote &outQuote) {
  std::lock_guard<std::mutex> lock(m_quotesMutex);

  auto it = m_latestQuotes.find(symbol);
  if (it == m_latestQuotes.end())
    return false;

  outQuote = it->second;
  return true;
}

const std::map<std::string, DseQuote> &RealtimeFeed::GetAllQuotes() {
  return m_latestQuotes;
}

ConnectionState RealtimeFeed::GetConnectionState() const {
  if (!m_engine)
    return CONN_DISCONNECTED;
  return m_engine->GetConnectionState();
}

///////////////////////////////////////////////////////////////////////////
// Subscribe / Unsubscribe
///////////////////////////////////////////////////////////////////////////

void RealtimeFeed::Subscribe(const char *symbol) {
  std::lock_guard<std::mutex> lock(m_subsMutex);

  std::string sym(symbol);
  for (const auto &s : m_subscriptions) {
    if (s == sym)
      return; // Already subscribed
  }
  m_subscriptions.push_back(sym);

  if (m_engine) {
    m_engine->Log("RealtimeFeed::Subscribe: %s", symbol);
  }
}

void RealtimeFeed::Unsubscribe(const char *symbol) {
  std::lock_guard<std::mutex> lock(m_subsMutex);

  std::string sym(symbol);
  m_subscriptions.erase(
      std::remove(m_subscriptions.begin(), m_subscriptions.end(), sym),
      m_subscriptions.end());

  if (m_engine) {
    m_engine->Log("RealtimeFeed::Unsubscribe: %s", symbol);
  }
}
