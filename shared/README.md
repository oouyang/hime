# HIME Shared Core Library

This directory contains the shared, platform-independent core implementation of the HIME input method engine.

## Structure

```
shared/
├── include/
│   └── hime-core.h      # Public API header
├── src/
│   └── hime-core-impl.c # Core implementation
└── README.md
```

## Usage

Each platform includes the shared implementation with a platform-specific wrapper:

### Windows
```c
// windows/src/hime-core.c
#define HIME_VERSION "0.10.1-win"
#include "../../shared/src/hime-core-impl.c"
```

### macOS
```c
// macos/src/hime-core.c
#define HIME_VERSION "0.10.1-macos"
#include "../../shared/src/hime-core-impl.c"
```

### iOS
```c
// ios/Shared/src/hime-core.c
#define HIME_VERSION "0.10.1-ios"
#include "../../../shared/src/hime-core-impl.c"
```

### Android
```c
// android/app/src/main/jni/hime-core.c
#define HIME_VERSION "0.10.1-android"
#include "../../../../../shared/src/hime-core-impl.c"
```

## Benefits

1. **Single Source of Truth**: All platforms use the same implementation
2. **Reduced Code Duplication**: ~6000 lines saved (from 8000 to 2000)
3. **Easier Maintenance**: Bug fixes apply to all platforms automatically
4. **Consistent Behavior**: All platforms have identical functionality

## Adding Platform-Specific Code

If platform-specific behavior is needed:

1. Add preprocessor checks in `hime-core-impl.c`:
   ```c
   #ifdef _WIN32
       // Windows-specific code
   #elif defined(__APPLE__)
       #include <TargetConditionals.h>
       #if TARGET_OS_IOS
           // iOS-specific code
       #else
           // macOS-specific code
       #endif
   #elif defined(__ANDROID__)
       // Android-specific code
   #endif
   ```

2. Or override functions in platform wrappers before including the implementation.

## Version String

Define `HIME_VERSION` before including the implementation to set the version string for that platform. If not defined, defaults to `"0.10.1"`.
