cmake_minimum_required(VERSION 3.20)

get_filename_component(RTFRAME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

include("${RTFRAME_ROOT}/cmake/toolchain.cmake")

set(BOARD vmu_rt1170/mimxrt1176/cm4)

include("${RTFRAME_ROOT}/cmake/zephyr.cmake")

project(rtframe_cm4 LANGUAGES C CXX ASM)

add_subdirectory("${RTFRAME_ROOT}/src" "${CMAKE_BINARY_DIR}/src")

target_sources(app PRIVATE "${BOARD_DIR}/cm4/main.cpp")
