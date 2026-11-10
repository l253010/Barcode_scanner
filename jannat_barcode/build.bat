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
    del /q /f BarcodeApp.exe test_validation.exe barcode_tests.exe *.obj *.ilk *.pdb 2>nul
    echo Done.
    exit /b 0
)

if /i "%TARGET%"=="test" (
    echo Building test binary...
    cl /nologo /EHsc /std:c++17 /W4 test_validation.cpp BarcodeGenerator.cpp /Fe:test_validation.exe
    if %ERRORLEVEL% equ 0 (
        echo Running tests...
        test_validation.exe
    )
    exit /b %ERRORLEVEL%
)

if /i "%TARGET%"=="app" (
    echo Building BarcodeApp.exe...
    cl /nologo /EHsc /std:c++17 /W4 main.cpp BarcodeGenerator.cpp /Fe:BarcodeApp.exe
    exit /b %ERRORLEVEL%
)

if /i "%TARGET%"=="run" (
    echo Building and launching BarcodeApp.exe...
    cl /nologo /EHsc /std:c++17 /W4 main.cpp BarcodeGenerator.cpp /Fe:BarcodeApp.exe
    if %ERRORLEVEL% equ 0 (
        start "" BarcodeApp.exe
    )
    exit /b %ERRORLEVEL%
)

if /i "%TARGET%"=="all" (
    echo Building BarcodeApp.exe...
    cl /nologo /EHsc /std:c++17 /W4 main.cpp BarcodeGenerator.cpp /Fe:BarcodeApp.exe
    if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

    echo Building test_validation.exe...
    cl /nologo /EHsc /std:c++17 /W4 test_validation.cpp BarcodeGenerator.cpp /Fe:test_validation.exe
    if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

    echo Build successful.
    exit /b 0
)

echo Unknown target: %TARGET%
echo Usage: build.bat [all ^| app ^| test ^| run ^| clean]
exit /b 1
