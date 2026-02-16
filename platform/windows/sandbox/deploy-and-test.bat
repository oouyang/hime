@echo off
chcp 65001 >nul
title HIME Deploy and Test
color 0A
echo ============================================
echo  HIME Input Method - Deploy and Test Script
echo ============================================
echo.

:: Create log directory
mkdir "c:\mu\tmp\hime" 2>nul
set LOGFILE=c:\mu\tmp\hime\deploy-test.log

:: Create install directory
mkdir "C:\Program Files\HIME\bin\data" 2>nul
mkdir "C:\Program Files\HIME\bin\icons" 2>nul

:: Copy files from sandbox mapped folder
echo [1/7] Copying files...
copy /Y "%~dp0hime-core.dll" "C:\Program Files\HIME\bin\" >nul
copy /Y "%~dp0hime-tsf.dll" "C:\Program Files\HIME\bin\" >nul
copy /Y "%~dp0hime-install.exe" "C:\Program Files\HIME\bin\" >nul
copy /Y "%~dp0hime-uninstall.exe" "C:\Program Files\HIME\bin\" >nul
copy /Y "%~dp0test-hime-core.exe" "C:\Program Files\HIME\bin\" >nul
copy /Y "%~dp0test-hime-installer.exe" "C:\Program Files\HIME\bin\" >nul
copy /Y "%~dp0data\pho.tab2" "C:\Program Files\HIME\bin\data\" >nul
copy /Y "%~dp0data\*.kbm" "C:\Program Files\HIME\bin\data\" >nul
copy /Y "%~dp0data\*.gtab" "C:\Program Files\HIME\bin\data\" >nul
copy /Y "%~dp0icons\*.png" "C:\Program Files\HIME\bin\icons\" >nul 2>nul
copy /Y "%~dp0icons\*.ico" "C:\Program Files\HIME\bin\icons\" >nul 2>nul
echo   Done.
echo.

:: Run engine tests
echo [2/7] Engine tests (test-hime-core)...
echo.
cd /d "C:\Program Files\HIME\bin"
test-hime-core.exe
if %errorlevel% neq 0 (
    echo.
    color 0C
    echo *** ENGINE TESTS FAILED ***
    pause
    exit /b 1
)

:: Run installer tests (issue #1-#4)
echo.
echo [3/7] Installer and integration tests (test-hime-installer)...
echo.
test-hime-installer.exe
echo.

:: Register the IME
echo [4/7] Registering HIME IME...
regsvr32 /s "C:\Program Files\HIME\bin\hime-tsf.dll"
if %errorlevel% equ 0 (
    echo   Registration successful.
) else (
    echo   Registration FAILED (exit code %errorlevel%)
)

:: ============================================
:: Diagnostics for the 4 known issues
:: ============================================
echo.
echo [5/7] Diagnostics for known issues...
echo.

echo --- Issue #1: Uninstall ghost entries ---
reg query "HKCU\Software\Microsoft\CTF\TIP\{B8A45C32-5F6D-4E2A-9C1B-0D3E4F5A6B7C}" >nul 2>nul
if %errorlevel% equ 0 (
    echo   [FOUND] HKCU CTF\TIP entry exists - would be ghost after uninstall
) else (
    echo   [OK] No HKCU CTF\TIP entry yet (clean - not added to user keyboards)
)
reg query "HKCU\Keyboard Layout\Preload" 2>nul | findstr /i "b8a45c32" >nul 2>nul
if %errorlevel% equ 0 (
    echo   [FOUND] HIME in Keyboard Layout\Preload
) else (
    echo   [OK] No HIME in Preload
)
echo.

echo --- Issue #2: System keyboard settings ---
reg query "HKCR\CLSID\{B8A45C32-5F6D-4E2A-9C1B-0D3E4F5A6B7C}" >nul 2>nul
if %errorlevel% equ 0 (
    echo   [OK] CLSID registered in HKCR
) else (
    echo   [FAIL] CLSID NOT in HKCR - regsvr32 failed?
)
reg query "HKLM\SOFTWARE\Microsoft\CTF\TIP\{B8A45C32-5F6D-4E2A-9C1B-0D3E4F5A6B7C}" >nul 2>nul
if %errorlevel% equ 0 (
    echo   [OK] TSF TIP registered in HKLM
) else (
    echo   [FAIL] TSF TIP NOT in HKLM - won't appear in Settings
)
reg query "HKLM\SOFTWARE\Microsoft\CTF\TIP\{B8A45C32-5F6D-4E2A-9C1B-0D3E4F5A6B7C}\LanguageProfile" >nul 2>nul
if %errorlevel% equ 0 (
    echo   [OK] Language profile registered
) else (
    echo   [FAIL] No language profile - won't appear in keyboard list
)
echo.

echo --- Issue #3: HIME toolbar ---
echo   Toolbar is created at runtime. Requires active HIME session.
echo   After adding HIME keyboard and switching to it, enable via right-click menu.
echo   Check TSF log for _ShowToolbar entries.
echo.

echo --- Issue #4: Mode indicator in tray ---
set /a IC=0
for %%f in ("C:\Program Files\HIME\bin\icons\*.png") do set /a IC+=1
echo   Icon PNGs installed: %IC%
if exist "C:\Program Files\HIME\bin\icons\hime-tray.png" (echo   [OK] hime-tray.png) else (echo   [MISSING] hime-tray.png)
if exist "C:\Program Files\HIME\bin\icons\juyin.png" (echo   [OK] juyin.png) else (echo   [MISSING] juyin.png)
if exist "C:\Program Files\HIME\bin\icons\cj5.png" (echo   [OK] cj5.png) else (echo   [MISSING] cj5.png)
if exist "C:\Program Files\HIME\bin\icons\hime.png" (echo   [OK] hime.png) else (echo   [MISSING] hime.png)
echo   Mode indicator requires HIME to be activated. After switching to HIME,
echo   check TSF log for ModeButton::GetIcon and _InitLanguageBar entries.
echo.

:: Dump registration details to log
echo [6/7] Writing detailed log to %LOGFILE%...
echo ============================================ > "%LOGFILE%"
echo HIME Deploy Log - %date% %time% >> "%LOGFILE%"
echo ============================================ >> "%LOGFILE%"
echo. >> "%LOGFILE%"
echo --- HKCR CLSID --- >> "%LOGFILE%"
reg query "HKCR\CLSID\{B8A45C32-5F6D-4E2A-9C1B-0D3E4F5A6B7C}" /s >> "%LOGFILE%" 2>&1
echo. >> "%LOGFILE%"
echo --- HKLM CTF TIP --- >> "%LOGFILE%"
reg query "HKLM\SOFTWARE\Microsoft\CTF\TIP\{B8A45C32-5F6D-4E2A-9C1B-0D3E4F5A6B7C}" /s >> "%LOGFILE%" 2>&1
echo. >> "%LOGFILE%"
echo --- HKCU CTF TIP --- >> "%LOGFILE%"
reg query "HKCU\Software\Microsoft\CTF\TIP\{B8A45C32-5F6D-4E2A-9C1B-0D3E4F5A6B7C}" /s >> "%LOGFILE%" 2>&1
echo. >> "%LOGFILE%"
echo --- HKCU Keyboard Layout Preload --- >> "%LOGFILE%"
reg query "HKCU\Keyboard Layout\Preload" >> "%LOGFILE%" 2>&1
echo. >> "%LOGFILE%"
echo --- HKCU Keyboard Layout Substitutes --- >> "%LOGFILE%"
reg query "HKCU\Keyboard Layout\Substitutes" >> "%LOGFILE%" 2>&1
echo. >> "%LOGFILE%"
echo --- HKCU CTF SortOrder --- >> "%LOGFILE%"
reg query "HKCU\Software\Microsoft\CTF\SortOrder\AssemblyItem\0x00000404" /s >> "%LOGFILE%" 2>&1
echo. >> "%LOGFILE%"
echo --- Installed icon files --- >> "%LOGFILE%"
dir "C:\Program Files\HIME\bin\icons\*.png" >> "%LOGFILE%" 2>&1
echo. >> "%LOGFILE%"
echo --- DLL files --- >> "%LOGFILE%"
dir "C:\Program Files\HIME\bin\*.dll" >> "%LOGFILE%" 2>&1
echo   Log written.

echo.
echo ============================================
echo  ALL TESTS AND DIAGNOSTICS COMPLETE
echo ============================================
echo.
echo  Next: Add HIME keyboard, switch to it, type, then check:
echo    type c:\mu\tmp\hime\test-*.log
echo    type %LOGFILE%
echo.
:: [7/7] Launch sandbox agent for MCP communication
echo [7/7] Starting sandbox agent for MCP...
start "HIME Sandbox Agent" powershell.exe -ExecutionPolicy Bypass -File "C:\HIME\sandbox-agent.ps1"
echo   Agent started in background.
echo.

echo  Press any key to open Settings for keyboard setup...
pause
start ms-settings:regionlanguage
