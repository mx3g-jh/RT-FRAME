if(CONFIG_RTFRAME_ZBUS)
    zephyr_library_named(rtframe_demo_pub)
    zephyr_library_sources(demo_pub.cpp)
    zephyr_library_include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${RTFRAME_ROOT}/src/middleware/zbus
    )
    zephyr_library_link_libraries(rtframe_core rtframe_hrt)
endif()
