# ──────────────────────────────────────────────────────────────────────────────
# Toolchain file for Linux native compilation
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/Toolchain-Linux.cmake ..
# ──────────────────────────────────────────────────────────────────────────────

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform-specific definitions
add_definitions(-DPLATFORM_LINUX)
add_definitions(-D_GNU_SOURCE)

# Compiler flags for optimization
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
