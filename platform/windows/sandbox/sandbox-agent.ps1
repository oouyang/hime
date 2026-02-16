# sandbox-agent.ps1 — Polling agent that runs inside Windows Sandbox
# Watches C:\hime-ipc for .cmd.json files, executes commands, writes .resp.json

$ErrorActionPreference = 'Stop'
$IpcDir = 'C:\hime-ipc'
$ScreenshotDir = Join-Path $IpcDir 'screenshots'
$PollIntervalMs = 500

# --- P/Invoke for mouse and keyboard ---
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;

public class NativeInput {
    [DllImport("user32.dll")]
    public static extern bool SetCursorPos(int X, int Y);

    [DllImport("user32.dll")]
    public static extern void mouse_event(uint dwFlags, int dx, int dy, uint dwData, IntPtr dwExtraInfo);

    [DllImport("user32.dll")]
    public static extern void keybd_event(byte bVk, byte bScan, uint dwFlags, IntPtr dwExtraInfo);

    public const uint MOUSEEVENTF_LEFTDOWN   = 0x0002;
    public const uint MOUSEEVENTF_LEFTUP     = 0x0004;
    public const uint MOUSEEVENTF_RIGHTDOWN  = 0x0008;
    public const uint MOUSEEVENTF_RIGHTUP    = 0x0010;
    public const uint KEYEVENTF_KEYUP        = 0x0002;

    // Virtual key codes
    public const byte VK_LWIN      = 0x5B;
    public const byte VK_RWIN      = 0x5C;
    public const byte VK_SHIFT     = 0x10;
    public const byte VK_CONTROL   = 0x11;
    public const byte VK_MENU      = 0x12; // Alt
    public const byte VK_RETURN    = 0x0D;
    public const byte VK_ESCAPE    = 0x1B;
    public const byte VK_TAB       = 0x09;
    public const byte VK_BACK      = 0x08;
    public const byte VK_DELETE    = 0x2E;
    public const byte VK_SPACE     = 0x20;
    public const byte VK_LEFT      = 0x25;
    public const byte VK_UP        = 0x26;
    public const byte VK_RIGHT     = 0x27;
    public const byte VK_DOWN      = 0x28;
    public const byte VK_HOME      = 0x24;
    public const byte VK_END       = 0x23;
    public const byte VK_F1        = 0x70;
}
'@

# Screenshot requires System.Drawing
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms

# --- Helper: virtual key lookup ---
function Get-VirtualKey([string]$name) {
    $map = @{
        'Win'       = [NativeInput]::VK_LWIN
        'LWin'      = [NativeInput]::VK_LWIN
        'RWin'      = [NativeInput]::VK_RWIN
        'Shift'     = [NativeInput]::VK_SHIFT
        'Ctrl'      = [NativeInput]::VK_CONTROL
        'Control'   = [NativeInput]::VK_CONTROL
        'Alt'       = [NativeInput]::VK_MENU
        'Enter'     = [NativeInput]::VK_RETURN
        'Return'    = [NativeInput]::VK_RETURN
        'Escape'    = [NativeInput]::VK_ESCAPE
        'Esc'       = [NativeInput]::VK_ESCAPE
        'Tab'       = [NativeInput]::VK_TAB
        'Backspace' = [NativeInput]::VK_BACK
        'Delete'    = [NativeInput]::VK_DELETE
        'Space'     = [NativeInput]::VK_SPACE
        'Left'      = [NativeInput]::VK_LEFT
        'Up'        = [NativeInput]::VK_UP
        'Right'     = [NativeInput]::VK_RIGHT
        'Down'      = [NativeInput]::VK_DOWN
        'Home'      = [NativeInput]::VK_HOME
        'End'       = [NativeInput]::VK_END
    }

    # Check named keys
    if ($map.ContainsKey($name)) { return $map[$name] }

    # F1-F24
    if ($name -match '^F(\d+)$') {
        $n = [int]$Matches[1]
        if ($n -ge 1 -and $n -le 24) { return [byte]([NativeInput]::VK_F1 + $n - 1) }
    }

    # Single character A-Z, 0-9
    if ($name.Length -eq 1) {
        $c = [char]$name.ToUpper()
        if ($c -ge [char]'A' -and $c -le [char]'Z') { return [byte]$c }
        if ($c -ge [char]'0' -and $c -le [char]'9') { return [byte]$c }
    }

    # OEM keys
    $oemMap = @{
        'Backtick'    = [byte]0xC0
        'OemTilde'    = [byte]0xC0
        'Minus'       = [byte]0xBD
        'Equals'      = [byte]0xBB
        'LeftBracket' = [byte]0xDB
        'RightBracket'= [byte]0xDD
        'Backslash'   = [byte]0xDC
        'Semicolon'   = [byte]0xBA
        'Quote'       = [byte]0xDE
        'Comma'       = [byte]0xBC
        'Period'      = [byte]0xBE
        'Slash'       = [byte]0xBF
    }
    if ($oemMap.ContainsKey($name)) { return $oemMap[$name] }

    return $null
}

# --- Tool handlers ---

function Invoke-Exec($params) {
    $cmd = $params.command
    if (-not $cmd) { return @{ error = 'missing "command" parameter' } }

    $tmpOut = [System.IO.Path]::GetTempFileName()
    $tmpErr = [System.IO.Path]::GetTempFileName()
    try {
        $proc = Start-Process -FilePath 'cmd.exe' -ArgumentList "/c $cmd" `
            -RedirectStandardOutput $tmpOut -RedirectStandardError $tmpErr `
            -NoNewWindow -Wait -PassThru
        $stdout = Get-Content -Path $tmpOut -Raw -ErrorAction SilentlyContinue
        $stderr = Get-Content -Path $tmpErr -Raw -ErrorAction SilentlyContinue
        return @{
            exitcode = $proc.ExitCode
            stdout   = if ($stdout) { $stdout.TrimEnd() } else { '' }
            stderr   = if ($stderr) { $stderr.TrimEnd() } else { '' }
        }
    } finally {
        Remove-Item $tmpOut, $tmpErr -Force -ErrorAction SilentlyContinue
    }
}

function Invoke-Screenshot($params) {
    if (-not (Test-Path $ScreenshotDir)) { New-Item -ItemType Directory -Path $ScreenshotDir -Force | Out-Null }

    $ts = (Get-Date).ToString('yyyyMMdd-HHmmss-fff')
    $filename = "screenshot-$ts.png"
    $filepath = Join-Path $ScreenshotDir $filename

    $bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
    $bmp = New-Object System.Drawing.Bitmap($bounds.Width, $bounds.Height)
    $gfx = [System.Drawing.Graphics]::FromImage($bmp)
    $gfx.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)
    $gfx.Dispose()
    $bmp.Save($filepath, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()

    # Return both sandbox path and host path (mapped folder)
    return @{
        sandbox_path = $filepath
        host_path    = $filepath -replace '^C:\\hime-ipc\\', 'C:\mu\tmp\hime-sandbox-ipc\'
        width        = $bounds.Width
        height       = $bounds.Height
    }
}

function Invoke-ReadFile($params) {
    $path = $params.path
    if (-not $path) { return @{ error = 'missing "path" parameter' } }
    if (-not (Test-Path $path)) { return @{ error = "file not found: $path" } }

    $content = Get-Content -Path $path -Raw -ErrorAction Stop
    return @{
        path    = $path
        content = $content
    }
}

function Invoke-Click($params) {
    $x = $params.x
    $y = $params.y
    $button = if ($params.button) { $params.button } else { 'left' }

    if ($null -eq $x -or $null -eq $y) { return @{ error = 'missing "x" and/or "y" parameters' } }

    [NativeInput]::SetCursorPos([int]$x, [int]$y)
    Start-Sleep -Milliseconds 50

    if ($button -eq 'right') {
        [NativeInput]::mouse_event([NativeInput]::MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, [IntPtr]::Zero)
        Start-Sleep -Milliseconds 30
        [NativeInput]::mouse_event([NativeInput]::MOUSEEVENTF_RIGHTUP, 0, 0, 0, [IntPtr]::Zero)
    } else {
        [NativeInput]::mouse_event([NativeInput]::MOUSEEVENTF_LEFTDOWN, 0, 0, 0, [IntPtr]::Zero)
        Start-Sleep -Milliseconds 30
        [NativeInput]::mouse_event([NativeInput]::MOUSEEVENTF_LEFTUP, 0, 0, 0, [IntPtr]::Zero)
    }

    return @{ clicked = @{ x = $x; y = $y; button = $button } }
}

function Invoke-Type($params) {
    $text = $params.text
    if ($null -eq $text) { return @{ error = 'missing "text" parameter' } }

    # Check if text is pure ASCII printable
    $isAscii = $true
    foreach ($c in $text.ToCharArray()) {
        if ([int]$c -lt 32 -or [int]$c -gt 126) { $isAscii = $false; break }
    }

    if ($isAscii) {
        # Use SendKeys for ASCII text (need to escape special chars)
        $escaped = $text -replace '([+^%~(){}])', '{$1}'
        [System.Windows.Forms.SendKeys]::SendWait($escaped)
    } else {
        # Use clipboard for Unicode text
        [System.Windows.Forms.Clipboard]::SetText($text)
        Start-Sleep -Milliseconds 50
        [System.Windows.Forms.SendKeys]::SendWait('^v')
        Start-Sleep -Milliseconds 50
    }

    return @{ typed = $text; method = if ($isAscii) { 'sendkeys' } else { 'clipboard' } }
}

function Invoke-Key($params) {
    $combo = $params.combo
    if (-not $combo) { return @{ error = 'missing "combo" parameter' } }

    # Parse combo like "Win+Space", "Ctrl+Shift+A", "Enter"
    $parts = $combo -split '\+'
    $modifiers = @()
    $mainKey = $null

    foreach ($part in $parts) {
        $part = $part.Trim()
        if ($part -in 'Win', 'LWin', 'RWin', 'Ctrl', 'Control', 'Shift', 'Alt') {
            $modifiers += $part
        } else {
            $mainKey = $part
        }
    }

    if (-not $mainKey -and $modifiers.Count -eq 1) {
        # Single modifier key press (e.g., just "Win")
        $mainKey = $modifiers[0]
        $modifiers = @()
    }

    if (-not $mainKey) { return @{ error = "could not parse key combo: $combo" } }

    $mainVk = Get-VirtualKey $mainKey
    if ($null -eq $mainVk) { return @{ error = "unknown key: $mainKey" } }

    # Press modifiers
    foreach ($mod in $modifiers) {
        $vk = Get-VirtualKey $mod
        [NativeInput]::keybd_event($vk, 0, 0, [IntPtr]::Zero)
        Start-Sleep -Milliseconds 20
    }

    # Press and release main key
    [NativeInput]::keybd_event($mainVk, 0, 0, [IntPtr]::Zero)
    Start-Sleep -Milliseconds 30
    [NativeInput]::keybd_event($mainVk, 0, [NativeInput]::KEYEVENTF_KEYUP, [IntPtr]::Zero)

    # Release modifiers in reverse order
    $revMods = $modifiers.Clone()
    [Array]::Reverse($revMods)
    foreach ($mod in $revMods) {
        $vk = Get-VirtualKey $mod
        Start-Sleep -Milliseconds 20
        [NativeInput]::keybd_event($vk, 0, [NativeInput]::KEYEVENTF_KEYUP, [IntPtr]::Zero)
    }

    return @{ pressed = $combo }
}

# --- Main loop ---

Write-Host "HIME Sandbox Agent starting..."
Write-Host "IPC directory: $IpcDir"

# Ensure directories exist
if (-not (Test-Path $IpcDir)) { New-Item -ItemType Directory -Path $IpcDir -Force | Out-Null }
if (-not (Test-Path $ScreenshotDir)) { New-Item -ItemType Directory -Path $ScreenshotDir -Force | Out-Null }

# Clean up stale command/response files from previous runs
Get-ChildItem -Path $IpcDir -Filter '*.cmd.json' -ErrorAction SilentlyContinue | Remove-Item -Force
Get-ChildItem -Path $IpcDir -Filter '*.resp.json' -ErrorAction SilentlyContinue | Remove-Item -Force
Get-ChildItem -Path $IpcDir -Filter '*.cmd.tmp' -ErrorAction SilentlyContinue | Remove-Item -Force
Get-ChildItem -Path $IpcDir -Filter '*.resp.tmp' -ErrorAction SilentlyContinue | Remove-Item -Force

Write-Host "Agent ready. Polling for commands..."

while ($true) {
    $cmdFiles = Get-ChildItem -Path $IpcDir -Filter '*.cmd.json' -ErrorAction SilentlyContinue
    foreach ($cmdFile in $cmdFiles) {
        $id = $cmdFile.BaseName -replace '\.cmd$', ''
        Write-Host "Processing command: $id"

        try {
            $raw = Get-Content -Path $cmdFile.FullName -Raw
            $cmd = $raw | ConvertFrom-Json

            $tool = $cmd.tool
            $params = $cmd.params

            $result = switch ($tool) {
                'sandbox_exec'       { Invoke-Exec $params }
                'sandbox_screenshot' { Invoke-Screenshot $params }
                'sandbox_read_file'  { Invoke-ReadFile $params }
                'sandbox_click'      { Invoke-Click $params }
                'sandbox_type'       { Invoke-Type $params }
                'sandbox_key'        { Invoke-Key $params }
                default              { @{ error = "unknown tool: $tool" } }
            }

            $response = @{
                id     = $id
                tool   = $tool
                result = $result
            }
        } catch {
            $response = @{
                id    = $id
                tool  = if ($tool) { $tool } else { 'unknown' }
                result = @{ error = $_.Exception.Message }
            }
        }

        # Atomic write: .resp.tmp -> .resp.json
        $respTmp = Join-Path $IpcDir "$id.resp.tmp"
        $respFinal = Join-Path $IpcDir "$id.resp.json"
        $response | ConvertTo-Json -Depth 10 | Set-Content -Path $respTmp -Encoding UTF8
        Rename-Item -Path $respTmp -NewName "$id.resp.json" -Force

        # Remove command file
        Remove-Item -Path $cmdFile.FullName -Force
        Write-Host "  Done: $id"
    }

    Start-Sleep -Milliseconds $PollIntervalMs
}
