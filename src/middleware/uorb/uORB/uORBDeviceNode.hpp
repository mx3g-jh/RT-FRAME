/****************************************************************************
 *
 *   Copyright (c) 2012-2016 PX4 Development Team. All rights reserved.
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

#pragma once

/**
 * @file uORBDeviceNode.hpp
 *
 * uORB DeviceNode — Zephyr port (no CDev, pure memory ring buffer).
 */

#include "uORBCommon.hpp"

#include <structs/List.hpp>
#include <structs/IntrusiveSortedList.hpp>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <atomic>
#include <cstring>
#include <cstdlib>

namespace uORB
{
class DeviceNode;
class DeviceMaster;
class Manager;
class SubscriptionCallback;
}

/**
 * Per-topic device instance — pure memory ring buffer, no CDev.
 */
class uORB::DeviceNode : public IntrusiveSortedListNode<uORB::DeviceNode *>
{
public:
	DeviceNode(const struct orb_metadata *meta, const uint8_t instance);
	~DeviceNode();

	DeviceNode(const DeviceNode &) = delete;
	DeviceNode &operator=(const DeviceNode &) = delete;
	DeviceNode(DeviceNode &&) = delete;
	DeviceNode &operator=(DeviceNode &&) = delete;

	bool operator<=(const DeviceNode &rhs) const
	{
		return (strcmp(_meta->o_name, rhs._meta->o_name) <= 0);
	}

	const char *get_name() const { return _meta->o_name; }
	const orb_metadata *get_meta() const { return _meta; }
	uint8_t get_instance() const { return _instance; }

	/**
	 * Publish data to the topic.
	 * @return true on success
	 */
	UORB_ITCM bool publish(const orb_metadata *meta, const void *data);

	/**
	 * Copy data from the topic ring buffer.
	 * @param dst       destination buffer (must be >= meta->o_size)
	 * @param generation in/out: caller's last-seen generation
	 * @return true if data was copied
	 */
	UORB_ITCM bool copy(void *dst, unsigned &generation);

	/**
	 * Number of updates available since given generation.
	 */
	unsigned updates_available(unsigned generation) const;

	bool is_advertised() const { return _advertised; }
	void mark_as_advertised() { _advertised = true; }

	bool is_published() const { return _data_valid; }

	uint8_t get_queue_size() const { return _queue_size; }
	bool set_queue_size(uint8_t queue_size);

	int8_t subscriber_count() const { return _subscriber_count; }
	void add_internal_subscriber() { _subscriber_count++; }
	void remove_internal_subscriber() { _subscriber_count--; }

	unsigned get_initial_generation();
	unsigned generation() const { return _generation.load(std::memory_order_acquire); }

	/* Callback registration */
	bool register_callback(SubscriptionCallback *callback_sub);
	void unregister_callback(SubscriptionCallback *callback_sub);

	void print_statistics(int max_topic_length = 0) const;

private:
	const orb_metadata *_meta;

	uint8_t *_data{nullptr};
	bool _data_valid{false};
	std::atomic<unsigned> _generation{0};
	List<uORB::SubscriptionCallback *> _callbacks;

	const uint8_t _instance;
	bool _advertised{false};
	int8_t _subscriber_count{0};
	uint8_t _queue_size{1};

	/* 保护 _data、_callbacks、_data_valid 的自旋锁（不禁用中断，多核安全） */
	struct k_spinlock _lock{};

	static inline bool is_in_range(unsigned left, unsigned value, unsigned right)
	{
		if (right > left) {
			return (left <= value) && (value <= right);
		} else {
			return (left <= value) || (value <= right);
		}
	}
};
