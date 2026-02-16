# Boshiamy (嘸蝦米) Input Method Setup Guide

This guide explains how to set up the Boshiamy (嘸蝦米) input method in HIME across all supported platforms.

## Overview

Boshiamy (嘸蝦米輸入法) is a shape-based Chinese input method that uses the structure of Chinese characters for input. HIME supports Boshiamy through the GTAB (Generic Table) system.

### Requirements

- Boshiamy table file (`boshiamy.gtab` or `liu.cin`)
- The table file must be obtained separately (licensed by Liu's Studio)

---

## Linux Setup

### Method 1: Using HIME with fcitx5

1. **Install HIME**

   ```bash
   # Ubuntu/Debian
   sudo apt update && sudo apt install hime

   # Arch Linux
   sudo pacman -S hime
   # or from AUR
   yay -S hime

   # Fedora
   sudo dnf install hime
   ```

2. **Install fcitx5 with table support**

   ```bash
   # Ubuntu/Debian
   sudo apt install fcitx5 fcitx5-table-extra

   # Arch Linux
   sudo pacman -S fcitx5 fcitx5-table-extra
   ```

3. **Configure Input Method Framework**
   - Open System Settings → Region & Language
   - Set input method framework to fcitx5
   - Log out and log back in

4. **Add Boshiamy Input Method**
   - Open fcitx5 Configuration
   - Click "Add Input Method"
   - Search for "boshiamy" or "嘸蝦米"
   - Add to your input method list

5. **Import Boshiamy Table (if needed)**
   - Obtain the Boshiamy `.cin` file
   - Convert to `.gtab` format:
     ```bash
     hime-cin2gtab liu.cin liu.gtab
     ```
   - Copy to HIME data directory:
     ```bash
     cp liu.gtab ~/.config/hime/
     ```

6. **Restart and Use**
   - Log out and log back in
   - Use `Ctrl+Space` to toggle input method
   - Use `Ctrl+Shift` to switch between methods

### Method 2: Direct HIME Configuration

1. **Edit gtab.list**

   ```bash
   nano ~/.config/hime/gtab.list
   ```

2. **Add Boshiamy entry**

   ```
   嘸蝦米,b,liu.gtab,liu.png,0
   ```

3. **Restart HIME**
   ```bash
   hime --daemon &
   ```

---

## Windows Setup

### Using HIME for Windows

1. **Install HIME**
   - Download HIME installer from releases
   - Run installer and follow prompts

2. **Configure Boshiamy via API**

   ```c
   #include "hime-core.h"

   // Initialize HIME
   hime_init("C:\\Program Files\\HIME\\data");

   // Create context
   HimeContext *ctx = hime_context_new();

   // Set to GTAB mode
   hime_set_input_method(ctx, HIME_IM_GTAB);

   // Load Boshiamy table
   hime_gtab_load_table_by_id(ctx, HIME_GTAB_BOSHIAMY);
   ```

3. **Place Table File**
   - Copy `liu.gtab` to `C:\Program Files\HIME\data\`

4. **Registry Configuration**
   - HIME TSF module auto-registers with Windows
   - Select "HIME" from language bar
   - Choose "嘸蝦米" from input method list

### Alternative: Rime (小狼毫)

For Windows users preferring Rime:

1. Install 小狼毫 from https://rime.im/
2. Add Boshiamy schema to Rime configuration

---

## macOS Setup

### Using HIME for macOS

1. **Install HIME**
   - Download HIME.pkg from releases
   - Install and enable in System Preferences → Keyboard → Input Sources

2. **Configure via Settings**
   - Open HIME Preferences
   - Select "嘸蝦米" from available input methods
   - Ensure `liu.gtab` is in `~/Library/Application Support/HIME/`

3. **Programmatic Setup**

   ```objc
   #import "HimeEngine.h"

   HimeEngine *engine = [[HimeEngine alloc] initWithDataPath:dataPath];

   // Boshiamy is available through GTAB
   // Use hime_gtab_load_table_by_id internally
   ```

### Alternative: Rime (鼠鬚管)

1. **Install Rime**

   ```bash
   brew install --cask squirrel
   ```

2. **Install Boshiamy Schema**

   ```bash
   curl -fsSL https://raw.githubusercontent.com/ryanwuson/rime-liur/main/rime_liur_installer.sh | bash
   ```

3. **Redeploy**
   - Click Rime icon in menu bar
   - Select "Redeploy"
   - Wait for deployment to complete

---

## iOS Setup

### Using HIME Keyboard

1. **Install HIME Keyboard**
   - Install from App Store or TestFlight

2. **Enable Keyboard**
   - Go to Settings → General → Keyboard → Keyboards
   - Add "HIME" keyboard
   - Enable "Allow Full Access" if required

3. **Select Boshiamy**
   - Open any text input
   - Switch to HIME keyboard
   - Tap settings icon
   - Select "嘸蝦米" from input method list

4. **Import Table (if needed)**
   - Tables are bundled with the app
   - For custom tables, use Files app to import to HIME

### Developer Integration

```objc
#import "HimeEngine.h"

// Initialize with app bundle path
NSString *dataPath = [[NSBundle mainBundle] pathForResource:@"data" ofType:nil];
HimeEngine *engine = [[HimeEngine alloc] initWithDataPath:dataPath];

// Load Boshiamy through GTAB system
// Table ID: HIME_GTAB_BOSHIAMY (40)
```

---

## Android Setup

### Using HIME IME

1. **Install HIME**
   - Install from Play Store or APK

2. **Enable Input Method**
   - Go to Settings → System → Language & Input → On-screen keyboard
   - Enable "HIME"
   - Set as default keyboard

3. **Configure Boshiamy**
   - Open HIME Settings app
   - Select "Input Methods"
   - Enable "嘸蝦米 (Boshiamy)"

4. **Use Boshiamy**
   - Open any text field
   - Long-press globe/language key to switch methods
   - Select "嘸蝦米"

### Developer Integration (JNI)

```java
// HimeEngine.java
public class HimeEngine {
    static {
        System.loadLibrary("hime-jni");
    }

    // Load Boshiamy table
    public native boolean loadGtabTable(int tableId);

    public boolean loadBoshiamy() {
        return loadGtabTable(40);  // HIME_GTAB_BOSHIAMY
    }
}
```

```c
// hime_jni.c
JNIEXPORT jboolean JNICALL
Java_org_hime_android_core_HimeEngine_loadGtabTable(
    JNIEnv *env, jobject thiz, jint table_id) {

    HimeContext *ctx = get_context(env, thiz);
    return hime_gtab_load_table_by_id(ctx, table_id) == 0;
}
```

---

## HIME API Reference

### Loading Boshiamy Programmatically

```c
#include "hime-core.h"

// Initialize
hime_init("/path/to/data");
HimeContext *ctx = hime_context_new();

// Set GTAB mode
hime_set_input_method(ctx, HIME_IM_GTAB);

// Load Boshiamy (ID = 40)
int result = hime_gtab_load_table_by_id(ctx, HIME_GTAB_BOSHIAMY);
if (result == 0) {
    printf("Boshiamy loaded successfully\n");
}

// Process input
HimeKeyResult result = hime_process_key(ctx, keycode, character, modifiers);

// Get candidates
if (hime_has_candidates(ctx)) {
    int count = hime_get_candidate_count(ctx);
    for (int i = 0; i < count; i++) {
        char buffer[64];
        hime_get_candidate(ctx, i, buffer, sizeof(buffer));
        printf("Candidate %d: %s\n", i, buffer);
    }
}

// Cleanup
hime_context_free(ctx);
hime_cleanup();
```

### Searching for Boshiamy

```c
// Search by name
HimeSearchResult results[10];
HimeSearchFilter filter = {
    .query = "嘸蝦米",
    .method_type = HIME_IM_GTAB,
    .include_disabled = false
};
int count = hime_search_methods(&filter, results, 10);

// Or search GTAB tables directly
HimeGtabInfo tables[10];
count = hime_gtab_search_tables("嘸蝦米", tables, 10);
```

### Table ID Constants

```c
typedef enum {
    HIME_GTAB_BOSHIAMY = 40,  /* 嘸蝦米 */
    // Other tables...
} HimeGtabTable;
```

---

## Troubleshooting

### Table File Not Found

**Symptom:** Boshiamy not appearing in input method list

**Solution:**

1. Verify table file exists:

   ```bash
   # Linux
   ls ~/.config/hime/liu.gtab
   ls /usr/share/hime/table/liu.gtab

   # macOS
   ls ~/Library/Application\ Support/HIME/liu.gtab
   ```

2. Convert .cin to .gtab if needed:
   ```bash
   hime-cin2gtab liu.cin liu.gtab
   ```

### Input Not Working

**Symptom:** Keys not producing Chinese characters

**Solution:**

1. Verify GTAB mode is active:

   ```c
   HimeInputMethod method = hime_get_input_method(ctx);
   assert(method == HIME_IM_GTAB);
   ```

2. Check Chinese mode is enabled:
   ```c
   hime_set_chinese_mode(ctx, true);
   ```

### Wrong Characters Output

**Symptom:** Simplified instead of Traditional characters

**Solution:**

```c
// Set Traditional Chinese output
hime_set_output_variant(ctx, HIME_OUTPUT_TRADITIONAL);
```

---

## Resources

- **HIME GitHub:** https://github.com/hime-ime/hime
- **Boshiamy Official:** https://boshiamy.com/
- **Rime (alternative):** https://rime.im/

C:\Program Files\BoshiamyTIP\liu-uni.tab

## License Note

The Boshiamy table file (`liu.gtab` / `liu.cin`) is proprietary and must be obtained separately from Liu's Studio (行易公司). HIME provides the framework to use Boshiamy but does not include the table file.
