@echo off
setlocal enabledelayedexpansion

echo ==========================================
echo Step 0: Setup Visual Studio Environment
echo ==========================================

:: msbuildコマンドがすでに使えるかチェック
where msbuild >nul 2>nul
if %errorlevel% equ 0 (
    echo [INFO] MSBuild is already available.
    goto START
)

:: 使えない場合、vswhereツールを使ってVSのインストール先を探す
echo [INFO] MSBuild not found. Searching for Visual Studio installation...
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist "%VSWHERE%" (
    echo [ERROR] 'vswhere.exe' not found. Is Visual Studio installed properly?
    pause
    exit /b 1
)

:: 最新のVisual Studioのパスを取得
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
    set "VS_PATH=%%i"
)

if not defined VS_PATH (
    echo [ERROR] Visual Studio installation not found.
    pause
    exit /b 1
)

:: 環境変数をロードする (VsDevCmd.bat)
echo [INFO] Found VS at: !VS_PATH!
call "!VS_PATH!\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul

:: 再度チェック
where msbuild >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] Failed to setup Visual Studio environment.
    pause
    exit /b 1
)

:START
echo.
echo ==========================================
echo Step 1: Submodule Update
echo ==========================================
git submodule update --init --recursive

echo.
echo ==========================================
echo Step 2 & 3: Build Assimp (CMake)
echo ==========================================
pushd external\assimp

if not exist build mkdir build
cd build

echo [Configuring Assimp...]
cmake ..
if %errorlevel% neq 0 goto ERROR

echo [Building Assimp Debug...]
cmake --build . --config Debug
if %errorlevel% neq 0 goto ERROR

echo [Building Assimp Release...]
cmake --build . --config Release
if %errorlevel% neq 0 goto ERROR

:: --- config.h のコピー ---
echo [Copying config.h...]
copy /Y "include\assimp\config.h" "..\include\assimp\"
if %errorlevel% neq 0 (
    echo [WARNING] Failed to copy config.h.
) else (
    echo [SUCCESS] config.h copied to external\assimp\include\assimp
)

popd

echo.
echo ==========================================
echo Step 4: Build DirectXTex (MSBuild .sln)
echo ==========================================
pushd external\DirectXTex

echo [Building DirectXTex Debug...]
msbuild DirectXTex_Desktop_2022_Win10.sln /t:Build /p:Configuration=Debug /p:Platform=x64 /m
if %errorlevel% neq 0 goto ERROR

echo [Building DirectXTex Release...]
msbuild DirectXTex_Desktop_2022_Win10.sln /t:Build /p:Configuration=Release /p:Platform=x64 /m
if %errorlevel% neq 0 goto ERROR

popd

echo.
echo ==========================================
echo SUCCESS: All builds completed.
echo ==========================================
pause
exit /b 0

:ERROR
echo.
echo ==========================================
echo [ERROR] Build failed. Check the messages above.
echo ==========================================
pause
exit /b 1