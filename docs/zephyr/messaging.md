# 消息传递

Zephyr 提供三种线程间消息传递机制：消息队列（固定大小，最常用）、邮箱（支持同步握手）、管道（字节流）。

---

## 全景

| 函数 / 宏 | 说明 | 典型用途 |
|-----------|------|---------|
| `K_MSGQ_DEFINE()` | 静态定义消息队列 | 编译期固定队列 |
| `k_msgq_init()` | 运行时初始化消息队列 | 动态场景 |
| `k_msgq_put()` | 发送消息，支持超时 | 生产者写入 |
| `k_msgq_get()` | 接收消息，支持超时 | 消费者读取 |
| `k_msgq_peek()` | 查看队首消息（不取出） | 非破坏性读 |
| `k_msgq_purge()` | 清空队列 | 重置状态 |
| `k_msgq_num_used_get()` | 查询已用槽位数 | 状态监控 |
| `k_msgq_num_free_get()` | 查询空闲槽位数 | 背压检测 |
| `k_mbox_put()` | 发送邮箱消息 | 同步 / 异步传递 |
| `k_mbox_get()` | 接收邮箱消息 | 等待消息到达 |
| `k_mbox_data_get()` | 取出邮箱消息数据 | 延迟数据拷贝 |
| `K_PIPE_DEFINE()` | 静态定义管道 | 字节流通信 |
| `k_pipe_init()` | 运行时初始化管道 | 动态场景 |
| `k_pipe_put()` | 写入字节流，支持超时 | 流式数据发送 |
| `k_pipe_get()` | 读取字节流，支持超时 | 流式数据接收 |

---

## k_msgq — 消息队列

最常用的线程间通信方式。固定大小消息，FIFO，可在 ISR 中 `put`。

```c
/* 静态定义：消息大小 sizeof(my_msg_t)，队列深度 10 */
K_MSGQ_DEFINE(my_queue, sizeof(my_msg_t), 10, 4);

/* 或运行时初始化（需提供 buffer） */
char msgq_buf[10 * sizeof(my_msg_t)];
struct k_msgq my_queue;
k_msgq_init(&my_queue, msgq_buf, sizeof(my_msg_t), 10);
```

发送：

```c
my_msg_t msg = { .value = 42 };

/* 永久等待直到有空位 */
k_msgq_put(&my_queue, &msg, K_FOREVER);

/* 带超时 */
if (k_msgq_put(&my_queue, &msg, K_MSEC(100)) != 0) {
    /* 队列满，超时 */
}

/* 非阻塞（ISR 中使用） */
k_msgq_put(&my_queue, &msg, K_NO_WAIT);
```

接收：

```c
my_msg_t msg;

/* 永久阻塞等待 */
k_msgq_get(&my_queue, &msg, K_FOREVER);

/* 带超时 */
if (k_msgq_get(&my_queue, &msg, K_MSEC(500)) == 0) {
    /* 收到消息 */
} else {
    /* 超时 */
}

/* 查看队首但不取出 */
k_msgq_peek(&my_queue, &msg);

/* 查询状态 */
uint32_t used = k_msgq_num_used_get(&my_queue);
uint32_t free = k_msgq_num_free_get(&my_queue);

/* 清空队列 */
k_msgq_purge(&my_queue);
```

典型模式：ISR 采集数据，线程处理。

```c
/* ISR 中（K_NO_WAIT，不可阻塞） */
sensor_data_t data = read_sensor();
k_msgq_put(&sensor_queue, &data, K_NO_WAIT);

/* 处理线程 */
while (true) {
    sensor_data_t data;
    k_msgq_get(&sensor_queue, &data, K_FOREVER);
    process(data);
}
```

---

## k_mbox — 邮箱

支持同步握手（发送方可等待接收方取走数据），适合需要确认的场景。消息可携带指针，避免大数据拷贝。

```c
struct k_mbox my_mbox;
k_mbox_init(&my_mbox);

/* 发送（异步，不等待接收方） */
struct k_mbox_msg msg = {
    .size    = sizeof(my_data),
    .info    = 0,
    .tx_data = &my_data,
    .tx_block = { 0 },
    .tx_target_thread = K_ANY,  /* 任意接收方 */
};
k_mbox_put(&my_mbox, &msg, K_NO_WAIT);

/* 接收 */
struct k_mbox_msg rx_msg = { .size = sizeof(my_data) };
k_mbox_get(&my_mbox, &rx_msg, NULL, K_FOREVER);
/* 若 tx_data 是指针，用 k_mbox_data_get 延迟拷贝 */
k_mbox_data_get(&rx_msg, &local_buf);
```

---

## k_pipe — 管道

字节流通信，适合串口、音频等流式数据场景。

```c
/* 静态定义：缓冲区 256 字节 */
K_PIPE_DEFINE(my_pipe, 256, 4);

/* 或运行时初始化 */
uint8_t pipe_buf[256];
struct k_pipe my_pipe;
k_pipe_init(&my_pipe, pipe_buf, sizeof(pipe_buf));

/* 写入（至少写 len 字节，否则超时） */
size_t written;
k_pipe_put(&my_pipe, data, len, &written, len, K_FOREVER);

/* 读取（至少读 min_xfer 字节） */
size_t read;
k_pipe_get(&my_pipe, buf, sizeof(buf), &read, 1, K_FOREVER);
```

---

## 选择指南

| 场景 | 用哪个 |
|------|--------|
| 固定大小消息，FIFO，最常用 | `k_msgq` |
| ISR → 线程传递数据 | `k_msgq`（`K_NO_WAIT` 发送） |
| 需要发送方确认接收方已取走 | `k_mbox` |
| 传递大数据避免拷贝 | `k_mbox`（指针传递） |
| 字节流（串口、音频） | `k_pipe` |
