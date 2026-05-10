# Build Toolchain Documentation

## Overview

This project supports compilation on multiple platforms:
- **Linux x86_64** (native)
- **Raspberry Pi** (ARM cross-compilation)
- **Windows x86_64** (cross-compilation from Linux or native)

---

## 1. Build Configuration

### 1.1 Standard Linux Build (x86_64)

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

**Output binaries:**
- `bin/crash_simulator`
- `bin/coredump_handler`
- `bin/crash_analyzer`
- `bin/test_suite`

---

## 2. Raspberry Pi Cross-Compilation (ARM)

### 2.1 Prerequisites

Install ARM cross-compilation tools:

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    crossbuild-essential-armhf \
    arm-linux-gnueabihf-gcc \
    arm-linux-gnueabihf-g++ \
    arm-linux-gnueabihf-gfortran \
    libc6-armhf-cross \
    libc6-dev-armhf-cross
```

**Fedora/RHEL:**
```bash
sudo dnf install -y \
    gcc-c++ \
    arm-linux-gnueabihf-gcc \
    arm-linux-gnueabihf-gcc-c++ \
    arm-linux-gnueabihf-binutils
```

### 2.2 Build for Raspberry Pi

```bash
mkdir build-rpi
cd build-rpi
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-RaspberryPi.cmake ..
make -j4
```

**Output binaries** (ARM compiled):
- `bin/crash_simulator`
- `bin/coredump_handler`
- `bin/crash_analyzer`
- `bin/test_suite`

### 2.3 Transfer to Raspberry Pi

```bash
# From your build machine
scp build-rpi/bin/* pi@raspberrypi.local:~/crash_analyzer/bin/

# Or use rsync
rsync -avz build-rpi/bin/ pi@raspberrypi.local:~/crash_analyzer/bin/
```

### 2.4 Run on Raspberry Pi

```bash
ssh pi@raspberrypi.local
cd ~/crash_analyzer

# Run the simulator
./bin/crash_simulator

# Run the analyzer
./bin/crash_analyzer

# Run the handler
sudo ./bin/coredump_handler
```

---

## 3. Windows Cross-Compilation

### 3.1 Prerequisites (Linux → Windows)

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install -y \
    mingw-w64 \
    mingw-w64-x86-64-dev \
    mingw-w64-tools
```

**Fedora/RHEL:**
```bash
sudo dnf install -y \
    mingw64-gcc \
    mingw64-gcc-c++ \
    mingw64-binutils
```

### 3.2 Build for Windows

```bash
mkdir build-windows
cd build-windows
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-Windows.cmake ..
make -j$(nproc)
```

**Output binaries** (Windows .exe):
- `bin/crash_simulator.exe`
- `bin/coredump_handler.exe`
- `bin/crash_analyzer.exe`
- `bin/test_suite.exe`

### 3.3 Transfer to Windows

```bash
# Copy to USB or network drive
cp build-windows/bin/*.exe /path/to/transfer/

# Or use scp if Windows has SSH
scp build-windows/bin/*.exe user@windows-machine:/path/to/bin/
```

### 3.4 Run on Windows

```cmd
REM Command Prompt
cd C:\path\to\analyzer\bin
crash_simulator.exe
crash_analyzer.exe
coredump_handler.exe
```

---

## 4. Native Windows Build (Windows only)

### 4.1 Prerequisites

- Visual Studio 2019+ with C++ tools, or
- MinGW with GCC 9+, CMake 3.14+

### 4.2 Build with MSVC

```cmd
mkdir build
cd build
cmake -G "Visual Studio 16 2019" -A x64 ..
cmake --build . --config Release
```

### 4.3 Build with MinGW

```cmd
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
cmake --build .
```

---

## 5. Build Options

### 5.1 Debug vs Release Build

```bash
# Debug build (with symbols)
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build (optimized)
cmake -DCMAKE_BUILD_TYPE=Release ..
```

### 5.2 Custom Compiler Flags

```bash
cmake -DCMAKE_CXX_FLAGS="-O3 -march=native" ..
```

### 5.3 Verbose Build

```bash
make VERBOSE=1
```

---

## 6. Testing

### 6.1 Run Unit Tests

```bash
cd build
./bin/test_suite
```

### 6.2 Run Full Pipeline

```bash
# Simulate crashes
./bin/crash_simulator

# Analyze results
./bin/crash_analyzer

# View dashboard
python3 ../dashboard/dashboard.py
```

---

## 7. Troubleshooting

### 7.1 Raspberry Pi Compilation Issues

**Problem:** `arm-linux-gnueabihf-gcc: not found`
```bash
# Solution: Install cross-compiler
sudo apt-get install arm-linux-gnueabihf-gcc
```

**Problem:** SQLite3 library not found
```bash
# Solution: Install SQLite3 for ARM
sudo apt-get install libsqlite3-dev:armhf
```

### 7.2 Windows Compilation Issues

**Problem:** `x86_64-w64-mingw32-gcc: not found`
```bash
# Solution: Install MinGW
sudo apt-get install mingw-w64
```

**Problem:** DbgHelp.h not found
```bash
# Solution: Already handled via CMAKE_FIND_ROOT_PATH
# Ensure MinGW Windows libraries are installed
```

### 7.3 General Issues

**Clean rebuild:**
```bash
rm -rf build
mkdir build
cd build
cmake ..
make clean
make
```

**Check CMake version:**
```bash
cmake --version
# Must be >= 3.14
```

---

## 8. Toolchain Files Reference

### 8.1 Toolchain-RaspberryPi.cmake

Located in: `cmake/Toolchain-RaspberryPi.cmake`

**Compiler:** `arm-linux-gnueabihf-gcc/g++`
**Target:** ARM 32-bit (ARMv7)
**Flags:** NEON FPU optimization, hard-float ABI

### 8.2 Toolchain-Windows.cmake

Located in: `cmake/Toolchain-Windows.cmake`

**Compiler:** `x86_64-w64-mingw32-gcc/g++`
**Target:** Windows x86_64
**Flags:** 64-bit architecture

### 8.3 Toolchain-Linux.cmake

Located in: `cmake/Toolchain-Linux.cmake`

**Compiler:** System default (gcc/g++)
**Target:** Linux x86_64
**Flags:** Native optimization

---

## 9. Platform Defines

Compilation automatically sets platform-specific defines:

| Platform | Define | Location |
|----------|--------|----------|
| Linux | `PLATFORM_LINUX` | Automatic |
| Raspberry Pi | `PLATFORM_LINUX`, `PLATFORM_RASPBERRY_PI` | Toolchain |
| Windows | `PLATFORM_WINDOWS` | Toolchain/CMakeLists |

Use in code:
```cpp
#ifdef PLATFORM_RASPBERRY_PI
    // Raspberry Pi specific code
#endif

#ifdef PLATFORM_WINDOWS
    // Windows specific code
#endif
```

---

## 10. Performance Notes

### 10.1 Raspberry Pi Performance

- **Single-threaded:** ~0.5-1s per crash analysis
- **Multi-threaded:** ~0.2-0.5s per crash analysis (4 cores)
- **Memory:** ~50-100MB typical

**Optimization:** Use `-O2` or `-O3` flags for Release builds

### 10.2 Windows Performance

- **Single-threaded:** ~0.1-0.2s per crash analysis
- **Multi-threaded:** ~0.05-0.1s per crash analysis

---

## 11. CI/CD Integration

### 11.1 GitHub Actions Example

```yaml
name: Cross-Platform Build

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install tools
        run: |
          sudo apt-get update
          sudo apt-get install -y crossbuild-essential-armhf mingw-w64
      - name: Build Linux
        run: |
          mkdir build && cd build
          cmake .. && make
      - name: Build Raspberry Pi
        run: |
          mkdir build-rpi && cd build-rpi
          cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-RaspberryPi.cmake .. && make
      - name: Build Windows
        run: |
          mkdir build-windows && cd build-windows
          cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-Windows.cmake .. && make
```

---

## 12. Support & Resources

- **CMake Documentation:** https://cmake.org/documentation/
- **ARM GCC Docs:** https://gcc.gnu.org/
- **MinGW:** http://www.mingw.org/
- **Raspberry Pi Dev:** https://www.raspberrypi.org/
