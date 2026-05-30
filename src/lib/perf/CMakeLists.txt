zephyr_library_named(rtframe_perf)
zephyr_library_sources(perf_counter.cpp)
zephyr_library_include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${RTFRAME_ROOT}/src/include
    ${RTFRAME_ROOT}/src/lib/log
)
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
zephyr_library_link_libraries(rtframe_hrt rtframe_log)
