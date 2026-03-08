// DseDataEngine.h — DSE HTTP Client & Data Parser
//
// Downloads and caches OHLCV bars (historical) and live quotes (real-time)
// from dsebd.org and amarstock.com. HTML parsing delegates to HtmlUtils;
// CSV I/O delegates to CsvUtils.

#ifndef DSE_DATA_ENGINE_H
#define DSE_DATA_ENGINE_H

#include "CsvUtils.h"
#include "DseTypes.h"
#include "HtmlUtils.h"
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <windows.h>
#include <wininet.h>


class DseDataEngine {
public:
  DseDataEngine();
  ~DseDataEngine();

  // Load config from INI and open a WinInet session.
  // Safe to call multiple times (reloads config on each call).
  bool Initialize(const char *configPath);

  // Release WinInet handles and close log file.
  void Shutdown();

  // ── Historical Data ──────────────────────────────────────────────────────

  // Fetch OHLCV bars for [startDate, endDate] ("YYYY-MM-DD").
  // Merges CSV seed + web data, caches result, calls onProgress when done.
  bool FetchHistoricalData(const char *symbol, const char *startDate,
                           const char *endDate, std::vector<DseBar> &outBars,
                           std::function<void()> onProgress = nullptr);

  // Return cached bars for a symbol (populated by FetchHistoricalData).
  bool GetCachedBars(const char *symbol, std::vector<DseBar> &outBars);

  // Export all cached bars to individual CSV files in exportPath.
  void ExportAllDataToCsv();

  // Trigger ExportAllDataToCsv if exportIntervalSec has elapsed.
  void CheckAutoExport();

  // ── Real-Time Data ───────────────────────────────────────────────────────

  // Fetch live quotes for every symbol from the DSE latest-price page.
  bool FetchLatestQuotes(std::vector<DseQuote> &outQuotes);

  // Fetch live quote for a single symbol (fetches all, then filters).
  bool FetchLatestQuote(const char *symbol, DseQuote &outQuote);

  // ── Symbol Management ────────────────────────────────────────────────────

  // Return cached symbol list; refreshes from DSE if empty.
  const std::vector<std::string> &GetSymbolList();

  // Re-scrape the DSE live-price page to rebuild the symbol list.
  bool RefreshSymbolList();

  // ── State ────────────────────────────────────────────────────────────────

  ConnectionState GetConnectionState() const { return m_connState; }
  const DseConfig &GetConfig() const { return m_config; }

  // Returns true if now falls within configured market hours (Sun–Thu).
  bool IsMarketOpen() const;

  // Append a timestamped log line (no-op when logging is disabled).
  void Log(const char *fmt, ...);

private:
  // ── HTTP Layer ───────────────────────────────────────────────────────────

  bool HttpGet(const char *url, std::string &outBody);
  bool HttpPost(const char *url, const char *payload, std::string &outBody);

  // Build day_end_archive.php query URL.
  std::string BuildHistoryUrl(const char *symbol, const char *startDate,
                              const char *endDate);

  // ── Parsing ──────────────────────────────────────────────────────────────

  // Parse the day_end_archive HTML page into bars.
  bool ParseHistoricalHtml(const std::string &html,
                           std::vector<DseBar> &outBars);

  // Parse the latest_share_price HTML page into quotes.
  bool ParseLatestPriceHtml(const std::string &html,
                            std::vector<DseQuote> &outQuotes);

  // ── Amarstock Indices ────────────────────────────────────────────────────

  bool IsAmarstockIndex(const char *symbol);

  // Fetch index bars day-by-day from amarstock.com/data/download/CSV.
  bool FetchAmarstockIndexData(const char *symbol, const char *startDate,
                               const char *endDate,
                               std::vector<DseBar> &outBars,
                               std::function<void()> onProgress);

  // ── Config ───────────────────────────────────────────────────────────────

  bool LoadConfig(const char *path);

  // ── Members ──────────────────────────────────────────────────────────────

  HINTERNET m_hInternet;
  HINTERNET m_hConnect;
  DseConfig m_config;
  ConnectionState m_connState;

  std::vector<std::string> m_symbols;
  std::map<std::string, std::vector<DseBar>> m_cache;

  mutable std::mutex m_mutex;

  FILE *m_logFile;
  time_t m_lastExportTime;
};

#endif // DSE_DATA_ENGINE_H
