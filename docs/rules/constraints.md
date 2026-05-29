# 项目强约束总览

本文是项目所有强约束的权威来源。**新代码、重构、迁移、工具链变更，都必须遵守本文规则。**

适用范围：rtframe 项目，基于 Zephyr RTOS，目标平台 NXP i.MX RT1176 (CM7 + CM4)。

---

## 规则 0 — 计划管理强制

所有跨文件改动、架构变更、引入新依赖，必须先在 `docs/rules/plans/` 创建计划文件，在 `plans/SUMMARY.md` 登记状态，用户确认后再动手。

文件命名：`<topic>.md`，内容包含：目标 / 方案 / 取舍 / 实施步骤 / 验收标准。

---

## 规则 1 — Git 提交格式

提交消息格式：`[TAG] 简洁描述`

| TAG | 用途 |
| --- | ---- |
| `FEAT` | 新功能、新模块 |
| `FIX` | Bug 修复 |
| `DOCS` | 纯文档变更 |
| `REFACTOR` | 重构 |
| `TEST` | 测试相关 |
| `CHORE` | 构建、工具链、依赖变更 |
| `PERF` | 性能优化 |
| `BREAKING` | 破坏性变更 |

详见 [`git-commit.md`](git-commit.md)。

---

## 规则 2 — 分层架构（PX4 风格，Zephyr 原生实现）

项目采用 PX4 启发的分层架构，通过 Zephyr 原生 CMake library 机制强制层边界。

```
┌──────────────────────────────────────────────┐
│           应用层 (modules)                    │  src/modules/
│  业务逻辑，通过通信中间件与其他模块通信         │
│  不直接调用驱动，不直接操作硬件                │
├──────────────────────────────────────────────┤
│           通信中间件层 (middleware)            │  src/middleware/
│  uORB（PX4 风格 pub/sub）                    │
│  zbus（Zephyr 原生 pub/sub）                 │
│  MAVLink、自定义协议等（按需添加）             │
│  各协议均为独立子目录，由 Kconfig 控制         │
├──────────────────────────────────────────────┤
│           通用库层 (lib)                      │  src/lib/
│  与平台无关的算法、数据结构、工具库            │
│  不依赖硬件，不依赖通信中间件                  │
├──────────────────────────────────────────────┤
│           驱动抽象层 (drivers)                │  src/drivers/
│  Zephyr 驱动 API 封装，屏蔽芯片差异           │
│  只通过 Zephyr API 访问硬件，不直接操作寄存器  │
├──────────────────────────────────────────────┤
│           核心层 (core)                       │  src/core/
│  系统初始化、线程管理、错误处理等基础设施       │
├──────────────────────────────────────────────┤
│           板级支持 (boards)                   │  boards/nxp/vmu_rt1170/
│  DTS、Kconfig、板级初始化、main 入口           │
│  CM7 和 CM4 各自独立                          │
├──────────────────────────────────────────────┤
│           Zephyr RTOS + HAL                  │  middlewares/zephyr/
│                                              │  hardware/hal_nxp/
└──────────────────────────────────────────────┘
```

详见 [`layer-usage.md`](layer-usage.md)。

---

## 规则 3 — CMake：只用 Zephyr 原生 Library API

所有 `src/` 下的 CMakeLists.txt 必须使用 Zephyr 原生 CMake API：

```cmake
# ✅ 正确
zephyr_library_named(rtframe_core)
zephyr_library_sources(foo.c bar.cpp)
zephyr_library_include_directories(include/)
zephyr_library_link_libraries(rtframe_lib)

# ❌ 禁止（仅 targets/ 入口点允许）
target_sources(app PRIVATE ...)
target_include_directories(app PRIVATE ...)
```

`target_sources(app PRIVATE ...)` 只允许在 `targets/cm7/CMakeLists.txt` 和 `targets/cm4/CMakeLists.txt` 中用于注册 `main.cpp`，其他地方禁止。

详见 [`cmake-rules.md`](cmake-rules.md)。

---

## 规则 4 — Kconfig 命名空间

所有自定义 Kconfig 符号必须以 `RTFRAME_` 为前缀：

```kconfig
# ✅ 正确
config RTFRAME_UORB
config RTFRAME_ZBUS
config RTFRAME_IMU_TASK

# ❌ 禁止
config UORB
config MY_FEATURE
```

每个 `src/` 子目录必须有对应的 `Kconfig` 文件，并在父目录 `Kconfig` 中 `rsource`。

---

## 规则 5 — 通信中间件

uORB 和 zbus 默认均启用，可独立关闭：

```kconfig
CONFIG_RTFRAME_UORB=y   # uORB pub/sub（默认开）
CONFIG_RTFRAME_ZBUS=y   # zbus pub/sub（默认开，自动 select ZBUS）
```

新增通信协议（如 MAVLink）在 `src/middleware/` 下新建子目录，提供独立 Kconfig，默认关闭，按需启用。

---

## 规则 6 — 新 Board 添加流程

1. 在 `boards/<vendor>/<board>/` 下创建目录结构（参考 用户指定的`middlewares/zephyr/boards`）
2. 提供 `<board>.yaml`、`<board>_defconfig`、`<board>.dts`、`<board>.dtsi`、`board.cmake`等
3. CM7/CM4 各自在子目录下提供 `main.cpp`
4. 在 `tools/toolchains.conf` 中确认所需工具链已启用
5. 在 `targets/` 下新建对应 target 目录，复制并修改 `CMakeLists.txt`
6. 在 `Makefile` 中添加对应 `make` 目标

详见 [`board-rules.md`](board-rules.md)。

---

## 规则 7 — 自包含构建环境

项目必须在 `git clone` + `bash tools/setup_env.sh` 后即可构建，不依赖任何外部环境变量或系统全局安装：

- 所有依赖通过 git submodule 管理（`middlewares/zephyr`、`hardware/hal_nxp` 等）
- Zephyr SDK 下载到 `toolchain/zephyr-sdk-<version>/`，不写入系统
- Python venv 在 `.venv/` 下，不依赖系统 Python 包
- 所需工具链在 `tools/toolchains.conf` 中声明

验证：`bash tools/setup_env.sh --check`

---

## 规则 8 — AI Coding 约束（强制）

**8.1 禁止自作主张改变架构**
- 不得在未经用户确认的情况下引入新的中间件、框架或依赖
- 不得修改 `CMakeLists.txt` 的顶层结构（`targets/`、`cmake/`）
- 不得修改 `.gitmodules` 或 submodule 指向

**8.2 禁止跨层污染**
- 不得在 `src/lib/` 或 `src/drivers/` 中引入对 `src/modules/` 的依赖
- 不得在任何层直接 include `fsl_*.h` 或操作寄存器
- 不得绕过 Zephyr 驱动 API 直接访问硬件

**8.3 重大改动必须先规划**
- 涉及 3 个以上文件的改动，必须先在 `docs/rules/plans/` 写计划
- 改动 DTS / Kconfig / CMakeLists.txt 必须说明原因
- 不得删除或重命名现有公共接口，除非用户明确要求

**8.4 代码风格**
- 不写无意义注释（不解释"做了什么"，只写"为什么"）
- 不添加用户未要求的功能或防御性代码
- 不引入新的抽象层或 helper，除非任务明确需要

---

## 文档索引

| 文档 | 说明 |
| ---- | ---- |
| [`constraints.md`](constraints.md) | 本文，所有强约束汇总 |
| [`cmake-rules.md`](cmake-rules.md) | Zephyr CMake library API 规范 |
| [`layer-usage.md`](layer-usage.md) | 分层架构详细说明 |
| [`board-rules.md`](board-rules.md) | 新 board 添加流程 |
| [`git-commit.md`](git-commit.md) | Git 提交格式 |
| [`plans/SUMMARY.md`](plans/SUMMARY.md) | 计划汇总索引 |
