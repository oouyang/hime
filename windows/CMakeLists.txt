# HIME Windows IME Build Configuration (MinGW-w64 Cross-Compiler)
# Copyright (C) 2020 The HIME team, Taiwan
# License: GNU LGPL v2.1
#
# Cross-compilation from Linux:
#   mkdir build && cd build
#   cmake .. -DCMAKE_TOOLCHAIN_FILE=../mingw-w64-x86_64.cmake
#   make -j$(nproc)
#
# Native MinGW build on Windows:
#   mkdir build && cd build
#   cmake .. -G "MinGW Makefiles"
#   mingw32-make

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
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Windows-specific definitions
add_definitions(-DUNICODE -D_UNICODE)
add_definitions(-DWIN32_LEAN_AND_MEAN)
add_definitions(-DNOMINMAX)

# MinGW-specific settings
if(MINGW)
    # Enable static linking of GCC runtime
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++")

    # Disable console window for DLLs
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--enable-stdcall-fixup")
endif()

# ========== HIME Core Library ==========
add_library(hime-core SHARED
    src/hime-core.c
)

target_include_directories(hime-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared/include
)

target_compile_definitions(hime-core PRIVATE
    HIME_CORE_EXPORTS
)

set_target_properties(hime-core PROPERTIES
    PREFIX ""
    OUTPUT_NAME "hime-core"
)

# ========== HIME TSF Text Service ==========
add_library(hime-tsf SHARED
    src/hime-tsf.cpp
    src/hime-tsf.rc
)

target_include_directories(hime-tsf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(hime-tsf PRIVATE
    hime-core
    ole32
    oleaut32
    uuid
    advapi32
    gdi32
    gdiplus
)

set_target_properties(hime-tsf PROPERTIES
    PREFIX ""
    OUTPUT_NAME "hime-tsf"
)

# Export DLL functions using .def file for MinGW
if(MINGW)
    set_target_properties(hime-tsf PROPERTIES
        LINK_FLAGS "-Wl,--kill-at"
    )
    target_sources(hime-tsf PRIVATE hime-tsf.def)
endif()

# ========== Copy data files to build output ==========
# Copy pho.tab2, .gtab tables, and .kbm files next to the DLL
file(GLOB DATA_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/pho.tab2
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/*.gtab
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/*.kbm
)
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data)
foreach(DATA_FILE ${DATA_FILES})
    get_filename_component(DATA_FILENAME ${DATA_FILE} NAME)
    configure_file(${DATA_FILE} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data/${DATA_FILENAME} COPYONLY)
endforeach()

# ========== Copy icon files to build output ==========
file(GLOB ICON_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/*.png
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/*.ico
)
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/icons)
foreach(ICON_FILE ${ICON_FILES})
    get_filename_component(ICON_FILENAME ${ICON_FILE} NAME)
    configure_file(${ICON_FILE} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/icons/${ICON_FILENAME} COPYONLY)
endforeach()

# ========== Installation ==========
install(TARGETS hime-core hime-tsf
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib
)

install(FILES include/hime-core.h
    DESTINATION include
)

# Install data files alongside DLLs for easy deployment
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../data/
    DESTINATION bin/data
    FILES_MATCHING
    PATTERN "pho.tab2"
    PATTERN "*.gtab"
    PATTERN "*.kbm"
)

# Install icon files alongside DLLs
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/icons/
    DESTINATION bin/icons
    FILES_MATCHING
    PATTERN "*.png"
    PATTERN "*.ico"
)

# Also install to separate data directory for reference
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../data/
    DESTINATION data
    FILES_MATCHING
    PATTERN "pho.tab2"
    PATTERN "*.gtab"
    PATTERN "*.kbm"
)

# ========== HIME Installer ==========
add_executable(hime-install
    src/hime-install.c
    src/hime-install.rc
)

target_link_libraries(hime-install PRIVATE
    advapi32
    shell32
)

target_link_options(hime-install PRIVATE -mconsole -municode)

set_target_properties(hime-install PROPERTIES
    OUTPUT_NAME "hime-install"
)

# ========== HIME Uninstaller ==========
add_executable(hime-uninstall
    src/hime-uninstall.c
    src/hime-uninstall.rc
)

target_link_libraries(hime-uninstall PRIVATE
    advapi32
    shell32
)

target_link_options(hime-uninstall PRIVATE -mconsole -municode)

set_target_properties(hime-uninstall PROPERTIES
    OUTPUT_NAME "hime-uninstall"
)

# ========== Registration Scripts ==========
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

# Test program for installer/uninstaller
add_executable(test-hime-installer
    tests/test-hime-installer.c
)

target_link_options(test-hime-installer PRIVATE -mconsole -municode)

set_target_properties(test-hime-installer PROPERTIES
    OUTPUT_NAME "test-hime-installer"
)

add_test(NAME hime-installer-test COMMAND test-hime-installer)

# ========== Package ==========
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_NAME "hime-ime")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "HIME Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "HIME Input Method Editor for Windows")
include(CPack)

message(STATUS "")
message(STATUS "HIME Windows IME configuration complete")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
