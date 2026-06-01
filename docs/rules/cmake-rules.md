# CMake 规范 — Zephyr 原生 Library API

rtframe 使用 Zephyr 原生 CMake library 机制管理 `src/` 下的编译单元。

---

## 核心原则

`src/` 下每个子目录通过 `zephyr_library_named()` 注册为 Zephyr library，由构建系统自动链接到最终镜像。不强制每个模块独立成库，可以按功能聚合。

---

## 标准模板

```cmake
zephyr_library_named(rtframe_<name>)

zephyr_library_sources(
    foo.c
    bar.cpp
)

zephyr_library_include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 依赖其他 library（可选）
zephyr_library_link_libraries(rtframe_core)
```

---

## API 速查

| API | 用途 |
|-----|------|
| `zephyr_library_named(name)` | 创建命名 library |
| `zephyr_library_sources(...)` | 添加源文件 |
| `zephyr_library_sources_ifdef(KCONFIG ...)` | 条件编译源文件 |
| `zephyr_library_include_directories(...)` | 添加 include 路径 |
| `zephyr_library_link_libraries(...)` | 链接其他 library |
| `zephyr_library_compile_definitions(...)` | 添加编译宏 |
| `zephyr_library_compile_options(...)` | 添加编译选项 |

---

## 禁止的写法

`src/` 下禁止直接操作 `app` target：

```cmake
# ❌ 禁止在 src/ 下使用
target_sources(app PRIVATE foo.c)
target_include_directories(app PRIVATE include/)
```

`target_sources(app PRIVATE ...)` 只允许出现在：
- `targets/cm7/CMakeLists.txt`（注册 `main.cpp`）
- `targets/cm4/CMakeLists.txt`（注册 `main.cpp`）

---

## 条件编译

```cmake
zephyr_library_named(rtframe_uorb)
zephyr_library_sources_ifdef(CONFIG_RTFRAME_UORB
    uorb.cpp
    publication.cpp
)
```

---

## 命名约定

所有 rtframe library 名称以 `rtframe_` 为前缀：

| 层 | library 名 |
|----|-----------|
| core | `rtframe_core` |
| drivers | `rtframe_drivers` |
| lib | `rtframe_lib` |
| modules | `rtframe_modules` |
| middleware/uorb | `rtframe_uorb` |
| middleware/zbus | `rtframe_zbus` |
| modules/mavlink | `rtframe_mavlink` |
| modules/logger | `rtframe_logger` |
| modules/param_loader | `rtframe_param_loader` |
| lib/mavlink_log | `rtframe_mavlink_log` |
| lib/console_buffer | `rtframe_console_buffer` |
| lib/sd_bench | `rtframe_sd_bench` |
