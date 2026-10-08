cmake_minimum_required(VERSION 3.22)
project(iceoryx2_payload_benchmark LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(iceoryx2-cxx REQUIRED)

foreach(name publisher subscriber)
    add_executable(${name} ${name}.cpp)
    target_compile_options(${name} PRIVATE -O2 -Wall -Wextra -Wpedantic)
    target_link_libraries(${name} PRIVATE iceoryx2-cxx::static-lib-cxx)
endforeach()
