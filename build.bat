@echo off
REM ────────────────────────────────────────────────────────────────────────────
REM Build script for CoreDump Analyzer - Windows native build
REM ────────────────────────────────────────────────────────────────────────────

setlocal enabledelayedexpansion

set PROJECT_ROOT=%~dp0
set BUILD_DIR=%PROJECT_ROOT%build
set TARGET=%1

if "%TARGET%"=="" (
    set TARGET=release
)

if /i "%TARGET%"=="release" (
    echo.
    echo ╔══════════════════════════════════════════════════════════╗
    echo ║   Building for Windows x86_64 (Release)                  ║
    echo ╚══════════════════════════════════════════════════════════╝
    echo.
    
    if exist "%BUILD_DIR%" (
        echo Cleaning previous build...
        rmdir /s /q "%BUILD_DIR%"
    )
    
    mkdir "%BUILD_DIR%"
    cd /d "%BUILD_DIR%"
    
    echo Running CMake...
    cmake -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Release ..
    
    echo Compiling...
    cmake --build . --config Release --parallel
    
    echo.
    echo ✓ Windows Release build complete!
    echo.
    echo Binaries in: %BUILD_DIR%\bin\Release\
    dir "%BUILD_DIR%\bin\Release\"
    
) else if /i "%TARGET%"=="debug" (
    echo.
    echo ╔══════════════════════════════════════════════════════════╗
    echo ║   Building for Windows x86_64 (Debug)                    ║
    echo ╚══════════════════════════════════════════════════════════╝
    echo.
    
    if exist "%BUILD_DIR%" (
        echo Cleaning previous build...
        rmdir /s /q "%BUILD_DIR%"
    )
    
    mkdir "%BUILD_DIR%"
    cd /d "%BUILD_DIR%"
    
    echo Running CMake...
    cmake -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Debug ..
    
    echo Compiling...
    cmake --build . --config Debug --parallel
    
    echo.
    echo ✓ Windows Debug build complete!
    echo.
    echo Binaries in: %BUILD_DIR%\bin\Debug\
    dir "%BUILD_DIR%\bin\Debug\"
    
) else if /i "%TARGET%"=="clean" (
    echo Cleaning all builds...
    if exist "%BUILD_DIR%" (
        rmdir /s /q "%BUILD_DIR%"
    )
    echo ✓ Clean complete!
    
) else if /i "%TARGET%"=="test" (
    echo Running tests...
    if exist "%BUILD_DIR%\bin\Release\test_suite.exe" (
        "%BUILD_DIR%\bin\Release\test_suite.exe"
    ) else if exist "%BUILD_DIR%\bin\Debug\test_suite.exe" (
        "%BUILD_DIR%\bin\Debug\test_suite.exe"
    ) else (
        echo Error: test_suite.exe not found!
        echo Run: build.bat release
        exit /b 1
    )
    
) else (
    echo.
    echo Usage: build.bat [target]
    echo.
    echo Targets:
    echo   release     Build Release configuration (default^)
    echo   debug       Build Debug configuration
    echo   clean       Remove build directory
    echo   test        Run test suite
    echo   help        Show this help
    echo.
    echo Examples:
    echo   build.bat release          # Release build
    echo   build.bat debug            # Debug build
    echo   build.bat clean            # Clean build
    echo.
)

endlocal
