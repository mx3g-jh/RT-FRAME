/****************************************************************************
 *
 *   Copyright (c) 2012-2015 PX4 Development Team. All rights reserved.
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

#include "uORBManager.hpp"
#include "uORBDeviceNode.hpp"
#include "SubscriptionCallback.hpp"

#include <zephyr/sys/printk.h>
#include <cstring>

/* 静态实例 */
uORB::Manager uORB::Manager::_instance;

/* ─── C API ──────────────────────────────────────────────────────────────── */

orb_advert_t
uORB::Manager::orb_advertise_multi(const struct orb_metadata *meta, const void *data, int *instance)
{
	if (!meta) {
		return nullptr;
	}

	uint8_t inst = instance ? (uint8_t)*instance : 0;

	/* 如果调用者传入 instance 指针，自动找下一个未被 advertise 的 instance */
	if (instance) {
		for (uint8_t i = inst; i < ORB_MULTI_MAX_INSTANCES; i++) {
			DeviceNode *existing = _instance._device_master.getDeviceNode(meta, i);
			if (!existing || !existing->is_advertised()) {
				inst = i;
				break;
			}
		}
	}

	DeviceNode *node = _instance._device_master.getOrCreateNode(meta, inst);

	if (!node) {
		return nullptr;
	}

	if (instance) {
		*instance = node->get_instance();
	}

	/* 如果有初始数据，立即发布 */
	if (data) {
		node->publish(meta, data);
	}

	node->mark_as_advertised();

	return static_cast<orb_advert_t>(node);
}

int
uORB::Manager::orb_publish(const struct orb_metadata *meta, orb_advert_t handle, const void *data)
{
	if (!handle || !data || !meta) {
		return -1;
	}

	DeviceNode *node = static_cast<DeviceNode *>(handle);
	return node->publish(meta, data) ? 0 : -1;
}

int
uORB::Manager::orb_exists(const struct orb_metadata *meta, int instance)
{
	if (!meta) {
		return -1;
	}

	DeviceNode *node = _instance._device_master.getDeviceNode(meta, (uint8_t)instance);

	if (node && node->is_advertised()) {
		return 0;
	}

	return -1;
}

/* ─── Internal API（Subscription 用）──────────────────────────────────── */

void *
uORB::Manager::orb_add_internal_subscriber(ORB_ID orb_id, uint8_t instance, unsigned *initial_generation)
{
	const struct orb_metadata *meta = get_orb_meta(orb_id);

	if (!meta) {
		return nullptr;
	}

	DeviceNode *node = _instance._device_master.getOrCreateNode(meta, instance);

	if (!node) {
		return nullptr;
	}

	if (initial_generation) {
		*initial_generation = node->get_initial_generation();
	}

	return static_cast<void *>(node);
}

bool
uORB::Manager::orb_data_copy(void *node_handle, void *dst, unsigned &generation, bool only_if_updated)
{
	if (!node_handle || !dst) {
		return false;
	}

	DeviceNode *node = static_cast<DeviceNode *>(node_handle);

	if (only_if_updated && node->updates_available(generation) == 0) {
		return false;
	}

	return node->copy(dst, generation);
}

uint8_t
uORB::Manager::orb_get_queue_size(const void *node_handle)
{
	if (!node_handle) {
		return 1;
	}

	const DeviceNode *node = static_cast<const DeviceNode *>(node_handle);
	return node->get_queue_size();
}

bool
uORB::Manager::register_callback(void *node_handle, SubscriptionCallback *callback_sub)
{
	if (!node_handle) {
		return false;
	}

	DeviceNode *node = static_cast<DeviceNode *>(node_handle);
	return node->register_callback(callback_sub);
}

void
uORB::Manager::unregister_callback(void *node_handle, SubscriptionCallback *callback_sub)
{
	if (!node_handle) {
		return;
	}

	DeviceNode *node = static_cast<DeviceNode *>(node_handle);
	node->unregister_callback(callback_sub);
}

uint8_t
uORB::Manager::orb_get_instance(const void *node_handle)
{
	if (!node_handle) {
		return 0;
	}

	const DeviceNode *node = static_cast<const DeviceNode *>(node_handle);
	return node->get_instance();
}

unsigned
uORB::Manager::updates_available(const void *node_handle, unsigned last_generation)
{
	if (!node_handle) {
		return 0;
	}

	const DeviceNode *node = static_cast<const DeviceNode *>(node_handle);
	return node->updates_available(last_generation);
}

bool
uORB::Manager::is_advertised(const void *node_handle)
{
	if (!node_handle) {
		return false;
	}

	const DeviceNode *node = static_cast<const DeviceNode *>(node_handle);
	return node->is_advertised();
}
