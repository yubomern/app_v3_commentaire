#!/bin/bash
# ─────────────────────────────────────────────────────────────────────────────
# Build script for CoreDump Analyzer - Multi-platform cross-compilation
# ─────────────────────────────────────────────────────────────────────────────

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Functions
print_header() {
    echo -e "${BLUE}╔════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║${NC} $1 ${BLUE}║${NC}"
    echo -e "${BLUE}╚════════════════════════════════════════════════════════╝${NC}"
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

print_info() {
    echo -e "${YELLOW}ℹ${NC} $1"
}

# Main script
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
BUILD_RPI_DIR="${PROJECT_ROOT}/build-rpi"
BUILD_WIN_DIR="${PROJECT_ROOT}/build-windows"

# Default target
TARGET=${1:-"linux"}

case "$TARGET" in
    linux)
        print_header "Building for Linux x86_64 (native)"
        
        if [ -d "$BUILD_DIR" ]; then
            print_info "Cleaning previous build..."
            rm -rf "$BUILD_DIR"
        fi
        
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        
        print_info "Running CMake..."
        cmake -DCMAKE_BUILD_TYPE=Release ..
        
        print_info "Compiling..."
        make -j$(nproc)
        
        print_success "Linux build complete!"
        print_info "Binaries in: $BUILD_DIR/bin"
        ls -lh bin/
        ;;
        
    rpi|raspberry)
        print_header "Building for Raspberry Pi (ARM cross-compile)"
        
        # Check for cross-compiler
        if ! command -v arm-linux-gnueabihf-gcc &> /dev/null; then
            print_error "arm-linux-gnueabihf-gcc not found!"
            print_info "Install with: sudo apt-get install crossbuild-essential-armhf"
            exit 1
        fi
        
        if [ -d "$BUILD_RPI_DIR" ]; then
            print_info "Cleaning previous build..."
            rm -rf "$BUILD_RPI_DIR"
        fi
        
        mkdir -p "$BUILD_RPI_DIR"
        cd "$BUILD_RPI_DIR"
        
        print_info "Running CMake with Raspberry Pi toolchain..."
        cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-RaspberryPi.cmake \
              -DCMAKE_BUILD_TYPE=Release ..
        
        print_info "Compiling for ARM..."
        make -j4
        
        print_success "Raspberry Pi build complete!"
        print_info "Binaries in: $BUILD_RPI_DIR/bin"
        file bin/*
        ls -lh bin/
        ;;
        
    windows|win)
        print_header "Building for Windows x86_64 (MinGW cross-compile)"
        
        # Check for cross-compiler
        if ! command -v x86_64-w64-mingw32-gcc &> /dev/null; then
            print_error "x86_64-w64-mingw32-gcc not found!"
            print_info "Install with: sudo apt-get install mingw-w64"
            exit 1
        fi
        
        if [ -d "$BUILD_WIN_DIR" ]; then
            print_info "Cleaning previous build..."
            rm -rf "$BUILD_WIN_DIR"
        fi
        
        mkdir -p "$BUILD_WIN_DIR"
        cd "$BUILD_WIN_DIR"
        
        print_info "Running CMake with Windows toolchain..."
        cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-Windows.cmake \
              -DCMAKE_BUILD_TYPE=Release ..
        
        print_info "Compiling for Windows..."
        make -j$(nproc)
        
        print_success "Windows build complete!"
        print_info "Binaries in: $BUILD_WIN_DIR/bin"
        file bin/*.exe
        ls -lh bin/
        ;;
        
    all)
        print_header "Building for ALL platforms"
        
        print_info "Step 1/3: Linux build"
        "$0" linux
        echo ""
        
        print_info "Step 2/3: Raspberry Pi build"
        "$0" rpi
        echo ""
        
        print_info "Step 3/3: Windows build"
        "$0" windows
        echo ""
        
        print_success "All platforms built successfully!"
        echo ""
        echo "Build artifacts:"
        echo "  Linux:   $BUILD_DIR/bin"
        echo "  RPi:     $BUILD_RPI_DIR/bin"
        echo "  Windows: $BUILD_WIN_DIR/bin"
        ;;
        
    clean)
        print_header "Cleaning all builds"
        rm -rf "$BUILD_DIR" "$BUILD_RPI_DIR" "$BUILD_WIN_DIR"
        print_success "Clean complete!"
        ;;
        
    test)
        print_header "Running tests"
        if [ ! -f "$BUILD_DIR/bin/test_suite" ]; then
            print_error "test_suite not found! Run: $0 linux"
            exit 1
        fi
        
        "$BUILD_DIR/bin/test_suite"
        ;;
        
    help|--help|-h|"")
        print_header "CoreDump Analyzer - Build Script"
        echo ""
        echo "Usage: ./build.sh [target]"
        echo ""
        echo "Targets:"
        echo "  linux       Build for Linux x86_64 (default)"
        echo "  rpi         Build for Raspberry Pi (ARM cross-compile)"
        echo "  windows     Build for Windows x86_64 (MinGW cross-compile)"
        echo "  all         Build for all platforms"
        echo "  clean       Remove all build directories"
        echo "  test        Run test suite (requires 'linux' build)"
        echo "  help        Show this help message"
        echo ""
        echo "Examples:"
        echo "  ./build.sh linux               # Build for Linux"
        echo "  ./build.sh rpi                 # Build for Raspberry Pi"
        echo "  ./build.sh all                 # Build all platforms"
        echo "  ./build.sh clean && ./build.sh all  # Full rebuild"
        echo ""
        ;;
        
    *)
        print_error "Unknown target: $TARGET"
        print_info "Run '$0 help' for usage"
        exit 1
        ;;
esac

echo ""
