# Quick Build Guide

## One-Line Builds

### Linux x86_64 (native)
```bash
mkdir build && cd build && cmake .. && make -j$(nproc) && cd ..
```

### Raspberry Pi (ARM cross-compile)
```bash
mkdir build-rpi && cd build-rpi && cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-RaspberryPi.cmake .. && make -j4 && cd ..
```

### Windows (MinGW cross-compile)
```bash
mkdir build-windows && cd build-windows && cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-Windows.cmake .. && make -j$(nproc) && cd ..
```

---

## Directory Structure

```
project/
├── cmake/                          # Toolchain files
│   ├── Toolchain-Linux.cmake      # Linux x86_64
│   ├── Toolchain-RaspberryPi.cmake # ARM cross-compile
│   └── Toolchain-Windows.cmake    # Windows cross-compile
├── CMakeLists.txt                 # Main build configuration
├── TOOLCHAIN.md                   # Detailed toolchain documentation
├── BUILD.md                        # This file
└── [source directories]
```

---

## Output Locations

### Debug Build
```
build/bin/
  ├── crash_simulator
  ├── coredump_handler
  ├── crash_analyzer
  └── test_suite
```

### Release Build
```
build/bin/
  ├── crash_simulator          (optimized, ~2MB)
  ├── coredump_handler        (optimized, ~1MB)
  ├── crash_analyzer          (optimized, ~3MB)
  └── test_suite             (optimized, ~1.5MB)
```

---

## Platform-Specific Details

| Platform | Toolchain | Compiler | Output | Status |
|----------|-----------|----------|--------|--------|
| Linux x86_64 | Toolchain-Linux.cmake | gcc/g++ | .o/.elf | Native ✓ |
| Raspberry Pi | Toolchain-RaspberryPi.cmake | arm-linux-gnueabihf-gcc | .o/.elf (ARM) | Cross ✓ |
| Windows x86_64 | Toolchain-Windows.cmake | x86_64-w64-mingw32-gcc | .exe | Cross ✓ |

---

## Clean Build

```bash
rm -rf build build-rpi build-windows
mkdir build && cd build && cmake .. && make
```

---

## Install

```bash
cd build
sudo cmake --install . --prefix /usr/local
```

Binaries will be installed to `/usr/local/bin/`

---

## Run Tests

```bash
cd build
./bin/test_suite
```

---

## Platform Detection

The build system automatically detects:
- `PLATFORM_LINUX` on Linux
- `PLATFORM_RASPBERRY_PI` on Raspberry Pi builds
- `PLATFORM_WINDOWS` on Windows

Use in C++ code:
```cpp
#ifdef PLATFORM_RASPBERRY_PI
    // RPi specific optimizations
#endif
```
