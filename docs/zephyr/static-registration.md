# Zephyr 静态注册宏

Zephyr 大量使用**编译期静态注册**机制：通过宏在特殊 ELF section 中放置描述符，内核启动时直接遍历，无需运行时注册调用。本文汇总项目中常用的静态注册宏。

---

## 全景

| 宏 | 所属子系统 | 注册对象 |
|----|-----------|---------|
| `ZBUS_CHAN_DEFINE()` | zbus | 消息通道 |
| `ZBUS_SUBSCRIBER_DEFINE()` | zbus | 订阅者（含通知队列） |
| `ZBUS_LISTENER_DEFINE()` | zbus | 监听者（同步回调） |
| `K_THREAD_DEFINE()` | 内核线程 | 静态线程 |
| `K_WORK_DEFINE()` | 工作队列 | 普通工作项 |
| `K_WORK_DELAYABLE_DEFINE()` | 工作队列 | 可延迟工作项 |
| `K_MSGQ_DEFINE()` | 消息队列 | 静态消息队列 |
| `K_SEM_DEFINE()` | 同步 | 信号量 |
| `K_MUTEX_DEFINE()` | 同步 | 互斥锁 |
| `K_CONDVAR_DEFINE()` | 同步 | 条件变量 |
| `K_TIMER_DEFINE()` | 定时器 | 软件定时器 |
| `K_MEM_SLAB_DEFINE()` | 内存 | 内存片分配器 |
| `K_FIFO_DEFINE()` | FIFO | 无类型 FIFO |
| `K_LIFO_DEFINE()` | LIFO | 无类型 LIFO |
| `K_STACK_DEFINE()` | 栈 | 整数栈 |
| `K_PIPE_DEFINE()` | 管道 | 字节流管道 |
| `SYS_INIT()` | 初始化钩子 | 启动阶段回调 |
| `DEVICE_DT_DEFINE()` | 驱动模型 | 设备实例 |
| `LOG_MODULE_REGISTER()` | 日志 | 模块日志域 |

---

## zbus 通道与观察者

详见 [zbus.md](zbus.md)，此处仅列签名。

```c
ZBUS_CHAN_DEFINE(name, msg_type, validator, user_data, observers, init_val)
ZBUS_SUBSCRIBER_DEFINE(name, queue_size)
ZBUS_LISTENER_DEFINE(name, callback)

/* 跨文件声明 */
ZBUS_CHAN_DECLARE(name)
ZBUS_OBS_DECLARE(name)
```

---

## K_THREAD_DEFINE — 静态线程

编译期创建线程，内核启动后自动运行，无需 `k_thread_create()`。

```c
/* 签名 */
K_THREAD_DEFINE(name, stack_size, entry, p1, p2, p3, prio, options, delay)
```

| 参数 | 说明 |
|------|------|
| `name` | 线程标识符（也是 `k_thread` 变量名） |
| `stack_size` | 栈大小（字节） |
| `entry` | 入口函数 `void entry(void*, void*, void*)` |
| `p1/p2/p3` | 传给入口函数的三个参数 |
| `prio` | 优先级，数值越小越高（0 最高，正数协作式） |
| `options` | 通常 `0`，或 `K_ESSENTIAL`（线程退出则系统 panic） |
| `delay` | 启动延迟（`K_NO_WAIT` 立即启动，`K_MSEC(n)` 延迟） |

```c
#include <zephyr/kernel.h>

static void sensor_thread_fn(void *a, void *b, void *c)
{
    while (true) {
        /* 处理逻辑 */
        k_sleep(K_MSEC(10));
    }
}

/* 栈 2048 字节，优先级 5，立即启动 */
K_THREAD_DEFINE(sensor_thread, 2048,
                sensor_thread_fn, NULL, NULL, NULL,
                5, 0, K_NO_WAIT);
```

跨文件引用线程对象：

```c
extern const k_tid_t sensor_thread;  /* K_THREAD_DEFINE 同时定义了 k_tid_t */
k_thread_suspend(sensor_thread);
```

---

## K_WORK_DEFINE / K_WORK_DELAYABLE_DEFINE — 工作项

工作项在系统工作队列（或自定义队列）中执行，适合将 ISR 中的处理延迟到线程上下文。

```c
K_WORK_DEFINE(name, handler)
K_WORK_DELAYABLE_DEFINE(name, handler)
```

```c
#include <zephyr/kernel.h>

static void process_work_fn(struct k_work *work)
{
    /* 在工作队列线程中执行 */
}

K_WORK_DEFINE(process_work, process_work_fn);

/* ISR 或任意上下文中提交 */
k_work_submit(&process_work);

/* 延迟版本 */
static void delayed_fn(struct k_work *work)
{
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    /* 处理 */
}

K_WORK_DELAYABLE_DEFINE(delayed_work, delayed_fn);

/* 500ms 后执行 */
k_work_schedule(&delayed_work, K_MSEC(500));

/* 取消 */
k_work_cancel_delayable(&delayed_work);
```

---

## K_MSGQ_DEFINE — 消息队列

```c
K_MSGQ_DEFINE(name, msg_size, max_msgs, align)
```

```c
#include <zephyr/kernel.h>

struct imu_sample { float ax, ay, az; };

/* 消息大小 sizeof(struct imu_sample)，深度 8，4 字节对齐 */
K_MSGQ_DEFINE(imu_queue, sizeof(struct imu_sample), 8, 4);

/* 发送（ISR 中用 K_NO_WAIT） */
struct imu_sample s = { .ax = 1.0f };
k_msgq_put(&imu_queue, &s, K_NO_WAIT);

/* 接收 */
struct imu_sample rx;
k_msgq_get(&imu_queue, &rx, K_FOREVER);
```

---

## K_SEM_DEFINE — 信号量

```c
K_SEM_DEFINE(name, initial_count, limit)
```

```c
K_SEM_DEFINE(data_ready, 0, 1);  /* 初始 0，最大 1（二值信号量） */

/* ISR 或发布者 */
k_sem_give(&data_ready);

/* 等待者 */
k_sem_take(&data_ready, K_FOREVER);
```

---

## K_MUTEX_DEFINE — 互斥锁

```c
K_MUTEX_DEFINE(name)
```

```c
K_MUTEX_DEFINE(shared_buf_lock);

k_mutex_lock(&shared_buf_lock, K_FOREVER);
/* 临界区 */
k_mutex_unlock(&shared_buf_lock);
```

---

## K_CONDVAR_DEFINE — 条件变量

```c
K_CONDVAR_DEFINE(name)
```

```c
K_MUTEX_DEFINE(state_lock);
K_CONDVAR_DEFINE(state_ready);

/* 等待方 */
k_mutex_lock(&state_lock, K_FOREVER);
while (!condition) {
    k_condvar_wait(&state_ready, &state_lock, K_FOREVER);
}
k_mutex_unlock(&state_lock);

/* 通知方 */
k_mutex_lock(&state_lock, K_FOREVER);
condition = true;
k_condvar_signal(&state_ready);
k_mutex_unlock(&state_lock);
```

---

## K_TIMER_DEFINE — 软件定时器

```c
K_TIMER_DEFINE(name, expiry_fn, stop_fn)
```

```c
static void timer_expiry(struct k_timer *timer)
{
    /* 在系统时钟中断上下文中执行，不可阻塞 */
    k_work_submit(&process_work);  /* 通常只做提交工作项 */
}

K_TIMER_DEFINE(my_timer, timer_expiry, NULL);

/* 首次 100ms 触发，之后每 50ms 重复 */
k_timer_start(&my_timer, K_MSEC(100), K_MSEC(50));

/* 停止 */
k_timer_stop(&my_timer);

/* 查询剩余时间 */
k_ticks_t remaining = k_timer_remaining_ticks(&my_timer);
```

---

## K_MEM_SLAB_DEFINE — 内存片

固定大小块的池式分配器，分配/释放为 O(1)，无碎片。

```c
K_MEM_SLAB_DEFINE(name, block_size, num_blocks, align)
```

```c
K_MEM_SLAB_DEFINE(packet_slab, 128, 16, 4);  /* 16 块，每块 128 字节 */

void *ptr;
if (k_mem_slab_alloc(&packet_slab, &ptr, K_NO_WAIT) == 0) {
    /* 使用 ptr */
    k_mem_slab_free(&packet_slab, ptr);
}

/* 查询空闲块数 */
uint32_t free = k_mem_slab_num_free_get(&packet_slab);
```

---

## K_FIFO_DEFINE / K_LIFO_DEFINE — 无类型队列

传递任意结构体指针（结构体首字段必须是 `void *` 保留给内核链表）。

```c
K_FIFO_DEFINE(name)
K_LIFO_DEFINE(name)
```

```c
struct my_item {
    void *fifo_reserved;  /* 必须是第一个字段 */
    int   value;
};

K_FIFO_DEFINE(my_fifo);

struct my_item item = { .value = 42 };
k_fifo_put(&my_fifo, &item);

struct my_item *rx = k_fifo_get(&my_fifo, K_FOREVER);
```

---

## K_PIPE_DEFINE — 字节流管道

```c
K_PIPE_DEFINE(name, pipe_buffer_size, align)
```

```c
K_PIPE_DEFINE(uart_pipe, 256, 4);

/* 写入 */
size_t written;
k_pipe_put(&uart_pipe, data, len, &written, len, K_FOREVER);

/* 读取（至少读 1 字节） */
size_t read;
k_pipe_get(&uart_pipe, buf, sizeof(buf), &read, 1, K_FOREVER);
```

---

## SYS_INIT — 启动阶段钩子

在系统启动的指定阶段自动调用初始化函数，无需在 `main()` 中手动调用。

```c
SYS_INIT(init_fn, level, priority)
```

| level | 阶段 | 说明 |
|-------|------|------|
| `PRE_KERNEL_1` | 最早 | 硬件初始化，中断未开启 |
| `PRE_KERNEL_2` | 早期 | 依赖 PRE_KERNEL_1 的硬件 |
| `POST_KERNEL` | 内核就绪后 | 可使用内核服务 |
| `APPLICATION` | 应用启动前 | 最晚，可使用全部服务 |

```c
#include <zephyr/init.h>

static int my_init(void)
{
    /* 初始化逻辑 */
    return 0;  /* 非 0 表示失败，会触发 panic（视配置） */
}

/* POST_KERNEL 阶段，优先级 10（同阶段内数值小的先执行） */
SYS_INIT(my_init, POST_KERNEL, 10);
```

---

## LOG_MODULE_REGISTER — 日志模块

```c
LOG_MODULE_REGISTER(name, level)
```

```c
#include <zephyr/logging/log.h>

/* 每个 .cpp/.c 文件一个，模块名与目录名一致 */
LOG_MODULE_REGISTER(imu_driver, LOG_LEVEL_INF);

void imu_init(void)
{
    LOG_INF("IMU initialized");
    LOG_DBG("reg=0x%02x", reg_val);
    LOG_ERR("SPI timeout");
}
```

跨文件引用同一模块（不重复注册）：

```c
LOG_MODULE_DECLARE(imu_driver, LOG_LEVEL_INF);
```

---

## DEVICE_DT_DEFINE — 设备实例

驱动开发时使用，将 DTS 节点绑定到驱动实现。应用层通过 `DEVICE_DT_GET()` 获取设备指针，不直接使用此宏。

```c
/* 驱动实现（通常在 Zephyr 驱动层，不在 src/ 下） */
DEVICE_DT_DEFINE(node_id, init_fn, pm, data, config, level, prio, api, ...);

/* 应用层获取设备 */
const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(lpuart1));
if (!device_is_ready(dev)) {
    LOG_ERR("device not ready");
}
```

---

## 使用原则

1. **优先静态注册**：能在编译期确定的对象（线程、队列、信号量）一律用静态宏，避免运行时 `k_malloc`
2. **zbus 定义放 `.c`**：`ZBUS_CHAN_DEFINE`、`ZBUS_SUBSCRIBER_DEFINE`、`ZBUS_LISTENER_DEFINE` 必须在 `.c` 文件中，C++ 通过 `DECLARE` 宏引用
3. **SYS_INIT 替代 main 初始化**：模块自己的初始化用 `SYS_INIT`，`main()` 只做顶层协调
4. **LOG_MODULE_REGISTER 一文件一个**：每个 `.cpp` 对应一个，模块名与目录名一致（规则 8.5）
