# Zephyr API 使用指南

本目录收录项目中常用 Zephyr 内核 API 的使用说明，结合项目实际代码举例，帮助快速定位"该用哪个 API"。

## 文档索引

| 文件 | 内容 |
| ---- | ---- |
| [time.md](time.md) | 时间 API：HRT（项目层）vs Zephyr 内核时间，时间戳、定时循环、性能测量 |
| [scheduling.md](scheduling.md) | 线程（`k_thread`）与工作队列（`k_work` / `k_work_delayable`） |
| [sync.md](sync.md) | 同步原语：mutex、semaphore、spinlock、condvar |
| [messaging.md](messaging.md) | 消息传递：消息队列（`k_msgq`）、邮箱、管道 |
| [logging.md](logging.md) | 日志系统：`LOG_INF` / `LOG_ERR` 用法与 Kconfig 配置 |
| [memory.md](memory.md) | 内存管理：系统堆、自定义堆、内存片（`k_mem_slab`） |
| [zbus.md](zbus.md) | zbus：Zephyr 原生 pub/sub，通道、订阅者、监听者、零拷贝 |
| [static-registration.md](static-registration.md) | 静态注册宏全览：`K_THREAD_DEFINE`、`SYS_INIT`、`ZBUS_CHAN_DEFINE` 等 |

## 快速导航

- **消息时间戳 / 测量耗时** → [time.md](time.md)
- **定时循环 / 线程睡眠** → [time.md](time.md)、[scheduling.md](scheduling.md)
- **线程间数据传递** → [messaging.md](messaging.md)
- **临界区保护** → [sync.md](sync.md)
- **日志输出** → [logging.md](logging.md)
- **动态内存** → [memory.md](memory.md)
- **模块间广播通信（Zephyr 原生）** → [zbus.md](zbus.md)
- **模块间通信（PX4 风格）** → 见 `src/middleware/uorb/`
- **编译期静态注册所有对象** → [static-registration.md](static-registration.md)
- **启动阶段自动初始化** → [static-registration.md](static-registration.md)（SYS_INIT 章节）
