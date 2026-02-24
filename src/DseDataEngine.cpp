///////////////////////////////////////////////////////////////////////////
// DseDataEngine.cpp — DSE HTTP Client & Data Parser Implementation
//
// [Explanation for non-developers]:
// This is the true "engine" of the software. It handles connecting to the Dhaka
// Stock Exchange (DSE) website, downloading web pages, searching through HTML
// code (like reading raw code of a webpage) to find rows and columns containing
// prices, and turning that text into numbers that AmiBroker can display as
// candlestick charts.
//
// Uses WinInet to fetch data from dsebd.org, parses HTML tables,
// and caches OHLCV bars per symbol. Replicates bdshare's logic.
///////////////////////////////////////////////////////////////////////////

#include "DseDataEngine.h"
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <wininet.h>

#pragma comment(lib, "wininet.lib")

///////////////////////////////////////////////////////////////////////////
// Constructor / Destructor
///////////////////////////////////////////////////////////////////////////

DseDataEngine::DseDataEngine()
    : m_hInternet(NULL), m_hConnect(NULL), m_connState(CONN_DISCONNECTED),
      m_logFile(NULL) {
  memset(&m_config, 0, sizeof(m_config));
}

DseDataEngine::~DseDataEngine() { Shutdown(); }

///////////////////////////////////////////////////////////////////////////
// Initialize — Load config and create WinInet session
///////////////////////////////////////////////////////////////////////////

bool DseDataEngine::Initialize(const char *configPath) {
  std::lock_guard<std::mutex> lock(m_mutex);

  // Load configuration
  if (!LoadConfig(configPath)) {
    // Use defaults if config file not found
    m_config.historyDays = 365; // Default to 1 year of days
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
    m_config.enableLogging = true; // FORCED TRUE FOR DEBUGGING
  }

  // Force logging anyway for this debug session
  m_config.enableLogging = true;

  // Open logging
  if (m_config.enableLogging && m_config.logFilePath[0]) {
    fopen_s(&m_logFile, m_config.logFilePath, "a");
  }

  m_lastExportTime = time(NULL); // Start timer from now

  Log("DseDataEngine::Initialize — starting");

  // Create WinInet session
  m_hInternet = InternetOpenA(m_config.userAgent, INTERNET_OPEN_TYPE_PRECONFIG,
                              NULL, NULL, 0);

  if (!m_hInternet) {
    Log("ERROR: InternetOpen failed, error=%lu", GetLastError());
    m_connState = CONN_ERROR;
    return false;
  }

  // Set timeouts
  DWORD timeout = m_config.httpTimeoutSec * 1000;
  InternetSetOptionA(m_hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout,
                     sizeof(timeout));
  InternetSetOptionA(m_hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout,
                     sizeof(timeout));
  InternetSetOptionA(m_hInternet, INTERNET_OPTION_SEND_TIMEOUT, &timeout,
                     sizeof(timeout));

  m_connState = CONN_CONNECTED;
  Log("DseDataEngine::Initialize — WinInet session created OK");
  return true;
}

///////////////////////////////////////////////////////////////////////////
// Shutdown — Cleanup resources
///////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////
// LoadConfig — Parse INI file
//
// [Explanation for non-developers]:
// Every time you change a setting in the "Configure" window, it saves to a tiny
// `.ini` file. This function reads that text file when you start the plugin so
// the program remembers how many years of history to download and where to save
// auto-export backups.
///////////////////////////////////////////////////////////////////////////

bool DseDataEngine::LoadConfig(const char *path) {
  if (!path || !path[0])
    return false;

  // [General]
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

  // Default to 1 (Web > Local)
  m_config.preferWebData =
      (GetPrivateProfileIntA("General", "PreferWebData", 1, path) != 0);

  // [Endpoints]
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

  // [Debug]
  m_config.enableLogging =
      (GetPrivateProfileIntA("Debug", "EnableLogging", 0, path) != 0);

  GetPrivateProfileStringA("Debug", "LogFilePath", "dse_plugin.log",
                           m_config.logFilePath, sizeof(m_config.logFilePath),
                           path);

  // [DataSource]
  GetPrivateProfileStringA("DataSource", "CsvSeedPath",
                           "d:\\software\\dse_2000-2025\\separated_data",
                           m_config.csvSeedPath, sizeof(m_config.csvSeedPath),
                           path);

  // [Export]
  GetPrivateProfileStringA("Export", "ExportPath", "", m_config.exportPath,
                           sizeof(m_config.exportPath), path);
  m_config.exportIntervalSec =
      GetPrivateProfileIntA("Export", "ExportIntervalSec", 0, path);

  return true;
}

///////////////////////////////////////////////////////////////////////////
// HttpGet — Fetch a URL using WinInet
//
// [Explanation for non-developers]:
// Think of this as opening a webpage in an invisible browser like Chrome or
// Edge. It connects to `https://www.dsebd.org/...`, waits for a response, and
// saves the entire webpage source code as a giant block of text, preparing it
// for reading.
///////////////////////////////////////////////////////////////////////////

bool DseDataEngine::HttpGet(const char *url, std::string &outBody) {
  if (!m_hInternet) {
    Log("ERROR: HttpGet — no WinInet session");
    return false;
  }

  Log("HttpGet: %s", url);

  HINTERNET hUrl =
      InternetOpenUrlA(m_hInternet, url, NULL, 0,
                       INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE |
                           INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_SECURE,
                       0);

  if (!hUrl) {
    DWORD err = GetLastError();
    Log("ERROR: InternetOpenUrl failed, error=%lu", err);

    // Try without SECURE flag (some DSE pages may not need HTTPS)
    hUrl = InternetOpenUrlA(m_hInternet, url, NULL, 0,
                            INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE,
                            0);

    if (!hUrl) {
      m_connState = CONN_ERROR;
      return false;
    }
  }

  // Read response
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

///////////////////////////////////////////////////////////////////////////
// HttpPost — Fetch a URL using WinInet with POST body
///////////////////////////////////////////////////////////////////////////

bool DseDataEngine::HttpPost(const char *url, const char *payload,
                             std::string &outBody) {
  if (!m_hInternet) {
    Log("ERROR: HttpPost — no WinInet session");
    return false;
  }

  Log("HttpPost: %s with payload len %zu", url, strlen(payload));

  // Crack URL to get host and path
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

  // Read response
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

///////////////////////////////////////////////////////////////////////////
// BuildHistoryUrl — Construct day_end_archive URL
///////////////////////////////////////////////////////////////////////////

std::string DseDataEngine::BuildHistoryUrl(const char *symbol,
                                           const char *startDate,
                                           const char *endDate) {
  // Format: day_end_archive.php?startDate=2024-01-01&endDate=2024-12-31&inst=GP
  // Or without inst for all symbols
  std::string url = m_config.dayEndArchiveUrl;
  url += "?startDate=";
  url += startDate;
  url += "&endDate=";
  url += endDate;

  if (symbol && symbol[0]) {
    url += "&inst=";
    url += symbol;
  }

  // CRITICAL: DSE requires archive=data to return the table
  url += "&archive=data";

  return url;
}

///////////////////////////////////////////////////////////////////////////
// HTML Parsing Utilities
///////////////////////////////////////////////////////////////////////////

std::string DseDataEngine::StripHtml(const std::string &input) {
  std::string result;
  result.reserve(input.size());
  bool inTag = false;

  for (size_t i = 0; i < input.size(); ++i) {
    if (input[i] == '<') {
      inTag = true;
    } else if (input[i] == '>') {
      inTag = false;
    } else if (!inTag) {
      result += input[i];
    }
  }
  return result;
}

std::string DseDataEngine::Trim(const std::string &s) {
  size_t start = s.find_first_not_of(" \t\r\n");
  if (start == std::string::npos)
    return "";
  size_t end = s.find_last_not_of(" \t\r\n");
  return s.substr(start, end - start + 1);
}

double DseDataEngine::SafeStod(const std::string &s, double fallback) {
  if (s.empty())
    return fallback;

  // Remove commas from numbers (e.g., "1,234.56" -> "1234.56")
  std::string clean;
  clean.reserve(s.size());
  for (char c : s) {
    if (c != ',')
      clean += c;
  }

  if (clean.empty())
    return fallback;

  char *endPtr = nullptr;
  double val = strtod(clean.c_str(), &endPtr);
  if (endPtr == clean.c_str()) {
    return fallback;
  }
  return val;
}

bool DseDataEngine::ValidateBar(const DseBar &bar) {
  // Strict filtering: Any zero price in OHLC is invalid
  // This fixes "messy" charts caused by 0.0 spikes in CSV data.
  const double MIN_PRICE = 0.01;
  if (bar.open < MIN_PRICE || bar.high < MIN_PRICE || bar.low < MIN_PRICE ||
      bar.close < MIN_PRICE)
    return false;

  // Consistency checks
  if (bar.high < bar.low)
    return false;

  // Date sanity
  if (bar.year < 1990 || bar.year > 2100)
    return false;
  if (bar.month < 1 || bar.month > 12)
    return false;
  if (bar.day < 1 || bar.day > 31)
    return false;

  return true;
}

///////////////////////////////////////////////////////////////////////////
// ExtractTargetTable — Isolate the specific data table
///////////////////////////////////////////////////////////////////////////

std::string DseDataEngine::ExtractTargetTable(const std::string &html) {
  // Strategy 1: Look for "shares-table" class (matches bdshare)
  // <table class="table table-bordered background-white shares-table
  // fixedHeader">

  size_t tableStart = std::string::npos;
  size_t searchPos = 0;

  while (true) {
    size_t found = html.find("shares-table", searchPos);
    if (found == std::string::npos)
      break;

    // Backtrack to find <table opening tag
    size_t tagOpen = html.rfind("<table", found);
    if (tagOpen != std::string::npos) {
      tableStart = tagOpen;
      Log("ExtractTargetTable: Found table with 'shares-table' class at pos "
          "%zu",
          tableStart);
      break;
    }
    searchPos = found + 12; // continue search
  }

  // Strategy 2: Fallback — Look for table with specific headers
  if (tableStart == std::string::npos) {
    Log("ExtractTargetTable: 'shares-table' not found. Trying content "
        "fallback.");
    // Look for a table containing "DATE" and "VOLUME" and "CLOSE"/ "LTP"
    // We initiate a linear scan of <table> tags
    size_t pos = 0;
    while (true) {
      size_t tStart = html.find("<table", pos);
      if (tStart == std::string::npos)
        break;

      size_t tEnd = html.find("</table>", tStart);
      if (tEnd == std::string::npos)
        break;

      std::string tableContent = html.substr(tStart, tEnd - tStart);
      // Case-insensitive check (simple)
      std::string upper = tableContent;
      for (auto &c : upper)
        c = toupper(c);

      if (upper.find("DATE") != std::string::npos &&
          upper.find("VOLUME") != std::string::npos &&
          (upper.find("CLOSE") != std::string::npos ||
           upper.find("LTP") != std::string::npos)) {
        tableStart = tStart;
        Log("ExtractTargetTable: Found table by content match at pos %zu",
            tableStart);
        break;
      }
      pos = tEnd + 8;
    }
  }

  if (tableStart == std::string::npos) {
    Log("ExtractTargetTable: Could not isolate target table. Using full HTML.");
    return html; // Fallback to original behavior
  }

  // Extract until </table>
  size_t tableEnd = html.find("</table>", tableStart);
  if (tableEnd == std::string::npos)
    return html.substr(tableStart);

  return html.substr(tableStart, tableEnd - tableStart + 8);
}

///////////////////////////////////////////////////////////////////////////
// ExtractTableRows — Pull <tr>...</tr> blocks from HTML
///////////////////////////////////////////////////////////////////////////

std::vector<std::string>
DseDataEngine::ExtractTableRows(const std::string &html) {
  std::vector<std::string> rows;

  size_t pos = 0;
  while (true) {
    // Find <tr (case insensitive search)
    size_t trStart = std::string::npos;
    for (size_t i = pos; i < html.size() - 3; ++i) {
      if ((html[i] == '<') && (html[i + 1] == 't' || html[i + 1] == 'T') &&
          (html[i + 2] == 'r' || html[i + 2] == 'R') &&
          (html[i + 3] == ' ' || html[i + 3] == '>' || html[i + 3] == '\t')) {
        trStart = i;
        break;
      }
    }

    if (trStart == std::string::npos)
      break;

    // Find matching </tr>
    size_t trEnd = std::string::npos;
    for (size_t i = trStart + 3; i < html.size() - 4; ++i) {
      if ((html[i] == '<') && (html[i + 1] == '/') &&
          (html[i + 2] == 't' || html[i + 2] == 'T') &&
          (html[i + 3] == 'r' || html[i + 3] == 'R') && (html[i + 4] == '>')) {
        trEnd = i + 5;
        break;
      }
    }

    if (trEnd == std::string::npos)
      break;

    rows.push_back(html.substr(trStart, trEnd - trStart));
    pos = trEnd;
  }

  return rows;
}

///////////////////////////////////////////////////////////////////////////
// ParseTableRow — Extract <td> cell values from a single <tr>
///////////////////////////////////////////////////////////////////////////

std::vector<std::string> DseDataEngine::ParseTableRow(const std::string &row) {
  std::vector<std::string> cells;

  size_t pos = 0;
  while (true) {
    // Find <td or <th
    size_t tdStart = std::string::npos;
    bool isTh = false;

    // Search for next tag
    for (size_t i = pos; i < row.size() - 3; ++i) {
      if (row[i] == '<') {
        // storage for tag type
        char t1 = (char)tolower((unsigned char)row[i + 1]);
        char t2 = (char)tolower((unsigned char)row[i + 2]);
        char t3 = (char)row[i + 3];

        if (t1 == 't' && (t2 == 'd' || t2 == 'h') &&
            (t3 == ' ' || t3 == '>' || t3 == '\t')) {
          tdStart = i;
          isTh = (t2 == 'h');
          break;
        }
      }
    }

    if (tdStart == std::string::npos)
      break;

    // Skip past the opening tag >
    size_t contentStart = row.find('>', tdStart);
    if (contentStart == std::string::npos)
      break;
    contentStart++;

    // Find </td> or </th> (allow mismatched closing tags for robustness)
    size_t tdEnd = std::string::npos;
    for (size_t i = contentStart; i < row.size() - 4; ++i) {
      if (row[i] == '<' && row[i + 1] == '/') {
        char t1 = (char)tolower((unsigned char)row[i + 2]);
        char t2 = (char)tolower((unsigned char)row[i + 3]);
        if (t1 == 't' && (t2 == 'd' || t2 == 'h') && row[i + 4] == '>') {
          tdEnd = i;
          break;
        }
      }
    }

    if (tdEnd == std::string::npos)
      break;

    std::string cellContent = row.substr(contentStart, tdEnd - contentStart);
    // Strip any nested HTML tags and whitespace
    cellContent = Trim(StripHtml(cellContent));
    cells.push_back(cellContent);

    pos = tdEnd + 5;
  }

  return cells;
}

///////////////////////////////////////////////////////////////////////////
// ParseHistoricalHtml — Parse day_end_archive page into bars
//
// [Explanation for non-developers]:
// This function reads the giant block of text downloaded from the DSE website
// specifically for historical data. It searches for the word "Date", "Close",
// "Volume", etc. and goes line by line, column by column, throwing out the HTML
// formatting tags and pulling out the pure numbers (Open, High, Low, Close
// prices) for past days.
///////////////////////////////////////////////////////////////////////////

bool DseDataEngine::ParseHistoricalHtml(const std::string &html,
                                        std::vector<DseBar> &outBars) {

  // 1. Dump HTML info
  Log("ParseHistoricalHtml: Input size=%zu", html.size());

  // 2. Extract specific table and then rows
  std::string tableHtml = ExtractTargetTable(html);
  auto rows = ExtractTableRows(tableHtml);
  Log("ParseHistoricalHtml: Extracted %zu total rows from targeted table",
      rows.size());

  // 3. Scan for header
  int colDate = -1, colOpen = -1, colHigh = -1, colLow = -1;
  int colClose = -1, colVol = -1;
  int headerRow = -1;

  for (int r = 0; r < (int)rows.size(); ++r) {
    if (r > 300)
      break; // Header should be somewhat near top (but after ticker)

    auto cells = ParseTableRow(rows[r]);
    if (cells.size() < 5)
      continue; // Skip noise

    // Keyword Search
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
                h.find("CLOSEP") != std::string::npos ||
                h.find("LTP") != std::string::npos) &&
               h.find("YCP") == std::string::npos)
        colClose = i;
      else if (h.find("VOLUME") != std::string::npos ||
               h.find("VOL") != std::string::npos)
        colVol = i;
    }

    // Strict check: Need Date, Close, Volume
    if (colDate != -1 && colClose != -1 && colVol != -1) {
      headerRow = r;
      Log("ParseHistoricalHtml: Header Found at Row %d", r);
      Log("Columns: Date=%d Open=%d High=%d Low=%d Close=%d Vol=%d", colDate,
          colOpen, colHigh, colLow, colClose, colVol);
      break;
    }
  }

  if (headerRow == -1) {
    Log("ParseHistoricalHtml: No header row found. Trying fallback (indices "
        "1,6,4,5,7,11).");
    // Fallback: Assume first row with > 10 columns is data if we didn't find
    // header This is the "Blind Faith" option if headers are in Bengali or
    // something wild.
    colDate = 1;
    colOpen = 6;
    colHigh = 4;
    colLow = 5;
    colClose = 7;
    colVol = 11;

    // Find first likely data row
    for (int r = 0; r < (int)rows.size(); ++r) {
      if (r > 300)
        break;
      auto cells = ParseTableRow(rows[r]);
      if (cells.size() > 11) {
        headerRow = r - 1; // Start parsing from this row
        Log("ParseHistoricalHtml: Fallback start at row %d", r);
        break;
      }
    }
  }

  if (headerRow == -1) {
    Log("ParseHistoricalHtml: Failed to find any usable table structure.");
    return false;
  }

  // 4. Parse Data
  int validCount = 0;
  for (size_t r = headerRow + 1; r < rows.size(); ++r) {
    auto cells = ParseTableRow(rows[r]);

    // Determine max index needed
    int maxIdx = colDate;
    if (colClose > maxIdx)
      maxIdx = colClose;
    if (colVol > maxIdx)
      maxIdx = colVol;

    if ((int)cells.size() <= maxIdx)
      continue;

    DseBar bar;
    memset(&bar, 0, sizeof(bar));
    bar.valid = true;

    if (colDate < (int)cells.size()) {
      std::string dateStr = cells[colDate];
      // Simple YYYY-MM-DD check or similar
      if (dateStr.size() >= 10) {
        bar.year = (int)SafeStod(dateStr.substr(0, 4));
        bar.month = (int)SafeStod(dateStr.substr(5, 2));
        bar.day = (int)SafeStod(dateStr.substr(8, 2));
      } else {
        continue; // Not a date row
      }
    }

    if (colOpen != -1 && colOpen < (int)cells.size())
      bar.open = SafeStod(cells[colOpen]);
    if (colHigh != -1 && colHigh < (int)cells.size())
      bar.high = SafeStod(cells[colHigh]);
    if (colLow != -1 && colLow < (int)cells.size())
      bar.low = SafeStod(cells[colLow]);
    if (colClose != -1 && colClose < (int)cells.size())
      bar.close = SafeStod(cells[colClose]);
    if (colVol != -1 && colVol < (int)cells.size())
      bar.volume = SafeStod(cells[colVol]);

    bar.valid = ValidateBar(bar);
    if (bar.valid) {
      outBars.push_back(bar);
      validCount++;
    }
  }

  Log("ParseHistoricalHtml: Parsed %d valid bars", validCount);
  return !outBars.empty();
}

///////////////////////////////////////////////////////////////////////////
// ParseLatestPriceHtml — Parse latest_share_price page
//
// [Explanation for non-developers]:
// This is exactly like scanning the live "Latest Price" scrolling board on the
// DSE homepage. Every few seconds, this function extracts the single most
// up-to-date trade price (LTP) and volume for every stock simultaneously. This
// makes your live charts "tick" and update without refreshing.
///////////////////////////////////////////////////////////////////////////

bool DseDataEngine::ParseLatestPriceHtml(const std::string &html,
                                         std::vector<DseQuote> &outQuotes) {
  // Columns for latest_share_price_scroll_l.php:
  // # | TRADING CODE | LTP | HIGH | LOW | CLOSE | YCP |
  // CHANGE | TRADE | VALUE (mn) | VOLUME

  auto rows = ExtractTableRows(html);

  if (rows.size() < 2) {
    Log("ParseLatestPriceHtml: too few rows (%zu)", rows.size());
    return false;
  }

  // Detect column indices from header
  int colSymbol = -1, colLtp = -1, colHigh = -1, colLow = -1;
  int colClose = -1, colYcp = -1, colChange = -1;
  int colTrade = -1, colValue = -1, colVolume = -1;

  auto headerCells = ParseTableRow(rows[0]);
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

  // Fallback
  if (colSymbol == -1)
    colSymbol = 1;
  if (colLtp == -1)
    colLtp = 2;
  if (colHigh == -1)
    colHigh = 3;
  if (colLow == -1)
    colLow = 4;
  if (colClose == -1)
    colClose = 5;
  if (colYcp == -1)
    colYcp = 6;
  if (colChange == -1)
    colChange = 7;
  if (colTrade == -1)
    colTrade = 8;
  if (colValue == -1)
    colValue = 9;
  if (colVolume == -1)
    colVolume = 10;

  Log("ParseLatestPriceHtml: detected %zu rows", rows.size());

  // Parse data rows
  for (size_t r = 1; r < rows.size(); ++r) {
    auto cells = ParseTableRow(rows[r]);
    if (cells.empty())
      continue;

    int maxCol = (int)cells.size();

    DseQuote q;
    memset(&q, 0, sizeof(q));
    q.valid = true;

    if (colSymbol < maxCol) {
      strncpy_s(q.symbol, cells[colSymbol].c_str(), sizeof(q.symbol) - 1);
      // Trim the symbol
      std::string sym = Trim(cells[colSymbol]);
      strncpy_s(q.symbol, sym.c_str(), sizeof(q.symbol) - 1);
    }

    if (colLtp < maxCol)
      q.ltp = SafeStod(cells[colLtp]);
    if (colHigh < maxCol)
      q.high = SafeStod(cells[colHigh]);
    if (colLow < maxCol)
      q.low = SafeStod(cells[colLow]);
    if (colClose < maxCol)
      q.close = SafeStod(cells[colClose]);
    if (colYcp < maxCol)
      q.ycp = SafeStod(cells[colYcp]);
    if (colChange < maxCol)
      q.change = SafeStod(cells[colChange]);
    if (colTrade < maxCol)
      q.trade = SafeStod(cells[colTrade]);
    if (colValue < maxCol)
      q.value = SafeStod(cells[colValue]);
    if (colVolume < maxCol)
      q.volume = SafeStod(cells[colVolume]);

    // Calculate change percent
    if (q.ycp > 0) {
      q.changePercent = ((q.ltp - q.ycp) / q.ycp) * 100.0;
    }

    // Open defaults to YCP if not available from live feed
    q.open = q.ycp;

    // Skip empty/invalid symbols
    if (q.symbol[0] && q.ltp > 0) {
      outQuotes.push_back(q);
    }
  }

  Log("ParseLatestPriceHtml: parsed %zu quotes", outQuotes.size());
  return !outQuotes.empty();
}

///////////////////////////////////////////////////////////////////////////
// FetchHistoricalData — Download OHLCV history for a symbol
///////////////////////////////////////////////////////////////////////////

bool DseDataEngine::FetchHistoricalData(const char *symbol,
                                        const char *startDate,
                                        const char *endDate,
                                        std::vector<DseBar> &outBars,
                                        std::function<void()> onProgress) {
  Log("FetchHistoricalData: %s from %s to %s", symbol, startDate, endDate);

  outBars.clear();

  // Redirect Amarstock Indices to special scraper logic
  if (IsAmarstockIndex(symbol)) {
    return FetchAmarstockIndexData(symbol, startDate, endDate, outBars,
                                   onProgress);
  }

  // 1. Check local seed CSV if enabled first
  std::vector<DseBar> seedBars;
  if (LoadCsvSeed(symbol, seedBars)) {
    Log("FetchHistoricalData: Loaded %zu bars from CSV seed for %s",
        seedBars.size(), symbol);

    // Sort seed bars
    std::sort(seedBars.begin(), seedBars.end(),
              [](const DseBar &a, const DseBar &b) {
                if (a.year != b.year)
                  return a.year < b.year;
                if (a.month != b.month)
                  return a.month < b.month;
                return a.day < b.day;
              });
  }

  // 2. Fetch from Web
  // We fetch strictly *after* the seed data if possible, or just fetch overlaps
  // and merge.

  std::string url = BuildHistoryUrl(symbol, startDate, endDate);
  std::string html;
  std::vector<DseBar> webBars;

  bool webSuccess = false;
  if (HttpGet(url.c_str(), html)) {
    if (ParseHistoricalHtml(html, webBars)) {
      Log("FetchHistoricalData: Fetched %zu bars from web for %s",
          webBars.size(), symbol);
      webSuccess = true;
    } else {
      Log("WARNING: FetchHistoricalData — parse failed or empty for %s",
          symbol);
    }
  } else {
    Log("ERROR: FetchHistoricalData — HTTP failed for %s", symbol);
  }

  // If both failed, return false
  if (seedBars.empty() && !webSuccess) {
    return false;
  }

  // 3. Merge Strategies
  // Map dedup: Key = YYYYMMDD
  std::map<int, DseBar> merged;

  if (m_config.preferWebData) {
    // Preference: Web > Seed
    // Insert Seed first
    for (const auto &b : seedBars) {
      int key = b.year * 10000 + b.month * 100 + b.day;
      merged[key] = b;
    }
    // Insert Web second (overwrites Seed)
    for (const auto &b : webBars) {
      int key = b.year * 10000 + b.month * 100 + b.day;
      merged[key] = b;
    }
  } else {
    // Preference: Seed > Web
    // Insert Web first
    for (const auto &b : webBars) {
      int key = b.year * 10000 + b.month * 100 + b.day;
      merged[key] = b;
    }
    // Insert Seed second (overwrites Web)
    for (const auto &b : seedBars) {
      int key = b.year * 10000 + b.month * 100 + b.day;
      merged[key] = b;
    }
  }

  outBars.clear();
  outBars.reserve(merged.size());
  for (const auto &entry : merged) {
    outBars.push_back(entry.second);
  }

  Log("FetchHistoricalData: Merged total %zu bars for %s", outBars.size(),
      symbol);

  // 4. Update Cache (for GetCachedBars usage)
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache[symbol] = outBars;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////
// GetCachedBars — Return cached bars for a symbol
///////////////////////////////////////////////////////////////////////////

bool DseDataEngine::GetCachedBars(const char *symbol,
                                  std::vector<DseBar> &outBars) {
  std::lock_guard<std::mutex> lock(m_mutex);

  auto it = m_cache.find(symbol);
  if (it == m_cache.end())
    return false;

  outBars = it->second;
  return !outBars.empty();
}

///////////////////////////////////////////////////////////////////////////
// FetchLatestQuotes — Get all current trading data
///////////////////////////////////////////////////////////////////////////

bool DseDataEngine::FetchLatestQuotes(std::vector<DseQuote> &outQuotes) {
  Log("FetchLatestQuotes: fetching all");

  std::string html;
  if (!HttpGet(m_config.latestPriceUrl, html)) {
    Log("ERROR: FetchLatestQuotes — HTTP failed");
    return false;
  }

  return ParseLatestPriceHtml(html, outQuotes);
}

///////////////////////////////////////////////////////////////////////////
// FetchLatestQuote — Get current trading data for one symbol
///////////////////////////////////////////////////////////////////////////

bool DseDataEngine::FetchLatestQuote(const char *symbol, DseQuote &outQuote) {
  std::vector<DseQuote> allQuotes;

  if (!FetchLatestQuotes(allQuotes))
    return false;

  for (const auto &q : allQuotes) {
    if (_stricmp(q.symbol, symbol) == 0) {
      outQuote = q;
      return true;
    }
  }

  Log("WARNING: FetchLatestQuote — symbol '%s' not found", symbol);
  return false;
}

///////////////////////////////////////////////////////////////////////////
// Symbol Management
///////////////////////////////////////////////////////////////////////////

const std::vector<std::string> &DseDataEngine::GetSymbolList() {
  if (m_symbols.empty()) {
    RefreshSymbolList();
  }
  return m_symbols;
}

bool DseDataEngine::RefreshSymbolList() {
  Log("RefreshSymbolList: fetching from DSE");

  std::vector<DseQuote> quotes;
  if (!FetchLatestQuotes(quotes))
    return false;

  std::lock_guard<std::mutex> lock(m_mutex);
  std::vector<std::string> fetched;

  for (const auto &q : quotes) {
    if (q.symbol[0]) {
      fetched.push_back(q.symbol);
    }
  }

  // Forcefully inject Amarstock Special Indices because they do not appear on
  // the DSE homepage
  fetched.push_back("00DS30");
  fetched.push_back("00DSES");
  fetched.push_back("00DSEX");
  fetched.push_back("00DSMEX");

  // Sort alphabetically
  std::sort(fetched.begin(), fetched.end());
  m_symbols = fetched;

  Log("RefreshSymbolList: found %zu symbols", m_symbols.size());
  return true;
}

///////////////////////////////////////////////////////////////////////////
// IsMarketOpen — Check if within DSE trading hours
///////////////////////////////////////////////////////////////////////////

bool DseDataEngine::IsMarketOpen() const {
  SYSTEMTIME st;
  GetLocalTime(&st);

  // Convert to minutes since midnight for easy comparison
  int nowMinutes = st.wHour * 60 + st.wMinute;
  int openMinutes = m_config.marketOpenHour * 60 + m_config.marketOpenMinute;
  int closeMinutes = m_config.marketCloseHour * 60 + m_config.marketCloseMinute;

  // Check if it's a weekday (Sun=0, Mon=1, ..., Sat=6)
  // DSE trades Sun-Thu
  if (st.wDayOfWeek == 5 || st.wDayOfWeek == 6) {
    return false; // Friday or Saturday
  }

  return (nowMinutes >= openMinutes && nowMinutes <= closeMinutes);
}

///////////////////////////////////////////////////////////////////////////
// Amarstock Index Scraper Integration
///////////////////////////////////////////////////////////////////////////

bool DseDataEngine::IsAmarstockIndex(const char *symbol) {
  if (!symbol)
    return false;
  return (_stricmp(symbol, "00DS30") == 0 || _stricmp(symbol, "00DSES") == 0 ||
          _stricmp(symbol, "00DSEX") == 0 || _stricmp(symbol, "00DSMEX") == 0);
}

bool DseDataEngine::ParseAmarstockCsv(const std::string &csv,
                                      const char *targetSymbol,
                                      std::vector<DseBar> &outBars) {
  // Split string by lines
  size_t pos = 0;
  size_t lastPos = 0;
  std::vector<std::string> lines;
  while ((pos = csv.find('\n', lastPos)) != std::string::npos) {
    std::string line = Trim(csv.substr(lastPos, pos - lastPos));
    if (!line.empty())
      lines.push_back(line);
    lastPos = pos + 1;
  }
  if (lastPos < csv.size()) {
    std::string line = Trim(csv.substr(lastPos));
    if (!line.empty())
      lines.push_back(line);
  }

  if (lines.empty())
    return false;

  // Expected columns: Date, Scrip, Open, High, Low, Close, Volume, Value, Trade
  int parsedCount = 0;
  for (size_t i = 1; i < lines.size(); ++i) { // Skip header
    std::string &line = lines[i];
    std::vector<std::string> parts;
    size_t commaPos = 0, lastComma = 0;
    while ((commaPos = line.find(',', lastComma)) != std::string::npos) {
      parts.push_back(Trim(line.substr(lastComma, commaPos - lastComma)));
      lastComma = commaPos + 1;
    }
    parts.push_back(Trim(line.substr(lastComma)));

    // Ensure we have at least 7 columns
    if (parts.size() < 7)
      continue;

    // Check target symbol matches
    if (_stricmp(parts[1].c_str(), targetSymbol) != 0)
      continue;

    DseBar bar;
    memset(&bar, 0, sizeof(bar));
    bar.valid = true;

    // Parse Date (YYYY-MM-DD, MM/DD/YYYY, or YYYYMMDD)
    std::string dateStr = parts[0];
    if (dateStr.size() >= 10 && dateStr[4] == '-') {
      // YYYY-MM-DD
      bar.year = (int)SafeStod(dateStr.substr(0, 4));
      bar.month = (int)SafeStod(dateStr.substr(5, 2));
      bar.day = (int)SafeStod(dateStr.substr(8, 2));
    } else if (dateStr.size() == 8) {
      // YYYYMMDD
      bar.year = (int)SafeStod(dateStr.substr(0, 4));
      bar.month = (int)SafeStod(dateStr.substr(4, 2));
      bar.day = (int)SafeStod(dateStr.substr(6, 2));
    } else {
      // Assume Amarstock changed format or invalid date string
      continue;
    }

    bar.open = SafeStod(parts[2]);
    bar.high = SafeStod(parts[3]);
    bar.low = SafeStod(parts[4]);
    bar.close = SafeStod(parts[5]);
    bar.volume = SafeStod(parts[6]);

    // Optional Trades and Value might be missing on some lines
    if (parts.size() >= 9) {
      bar.value = SafeStod(parts[7]);
      bar.trade = SafeStod(parts[8]);
    }

    bar.valid = ValidateBar(bar);
    if (bar.valid) {
      outBars.push_back(bar);
      parsedCount++;
    }
  }

  return parsedCount > 0;
}

bool DseDataEngine::FetchAmarstockIndexData(const char *symbol,
                                            const char *startDate,
                                            const char *endDate,
                                            std::vector<DseBar> &outBars,
                                            std::function<void()> onProgress) {
  Log("FetchAmarstockIndexData: %s from %s to %s", symbol, startDate, endDate);

  // Parse start/end dates for iteration
  int sy = 0, sm = 0, sd = 0, ey = 0, em = 0, ed = 0;
  if (sscanf_s(startDate, "%04d-%02d-%02d", &sy, &sm, &sd) != 3) {
    Log("ERROR: Invalid startDate %s", startDate);
    return false;
  }
  if (sscanf_s(endDate, "%04d-%02d-%02d", &ey, &em, &ed) != 3) {
    Log("ERROR: Invalid endDate %s", endDate);
    return false;
  }

  SYSTEMTIME st = {0};
  st.wYear = sy;
  st.wMonth = sm;
  st.wDay = sd;

  SYSTEMTIME endSt = {0};
  endSt.wYear = ey;
  endSt.wMonth = em;
  endSt.wDay = ed;

  // Windows filetime comparison tricks
  FILETIME ftCur, ftEnd;
  SystemTimeToFileTime(&st, &ftCur);
  SystemTimeToFileTime(&endSt, &ftEnd);
  ULARGE_INTEGER uCur, uEnd;
  uCur.LowPart = ftCur.dwLowDateTime;
  uCur.HighPart = ftCur.dwHighDateTime;
  uEnd.LowPart = ftEnd.dwLowDateTime;
  uEnd.HighPart = ftEnd.dwHighDateTime;

  int daysFetched = 0;
  int missingDays = 0;

  // Read cache safely
  std::vector<DseBar> existing;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_cache.find(symbol);
    if (it != m_cache.end()) {
      existing = it->second;
    }
  }

  outBars = existing; // start with existing data

  // Iterate day by day
  while (uCur.QuadPart <= uEnd.QuadPart) {
    // Unpack current date
    FileTimeToSystemTime(&ftCur, &st);

    char curDateStr[32];
    sprintf_s(curDateStr, "%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);

    // Skip fetching if already in cache (duplicate check logic as used by
    // standard DSE engine)
    bool haveIt = false;
    for (const auto &bar : outBars) {
      if (bar.year == st.wYear && bar.month == st.wMonth &&
          bar.day == st.wDay) {
        haveIt = true;
        break;
      }
    }

    if (!haveIt) {
      missingDays++;

      char payload[256];
      // User requested adjusted data.
      sprintf_s(payload, "date=%s&type=adjusted", curDateStr);

      Log("FetchAmarstockIndexData: Requesting missing day %s", curDateStr);

      std::string responseBody;
      if (HttpPost("https://www.amarstock.com/data/download/CSV", payload,
                   responseBody)) {
        if (responseBody.size() > 50) {
          // Parse directly
          std::vector<DseBar> parsedBars;
          if (ParseAmarstockCsv(responseBody, symbol, parsedBars)) {
            outBars.insert(outBars.end(), parsedBars.begin(), parsedBars.end());
            daysFetched++;

            Log("FetchAmarstockIndexData: Collected bar(s) for %s. Running "
                "total: %zu bars (batching, no AB update yet)",
                curDateStr, outBars.size());
          } else {
            Log("FetchAmarstockIndexData: Parse returned 0 valid rows for %s. "
                "Likely empty market. Marking as empty in cache to prevent "
                "re-fetch loops.",
                curDateStr);
            // Insert dummy/empty bar with VALID=FALSE so we remember we checked
            // this date!
            DseBar dummy;
            memset(&dummy, 0, sizeof(dummy));
            dummy.year = st.wYear;
            dummy.month = st.wMonth;
            dummy.day = st.wDay;
            dummy.valid = false;

            outBars.push_back(dummy);
          }
        }
      } else {
        Log("WARNING: FetchAmarstockIndexData HTTP POST failed for %s",
            curDateStr);
      }

      // Very small sleep to be gentle
      Sleep(50);
    }

    // Add 1 day (864000000000 100-ns intervals)
    uCur.QuadPart += 864000000000ULL;
    ftCur.dwLowDateTime = uCur.LowPart;
    ftCur.dwHighDateTime = uCur.HighPart;
  }

  // All days fetched — now do one single sort, cache update, and AB
  // notification
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

  Log("FetchAmarstockIndexData: Completed. Requested %d missing days, "
      "fetched %d days. Total bars: %zu. Firing single onProgress().",
      missingDays, daysFetched, outBars.size());

  // Fire ONE single update to AmiBroker with the full batch
  if (onProgress && daysFetched > 0) {
    onProgress();
  }

  return (daysFetched > 0 || !outBars.empty());
}

///////////////////////////////////////////////////////////////////////////
// Logging
///////////////////////////////////////////////////////////////////////////

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

  // Also output to debug console
  OutputDebugStringA(timestamp);
  OutputDebugStringA(message);
  OutputDebugStringA("\n");
}

///////////////////////////////////////////////////////////////////////////
// LoadCsvSeed — Read local CSV file (Ticker,Date,Open,High,Low,Close,Volume)
///////////////////////////////////////////////////////////////////////////

bool DseDataEngine::LoadCsvSeed(const char *symbol,
                                std::vector<DseBar> &outBars) {
  if (!m_config.csvSeedPath[0])
    return false;

  char path[1024];
  sprintf_s(path, "%s\\%s.csv", m_config.csvSeedPath, symbol);

  FILE *fp = NULL;
  errno_t err = fopen_s(&fp, path, "r");
  if (err != 0 || !fp) {
    // Try without extension just in case
    return false;
  }

  Log("LoadCsvSeed: Reading %s", path);

  char line[2048]; // Increased buffer
  int rowCount = 0;

  while (fgets(line, sizeof(line), fp)) {
    // Trim potential whitespace/newlines
    char *p = line;
    while (*p)
      p++;
    p--;
    while (p >= line && (*p == '\n' || *p == '\r' || *p == ' ' || *p == '\t'))
      *p-- = 0;

    if (line[0] == 0)
      continue;

    // We need to parse two formats:
    // Format A (Standard): Ticker,Date,Open,High,Low,Close,Volume
    // Format B (User):     Date,Open,High,Low,Close,Volume

    // Let's tokenize a copy
    char tmp[2048];
    strcpy_s(tmp, line);

    char *ctx = NULL;
    char *c1 = strtok_s(tmp, ",", &ctx);
    if (!c1)
      continue;

    // Heuristic: Does the first token look like a date (YYYY-MM-DD)?
    bool firstIsDate = false;
    if (strlen(c1) >= 10 && c1[4] == '-' && c1[7] == '-') {
      firstIsDate = true;
    }

    // Header check
    if (isalpha(c1[0])) {
      // "Date" or "Ticker" or "Trading_Code"
      // If it's literally "Date", it's a header for Format B
      // If it's "Ticker" etc, header for Format A
      // If it's "GP" (symbol), it might be data for Format A
      // But if we are here, and it's NOT a date format (20xx-xx-xx starts with
      // digit), it's likely a header or Format A data.

      // Simple header skip
      if (_stricmp(c1, "Date") == 0 || _stricmp(c1, "Ticker") == 0 ||
          _stricmp(c1, "Trading_Code") == 0) {
        continue;
      }
    }

    DseBar bar;
    memset(&bar, 0, sizeof(bar));

    char *dateStr = NULL;
    char *openStr = NULL;
    char *highStr = NULL;
    char *lowStr = NULL;
    char *closeStr = NULL;
    char *volStr = NULL;

    if (firstIsDate) {
      // Format B: Date,Open,High,Low,Close,Volume
      dateStr = c1;
      openStr = strtok_s(NULL, ",", &ctx);
      highStr = strtok_s(NULL, ",", &ctx);
      lowStr = strtok_s(NULL, ",", &ctx);
      closeStr = strtok_s(NULL, ",", &ctx);
      volStr = strtok_s(NULL, ",", &ctx);
    } else {
      // Format A: Ticker,Date,Open,High,Low,Close,Volume
      // c1 is Ticker, ignore
      dateStr = strtok_s(NULL, ",", &ctx);
      openStr = strtok_s(NULL, ",", &ctx);
      highStr = strtok_s(NULL, ",", &ctx);
      lowStr = strtok_s(NULL, ",", &ctx);
      closeStr = strtok_s(NULL, ",", &ctx);
      volStr = strtok_s(NULL, ",", &ctx);
    }

    if (!dateStr || strlen(dateStr) < 10)
      continue;

    // Parse Date (YYYY-MM-DD)
    bar.year = atoi(std::string(dateStr, 4).c_str());
    bar.month = atoi(std::string(dateStr + 5, 2).c_str());
    bar.day = atoi(std::string(dateStr + 8, 2).c_str());
    if (bar.year == 0 || bar.month == 0 || bar.day == 0) {
      continue;
    }

    if (openStr)
      bar.open = atof(openStr);
    if (highStr)
      bar.high = atof(highStr);
    if (lowStr)
      bar.low = atof(lowStr);
    if (closeStr)
      bar.close = atof(closeStr);
    if (volStr)
      bar.volume = atof(volStr);

    // Filter validation
    if (ValidateBar(bar)) {
      outBars.push_back(bar);
      rowCount++;
    }
  }

  fclose(fp);
  Log("LoadCsvSeed: Parsed %d bars for %s (FirstIsDate support active)",
      rowCount, symbol);
  return !outBars.empty();
}

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

  std::vector<std::string> keys;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto const &item : m_cache) {
      keys.push_back(item.first);
    }
  }

  if (keys.empty()) {
    Log("ExportAllDataToCsv: Cache empty, nothing to export.");
    return;
  }

  Log("ExportAllDataToCsv: Starting export of %zu symbols to %s", keys.size(),
      m_config.exportPath);

  int count = 0;
  for (const auto &sym : keys) {
    std::vector<DseBar> bars;
    GetCachedBars(sym.c_str(), bars); // Locks inside again

    if (bars.empty())
      continue;

    char path[1024];
    sprintf_s(path, "%s\\%s.csv", m_config.exportPath, sym.c_str());

    FILE *fp = NULL;
    if (fopen_s(&fp, path, "w") == 0 && fp) {
      // Header: User liked "Date,Open,High,Low,Close,Volume"
      fprintf(fp, "Date,Open,High,Low,Close,Volume\n");

      for (const auto &b : bars) {
        fprintf(fp, "%04d-%02d-%02d,%.2f,%.2f,%.2f,%.2f,%.0f\n", b.year,
                b.month, b.day, b.open, b.high, b.low, b.close, b.volume);
      }
      fclose(fp);
      count++;
    }
  }
  Log("ExportAllDataToCsv: Exported %d files.", count);
}
