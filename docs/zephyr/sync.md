# 同步原语

Zephyr 提供四种同步原语，按使用场景分：mutex 保护临界区，semaphore 计数信号，
spinlock 用于 ISR 与线程间的极短临界区，condvar 用于条件等待。

---

## 全景

| 函数 / 宏 | 说明 | 典型用途 |
|-----------|------|---------|
| `K_MUTEX_DEFINE()` | 静态定义并初始化 mutex | 编译期全局锁 |
| `Z_MUTEX_INITIALIZER()` | 静态初始化器（用于结构体成员） | 全局结构体内嵌 mutex |
| `k_mutex_init()` | 运行时初始化 mutex | 动态分配的对象 |
| `k_mutex_lock()` | 加锁，支持超时 | 进入临界区 |
| `k_mutex_unlock()` | 解锁 | 离开临界区 |
| `K_SEM_DEFINE()` | 静态定义信号量 | 编译期初始化 |
| `k_sem_init()` | 运行时初始化信号量 | 动态场景 |
| `k_sem_give()` | 释放（+1），可在 ISR 调用 | 通知等待方 |
| `k_sem_take()` | 获取（-1），支持超时 | 等待事件 |
| `k_sem_count_get()` | 读取当前计数 | 状态查询 |
| `k_sem_reset()` | 计数清零 | 重置状态 |
| `k_spin_lock()` | 获取自旋锁，返回 key | ISR + 线程共享数据 |
| `k_spin_unlock()` | 释放自旋锁，传入 key | 配合 `k_spin_lock` |
| `k_condvar_init()` | 初始化条件变量 | 条件等待 |
| `k_condvar_wait()` | 等待条件，原子释放 mutex | 生产者-消费者 |
| `k_condvar_signal()` | 唤醒一个等待者 | 通知单个 |
| `k_condvar_broadcast()` | 唤醒所有等待者 | 广播通知 |

---

## Mutex — 互斥锁

保护临界区，支持优先级继承，不可在 ISR 中使用。

```c
/* 静态定义（全局） */
K_MUTEX_DEFINE(my_mutex);

/* 或作为结构体成员静态初始化 */
struct k_mutex g_mutex = Z_MUTEX_INITIALIZER(g_mutex);

/* 或运行时初始化 */
struct k_mutex my_mutex;
k_mutex_init(&my_mutex);

/* 加锁（永久等待） */
k_mutex_lock(&my_mutex, K_FOREVER);
/* ... 临界区 ... */
k_mutex_unlock(&my_mutex);

/* 加锁（带超时） */
if (k_mutex_lock(&my_mutex, K_MSEC(100)) == 0) {
    /* ... 临界区 ... */
    k_mutex_unlock(&my_mutex);
} else {
    /* 超时 */
}
```

项目用法：

- [src/core/work_queue_manager.cpp](../../src/core/work_queue_manager.cpp)：`k_mutex_init` + `k_mutex_lock/unlock` 保护工作队列槽位
- [src/middleware/uorb/uORB/uORB.cpp](../../src/middleware/uorb/uORB/uORB.cpp)：`Z_MUTEX_INITIALIZER` 保护订阅句柄表

---

## Semaphore — 信号量

计数信号量，可在 ISR 中 `give`，常用于线程间事件通知。

```c
/* 静态定义：初始值 0，最大值 1（二值信号量） */
K_SEM_DEFINE(my_sem, 0, 1);

/* 或运行时初始化 */
struct k_sem my_sem;
k_sem_init(&my_sem, 0, 1);

/* 释放（可在 ISR 中调用） */
k_sem_give(&my_sem);

/* 等待（永久阻塞） */
k_sem_take(&my_sem, K_FOREVER);

/* 等待（带超时） */
if (k_sem_take(&my_sem, K_MSEC(500)) == 0) {
    /* 获取成功 */
} else {
    /* 超时 */
}

/* 非阻塞尝试 */
k_sem_take(&my_sem, K_NO_WAIT);

/* 查询计数 / 重置 */
uint32_t cnt = k_sem_count_get(&my_sem);
k_sem_reset(&my_sem);
```

典型模式：ISR 触发，线程消费。

```c
/* ISR 中 */
k_sem_give(&data_ready_sem);

/* 线程中 */
while (true) {
    k_sem_take(&data_ready_sem, K_FOREVER);
    process_data();
}
```

---

## Spinlock — 自旋锁

极短临界区的 ISR 安全锁。持锁期间禁止抢占和中断，**不可阻塞，不可调用任何可能睡眠的函数**。

```c
struct k_spinlock my_lock;   /* 零初始化即可，无需 init */

/* 加锁，保存中断状态到 key */
k_spinlock_key_t key = k_spin_lock(&my_lock);

/* ... 极短临界区，不可阻塞 ... */

/* 解锁，恢复中断状态 */
k_spin_unlock(&my_lock, key);
```

项目用法（[src/middleware/uorb/uORB/uORBDeviceNode.cpp](../../src/middleware/uorb/uORB/uORBDeviceNode.cpp)）：
保护 uORB 数据拷贝和 generation 计数器更新，回调在锁外执行。

```cpp
k_spinlock_key_t key = k_spin_lock(&_lock);

memcpy(_data + (gen % _queue_size) * meta->o_size, data, meta->o_size);
_generation.store(gen + 1, std::memory_order_release);

k_spin_unlock(&_lock, key);

/* 回调在锁外：k_sem_give / k_work_submit 不能在 spinlock 内调用 */
notify_subscribers();
```

---

## Condvar — 条件变量

配合 mutex 使用，适合生产者-消费者模式。

```c
struct k_mutex my_mutex;
struct k_condvar my_condvar;

k_mutex_init(&my_mutex);
k_condvar_init(&my_condvar);

/* 消费者线程 */
k_mutex_lock(&my_mutex, K_FOREVER);
while (!data_ready) {
    k_condvar_wait(&my_condvar, &my_mutex, K_FOREVER);  /* 原子释放锁并等待 */
}
consume_data();
k_mutex_unlock(&my_mutex);

/* 生产者线程 */
k_mutex_lock(&my_mutex, K_FOREVER);
produce_data();
data_ready = true;
k_condvar_signal(&my_condvar);   /* 唤醒一个等待者 */
/* k_condvar_broadcast 唤醒所有 */
k_mutex_unlock(&my_mutex);
```

---

## 选择指南

| 场景 | 用哪个 |
|------|--------|
| 保护临界区（线程间） | `k_mutex` |
| 线程间事件通知，ISR 可触发 | `k_sem` |
| ISR 与线程共享数据，极短临界区 | `k_spinlock` |
| 条件等待（生产者-消费者） | `k_condvar` + `k_mutex` |
| 不要在 spinlock 内 | 调用 `k_sem_give`、`k_work_submit` 等可能调度的函数 |
