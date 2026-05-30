# zbus

zbus 是 Zephyr 原生的发布/订阅总线，基于**静态注册**，零动态内存，编译期确定所有通道和观察者。

---

## 全景

### 核心概念

| 概念 | 说明 |
|------|------|
| **Channel（通道）** | 承载一种消息类型的总线槽，全局唯一，编译期静态定义 |
| **Publisher（发布者）** | 任意代码，调用 `zbus_chan_pub()` 写入通道 |
| **Subscriber（订阅者）** | 持有消息队列，收到通知后主动 `zbus_chan_read()` 读取 |
| **Listener（监听者）** | 回调函数，通道更新时在发布者上下文中同步调用 |
| **Observer（观察者）** | Subscriber 和 Listener 的统称 |

### 主要 API 全景

| 宏 / 函数 | 说明 |
|-----------|------|
| `ZBUS_CHAN_DEFINE()` | 静态定义通道（必须在 `.c` 文件中） |
| `ZBUS_CHAN_DECLARE()` | 在其他文件中声明已定义的通道 |
| `ZBUS_SUBSCRIBER_DEFINE()` | 静态定义订阅者（含消息队列） |
| `ZBUS_LISTENER_DEFINE()` | 静态定义监听者（回调） |
| `ZBUS_OBS_DECLARE()` | 在其他文件中声明已定义的观察者 |
| `zbus_chan_pub()` | 发布消息到通道 |
| `zbus_chan_read()` | 读取通道当前值 |
| `zbus_chan_notify()` | 手动触发通知（不更新值） |
| `zbus_sub_wait()` | 订阅者等待通知 |
| `zbus_sub_wait_msg()` | 订阅者等待并直接取出消息 |
| `zbus_chan_claim()` / `zbus_chan_finish()` | 零拷贝访问通道内部缓冲区 |
| `zbus_chan_add_obs()` / `zbus_chan_rm_obs()` | 运行时动态增删观察者 |

### 与 uORB / k_msgq 对比

| 维度 | zbus | uORB | k_msgq |
|------|------|------|--------|
| 注册方式 | 静态（编译期） | 动态（运行时） | 静态或动态 |
| 消息模型 | 最新值（last-value cache）+ 通知 | 最新值 + 订阅队列 | FIFO 队列 |
| 多订阅者 | 原生支持（广播） | 原生支持 | 需手动复制 |
| ISR 发布 | 支持（`K_NO_WAIT`） | 不支持 | 支持 |
| 零拷贝 | 支持（claim/finish） | 不支持 | 不支持 |
| Zephyr 原生 | ✅ | ❌（PX4 移植） | ✅ |

---

## 通道定义与声明

通道定义**必须放在 `.c` 文件**中（Zephyr C 宏限制）。

```c
/* my_channels.c */
#include <zephyr/zbus/zbus.h>

/* 定义通道：消息类型 struct sensor_data，无观察者列表（后续动态添加或在定义时列出） */
ZBUS_CHAN_DEFINE(sensor_chan,          /* 通道名 */
                struct sensor_data,   /* 消息类型 */
                NULL,                 /* validator（可选，NULL 表示不校验） */
                NULL,                 /* user_data */
                ZBUS_OBSERVERS_EMPTY, /* 初始观察者列表 */
                ZBUS_MSG_INIT(.x = 0, .y = 0, .z = 0) /* 初始值 */
);
```

在其他文件中引用：

```c
/* 任意 .c 或 .cpp 文件 */
#include <zephyr/zbus/zbus.h>
ZBUS_CHAN_DECLARE(sensor_chan);
```

---

## 发布者

发布者不需要注册，任意代码均可发布：

```c
struct sensor_data data = { .x = 1, .y = 2, .z = 3 };

/* 永久等待（通道被 claim 时会阻塞） */
int ret = zbus_chan_pub(&sensor_chan, &data, K_FOREVER);

/* 带超时 */
ret = zbus_chan_pub(&sensor_chan, &data, K_MSEC(10));

/* ISR 中（非阻塞） */
ret = zbus_chan_pub(&sensor_chan, &data, K_NO_WAIT);
```

`zbus_chan_pub()` 会：
1. 将 `data` 写入通道内部缓冲区
2. 通知所有观察者（Listener 同步回调，Subscriber 入队通知）

---

## 订阅者（Subscriber）

订阅者持有一个消息通知队列，收到通知后在自己的线程中读取数据，**发布者和订阅者完全解耦**。

```c
/* my_sub.c */
#include <zephyr/zbus/zbus.h>

ZBUS_CHAN_DECLARE(sensor_chan);

/* 定义订阅者：队列深度 4 */
ZBUS_SUBSCRIBER_DEFINE(sensor_sub, 4);

void sensor_thread(void *a, void *b, void *c)
{
    const struct zbus_channel *chan;

    while (true) {
        /* 等待任意通道通知，获取是哪个通道触发的 */
        zbus_sub_wait(&sensor_sub, &chan, K_FOREVER);

        if (chan == &sensor_chan) {
            struct sensor_data data;
            zbus_chan_read(&sensor_chan, &data, K_MSEC(10));
            /* 处理 data */
        }
    }
}
```

`zbus_sub_wait_msg()` 可以一步完成等待 + 读取（仅适用于只订阅一个通道的场景）：

```c
struct sensor_data data;
zbus_sub_wait_msg(&sensor_sub, &sensor_chan, &data, K_FOREVER);
```

---

## 监听者（Listener）

监听者是同步回调，在**发布者的执行上下文**中直接调用，适合轻量处理（如设置标志位、唤醒信号量）。

```c
/* my_listener.c */
#include <zephyr/zbus/zbus.h>

static void on_sensor_update(const struct zbus_channel *chan)
{
    /* 在发布者上下文中执行，不可阻塞 */
    struct sensor_data data;
    zbus_chan_read(chan, &data, K_NO_WAIT);
    /* 轻量处理，如设置 flag */
}

ZBUS_LISTENER_DEFINE(sensor_listener, on_sensor_update);
```

---

## 将观察者绑定到通道

### 方式一：定义通道时静态绑定

```c
ZBUS_CHAN_DEFINE(sensor_chan,
                struct sensor_data,
                NULL, NULL,
                ZBUS_OBSERVERS(sensor_sub, sensor_listener),
                ZBUS_MSG_INIT(.x = 0)
);
```

### 方式二：运行时动态绑定

```c
zbus_chan_add_obs(&sensor_chan, &sensor_sub, K_MSEC(200));
zbus_chan_add_obs(&sensor_chan, &sensor_listener, K_MSEC(200));

/* 移除 */
zbus_chan_rm_obs(&sensor_chan, &sensor_sub, K_MSEC(200));
```

---

## 零拷贝访问（claim / finish）

适合消息体较大、希望避免额外拷贝的场景：

```c
struct sensor_data *ptr;

/* claim：锁定通道，获取内部缓冲区指针 */
zbus_chan_claim(&sensor_chan, (void **)&ptr, K_MSEC(10));

/* 直接读写缓冲区，期间其他发布者会阻塞 */
ptr->x = 100;

/* finish：释放锁，触发通知 */
zbus_chan_finish(&sensor_chan);
```

---

## 在 C++ 中使用 zbus

Zephyr 的 `ZBUS_CHAN_DEFINE`、`ZBUS_SUBSCRIBER_DEFINE`、`ZBUS_LISTENER_DEFINE` 必须放在 `.c` 文件中。C++ 文件通过声明宏引用：

```cpp
/* my_module.cpp */
#include <zephyr/zbus/zbus.h>

ZBUS_CHAN_DECLARE(sensor_chan);
ZBUS_OBS_DECLARE(sensor_sub);

void MyModule::publish(const SensorData &d)
{
    struct sensor_data raw = { .x = d.x, .y = d.y, .z = d.z };
    zbus_chan_pub(&sensor_chan, &raw, K_NO_WAIT);
}
```

---

## Kconfig

```kconfig
CONFIG_ZBUS=y
CONFIG_ZBUS_CHANNEL_NAME=y   # 可选：通道名字符串（调试用）
CONFIG_ZBUS_OBS_PRIO_STACK_SIZE=2048
```

---

## 选择指南

| 场景 | 推荐 |
|------|------|
| 多模块广播同一数据（如传感器值） | zbus |
| 需要 Zephyr 原生、零动态内存 | zbus |
| 需要 PX4 风格 pub/sub、与 PX4 代码兼容 | uORB |
| ISR → 线程，FIFO 队列语义 | `k_msgq` |
| 轻量回调，不需要独立线程 | zbus Listener |
| 需要历史队列（不只是最新值） | uORB 或 `k_msgq` |
