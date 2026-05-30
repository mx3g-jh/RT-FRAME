# 日志系统

Zephyr 日志子系统提供分级、分模块的结构化日志，支持延迟格式化（不在调用点格式化字符串，降低中断延迟）。

---

## 全景

| 函数 / 宏 | 说明 | 典型用途 |
|-----------|------|---------|
| `LOG_MODULE_REGISTER(name, level)` | 在 .c/.cpp 文件中注册模块 | 每个源文件一次 |
| `LOG_MODULE_DECLARE(name, level)` | 在头文件中声明模块（跨文件共享） | 头文件声明 |
| `LOG_ERR(fmt, ...)` | 错误级别日志 | 不可恢复错误 |
| `LOG_WRN(fmt, ...)` | 警告级别日志 | 异常但可继续 |
| `LOG_INF(fmt, ...)` | 信息级别日志 | 正常运行状态 |
| `LOG_DBG(fmt, ...)` | 调试级别日志 | 开发期详细信息 |
| `LOG_HEXDUMP_ERR(data, len, str)` | 十六进制 dump，错误级别 | 二进制数据调试 |
| `LOG_HEXDUMP_WRN(data, len, str)` | 十六进制 dump，警告级别 | 协议帧调试 |
| `LOG_HEXDUMP_INF(data, len, str)` | 十六进制 dump，信息级别 | 数据包内容 |
| `LOG_HEXDUMP_DBG(data, len, str)` | 十六进制 dump，调试级别 | 详细数据追踪 |

**Kconfig 配置：**

| 配置项 | 说明 |
|--------|------|
| `CONFIG_LOG=y` | 启用日志子系统 |
| `CONFIG_LOG_DEFAULT_LEVEL` | 全局默认级别（0=off, 1=err, 2=wrn, 3=inf, 4=dbg） |
| `CONFIG_LOG_MODE_DEFERRED` | 延迟模式（默认，低开销） |
| `CONFIG_LOG_MODE_IMMEDIATE` | 立即模式（调用点直接输出，调试死机时有用） |
| `CONFIG_LOG_BACKEND_UART=y` | 输出到 UART |

---

## LOG_MODULE_REGISTER — 注册模块

每个 .c / .cpp 文件调用一次，定义该文件的模块名和日志级别。

```c
#include <zephyr/logging/log.h>

/* 注册模块，级别 LOG_LEVEL_INF */
LOG_MODULE_REGISTER(my_module, LOG_LEVEL_INF);
```

级别常量：

| 常量 | 值 | 说明 |
|------|----|------|
| `LOG_LEVEL_NONE` | 0 | 关闭 |
| `LOG_LEVEL_ERR` | 1 | 仅错误 |
| `LOG_LEVEL_WRN` | 2 | 警告及以上 |
| `LOG_LEVEL_INF` | 3 | 信息及以上 |
| `LOG_LEVEL_DBG` | 4 | 全部 |

项目用法（[src/modules/demo_pub/demo_pub.cpp](../../src/modules/demo_pub/demo_pub.cpp)）：

```cpp
LOG_MODULE_REGISTER(demo_pub, LOG_LEVEL_INF);
```

---

## 日志宏

格式字符串与 `printf` 兼容，但浮点数需用 `(double)` 强转（Zephyr 日志默认不支持 `%f` 直接输出 `float`）。

```c
LOG_ERR("init failed, ret=%d", ret);
LOG_WRN("timeout (no accel for 1 s)");
LOG_INF("seq=%u t=%llu accel=(%.2f,%.2f,%.2f)",
        seq, now, (double)x, (double)y, (double)z);
LOG_DBG("raw reg=0x%02x", reg_val);
```

项目用法（[src/modules/demo_sub/demo_sub.cpp](../../src/modules/demo_sub/demo_sub.cpp)）：

```cpp
LOG_INF("inst0 t=%llu x=%.3f y=%.3f z=%.3f",
        accel.timestamp,
        (double)accel.x, (double)accel.y, (double)accel.z);

LOG_WRN("timeout (no accel for 1 s)");
```

---

## LOG_HEXDUMP — 十六进制转储

调试二进制数据（协议帧、寄存器内容）时使用。

```c
uint8_t buf[] = { 0x01, 0x02, 0x03, 0xFF };

LOG_HEXDUMP_INF(buf, sizeof(buf), "rx frame");
/* 输出：
 * [00:00:01.234] <inf> my_module: rx frame
 *                01 02 03 ff                      ....
 */
```

---

## 运行时过滤

可在运行时动态调整模块日志级别（需 `CONFIG_LOG_RUNTIME_FILTERING=y`）：

```c
#include <zephyr/logging/log_ctrl.h>

/* 获取模块 source ID */
int src_id = log_source_id_get("my_module");

/* 设置运行时级别 */
log_filter_set(NULL, 0, src_id, LOG_LEVEL_DBG);
```

---

## 选择指南

| 场景 | 用哪个 |
|------|--------|
| 正常运行状态输出 | `LOG_INF` |
| 异常但程序可继续 | `LOG_WRN` |
| 不可恢复错误 | `LOG_ERR` |
| 开发期详细追踪 | `LOG_DBG`（发布前关闭） |
| 二进制数据调试 | `LOG_HEXDUMP_DBG` / `LOG_HEXDUMP_INF` |
| 调试死机 / 硬件异常 | 改用 `CONFIG_LOG_MODE_IMMEDIATE` |
| 浮点数输出 | 强转为 `(double)` 再传给 `%.2f` |
