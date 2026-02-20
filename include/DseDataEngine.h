///////////////////////////////////////////////////////////////////////////
// DseDataEngine.h — DSE HTTP Client & Data Parser
//
// Replicates bdshare's scraping logic in native C++.
// Uses WinInet for HTTP, parses HTML tables from dsebd.org.
///////////////////////////////////////////////////////////////////////////

#ifndef DSE_DATA_ENGINE_H
#define DSE_DATA_ENGINE_H

#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <windows.h>
#include <wininet.h>

///////////////////////////////////////////////////////////////////////////
// Data structures for parsed DSE data
///////////////////////////////////////////////////////////////////////////

// A single OHLCV bar parsed from DSE
struct DseBar {
  int year, month, day;
  double open, high, low, close;
  double volume;
  double trade; // number of trades
  double value; // turnover value
  bool valid;   // false if bad tick
};

// Real-time quote for a single instrument
struct DseQuote {
  char symbol[32];
  double ltp; // Last Traded Price
  double high;
  double low;
  double open;
  double close; // previous close / closing price
  double ycp;   // yesterday's closing price
  double change;
  double changePercent;
  double volume;
  double trade;
  double value;
  bool valid;
};

// Configuration loaded from INI
struct DseConfig {
  int historyYears;
  int pollIntervalMs;
  int marketOpenHour, marketOpenMinute;
  int marketCloseHour, marketCloseMinute;
  int maxReconnectAttempts;
  int httpTimeoutSec;
  char userAgent[256];
  bool preferWebData; // New: Priority for Web vs Local data
  char latestPriceUrl[512];
  char dayEndArchiveUrl[512];
  char altLatestPriceUrl[512];
  bool enableLogging;
  char logFilePath[260];
  char csvSeedPath[512]; // Folder with per-symbol CSV seed files (1999–present)
  char exportPath[MAX_PATH]; // New: CSV Export Path
  int exportIntervalSec;     // New: Auto-export interval (0=disabled)
};

///////////////////////////////////////////////////////////////////////////
// Connection state
///////////////////////////////////////////////////////////////////////////
enum ConnectionState {
  CONN_DISCONNECTED = 0,
  CONN_CONNECTING = 1,
  CONN_CONNECTED = 2,
  CONN_RECONNECTING = 3,
  CONN_ERROR = 4
};

///////////////////////////////////////////////////////////////////////////
// DseDataEngine — Main data retrieval and parsing class
///////////////////////////////////////////////////////////////////////////
class DseDataEngine {
public:
  DseDataEngine();
  ~DseDataEngine();

  // Initialize the engine (load config, open WinInet handle)
  bool Initialize(const char *configPath);

  // Shutdown and cleanup
  void Shutdown();

  // ─── Historical Data ───────────────────────────────────

  // Fetch historical OHLCV bars for a symbol
  // startDate/endDate format: "YYYY-MM-DD"
  bool FetchHistoricalData(const char *symbol, const char *startDate,
                           const char *endDate, std::vector<DseBar> &outBars);

  // Get cached historical bars for a symbol
  // Get cached historical bars for a symbol
  bool GetCachedBars(const char *symbol, std::vector<DseBar> &outBars);

  // Export
  void ExportAllDataToCsv();
  void CheckAutoExport();

  // ─── Real-Time Data ────────────────────────────────────

  // Fetch latest quotes for all instruments
  bool FetchLatestQuotes(std::vector<DseQuote> &outQuotes);

  // Fetch latest quote for a specific symbol
  bool FetchLatestQuote(const char *symbol, DseQuote &outQuote);

  // ─── Symbol Management ─────────────────────────────────

  // Get list of all known DSE symbols
  const std::vector<std::string> &GetSymbolList();

  // Refresh symbol list from DSE
  bool RefreshSymbolList();

  // ─── State ─────────────────────────────────────────────

  ConnectionState GetConnectionState() const { return m_connState; }
  const DseConfig &GetConfig() const { return m_config; }

  // Check if currently within DSE market hours
  bool IsMarketOpen() const;

  // Log a message (if logging enabled)
  void Log(const char *fmt, ...);

private:
  // ─── HTTP Layer ────────────────────────────────────────

  // Perform HTTP GET and return response body as string
  bool HttpGet(const char *url, std::string &outBody);

  // Build full URL with query parameters
  std::string BuildHistoryUrl(const char *symbol, const char *startDate,
                              const char *endDate);

  // ─── Parsing Layer ─────────────────────────────────────

  // Parse historical data HTML table into bars
  bool ParseHistoricalHtml(const std::string &html,
                           std::vector<DseBar> &outBars);

  // Parse latest share price HTML table into quotes
  bool ParseLatestPriceHtml(const std::string &html,
                            std::vector<DseQuote> &outQuotes);

  // Parse a single HTML table row into cells
  std::vector<std::string> ParseTableRow(const std::string &row);

  // Extract specific table by class or content
  std::string ExtractTargetTable(const std::string &html);

  // Extract all <tr> blocks from HTML
  std::vector<std::string> ExtractTableRows(const std::string &html);

  // ─── Utility ───────────────────────────────────────────

  // Strip HTML tags from a string
  static std::string StripHtml(const std::string &input);

  // Trim whitespace
  static std::string Trim(const std::string &s);

  // Safe string to double
  static double SafeStod(const std::string &s, double fallback = 0.0);

  // Validate a bar (bad tick filtering)
  static bool ValidateBar(const DseBar &bar);

  // Load configuration from INI file
  bool LoadConfig(const char *path);

  // Load historical bars from a CSV seed file (Trading_Code,Date,O,H,L,C,V)
  bool LoadCsvSeed(const char *symbol, std::vector<DseBar> &outBars);

  // ─── Members ───────────────────────────────────────────

  HINTERNET m_hInternet; // WinInet session
  HINTERNET m_hConnect;  // WinInet connection
  DseConfig m_config;
  ConnectionState m_connState;

  // Symbol list
  std::vector<std::string> m_symbols;

  // Cache: symbol -> bars
  std::map<std::string, std::vector<DseBar>> m_cache;

  // Thread safety
  mutable std::mutex m_mutex;

  // Logging
  FILE *m_logFile;

  // Auto-export timer
  time_t m_lastExportTime;
};

#endif // DSE_DATA_ENGINE_H
