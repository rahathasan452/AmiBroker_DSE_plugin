# 🇧🇩 AmiBroker DSE Data Plugin (bdshare Edition)

A **native C++ DLL** data plugin for [AmiBroker](https://www.amibroker.com/) that integrates [bdshare](https://github.com/rochi88/bdshare)'s DSE scraping logic directly — no Python, no external scripts, no background servers.

## ⚡ Features

| Feature | Description |
|---------|-------------|
| **Click-to-Load** | Auto-fetches 3 years of history when you click a new symbol |
| **Real-Time Feed** | Live LTP/Volume updates during market hours (10:00–14:30 BST) |
| **Bad Tick Filter** | Ignores zero/erroneous ticks from pre-open/lunch phases |
| **Auto-Reconnect** | Exponential backoff reconnection on DSE timeouts |
| **Zero Dependencies** | Uses WinInet (built into Windows) — no curl, no Python |
| **Connection Status** | Visual indicator: ✅ Connected / 🔴 Disconnected |
| **Intraday Support** | Supports intraday charts (1-min, 5-min) with live timestamps |

## 📦 Build

### Prerequisites
- **Visual Studio 2019+** or MSVC Build Tools
- **CMake 3.15+**

### Steps

Option 1: **Automatic Build Script (Recommended)**
Double-click `build_manual.bat` or run in terminal:
```powershell
.\build_manual.bat
```
This script attempts to find Visual Studio automatically and compile the plugin.

Option 2: **CMake (Manual)**
```powershell
cd d:\software\AmiBroker_DSE_plugin

# Configure (Win32 for 32-bit AmiBroker)
cmake -B build -G "Visual Studio 17 2022" -A Win32

# Build
cmake --build build --config Release
```

> **64-bit AmiBroker?** Change `-A Win32` to `-A x64`

Output: `build\Release\DSE_DataPlugin.dll`

## 🚀 Installation

1. Copy `DSE_DataPlugin.dll` to AmiBroker's `Plugins` folder
2. Copy the `config` folder alongside the DLL
3. Launch AmiBroker → **File → Database Settings**
4. Select **"DSE Data Plugin (bdshare)"** as data source
5. Click any DSE symbol (e.g., `GP`, `BRAC`, `ACI`) — data loads instantly

## ⚙️ Configuration

Edit `config\dse_config.ini`:

| Setting | Default | Description |
|---------|---------|-------------|
| `HistoryYears` | 3 | Years of history on first load |
| `PollIntervalMs` | 5000 | Real-time polling interval (ms) |
| `MarketOpenHour` | 10 | DSE market open hour (BST) |
| `MarketCloseHour` | 14 | DSE market close hour (BST) |
| `HttpTimeoutSec` | 30 | HTTP request timeout |

## 📁 Project Structure

```
├── CMakeLists.txt          # Build configuration
├── Plugin.def              # DLL exports
├── config/
│   └── dse_config.ini      # Default settings
├── include/
│   ├── Plugin.h            # AmiBroker ADK header
│   ├── DseDataEngine.h     # DSE HTTP client & parser
│   └── RealtimeFeed.h      # Background polling thread
└── src/
    ├── Plugin.cpp           # Main entry points
    ├── DseDataEngine.cpp    # DSE scraping logic
    └── RealtimeFeed.cpp     # Real-time feed
```

## 🔌 AmiBroker Integration

The plugin exports these ADK functions:

| Function | Purpose |
|----------|---------|
| `GetPluginInfo()` | Identifies the plugin to AmiBroker |
| `Init()` / `Release()` | Lifecycle management |
| `Configure()` | Settings dialog |
| `GetQuotesEx()` | Delivers OHLCV bars to charts |
| `SetTimeBase()` | Supports daily timeframe |
| `Notify()` | Symbol-change event → lazy backfill |
| `GetRecentInfo()` | Real-time quote for ticker window |

## ⚠️ Limitations

- **Daily data only** — DSE doesn't provide intraday OHLCV
- **Web scraping** — may break if DSE changes their website
- **Not certified** by AmiBroker — use at your own risk

## 📜 License

MIT — Based on [bdshare](https://github.com/rochi88/bdshare) by Raisul Islam.
