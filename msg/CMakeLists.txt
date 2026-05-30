# =============================================================================
# msg/CMakeLists.txt — uORB 消息生成
#
# 产出：
#   ${MSG_OUT_DIR}/<snake>.h              每个消息头文件
#   ${MSG_OUT_DIR}/uORBTopics.hpp         topic 枚举 + orb_get_topics()
#   ${MSG_OUT_DIR}/uORBTopics.cpp         orb_get_topics() 实现
#   ${MSG_OUT_DIR}/<snake>.json           字段描述（供压缩字段生成器使用）
#   ${MSG_OUT_DIR}/uORBMessageFieldsGenerated.hpp   压缩字段数据
#   ${MSG_OUT_DIR}/uORBMessageFieldsGenerated.cpp
#
# 依赖：venv 里的 empy==3.3.4 + pyros-genmsg（由 tools/setup_env.sh 安装）
# =============================================================================

if(NOT DEFINED RTFRAME_ROOT)
    get_filename_component(RTFRAME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
endif()

set(MSG_GEN_SCRIPT       "${RTFRAME_ROOT}/tools/msg/px_generate_uorb_topic_files.py")
set(MSG_FIELDS_SCRIPT    "${RTFRAME_ROOT}/tools/msg/px_generate_uorb_compressed_fields.py")
set(MSG_TEMPLATE_DIR     "${RTFRAME_ROOT}/tools/msg/templates/uorb")
set(MSG_OUT_DIR          "${CMAKE_CURRENT_BINARY_DIR}/uORB/topics")

# ── 收集 .msg 文件 ─────────────────────────────────────────────────────────────
file(GLOB msg_files CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.msg"
    "${CMAKE_CURRENT_SOURCE_DIR}/versioned/*.msg"
)
list(SORT msg_files)

# ── 推导生成的头文件路径（PascalCase → snake_case）────────────────────────────
set(uorb_headers)
set(uorb_topic_cpps)
set(uorb_json_files)
foreach(msg_file ${msg_files})
    get_filename_component(msg_name ${msg_file} NAME_WE)
    string(REGEX REPLACE "(.)([A-Z][a-z]+)" "\\1_\\2" snake "${msg_name}")
    string(REGEX REPLACE "([a-z0-9])([A-Z])" "\\1_\\2" snake "${snake}")
    string(TOLOWER "${snake}" snake)
    list(APPEND uorb_headers    "${MSG_OUT_DIR}/${snake}.h")
    list(APPEND uorb_topic_cpps "${MSG_OUT_DIR}/${snake}.cpp")
    list(APPEND uorb_json_files "${MSG_OUT_DIR}/${snake}.json")
endforeach()

# ── Step 1：生成 .h + uORBTopics.hpp/.cpp ─────────────────────────────────────
add_custom_command(
    OUTPUT
        ${uorb_headers}
        "${MSG_OUT_DIR}/uORBTopics.hpp"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${MSG_OUT_DIR}"
    COMMAND ${PYTHON_EXECUTABLE} "${MSG_GEN_SCRIPT}"
        --headers
        -f ${msg_files}
        -i "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/versioned"
        -o "${MSG_OUT_DIR}"
        -e "${MSG_TEMPLATE_DIR}"
    DEPENDS
        ${msg_files}
        "${MSG_GEN_SCRIPT}"
        "${RTFRAME_ROOT}/tools/msg/px_generate_uorb_topic_helper.py"
        "${MSG_TEMPLATE_DIR}/msg.h.em"
        "${MSG_TEMPLATE_DIR}/uORBTopics.hpp.em"
    COMMENT "Generating uORB topic headers"
    VERBATIM
)

# ── Step 1b：生成 per-topic .cpp + uORBTopics.cpp ────────────────────────────
add_custom_command(
    OUTPUT
        ${uorb_topic_cpps}
        "${MSG_OUT_DIR}/uORBTopics.cpp"
    COMMAND ${PYTHON_EXECUTABLE} "${MSG_GEN_SCRIPT}"
        --sources
        -f ${msg_files}
        -i "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/versioned"
        -o "${MSG_OUT_DIR}"
        -e "${MSG_TEMPLATE_DIR}"
    DEPENDS
        ${msg_files}
        "${MSG_GEN_SCRIPT}"
        "${RTFRAME_ROOT}/tools/msg/px_generate_uorb_topic_helper.py"
        "${MSG_TEMPLATE_DIR}/msg.cpp.em"
        "${MSG_TEMPLATE_DIR}/uORBTopics.cpp.em"
        ${uorb_headers}
    COMMENT "Generating uORB per-topic cpp files and uORBTopics.cpp"
    VERBATIM
)


# ── Step 2：生成 .json（字段描述）─────────────────────────────────────────────
add_custom_command(
    OUTPUT ${uorb_json_files}
    COMMAND ${PYTHON_EXECUTABLE} "${MSG_GEN_SCRIPT}"
        --json
        -f ${msg_files}
        -i "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/versioned"
        -o "${MSG_OUT_DIR}"
        -e "${MSG_TEMPLATE_DIR}"
    DEPENDS
        ${msg_files}
        "${MSG_GEN_SCRIPT}"
        "${RTFRAME_ROOT}/tools/msg/px_generate_uorb_topic_helper.py"
        "${MSG_TEMPLATE_DIR}/msg.json.em"
        ${uorb_headers}
    COMMENT "Generating uORB topic JSON"
    VERBATIM
)

# ── Step 3：生成压缩字段 hpp/cpp ──────────────────────────────────────────────
add_custom_command(
    OUTPUT
        "${MSG_OUT_DIR}/uORBMessageFieldsGenerated.hpp"
        "${MSG_OUT_DIR}/uORBMessageFieldsGenerated.cpp"
    COMMAND ${PYTHON_EXECUTABLE} "${MSG_FIELDS_SCRIPT}"
        -f ${uorb_json_files}
        --header-output-file "${MSG_OUT_DIR}/uORBMessageFieldsGenerated.hpp"
        --source-output-file "${MSG_OUT_DIR}/uORBMessageFieldsGenerated.cpp"
    DEPENDS
        ${uorb_json_files}
        "${MSG_FIELDS_SCRIPT}"
    COMMENT "Generating uORBMessageFieldsGenerated"
    VERBATIM
)

# ── 汇总 target ───────────────────────────────────────────────────────────────
add_custom_target(uorb_headers_gen
    DEPENDS
        ${uorb_headers}
        ${uorb_topic_cpps}
        "${MSG_OUT_DIR}/uORBTopics.hpp"
        "${MSG_OUT_DIR}/uORBTopics.cpp"
        ${uorb_json_files}
        "${MSG_OUT_DIR}/uORBMessageFieldsGenerated.hpp"
        "${MSG_OUT_DIR}/uORBMessageFieldsGenerated.cpp"
)

# 导出给父作用域（uorb/CMakeLists.txt 使用）
set(UORB_TOPIC_CPPS ${uorb_topic_cpps} PARENT_SCOPE)

# ── INTERFACE 库：把生成目录加进 include path ─────────────────────────────────
add_library(uorb_msgs INTERFACE)
target_include_directories(uorb_msgs INTERFACE "${CMAKE_CURRENT_BINARY_DIR}")
add_dependencies(uorb_msgs uorb_headers_gen)
