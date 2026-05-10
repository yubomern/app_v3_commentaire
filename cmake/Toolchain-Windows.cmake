# ──────────────────────────────────────────────────────────────────────────────
# Toolchain file for Windows cross-compilation (from Linux to Windows)
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/Toolchain-Windows.cmake ..
# ──────────────────────────────────────────────────────────────────────────────

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Set the cross-compiler paths for MinGW
# Adjust these based on your MinGW installation
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
set(CMAKE_AR x86_64-w64-mingw32-ar)
set(CMAKE_RANLIB x86_64-w64-mingw32-ranlib)

# Set compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64" CACHE STRING "C flags")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64" CACHE STRING "CXX flags")

# Search for programs, libraries and include files in the target environment
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform-specific definitions
add_definitions(-DPLATFORM_WINDOWS)
add_definitions(-D_WIN32_WINNT=0x0601)

# Windows specific settings
set(CMAKE_EXECUTABLE_SUFFIX ".exe")
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib")
