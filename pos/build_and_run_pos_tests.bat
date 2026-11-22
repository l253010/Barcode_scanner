@echo off
setlocal enabledelayedexpansion

cd /d "%~dp0"

echo =================================================================
echo  Building POS Integration Test Suite
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
    echo Error: MSVC cl.exe compiler not found.
    exit /b 1
)

echo Compiling pos_integration_tests.exe...
cl /nologo /EHsc /std:c++17 /W4 /utf-8 /I. /I.. /I../product_module /I../barcode ^
   test_pos_integration.cpp ProductBillingService.cpp MockDataService.cpp ^
   ../product_module/Category.cpp ../product_module/Product.cpp ../product_module/ProductManager.cpp ../product_module/Billing.cpp ^
   /Fe:pos_integration_tests.exe

if %ERRORLEVEL% neq 0 (
    echo Compilation failed with error code %ERRORLEVEL%.
    exit /b %ERRORLEVEL%
)

echo.
echo =================================================================
echo  Running POS Integration Test Suite
echo =================================================================
pos_integration_tests.exe

exit /b %ERRORLEVEL%
