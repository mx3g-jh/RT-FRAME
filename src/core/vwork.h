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

	/* 启动独立线程。period_us 仅作记录（run() 自己控制节奏），可为 0。 */
	bool start(uint32_t period_us = 0);

protected:
	explicit Thread(const config_t &cfg) : _cfg(cfg) {}

	/* 用户实现：线程主体，自己控制循环和节奏。 */
	virtual void run() = 0;

	uint32_t period_us() const { return _period_us; }

private:
	static void thread_entry(void *p1, void *p2, void *p3);

	const config_t &_cfg;
	struct k_thread _thread {};
	uint32_t _period_us{0};
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
	explicit Event(const config_t &cfg) : _cfg(cfg) {}

	virtual void init() {}
	virtual void callback() = 0;

	uint32_t period_us() const { return _period_us; }

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
	bool _inited{false};
};

} // namespace vwork
