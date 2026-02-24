///////////////////////////////////////////////////////////////////////////
// HtmlUtils.cpp — HTML Parsing Utilities Implementation
///////////////////////////////////////////////////////////////////////////

#include "HtmlUtils.h"
#include <algorithm>
#include <cctype>

namespace HtmlUtils {

std::string StripHtml(const std::string &input) {
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

std::string Trim(const std::string &s) {
  size_t start = s.find_first_not_of(" \t\r\n");
  if (start == std::string::npos)
    return "";
  size_t end = s.find_last_not_of(" \t\r\n");
  return s.substr(start, end - start + 1);
}

double SafeStod(const std::string &s, double fallback) {
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

std::string ExtractTargetTable(const std::string &html) {
  // Strategy 1: Look for "shares-table" class
  size_t tableStart = std::string::npos;
  size_t searchPos = 0;

  while (true) {
    size_t found = html.find("shares-table", searchPos);
    if (found == std::string::npos)
      break;

    size_t tagOpen = html.rfind("<table", found);
    if (tagOpen != std::string::npos) {
      tableStart = tagOpen;
      break;
    }
    searchPos = found + 12;
  }

  // Strategy 2: Fallback — Look for table with specific headers
  if (tableStart == std::string::npos) {
    size_t pos = 0;
    while (true) {
      size_t tStart = html.find("<table", pos);
      if (tStart == std::string::npos)
        break;

      size_t tEnd = html.find("</table>", tStart);
      if (tEnd == std::string::npos)
        break;

      std::string tableContent = html.substr(tStart, tEnd - tStart);
      std::string upper = tableContent;
      for (auto &c : upper)
        c = static_cast<char>(toupper(static_cast<unsigned char>(c)));

      if (upper.find("DATE") != std::string::npos &&
          upper.find("VOLUME") != std::string::npos &&
          (upper.find("CLOSE") != std::string::npos ||
           upper.find("LTP") != std::string::npos)) {
        tableStart = tStart;
        break;
      }
      pos = tEnd + 8;
    }
  }

  if (tableStart == std::string::npos)
    return html;

  size_t tableEnd = html.find("</table>", tableStart);
  if (tableEnd == std::string::npos)
    return html.substr(tableStart);

  return html.substr(tableStart, tableEnd - tableStart + 8);
}

std::vector<std::string> ExtractTableRows(const std::string &html) {
  std::vector<std::string> rows;
  size_t pos = 0;

  while (true) {
    size_t trStart = std::string::npos;
    for (size_t i = pos; i < html.size() - 3; ++i) {
      if ((html[i] == '<') &&
          (tolower(static_cast<unsigned char>(html[i + 1])) == 't') &&
          (tolower(static_cast<unsigned char>(html[i + 2])) == 'r') &&
          (html[i + 3] == ' ' || html[i + 3] == '>' || html[i + 3] == '\t')) {
        trStart = i;
        break;
      }
    }

    if (trStart == std::string::npos)
      break;

    size_t trEnd = std::string::npos;
    for (size_t i = trStart + 3; i < html.size() - 4; ++i) {
      if ((html[i] == '<') && (html[i + 1] == '/') &&
          (tolower(static_cast<unsigned char>(html[i + 2])) == 't') &&
          (tolower(static_cast<unsigned char>(html[i + 3])) == 'r') &&
          (html[i + 4] == '>')) {
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

std::vector<std::string> ParseTableRow(const std::string &row) {
  std::vector<std::string> cells;
  size_t pos = 0;

  while (true) {
    size_t tdStart = std::string::npos;
    bool isTh = false;

    for (size_t i = pos; i < row.size() - 3; ++i) {
      if (row[i] == '<') {
        char t1 = static_cast<char>(tolower(static_cast<unsigned char>(row[i + 1])));
        char t2 = static_cast<char>(tolower(static_cast<unsigned char>(row[i + 2])));
        char t3 = row[i + 3];

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

    size_t contentStart = row.find('>', tdStart);
    if (contentStart == std::string::npos)
      break;
    contentStart++;

    size_t tdEnd = std::string::npos;
    for (size_t i = contentStart; i < row.size() - 4; ++i) {
      if (row[i] == '<' && row[i + 1] == '/') {
        char t1 = static_cast<char>(tolower(static_cast<unsigned char>(row[i + 2])));
        char t2 = static_cast<char>(tolower(static_cast<unsigned char>(row[i + 3])));
        if (t1 == 't' && (t2 == 'd' || t2 == 'h') && row[i + 4] == '>') {
          tdEnd = i;
          break;
        }
      }
    }

    if (tdEnd == std::string::npos)
      break;

    std::string cellContent = row.substr(contentStart, tdEnd - contentStart);
    cellContent = Trim(StripHtml(cellContent));
    cells.push_back(cellContent);

    pos = tdEnd + 5;
  }

  return cells;
}

} // namespace HtmlUtils
