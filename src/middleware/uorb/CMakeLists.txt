if(CONFIG_RTFRAME_UORB)
    if(NOT DEFINED RTFRAME_ROOT)
        get_filename_component(RTFRAME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
    endif()

    # ── 消息生成 ──────────────────────────────────────────────────────────────
    add_subdirectory("${RTFRAME_ROOT}/msg" "${CMAKE_BINARY_DIR}/msg")

    # ── uORB 库本体 ───────────────────────────────────────────────────────────
    set(UORB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/uORB)
    set(MSG_OUT_DIR ${CMAKE_BINARY_DIR}/msg/uORB/topics)

    zephyr_library_named(rtframe_uorb)

    zephyr_library_sources(
        ${UORB_DIR}/uORB.cpp
        ${UORB_DIR}/uORBDeviceNode.cpp
        ${UORB_DIR}/uORBDeviceMaster.cpp
        ${UORB_DIR}/uORBManager.cpp
        ${UORB_DIR}/uORBUtils.cpp
        ${UORB_DIR}/Subscription.cpp
        ${UORB_DIR}/SubscriptionInterval.cpp
        ${UORB_DIR}/uORBMessageFields.cpp
        # 生成文件：消息 topic 列表 + 压缩字段数据
        ${MSG_OUT_DIR}/uORBTopics.cpp
        ${MSG_OUT_DIR}/uORBMessageFieldsGenerated.cpp
    )

    # 每个 topic 的 __orb_xxx 符号定义在各自的 .cpp 里，由 msg/CMakeLists.txt 生成并传入
    if(UORB_TOPIC_CPPS)
        zephyr_library_sources(${UORB_TOPIC_CPPS})
    endif()

    # uORBManagerUsr.cpp 是 NuttX 用户态空文件，不编译

    zephyr_library_include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}          # uORB/ 子目录可见
        ${CMAKE_BINARY_DIR}/msg              # <uORB/topics/xxx.h>
        ${RTFRAME_ROOT}/src/lib              # <lib/mathlib/mathlib.h> 等
        ${RTFRAME_ROOT}/src/include          # <structs/List.hpp> 等
    )

    # 生成文件在编译前必须就绪
    add_dependencies(rtframe_uorb uorb_headers_gen)

    # ── 全局 include（其他模块 #include <uORB/...> 可用）─────────────────────
    zephyr_include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/msg
        ${RTFRAME_ROOT}/src/lib
        ${RTFRAME_ROOT}/src/include
    )

    add_dependencies(app uorb_headers_gen)

endif()
