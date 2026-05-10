# ──────────────────────────────────────────────────────────────────────────────
# Toolchain file for Raspberry Pi cross-compilation
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/Toolchain-RaspberryPi.cmake ..
# ──────────────────────────────────────────────────────────────────────────────

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Set the cross-compiler paths
# Adjust these paths based on your arm-linux-gnueabihf installation
set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)
set(CMAKE_AR arm-linux-gnueabihf-ar)
set(CMAKE_RANLIB arm-linux-gnueabihf-ranlib)

# Set compiler flags for Raspberry Pi
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=neon -mfloat-abi=hard -march=armv7-a" CACHE STRING "C flags")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=neon -mfloat-abi=hard -march=armv7-a" CACHE STRING "CXX flags")

# Search for programs, libraries and include files in the target environment
set(CMAKE_FIND_ROOT_PATH /usr/arm-linux-gnueabihf)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform-specific definitions
add_definitions(-DPLATFORM_LINUX)
add_definitions(-DPLATFORM_RASPBERRY_PI)

# Raspberry Pi specific optimizations
add_definitions(-D_GNU_SOURCE)
