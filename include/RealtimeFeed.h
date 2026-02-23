///////////////////////////////////////////////////////////////////////////
// RealtimeFeed.h — Background Polling Thread for Live DSE Data
//
// [Explanation for non-developers]:
// This is simply the "blueprint" for the RealtimeFeed (the background
// downloader). It defines how the background timer works and what controls we
// have over it (Start, Stop, Reconnect).
//
// Polls DSE during market hours and pushes updates to AmiBroker
// via WM_USER_STREAMING_UPDATE messages.
///////////////////////////////////////////////////////////////////////////

#ifndef REALTIME_FEED_H
#define REALTIME_FEED_H

#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <windows.h>


#include "DseDataEngine.h"

///////////////////////////////////////////////////////////////////////////
// RealtimeFeed — Manages the background polling thread
///////////////////////////////////////////////////////////////////////////
class RealtimeFeed {
public:
  RealtimeFeed();
  ~RealtimeFeed();

  // Start the polling thread
  // hMainWnd: AmiBroker main window to receive updates
  // engine:   shared DseDataEngine instance
  bool Start(HWND hMainWnd, DseDataEngine *engine);

  // Stop the polling thread
  void Stop();

  // Check if the feed is running
  bool IsRunning() const { return m_running.load(); }

  // Get the latest quote for a specific symbol
  bool GetLatestQuote(const char *symbol, DseQuote &outQuote);

  // Get all latest quotes
  const std::map<std::string, DseQuote> &GetAllQuotes();

  // Get connection state
  ConnectionState GetConnectionState() const;

  // Set poll interval (milliseconds)
  void SetPollInterval(int ms) { m_pollIntervalMs = ms; }

  // Subscribe to a specific symbol (prioritize it in updates)
  void Subscribe(const char *symbol);

  // Unsubscribe from a symbol
  void Unsubscribe(const char *symbol);

private:
  // Thread function
  static DWORD WINAPI ThreadProc(LPVOID lpParam);

  // Main polling loop
  void PollLoop();

  // Send update to AmiBroker
  void SendStreamingUpdate(const DseQuote &quote);

  // Reconnect with exponential backoff
  bool TryReconnect();

  // ─── Members ───────────────────────────────────────────

  HWND m_hMainWnd;                   // AmiBroker window
  DseDataEngine *m_engine;           // Shared data engine
  HANDLE m_hThread;                  // Worker thread handle
  std::atomic<bool> m_running;       // Thread running flag
  std::atomic<bool> m_stopRequested; // Stop signal

  int m_pollIntervalMs; // Poll interval
  int m_reconnectAttempts;
  int m_maxReconnectAttempts;

  // Latest quotes cache (symbol -> quote)
  std::map<std::string, DseQuote> m_latestQuotes;
  mutable std::mutex m_quotesMutex;

  // Subscribed symbols (for priority polling)
  std::vector<std::string> m_subscriptions;
  std::mutex m_subsMutex;
};

#endif // REALTIME_FEED_H
