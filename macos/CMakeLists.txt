# HIME macOS Input Method Build Configuration
# Copyright (C) 2020 The HIME team, Taiwan
# License: GNU LGPL v2.1
#
# Build instructions (on macOS only):
#   mkdir build && cd build
#   cmake ..
#   make
#
# Install:
#   sudo cp -r HIME.app /Library/Input\ Methods/
#   # Then logout/login or restart

cmake_minimum_required(VERSION 3.16)
project(hime-macos VERSION 0.10.1 LANGUAGES C OBJC)

# Require macOS
if(NOT APPLE)
    message(FATAL_ERROR "This project requires macOS to build (Input Method Kit)")
endif()

# macOS deployment target
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")

# C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find required frameworks
find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
find_library(INPUTMETHODKIT_FRAMEWORK InputMethodKit REQUIRED)
find_library(CARBON_FRAMEWORK Carbon REQUIRED)

# ========== HIME Core Library (Static) ==========
add_library(hime-core-static STATIC
    src/hime-core.c
)

target_include_directories(hime-core-static PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(hime-core-static PRIVATE
    HIME_CORE_EXPORTS
)

# ========== HIME Input Method Application ==========
set(HIME_BUNDLE_NAME "HIME")
set(HIME_BUNDLE_IDENTIFIER "org.hime-ime.inputmethod.HIME")
set(HIME_BUNDLE_VERSION "${PROJECT_VERSION}")

# Source files
set(HIME_SOURCES
    src/main.m
    src/HimeEngine.m
    src/HimeInputController.m
)

# Create application bundle
add_executable(${HIME_BUNDLE_NAME} MACOSX_BUNDLE
    ${HIME_SOURCES}
)

# Link frameworks and libraries
target_link_libraries(${HIME_BUNDLE_NAME} PRIVATE
    hime-core-static
    ${COCOA_FRAMEWORK}
    ${INPUTMETHODKIT_FRAMEWORK}
    ${CARBON_FRAMEWORK}
)

target_include_directories(${HIME_BUNDLE_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Enable ARC (Automatic Reference Counting)
set_target_properties(${HIME_BUNDLE_NAME} PROPERTIES
    XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
)

# Bundle properties
set_target_properties(${HIME_BUNDLE_NAME} PROPERTIES
    BUNDLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Info.plist.in
    MACOSX_BUNDLE_BUNDLE_NAME "${HIME_BUNDLE_NAME}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${HIME_BUNDLE_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${HIME_BUNDLE_VERSION}"
    MACOSX_BUNDLE_GUI_IDENTIFIER "${HIME_BUNDLE_IDENTIFIER}"
    MACOSX_BUNDLE_COPYRIGHT "Copyright (C) 2020 The HIME team, Taiwan"
)

# Copy resources to bundle
set(HIME_RESOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/pho.tab2
)

# Add resources to bundle
set_source_files_properties(${HIME_RESOURCES} PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources
)
target_sources(${HIME_BUNDLE_NAME} PRIVATE ${HIME_RESOURCES})

# Copy icon files if they exist
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Resources/HIME.icns)
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/Resources/HIME.icns
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(${HIME_BUNDLE_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Resources/HIME.icns)
    set_target_properties(${HIME_BUNDLE_NAME} PROPERTIES
        MACOSX_BUNDLE_ICON_FILE HIME.icns)
endif()

# ========== Unit Tests ==========
option(BUILD_TESTS "Build unit tests" OFF)

if(BUILD_TESTS)
    enable_testing()
    find_library(XCTEST_FRAMEWORK XCTest REQUIRED)

    add_executable(hime-tests
        tests/HimeEngineTests.m
        src/HimeEngine.m
    )

    target_link_libraries(hime-tests PRIVATE
        hime-core-static
        ${COCOA_FRAMEWORK}
        ${XCTEST_FRAMEWORK}
    )

    target_include_directories(hime-tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    set_target_properties(hime-tests PROPERTIES
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
    )

    add_test(NAME HimeEngineTests COMMAND hime-tests)
endif()

# ========== Installation ==========
install(TARGETS ${HIME_BUNDLE_NAME}
    BUNDLE DESTINATION "/Library/Input Methods"
)

# ========== Package ==========
set(CPACK_GENERATOR "DragNDrop")
set(CPACK_PACKAGE_NAME "HIME")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "HIME Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "HIME Input Method Editor for macOS")
set(CPACK_DMG_VOLUME_NAME "HIME ${PROJECT_VERSION}")
include(CPack)

message(STATUS "")
message(STATUS "HIME macOS Input Method configuration complete")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Bundle ID: ${HIME_BUNDLE_IDENTIFIER}")
message(STATUS "  Deployment target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
message(STATUS "")
message(STATUS "After building, install with:")
message(STATUS "  sudo cp -r HIME.app /Library/Input\\ Methods/")
message(STATUS "")
