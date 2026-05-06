cmake_minimum_required(VERSION 3.14)
project(CoreDumpPFE VERSION 3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

target_compile_options(crash_simulator PRIVATE
    -Wall -Wextra
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O2>
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

target_compile_options(coredump_handler PRIVATE
    -Wall -Wextra
)

if(WIN32)
    target_compile_definitions(coredump_handler PRIVATE PLATFORM_WINDOWS)
    target_link_libraries(coredump_handler PRIVATE DbgHelp)
else()
    target_compile_definitions(coredump_handler PRIVATE PLATFORM_LINUX)
    target_link_libraries(coredump_handler PRIVATE pthread dl)
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

target_compile_options(crash_analyzer PRIVATE
    -Wall -Wextra
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O2>
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

if(NOT WIN32)
    target_link_libraries(crash_analyzer PRIVATE stdc++fs)
endif()

# ─── 4. Automated Test Suite ──────────────────────────────────────────────────
add_executable(test_suite test/test_suite.cpp)

target_include_directories(test_suite PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/simulator/include
    ${CMAKE_CURRENT_SOURCE_DIR}/analyzer/include
)

target_compile_options(test_suite PRIVATE -Wall -g)
if(NOT WIN32)
    target_link_libraries(test_suite PRIVATE pthread)
endif()

# ─── Install targets ──────────────────────────────────────────────────────────
install(TARGETS crash_simulator crash_analyzer coredump_handler
    RUNTIME DESTINATION bin
)
