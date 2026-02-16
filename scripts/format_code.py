#!/usr/bin/env python3
"""The python script calling clang-format to unify the source code format"""

import os
import re
import shutil
import sys


def get_formattable_files(directory_list: list[str]) -> str:
    """Get all formattable files (C, C++, ObjC, Java) under the directory list"""
    target_files = ""
    for directory in directory_list:
        if not os.path.exists(directory):
            continue
        for root, _, files in os.walk(directory):
            for file in files:
                filepath = root + "/" + file
                if is_formattable_file(filepath):
                    target_files = target_files + filepath + " "
    return target_files


def is_formattable_file(filepath: str) -> bool:
    """Check whether the file can be formatted by clang-format"""
    file = os.path.basename(filepath)
    # C, C++, Objective-C, Objective-C++, Java
    return re.search(r"\.(c|cpp|h|m|mm|java)$", file) is not None


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
        # Core source (C)
        "data",
        "src",
        # Shared cross-platform implementation (C)
        "shared/include",
        "shared/src",
        "shared/tests",
        # Windows platform (C/C++)
        "platform/windows/include",
        "platform/windows/src",
        # macOS platform (Objective-C)
        "platform/macos/src",
        "platform/macos/tests",
        # iOS platform (C + Objective-C)
        "platform/ios/Shared/include",
        "platform/ios/Shared/src",
        "platform/ios/HIMEKeyboard",
        "platform/ios/HIMEApp",
        "platform/ios/HIMEKeyboardTests",
        # Android platform (C + Java)
        "platform/android/app/src/main/jni",
        "platform/android/app/src/main/java",
        "platform/android/app/src/test/java",
        "platform/android/app/src/androidTest/java",
        # Tests (C)
        "tests",
    ]

    clang_format = find_clang_format()
    if not clang_format:
        print("Error: clang-format not found. Please install clang-format-14 or newer.", file=sys.stderr)
        sys.exit(1)

    files = get_formattable_files(directories)
    if not files.strip():
        print("No formattable files found.")
        sys.exit(0)

    print(f"Using {clang_format}")
    ret = os.system(f"{clang_format} -i {files} --verbose")
    sys.exit(ret >> 8)
