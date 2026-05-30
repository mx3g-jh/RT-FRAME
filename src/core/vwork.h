#pragma once

#include <zephyr/kernel.h>
#include <stdint.h>
#include "vwork_config.h"

namespace vwork
{

/* ============================================================
 *  三种任务执行模型，统一从 config_t 取栈/优先级。
 *
 *  Periodic —— k_work_delayable 周期任务。多任务共享 work queue 线程，
 *              串行执行，callback 不可长时间阻塞。省内存，适合大量轻量周期任务。
 *
 *  Thread   —— 独占 k_thread。用户在 run() 里自己循环，可阻塞（等 DMA/消息）。
 *              适合需要阻塞或严格独立时序的任务。
 *
 *  Event    —— k_work 事件触发任务。外部调 trigger() 才执行一次 callback。
 *              适合中断/事件驱动的非周期任务。
 *
 *  注：Periodic/Event 跑在 Model::WORKQUEUE 坑位，Thread 跑在 Model::THREAD 坑位。
 *      注册宏会 static_assert 校验基类与 config.model 匹配。
 * ============================================================ */

/* ---------- Periodic：周期 work ---------- */
class Periodic
{
public:
	static constexpr Model MODEL = Model::WORKQUEUE;

	Periodic(const Periodic &) = delete;
	Periodic &operator=(const Periodic &) = delete;
	virtual ~Periodic();

	bool start(uint32_t period_us);
	void stop();

	bool scheduled() const { return _scheduled; }
	uint32_t period_us() const { return _period_us; }

protected:
	explicit Periodic(const config_t &cfg) : _cfg(cfg) {}

	virtual void init() {}
	virtual void callback() = 0;

private:
	static void work_handler(struct k_work *work);

	/* POD 包装：handler 对它用 offsetof 合法（无虚表），再取 self 指针。 */
	struct work_wrapper {
		struct k_work_delayable work;
		Periodic *self;
	};

	const config_t &_cfg;
	struct k_work_q *_wq{nullptr};
	work_wrapper _wrap {};
	uint32_t _period_us{0};
	int64_t _next_tick{0};
	bool _inited{false};
	bool _scheduled{false};
};

/* ---------- Thread：独占线程 ---------- */
class Thread
{
public:
	static constexpr Model MODEL = Model::THREAD;

	Thread(const Thread &) = delete;
	Thread &operator=(const Thread &) = delete;
	virtual ~Thread() = default;

	bool start(uint32_t period_us = 0);

protected:
	explicit Thread(const config_t &cfg) : _cfg(cfg) {}

	/* 周期模式：只需重写 callback()，基类 run() 负责循环和定时。
	 * 需要自定义循环（阻塞等待等）时重写 run() 覆盖默认行为。 */
	virtual void init() {}
	virtual void callback() {}

	/* 自定义循环模式：重写此函数，自己控制节奏。
	 * 默认实现：while(true) { callback(); sleep_until_next(); } */
	virtual void run()
	{
		init();
		while (true) {
			callback();
			sleep_until_next();
		}
	}

	uint32_t period_us() const { return _period_us; }

	void sleep_until_next()
	{
		if (_tick_period_us == 0) {
			return;
		}
		k_sleep(K_TIMEOUT_ABS_TICKS(_next_tick));
		_next_tick += k_us_to_ticks_ceil64(_tick_period_us);
	}

private:
	static void thread_entry(void *p1, void *p2, void *p3);

	void set_period(uint32_t period_us)
	{
		_tick_period_us = period_us;
		_next_tick = k_uptime_ticks() + k_us_to_ticks_ceil64(period_us);
	}

	const config_t &_cfg;
	struct k_thread _thread {};
	uint32_t _period_us{0};
	uint32_t _tick_period_us{0};
	int64_t  _next_tick{0};
	bool _started{false};
};

/* ---------- Event：事件触发 work ---------- */
class Event
{
public:
	static constexpr Model MODEL = Model::WORKQUEUE;

	Event(const Event &) = delete;
	Event &operator=(const Event &) = delete;
	virtual ~Event() = default;

	/* 绑定到 work queue，等待 trigger()。period_us 仅作记录，可为 0。 */
	bool start(uint32_t period_us = 0);

	/* 触发一次 callback（可在中断或其他线程调用）。 */
	void trigger();

protected:
	explicit Event(const config_t &cfg) : _cfg(cfg)
	{
		_wrap.self = this;
		k_work_init(&_wrap.work, work_handler);
	}

	virtual void init() {}
	virtual void callback() = 0;

	uint32_t period_us() const { return _period_us; }

	/* 供子类将 k_work 传给外部系统（如 uORB SubscriptionCallbackWorkItem）。
	 * 构造时即有效，无需等待 start()。 */
	struct k_work *work_ptr() { return &_wrap.work; }

private:
	static void work_handler(struct k_work *work);

	/* POD 包装：handler 对它用 offsetof 合法（无虚表），再取 self 指针。 */
	struct work_wrapper {
		struct k_work work;
		Event *self;
	};

	const config_t &_cfg;
	struct k_work_q *_wq{nullptr};
	work_wrapper _wrap {};
	uint32_t _period_us{0};
};

} // namespace vwork

namespace freq_literals
{
constexpr uint32_t operator""_hz(unsigned long long hz)  { return static_cast<uint32_t>(1000000ULL / hz); }
constexpr uint32_t operator""_khz(unsigned long long khz) { return static_cast<uint32_t>(1000ULL / khz); }
} /* namespace freq_literals */
