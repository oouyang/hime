# HIME Windows Sandbox Testing

Test the HIME input method in an isolated Windows Sandbox environment.
No reboots, no DLL locking, no stale processes — each test starts fresh.

## Prerequisites

- Windows 10/11 Pro or Enterprise
- Windows Sandbox enabled:
  ```powershell
  Enable-WindowsOptionalFeature -FeatureName "Containers-DisposableClientVM" -All -Online
  ```
  Reboot if prompted.

## Test Workflow

### 1. Build (WSL)

```bash
cd /opt/ws/hime/platform/windows/build
make -j$(nproc)
```

### 2. Prepare sandbox package (Windows, elevated)

```cmd
"\\wsl$\Ubuntu\opt\ws\hime\platform\windows\sandbox\prepare-sandbox.bat"
```

Copies DLLs, test exe, data files, and scripts to `C:\Program Files\HIME\sandbox\`.

### 3. Launch sandbox

Double-click or run:

```cmd
"C:\Program Files\HIME\sandbox\HIME-Sandbox.wsb"
```

The sandbox auto-runs `deploy-and-test.bat` which:
1. Copies files to `C:\Program Files\HIME\bin\`
2. Runs all engine tests (`test-hime-core.exe`)
3. Registers the IME via `regsvr32`
4. Shows setup instructions

### 4. Manual testing in sandbox

1. Open **Settings > Time & Language > Language & Region**
2. Add **Chinese (Traditional, Taiwan)** if not present
3. Click it > **Language options** > **Add a keyboard**
4. Select **HIME** (in Debug builds, look for `[DEBUG]` suffix)
5. Open **Notepad**
6. Press **Win+Space** to switch to HIME
7. Test typing:
   - `a` `8` `Space` `1` → should commit a Chinese character (媽)
   - `.` `Space` `1` → should commit another character (歐)
   - Characters should appear left-to-right in reading order

### 5. Check logs

Inside the sandbox:

```cmd
dir c:\mu\tmp\hime\test-*.log
type c:\mu\tmp\hime\test-*.log
```

### 6. Close sandbox

Close the sandbox window. Everything is destroyed. Repeat from step 1.

## Quick iteration cycle

```
Edit code (WSL) → make → prepare-sandbox.bat → double-click .wsb → test → close → repeat
```

## Files

| File | Purpose |
|------|---------|
| `prepare-sandbox.bat` | Copies build output to sandbox package directory |
| `deploy-and-test.bat` | Runs inside sandbox: installs, tests, registers |
| `HIME-Sandbox.wsb` | Windows Sandbox configuration file |

## Known Issues

### Reverse character order in Windows 11 Notepad

**Symptom**: When typing multiple Chinese characters in sequence, they appear in
reverse order (e.g., typing 你好 produces 好你) — but only in Windows 11's new
Notepad. Other applications (WordPad, browsers, Word, etc.) display correct order.

**Root cause**: Windows 11 shipped a rewritten Notepad that uses a non-standard TSF
composition model. After `EndComposition`, the new Notepad resets the cursor to the
start of the previous composition range instead of placing it after the committed
text. This causes the next composition to start before the previous character.

This is a known compatibility issue affecting many third-party IMEs — not specific
to HIME.

**Workaround**: Enable the legacy IME framework in Windows:

1. Open **Settings > Time & Language > Language & Region**
2. Click **Chinese (Traditional, Taiwan)**
3. Click **Language options**
4. Under Keyboards, click **Microsoft Zhuyin** (or any Microsoft IME)
5. Click **Options**
6. Turn ON **"Use previous version of Microsoft IME"**

This switches the system TSF framework to the legacy version, which handles
`EndComposition` cursor positioning correctly. All apps including the new Notepad
will work with correct character order.

**Verification**: Test in WordPad (`write.exe`) or a browser to confirm the IME
works correctly in standard apps. If characters appear in correct order there but
reversed in Notepad, the issue is Notepad-specific.

### Only EN / Zhuyin / Cangjie in toggle cycle (stale settings)

**Symptom**: After upgrading HIME, Ctrl+\`/F4 only cycles through 1-3 methods
instead of all 22 available methods.

**Root cause**: `HKCU\Software\HIME\EnabledMethods` contains a stale value from
an older version (e.g. `"liu.gtab"` or `"zhuyin,cj5.gtab"`). The `_LoadSettings()`
function disables all methods first, then only enables the ones listed in the
registry. A stale value with few entries overrides the new all-enabled defaults.

**Fix**: Delete the stale registry key:
```cmd
reg delete HKCU\Software\HIME /f
```
Then switch away from HIME and back (Win+Space twice). All available methods
will use the default (all enabled). The Settings dialog saves preferences back
to the registry when you click OK.

**Debug process**: Check TSF logs (`c:\mu\tmp\hime\test-*.log`) for:
- `DiscoverMethods: found 22 methods` — all 22 should show `enabled=1`
- `LoadSettings: '...'` — if present, shows the registry value being loaded
- `Toggle: now '...'` — shows which methods are in the cycle

### DLL not updating after copy

**Symptom**: After copying new DLLs, behavior doesn't change. Logs still show
old behavior (e.g. `enabled=0` for most methods).

**Root cause**: Windows keeps TSF DLLs loaded in all processes with input contexts.
`copy /Y` silently fails on locked files. Even `regsvr32 /u` + `regsvr32 /s` only
loads the DLL in the regsvr32 process — other processes keep the old DLL.

**Fix**: Use `hime-install.exe` (handles locked DLLs via rename-to-`.old` trick),
then sign out and sign back in. Verify by checking the timestamp:
```cmd
dir "C:\Program Files\HIME\hime-tsf.dll"
```

## Troubleshooting

- **Sandbox won't start**: Ensure Windows Sandbox is enabled and your CPU supports virtualization (VT-x/AMD-V). If "failed to initialize", kill all `WindowsSandbox*` processes and restart `CmService`: `Restart-Service CmService -Force`
- **Engine tests fail**: Check build output. The IME won't be registered if tests fail.
- **HIME not in keyboard list**: Make sure Chinese (Traditional) language is added first
- **Can't switch to HIME**: Try Win+Space multiple times, or check if it appears in the taskbar input indicator
- **No log files**: Check all three paths: `c:\mu\tmp\hime\`, `C:\Program Files\HIME\bin\`, and `%TEMP%`
- **Reverse order in Notepad**: See "Known Issues" above
- **Only 1-3 methods in cycle**: See "Only EN / Zhuyin / Cangjie in toggle cycle" above
