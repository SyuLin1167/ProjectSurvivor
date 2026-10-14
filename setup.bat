@echo off
chcp 65001 >nul
mode con: cols=80 lines=30

title Setup

powershell -Command ^
  "$lines = @(" ^
  "'███████╗███████╗████████╗██╗   ██╗██████╗ '," ^
  "'██╔════╝██╔════╝╚══██╔══╝██║   ██║██╔══██╗'," ^
  "'███████╗█████╗     ██║   ██║   ██║██████╔╝'," ^
  "'╚════██║██╔══╝     ██║   ██║   ██║██╔═══╝ '," ^
  "'███████║███████╗   ██║   ╚██████╔╝██║     '," ^
  "'╚══════╝╚══════╝   ╚═╝    ╚═════╝ ╚═╝     '" ^
  "); foreach ($line in $lines) { Write-Host $line -ForegroundColor Cyan }"

REM ---- 変数の設定 ----
set "PROJECT_NAME=ProjectName"
set "DOWNLOAD_URL=https://dxlib.xsrv.jp/DxLib/DxLib_VC3_24f.zip"
set "DXLIB_ZIP=DxLib_VC.zip"
set "TEMP_DIR=dxlib_tmp"

powershell -Command ^
  "$lines = @(" ^
  "'================================='," ^
  "'=          Dxlib Setup          ='," ^
  "'================================='" ^
  "); foreach ($line in $lines) { Write-Host $line -ForegroundColor DarkMagenta }"

REM ---- DxLibのダウンロード ----
powershell -Command "Write-Host '[ ]Download Dxlib...' -ForegroundColor DarkCyan"
curl -L --progress-bar "%DOWNLOAD_URL%" -o "%DXLIB_ZIP%"
if errorlevel 1 (
    powershell -Command "Write-Host 'Failed to Download' -ForegroundColor Red"
    pause
    exit /b 1
)
powershell -Command "Write-Host '[✔] Download Complete!' -ForegroundColor Green"

REM ---- 一時ディレクトリの作成 ----
if exist "%TEMP_DIR%" rmdir /s /q "%TEMP_DIR%"
mkdir "%TEMP_DIR%"

REM ---- ダウンロードしたZIPを解凍 ----
powershell -Command "Write-Host '[ ]Extract DxLib.zip file...' -ForegroundColor DarkCyan"
powershell -Command "Expand-Archive -Path '%DXLIB_ZIP%' -DestinationPath '%TEMP_DIR%'"
if errorlevel 1 (
    powershell -Command "Write-Host 'Failed to Extract' -ForegroundColor Red"
    pause
    exit /b 1
)
powershell -Command "Write-Host '[✔] Extract Complete!' -ForegroundColor Green"

REM ---- 必要なディレクトリのリネーム ----
powershell -Command "Write-Host '[ ]Rename Directory' -ForegroundColor DarkCyan"
set "SRC_DIR=%TEMP_DIR%\DxLib_VC\プロジェクトに追加すべきファイル_VC用"
set "DST_DIR=%TEMP_DIR%\DxLib_VC\DxLib"
if not exist "%SRC_DIR%" (
    powershell -Command "Write-Host 'not found : "%SRC_DIR%"' -ForegroundColor Red"
    pause
    exit /b 1
)
ren "%SRC_DIR%" DxLib
powershell -Command "Write-Host '[✔] Rename Complete!' -ForegroundColor Green"

REM ---- Dxlibディレクトリの移動 ----
powershell -Command "Write-Host '[ ]Move DxLib to include Directory' -ForegroundColor DarkCyan"
move "%TEMP_DIR%\DxLib_VC\DxLib" "%PROJECT_NAME%\include"
powershell -Command "Write-Host '[✔] Move Complete!' -ForegroundColor Green"

REM ---- 残りの一時ファイル・ZIPを削除 ----
powershell -Command "Write-Host '======= CleneUp =======' -ForegroundColor DarkGray"
rmdir /s /q "%TEMP_DIR%"
del "%DXLIB_ZIP%"
powershell -Command "Write-Host '=======  Done  =======' -ForegroundColor Green"

powershell -Command ^
  "$lines = @(" ^
  "'---------------------------------'," ^
  "'    Dxlib Setup is Complete!!    '," ^
  "'---------------------------------'" ^
  "); foreach ($line in $lines) { Write-Host $line -ForegroundColor DarkYellow }"

powershell -Command ^
  "$lines = @(" ^
  "'================================='," ^
  "'=      nlohman-json Setup       ='," ^
  "'================================='" ^
  "); foreach ($line in $lines) { Write-Host $line -ForegroundColor DarkMagenta }"

REM nlohmann-jsonをダウンロード
powershell -Command "Write-Host '[ ]Download nlohmann-json...' -ForegroundColor DarkCyan"
if exist "%PROJECT_NAME%/include/nlohmann" rmdir /s /q "%PROJECT_NAME%/include/nlohmann"
mkdir "%PROJECT_NAME%/include/nlohmann"
curl -L -o "%PROJECT_NAME%/include/nlohmann/json.hpp" ^
  "https://github.com/nlohmann/json/raw/refs/heads/develop/single_include/nlohmann/json.hpp"

curl -L -o "%PROJECT_NAME%/include/nlohmann/json_fwd.hpp" ^
  "https://github.com/nlohmann/json/raw/refs/heads/develop/single_include/nlohmann/json_fwd.hpp"

REM ダウンロード失敗時はエラー表示
if errorlevel 1 (
    powershell -Command "Write-Host 'failed to download nlohmann-json' -ForegroundColor Red"
    pause
    exit /b 1
)
powershell -Command "Write-Host '[✔] Download Complete!' -ForegroundColor Green"

rem 完了報告
powershell -Command ^
  "$lines = @(" ^
  "'---------------------------------'," ^
  "' nlohman-json Setup is Complete! '," ^
  "'---------------------------------'" ^
  "); foreach ($line in $lines) { Write-Host $line -ForegroundColor DarkYellow }"

powershell -Command ^
  "$lines = @(" ^
  "'======== INSTALL COMPLETE ========'," ^
  "'        Let''s develop it!         '" ^
  "); foreach ($line in $lines) { Write-Host $line -ForegroundColor Black -BackgroundColor DarkGreen }"
pause

rem slnを開く
for %%f in ("%PROJECT_NAME%\*.sln") do (
    start "" "%%f"
    goto :done
)
:done
