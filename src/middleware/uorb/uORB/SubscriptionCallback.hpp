/****************************************************************************
 *
 *   Copyright (c) 2019 PX4 Development Team. All rights reserved.
 *   Copyright (c) 2025 rtframe contributors. Zephyr port.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file SubscriptionCallback.hpp
 * uORB SubscriptionCallback — Zephyr port (no fd, k_work based)
 */

#pragma once

#include <uORB/SubscriptionInterval.hpp>
#include <zephyr/kernel.h>

namespace uORB
{

/**
 * Subscription with callback on new publications.
 * Subclass must implement call().
 */
class SubscriptionCallback : public SubscriptionInterval,
	public ListNode<SubscriptionCallback *>
{
public:
	SubscriptionCallback(const orb_metadata *meta, uint32_t interval_us = 0, uint8_t instance = 0) :
		SubscriptionInterval(meta, interval_us, instance)
	{}

	virtual ~SubscriptionCallback()
	{
		unregisterCallback();
	}

	bool registerCallback()
	{
		if (!_registered) {
			/* 确保已订阅 */
			_subscription.subscribe();

			if (_subscription.get_node() &&
			    Manager::register_callback(_subscription.get_node(), this)) {
				_registered = true;
			}
		}

		return _registered;
	}

	void unregisterCallback()
	{
		if (_subscription.get_node()) {
			Manager::unregister_callback(_subscription.get_node(), this);
		}

		_registered = false;
	}

	bool ChangeInstance(uint8_t instance)
	{
		if (instance == get_instance()) {
			return true;
		}

		const bool was_registered = _registered;

		if (was_registered) {
			unregisterCallback();
		}

		bool ret = _subscription.ChangeInstance(instance);

		if (was_registered) {
			registerCallback();
		}

		return ret;
	}

	virtual void call() = 0;

	bool registered() const { return _registered; }

protected:
	bool _registered{false};
};

/**
 * Subscription with callback that submits a k_work item.
 *
 * Usage:
 *   SubscriptionCallbackWorkItem _sub{this, ORB_ID(sensor_accel)};
 *   // in constructor: _sub.registerCallback();
 *   // implement Run() in your class
 */
class SubscriptionCallbackWorkItem : public SubscriptionCallback
{
public:
	/**
	 * @param work_item  pointer to the k_work to submit on new data
	 * @param meta       topic metadata
	 * @param instance   multi-instance index
	 */
	SubscriptionCallbackWorkItem(struct k_work *work_item, const orb_metadata *meta, uint8_t instance = 0) :
		SubscriptionCallback(meta, 0, instance),
		_work_item(work_item)
	{}

	~SubscriptionCallbackWorkItem() override = default;

	void call() override
	{
		if (updated()) {
			k_work_submit(_work_item);
		}
	}

	void set_required_updates(uint8_t required_updates)
	{
		_required_updates = required_updates;
	}

private:
	struct k_work *_work_item;
	uint8_t _required_updates{0};
};

} // namespace uORB
