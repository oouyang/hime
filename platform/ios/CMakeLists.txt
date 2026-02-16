# HIME iOS Keyboard Extension Build Configuration
# Copyright (C) 2020 The HIME team, Taiwan
# License: GNU LGPL v2.1
#
# Build instructions (on macOS with Xcode):
#   mkdir build && cd build
#   cmake .. -G Xcode -DCMAKE_SYSTEM_NAME=iOS
#   open HIME-iOS.xcodeproj
#
# Or build from command line:
#   cmake --build . --config Release -- -sdk iphoneos

cmake_minimum_required(VERSION 3.16)
project(HIME-iOS VERSION 0.10.1 LANGUAGES C OBJC)

# Project root (two levels up from platform/ios/)
get_filename_component(HIME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

# Require iOS or macOS (for simulator)
if(NOT APPLE)
    message(FATAL_ERROR "This project requires macOS with Xcode to build iOS apps")
endif()

# iOS deployment target
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum iOS version")
set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "12.0")

# C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Bundle identifiers
set(HIME_APP_BUNDLE_ID "org.hime-ime.ios.HIME")
set(HIME_KEYBOARD_BUNDLE_ID "org.hime-ime.ios.HIME.Keyboard")
set(HIME_APP_GROUP "group.org.hime-ime.ios.HIME")

# ========== Shared Library (Static) ==========
add_library(hime-shared STATIC
    Shared/src/hime-core.c
    Shared/src/HimeEngine.m
)

target_include_directories(hime-shared PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Shared/include
    ${HIME_ROOT}/shared/include
)

target_compile_definitions(hime-shared PRIVATE
    HIME_CORE_EXPORTS
)

set_target_properties(hime-shared PROPERTIES
    XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
)

# ========== Main App ==========
set(HIME_APP_SOURCES
    HIMEApp/main.m
    HIMEApp/AppDelegate.h
    HIMEApp/AppDelegate.m
    HIMEApp/ViewController.h
    HIMEApp/ViewController.m
)

add_executable(HIMEApp MACOSX_BUNDLE
    ${HIME_APP_SOURCES}
)

target_link_libraries(HIMEApp PRIVATE
    hime-shared
    "-framework UIKit"
    "-framework Foundation"
)

target_include_directories(HIMEApp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Shared/include
)

set_target_properties(HIMEApp PROPERTIES
    BUNDLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/HIMEApp/Info.plist.in
    MACOSX_BUNDLE_BUNDLE_NAME "HIME"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_GUI_IDENTIFIER "${HIME_APP_BUNDLE_ID}"
    XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${HIME_APP_BUNDLE_ID}"
)

# App icon asset catalog
set(HIME_ASSET_CATALOG ${CMAKE_CURRENT_SOURCE_DIR}/HIMEApp/Assets.xcassets)
if(EXISTS ${HIME_ASSET_CATALOG})
    file(GLOB_RECURSE HIME_ICON_ASSETS ${HIME_ASSET_CATALOG}/*)
    set_source_files_properties(${HIME_ICON_ASSETS} PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)
    target_sources(HIMEApp PRIVATE ${HIME_ICON_ASSETS})
    set_target_properties(HIMEApp PROPERTIES
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
    )
endif()

# ========== Keyboard Extension ==========
set(HIME_KEYBOARD_SOURCES
    HIMEKeyboard/HIMEKeyboardViewController.h
    HIMEKeyboard/HIMEKeyboardViewController.m
    HIMEKeyboard/Views/HIMEKeyboardView.h
    HIMEKeyboard/Views/HIMEKeyboardView.m
)

add_library(HIMEKeyboard MODULE
    ${HIME_KEYBOARD_SOURCES}
)

target_link_libraries(HIMEKeyboard PRIVATE
    hime-shared
    "-framework UIKit"
    "-framework Foundation"
)

target_include_directories(HIMEKeyboard PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Shared/include
    ${CMAKE_CURRENT_SOURCE_DIR}/HIMEKeyboard
    ${CMAKE_CURRENT_SOURCE_DIR}/HIMEKeyboard/Views
)

set_target_properties(HIMEKeyboard PROPERTIES
    BUNDLE TRUE
    BUNDLE_EXTENSION "appex"
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/HIMEKeyboard/Info.plist.in
    XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${HIME_KEYBOARD_BUNDLE_ID}"
    XCODE_PRODUCT_TYPE "com.apple.product-type.app-extension"
)

# Copy data files to keyboard extension
set(HIME_DATA_FILES
    ${HIME_ROOT}/data/pho.tab2
)

# Note: In Xcode, add these files to the keyboard extension target manually
# CMake's iOS support for app extensions is limited

# ========== Unit Tests ==========
# Note: iOS tests typically run via Xcode. This target is for reference.
option(BUILD_TESTS "Build unit tests" OFF)

if(BUILD_TESTS)
    enable_testing()

    add_library(HIMEKeyboardTests MODULE
        HIMEKeyboardTests/HimeEngineTests.m
        HIMEKeyboardTests/HimeCryptTests.m
        HIMEKeyboardTests/HimeConfTests.m
    )

    target_link_libraries(HIMEKeyboardTests PRIVATE
        hime-shared
        "-framework UIKit"
        "-framework Foundation"
        "-framework XCTest"
    )

    target_include_directories(HIMEKeyboardTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Shared/include
    )

    set_target_properties(HIMEKeyboardTests PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION "xctest"
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
        XCODE_PRODUCT_TYPE "com.apple.product-type.bundle.unit-test"
    )
endif()

message(STATUS "")
message(STATUS "HIME iOS configuration complete")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  App Bundle ID: ${HIME_APP_BUNDLE_ID}")
message(STATUS "  Keyboard Bundle ID: ${HIME_KEYBOARD_BUNDLE_ID}")
message(STATUS "  Deployment target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
message(STATUS "")
message(STATUS "Note: For full iOS build support, use Xcode directly.")
message(STATUS "The generated project may need manual configuration for:")
message(STATUS "  - App Extension embedding")
message(STATUS "  - Code signing")
message(STATUS "  - Data file bundling")
message(STATUS "")
