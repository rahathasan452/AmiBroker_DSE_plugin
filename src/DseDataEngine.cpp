// DseDataEngine.cpp — DSE HTTP Client & Data Parser
//
// Fetches historical OHLCV bars and real-time quotes from dsebd.org and
// amarstock.com. Parses HTML tables using HtmlUtils, loads/exports CSV
// data via CsvUtils, and caches results per symbol.

#include "DseDataEngine.h"
#include "CsvUtils.h"
#include "HtmlUtils.h"
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <wininet.h>

#undef min
#undef max

// ---------------------------------------------------------------------------
// File-scope helpers
// ---------------------------------------------------------------------------

// Validates a parsed DseBar — rejects zero prices, bad dates, and inverted H/L.
static bool ValidateBar(const DseBar &bar) {
  const double MIN = 0.01;
  return bar.open >= MIN && bar.high >= MIN && bar.low >= MIN &&
         bar.close >= MIN && bar.high >= bar.low && bar.year >= 1990 &&
         bar.year <= 2100 && bar.month >= 1 && bar.month <= 12 &&
         bar.day >= 1 && bar.day <= 31;
}

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

DseDataEngine::DseDataEngine()
    : m_hInternet(NULL), m_hConnect(NULL), m_connState(CONN_DISCONNECTED),
      m_logFile(NULL) {
  memset(&m_config, 0, sizeof(m_config));
}

DseDataEngine::~DseDataEngine() { Shutdown(); }

// ---------------------------------------------------------------------------
// Initialize / Shutdown
// ---------------------------------------------------------------------------

bool DseDataEngine::Initialize(const char *configPath) {
  std::lock_guard<std::mutex> lock(m_mutex);

  if (!LoadConfig(configPath)) {
    // Apply built-in defaults when no config file is present
    m_config.historyDays = 365;
    m_config.pollIntervalMs = 5000;
    m_config.marketOpenHour = 10;
    m_config.marketOpenMinute = 0;
    m_config.marketCloseHour = 14;
    m_config.marketCloseMinute = 30;
    m_config.maxReconnectAttempts = 10;
    m_config.httpTimeoutSec = 30;
    strcpy_s(m_config.userAgent,
             "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    strcpy_s(m_config.latestPriceUrl,
             "https://www.dsebd.org/latest_share_price_scroll_l.php");
    strcpy_s(m_config.dayEndArchiveUrl,
             "https://www.dsebd.org/day_end_archive.php");
    strcpy_s(m_config.altLatestPriceUrl,
             "https://www.dsebd.org/latest_share_price_all_,ajax.php");
    m_config.enableLogging = true;
  }

  // Force logging on for this debug build
  m_config.enableLogging = true;

  if (m_config.enableLogging && m_config.logFilePath[0])
    fopen_s(&m_logFile, m_config.logFilePath, "a");

  m_lastExportTime = time(NULL);
  Log("DseDataEngine::Initialize — starting");

  m_hInternet = InternetOpenA(m_config.userAgent, INTERNET_OPEN_TYPE_PRECONFIG,
                              NULL, NULL, 0);
  if (!m_hInternet) {
    Log("ERROR: InternetOpen failed, error=%lu", GetLastError());
    m_connState = CONN_ERROR;
    return false;
  }

  DWORD timeout = m_config.httpTimeoutSec * 1000;
  InternetSetOptionA(m_hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout,
                     sizeof(timeout));
  InternetSetOptionA(m_hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout,
                     sizeof(timeout));
  InternetSetOptionA(m_hInternet, INTERNET_OPTION_SEND_TIMEOUT, &timeout,
                     sizeof(timeout));

  m_connState = CONN_CONNECTED;
  Log("DseDataEngine::Initialize — OK");
  return true;
}

void DseDataEngine::Shutdown() {
  std::lock_guard<std::mutex> lock(m_mutex);
  Log("DseDataEngine::Shutdown");

  if (m_hConnect) {
    InternetCloseHandle(m_hConnect);
    m_hConnect = NULL;
  }
  if (m_hInternet) {
    InternetCloseHandle(m_hInternet);
    m_hInternet = NULL;
  }

  m_connState = CONN_DISCONNECTED;

  if (m_logFile) {
    fclose(m_logFile);
    m_logFile = NULL;
  }
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

bool DseDataEngine::LoadConfig(const char *path) {
  if (!path || !path[0])
    return false;

  m_config.historyDays =
      GetPrivateProfileIntA("Settings", "HistoryDays", 365, path);
  m_config.pollIntervalMs =
      GetPrivateProfileIntA("General", "PollIntervalMs", 5000, path);
  m_config.marketOpenHour =
      GetPrivateProfileIntA("General", "MarketOpenHour", 10, path);
  m_config.marketOpenMinute =
      GetPrivateProfileIntA("General", "MarketOpenMinute", 0, path);
  m_config.marketCloseHour =
      GetPrivateProfileIntA("General", "MarketCloseHour", 14, path);
  m_config.marketCloseMinute =
      GetPrivateProfileIntA("General", "MarketCloseMinute", 30, path);
  m_config.maxReconnectAttempts =
      GetPrivateProfileIntA("General", "MaxReconnectAttempts", 10, path);
  m_config.httpTimeoutSec =
      GetPrivateProfileIntA("General", "HttpTimeoutSec", 30, path);

  GetPrivateProfileStringA(
      "General", "UserAgent",
      "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36",
      m_config.userAgent, sizeof(m_config.userAgent), path);

  m_config.preferWebData =
      (GetPrivateProfileIntA("General", "PreferWebData", 1, path) != 0);

  GetPrivateProfileStringA(
      "Endpoints", "LatestPrice",
      "https://www.dsebd.org/latest_share_price_scroll_l.php",
      m_config.latestPriceUrl, sizeof(m_config.latestPriceUrl), path);

  GetPrivateProfileStringA(
      "Endpoints", "DayEndArchive", "https://www.dsebd.org/day_end_archive.php",
      m_config.dayEndArchiveUrl, sizeof(m_config.dayEndArchiveUrl), path);

  GetPrivateProfileStringA(
      "Endpoints", "AltLatestPrice",
      "https://www.dsebd.org/latest_share_price_all_,ajax.php",
      m_config.altLatestPriceUrl, sizeof(m_config.altLatestPriceUrl), path);

  m_config.enableLogging =
      (GetPrivateProfileIntA("Debug", "EnableLogging", 0, path) != 0);

  GetPrivateProfileStringA("Debug", "LogFilePath", "dse_plugin.log",
                           m_config.logFilePath, sizeof(m_config.logFilePath),
                           path);

  GetPrivateProfileStringA("DataSource", "CsvSeedPath",
                           "d:\\software\\dse_2000-2025\\separated_data",
                           m_config.csvSeedPath, sizeof(m_config.csvSeedPath),
                           path);

  GetPrivateProfileStringA("Export", "ExportPath", "", m_config.exportPath,
                           sizeof(m_config.exportPath), path);

  m_config.exportIntervalSec =
      GetPrivateProfileIntA("Export", "ExportIntervalSec", 0, path);

  return true;
}

// ---------------------------------------------------------------------------
// HTTP Layer
// ---------------------------------------------------------------------------

bool DseDataEngine::HttpGet(const char *url, std::string &outBody) {
  if (!m_hInternet) {
    Log("ERROR: HttpGet — no WinInet session");
    return false;
  }

  Log("HttpGet: %s", url);

  // Try HTTPS first, fall back to plain HTTP on failure
  HINTERNET hUrl =
      InternetOpenUrlA(m_hInternet, url, NULL, 0,
                       INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE |
                           INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_SECURE,
                       0);

  if (!hUrl) {
    Log("WARNING: HttpGet HTTPS failed (err=%lu), retrying without SECURE",
        GetLastError());
    hUrl = InternetOpenUrlA(m_hInternet, url, NULL, 0,
                            INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE,
                            0);
    if (!hUrl) {
      m_connState = CONN_ERROR;
      return false;
    }
  }

  outBody.clear();
  char buffer[8192];
  DWORD bytesRead = 0;
  while (InternetReadFile(hUrl, buffer, sizeof(buffer) - 1, &bytesRead) &&
         bytesRead > 0) {
    buffer[bytesRead] = '\0';
    outBody.append(buffer, bytesRead);
    bytesRead = 0;
  }
  InternetCloseHandle(hUrl);

  if (outBody.empty()) {
    Log("WARNING: HttpGet — empty response");
    return false;
  }

  m_connState = CONN_CONNECTED;
  Log("HttpGet: received %zu bytes", outBody.size());
  return true;
}

bool DseDataEngine::HttpPost(const char *url, const char *payload,
                             std::string &outBody) {
  if (!m_hInternet) {
    Log("ERROR: HttpPost — no WinInet session");
    return false;
  }

  Log("HttpPost: %s (payload %zu bytes)", url, strlen(payload));

  URL_COMPONENTSA urlComp = {0};
  char host[256] = {0};
  char path[1024] = {0};
  urlComp.dwStructSize = sizeof(urlComp);
  urlComp.lpszHostName = host;
  urlComp.dwHostNameLength = sizeof(host);
  urlComp.lpszUrlPath = path;
  urlComp.dwUrlPathLength = sizeof(path);

  if (!InternetCrackUrlA(url, 0, 0, &urlComp)) {
    Log("ERROR: InternetCrackUrl failed, error=%lu", GetLastError());
    return false;
  }

  HINTERNET hConnect =
      InternetConnectA(m_hInternet, host, INTERNET_DEFAULT_HTTPS_PORT, NULL,
                       NULL, INTERNET_SERVICE_HTTP, 0, 0);
  if (!hConnect) {
    Log("ERROR: InternetConnect failed, error=%lu", GetLastError());
    return false;
  }

  const char *acceptTypes[] = {"*/*", NULL};
  HINTERNET hRequest =
      HttpOpenRequestA(hConnect, "POST", path, NULL, NULL, acceptTypes,
                       INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE |
                           INTERNET_FLAG_SECURE,
                       0);
  if (!hRequest) {
    Log("ERROR: HttpOpenRequest failed, error=%lu", GetLastError());
    InternetCloseHandle(hConnect);
    return false;
  }

  const char *headers = "Content-Type: application/x-www-form-urlencoded";
  DWORD headersLen = (DWORD)strlen(headers);
  if (!HttpSendRequestA(hRequest, headers, headersLen, (LPVOID)payload,
                        (DWORD)strlen(payload))) {
    Log("ERROR: HttpSendRequest failed, error=%lu", GetLastError());
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    return false;
  }

  outBody.clear();
  char buffer[8192];
  DWORD bytesRead = 0;
  while (InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) &&
         bytesRead > 0) {
    buffer[bytesRead] = '\0';
    outBody.append(buffer, bytesRead);
    bytesRead = 0;
  }

  InternetCloseHandle(hRequest);
  InternetCloseHandle(hConnect);

  if (outBody.empty()) {
    Log("WARNING: HttpPost — empty response");
    return false;
  }

  m_connState = CONN_CONNECTED;
  Log("HttpPost: received %zu bytes", outBody.size());
  return true;
}

// ---------------------------------------------------------------------------
// URL Builder
// ---------------------------------------------------------------------------

std::string DseDataEngine::BuildHistoryUrl(const char *symbol,
                                           const char *startDate,
                                           const char *endDate) {
  // Format:
  // day_end_archive.php?startDate=YYYY-MM-DD&endDate=YYYY-MM-DD&inst=SYM&archive=data
  std::string url = m_config.dayEndArchiveUrl;
  url += "?startDate=";
  url += startDate;
  url += "&endDate=";
  url += endDate;
  if (symbol && symbol[0]) {
    url += "&inst=";
    url += symbol;
  }
  url += "&archive=data";
  return url;
}

// ---------------------------------------------------------------------------
// HTML Parsers
// ---------------------------------------------------------------------------

bool DseDataEngine::ParseHistoricalHtml(const std::string &html,
                                        std::vector<DseBar> &outBars) {
  Log("ParseHistoricalHtml: input=%zu bytes", html.size());

  std::string tableHtml = HtmlUtils::ExtractTargetTable(html);
  auto rows = HtmlUtils::ExtractTableRows(tableHtml);
  Log("ParseHistoricalHtml: %zu rows extracted", rows.size());

  // Detect column indices from header row
  int colDate = -1, colOpen = -1, colHigh = -1, colLow = -1;
  int colClose = -1, colVol = -1, headerRow = -1;

  for (int r = 0; r < (int)rows.size() && r <= 300; ++r) {
    auto cells = HtmlUtils::ParseTableRow(rows[r]);
    if (cells.size() < 5)
      continue;

    for (int i = 0; i < (int)cells.size(); ++i) {
      std::string h = cells[i];
      std::transform(h.begin(), h.end(), h.begin(), ::toupper);

      if (h.find("DATE") != std::string::npos)
        colDate = i;
      else if (h.find("OPENP") != std::string::npos ||
               h.find("OPEN") != std::string::npos)
        colOpen = i;
      else if (h.find("HIGH") != std::string::npos)
        colHigh = i;
      else if (h.find("LOW") != std::string::npos)
        colLow = i;
      else if ((h.find("CLOSE") != std::string::npos ||
                h.find("LTP") != std::string::npos) &&
               h.find("YCP") == std::string::npos)
        colClose = i;
      else if (h.find("VOLUME") != std::string::npos ||
               h.find("VOL") != std::string::npos)
        colVol = i;
    }

    if (colDate != -1 && colClose != -1 && colVol != -1) {
      headerRow = r;
      Log("ParseHistoricalHtml: header at row %d "
          "(Date=%d Open=%d High=%d Low=%d Close=%d Vol=%d)",
          r, colDate, colOpen, colHigh, colLow, colClose, colVol);
      break;
    }
  }

  if (headerRow == -1) {
    // Fallback: assume known DSE column layout from observed pages
    Log("ParseHistoricalHtml: no header found, using fallback column indices");
    colDate = 1;
    colOpen = 6;
    colHigh = 4;
    colLow = 5;
    colClose = 7;
    colVol = 11;
    for (int r = 0; r < (int)rows.size() && r <= 300; ++r) {
      if ((int)HtmlUtils::ParseTableRow(rows[r]).size() > 11) {
        headerRow = r - 1;
        break;
      }
    }
  }

  if (headerRow == -1) {
    Log("ParseHistoricalHtml: could not find usable table structure");
    return false;
  }

  int validCount = 0;
  for (size_t r = (size_t)(headerRow + 1); r < rows.size(); ++r) {
    auto cells = HtmlUtils::ParseTableRow(rows[r]);

    int maxIdx = std::max({colDate, colClose, colVol});
    if ((int)cells.size() <= maxIdx)
      continue;

    const std::string &dateStr = cells[colDate];
    if (dateStr.size() < 10)
      continue;

    DseBar bar;
    memset(&bar, 0, sizeof(bar));
    bar.year = (int)HtmlUtils::SafeStod(dateStr.substr(0, 4));
    bar.month = (int)HtmlUtils::SafeStod(dateStr.substr(5, 2));
    bar.day = (int)HtmlUtils::SafeStod(dateStr.substr(8, 2));

    if (colOpen != -1 && colOpen < (int)cells.size())
      bar.open = HtmlUtils::SafeStod(cells[colOpen]);
    if (colHigh != -1 && colHigh < (int)cells.size())
      bar.high = HtmlUtils::SafeStod(cells[colHigh]);
    if (colLow != -1 && colLow < (int)cells.size())
      bar.low = HtmlUtils::SafeStod(cells[colLow]);
    if (colClose != -1 && colClose < (int)cells.size())
      bar.close = HtmlUtils::SafeStod(cells[colClose]);
    if (colVol != -1 && colVol < (int)cells.size())
      bar.volume = HtmlUtils::SafeStod(cells[colVol]);

    bar.valid = ValidateBar(bar);
    if (bar.valid) {
      outBars.push_back(bar);
      ++validCount;
    }
  }

  Log("ParseHistoricalHtml: %d valid bars", validCount);
  return !outBars.empty();
}

bool DseDataEngine::ParseLatestPriceHtml(const std::string &html,
                                         std::vector<DseQuote> &outQuotes) {
  // Expected columns: # | TRADING CODE | LTP | HIGH | LOW | CLOSE |
  //                   YCP | CHANGE | TRADE | VALUE(mn) | VOLUME
  auto rows = HtmlUtils::ExtractTableRows(html);
  if (rows.size() < 2) {
    Log("ParseLatestPriceHtml: too few rows (%zu)", rows.size());
    return false;
  }

  int colSymbol = -1, colLtp = -1, colHigh = -1, colLow = -1;
  int colClose = -1, colYcp = -1, colChange = -1;
  int colTrade = -1, colValue = -1, colVolume = -1;

  auto headerCells = HtmlUtils::ParseTableRow(rows[0]);
  for (int i = 0; i < (int)headerCells.size(); ++i) {
    std::string h = headerCells[i];
    for (auto &c : h)
      c = (char)toupper((unsigned char)c);

    if (h.find("TRADING") != std::string::npos)
      colSymbol = i;
    else if (h.find("LTP") != std::string::npos)
      colLtp = i;
    else if (h.find("HIGH") != std::string::npos)
      colHigh = i;
    else if (h.find("LOW") != std::string::npos)
      colLow = i;
    else if (h.find("CLOSE") != std::string::npos &&
             h.find("YCP") == std::string::npos)
      colClose = i;
    else if (h.find("YCP") != std::string::npos)
      colYcp = i;
    else if (h.find("CHANGE") != std::string::npos)
      colChange = i;
    else if (h.find("TRADE") != std::string::npos &&
             h.find("TRADING") == std::string::npos)
      colTrade = i;
    else if (h.find("VALUE") != std::string::npos)
      colValue = i;
    else if (h.find("VOLUME") != std::string::npos)
      colVolume = i;
  }

  // Fallback to observed column positions on dsebd.org
  if (colSymbol < 0)
    colSymbol = 1;
  if (colLtp < 0)
    colLtp = 2;
  if (colHigh < 0)
    colHigh = 3;
  if (colLow < 0)
    colLow = 4;
  if (colClose < 0)
    colClose = 5;
  if (colYcp < 0)
    colYcp = 6;
  if (colChange < 0)
    colChange = 7;
  if (colTrade < 0)
    colTrade = 8;
  if (colValue < 0)
    colValue = 9;
  if (colVolume < 0)
    colVolume = 10;

  for (size_t r = 1; r < rows.size(); ++r) {
    auto cells = HtmlUtils::ParseTableRow(rows[r]);
    if (cells.empty())
      continue;

    int maxCol = (int)cells.size();
    DseQuote q;
    memset(&q, 0, sizeof(q));
    q.valid = true;

    if (colSymbol < maxCol) {
      std::string sym = HtmlUtils::Trim(cells[colSymbol]);
      strncpy_s(q.symbol, sym.c_str(), sizeof(q.symbol) - 1);
    }

    if (colLtp < maxCol)
      q.ltp = HtmlUtils::SafeStod(cells[colLtp]);
    if (colHigh < maxCol)
      q.high = HtmlUtils::SafeStod(cells[colHigh]);
    if (colLow < maxCol)
      q.low = HtmlUtils::SafeStod(cells[colLow]);
    if (colClose < maxCol)
      q.close = HtmlUtils::SafeStod(cells[colClose]);
    if (colYcp < maxCol)
      q.ycp = HtmlUtils::SafeStod(cells[colYcp]);
    if (colChange < maxCol)
      q.change = HtmlUtils::SafeStod(cells[colChange]);
    if (colTrade < maxCol)
      q.trade = HtmlUtils::SafeStod(cells[colTrade]);
    if (colValue < maxCol)
      q.value = HtmlUtils::SafeStod(cells[colValue]);
    if (colVolume < maxCol)
      q.volume = HtmlUtils::SafeStod(cells[colVolume]);

    if (q.ycp > 0)
      q.changePercent = ((q.ltp - q.ycp) / q.ycp) * 100.0;
    q.open = q.ycp; // open defaults to previous close when not provided

    if (q.symbol[0] && q.ltp > 0)
      outQuotes.push_back(q);
  }

  Log("ParseLatestPriceHtml: parsed %zu quotes", outQuotes.size());
  return !outQuotes.empty();
}

// ---------------------------------------------------------------------------
// Historical Data
// ---------------------------------------------------------------------------

bool DseDataEngine::FetchHistoricalData(const char *symbol,
                                        const char *startDate,
                                        const char *endDate,
                                        std::vector<DseBar> &outBars,
                                        std::function<void()> onProgress) {
  Log("FetchHistoricalData: %s [%s -> %s]", symbol, startDate, endDate);
  outBars.clear();

  // Amarstock indices have a completely different data source
  if (IsAmarstockIndex(symbol))
    return FetchAmarstockIndexData(symbol, startDate, endDate, outBars,
                                   onProgress);

  // 1. Load local CSV seed
  std::vector<DseBar> seedBars;
  if (CsvUtils::LoadCsvSeed(symbol, m_config.csvSeedPath, seedBars)) {
    std::sort(seedBars.begin(), seedBars.end(),
              [](const DseBar &a, const DseBar &b) {
                if (a.year != b.year)
                  return a.year < b.year;
                if (a.month != b.month)
                  return a.month < b.month;
                return a.day < b.day;
              });
    Log("FetchHistoricalData: %zu bars from CSV seed for %s", seedBars.size(),
        symbol);
  }

  // 2. Fetch from web
  std::string url = BuildHistoryUrl(symbol, startDate, endDate);
  std::string html;
  std::vector<DseBar> webBars;
  bool webSuccess = false;

  if (HttpGet(url.c_str(), html)) {
    if (ParseHistoricalHtml(html, webBars)) {
      Log("FetchHistoricalData: %zu bars from web for %s", webBars.size(),
          symbol);
      webSuccess = true;
    } else {
      Log("WARNING: FetchHistoricalData — HTML parse failed for %s", symbol);
    }
  } else {
    Log("ERROR: FetchHistoricalData — HTTP failed for %s", symbol);
  }

  if (seedBars.empty() && !webSuccess)
    return false;

  // 3. Merge by date key, preferWebData controls which source wins on overlap
  std::map<int, DseBar> merged;
  if (m_config.preferWebData) {
    for (const auto &b : seedBars)
      merged[b.year * 10000 + b.month * 100 + b.day] = b;
    for (const auto &b : webBars)
      merged[b.year * 10000 + b.month * 100 + b.day] = b;
  } else {
    for (const auto &b : webBars)
      merged[b.year * 10000 + b.month * 100 + b.day] = b;
    for (const auto &b : seedBars)
      merged[b.year * 10000 + b.month * 100 + b.day] = b;
  }

  outBars.clear();
  outBars.reserve(merged.size());
  for (const auto &e : merged)
    outBars.push_back(e.second);

  Log("FetchHistoricalData: merged %zu bars for %s", outBars.size(), symbol);

  // 4. Update cache
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache[symbol] = outBars;
  }

  return true;
}

bool DseDataEngine::GetCachedBars(const char *symbol,
                                  std::vector<DseBar> &outBars) {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_cache.find(symbol);
  if (it == m_cache.end())
    return false;
  outBars = it->second;
  return !outBars.empty();
}

// ---------------------------------------------------------------------------
// Real-Time Data
// ---------------------------------------------------------------------------

bool DseDataEngine::FetchLatestQuotes(std::vector<DseQuote> &outQuotes) {
  Log("FetchLatestQuotes: fetching all");
  std::string html;
  if (!HttpGet(m_config.latestPriceUrl, html)) {
    Log("ERROR: FetchLatestQuotes — HTTP failed");
    return false;
  }
  return ParseLatestPriceHtml(html, outQuotes);
}

bool DseDataEngine::FetchLatestQuote(const char *symbol, DseQuote &outQuote) {
  std::vector<DseQuote> all;
  if (!FetchLatestQuotes(all))
    return false;

  for (const auto &q : all) {
    if (_stricmp(q.symbol, symbol) == 0) {
      outQuote = q;
      return true;
    }
  }
  Log("WARNING: FetchLatestQuote — '%s' not found", symbol);
  return false;
}

// ---------------------------------------------------------------------------
// Symbol Management
// ---------------------------------------------------------------------------

const std::vector<std::string> &DseDataEngine::GetSymbolList() {
  if (m_symbols.empty())
    RefreshSymbolList();
  return m_symbols;
}

bool DseDataEngine::RefreshSymbolList() {
  Log("RefreshSymbolList: fetching from DSE");
  std::vector<DseQuote> quotes;
  if (!FetchLatestQuotes(quotes))
    return false;

  std::lock_guard<std::mutex> lock(m_mutex);
  std::vector<std::string> fetched;
  for (const auto &q : quotes)
    if (q.symbol[0])
      fetched.push_back(q.symbol);

  // Amarstock indices are not listed on the DSE live page — inject them
  // manually
  fetched.push_back("00DS30");
  fetched.push_back("00DSES");
  fetched.push_back("00DSEX");
  fetched.push_back("00DSMEX");

  std::sort(fetched.begin(), fetched.end());
  m_symbols = fetched;
  Log("RefreshSymbolList: %zu symbols", m_symbols.size());
  return true;
}

// ---------------------------------------------------------------------------
// Market Hours
// ---------------------------------------------------------------------------

bool DseDataEngine::IsMarketOpen() const {
  SYSTEMTIME st;
  GetLocalTime(&st);

  // Convert to minutes-since-midnight for easy range check
  int now = st.wHour * 60 + st.wMinute;
  int open = m_config.marketOpenHour * 60 + m_config.marketOpenMinute;
  int close = m_config.marketCloseHour * 60 + m_config.marketCloseMinute;

  // DSE trades Sunday–Thursday (wDayOfWeek: Sun=0 ... Sat=6)
  if (st.wDayOfWeek == 5 || st.wDayOfWeek == 6)
    return false;

  return (now >= open && now <= close);
}

// ---------------------------------------------------------------------------
// Amarstock Index Support
// ---------------------------------------------------------------------------

bool DseDataEngine::IsAmarstockIndex(const char *symbol) {
  if (!symbol)
    return false;
  return (_stricmp(symbol, "00DS30") == 0 || _stricmp(symbol, "00DSES") == 0 ||
          _stricmp(symbol, "00DSEX") == 0 || _stricmp(symbol, "00DSMEX") == 0);
}

bool DseDataEngine::FetchAmarstockIndexData(const char *symbol,
                                            const char *startDate,
                                            const char *endDate,
                                            std::vector<DseBar> &outBars,
                                            std::function<void()> onProgress) {

  Log("FetchAmarstockIndexData: %s [%s -> %s]", symbol, startDate, endDate);

  int sy, sm, sd, ey, em, ed;
  if (sscanf_s(startDate, "%04d-%02d-%02d", &sy, &sm, &sd) != 3 ||
      sscanf_s(endDate, "%04d-%02d-%02d", &ey, &em, &ed) != 3) {
    Log("ERROR: Invalid date strings for FetchAmarstockIndexData");
    return false;
  }

  SYSTEMTIME st = {0}, endSt = {0};
  st.wYear = (WORD)sy;
  st.wMonth = (WORD)sm;
  st.wDay = (WORD)sd;
  endSt.wYear = (WORD)ey;
  endSt.wMonth = (WORD)em;
  endSt.wDay = (WORD)ed;

  FILETIME ftCur, ftEnd;
  SystemTimeToFileTime(&st, &ftCur);
  SystemTimeToFileTime(&endSt, &ftEnd);

  ULARGE_INTEGER uCur, uEnd;
  uCur.LowPart = ftCur.dwLowDateTime;
  uCur.HighPart = ftCur.dwHighDateTime;
  uEnd.LowPart = ftEnd.dwLowDateTime;
  uEnd.HighPart = ftEnd.dwHighDateTime;

  // Seed outBars with whatever is already cached
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_cache.find(symbol);
    if (it != m_cache.end())
      outBars = it->second;
  }

  int daysFetched = 0, missingDays = 0;

  while (uCur.QuadPart <= uEnd.QuadPart) {
    FileTimeToSystemTime(&ftCur, &st);

    // Skip if this day is already in cache
    bool haveIt = false;
    for (const auto &bar : outBars) {
      if (bar.year == st.wYear && bar.month == st.wMonth &&
          bar.day == st.wDay) {
        haveIt = true;
        break;
      }
    }

    if (!haveIt) {
      ++missingDays;
      char curDate[32];
      sprintf_s(curDate, "%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);

      char payload[256];
      sprintf_s(payload, "date=%s&type=adjusted", curDate);

      std::string body;
      if (HttpPost("https://www.amarstock.com/data/download/CSV", payload,
                   body) &&
          body.size() > 50) {
        std::vector<DseBar> parsed;
        if (CsvUtils::ParseAmarstockCsv(body, symbol, parsed)) {
          outBars.insert(outBars.end(), parsed.begin(), parsed.end());
          ++daysFetched;
          Log("FetchAmarstockIndexData: bar for %s (total=%zu)", curDate,
              outBars.size());
        } else {
          // Insert a placeholder so we don't re-fetch this empty day
          DseBar placeholder;
          memset(&placeholder, 0, sizeof(placeholder));
          placeholder.year = st.wYear;
          placeholder.month = st.wMonth;
          placeholder.day = st.wDay;
          placeholder.valid = false;
          outBars.push_back(placeholder);
        }
      }
      Sleep(50); // gentle rate-limiting
    }

    uCur.QuadPart += 864000000000ULL; // advance by one day (100-ns units)
    ftCur.dwLowDateTime = uCur.LowPart;
    ftCur.dwHighDateTime = uCur.HighPart;
  }

  // Sort and update cache in one shot
  std::sort(outBars.begin(), outBars.end(),
            [](const DseBar &a, const DseBar &b) {
              if (a.year != b.year)
                return a.year < b.year;
              if (a.month != b.month)
                return a.month < b.month;
              return a.day < b.day;
            });
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache[symbol] = outBars;
  }

  Log("FetchAmarstockIndexData: done — requested=%d fetched=%d total=%zu",
      missingDays, daysFetched, outBars.size());

  if (onProgress && daysFetched > 0)
    onProgress();

  return daysFetched > 0 || !outBars.empty();
}

// ---------------------------------------------------------------------------
// Export
// ---------------------------------------------------------------------------

void DseDataEngine::CheckAutoExport() {
  if (m_config.exportIntervalSec <= 0 || !m_config.exportPath[0])
    return;
  time_t now = time(NULL);
  if (now - m_lastExportTime >= m_config.exportIntervalSec) {
    ExportAllDataToCsv();
    m_lastExportTime = now;
  }
}

void DseDataEngine::ExportAllDataToCsv() {
  if (!m_config.exportPath[0])
    return;

  // Snapshot the cache so we hold the lock for minimum time
  std::map<std::string, std::vector<DseBar>> snapshot;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    snapshot = m_cache;
  }

  if (snapshot.empty()) {
    Log("ExportAllDataToCsv: cache empty, nothing to export");
    return;
  }

  int count = CsvUtils::ExportAllDataToCsv(snapshot, m_config.exportPath);
  Log("ExportAllDataToCsv: exported %d files to %s", count,
      m_config.exportPath);
}

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------

void DseDataEngine::Log(const char *fmt, ...) {
  if (!m_config.enableLogging && !m_logFile)
    return;

  SYSTEMTIME st;
  GetLocalTime(&st);

  char timestamp[64];
  sprintf_s(timestamp, "[%04d-%02d-%02d %02d:%02d:%02d] ", st.wYear, st.wMonth,
            st.wDay, st.wHour, st.wMinute, st.wSecond);

  va_list args;
  va_start(args, fmt);
  char message[2048];
  vsnprintf(message, sizeof(message), fmt, args);
  va_end(args);

  if (m_logFile) {
    fprintf(m_logFile, "%s%s\n", timestamp, message);
    fflush(m_logFile);
  }

  OutputDebugStringA(timestamp);
  OutputDebugStringA(message);
  OutputDebugStringA("\n");
}
