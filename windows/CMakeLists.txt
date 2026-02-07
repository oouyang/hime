# HIME Windows IME Build Configuration
# Copyright (C) 2020 The HIME team, Taiwan
# License: GNU LGPL v2.1
#
# Build instructions:
#   mkdir build && cd build
#   cmake .. -G "Visual Studio 17 2022" -A x64
#   cmake --build . --config Release
#
# Or for 32-bit:
#   cmake .. -G "Visual Studio 17 2022" -A Win32
#   cmake --build . --config Release

cmake_minimum_required(VERSION 3.16)
project(hime-ime VERSION 0.10.1 LANGUAGES C CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Windows-specific settings
if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
endif()

# ========== HIME Core Library ==========
add_library(hime-core SHARED
    src/hime-core.c
)

target_include_directories(hime-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(hime-core PRIVATE
    HIME_CORE_EXPORTS
)

# Export symbols for Windows DLL
if(WIN32)
    set_target_properties(hime-core PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS OFF
        OUTPUT_NAME "hime-core"
    )
endif()

# ========== HIME TSF Text Service ==========
add_library(hime-tsf SHARED
    src/hime-tsf.cpp
    hime-tsf.def
)

target_include_directories(hime-tsf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(hime-tsf PRIVATE
    hime-core
    ole32
    oleaut32
    uuid
)

set_target_properties(hime-tsf PROPERTIES
    OUTPUT_NAME "hime-tsf"
)

# Module definition file for TSF exports
if(WIN32)
    set_target_properties(hime-tsf PROPERTIES
        LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/hime-tsf.def"
    )
endif()

# ========== Installation ==========
install(TARGETS hime-core hime-tsf
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES include/hime-core.h
    DESTINATION include
)

# Install data files
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../data/
    DESTINATION data
    FILES_MATCHING
    PATTERN "pho.tab2"
    PATTERN "*.kbm"
)

# ========== Registration Helper ==========
# Create a registration batch file
file(WRITE ${CMAKE_BINARY_DIR}/register.bat
"@echo off
echo Registering HIME Text Service...
regsvr32 /s \"%~dp0bin\\hime-tsf.dll\"
if %errorlevel% equ 0 (
    echo Registration successful!
) else (
    echo Registration failed. Please run as Administrator.
)
pause
")

file(WRITE ${CMAKE_BINARY_DIR}/unregister.bat
"@echo off
echo Unregistering HIME Text Service...
regsvr32 /u /s \"%~dp0bin\\hime-tsf.dll\"
if %errorlevel% equ 0 (
    echo Unregistration successful!
) else (
    echo Unregistration failed. Please run as Administrator.
)
pause
")

# ========== Testing ==========
enable_testing()

# Test program for hime-core
add_executable(test-hime-core
    tests/test-hime-core.c
)

target_include_directories(test-hime-core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(test-hime-core PRIVATE
    hime-core
)

add_test(NAME hime-core-test COMMAND test-hime-core)

# ========== Package ==========
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_NAME "hime-ime")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "HIME Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "HIME Input Method Editor for Windows")
include(CPack)

message(STATUS "HIME Windows IME configuration complete")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
