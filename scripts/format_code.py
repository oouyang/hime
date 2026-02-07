#!/usr/bin/env python3
"""The python script calling clang-format to unify the source code format"""

import os
import re
import shutil
import sys


def get_c_files_under_dir(directory_list: list[str]) -> str:
    """Get all the C/C++ files under the directory list"""
    target_files = ""
    for directory in directory_list:
        if not os.path.exists(directory):
            continue
        for root, _, files in os.walk(directory):
            for file in files:
                filepath = root + "/" + file
                if is_c_file(filepath):
                    target_files = target_files + filepath + " "
    return target_files


# Directories containing Objective-C code (headers use ObjC syntax)
OBJC_DIRECTORIES = [
    "ios/HIMEKeyboard",
    "ios/Shared/include/HimeEngine.h",
    "macos/HIME",
]


def is_objc_path(filepath: str) -> bool:
    """Check if file is in an Objective-C directory or is an ObjC file"""
    for objc_dir in OBJC_DIRECTORIES:
        if objc_dir in filepath:
            return True
    return False


def is_c_file(filepath: str) -> bool:
    """Check whether the file is C/C++ code (excludes Objective-C)"""
    file = os.path.basename(filepath)
    # Exclude Objective-C files (.m, .mm)
    if re.search(r"\.(m|mm)$", file):
        return False
    # Exclude files in Objective-C directories
    if is_objc_path(filepath):
        return False
    return re.search(r"\.(c|cpp|h)$", file) is not None


def find_clang_format() -> str:
    """Find available clang-format executable"""
    # Try specific versions first (prefer newer versions)
    versions = ["clang-format-14", "clang-format-15", "clang-format-13",
                "clang-format-12", "clang-format-11", "clang-format"]
    for version in versions:
        if shutil.which(version):
            return version
    return None


if __name__ == "__main__":
    # All platform directories
    directories = [
        # Core source
        "data",
        "src",
        # Shared cross-platform implementation
        "shared/include",
        "shared/src",
        "shared/tests",
        # Windows platform
        "windows/include",
        "windows/src",
        # macOS platform
        "macos/HIME/src",
        # iOS platform
        "ios/Shared/include",
        "ios/Shared/src",
        "ios/HIMEKeyboard",
        # Android platform (JNI)
        "android/app/src/main/jni",
        # Tests
        "tests",
    ]

    clang_format = find_clang_format()
    if not clang_format:
        print("Error: clang-format not found. Please install clang-format-14 or newer.", file=sys.stderr)
        sys.exit(1)

    files = get_c_files_under_dir(directories)
    if not files.strip():
        print("No C/C++ files found to format.")
        sys.exit(0)

    print(f"Using {clang_format}")
    ret = os.system(f"{clang_format} -i {files} --verbose")
    sys.exit(ret >> 8)
