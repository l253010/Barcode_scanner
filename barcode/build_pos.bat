@echo off
setlocal enabledelayedexpansion

cd /d "%~dp0"

echo ======================================================
echo       Windows POS System - Build and Test Pipeline     
echo ======================================================

where cl.exe >nul 2>&1
if %ERRORLEVEL% neq 0 (
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if exist "!VSWHERE!" (
        for /f "usebackq tokens=*" %%i in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
            set "VS_PATH=%%i"
        )
    )
    if defined VS_PATH (
        if exist "!VS_PATH!\VC\Auxiliary\Build\vcvars64.bat" (
            call "!VS_PATH!\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
        )
    )
)

where cl.exe >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] cl.exe compiler not found. Please run from a Visual Studio Developer Command Prompt.
    exit /b 1
)

set "TARGET=%~1"
if /i "%TARGET%"=="clean" (
    echo Cleaning build artifacts...
    del /q /f POS.exe test_camera_scanner.exe *.obj *.ilk *.pdb 2>nul
    echo Clean completed.
    exit /b 0
)

set "CV_DIR="
if defined OPENCV_DIR (
    if exist "!OPENCV_DIR!\include" set "CV_DIR=!OPENCV_DIR!"
)

if not defined CV_DIR (
    if exist "C:\Users\AR COMPUTER\Downloads\opencv\build\include" (
        set "CV_DIR=C:\Users\AR COMPUTER\Downloads\opencv\build"
    ) else if exist "C:\opencv\build\include" (
        set "CV_DIR=C:\opencv\build"
    ) else if exist "D:\opencv\build\include" (
        set "CV_DIR=D:\opencv\build"
    )
)

if not defined CV_DIR (
    echo [ERROR] OpenCV directory not found.
    echo Please set OPENCV_DIR environment variable to your OpenCV build folder.
    exit /b 1
)

set "CV_INC=!CV_DIR!\include"
set "CV_LIB_DIR="
set "CV_BIN_DIR="

if exist "!CV_DIR!\x64\vc16\lib" (
    set "CV_LIB_DIR=!CV_DIR!\x64\vc16\lib"
    set "CV_BIN_DIR=!CV_DIR!\x64\vc16\bin"
) else if exist "!CV_DIR!\x64\vc15\lib" (
    set "CV_LIB_DIR=!CV_DIR!\x64\vc15\lib"
    set "CV_BIN_DIR=!CV_DIR!\x64\vc15\bin"
) else if exist "!CV_DIR!\lib" (
    set "CV_LIB_DIR=!CV_DIR!\lib"
    set "CV_BIN_DIR=!CV_DIR!\bin"
)

if not defined CV_LIB_DIR (
    echo [ERROR] OpenCV library directory not found under "!CV_DIR!".
    exit /b 1
)

set "CV_LIB="
if exist "!CV_LIB_DIR!\opencv_world4140.lib" (
    set "CV_LIB=opencv_world4140.lib"
) else (
    for %%f in ("!CV_LIB_DIR!\opencv_world*.lib") do (
        set "CV_LIB=%%~nxf"
    )
)

if not defined CV_LIB (
    echo [ERROR] Could not find opencv_world library in "!CV_LIB_DIR!".
    exit /b 1
)

echo [CONFIG] Compiler       : MSVC C++17 (/W4 /EHsc)
echo [CONFIG] OpenCV Include : "!CV_INC!"
echo [CONFIG] OpenCV Lib Dir : "!CV_LIB_DIR!"
echo [CONFIG] OpenCV Library : "!CV_LIB!"
echo.

echo ------------------------------------------------------
echo [PHASE 1] Building Offline Tests (test_camera_scanner)
echo ------------------------------------------------------
cl /nologo /EHsc /std:c++17 /W4 test_camera_scanner.cpp BarcodeDecoder.cpp BarcodeGenerator.cpp /Fe:test_camera_scanner.exe
if %ERRORLEVEL% neq 0 (
    echo.
    echo [BUILD ERROR] Failed to compile offline tests.
    exit /b %ERRORLEVEL%
)

echo.
echo ------------------------------------------------------
echo [PHASE 2] Executing Offline Tests
echo ------------------------------------------------------
test_camera_scanner.exe
if %ERRORLEVEL% neq 0 (
    echo.
    echo [TEST FAILED] Offline tests failed! Halting build. POS.exe will NOT be built.
    exit /b %ERRORLEVEL%
)

if /i "%TARGET%"=="test" (
    echo.
    echo [SUCCESS] Offline test run complete.
    exit /b 0
)

echo.
echo ------------------------------------------------------
echo [PHASE 3] Offline Tests PASSED! Building POS.exe
echo ------------------------------------------------------
cl /nologo /EHsc /std:c++17 /W4 /I"!CV_INC!" pos_main.cpp camera_scanner.cpp BarcodeDecoder.cpp BarcodeGenerator.cpp /Fe:POS.exe /link /LIBPATH:"!CV_LIB_DIR!" !CV_LIB!
if %ERRORLEVEL% neq 0 (
    echo.
    echo [BUILD ERROR] Failed to compile POS.exe
    exit /b %ERRORLEVEL%
)

if defined CV_BIN_DIR (
    copy /y "!CV_BIN_DIR!\opencv_world*.dll" "%~dp0" >nul 2>&1
)

echo.
echo ======================================================
echo [BUILD SUCCESS] All tests passed and POS.exe is ready!
echo ======================================================

if /i "%TARGET%"=="run" (
    echo Launching POS.exe...
    POS.exe
)

exit /b 0
