///////////////////////////////////////////////////////////////////////////
// Plugin.cpp — Main Plugin Implementation
//
// [Explanation for non-developers]:
// This file is the main "bridge" between AmiBroker (the trading software) and
// our custom data downloader. It handles starting up the plugin, showing the
// settings window when you click "Configure", and passing the downloaded stock
// charts and live prices directly into AmiBroker's charts.
//
// Handles AmiBroker interaction, configuration dialog, and data delivery.
///////////////////////////////////////////////////////////////////////////

#include "Plugin.h"
#include "DseDataEngine.h"
#include "RealtimeFeed.h"
#include <commctrl.h>
#include <mutex>
#include <set>
#include <shlobj.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <windows.h>

// Global instances
PluginInfo g_pluginInfo = {0};
DseDataEngine g_engine;
RealtimeFeed g_feed;

char g_configPath[MAX_PATH] = {0};
char g_dbPath[MAX_PATH] = {0};
char g_currentSymbol[64] = {0};
HWND g_hAmiBrokerWnd = NULL;
bool g_initialized = false;
int g_timeBase = 0; // 0 = EOD, <86400 = Intraday

HINSTANCE g_hInstance = NULL;

///////////////////////////////////////////////////////////////////////////
// DLL Entry Point
///////////////////////////////////////////////////////////////////////////

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call,
                      LPVOID lpReserved) {
  switch (ul_reason_for_call) {
  case DLL_PROCESS_ATTACH:
    g_hInstance = (HINSTANCE)hModule;
    break;
  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
  case DLL_PROCESS_DETACH:
    break;
  }
  return TRUE;
}

///////////////////////////////////////////////////////////////////////////
// Helpers
///////////////////////////////////////////////////////////////////////////

static void FindConfigPath() {
  if (g_configPath[0])
    return;

  // 1. Try plugin directory/config
  char path[MAX_PATH];
  GetModuleFileNameA(g_hInstance, path, MAX_PATH);
  char *p = strrchr(path, '\\');
  if (p) {
    *p = 0; // Remove DLL filename
    strcat_s(path, "\\config\\dse_config.ini");
    if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) {
      strcpy_s(g_configPath, path);
      return;
    }
  }

  // 2. Try just "config" relative to CWD
  if (GetFileAttributesA("config\\dse_config.ini") != INVALID_FILE_ATTRIBUTES) {
    strcpy_s(g_configPath, "config\\dse_config.ini");
    return;
  }

  // 3. Fallback to default path relative to DLL
  if (p) {
    // path is currently DLL dir
    strcat_s(path, "\\config\\dse_config.ini");
    strcpy_s(g_configPath, path); // Use it even if not exists (will create)
  }
}

static std::string DateDaysAgo(int days) {
  SYSTEMTIME st;
  GetLocalTime(&st);

  FILETIME ft;
  SystemTimeToFileTime(&st, &ft);

  ULARGE_INTEGER ul;
  ul.LowPart = ft.dwLowDateTime;
  ul.HighPart = ft.dwHighDateTime;

  // Subtract 'days' (864000000000 100-ns intervals per day)
  ul.QuadPart -= (10000000ULL * 60 * 60 * 24) * days;

  ft.dwLowDateTime = ul.LowPart;
  ft.dwHighDateTime = ul.HighPart;
  FileTimeToSystemTime(&ft, &st);

  char buf[16];
  sprintf_s(buf, "%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
  return std::string(buf);
}

static std::string DateToday() {
  SYSTEMTIME st;
  GetLocalTime(&st);

  char buf[16];
  sprintf_s(buf, "%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
  return std::string(buf);
}

///////////////////////////////////////////////////////////////////////////
// Helper: Trigger lazy backfill for a symbol
//
// [Explanation for non-developers]:
// When you open a chart for a new stock (like GP or BRACBANK), we might not
// have its historical data yet. Instead of freezing your AmiBroker screen while
// it downloads 3 years of data, this function downloads the missing data
// invisibly in the background. Once it finishes downloading, it tells the chart
// to refresh.
///////////////////////////////////////////////////////////////////////////

static void LazyBackfill(const char *symbol) {
  if (!symbol || !symbol[0])
    return;

  // Check if we already have cached data
  std::vector<DseBar> existing;
  if (g_engine.GetCachedBars(symbol, existing) && !existing.empty()) {
    // Already have data — just refresh the latest
    std::string today = DateToday();

    // Find the last cached date
    const DseBar &last = existing.back();
    char lastDate[16];
    sprintf_s(lastDate, "%04d-%02d-%02d", last.year, last.month, last.day);

    // Fetch from last cached date to today
    std::vector<DseBar> newBars;
    g_engine.FetchHistoricalData(
        symbol, lastDate, today.c_str(), newBars, [symbol]() {
          // streaming update to AB
          if (g_hAmiBrokerWnd && IsWindow(g_hAmiBrokerWnd)) {
            g_engine.Log("DEBUG: LazyBackfill (incremental) - Firing "
                         "WM_USER_STREAMING_UPDATE for symbol: %s",
                         symbol);
            RecentInfo *ri = new RecentInfo;
            memset(ri, 0, sizeof(RecentInfo));
            ri->nStructSize = sizeof(RecentInfo);
            strncpy_s(ri->Name, symbol,
                      sizeof(ri->Name) - 1); // Pass the actual symbol
            ri->nStatus = 1;
            ri->nBitmap = 0xFFFF; // Tell AmiBroker to process this update
            PostMessage(g_hAmiBrokerWnd, WM_USER_STREAMING_UPDATE,
                        (WPARAM)ri->Name, (LPARAM)ri);
          } else {
            g_engine.Log("DEBUG: LazyBackfill (incremental) - Cannot fire "
                         "update, g_hAmiBrokerWnd is NULL or invalid!");
          }
        });

    g_engine.Log("LazyBackfill: incremental update for %s, "
                 "%zu new bars",
                 symbol, newBars.size());
  } else {
    // No cached data — full backfill
    int days = g_engine.GetConfig().historyDays;
    std::string startDate = DateDaysAgo(days);
    std::string endDate = DateToday();

    std::vector<DseBar> bars;
    g_engine.FetchHistoricalData(
        symbol, startDate.c_str(), endDate.c_str(), bars, [symbol]() {
          // streaming update to AB
          if (g_hAmiBrokerWnd && IsWindow(g_hAmiBrokerWnd)) {
            g_engine.Log("DEBUG: LazyBackfill (full backfill) - Firing "
                         "WM_USER_STREAMING_UPDATE for symbol: %s",
                         symbol);
            RecentInfo *ri = new RecentInfo;
            memset(ri, 0, sizeof(RecentInfo));
            ri->nStructSize = sizeof(RecentInfo);
            strncpy_s(ri->Name, symbol,
                      sizeof(ri->Name) - 1); // Pass the actual symbol
            ri->nStatus = 1;
            ri->nBitmap = 0xFFFF; // Tell AmiBroker to process this update
            PostMessage(g_hAmiBrokerWnd, WM_USER_STREAMING_UPDATE,
                        (WPARAM)ri->Name, (LPARAM)ri);
          } else {
            g_engine.Log("DEBUG: LazyBackfill (full backfill) - Cannot fire "
                         "update, g_hAmiBrokerWnd is NULL or invalid!");
          }
        });

    g_engine.Log("LazyBackfill: full backfill for %s, "
                 "%zu bars (%d days)",
                 symbol, bars.size(), days);
  }
}

struct BackfillParams {
  char symbol[64];
};

static DWORD WINAPI BackfillThreadProc(LPVOID lpParam) {
  BackfillParams *p = (BackfillParams *)lpParam;
  char symbol[64];
  strncpy_s(symbol, p->symbol, sizeof(symbol) - 1);
  symbol[sizeof(symbol) - 1] = '\0';
  LazyBackfill(p->symbol);
  delete p;

  // Tell AmiBroker to refresh
  if (g_hAmiBrokerWnd && IsWindow(g_hAmiBrokerWnd)) {
    g_engine.Log("DEBUG: BackfillThreadProc done, posting final "
                 "WM_USER_STREAMING_UPDATE");
    RecentInfo *ri = new RecentInfo;
    memset(ri, 0, sizeof(RecentInfo));
    ri->nStructSize = sizeof(RecentInfo);
    strncpy_s(ri->Name, symbol, sizeof(ri->Name) - 1); // Pass the actual symbol
    ri->nStatus = 1;
    ri->nBitmap = 0xFFFF; // Tell AmiBroker to process this update
    PostMessage(g_hAmiBrokerWnd, WM_USER_STREAMING_UPDATE, (WPARAM)ri->Name,
                (LPARAM)ri);
  }
  return 0;
}

// Thread for handling bulk synchronization of all symbols
static DWORD WINAPI BulkSyncThreadProc(LPVOID lpParam) {
  int syncType = (int)(intptr_t)lpParam; // 1 = DSE, 2 = Amarstock
  g_engine.Log("BulkSync: Starting full database synchronization (Type %d)...",
               syncType);

  // We need a stable copy of the symbol list to iterate over
  std::vector<std::string> syms = g_engine.GetSymbolList();

  if (syms.empty()) {
    g_engine.RefreshSymbolList();
    syms = g_engine.GetSymbolList();
  }

  int successCount = 0;
  int processedTargetCount = 0;
  for (const auto &sym : syms) {
    bool isAmarstock = (_stricmp(sym.c_str(), "00DS30") == 0 ||
                        _stricmp(sym.c_str(), "00DSES") == 0 ||
                        _stricmp(sym.c_str(), "00DSEX") == 0 ||
                        _stricmp(sym.c_str(), "00DSMEX") == 0);

    if (syncType == 1 && isAmarstock)
      continue;
    if (syncType == 2 && !isAmarstock)
      continue;

    processedTargetCount++;

    g_engine.Log("BulkSync: Processing %s...", sym.c_str());

    // LazyBackfill will handle creating individual threads or synchronous
    // backfill To prevent 400 simultaneous threads, we call FetchHistoricalData
    // directly in this loop
    int days = g_engine.GetConfig().historyDays;
    std::string startDate = DateDaysAgo(days);
    std::string endDate = DateToday();

    std::vector<DseBar> bars;
    bool success = g_engine.FetchHistoricalData(sym.c_str(), startDate.c_str(),
                                                endDate.c_str(), bars, nullptr);

    if (success && !bars.empty()) {
      successCount++;
    }

    // Optional: Sleep briefly between symbols to avoid hammering DSE/Amarstock
    // servers too aggressively
    Sleep(100);
  }

  char msg[256];
  sprintf_s(msg,
            "Bulk Background Sync Finished!\nSuccessfully processed %d out of "
            "%d targeted symbols.",
            successCount, processedTargetCount);

  // Pop up a message box onto AmiBroker informing the user it's done
  if (g_hAmiBrokerWnd) {
    MessageBoxA(g_hAmiBrokerWnd, msg, "DSE Plugin - Auto Sync",
                MB_OK | MB_ICONINFORMATION);
  }

  g_engine.Log("BulkSync: Finished full database synchronization.");
  return 0;
}

PLUGINAPI int GetPluginInfo(struct PluginInfo *pInfo) {
  memset(pInfo, 0, sizeof(PluginInfo));
  pInfo->nStructSize = sizeof(PluginInfo);
  pInfo->nType = 2; // PLUGIN_TYPE_DATA
  pInfo->nVersion =
      10000; // v1.0.0 (coded as MAJOR*10000 + MINOR*100 + RELEASE)
  pInfo->nIDCode = 0xD5E1; // Unique ID
  pInfo->nCertificate = 0;
  pInfo->nMinAmiVersion = 52700; // Requires AmiBroker 5.27+ (coded as 527*100)

  strcpy_s(pInfo->szName, "DSE Data Plugin");
  strcpy_s(pInfo->szVendor, "AmiBroker DSE Community");

  return 1; // TRUE
}

///////////////////////////////////////////////////////////////////////////
// Date/Time Helpers for 64-bit AmiDate
///////////////////////////////////////////////////////////////////////////

AmiDate PackAmiDate(int year, int month, int day, int hour = 0, int minute = 0,
                    int second = 0) {
  AmiDate d;
  d.Date = 0; // Clear all bits including milliseconds/reserved
  d.PackDate.Year = year;
  d.PackDate.Month = month;
  d.PackDate.Day = day;
  d.PackDate.Hour = hour;
  d.PackDate.Minute = minute;
  d.PackDate.Second = second;
  return d;
}

void UnpackAmiDate(AmiDate d, int *year, int *month, int *day, int *hour,
                   int *minute, int *second) {
  if (year)
    *year = d.PackDate.Year;
  if (month)
    *month = d.PackDate.Month;
  if (day)
    *day = d.PackDate.Day;
  if (hour)
    *hour = d.PackDate.Hour;
  if (minute)
    *minute = d.PackDate.Minute;
  if (second)
    *second = d.PackDate.Second;
}

///////////////////////////////////////////////////////////////////////////
// Init — Initialize the plugin
//
// [Explanation for non-developers]:
// This block runs when AmiBroker starts up and loads our plugin. It finds your
// saved settings (like your poll interval) and starts the "data engine" which
// is responsible for pulling data from the internet.
///////////////////////////////////////////////////////////////////////////

extern "C" __declspec(dllexport) int Init(void) {
  if (g_initialized)
    return 1;

  g_engine.Log("Plugin::Init — starting");

  // Find configuration file
  FindConfigPath();

  // Initialize the data engine
  if (!g_engine.Initialize(g_configPath)) {
    g_engine.Log("Plugin::Init — engine initialization failed");
    // Continue anyway with defaults
  }

  // FORCE ENABLE LOGGING FOR DEBUGGING
  g_engine.Log("Plugin::Init - FORCING DEBUG LOGGING ON");

  g_initialized = true;

  // Start the real-time polling feed if the AB window handle is already known
  // (it may not be at this point — Notify(REASON_DATABASE_LOADED) will also
  // attempt to start it when the handle becomes available).
  if (g_hAmiBrokerWnd && IsWindow(g_hAmiBrokerWnd)) {
    if (!g_feed.IsRunning()) {
      g_engine.Log("Plugin::Init — starting realtime feed");
      g_feed.Start(g_hAmiBrokerWnd, &g_engine);
    }
  }

  g_engine.Log("Plugin::Init — complete");

  return 1; // Success
}

///////////////////////////////////////////////////////////////////////////
// Release — Cleanup the plugin
///////////////////////////////////////////////////////////////////////////

extern "C" __declspec(dllexport) int Release(void) {
  g_engine.Log("Plugin::Release — shutting down");

  // Stop real-time feed first
  g_feed.Stop();

  // Shutdown the data engine
  g_engine.Shutdown();

  g_initialized = false;
  return 1;
}

///////////////////////////////////////////////////////////////////////////
// Configure — Show settings dialog
//
// [Explanation for non-developers]:
// This block populates the pop-up window you see when you click "Configure" in
// AmiBroker. It builds all the text boxes, buttons, and checkboxes for you to
// customize how many years of history to download, how fast to poll for live
// prices, and where to save backup files.
///////////////////////////////////////////////////////////////////////////

// Dialog resource IDs (using programmatic dialog creation)
#define ID_STATIC_YEARS 1001
#define ID_EDIT_YEARS 1002
#define ID_STATIC_POLL 1003
#define ID_EDIT_POLL 1004
#define ID_STATIC_STATUS 1005
#define ID_CHK_PREFER_WEB 1006
#define ID_EDIT_EXPORT_PATH 1007
#define ID_EDIT_EXPORT_INT 1008
#define ID_BTN_SAVE_NOW 1009
#define ID_BTN_OK IDOK
#define ID_BTN_CANCEL IDCANCEL
#define ID_BTN_REFRESH 1010
#define ID_BTN_SYNC 1011
#define ID_BTN_BROWSE 1012
#define ID_BTN_SYNC_INDICES 1013

// Helper function for adding controls
static void AddCtrl(WORD *&p, DWORD style, short x, short y, short cx, short cy,
                    WORD id, const wchar_t *cls, const wchar_t *txt) {
  DLGITEMTEMPLATE *pItem = (DLGITEMTEMPLATE *)p;
  pItem->style = style | WS_CHILD | WS_VISIBLE;
  pItem->dwExtendedStyle = 0;
  pItem->x = x;
  pItem->y = y;
  pItem->cx = cx;
  pItem->cy = cy;
  pItem->id = id;
  p += sizeof(DLGITEMTEMPLATE) / sizeof(WORD);

  // Class
  size_t clsLen = wcslen(cls) + 1;
  memcpy(p, cls, clsLen * sizeof(WORD));
  p += clsLen;

  // Text
  size_t txtLen = wcslen(txt) + 1;
  memcpy(p, txt, txtLen * sizeof(WORD));
  p += txtLen;

  // Extra data
  *p++ = 0;

  // Align to DWORD boundary for next control
  if ((ULONG_PTR)p & 2)
    p++;
}

static INT_PTR CALLBACK ConfigDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                      LPARAM lParam) {
  switch (msg) {
  case WM_INITDIALOG: {
    // Store pSite in user data
    SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam);

    // Fill current config values
    char buf[64];
    sprintf_s(buf, "%d", g_engine.GetConfig().historyDays);
    SetDlgItemTextA(hDlg, ID_EDIT_YEARS, buf);

    sprintf_s(buf, "%d", g_engine.GetConfig().pollIntervalMs);
    SetDlgItemTextA(hDlg, ID_EDIT_POLL, buf);

    // Connection status
    const char *statusText = (g_engine.GetConnectionState() == CONN_CONNECTED)
                                 ? "Connected"
                                 : "Disconnected";
    SetDlgItemTextA(hDlg, ID_STATIC_STATUS, statusText);

    // Set Checkbox
    CheckDlgButton(hDlg, ID_CHK_PREFER_WEB,
                   g_engine.GetConfig().preferWebData ? BST_CHECKED
                                                      : BST_UNCHECKED);

    // Set Export Path
    SetDlgItemTextA(hDlg, ID_EDIT_EXPORT_PATH, g_engine.GetConfig().exportPath);

    // Set Export Interval
    sprintf_s(buf, "%d", g_engine.GetConfig().exportIntervalSec);
    SetDlgItemTextA(hDlg, ID_EDIT_EXPORT_INT, buf);

    return TRUE;
  }

  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case ID_BTN_SAVE_NOW: {
      // Manual save
      char path[MAX_PATH];
      GetDlgItemTextA(hDlg, ID_EDIT_EXPORT_PATH, path, sizeof(path));

      // Save just this setting temporarily to config file so engine can reload
      WritePrivateProfileStringA("Export", "ExportPath", path, g_configPath);
      g_engine.Initialize(g_configPath); // Reloads config

      g_engine.ExportAllDataToCsv();
      MessageBoxA(hDlg, "Export started! Check logs/files.", "DSE Plugin",
                  MB_OK);
      return TRUE;
    }

    case ID_BTN_BROWSE: {
      BROWSEINFOA bi = {0};
      bi.hwndOwner = hDlg;
      bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
      bi.lpszTitle = "Select Export Folder";
      LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
      if (pidl) {
        char path[MAX_PATH];
        if (SHGetPathFromIDListA(pidl, path)) {
          SetDlgItemTextA(hDlg, ID_EDIT_EXPORT_PATH, path);
        }
        CoTaskMemFree(pidl);
      }
      return TRUE;
    }

    case ID_BTN_OK: {
      // Read values and save
      char buf[32];
      GetDlgItemTextA(hDlg, ID_EDIT_YEARS, buf, sizeof(buf));
      int days = atoi(buf);
      if (days < 1)
        days = 1;
      if (days > 7300)
        days = 7300; // Cap at 20 years

      GetDlgItemTextA(hDlg, ID_EDIT_POLL, buf, sizeof(buf));
      int pollMs = atoi(buf);
      if (pollMs < 1000)
        pollMs = 1000;
      if (pollMs > 60000)
        pollMs = 60000;

      int preferWeb =
          (IsDlgButtonChecked(hDlg, ID_CHK_PREFER_WEB) == BST_CHECKED) ? 1 : 0;

      // Save to INI
      sprintf_s(buf, "%d", days);
      WritePrivateProfileStringA("Settings", "HistoryDays", buf, g_configPath);

      sprintf_s(buf, "%d", pollMs);
      WritePrivateProfileStringA("General", "PollIntervalMs", buf,
                                 g_configPath);

      sprintf_s(buf, "%d", preferWeb);
      WritePrivateProfileStringA("General", "PreferWebData", buf, g_configPath);

      // Save Export Config
      char exportPath[MAX_PATH];
      GetDlgItemTextA(hDlg, ID_EDIT_EXPORT_PATH, exportPath,
                      sizeof(exportPath));
      WritePrivateProfileStringA("Export", "ExportPath", exportPath,
                                 g_configPath);

      char exportIntBuf[32];
      GetDlgItemTextA(hDlg, ID_EDIT_EXPORT_INT, exportIntBuf,
                      sizeof(exportIntBuf));
      WritePrivateProfileStringA("Export", "ExportIntervalSec", exportIntBuf,
                                 g_configPath);

      // Reload config
      g_engine.Initialize(g_configPath);

      // Update feed interval
      g_feed.SetPollInterval(pollMs);

      EndDialog(hDlg, IDOK);
      return TRUE;
    }

    case ID_BTN_CANCEL:
      EndDialog(hDlg, IDCANCEL);
      return TRUE;

    case ID_BTN_REFRESH: {
      // Force refresh symbol list
      g_engine.RefreshSymbolList();
      auto &syms = g_engine.GetSymbolList();

      char msg[128];
      sprintf_s(msg, "Refreshed: %zu symbols found", syms.size());
      SetDlgItemTextA(hDlg, ID_STATIC_STATUS, msg);
      return TRUE;
    }

    case ID_BTN_SYNC: {
      InfoSite *pSite = (InfoSite *)GetWindowLongPtr(hDlg, GWLP_USERDATA);
      if (!pSite || !pSite->AddStockNew) {
        MessageBoxA(hDlg, "Internal Error: AddStockNew not available.",
                    "DSE Plugin", MB_OK | MB_ICONERROR);
        return TRUE;
      }

      const std::vector<std::string> *pSyms = &g_engine.GetSymbolList();
      if (pSyms->empty()) {
        g_engine.RefreshSymbolList();
        pSyms = &g_engine.GetSymbolList();
      }

      if (pSyms->empty()) {
        MessageBoxA(hDlg, "No symbols found to sync. Try Refresh first.",
                    "DSE Plugin", MB_OK | MB_ICONWARNING);
        return TRUE;
      }

      int added = 0;
      int targetCount = 0;
      for (const auto &sym : *pSyms) {
        bool isAmarstock = (_stricmp(sym.c_str(), "00DS30") == 0 ||
                            _stricmp(sym.c_str(), "00DSES") == 0 ||
                            _stricmp(sym.c_str(), "00DSEX") == 0 ||
                            _stricmp(sym.c_str(), "00DSMEX") == 0);
        if (isAmarstock)
          continue; // Only process DSEBD symbols

        if (pSite->AddStockNew(sym.c_str())) {
          added++;
        }
        targetCount++;
      }

      // Launch the background thread to perform bulk fetching -> parameter 1
      // for DSE
      HANDLE hThread = CreateThread(NULL, 0, BulkSyncThreadProc,
                                    (LPVOID)(intptr_t)1, 0, NULL);
      if (hThread) {
        CloseHandle(hThread); // We don't need to join it
      }

      char msg[256];
      sprintf_s(
          msg,
          "Verified %d DSEbd symbols in AmiBroker.\n\nA background bulk-sync "
          "has started to download past data for all %d DSEbd symbols.\n\nYou "
          "may close this window. You will receive another popup when "
          "it finishes.",
          added, targetCount);
      MessageBoxA(hDlg, msg, "DSE Plugin Sync Started",
                  MB_OK | MB_ICONINFORMATION);
      return TRUE;
    }

    case ID_BTN_SYNC_INDICES: {
      InfoSite *pSite = (InfoSite *)GetWindowLongPtr(hDlg, GWLP_USERDATA);
      if (!pSite || !pSite->AddStockNew) {
        MessageBoxA(hDlg, "Internal Error: AddStockNew not available.",
                    "DSE Plugin", MB_OK | MB_ICONERROR);
        return TRUE;
      }

      const std::vector<std::string> *pSyms = &g_engine.GetSymbolList();
      if (pSyms->empty()) {
        g_engine.RefreshSymbolList();
        pSyms = &g_engine.GetSymbolList();
      }

      int added = 0;
      int targetCount = 0;
      for (const auto &sym : *pSyms) {
        bool isAmarstock = (_stricmp(sym.c_str(), "00DS30") == 0 ||
                            _stricmp(sym.c_str(), "00DSES") == 0 ||
                            _stricmp(sym.c_str(), "00DSEX") == 0 ||
                            _stricmp(sym.c_str(), "00DSMEX") == 0);
        if (!isAmarstock)
          continue; // Only process Amarstock symbols

        if (pSite->AddStockNew(sym.c_str())) {
          added++;
        }
        targetCount++;
      }

      // Launch the background thread to perform bulk fetching -> parameter 2
      // for Amarstock
      HANDLE hThread = CreateThread(NULL, 0, BulkSyncThreadProc,
                                    (LPVOID)(intptr_t)2, 0, NULL);
      if (hThread) {
        CloseHandle(hThread); // We don't need to join it
      }

      char msg[256];
      sprintf_s(msg,
                "Verified %d Amarstock indices in AmiBroker.\n\nA background "
                "bulk-sync "
                "has started to download past data for all %d Amarstock "
                "indices.\n\nYou "
                "may close this window. You will receive another popup when "
                "it finishes.",
                added, targetCount);
      MessageBoxA(hDlg, msg, "Amarstock Sync Started",
                  MB_OK | MB_ICONINFORMATION);
      return TRUE;
    }
    }
    break;

  case WM_CLOSE:
    EndDialog(hDlg, IDCANCEL);
    return TRUE;
  }

  return FALSE;
}

PLUGINAPI int Configure(LPCTSTR pszPath, struct InfoSite *pSite) {
  // Store database path
  if (pszPath) {
    strcpy_s(g_dbPath, pszPath);
  }

  HWND hParentWnd = GetActiveWindow();

  // Template buffer
  WORD dlgTemplate[2048];
  memset(dlgTemplate, 0, sizeof(dlgTemplate));

  WORD *p = dlgTemplate;

  // DLGTEMPLATE header
  DLGTEMPLATE *pDlg = (DLGTEMPLATE *)p;
  pDlg->style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_CENTER |
                WS_VISIBLE;
  pDlg->dwExtendedStyle = 0;
  pDlg->cdit = 18; // Number of controls
  pDlg->x = 0;
  pDlg->y = 0;
  pDlg->cx = 300;
  pDlg->cy = 266;
  p += sizeof(DLGTEMPLATE) / sizeof(WORD);

  // Menu (none)
  *p++ = 0;
  // Class (default)
  *p++ = 0;
  // Title
  const wchar_t *title = L"DSE Data Plugin — Settings";
  size_t titleLen = wcslen(title) + 1;
  memcpy(p, title, titleLen * sizeof(WORD));
  p += titleLen;

  // Align to DWORD boundary
  if ((ULONG_PTR)p & 2)
    p++;

  // Align to DWORD boundary
  if ((ULONG_PTR)p & 2)
    p++;

  // Label: History Days
  AddCtrl(p, WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 12, 80, 10, ID_STATIC_YEARS,
          L"STATIC", L"History Days:");

  // Edit: History Days
  AddCtrl(p, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_NUMBER, 95, 10,
          30, 12, ID_EDIT_YEARS, L"EDIT", L"365");

  // Static: "Poll interval (ms):"
  AddCtrl(p, SS_LEFT, 10, 35, 85, 10, ID_STATIC_POLL, L"STATIC",
          L"Poll interval (ms):");

  // Edit: Poll interval
  AddCtrl(p, WS_BORDER | WS_TABSTOP | ES_NUMBER, 100, 33, 50, 12, ID_EDIT_POLL,
          L"EDIT", L"5000");

  // Static: "Status:"
  AddCtrl(p, SS_LEFT, 10, 60, 30, 10, (WORD)-1, L"STATIC", L"Status:");

  // Static: Connection status (dynamic)
  AddCtrl(p, SS_LEFT, 50, 60, 160, 10, ID_STATIC_STATUS, L"STATIC",
          L"Checking...");

  // Checkbox: Prefer Web Data
  AddCtrl(p, BS_AUTOCHECKBOX | WS_TABSTOP, 10, 64, 260, 12, ID_CHK_PREFER_WEB,
          L"BUTTON", L"Prefer Web Data (Overwrite Local)");

  // Refresh / Sync buttons
  AddCtrl(p, WS_TABSTOP | BS_PUSHBUTTON, 10, 84, 110, 14, ID_BTN_REFRESH,
          L"BUTTON", L"Refresh Symbols");
  AddCtrl(p, WS_TABSTOP | BS_PUSHBUTTON, 130, 84, 150, 14, ID_BTN_SYNC,
          L"BUTTON", L"Sync DSEbd Database");
  AddCtrl(p, WS_TABSTOP | BS_PUSHBUTTON, 130, 100, 150, 14, ID_BTN_SYNC_INDICES,
          L"BUTTON", L"Sync Amarstock Indices");

  // Export Path label + edit + Browse button
  AddCtrl(p, SS_LEFT, 10, 124, 60, 10, (WORD)-1, L"STATIC", L"Export Path:");
  AddCtrl(p, WS_BORDER | WS_TABSTOP, 10, 136, 240, 12, ID_EDIT_EXPORT_PATH,
          L"EDIT", L"");
  AddCtrl(p, WS_TABSTOP | BS_PUSHBUTTON, 255, 135, 35, 14, ID_BTN_BROWSE,
          L"BUTTON", L"...");

  // Auto-Save label + edit + Save Now button
  AddCtrl(p, SS_LEFT, 10, 158, 65, 10, (WORD)-1, L"STATIC", L"Auto-Save (s):");
  AddCtrl(p, WS_BORDER | WS_TABSTOP | ES_NUMBER, 78, 156, 35, 12,
          ID_EDIT_EXPORT_INT, L"EDIT", L"0");
  AddCtrl(p, WS_TABSTOP | BS_PUSHBUTTON, 120, 155, 100, 14, ID_BTN_SAVE_NOW,
          L"BUTTON", L"Save CSVs Now");

  // OK / Cancel
  AddCtrl(p, WS_TABSTOP | BS_DEFPUSHBUTTON, 150, 236, 60, 14, ID_BTN_OK,
          L"BUTTON", L"OK");
  AddCtrl(p, WS_TABSTOP | BS_PUSHBUTTON, 220, 236, 60, 14, ID_BTN_CANCEL,
          L"BUTTON", L"Cancel");

  // Show the dialog
  DialogBoxIndirectParamA(g_hInstance, (DLGTEMPLATE *)dlgTemplate, hParentWnd,
                          ConfigDlgProc, (LPARAM)pSite);

  return 1;
}

///////////////////////////////////////////////////////////////////////////
// GetQuotesEx — Deliver OHLCV data to AmiBroker
//
// [Explanation for non-developers]:
// This is arguably the most important function. AmiBroker constantly asks this
// function: "Hey, give me the bar data (Open, High, Low, Close, Volume) for
// this stock." This function looks at the data we've saved locally and hands it
// over to AmiBroker so it can draw the candlestick chart.
///////////////////////////////////////////////////////////////////////////

PLUGINAPI int GetQuotesEx(const char *pszTicker, int nPeriodicity,
                          int nLastValid, int nSize, struct Quotation *pQuotes,
                          GQEContext *pContext) {
  if (!pszTicker || !pszTicker[0] || !pQuotes || nSize <= 0)
    return (nLastValid < 0) ? 0 : nLastValid + 1;

  // Get cached bars
  std::vector<DseBar> bars;

  bool isAmarstock = (_stricmp(pszTicker, "00DS30") == 0 ||
                      _stricmp(pszTicker, "00DSES") == 0 ||
                      _stricmp(pszTicker, "00DSEX") == 0 ||
                      _stricmp(pszTicker, "00DSMEX") == 0);

  if (isAmarstock) {
    g_engine.Log("\n=======================================================");
    g_engine.Log("AMARSTOCK TRACE: GetQuotesEx requested for %s", pszTicker);
    g_engine.Log("AMARSTOCK TRACE: Request details -> nSize=%d, nLastValid=%d",
                 nSize, nLastValid);
  } else {
    g_engine.Log("DEBUG: GetQuotesEx called for %s, nSize=%d", pszTicker,
                 nSize);
  }

  g_engine.GetCachedBars(pszTicker, bars);

  // Count how many bars are actually valid
  int validCacheCount = 0;
  for (const auto &b : bars) {
    if (b.valid)
      validCacheCount++;
  }

  // Check if we need to fetch data (empty OR only contains the dummy/invalid
  // bar)
  if (validCacheCount == 0) {
    g_engine.Log("DEBUG: GetQuotesEx - No cached data for %s!", pszTicker);
    // No cached data — trigger backfill in background.
    // Use a set so every unique symbol gets exactly one backfill thread,
    // not just the first symbol ever seen (the old static-char bug).
    static std::set<std::string> s_enqueuedBackfills;
    static std::mutex s_backfillMutex;
    {
      std::lock_guard<std::mutex> lock(s_backfillMutex);
      std::string sym(pszTicker);
      if (s_enqueuedBackfills.find(sym) == s_enqueuedBackfills.end()) {
        s_enqueuedBackfills.insert(sym);
        g_engine.Log("GetQuotesEx: triggering background backfill for %s",
                     pszTicker);
        BackfillParams *p = new BackfillParams;
        strncpy_s(p->symbol, pszTicker, sizeof(p->symbol) - 1);
        g_engine.Log("DEBUG: GetQuotesEx: Creating thread for %s", pszTicker);
        CreateThread(NULL, 0, BackfillThreadProc, p, 0, NULL);
      } else {
        g_engine.Log("DEBUG: GetQuotesEx - Backfill already enqueued for %s",
                     pszTicker);
      }
    }
    return (nLastValid < 0) ? 0 : nLastValid + 1;
  }

  if (isAmarstock) {
    g_engine.Log("AMARSTOCK TRACE: GetQuotesEx found %zu bars in cache "
                 "(valid=%d) for %s",
                 bars.size(), validCacheCount, pszTicker);
  } else {
    g_engine.Log("DEBUG: GetQuotesEx - Found %zu bars for %s", bars.size(),
                 pszTicker);
  }

  // ------------------------------------------------------------------------
  // MERGE LOGIC: Combine AmiBroker's existing local data with Plugin's web data
  // ------------------------------------------------------------------------
  std::map<unsigned __int64, Quotation> mergedQuotes;

  // 1. Read existing AmiBroker data into the map (if any)
  if (nLastValid >= 0) {
    for (int i = 0; i <= nLastValid; ++i) {
      // Pack the date components into a single 64-bit int for map sorting
      unsigned __int64 key = pQuotes[i].DateTime.Date;
      mergedQuotes[key] = pQuotes[i];
    }
  }

  // 2. Filter valid bars from plugin cache
  std::vector<DseBar> validBars;
  for (const auto &b : bars) {
    if (b.valid)
      validBars.push_back(b);
  }

  // 3. Merge plugin data into the map
  // Note: DseDataEngine already applies `preferWebData` internally for merging
  // seed+web. Here, we generally let the plugin's cached data overwrite
  // AmiBroker's local data for the overlapping dates because the plugin just
  // refreshed it.
  for (const auto &bar : validBars) {
    AmiDate ad = PackAmiDate(bar.year, bar.month, bar.day, 0, 0, 0);
    unsigned __int64 key = ad.Date;

    Quotation q;
    memset(&q, 0, sizeof(q));
    q.DateTime = ad;
    q.Open = (float)bar.open;
    q.High = (float)bar.high;
    q.Low = (float)bar.low;
    q.Price = (float)bar.close;
    q.Volume = (float)bar.volume;
    q.OpenInterest = 0;
    q.AuxData1 = (float)bar.trade;
    q.AuxData2 = (float)bar.value;

    mergedQuotes[key] = q;
  }

  // 4. Fill the Quotation array with the merged data
  // AmiBroker expects data from oldest (index 0) to newest (index N)
  int count = (int)mergedQuotes.size();
  if (count > nSize)
    count = nSize;

  // If we have more data than array size, skip oldest
  int skipCount = (int)mergedQuotes.size() - count;
  int i = 0;
  int skipped = 0;

  for (const auto &entry : mergedQuotes) {
    if (skipped < skipCount) {
      skipped++;
      continue;
    }
    pQuotes[i] = entry.second;
    i++;
    if (i >= count)
      break;
  }

  // If we have a real-time quote, update the last bar
  DseQuote liveQuote;
  if (g_feed.GetLatestQuote(pszTicker, liveQuote) && count > 0) {
    int lastIdx = count - 1;

    // Only update if it's the same day
    SYSTEMTIME st;
    GetLocalTime(&st);

    int lastYear, lastMonth, lastDay, h, m, s;
    UnpackAmiDate(pQuotes[lastIdx].DateTime, &lastYear, &lastMonth, &lastDay,
                  &h, &m, &s);

    if (lastYear == (int)st.wYear && lastMonth == (int)st.wMonth &&
        lastDay == (int)st.wDay) {
      // Update today's bar with live data
      if (liveQuote.ltp > 0)
        pQuotes[lastIdx].Price = (float)liveQuote.ltp;
      if (liveQuote.high > 0)
        pQuotes[lastIdx].High = (float)liveQuote.high;
      if (liveQuote.low > 0)
        pQuotes[lastIdx].Low = (float)liveQuote.low;
      if (liveQuote.volume > 0)
        pQuotes[lastIdx].Volume = (float)liveQuote.volume;

      // Update time if intraday
      if (g_timeBase < 86400) {
        pQuotes[lastIdx].DateTime = PackAmiDate(
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
      }
    } else if (g_engine.IsMarketOpen()) {
      // It's a new day — add a new bar
      if (count < nSize) {
        int newIdx = count;
        pQuotes[newIdx].DateTime = PackAmiDate(
            st.wYear, st.wMonth, st.wDay, (g_timeBase < 86400) ? st.wHour : 0,
            (g_timeBase < 86400) ? st.wMinute : 0,
            (g_timeBase < 86400) ? st.wSecond : 0);

        pQuotes[newIdx].Open = (float)liveQuote.open;
        pQuotes[newIdx].High = (float)liveQuote.high;
        pQuotes[newIdx].Low = (float)liveQuote.low;
        pQuotes[newIdx].Price = (float)liveQuote.ltp;
        pQuotes[newIdx].Volume = (float)liveQuote.volume;
        pQuotes[newIdx].OpenInterest = 0;
        pQuotes[newIdx].AuxData1 = (float)liveQuote.trade;
        pQuotes[newIdx].AuxData2 = (float)liveQuote.value;
        count++;
      }
    }
  }

  return count; // Return number of quotes (AmiBroker API change: return
                // COUNT, not last index)
}

///////////////////////////////////////////////////////////////////////////
// GetQuotes — Legacy OHLCV data for older AmiBroker
///////////////////////////////////////////////////////////////////////////
struct QuotationFormat4 {
  float Price;
  float Open;
  float High;
  float Low;
  float Volume;
  float OpenInterest;
  union {
    DATE_TIME_INT Date;
    struct {
      unsigned int MicroSec : 10;
      unsigned int MilliSec : 10;
      unsigned int Second : 6;
      unsigned int Minute : 6;
      unsigned int Hour : 5;
      unsigned int Day : 5;
      unsigned int Month : 4;
      unsigned int Year : 12;
    } PackDate;
  } DateTime;
};

PLUGINAPI int GetQuotes(LPCTSTR pszTicker, int nPeriodicity, int nLastValid,
                        int nSize, struct QuotationFormat4 *pQuotes) {
  bool isAmarstock = false;
  if (pszTicker && pszTicker[0]) {
    isAmarstock = (_stricmp(pszTicker, "00DS30") == 0 ||
                   _stricmp(pszTicker, "00DSES") == 0 ||
                   _stricmp(pszTicker, "00DSEX") == 0 ||
                   _stricmp(pszTicker, "00DSMEX") == 0);
  }

  if (isAmarstock) {
    g_engine.Log("\n=======================================================");
    g_engine.Log("AMARSTOCK TRACE: LEGACY GetQuotes called for %s", pszTicker);
  } else {
    g_engine.Log("DEBUG: Legacy GetQuotes called for %s", pszTicker);
  }

  if (!pszTicker || !pszTicker[0] || !pQuotes || nSize <= 0)
    return (nLastValid < 0) ? 0 : nLastValid + 1;

  // Allocate modern Quotation structs
  std::vector<Quotation> modernQuotes(nSize);
  memset(modernQuotes.data(), 0, nSize * sizeof(Quotation));

  // Call the modern function
  int count = GetQuotesEx(pszTicker, nPeriodicity, nLastValid, nSize,
                          modernQuotes.data(), nullptr);

  // Translate back to legacy format (QuotationFormat4)
  // GetQuotesEx returns the *number* of elements in modern AB, but legacy AB
  // might expect last valid index. Actually, both return number of elements
  // in 5.27+ but legacy returned last valid index if < 5.27. We'll translate
  // the data up to the returned count or last valid index.
  int limit = count;
  if (limit > nSize)
    limit = nSize;

  for (int i = 0; i < limit; ++i) {
    pQuotes[i].Price = modernQuotes[i].Price;
    pQuotes[i].Open = modernQuotes[i].Open;
    pQuotes[i].High = modernQuotes[i].High;
    pQuotes[i].Low = modernQuotes[i].Low;
    pQuotes[i].Volume = modernQuotes[i].Volume;
    pQuotes[i].OpenInterest = modernQuotes[i].OpenInterest;

    // Date packing is slightly different in older structs, but AmiBroker
    // usually accepts the 64-bit int even if it's placed weirdly in Format4.
    // Let's manually copy the internal struct fields just in case
    pQuotes[i].DateTime.Date = modernQuotes[i].DateTime.Date;
  }

  return count;
}

///////////////////////////////////////////////////////////////////////////
// SetTimeBase — Handle time interval changes
///////////////////////////////////////////////////////////////////////////

extern "C" __declspec(dllexport) int SetTimeBase(int nTimeBase) {
  // Return 1 if we support this interval, 0 if not
  // We allow all intervals to support real-time intraday charting
  // (even if history is only EOD)
  g_timeBase = nTimeBase;
  g_engine.Log("Plugin::SetTimeBase — %d seconds", nTimeBase);
  return 1;
}

///////////////////////////////////////////////////////////////////////////
// Notify — Handle plugin events from AmiBroker
///////////////////////////////////////////////////////////////////////////

PLUGINAPI int Notify(struct PluginNotification *pNotification) {
  if (!pNotification)
    return 0;

  switch (pNotification->nReason) {
  case REASON_DATABASE_LOADED:
    g_engine.Log("Notify: database loaded - %s",
                 pNotification->pszDatabasePath);
    if (pNotification->pszDatabasePath) {
      strcpy_s(g_dbPath, pNotification->pszDatabasePath);
    }
    // Capture AmiBroker window handle
    if (pNotification->hMainWnd) {
      g_hAmiBrokerWnd = pNotification->hMainWnd;
      g_engine.Log("Notify: Captured AB window handle: %p", g_hAmiBrokerWnd);
    }

    // Find configuration file
    FindConfigPath();
    // Initialize the data engine if not already
    if (!g_initialized) {
      Init();
    }

    // Start real-time feed now that we have the AB window handle.
    // This is the primary start point because hMainWnd is guaranteed here.
    if (g_hAmiBrokerWnd && IsWindow(g_hAmiBrokerWnd)) {
      if (!g_feed.IsRunning()) {
        g_engine.Log("Notify: starting realtime feed (database loaded)");
        g_feed.Start(g_hAmiBrokerWnd, &g_engine);
      }
    }
    break;

  case REASON_DATABASE_UNLOADED:
    g_engine.Log("Notify: database unloaded");
    break;

  case REASON_SETTINGS_CHANGE:
    g_engine.Log("Notify: settings change");
    break;
  }

  return 1;
}

PLUGINAPI struct RecentInfo *GetRecentInfo(const char *pszTicker) {
  bool isAmarstock = false;
  if (pszTicker && pszTicker[0]) {
    isAmarstock = (_stricmp(pszTicker, "00DS30") == 0 ||
                   _stricmp(pszTicker, "00DSES") == 0 ||
                   _stricmp(pszTicker, "00DSEX") == 0 ||
                   _stricmp(pszTicker, "00DSMEX") == 0);
  }

  if (isAmarstock) {
    g_engine.Log("AMARSTOCK TRACE: GetRecentInfo called for '%s'", pszTicker);
  } else {
    g_engine.Log("DEBUG: GetRecentInfo called for %s",
                 pszTicker ? pszTicker : "NULL");
  }

  static RecentInfo ri;
  memset(&ri, 0, sizeof(ri));

  if (!pszTicker || !pszTicker[0])
    return &ri;

  DseQuote quote;
  if (g_feed.GetLatestQuote(pszTicker, quote)) {
    strncpy_s(ri.Name, sizeof(ri.Name), quote.symbol, _TRUNCATE);

    ri.fOpen = (float)quote.open;
    ri.fHigh = (float)quote.high;
    ri.fLow = (float)quote.low;
    ri.fLast = (float)quote.ltp;
    ri.fPrev = (float)quote.ycp;
    ri.iTotalVol = (int)quote.volume;
    ri.fChange = (float)quote.change;
    ri.iTradeVol = (int)quote.trade;
    ri.nBitmap = 0xFFFF;

    ri.nStatus = (g_engine.GetConnectionState() == CONN_CONNECTED) ? 1 : 2;
    if (isAmarstock) {
      g_engine.Log("AMARSTOCK TRACE: GetRecentInfo -> Live quote found for %s "
                   "(LTP: %.2f)",
                   pszTicker, ri.fLast);
    }
  } else {
    strncpy_s(ri.Name, sizeof(ri.Name), pszTicker, _TRUNCATE);
    ri.nStatus = 3; // Wait
    if (isAmarstock) {
      g_engine.Log(
          "AMARSTOCK TRACE: GetRecentInfo -> NO live quote for %s (Status 3)",
          pszTicker);
    }
  }

  return &ri;
}
