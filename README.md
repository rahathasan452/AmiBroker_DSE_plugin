# 🇧🇩 AmiBroker DSE Data Plugin

A **native C++ DLL** data plugin for [AmiBroker](https://www.amibroker.com/) that fetches historical and real-time price data from the **Dhaka Stock Exchange (DSE)** website ([dsebd.org](https://www.dsebd.org)). Zero external dependencies — uses only Windows built-in APIs.

---

## ✨ Features

| Feature | Description |
|---------|-------------|
| **Click-to-Load History** | Auto-fetches OHLCV history when you click any DSE symbol |
| **CSV Seed Support** | Loads bulk historical data from local CSV files for speed |
| **Hybrid Data Merge** | Merges local CSV seed + web data (configurable priority) |
| **Real-Time Feed** | Live LTP/Volume updates during market hours (10:00–14:30 BST) |
| **Bad Tick Filter** | Rejects bars where any OHLC price is zero or < 0.01 |
| **Auto-Reconnect** | Exponential backoff reconnection on network failure |
| **CSV Export** | Export cached symbol data back to CSV (on-demand or scheduled) |
| **Zero Dependencies** | Uses WinInet (built into Windows) — no curl, Python, or 3rd-party libs |
| **Symbol Sync** | One-click sync of all DSE symbols into AmiBroker's database |

---

## 📁 Project Structure

```
AmiBroker_DSE_plugin/
├── CMakeLists.txt           # CMake build configuration
├── Plugin.def               # DLL export definitions
├── build_manual.bat         # Quick-build batch script (no CMake needed)
├── config/
│   └── dse_config.ini       # Plugin configuration (INI format)
├── include/
│   ├── Plugin.h             # AmiBroker ADK v2.1 header (official, unmodified)
│   ├── DseDataEngine.h      # DSE HTTP client & HTML parser class
│   └── RealtimeFeed.h       # Background polling thread class
└── src/
    ├── Plugin.cpp           # AmiBroker plugin entry points & config dialog
    ├── DseDataEngine.cpp    # Core: HTTP fetch, HTML parse, CSV seed, cache
    └── RealtimeFeed.cpp     # Real-time polling thread implementation
```

---

## 🏗️ Architecture

```
AmiBroker
   │
   ├─ GetQuotesEx()  ──────────────────────────────────► DseDataEngine::GetCachedBars()
   │                                                           │
   │                                             ┌────────────┴────────────┐
   │                                       CSV Seed                    Web Fetch
   │                                    (LoadCsvSeed)            (HttpGet + ParseHTML)
   │                                             └────────────┬────────────┘
   │                                                     Merge & Cache
   │
   ├─ GetRecentInfo() ─────────────────────────────────► RealtimeFeed::GetLatestQuote()
   │                                                           │
   │                                                    Background Thread
   │                                                   (polls every 5s during
   │                                                    market hours 10–14:30 BST)
   │
   └─ Notify(REASON_DATABASE_LOADED) ─────────────────► Init() → DseDataEngine::Initialize()
```

**Data flow on first symbol load:**
1. AmiBroker calls `GetQuotesEx("GP", ...)` — no cached data found
2. Plugin starts a background thread (`BackfillThreadProc`)
3. Thread calls `FetchHistoricalData()`:
   - Reads `<CsvSeedPath>/GP.csv` if it exists
   - Fetches from `dsebd.org/day_end_archive.php?inst=GP&...`
   - Merges both sources, deduplicates by date
4. Cache is updated → AmiBroker is notified via `WM_USER_STREAMING_UPDATE`
5. Chart refreshes with data

---

## ⚙️ Configuration (`config/dse_config.ini`)

### `[General]`

| Key | Default | Description |
|-----|---------|-------------|
| `HistoryYears` | `3` | Years of history to fetch on first symbol load |
| `PollIntervalMs` | `5000` | Real-time polling interval in milliseconds |
| `MarketOpenHour` | `10` | DSE session open hour (BST = UTC+6) |
| `MarketOpenMinute` | `0` | DSE session open minute |
| `MarketCloseHour` | `14` | DSE session close hour |
| `MarketCloseMinute` | `30` | DSE session close minute |
| `MaxReconnectAttempts` | `10` | Max retries before pausing for 60 seconds |
| `HttpTimeoutSec` | `30` | Per-request HTTP timeout |
| `UserAgent` | `Mozilla/5.0...` | Browser user-agent for HTTP requests |
| `PreferWebData` | `1` | `1` = web overwrites local CSV; `0` = local CSV wins |

### `[Endpoints]`

| Key | Default |
|-----|---------|
| `LatestPrice` | `https://www.dsebd.org/latest_share_price_scroll_l.php` |
| `DayEndArchive` | `https://www.dsebd.org/day_end_archive.php` |
| `AltLatestPrice` | `https://www.dsebd.org/latest_share_price_all_,ajax.php` |

### `[DataSource]`

| Key | Default | Description |
|-----|---------|-------------|
| `CsvSeedPath` | *(empty)* | Folder with per-symbol CSV files (e.g., `GP.csv`, `BRAC.csv`). Leave empty for web-only mode. |

### `[Export]`

| Key | Default | Description |
|-----|---------|-------------|
| `ExportPath` | *(empty)* | Folder to export cached data as CSV files. Leave empty to disable. |
| `ExportIntervalSec` | `0` | Auto-export every N seconds. `0` = manual only (use "Save CSVs Now" in dialog). |

### `[Debug]`

| Key | Default | Description |
|-----|---------|-------------|
| `EnableLogging` | `0` | Set to `1` to enable debug logging |
| `LogFilePath` | `dse_plugin.log` | Log file path (relative or absolute) |

---

## 📄 CSV Seed File Format

The plugin supports two CSV formats for seed data:

**Format A — with ticker column (standard):**
```csv
Trading_Code,Date,Open,High,Low,Close,Volume
GP,2024-01-02,360.00,365.50,358.00,363.20,1234567
```

**Format B — date-first (simplified):**
```csv
Date,Open,High,Low,Close,Volume
2024-01-02,360.00,365.50,358.00,363.20,1234567
```

- Dates must be in `YYYY-MM-DD` format
- Files must be named `<SYMBOL>.csv` (e.g., `GP.csv`)
- Commas in numbers (e.g., `1,234,567`) are handled automatically
- Rows with zero or invalid OHLC values are silently discarded

---

## 🔨 Building

### Option 1: Quick Batch Build (Recommended)

```cmd
cd d:\software\AmiBroker_DSE_plugin
build_manual.bat
```

Produces:
- `build\Release\x86\DSE_DataPlugin_x86.dll` — for 32-bit AmiBroker
- `build\Release\x64\DSE_DataPlugin_x64.dll` — for 64-bit AmiBroker

> **Note:** Edit `build_manual.bat` line 5 if your Visual Studio is installed in a different path.

### Option 2: CMake

```powershell
cd d:\software\AmiBroker_DSE_plugin

# 32-bit (for most AmiBroker installs)
cmake -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release

# 64-bit
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Output: `build\Release\DSE_DataPlugin.dll`

### Prerequisites

- **Visual Studio 2019** or later (MSVC compiler)
- **CMake 3.15+** (only for Option 2)
- Windows SDK (included with Visual Studio)

---

## 🚀 Installation

1. **Copy the DLL** to AmiBroker's `Plugins` folder:
   ```
   D:\software\AmiBroker\Plugins\DSE_DataPlugin.dll
   ```
   Use the x86 version if your AmiBroker is 32-bit (most installs are).

2. **Copy the `config` folder** alongside the DLL:
   ```
   D:\software\AmiBroker\Plugins\config\dse_config.ini
   ```

3. **Open AmiBroker** → **File → Database Settings**

4. Under **Data Source**, select **"DSE Data Plugin"**

5. Click **OK** — the plugin is now active

6. *(Optional)* Click **Configure** to open the settings dialog:
   - Set history years
   - Set CSV seed path
   - Set export path
   - Click **Sync with Database** to import all DSE symbols

7. Click any DSE symbol (e.g., `GP`, `BRAC`, `ACI`) — history loads automatically

---

## 🔌 Exported API Functions

These functions are called by AmiBroker via the plugin DLL interface:

| Function | When Called | Purpose |
|----------|-------------|---------|
| `GetPluginInfo()` | On DLL load | Returns plugin name, version, type |
| `Init()` | On plugin start | Loads config, initializes WinInet session |
| `Release()` | On plugin stop | Stops feed thread, closes handles |
| `Configure()` | User clicks "Configure" | Shows settings dialog |
| `GetQuotesEx()` | Chart opened/refreshed | Returns OHLCV bars from cache |
| `SetTimeBase()` | User changes interval | Handles daily/intraday mode switch |
| `Notify()` | Database load/unload | Captures AB window handle, calls Init |
| `GetRecentInfo()` | Real-time ticker window | Returns latest LTP/OHLCV from feed |

---

## 🐛 Troubleshooting

### No data appears for a symbol
- Check that the DLL and `config\` folder are in the same `Plugins` directory
- Enable logging: set `EnableLogging=1` in `dse_config.ini` and check `dse_plugin.log`
- Verify internet access to `https://www.dsebd.org`
- Try clicking **Configure → Refresh Symbols** to confirm DSE is reachable

### Chart shows "messy" spikes or zeros
- The bad-tick filter (any OHLC < 0.01) should reject these automatically
- Check if the CSV seed files contain invalid rows (zero prices, missing data)
- Enable logging and look for `"ValidateBar: rejected"` lines

### Plugin not listed in AmiBroker's Data Source
- Ensure you copied the correct bitness DLL (x86 for 32-bit AmiBroker)
- AmiBroker 32-bit requires `DSE_DataPlugin_x86.dll`, renamed to `DSE_DataPlugin.dll`

### "DSE website changed" — data stops loading
- DSE occasionally changes their HTML table structure
- This plugin uses two fallback parsing strategies; enable logging to diagnose
- The `ParseHistoricalHtml` function scans for column names flexibly (DATE, OPEN, HIGH, LOW, CLOSE/LTP, VOLUME)

---

## ⚠️ Limitations

- **Daily data only** — DSE does not publish intraday OHLCV (only tick price via latest_share_price)
- **Web scraping** — may break if DSE changes their HTML layout significantly
- **Bangladesh market only** — hardcoded for `dsebd.org` endpoints
- **Not officially certified** by AmiBroker — use at your own risk

---

## 📜 License

MIT License

Plugin skeleton based on the AmiBroker Plugin API (© 2001–2010 AmiBroker.com, all rights reserved).  
Scraping logic inspired by [bdshare](https://github.com/rochi88/bdshare) by Raisul Islam.
