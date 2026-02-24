///////////////////////////////////////////////////////////////////////////
// CsvUtils.h — CSV Handling Utilities for DSE Data Plugin
//
// Provides CSV seed loading and export functionality.
///////////////////////////////////////////////////////////////////////////

#ifndef CSV_UTILS_H
#define CSV_UTILS_H

#include "DseTypes.h"
#include <string>
#include <vector>
#include <map>

namespace CsvUtils {

  /// Load historical bars from a CSV seed file
  /// Supports two formats:
  ///   Format A: Trading_Code,Date,Open,High,Low,Close,Volume
  ///   Format B: Date,Open,High,Low,Close,Volume
  bool LoadCsvSeed(const char *symbol, const char *csvSeedPath,
                   std::vector<DseBar> &outBars);

  /// Export cached bars to a CSV file
  bool ExportBarsToCsv(const char *symbol, const char *exportPath,
                       const std::vector<DseBar> &bars);

  /// Export all cached data to CSV files (one file per symbol)
  int ExportAllDataToCsv(const std::map<std::string, std::vector<DseBar>> &cache,
                         const char *exportPath);

  /// Parse Amarstock CSV response into bars
  bool ParseAmarstockCsv(const std::string &csv, const char *targetSymbol,
                         std::vector<DseBar> &outBars);

} // namespace CsvUtils

#endif // CSV_UTILS_H
