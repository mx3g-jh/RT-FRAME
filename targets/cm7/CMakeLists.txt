cmake_minimum_required(VERSION 3.20)

get_filename_component(RTFRAME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

include("${RTFRAME_ROOT}/cmake/toolchain.cmake")

set(BOARD vmu_rt1170/mimxrt1176/cm7)

include("${RTFRAME_ROOT}/cmake/zephyr.cmake")

project(rtframe_cm7 LANGUAGES C CXX ASM)

# Core and module sources
add_subdirectory("${RTFRAME_ROOT}/src/core"    "${CMAKE_BINARY_DIR}/core")
add_subdirectory("${RTFRAME_ROOT}/src/drivers" "${CMAKE_BINARY_DIR}/drivers")
add_subdirectory("${RTFRAME_ROOT}/src/lib"     "${CMAKE_BINARY_DIR}/lib")
add_subdirectory("${RTFRAME_ROOT}/src/modules" "${CMAKE_BINARY_DIR}/modules")

target_sources(app PRIVATE
  "${RTFRAME_ROOT}/src/main.cpp"
)

target_include_directories(app PRIVATE
  "${RTFRAME_ROOT}/src"
  "${RTFRAME_ROOT}/src/core/include"
)
