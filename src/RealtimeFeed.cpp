// RealtimeFeed.cpp — Background Polling Thread
//
// Runs a background thread that polls FetchLatestQuotes() at the configured
// interval during market hours and pushes each update to AmiBroker via
// PostMessage(WM_USER_STREAMING_UPDATE).

#include "RealtimeFeed.h"
#include "Plugin.h"
#include <cstring>
#include <windows.h>

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

RealtimeFeed::RealtimeFeed()
    : m_hMainWnd(NULL), m_engine(nullptr), m_hThread(NULL), m_running(false),
      m_stopRequested(false), m_pollIntervalMs(5000), m_reconnectAttempts(0),
      m_maxReconnectAttempts(10) {}

RealtimeFeed::~RealtimeFeed() { Stop(); }

// ---------------------------------------------------------------------------
// Start / Stop
// ---------------------------------------------------------------------------

bool RealtimeFeed::Start(HWND hMainWnd, DseDataEngine *engine) {
  if (m_running.load())
    return true;

  m_hMainWnd = hMainWnd;
  m_engine = engine;
  m_stopRequested = false;
  m_reconnectAttempts = 0;

  if (engine) {
    m_pollIntervalMs = engine->GetConfig().pollIntervalMs;
    m_maxReconnectAttempts = engine->GetConfig().maxReconnectAttempts;
  }

  m_hThread = CreateThread(NULL, 0, ThreadProc, this, 0, NULL);
  if (!m_hThread) {
    m_engine->Log("ERROR: RealtimeFeed::Start — CreateThread failed");
    return false;
  }

  m_running = true;
  m_engine->Log("RealtimeFeed::Start — polling thread started");
  return true;
}

void RealtimeFeed::Stop() {
  if (!m_running.load())
    return;

  m_stopRequested = true;

  if (m_hThread) {
    DWORD result = WaitForSingleObject(m_hThread, 10000);
    if (result == WAIT_TIMEOUT)
      TerminateThread(m_hThread, 0);
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
  m_engine->Log("PollLoop: entering");

  while (!m_stopRequested.load()) {

    if (m_engine->IsMarketOpen()) {
      std::vector<DseQuote> quotes;
      bool ok = m_engine->FetchLatestQuotes(quotes);

      if (ok) {
        m_reconnectAttempts = 0;

        // Update quote cache
        {
          std::lock_guard<std::mutex> lock(m_quotesMutex);
          for (const auto &q : quotes)
            m_latestQuotes[q.symbol] = q;
        }

        // Push subscribed symbols first (priority), then all others
        {
          std::lock_guard<std::mutex> subLock(m_subsMutex);
          for (const auto &sym : m_subscriptions) {
            auto it = m_latestQuotes.find(sym);
            if (it != m_latestQuotes.end())
              SendStreamingUpdate(it->second);
          }
        }
        for (const auto &q : quotes)
          SendStreamingUpdate(q);

        m_engine->Log("PollLoop: pushed %zu quotes", quotes.size());

      } else {
        m_engine->Log("PollLoop: fetch failed, attempting reconnect");
        if (!TryReconnect()) {
          m_engine->Log("PollLoop: max reconnects reached, sleeping 60s");
          for (int i = 0; i < 60 && !m_stopRequested.load(); ++i)
            Sleep(1000);
          m_reconnectAttempts = 0;
          continue;
        }
      }

      // Sleep for poll interval in 100 ms chunks (responsive to stop signal)
      int chunks = m_pollIntervalMs / 100;
      for (int i = 0; i < chunks && !m_stopRequested.load(); ++i)
        Sleep(100);

    } else {
      // Market is closed — check again in 60 s
      m_engine->Log("PollLoop: market closed, sleeping 60s");
      for (int i = 0; i < 60 && !m_stopRequested.load(); ++i)
        Sleep(1000);
    }
  }

  m_engine->Log("PollLoop: exiting");
}

// ---------------------------------------------------------------------------
// Streaming Update
// ---------------------------------------------------------------------------

// Packages a DseQuote into a RecentInfo and posts it to the AmiBroker window.
// AmiBroker reads the struct asynchronously, so it is heap-allocated here and
// AmiBroker is responsible for freeing it.
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
  ri->nStatus = (m_engine->GetConnectionState() == CONN_CONNECTED) ? 1 : 2;

  PostMessage(m_hMainWnd, WM_USER_STREAMING_UPDATE, (WPARAM)ri->Name,
              (LPARAM)ri);
}

// ---------------------------------------------------------------------------
// Reconnect
// ---------------------------------------------------------------------------

// Exponential backoff: 5 s, 10 s, 20 s, ... capped at 60 s.
bool RealtimeFeed::TryReconnect() {
  if (m_reconnectAttempts >= m_maxReconnectAttempts)
    return false;

  ++m_reconnectAttempts;
  int backoffMs = 5000 * (1 << (m_reconnectAttempts - 1));
  if (backoffMs > 60000)
    backoffMs = 60000;

  m_engine->Log("TryReconnect: attempt %d/%d, waiting %d ms",
                m_reconnectAttempts, m_maxReconnectAttempts, backoffMs);

  int chunks = backoffMs / 100;
  for (int i = 0; i < chunks && !m_stopRequested.load(); ++i)
    Sleep(100);

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

const std::map<std::string, DseQuote> &RealtimeFeed::GetAllQuotes() {
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
