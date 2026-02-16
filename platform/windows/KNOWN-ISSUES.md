# HIME Windows — Known Issues

## Issues Found and Fixed

### 1. Only 3 input methods in toggle cycle (FIXED)

**Symptom**: Ctrl+`/F4 only cycles EN → Zhuyin → Cangjie 5 → EN. The other
18 GTAB methods (Simplex, DaYi, Array, Boshiamy, Pinyin, Hangul, etc.) are
not reachable via keyboard toggle, right-click menu, or settings.

**Root cause**: The TSF DLL (`hime-tsf.cpp`) hardcoded only 3 methods with
`g_enabledZhuyin`, `g_enabledCangjie`, `g_enabledBoshiamy` flags. The core
library registers 21 GTAB methods in `GTAB_REGISTRY[]` but the Windows UI
never exposed them.

**Fix**: Replaced the hardcoded 3-method system with a data-driven
`HimeMethodSlot g_methods[]` array populated by `_DiscoverMethods()` at
Activate time. All available `.gtab` files are now discovered, and all
methods default to enabled. The toggle cycle, right-click menu, settings
dialog, and icon system are all driven by this array.

**Commit**: `124c789` feat(windows): add all 21 GTAB input methods to Windows UI

---

### 2. Stale registry settings reduce methods after upgrade (FIXED)

**Symptom**: After deploying the new multi-method DLL, toggle still only
cycles 1-3 methods. Logs show `DiscoverMethods` with `enabled=1` for all
methods, but then `LoadSettings: 'liu.gtab'` overrides to just one method.

**Root cause**: `_LoadSettings()` reads `HKCU\Software\HIME\EnabledMethods`
and disables all methods first, then enables only the listed ones. A stale
value from a previous session (e.g. `"liu.gtab"`) overrides the new
all-enabled defaults.

**Fix**: `_SaveSettings()` now writes a `MethodCount` DWORD alongside the
enabled list. `_LoadSettings()` rejects the saved value if `MethodCount`
doesn't match the current `g_methodCount` (22), and deletes the stale key.

**Manual workaround** (if running an older DLL):
```cmd
reg delete HKCU\Software\HIME /f
```

**Commit**: `124c789` (same commit, amended)

---

### 3. hime.png app icon missing from build output (FIXED)

**Symptom**: The main HIME language bar button shows a fallback "姬" GDI icon
instead of the hime.png artwork.

**Root cause**: `hime.png` lives at `icons/hime.png` (repository root) but
CMakeLists.txt only copied from `windows/icons/` and `icons/blue/`. The file
was never present in the build output.

**Fix**: Added explicit copy rule in CMakeLists.txt for `icons/hime.png`.

**Commit**: `124c789` (same commit)

---

### 4. DLL not updating after copy (DOCUMENTED)

**Symptom**: After copying new DLLs to `C:\Program Files\HIME\`, behavior
doesn't change. TSF logs still show old code paths.

**Root cause**: Windows keeps TSF DLLs loaded in all processes that have input
contexts (Explorer, browsers, editors, `ctfmon.exe`). `copy /Y` silently fails
on locked files — it returns success but doesn't write. Even `regsvr32 /u` +
`regsvr32 /s` only loads the DLL in the regsvr32 process; all other processes
keep the old DLL mapped in memory.

**Workaround**: Use `hime-install.exe` which handles locked DLLs via the
rename-to-`.old` trick, then sign out and sign back in. Verify by checking
the file timestamp:
```cmd
dir "C:\Program Files\HIME\hime-tsf.dll"
```

---

### 5. Debug logging added for 4 known issues (NEW)

Added `hime_log()` calls (active in Debug builds only) to:
- `DllRegisterServer` / `DllUnregisterServer` — logs each TSF registration step
  and HRESULT codes
- `_InitLanguageBar` — logs AddItem results for both language bar buttons
- `ModeButton::GetIcon` — logs icon handle and current method index
- `ModeButton::Update` — logs whether the sink is connected
- `_ShowToolbar` — logs window creation, class registration, and errors
- `_ShowSettingsDialog` — logs buffer usage and DialogBoxIndirectW result

Logs write to `c:\mu\tmp\hime\test-<PID>.log`.

---

## Open / Unresolved Issues

### 6. Uninstall ghost entry in keyboard list

**Symptom**: After uninstalling HIME, a ghost "無法使用的輸入法" (unavailable
input method) entry remains in Windows Settings > Language > Keyboard list.

**Root cause**: `regsvr32 /u` removes HKLM registration (TSF TIP, categories,
CLSID) but Windows caches user keyboard preferences in multiple HKCU locations:
- `HKCU\Software\Microsoft\CTF\TIP\{CLSID}`
- `HKCU\Software\Microsoft\CTF\SortOrder\AssemblyItem\0x00000404`
- `HKCU\Software\Microsoft\CTF\SortOrder\Language\00000404`
- `HKCU\Keyboard Layout\Preload`
- `HKCU\Keyboard Layout\Substitutes`

The uninstaller (`hime-uninstall.c`) already cleans all 5 locations. The ghost
appears when the user uses `regsvr32 /u` directly instead of the full uninstaller.

**Status**: The uninstaller handles this correctly. Needs user education to use
`hime-uninstall.exe` instead of raw `regsvr32`.

**Diagnostic**: Check HKCU for HIME's CLSID after uninstall:
```cmd
reg query "HKCU\Software\Microsoft\CTF\TIP\{B8A45C32-5F6D-4E2A-9C1B-0D3E4F5A6B7C}" 2>nul
reg query "HKCU\Keyboard Layout\Preload" | findstr /i "b8a45c32"
```

---

### 7. System keyboard settings — HIME may not appear after registration

**Symptom**: After `regsvr32 /s hime-tsf.dll`, HIME doesn't appear in
Settings > Language > Add a keyboard.

**Possible causes**:
- Chinese (Traditional, Taiwan) language not added to Windows
- TSF registration succeeded but category registration failed (check log for
  `RegisterCategory` HRESULTs)
- The user needs to sign out/in for Windows to pick up the new TIP

**Status**: Registration code is correct (verified by logs showing all
hr=0x00000000). This is a Windows caching issue.

**Diagnostic**: Check TSF log for `DllRegisterServer` entries, then verify:
```cmd
reg query "HKLM\SOFTWARE\Microsoft\CTF\TIP\{B8A45C32-5F6D-4E2A-9C1B-0D3E4F5A6B7C}" /s
```

---

### 8. HIME floating toolbar not visible

**Symptom**: Enabling "IME Toolbar" from the right-click menu doesn't show
the floating toolbar window.

**Possible causes**:
- TSF DLLs run inside the host application's process. `CreateWindowExW` with
  `WS_POPUP` may fail if the process doesn't have a message loop compatible
  with the toolbar's window class.
- The toolbar is positioned at bottom-right of the work area, which may be
  off-screen on multi-monitor setups.
- `RegisterClassW` may fail silently in some security contexts.

**Status**: Open. The toolbar works in some applications but not all. Debug
logging added to `_ShowToolbar()` to capture `CreateWindowExW` failures.

**Diagnostic**: Check TSF log for `_ShowToolbar` entries:
```
_ShowToolbar: creating window at (x,y) WxH hInst=...
_ShowToolbar: CreateWindowExW FAILED err=...
```

---

### 9. Mode indicator icon not displayed in system tray

**Symptom**: The HIME mode icon (showing current input method) doesn't appear
in the Windows system tray / language bar area.

**Possible causes**:
- `ITfLangBarItemMgr::AddItem` failed for the mode button (check log for
  `_InitLanguageBar: AddItem(ModeButton)` HRESULT)
- `ITfLangBarItemSink` is NULL — Windows never advised the sink, so
  `OnUpdate` calls are no-ops (check log for `ModeButton::Update: sink=0000...`)
- Icon loading failed — `_CreateModeIconForContext` returned NULL (check log
  for `ModeButton::GetIcon: icon=0000000000000000`)
- Windows 11 may suppress language bar items in favor of the new input indicator

**Status**: Open. The mode button registers successfully (AddItem hr=0x00000000)
and GetIcon returns valid handles, but visibility depends on Windows language bar
settings. The main HIME button (with app icon and right-click menu) is always
visible as it uses `TF_LBI_STYLE_BTN_MENU`.

**Diagnostic**: Check TSF log for:
```
_InitLanguageBar: AddItem(ModeButton) hr=...
ModeButton::GetIcon: ctx=... icon=... methodIdx=...
ModeButton::Update: sink=...
```

---

### 10. Reverse character order in Windows 11 Notepad

**Symptom**: Typing multiple Chinese characters produces them in reverse order
(e.g., 你好 → 好你). Only affects Windows 11's new Notepad; other apps work.

**Root cause**: Windows 11 Notepad uses a non-standard TSF composition model.
After `EndComposition`, it resets the cursor to the start of the previous
composition range instead of placing it after the committed text.

**Status**: Won't fix (upstream Windows bug affecting all third-party IMEs).

**Workaround**: Enable legacy IME framework:
1. Settings > Language > Chinese (Traditional) > Language options
2. Click Microsoft Zhuyin > Options
3. Turn ON "Use previous version of Microsoft IME"

---

## Debug Checklist

For any Windows issue, collect these from `c:\mu\tmp\hime\test-*.log`:

| What to look for | Diagnoses |
|---|---|
| `DiscoverMethods: found N methods` | Method discovery working, check `enabled=` flags |
| `LoadSettings: '...'` | Registry overriding defaults — may be stale |
| `Toggle: now '...'` | Which methods are in the cycle |
| `DllRegisterServer: ... hr=0x...` | Registration failures |
| `_InitLanguageBar: AddItem(...) hr=0x...` | Language bar button failures |
| `ModeButton::GetIcon: icon=... methodIdx=...` | Icon loading (NULL = failed) |
| `ModeButton::Update: sink=...` | NULL sink = tray updates won't work |
| `_ShowToolbar: FAILED err=...` | Toolbar window creation failures |
| `_ShowSettingsDialog: ... returned -1` | Settings dialog creation failure |

To force all methods enabled (emergency workaround):
```cmd
reg delete HKCU\Software\HIME /f
```

To check HIME registration health:
```cmd
reg query "HKCR\CLSID\{B8A45C32-5F6D-4E2A-9C1B-0D3E4F5A6B7C}\InprocServer32"
reg query "HKLM\SOFTWARE\Microsoft\CTF\TIP\{B8A45C32-5F6D-4E2A-9C1B-0D3E4F5A6B7C}" /s
```
