///////////////////////////////////////////////////////////////////////////
// HtmlUtils.h — HTML Parsing Utilities for DSE Data Plugin
//
// Provides reusable HTML table parsing functions used by DseDataEngine.
///////////////////////////////////////////////////////////////////////////

#ifndef HTML_UTILS_H
#define HTML_UTILS_H

#include <string>
#include <vector>

namespace HtmlUtils {

  /// Extract the main data table from HTML by looking for known class names
  std::string ExtractTargetTable(const std::string &html);

  /// Extract all <tr>...</tr> blocks from HTML
  std::vector<std::string> ExtractTableRows(const std::string &html);

  /// Parse a single <tr> row into cell values (strips <td> tags)
  std::vector<std::string> ParseTableRow(const std::string &row);

  /// Strip all HTML tags from a string
  std::string StripHtml(const std::string &input);

  /// Trim whitespace from both ends of a string
  std::string Trim(const std::string &s);

  /// Safe string to double conversion (handles commas in numbers)
  double SafeStod(const std::string &s, double fallback = 0.0);

} // namespace HtmlUtils

#endif // HTML_UTILS_H
