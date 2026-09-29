cmake_minimum_required(VERSION 3.22)

project(ipc_demo_iceoryx2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(iceoryx2-cxx REQUIRED)

function(add_iox2_demo target source output_dir output_name)
    add_executable(${target} ${source})
    target_compile_options(${target} PRIVATE -O2 -Wall -Wextra -Wpedantic)
    target_link_libraries(${target} PRIVATE iceoryx2-cxx::static-lib-cxx)
    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME ${output_name}
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${output_dir}")
    target_include_directories(${target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/${output_dir}")
endfunction()

add_iox2_demo(iox2_basic_publisher basic/publisher.cpp basic publisher)
add_iox2_demo(iox2_basic_subscriber basic/subscriber.cpp basic subscriber)
add_iox2_demo(iox2_large_publisher large-message/publisher.cpp large-message publisher)
add_iox2_demo(iox2_large_subscriber large-message/subscriber.cpp large-message subscriber)

