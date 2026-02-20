@echo off
setlocal enabledelayedexpansion

REM --- 1. Find Visual Studio (Hardcoded based on successful detection) ---
set "VC_VARS=D:\software\Visual studio\Insiders\VC\Auxiliary\Build\vcvarsall.bat"

if not exist "%VC_VARS%" (
    echo [ERROR] vcvarsall.bat not found at: %VC_VARS%
    pause
    exit /b 1
)

echo [INFO] Found Visual Studio environment at: %VC_VARS%

REM =========================================================================
REM Build 32-bit (x86)
REM =========================================================================
echo.
echo [INFO] -------------------------------------------------------------
echo [INFO] Building DSE Data Plugin (x86 / 32-bit)...
echo [INFO] -------------------------------------------------------------

if not exist "build\Release\x86" mkdir "build\Release\x86"

REM Use cmd /c to run in isolated environment so variables don't persist
cmd /c "call "%VC_VARS%" x86 >nul && cl /nologo /LD /MT /O2 /W3 /EHsc /DNDEBUG /D_USRDLL /I.\include src\Plugin.cpp src\DseDataEngine.cpp src\RealtimeFeed.cpp /Fe:build\Release\x86\DSE_DataPlugin_x86.dll /link /DEF:Plugin.def user32.lib kernel32.lib wininet.lib ws2_32.lib comctl32.lib"

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] x86 Build FAILED!
) else (
    echo [SUCCESS] x86 Build Complete.
    echo File: %CD%\build\Release\x86\DSE_DataPlugin_x86.dll
)

REM =========================================================================
REM Build 64-bit (x64)
REM =========================================================================
echo.
echo [INFO] -------------------------------------------------------------
echo [INFO] Building DSE Data Plugin (x64 / 64-bit)...
echo [INFO] -------------------------------------------------------------

if not exist "build\Release\x64" mkdir "build\Release\x64"

REM Use cmd /c to run in isolated environment
cmd /c "call "%VC_VARS%" x64 >nul && cl /nologo /LD /MT /O2 /W3 /EHsc /DNDEBUG /D_USRDLL /I.\include src\Plugin.cpp src\DseDataEngine.cpp src\RealtimeFeed.cpp /Fe:build\Release\x64\DSE_DataPlugin_x64.dll /link /DEF:Plugin.def user32.lib kernel32.lib wininet.lib ws2_32.lib comctl32.lib"

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] x64 Build FAILED!
) else (
    echo [SUCCESS] x64 Build Complete.
    echo File: %CD%\build\Release\x64\DSE_DataPlugin_x64.dll
)

echo.
echo [INFO] Build process finished.
echo If you have 32-bit AmiBroker, use: build\Release\x86\DSE_DataPlugin_x86.dll
echo If you have 64-bit AmiBroker, use: build\Release\x64\DSE_DataPlugin_x64.dll
pause
