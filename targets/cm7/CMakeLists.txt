cmake_minimum_required(VERSION 3.20)

get_filename_component(RTFRAME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

include("${RTFRAME_ROOT}/cmake/toolchain.cmake")

set(BOARD vmu_rt1170/mimxrt1176/cm7)

include("${RTFRAME_ROOT}/cmake/zephyr.cmake")

project(rtframe_cm7 LANGUAGES C CXX ASM)

target_sources(app PRIVATE "${BOARD_DIR}/cm7/main.cpp")
target_include_directories(app PRIVATE "${RTFRAME_ROOT}/src")
