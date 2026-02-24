///////////////////////////////////////////////////////////////////////////
// DseTypes.h — Common Data Structures for DSE Data Plugin
//
// Shared data structures used across all plugin modules.
///////////////////////////////////////////////////////////////////////////

#ifndef DSE_TYPES_H
#define DSE_TYPES_H

///////////////////////////////////////////////////////////////////////////
// A single OHLCV bar parsed from DSE
///////////////////////////////////////////////////////////////////////////
struct DseBar {
  int year, month, day;
  double open, high, low, close;
  double volume;
  double trade;   // number of trades
  double value;   // turnover value
  bool valid;     // false if bad tick
};

///////////////////////////////////////////////////////////////////////////
// Real-time quote for a single instrument
///////////////////////////////////////////////////////////////////////////
struct DseQuote {
  char symbol[32];
  double ltp;         // Last Traded Price
  double high;
  double low;
  double open;
  double close;       // previous close / closing price
  double ycp;         // yesterday's closing price
  double change;
  double changePercent;
  double volume;
  double trade;
  double value;
  bool valid;
};

///////////////////////////////////////////////////////////////////////////
// Configuration loaded from INI
///////////////////////////////////////////////////////////////////////////
struct DseConfig {
  int historyDays;
  int pollIntervalMs;
  int marketOpenHour, marketOpenMinute;
  int marketCloseHour, marketCloseMinute;
  int maxReconnectAttempts;
  int httpTimeoutSec;
  char userAgent[256];
  bool preferWebData;   // Priority for Web vs Local data
  char latestPriceUrl[512];
  char dayEndArchiveUrl[512];
  char altLatestPriceUrl[512];
  bool enableLogging;
  char logFilePath[260];
  char csvSeedPath[512];
  char exportPath[512];
  int exportIntervalSec;
};

///////////////////////////////////////////////////////////////////////////
// Connection state enum
///////////////////////////////////////////////////////////////////////////
enum ConnectionState {
  CONN_DISCONNECTED = 0,
  CONN_CONNECTING = 1,
  CONN_CONNECTED = 2,
  CONN_RECONNECTING = 3,
  CONN_ERROR = 4
};

#endif // DSE_TYPES_H
