# 时间 API

项目中存在两层时间 API，分工明确：

| | HRT（项目层） | Zephyr 内核时间 |
|---|---|---|
| 头文件 | `src/lib/hrt/hrt.h` | `<zephyr/kernel.h>` |
| 单位 | µs（`uint64_t`） | ticks 或 ms |
| 精度 | cycle counter，纳秒级 | 取决于 `CONFIG_SYS_CLOCK_TICKS_PER_SEC` |
| 典型用途 | 消息时间戳、性能测量 | 定时循环、线程睡眠、调度 |

---

## HRT（High-Resolution Timer）

### API

```c
// 当前绝对时间，单位 µs
hrt_abstime hrt_absolute_time();

// 自 *then 起经过的时间，单位 µs
hrt_abstime hrt_elapsed_time(const hrt_abstime *then);
```

C++ 字面量（需 `using namespace time_literals`）：

```cpp
1_s    // 1000000 µs
500_ms // 500000 µs
100_us // 100 µs
```

### 示例 1：消息时间戳

```cpp
// demo_pub.cpp, uORB.cpp 中的标准写法
hrt_abstime now = hrt_absolute_time();
accel.timestamp        = now;
accel.timestamp_sample = now;
```

### 示例 2：性能测量

```cpp
hrt_abstime t0 = hrt_absolute_time();
do_work();
LOG_INF("elapsed %llu us", hrt_elapsed_time(&t0));
```

### 示例 3：超时判断（C++ 字面量）

```cpp
using namespace time_literals;

if (hrt_elapsed_time(&_last_update) > 500_ms) {
    // 超过 500ms 未更新，触发超时处理
}
```

---

## Zephyr 内核时间

### 常用函数

```c
// 系统启动后的 tick 数（int64_t）
int64_t k_uptime_ticks(void);

// 系统启动后的毫秒数（int64_t）
int64_t k_uptime_get(void);

// 单位转换：ms / µs → ticks（向上取整）
int64_t k_ms_to_ticks_ceil64(int64_t ms);
int64_t k_us_to_ticks_ceil64(int64_t us);

// 绝对 tick 时刻睡眠（到达指定 tick 才唤醒）
k_sleep(K_TIMEOUT_ABS_TICKS(tick));
```

### 示例 4：定时循环，无累积漂移

这是项目中推荐的定时循环写法（来自 `demo_pub.cpp`）。
用绝对 tick 对齐，每次睡到固定时刻，而不是"睡固定时长"，避免误差累积。

```cpp
// 初始化：计算第一次触发时刻
int64_t next_tick = k_uptime_ticks() + k_ms_to_ticks_ceil64(1);

while (true) {
    // 业务逻辑
    do_work();

    // 睡到绝对时刻，而非相对延迟
    k_sleep(K_TIMEOUT_ABS_TICKS(next_tick));

    // 推进到下一个周期
    next_tick += k_ms_to_ticks_ceil64(1);
}
```

对比错误写法（会累积漂移）：

```cpp
// 不推荐：每次相对延迟，业务耗时会叠加进去
while (true) {
    do_work();
    k_msleep(1); // 实际周期 = 1ms + do_work() 耗时
}
```

### 示例 5：vwork::Periodic 内部实现

`src/core/vwork.cpp` 中工作队列的绝对时间对齐：

```cpp
// 启动时记录第一次触发时刻
_next_tick = k_uptime_ticks() + k_us_to_ticks_ceil64(period_us);

// 每次回调后推进
_next_tick += k_us_to_ticks_ceil64(_period_us);
k_work_reschedule_for_queue(_wq, &_wrap.work,
                            K_TIMEOUT_ABS_TICKS(_next_tick));
```

---

## 选择指南

| 场景 | 用哪个 |
|------|--------|
| 给消息/日志打时间戳 | `hrt_absolute_time()` |
| 测量一段代码耗时 | `hrt_elapsed_time()` |
| 判断是否超时 | `hrt_elapsed_time()` + C++ 字面量 |
| 定时循环（无漂移） | `k_uptime_ticks()` + `K_TIMEOUT_ABS_TICKS` |
| 简单延迟（精度要求低） | `k_msleep()` |
| 不要直接用 | `k_cycle_get_64()`（HRT 已封装） |
