cmake_minimum_required(VERSION 3.14)
project(CoreDumpPFE VERSION 3.0 LANGUAGES CXX)

# ─── Build Type Detection ──────────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")

# ─── C++ Standard Configuration ────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ─── Compiler-specific flags ──────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS _WINSOCK_DEPRECATED_NO_WARNINGS NOMINMAX)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /Od")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
endif()

# ─── Output directories ───────────────────────────────────────────────────────
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ─── 1. Crash Simulator (independent application) ────────────────────────────
set(SIMULATOR_SOURCES
    simulator/src/simulator_main.cpp
    simulator/src/Simulator.cpp
    simulator/src/Randomizer.cpp
)

add_executable(crash_simulator ${SIMULATOR_SOURCES})

target_include_directories(crash_simulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/simulator/include
)

if(WIN32)
    target_compile_definitions(crash_simulator PRIVATE PLATFORM_WINDOWS)
    target_link_libraries(crash_simulator PRIVATE DbgHelp)
else()
    target_compile_definitions(crash_simulator PRIVATE PLATFORM_LINUX)
    target_link_libraries(crash_simulator PRIVATE pthread)
endif()

# ─── 2. Core Dump Handler (OS kernel integration) ─────────────────────────────
set(COREDUMP_SOURCES
    coredump_handler/src/CoreDump.cpp
    coredump_handler/src/SystemMetrics.cpp
    coredump_handler/src/BacktraceCollector.cpp
    coredump_handler/src/CoreDumpConfig.cpp
    coredump_handler/src/main.cpp
)

add_executable(coredump_handler ${COREDUMP_SOURCES})

target_include_directories(coredump_handler PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/coredump_handler/inc
)

if(WIN32)
    target_compile_definitions(coredump_handler PRIVATE PLATFORM_WINDOWS)
endif()

# ─── 3. Crash Analyzer (intelligent module) ──────────────────────────────────
set(ANALYZER_SOURCES
    analyzer/src/main.cpp
    analyzer/src/CoreAnalyzer.cpp
    analyzer/src/CSVExporter.cpp
    analyzer/src/DatabaseManager.cpp
    analyzer/src/PatternMatcher.cpp
    analyzer/src/MLAnalyzer.cpp
    analyzer/src/SolutionProposer.cpp
    analyzer/src/CoreFileParser.cpp
)

add_executable(crash_analyzer ${ANALYZER_SOURCES})

target_include_directories(crash_analyzer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/analyzer/include
)

if(WIN32)
    target_compile_definitions(crash_analyzer PRIVATE PLATFORM_WINDOWS)
else()
    target_compile_definitions(crash_analyzer PRIVATE PLATFORM_LINUX)
endif()

find_package(SQLite3 QUIET)
if(SQLite3_FOUND)
    target_include_directories(crash_analyzer PRIVATE ${SQLite3_INCLUDE_DIRS})
    target_link_libraries(crash_analyzer PRIVATE ${SQLite3_LIBRARIES})
else()
    target_link_libraries(crash_analyzer PRIVATE sqlite3)
endif()

# ─── 4. Automated Test Suite ──────────────────────────────────────────────────
add_executable(test_suite test/test_suite.cpp)

target_include_directories(test_suite PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/simulator/include
    ${CMAKE_CURRENT_SOURCE_DIR}/analyzer/include
)

if(NOT WIN32)
    target_link_libraries(test_suite PRIVATE pthread)
endif()

# ─── Install targets ──────────────────────────────────────────────────────────
install(TARGETS crash_simulator crash_analyzer coredump_handler
    RUNTIME DESTINATION bin
)

# ─── Build Summary ────────────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "╔════════════════════════════════════════════════════════════╗")
message(STATUS "║     CoreDump Analysis & Solution Engine v3.0               ║")
message(STATUS "╚════════════════════════════════════════════════════════════╝")
message(STATUS "")
message(STATUS "✓ Crash Simulator ............ crash_simulator")
message(STATUS "✓ Core Dump Handler ......... coredump_handler")
message(STATUS "✓ Crash Analyzer (ML) ....... crash_analyzer")
message(STATUS "✓ Test Suite ................. test_suite")
message(STATUS "")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM_PROCESSOR})")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "")
message(STATUS "To build: cmake --build . --config ${CMAKE_BUILD_TYPE}")
message(STATUS "To install: cmake --install .")
message(STATUS "")
