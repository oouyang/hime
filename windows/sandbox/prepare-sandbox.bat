@echo off
echo ============================================
echo  Prepare HIME Sandbox Test Package
echo ============================================
echo.子→阿埃

set SANDBOX_DIR=C:\Program Files\HIME\sandbox
set BUILD_DIR=\\wsl$\Debian\opt\ws\hime\windows\build\bin

:: Create sandbox directory
mkdir "%SANDBOX_DIR%" 2>nul
mkdir "%SANDBOX_DIR%\data" 2>nul

:: Copy build output
echo Copying build files...
copy /Y "%BUILD_DIR%\hime-core.dll" "%SANDBOX_DIR%\" >nul
copy /Y "%BUILD_DIR%\hime-tsf.dll" "%SANDBOX_DIR%\" >nul
copy /Y "%BUILD_DIR%\test-hime-core.exe" "%SANDBOX_DIR%\" >nul
copy /Y "%BUILD_DIR%\data\pho.tab2" "%SANDBOX_DIR%\data\" >nul
copy /Y "%BUILD_DIR%\data\*.kbm" "%SANDBOX_DIR%\data\" >nul

:: Copy deployment script and sandbox config
copy /Y "\\wsl$\Debian\opt\ws\hime\windows\sandbox\deploy-and-test.bat" "%SANDBOX_DIR%\" >nul
copy /Y "\\wsl$\Debian\opt\ws\hime\windows\sandbox\HIME-Sandbox.wsb" "%SANDBOX_DIR%\" >nul

echo.
echo Done! Files prepared in: %SANDBOX_DIR%
echo.
echo To test:
echo   1. Double-click: "%SANDBOX_DIR%\HIME-Sandbox.wsb"
echo   2. Sandbox will open and auto-run the deploy script
echo   3. Follow the on-screen instructions
echo.
pause
