cmake_minimum_required(VERSION 3.16)
project(aeron_backpressure_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Threads REQUIRED)

set(AERON_DIR "${CMAKE_CURRENT_LIST_DIR}/../aeron" CACHE PATH "Aeron source directory")
if(NOT EXISTS "${AERON_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "Aeron source not found at ${AERON_DIR}. Run ./run_demo.sh or set -DAERON_DIR=/path/to/aeron")
endif()

set(AERON_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(AERON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory("${AERON_DIR}" "${CMAKE_BINARY_DIR}/aeron" EXCLUDE_FROM_ALL)

foreach(name publisher subscriber)
    add_executable(${name} ${name}.cpp)
    target_compile_options(${name} PRIVATE -Wall -Wextra -Wpedantic)
    target_include_directories(${name} PRIVATE
        "${AERON_DIR}/aeron-client/src/main/cpp_wrapper"
        "${AERON_DIR}/aeron-client/src/main/c")
    target_link_libraries(${name} PRIVATE aeron_static Threads::Threads)
endforeach()
