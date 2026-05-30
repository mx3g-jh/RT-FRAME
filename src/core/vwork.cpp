#include "vwork.h"
#include "work_queue_manager.h"

namespace vwork
{

/* ==================== Periodic ==================== */

Periodic::~Periodic()
{
	stop();
}

bool Periodic::start(uint32_t period_us)
{
	if (period_us == 0) {
		return false;
	}

	_wq = work_queue_find_or_create(_cfg);
	if (_wq == nullptr) {
		return false;
	}

	_period_us = period_us;
	_wrap.self = this;
	k_work_init_delayable(&_wrap.work, work_handler);
	_scheduled = true;
	_next_tick = k_uptime_ticks() + k_us_to_ticks_ceil64(period_us);
	k_work_reschedule_for_queue(_wq, &_wrap.work, K_NO_WAIT);
	return true;
}

void Periodic::stop()
{
	if (_scheduled) {
		k_work_cancel_delayable(&_wrap.work);
		_scheduled = false;
	}
}

void Periodic::work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	work_wrapper *wrap = CONTAINER_OF(dwork, work_wrapper, work);
	Periodic *self = wrap->self;

	if (!self->_inited) {
		self->init();
		self->_inited = true;
	}

	self->callback();

	if (self->_scheduled) {
		/* 绝对时间对齐：下次触发 = 上次计划时刻 + 周期，无累积漂移。 */
		self->_next_tick += k_us_to_ticks_ceil64(self->_period_us);
		k_work_reschedule_for_queue(self->_wq, &self->_wrap.work,
					    K_TIMEOUT_ABS_TICKS(self->_next_tick));
	}
}

/* ==================== Thread ==================== */

bool Thread::start(uint32_t period_us)
{
	if (_started) {
		return true;
	}

	_period_us = period_us;
	if (period_us > 0) {
		set_period(period_us);
	}

	k_tid_t tid = k_thread_create(&_thread,
				      _cfg.stack,
				      _cfg.stacksize,
				      thread_entry,
				      this, nullptr, nullptr,
				      _cfg.priority, 0, K_NO_WAIT);
	if (tid == nullptr) {
		return false;
	}

	k_thread_name_set(tid, _cfg.name);
	_started = true;
	return true;
}

void Thread::thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	Thread *self = static_cast<Thread *>(p1);
	self->run();
}

/* ==================== Event ==================== */

bool Event::start(uint32_t period_us)
{
	_wq = work_queue_find_or_create(_cfg);
	if (_wq == nullptr) {
		return false;
	}

	_period_us = period_us;
	init();
	return true;
}

void Event::trigger()
{
	if (_wq != nullptr) {
		k_work_submit_to_queue(_wq, &_wrap.work);
	}
}

void Event::work_handler(struct k_work *work)
{
	work_wrapper *wrap = CONTAINER_OF(work, work_wrapper, work);
	Event *self = wrap->self;
	self->callback();
}

} // namespace vwork
