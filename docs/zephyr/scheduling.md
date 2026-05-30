# 调度：线程与工作队列

Zephyr 提供两种执行单元：`k_thread`（独占线程）和 `k_work` 系列（工作队列任务）。
线程适合阻塞场景，工作队列适合轻量异步任务。

---

## 全景

| 函数 / 宏 | 说明 | 典型用途 |
|-----------|------|---------|
| `k_thread_create()` | 动态创建线程 | 运行时创建独占线程 |
| `K_THREAD_DEFINE()` | 静态定义并自动启动线程 | 编译期固定线程 |
| `K_THREAD_STACK_DEFINE()` | 静态分配线程栈 | 配合 `k_thread_create` |
| `k_thread_name_set()` | 设置线程名 | 调试、日志识别 |
| `k_thread_suspend()` | 挂起线程 | 暂停执行 |
| `k_thread_resume()` | 恢复线程 | 恢复挂起的线程 |
| `k_thread_abort()` | 终止线程 | 销毁线程 |
| `k_work_init()` | 初始化 work | 事件触发任务 |
| `k_work_submit()` | 提交到系统队列 | 快速异步执行 |
| `k_work_submit_to_queue()` | 提交到指定队列 | 控制执行上下文 |
| `K_WORK_DEFINE()` | 静态定义 work | 编译期初始化 |
| `k_work_init_delayable()` | 初始化延迟 work | 周期 / 延迟任务 |
| `k_work_reschedule()` | 延迟调度到系统队列 | 简单延迟执行 |
| `k_work_reschedule_for_queue()` | 延迟调度到指定队列 | 周期任务绝对对齐 |
| `k_work_cancel_delayable()` | 取消延迟 work | 停止周期任务 |
| `k_work_delayable_from_work()` | 从 work 指针取 delayable | handler 内部反向查找 |
| `K_WORK_DELAYABLE_DEFINE()` | 静态定义延迟 work | 编译期初始化 |
| `k_work_queue_start()` | 启动工作队列线程 | 创建自定义队列 |

---

## k_thread — 独占线程

适合需要阻塞等待（信号量、消息队列、`updateBlocking`）或严格独立时序的任务。

```c
K_THREAD_STACK_DEFINE(my_stack, 2048);
struct k_thread my_thread;

static void thread_fn(void *p1, void *p2, void *p3)
{
    while (true) {
        /* 可以阻塞 */
        k_sleep(K_MSEC(100));
    }
}

k_tid_t tid = k_thread_create(&my_thread,
                               my_stack, K_THREAD_STACK_SIZEOF(my_stack),
                               thread_fn,
                               NULL, NULL, NULL,
                               K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
k_thread_name_set(tid, "my_thread");
```

项目用法（[src/core/vwork.cpp](../../src/core/vwork.cpp)）：`vwork::Thread::start()` 封装了上述流程，
栈由 `vwork_config.h` 中的 `K_THREAD_STACK_DEFINE` 静态分配。

**优先级说明：**

| 宏 | 说明 |
|----|------|
| `K_PRIO_PREEMPT(n)` | 抢占式，可被更高优先级线程打断，n 越小优先级越高 |
| `K_PRIO_COOP(n)` | 协作式，主动让出才切换 |

---

## k_work — 事件触发工作

最轻量的异步执行单元，提交后由工作队列线程串行执行，handler 不可阻塞。

```c
static void my_handler(struct k_work *work)
{
    /* 处理事件，不可阻塞 */
}

/* 静态初始化 */
static K_WORK_DEFINE(my_work, my_handler);

/* 或运行时初始化 */
struct k_work my_work;
k_work_init(&my_work, my_handler);

/* 提交到系统默认队列 */
k_work_submit(&my_work);

/* 提交到指定队列 */
k_work_submit_to_queue(&my_wq, &my_work);
```

项目用法（[src/core/vwork.cpp](../../src/core/vwork.cpp)）：`vwork::Event::trigger()` 内部调用 `k_work_submit_to_queue`。

---

## k_work_delayable — 延迟 / 周期工作

在 `k_work` 基础上增加延迟调度能力。配合绝对 tick 可实现无漂移周期任务。

```c
struct k_work_delayable dwork;
k_work_init_delayable(&dwork, my_handler);

/* 延迟 100ms 后执行 */
k_work_reschedule(&dwork, K_MSEC(100));

/* 提交到指定队列，延迟执行 */
k_work_reschedule_for_queue(&my_wq, &dwork, K_MSEC(100));

/* 取消 */
k_work_cancel_delayable(&dwork);

/* handler 内部反向取 delayable 指针 */
struct k_work_delayable *dwork = k_work_delayable_from_work(work);
```

**无漂移周期循环**（[src/core/vwork.cpp](../../src/core/vwork.cpp)）：

```c
/* 启动时记录第一次触发时刻 */
int64_t next_tick = k_uptime_ticks() + k_us_to_ticks_ceil64(period_us);
k_work_reschedule_for_queue(&wq, &dwork, K_NO_WAIT);

/* 每次 handler 结束后推进到下一个绝对时刻 */
next_tick += k_us_to_ticks_ceil64(period_us);
k_work_reschedule_for_queue(&wq, &dwork, K_TIMEOUT_ABS_TICKS(next_tick));
```

项目用法：`vwork::Periodic` 封装了上述模式。

---

## k_work_queue — 工作队列

工作队列是一个专用线程，串行执行提交给它的 work。

```c
K_THREAD_STACK_DEFINE(wq_stack, 1024);
struct k_work_q my_wq;

struct k_work_queue_config cfg = {
    .name     = "my_wq",
    .no_yield = false,
};

k_work_queue_start(&my_wq,
                   wq_stack, K_THREAD_STACK_SIZEOF(wq_stack),
                   K_PRIO_PREEMPT(5), &cfg);
```

项目用法（[src/core/work_queue_manager.cpp](../../src/core/work_queue_manager.cpp)）：
`work_queue_find_or_create` 按名称复用或新建队列，最多 8 个。

---

## 选择指南

| 场景 | 用哪个 |
|------|--------|
| 需要阻塞等待数据 / 信号 | `k_thread` |
| 中断 / 外部事件触发，不阻塞 | `k_work` |
| 周期任务，不阻塞，多任务共享线程 | `k_work_delayable` + 绝对 tick |
| 管理多个 work 的执行上下文 | `k_work_queue` |
| 不要直接用 | `k_thread_create` / `k_work_init`（项目中 vwork 已封装） |
