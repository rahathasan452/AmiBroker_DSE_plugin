###########################################################################
# CMakeLists.txt — AmiBroker DSE Data Plugin
#
# Build:
#   cmake -B build -G "Visual Studio 17 2022" -A Win32
#   cmake --build build --config Release
#
# Output: build/Release/DSE_DataPlugin.dll
###########################################################################

cmake_minimum_required(VERSION 3.15)
project(DSE_DataPlugin VERSION 1.0.0 LANGUAGES CXX)

# ───────────────────────────────────────────────────────
# Configuration
# ───────────────────────────────────────────────────────

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC-specific flags
if(MSVC)
    # /MT  = Static CRT (no vcruntime dependency)
    # /O2  = Optimize for speed
    # /W3  = Warning level 3
    set(CMAKE_CXX_FLAGS_RELEASE "/MT /O2 /W3 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG   "/MTd /Od /W4 /Zi /D_DEBUG")

    # Disable specific warnings
    add_compile_options(/wd4996)  # Deprecated CRT functions
endif()

# ───────────────────────────────────────────────────────
# Source Files
# ───────────────────────────────────────────────────────

set(PLUGIN_SOURCES
    src/Plugin.cpp
    src/DseDataEngine.cpp
    src/RealtimeFeed.cpp
)

set(PLUGIN_HEADERS
    include/Plugin.h
    include/DseDataEngine.h
    include/RealtimeFeed.h
)

# ───────────────────────────────────────────────────────
# DLL Target
# ───────────────────────────────────────────────────────

add_library(DSE_DataPlugin SHARED
    ${PLUGIN_SOURCES}
    ${PLUGIN_HEADERS}
    Plugin.def
)

# Include directories
target_include_directories(DSE_DataPlugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ───────────────────────────────────────────────────────
# Link Libraries (Windows system libs only — zero deps)
# ───────────────────────────────────────────────────────

target_link_libraries(DSE_DataPlugin PRIVATE
    wininet     # HTTP requests
    ws2_32      # Winsock
    comctl32    # Common controls for dialog
)

# ───────────────────────────────────────────────────────
# Module Definition File (exports)
# ───────────────────────────────────────────────────────

set_target_properties(DSE_DataPlugin PROPERTIES
    LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_SOURCE_DIR}/Plugin.def\""
)

# ───────────────────────────────────────────────────────
# Output
# ───────────────────────────────────────────────────────

set_target_properties(DSE_DataPlugin PROPERTIES
    OUTPUT_NAME "DSE_DataPlugin"
    PREFIX ""
    SUFFIX ".dll"
)

# ───────────────────────────────────────────────────────
# Post-build: Copy config to output directory
# ───────────────────────────────────────────────────────

add_custom_command(TARGET DSE_DataPlugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/config"
        "$<TARGET_FILE_DIR:DSE_DataPlugin>/config"
    COMMENT "Copying config files..."
)

# ───────────────────────────────────────────────────────
# Install (optional)
# ───────────────────────────────────────────────────────

install(TARGETS DSE_DataPlugin
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)

install(DIRECTORY config/
    DESTINATION bin/config
)

message(STATUS "")
message(STATUS "═══════════════════════════════════════════")
message(STATUS "  DSE Data Plugin for AmiBroker")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "═══════════════════════════════════════════")
message(STATUS "")
