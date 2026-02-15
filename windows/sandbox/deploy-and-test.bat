@echo off
chcp 65001 >nul
echo ============================================
echo  HIME Input Method - Deploy and Test Script
echo ============================================
echo.

:: Create install directory
mkdir "C:\Program Files\HIME\bin\data" 2>nul

:: Copy files from sandbox mapped folder
echo [1/4] Copying files...
copy /Y "%~dp0hime-core.dll" "C:\Program Files\HIME\bin\" >nul
copy /Y "%~dp0hime-tsf.dll" "C:\Program Files\HIME\bin\" >nul
copy /Y "%~dp0hime-install.exe" "C:\Program Files\HIME\bin\" >nul
copy /Y "%~dp0hime-uninstall.exe" "C:\Program Files\HIME\bin\" >nul
copy /Y "%~dp0test-hime-core.exe" "C:\Program Files\HIME\bin\" >nul
copy /Y "%~dp0data\pho.tab2" "C:\Program Files\HIME\bin\data\" >nul
copy /Y "%~dp0data\*.kbm" "C:\Program Files\HIME\bin\data\" >nul
copy /Y "%~dp0data\*.gtab" "C:\Program Files\HIME\bin\data\" >nul

:: Run engine tests first
echo.
echo [2/4] Running engine tests...
echo.
cd /d "C:\Program Files\HIME\bin"
test-hime-core.exe
if %errorlevel% neq 0 (
    echo.
    echo *** ENGINE TESTS FAILED - not registering IME ***
    pause
    exit /b 1
)

:: Register the IME
echo.
echo [3/4] Registering HIME IME...
regsvr32 /s "C:\Program Files\HIME\bin\hime-tsf.dll"
if %errorlevel% equ 0 (
    echo Registration successful!
) else (
    echo Registration FAILED. Try running as Administrator.
    pause
    exit /b 1
)

:: Instructions
echo.
echo [4/4] Setup complete!
echo.
echo ============================================
echo  Next steps:
echo  1. Open Settings ^> Time ^& Language ^> Language ^& Region
echo  2. Click "Chinese (Traditional, Taiwan)"
echo     (Add it first if not present: Add a language ^> Chinese Traditional)
echo  3. Click Language options ^> Add a keyboard
echo  4. Select "HIME" (look for [DEBUG-0213] tag)
echo  5. Open Notepad
echo  6. Press Win+Space to switch to HIME
echo  7. Type: a 8 Space 1  (should produce a Chinese character)
echo  8. Type: . Space 1    (should produce another character)
echo  9. Check order: characters should be left-to-right
echo ============================================
echo.
echo Log files will be in: c:\mu\tmp\hime\test-*.log
echo.
pause
