#include "RealtimeFeed.h"
#include "Plugin.h"
#include <cstring>
#include <unordered_set>
#include <windows.h>

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

RealtimeFeed::RealtimeFeed()
    : m_hMainWnd(NULL), m_engine(nullptr), m_hThread(NULL),
      m_hStopEvent(NULL), m_running(false), m_stopRequested(false),
      m_pollIntervalMs(5000), m_reconnectAttempts(0),
      m_maxReconnectAttempts(10) {
  m_hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

RealtimeFeed::~RealtimeFeed() {
  Stop();
  if (m_hStopEvent) {
    CloseHandle(m_hStopEvent);
    m_hStopEvent = NULL;
  }
}

// ---------------------------------------------------------------------------
// Start / Stop
// ---------------------------------------------------------------------------

bool RealtimeFeed::Start(HWND hMainWnd, DseDataEngine *engine) {
  if (m_running.load())
    return true;

  m_hMainWnd = hMainWnd;
  m_engine = engine;
  m_stopRequested = false;
  if (m_hStopEvent)
    ResetEvent(m_hStopEvent);
  m_reconnectAttempts = 0;

  if (engine) {
    m_pollIntervalMs = engine->GetConfig().pollIntervalMs;
    m_maxReconnectAttempts = engine->GetConfig().maxReconnectAttempts;
  }

  m_hThread = CreateThread(NULL, 0, ThreadProc, this, 0, NULL);
  if (!m_hThread) {
    if (m_engine)
      m_engine->Log("ERROR: RealtimeFeed::Start — CreateThread failed");
    return false;
  }

  m_running = true;
  if (m_engine)
    m_engine->Log("RealtimeFeed::Start — polling thread started");
  return true;
}

void RealtimeFeed::Stop() {
  if (!m_running.load())
    return;

  m_stopRequested = true;
  if (m_hStopEvent)
    SetEvent(m_hStopEvent);

  if (m_hThread) {
    DWORD result = WaitForSingleObject(m_hThread, 5000);
    if (result == WAIT_TIMEOUT && m_engine) {
      m_engine->Log("WARNING: RealtimeFeed::Stop — thread wait timed out");
    }
    CloseHandle(m_hThread);
    m_hThread = NULL;
  }

  m_running = false;
  if (m_engine)
    m_engine->Log("RealtimeFeed::Stop — polling thread stopped");
}

// ---------------------------------------------------------------------------
// Thread Entry
// ---------------------------------------------------------------------------

DWORD WINAPI RealtimeFeed::ThreadProc(LPVOID lpParam) {
  static_cast<RealtimeFeed *>(lpParam)->PollLoop();
  return 0;
}

// ---------------------------------------------------------------------------
// Poll Loop
// ---------------------------------------------------------------------------

void RealtimeFeed::PollLoop() {
  if (m_engine)
    m_engine->Log("PollLoop: entering");

  while (!m_stopRequested.load()) {

    if (m_engine && m_engine->IsMarketOpen()) {
      std::vector<DseQuote> quotes;
      bool ok = m_engine->FetchLatestQuotes(quotes);

      if (ok) {
        m_reconnectAttempts = 0;

        // Take snapshot of subscriptions
        std::vector<std::string> subs;
        {
          std::lock_guard<std::mutex> subLock(m_subsMutex);
          subs = m_subscriptions;
        }

        std::unordered_set<std::string> sentSymbols;

        // Update quote cache under lock and send quotes
        {
          std::lock_guard<std::mutex> lock(m_quotesMutex);
          for (const auto &q : quotes)
            m_latestQuotes[q.symbol] = q;

          // Push subscribed symbols first (priority)
          for (const auto &sym : subs) {
            auto it = m_latestQuotes.find(sym);
            if (it != m_latestQuotes.end()) {
              SendStreamingUpdate(it->second);
              sentSymbols.insert(sym);
            }
          }

          // Push remaining symbols without duplicate sending
          for (const auto &q : quotes) {
            if (sentSymbols.find(q.symbol) == sentSymbols.end()) {
              SendStreamingUpdate(q);
              sentSymbols.insert(q.symbol);
            }
          }
        }

        if (m_engine)
          m_engine->Log("PollLoop: pushed %zu unique quotes", sentSymbols.size());

      } else {
        if (m_engine)
          m_engine->Log("PollLoop: fetch failed, attempting reconnect");
        if (!TryReconnect()) {
          if (m_engine)
            m_engine->Log("PollLoop: max reconnects reached, sleeping 60s");
          WaitForSingleObject(m_hStopEvent, 60000);
          m_reconnectAttempts = 0;
          continue;
        }
      }

      // Check auto-export if interval is configured
      if (m_engine)
        m_engine->CheckAutoExport();

      // Event-driven sleep for poll interval (instantly wakeable on stop)
      WaitForSingleObject(m_hStopEvent, m_pollIntervalMs);

    } else {
      // Market is closed — check auto export and wait up to 60s
      if (m_engine) {
        m_engine->CheckAutoExport();
        m_engine->Log("PollLoop: market closed, sleeping 60s");
      }
      WaitForSingleObject(m_hStopEvent, 60000);
    }
  }

  if (m_engine)
    m_engine->Log("PollLoop: exiting");
}

// ---------------------------------------------------------------------------
// Streaming Update
// ---------------------------------------------------------------------------

// Packages a DseQuote into a RecentInfo and posts it to the AmiBroker window.
// AmiBroker reads the struct asynchronously, so it is heap-allocated here and
// AmiBroker is responsible for freeing it upon receipt.
// If PostMessage fails, it is deleted immediately to prevent leaks.
void RealtimeFeed::SendStreamingUpdate(const DseQuote &quote) {
  if (!m_hMainWnd || !IsWindow(m_hMainWnd))
    return;

  RecentInfo *ri = new RecentInfo;
  memset(ri, 0, sizeof(RecentInfo));

  ri->nStructSize = sizeof(RecentInfo);
  strncpy_s(ri->Name, quote.symbol, sizeof(ri->Name) - 1);

  ri->fOpen = (float)quote.open;
  ri->fHigh = (float)quote.high;
  ri->fLow = (float)quote.low;
  ri->fLast = (float)quote.ltp;
  ri->fPrev = (float)quote.ycp;
  ri->iTotalVol = (int)quote.volume;
  ri->fChange = (float)quote.change;
  ri->iTradeVol = (int)quote.trade;
  ri->nBitmap = 0xFFFF; // all fields valid

  SYSTEMTIME st;
  GetLocalTime(&st);
  ri->nDateUpdate = (st.wYear * 10000) + (st.wMonth * 100) + st.wDay;
  ri->nTimeUpdate = (st.wHour * 10000) + (st.wMinute * 100) + st.wSecond;
  ri->nStatus = (m_engine && m_engine->GetConnectionState() == CONN_CONNECTED) ? 1 : 2;

  if (!PostMessage(m_hMainWnd, WM_USER_STREAMING_UPDATE, (WPARAM)ri->Name,
                   (LPARAM)ri)) {
    delete ri; // Prevent memory leak if message could not be posted
  }
}

// ---------------------------------------------------------------------------
// Reconnect
// ---------------------------------------------------------------------------

// Exponential backoff: 5 s, 10 s, 20 s, ... capped at 60 s.
bool RealtimeFeed::TryReconnect() {
  if (!m_engine || m_reconnectAttempts >= m_maxReconnectAttempts)
    return false;

  ++m_reconnectAttempts;
  int backoffMs = 5000 * (1 << (m_reconnectAttempts - 1));
  if (backoffMs > 60000)
    backoffMs = 60000;

  m_engine->Log("TryReconnect: attempt %d/%d, waiting %d ms",
                m_reconnectAttempts, m_maxReconnectAttempts, backoffMs);

  DWORD waitRes = WaitForSingleObject(m_hStopEvent, backoffMs);
  if (waitRes == WAIT_OBJECT_0 || m_stopRequested.load())
    return false;

  std::vector<DseQuote> test;
  return m_engine->FetchLatestQuotes(test);
}

// ---------------------------------------------------------------------------
// Quote Access
// ---------------------------------------------------------------------------

bool RealtimeFeed::GetLatestQuote(const char *symbol, DseQuote &outQuote) {
  std::lock_guard<std::mutex> lock(m_quotesMutex);
  auto it = m_latestQuotes.find(symbol);
  if (it == m_latestQuotes.end())
    return false;
  outQuote = it->second;
  return true;
}

std::map<std::string, DseQuote> RealtimeFeed::GetAllQuotes() {
  std::lock_guard<std::mutex> lock(m_quotesMutex);
  return m_latestQuotes;
}

ConnectionState RealtimeFeed::GetConnectionState() const {
  return m_engine ? m_engine->GetConnectionState() : CONN_DISCONNECTED;
}

// ---------------------------------------------------------------------------
// Subscribe / Unsubscribe
// ---------------------------------------------------------------------------

void RealtimeFeed::Subscribe(const char *symbol) {
  std::lock_guard<std::mutex> lock(m_subsMutex);
  std::string sym(symbol);
  for (const auto &s : m_subscriptions)
    if (s == sym)
      return;
  m_subscriptions.push_back(sym);
  if (m_engine)
    m_engine->Log("RealtimeFeed::Subscribe: %s", symbol);
}

void RealtimeFeed::Unsubscribe(const char *symbol) {
  std::lock_guard<std::mutex> lock(m_subsMutex);
  std::string sym(symbol);
  m_subscriptions.erase(
      std::remove(m_subscriptions.begin(), m_subscriptions.end(), sym),
      m_subscriptions.end());
  if (m_engine)
    m_engine->Log("RealtimeFeed::Unsubscribe: %s", symbol);
}
