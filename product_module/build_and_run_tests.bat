@echo off
setlocal enabledelayedexpansion

cd /d "%~dp0"

echo =================================================================
echo  Building Product Management and Billing Module Test Suite
echo =================================================================

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
    echo Error: MSVC cl.exe compiler not found. Please run this from a Visual Studio Developer Command Prompt.
    exit /b 1
)

echo Compiling test_product_module.exe...
cl /nologo /EHsc /std:c++17 /W4 Category.cpp Product.cpp ProductManager.cpp Billing.cpp test_product_module.cpp /Fe:test_product_module.exe

if %ERRORLEVEL% neq 0 (
    echo Compilation failed with error code %ERRORLEVEL%.
    exit /b %ERRORLEVEL%
)

echo.
echo =================================================================
echo  Running Test Suite
echo =================================================================
test_product_module.exe

exit /b %ERRORLEVEL%
