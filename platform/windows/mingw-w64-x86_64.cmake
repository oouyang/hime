# CMake Toolchain File for MinGW-w64 Cross-Compilation (64-bit)
#
# Usage:
#   cmake .. -DCMAKE_TOOLCHAIN_FILE=../mingw-w64-x86_64.cmake
#
# Prerequisites (Debian/Ubuntu):
#   sudo apt-get install mingw-w64
#
# Prerequisites (Fedora):
#   sudo dnf install mingw64-gcc mingw64-gcc-c++
#
# Prerequisites (Arch Linux):
#   sudo pacman -S mingw-w64-gcc

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Cross-compiler prefix
set(TOOLCHAIN_PREFIX x86_64-w64-mingw32)

# Compilers
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_RC_COMPILER ${TOOLCHAIN_PREFIX}-windres)

# Target environment
set(CMAKE_FIND_ROOT_PATH /usr/${TOOLCHAIN_PREFIX})

# Search behavior
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Static linking for standalone executables
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static-libgcc -static-libstdc++")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-static-libgcc -static-libstdc++")
