# HIME Android Native Library
# CMake configuration for JNI bridge and hime-core

cmake_minimum_required(VERSION 3.18)
project(hime-android C)

# HIME_ROOT is passed from Gradle via cmake { arguments }
if(NOT DEFINED HIME_ROOT)
    # Fallback: derive from source tree (platform/android/app/src/main/jni/)
    get_filename_component(HIME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../" ABSOLUTE)
endif()

# C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -DNDEBUG")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")

# Build the shared library
add_library(hime-jni SHARED
    hime-core.c
    hime_jni.c
)

# Include directories
target_include_directories(hime-jni PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${HIME_ROOT}/shared/include
)

# Link Android log library
find_library(log-lib log)
target_link_libraries(hime-jni ${log-lib})

# Strip symbols in release build
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(hime-jni PROPERTIES
        LINK_FLAGS "-s"
    )
endif()
