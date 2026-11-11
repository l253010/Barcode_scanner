@echo off
setlocal enabledelayedexpansion

cd /d "%~dp0"

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
    echo Error: cl.exe not found. Please run this from a Visual Studio Developer Command Prompt.
    exit /b 1
)

set "TARGET=%~1"
if "%TARGET%"=="" set "TARGET=all"

if /i "%TARGET%"=="clean" (
    echo Cleaning build artifacts...
    del /q /f BarcodeApp.exe BarcodeScannerApp.exe test_validation.exe test_decoder.exe barcode_tests.exe *.obj *.ilk *.pdb 2>nul
    echo Done.
    exit /b 0
)

if /i "%TARGET%"=="test" (
    echo Building generator validation tests...
    cl /nologo /EHsc /std:c++17 /W4 test_validation.cpp BarcodeGenerator.cpp /Fe:test_validation.exe
    if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

    echo Building decoder tests...
    cl /nologo /EHsc /std:c++17 /W4 test_decoder.cpp BarcodeDecoder.cpp BarcodeGenerator.cpp /Fe:test_decoder.exe
    if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

    if %ERRORLEVEL% equ 0 (
        echo Running generator validation tests...
        test_validation.exe
        if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
        echo Running decoder tests...
        test_decoder.exe
    )
    exit /b %ERRORLEVEL%
)

if /i "%TARGET%"=="app" (
    echo Building BarcodeScannerApp.exe...
    cl /nologo /EHsc /std:c++17 /W4 scanner_app.cpp BarcodeDecoder.cpp /Fe:BarcodeScannerApp.exe
    exit /b %ERRORLEVEL%
)

if /i "%TARGET%"=="generator" (
    echo Building BarcodeApp.exe...
    cl /nologo /EHsc /std:c++17 /W4 main.cpp BarcodeGenerator.cpp /Fe:BarcodeApp.exe
    exit /b %ERRORLEVEL%
)

if /i "%TARGET%"=="run" (
    echo Building and launching BarcodeScannerApp.exe...
    cl /nologo /EHsc /std:c++17 /W4 scanner_app.cpp BarcodeDecoder.cpp /Fe:BarcodeScannerApp.exe
    if %ERRORLEVEL% equ 0 (
        start "" BarcodeScannerApp.exe
    )
    exit /b %ERRORLEVEL%
)

if /i "%TARGET%"=="run-generator" (
    echo Building and launching BarcodeApp.exe...
    cl /nologo /EHsc /std:c++17 /W4 main.cpp BarcodeGenerator.cpp /Fe:BarcodeApp.exe
    if %ERRORLEVEL% equ 0 (
        start "" BarcodeApp.exe
    )
    exit /b %ERRORLEVEL%
)

if /i "%TARGET%"=="all" (
    echo Building BarcodeScannerApp.exe...
    cl /nologo /EHsc /std:c++17 /W4 scanner_app.cpp BarcodeDecoder.cpp /Fe:BarcodeScannerApp.exe
    if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

    echo Building BarcodeApp.exe...
    cl /nologo /EHsc /std:c++17 /W4 main.cpp BarcodeGenerator.cpp /Fe:BarcodeApp.exe
    if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

    echo Building test_validation.exe...
    cl /nologo /EHsc /std:c++17 /W4 test_validation.cpp BarcodeGenerator.cpp /Fe:test_validation.exe
    if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

    echo Building test_decoder.exe...
    cl /nologo /EHsc /std:c++17 /W4 test_decoder.cpp BarcodeDecoder.cpp BarcodeGenerator.cpp /Fe:test_decoder.exe
    if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

    echo Build successful.
    exit /b 0
)

echo Unknown target: %TARGET%
echo Usage: build.bat [all ^| app ^| run ^| generator ^| run-generator ^| test ^| clean]
exit /b 1
