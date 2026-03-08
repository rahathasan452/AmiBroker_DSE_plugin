# 🇧🇩 AmiBroker DSE Data Plugin

A native C++ data plugin for [AmiBroker](https://www.amibroker.com/) that fetches historical OHLCV bars and real-time prices from the **Dhaka Stock Exchange (DSE)** website.

**Zero external dependencies** — uses only Windows built-in WinInet APIs. No Python, no curl, no `requests`.

---

## ✨ Features

- **Click-to-Load History** — Auto-fetches up to 3 years of OHLCV history when you click any DSE symbol.
- **Real-Time Feed** — Live background polling for LTP/Volume during market hours (Sun–Thu, 10:00–14:30 BST).
- **Hybrid Data Merge** — Merges blazing-fast local CSV seeds with live web data.
- **Auto-Reconnect** — Exponential backoff reconnection if the internet drops.
- **Amarstock Support** — Fallback scraping for broad market indices (00DS30, 00DSES, 00DSEX).

---

## 🚀 Installation & Usage

1. **Copy the DLL** to AmiBroker's `Plugins` folder:
   - 32-bit AmiBroker: `build\Release\x86\DSE_DataPlugin_x86.dll` *(most common)*
   - 64-bit AmiBroker: `build\Release\x64\DSE_DataPlugin_x64.dll`
2. **Copy the config file** alongside the DLL:
   - `Plugins\config\dse_config.ini`
3. **Open AmiBroker** → `File` → `Database Settings`
4. Under **Data Source**, select `DSE Data Plugin`
5. Click **Configure** to open the settings dialog and customize your preferences.
6. Click any DSE symbol to trigger an automatic data fetch.

---

## ⚙️ Configuration (`config/dse_config.ini`)

| Section | Key | Default | Description |
|---|---|---|---|
| `[Settings]` | `HistoryDays` | `365` | Days of history to fetch on first symbol load |
| `[General]` | `PollIntervalMs` | `5000` | Real-time polling interval in milliseconds |
| `[General]` | `MarketOpenHour` | `10` | DSE session open hour (BST = UTC+6) |
| `[General]` | `MarketCloseHour` | `14` | DSE session close hour |
| `[General]` | `MaxReconnectAttempts` | `10` | Max retries before pausing for 60 seconds |
| `[General]` | `PreferWebData` | `1` | `1` = web overwrites local CSV |
| `[DataSource]` | `CsvSeedPath` | | Folder with per-symbol CSV seed files (e.g. `GP.csv`). Leave empty for web-only. |
| `[Export]` | `ExportPath` | | Folder to auto-export cached CSV data. |
| `[Debug]` | `EnableLogging` | `0` | Set to `1` to write debug logs to `LogFilePath`. |

---

## 🔨 Building from Source

### Option 1: Quick Batch Build (Recommended)
Open a standard Command Prompt (cmd.exe):
```cmd
cd d:\software\AmiBroker_DSE_plugin
build_manual.bat
```
*(Produces both x86 and x64 DLLs in `build\Release\`)*

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

---

## 🗃️ CSV Seed Format

If using `CsvSeedPath`, place `<SYMBOL>.csv` files in that folder. Supported formats:

**Format A (Ticker First):**
```csv
Trading_Code,Date,Open,High,Low,Close,Volume
GP,2024-01-02,360.00,365.50,358.00,363.20,1234567
```

**Format B (Date First):**
```csv
Date,Open,High,Low,Close,Volume
2024-01-02,360.00,365.50,358.00,363.20,1234567
```
