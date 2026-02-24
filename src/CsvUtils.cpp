///////////////////////////////////////////////////////////////////////////
// CsvUtils.cpp — CSV Handling Utilities Implementation
///////////////////////////////////////////////////////////////////////////

#include "CsvUtils.h"
#include "DseTypes.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace CsvUtils {

bool LoadCsvSeed(const char *symbol, const char *csvSeedPath,
                 std::vector<DseBar> &outBars) {
  if (!csvSeedPath || !csvSeedPath[0])
    return false;

  char path[1024];
  sprintf_s(path, "%s\\%s.csv", csvSeedPath, symbol);

  FILE *fp = NULL;
  errno_t err = fopen_s(&fp, path, "r");
  if (err != 0 || !fp) {
    return false;
  }

  char line[2048];
  int rowCount = 0;

  while (fgets(line, sizeof(line), fp)) {
    // Trim trailing whitespace/newlines
    char *p = line;
    while (*p) p++;
    p--;
    while (p >= line && (*p == '\n' || *p == '\r' || *p == ' ' || *p == '\t'))
      *p-- = 0;

    if (line[0] == 0)
      continue;

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

    // Header check - skip header rows
    if (isalpha(static_cast<unsigned char>(c1[0]))) {
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

    if (openStr) bar.open = atof(openStr);
    if (highStr) bar.high = atof(highStr);
    if (lowStr) bar.low = atof(lowStr);
    if (closeStr) bar.close = atof(closeStr);
    if (volStr) bar.volume = atof(volStr);

    // Validate bar before adding
    const double MIN_PRICE = 0.01;
    bool valid = (bar.open >= MIN_PRICE && bar.high >= MIN_PRICE &&
                  bar.low >= MIN_PRICE && bar.close >= MIN_PRICE &&
                  bar.high >= bar.low &&
                  bar.year >= 1990 && bar.year <= 2100 &&
                  bar.month >= 1 && bar.month <= 12 &&
                  bar.day >= 1 && bar.day <= 31);

    if (valid) {
      outBars.push_back(bar);
      rowCount++;
    }
  }

  fclose(fp);
  return !outBars.empty();
}

bool ExportBarsToCsv(const char *symbol, const char *exportPath,
                     const std::vector<DseBar> &bars) {
  if (!exportPath || !exportPath[0] || bars.empty())
    return false;

  char path[1024];
  sprintf_s(path, "%s\\%s.csv", exportPath, symbol);

  FILE *fp = NULL;
  if (fopen_s(&fp, path, "w") != 0 || !fp)
    return false;

  fprintf(fp, "Date,Open,High,Low,Close,Volume\n");

  for (const auto &b : bars) {
    fprintf(fp, "%04d-%02d-%02d,%.2f,%.2f,%.2f,%.2f,%.0f\n",
            b.year, b.month, b.day, b.open, b.high, b.low, b.close, b.volume);
  }

  fclose(fp);
  return true;
}

int ExportAllDataToCsv(const std::map<std::string, std::vector<DseBar>> &cache,
                       const char *exportPath) {
  if (!exportPath || !exportPath[0])
    return 0;

  int count = 0;
  for (const auto &item : cache) {
    if (!item.second.empty()) {
      if (ExportBarsToCsv(item.first.c_str(), exportPath, item.second)) {
        count++;
      }
    }
  }
  return count;
}

bool ParseAmarstockCsv(const std::string &csv, const char *targetSymbol,
                       std::vector<DseBar> &outBars) {
  std::vector<std::string> lines;
  size_t pos = 0, lastPos = 0;

  while ((pos = csv.find('\n', lastPos)) != std::string::npos) {
    std::string line = csv.substr(lastPos, pos - lastPos);
    // Trim
    size_t start = line.find_first_not_of(" \t\r\n");
    if (start != std::string::npos) {
      size_t end = line.find_last_not_of(" \t\r\n");
      line = line.substr(start, end - start + 1);
      if (!line.empty())
        lines.push_back(line);
    }
    lastPos = pos + 1;
  }
  if (lastPos < csv.size()) {
    std::string line = csv.substr(lastPos);
    size_t start = line.find_first_not_of(" \t\r\n");
    if (start != std::string::npos) {
      size_t end = line.find_last_not_of(" \t\r\n");
      line = line.substr(start, end - start + 1);
      if (!line.empty())
        lines.push_back(line);
    }
  }

  if (lines.empty())
    return false;

  int parsedCount = 0;
  for (size_t i = 1; i < lines.size(); ++i) {  // Skip header
    const std::string &line = lines[i];
    std::vector<std::string> parts;
    size_t commaPos = 0, lastComma = 0;

    while ((commaPos = line.find(',', lastComma)) != std::string::npos) {
      std::string part = line.substr(lastComma, commaPos - lastComma);
      // Trim
      size_t start = part.find_first_not_of(" \t");
      if (start != std::string::npos) {
        size_t end = part.find_last_not_of(" \t");
        part = part.substr(start, end - start + 1);
      }
      parts.push_back(part);
      lastComma = commaPos + 1;
    }
    std::string lastPart = line.substr(lastComma);
    size_t start = lastPart.find_first_not_of(" \t");
    if (start != std::string::npos) {
      size_t end = lastPart.find_last_not_of(" \t");
      lastPart = lastPart.substr(start, end - start + 1);
    }
    parts.push_back(lastPart);

    if (parts.size() < 7)
      continue;

    if (_stricmp(parts[1].c_str(), targetSymbol) != 0)
      continue;

    DseBar bar;
    memset(&bar, 0, sizeof(bar));
    bar.valid = true;

    std::string dateStr = parts[0];
    if (dateStr.size() >= 10 && dateStr[4] == '-') {
      bar.year = atoi(dateStr.substr(0, 4).c_str());
      bar.month = atoi(dateStr.substr(5, 2).c_str());
      bar.day = atoi(dateStr.substr(8, 2).c_str());
    } else if (dateStr.size() == 8) {
      bar.year = atoi(dateStr.substr(0, 4).c_str());
      bar.month = atoi(dateStr.substr(4, 2).c_str());
      bar.day = atoi(dateStr.substr(6, 2).c_str());
    } else {
      continue;
    }

    bar.open = atof(parts[2].c_str());
    bar.high = atof(parts[3].c_str());
    bar.low = atof(parts[4].c_str());
    bar.close = atof(parts[5].c_str());
    bar.volume = atof(parts[6].c_str());

    if (parts.size() >= 9) {
      bar.value = atof(parts[7].c_str());
      bar.trade = atof(parts[8].c_str());
    }

    // Validate
    const double MIN_PRICE = 0.01;
    bar.valid = (bar.open >= MIN_PRICE && bar.high >= MIN_PRICE &&
                 bar.low >= MIN_PRICE && bar.close >= MIN_PRICE &&
                 bar.high >= bar.low);

    if (bar.valid) {
      outBars.push_back(bar);
      parsedCount++;
    }
  }

  return parsedCount > 0;
}

} // namespace CsvUtils
