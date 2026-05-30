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

#include "uORBDeviceNode.hpp"
#include "SubscriptionCallback.hpp"

#define MODULE_NAME "uORB"
#include <log.h>

uORB::DeviceNode::DeviceNode(const struct orb_metadata *meta, const uint8_t instance) :
	_meta(meta),
	_instance(instance)
{
	_queue_size = meta->o_queue > 0 ? meta->o_queue : 1;
	_data = static_cast<uint8_t *>(k_malloc(meta->o_size * _queue_size));

	if (_data) {
		memset(_data, 0, meta->o_size * _queue_size);
	} else {
		PX4_ERR("alloc failed for %s (%u bytes)", meta->o_name, meta->o_size * _queue_size);
	}
}

uORB::DeviceNode::~DeviceNode()
{
	if (_data) {
		k_free(_data);
		_data = nullptr;
	}
}

bool
uORB::DeviceNode::publish(const orb_metadata *meta, const void *data)
{
	if (!meta || !data || !_data) {
		return false;
	}

	/* 只保护数据拷贝和 generation 更新 */
	k_spinlock_key_t key = k_spin_lock(&_lock);

	unsigned gen = _generation.load(std::memory_order_relaxed);
	memcpy(_data + (gen % _queue_size) * meta->o_size, data, meta->o_size);
	_generation.store(gen + 1, std::memory_order_release);
	_data_valid = true;

	k_spin_unlock(&_lock, key);

	/* 回调在锁外执行：k_sem_give/k_work_submit 不能在 spinlock 内调用 */
	for (auto cb : _callbacks) {
		cb->call();
	}

	_advertised = true;

	return true;
}

bool
uORB::DeviceNode::copy(void *dst, unsigned &generation)
{
	if (!dst || !_data || !_data_valid) {
		return false;
	}

	k_spinlock_key_t key = k_spin_lock(&_lock);

	unsigned current_gen = _generation.load(std::memory_order_acquire);

	if (current_gen > generation + _queue_size) {
		generation = current_gen - _queue_size;
	}

	unsigned idx = generation % _queue_size;
	memcpy(dst, _data + idx * _meta->o_size, _meta->o_size);
	generation = current_gen;

	k_spin_unlock(&_lock, key);

	return true;
}

unsigned
uORB::DeviceNode::updates_available(unsigned last_generation) const
{
	unsigned current = _generation.load(std::memory_order_acquire);
	return (current > last_generation) ? current - last_generation : 0;
}

unsigned
uORB::DeviceNode::get_initial_generation()
{
	k_spinlock_key_t key = k_spin_lock(&_lock);
	unsigned gen = _generation.load(std::memory_order_relaxed);

	if (_data_valid && gen > 0) {
		gen = gen - 1;
	}

	k_spin_unlock(&_lock, key);
	return gen;
}

bool
uORB::DeviceNode::register_callback(uORB::SubscriptionCallback *callback_sub)
{
	if (!callback_sub) {
		return false;
	}

	k_spinlock_key_t key = k_spin_lock(&_lock);

	for (auto existing : _callbacks) {
		if (callback_sub == existing) {
			k_spin_unlock(&_lock, key);
			return true;
		}
	}

	_callbacks.add(callback_sub);
	k_spin_unlock(&_lock, key);
	return true;
}

void
uORB::DeviceNode::unregister_callback(uORB::SubscriptionCallback *callback_sub)
{
	k_spinlock_key_t key = k_spin_lock(&_lock);
	_callbacks.remove(callback_sub);
	k_spin_unlock(&_lock, key);
}

void
uORB::DeviceNode::print_statistics(int max_topic_length) const
{
	const int width = (max_topic_length > 0) ? max_topic_length : 40;
	PX4_INFO_RAW("%-*s %2u %6u %4d %5u\n",
	       width,
	       get_name(),
	       get_instance(),
	       generation(),
	       subscriber_count(),
	       get_queue_size());
}
