zephyr_library_named(rtframe_hrt)
zephyr_library_sources(hrt.cpp)
zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# hrt.h 对所有模块可见
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
