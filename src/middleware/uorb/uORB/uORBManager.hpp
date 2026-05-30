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

#ifndef _uORBManager_hpp_
#define _uORBManager_hpp_

/**
 * @file uORBManager.hpp
 *
 * uORB Manager — Zephyr port (no CDev, no fd-based API).
 *
 * 提供两组 API：
 * 1. C API（orb_advertise / orb_publish / orb_exists）— Publication 用
 * 2. Internal API（orb_add_internal_subscriber / orb_data_copy / register_callback）— Subscription 用
 */

#include "uORBCommon.hpp"
#include "uORBDeviceMaster.hpp"
#include <uORB/topics/uORBTopics.hpp>
#include <zephyr/kernel.h>

namespace uORB
{
class Manager;
class SubscriptionCallback;
}

class uORB::Manager
{
public:
	static Manager *get_instance() { return &_instance; }

	/* ─── C API（Publication 用）─────────────────────────────────── */

	static orb_advert_t orb_advertise(const struct orb_metadata *meta, const void *data)
	{
		return orb_advertise_multi(meta, data, nullptr);
	}

	static orb_advert_t orb_advertise_queue(const struct orb_metadata *meta, const void *data,
						unsigned int queue_size)
	{
		(void)queue_size; /* queue size 由 meta->o_queue 决定 */
		return orb_advertise_multi(meta, data, nullptr);
	}

	static orb_advert_t orb_advertise_multi(const struct orb_metadata *meta, const void *data,
						int *instance);

	static orb_advert_t orb_advertise_multi_queue(const struct orb_metadata *meta, const void *data,
						      int *instance, unsigned int queue_size)
	{
		(void)queue_size;
		return orb_advertise_multi(meta, data, instance);
	}

	static int orb_unadvertise(orb_advert_t handle)
	{
		(void)handle; /* DeviceNode 不会被删除 */
		return 0;
	}

	static int orb_publish(const struct orb_metadata *meta, orb_advert_t handle, const void *data);

	static int orb_exists(const struct orb_metadata *meta, int instance);

	static bool orb_device_node_exists(ORB_ID id, uint8_t instance)
	{
		return _instance._device_master.deviceNodeExists(id, instance);
	}

	/* ─── Internal API（Subscription 用）──────────────────────────── */

	/**
	 * 订阅者获取 DeviceNode 指针 + 初始 generation。
	 * @return DeviceNode* (as void*), 失败返回 nullptr
	 */
	static void *orb_add_internal_subscriber(ORB_ID orb_id, uint8_t instance, unsigned *initial_generation);

	/**
	 * 取消内部订阅（当前无需操作，DeviceNode 不跟踪订阅者计数）
	 */
	static void orb_remove_internal_subscriber(void *node_handle)
	{
		(void)node_handle;
	}

	/**
	 * 从 DeviceNode 拷贝数据。
	 * @param node_handle   DeviceNode* (from orb_add_internal_subscriber)
	 * @param dst           目标缓冲
	 * @param generation    调用者的 generation（会被更新）
	 * @param only_if_updated 仅在有新数据时拷贝
	 * @return true if data was copied
	 */
	static bool orb_data_copy(void *node_handle, void *dst, unsigned &generation, bool only_if_updated);

	/**
	 * 获取队列大小
	 */
	static uint8_t orb_get_queue_size(const void *node_handle);

	/**
	 * 注册/注销回调
	 */
	static bool register_callback(void *node_handle, SubscriptionCallback *callback_sub);
	static void unregister_callback(void *node_handle, SubscriptionCallback *callback_sub);

	/**
	 * 获取实例号
	 */
	static uint8_t orb_get_instance(const void *node_handle);

	/**
	 * 可用更新数
	 */
	static unsigned updates_available(const void *node_handle, unsigned last_generation);

	/**
	 * 是否已有发布者
	 */
	static bool is_advertised(const void *node_handle);

	/**
	 * 获取 DeviceMaster
	 */
	DeviceMaster *get_device_master() { return &_device_master; }

private:
	Manager() = default;
	~Manager() = default;

	static Manager _instance;
	DeviceMaster _device_master;
};

#endif /* _uORBManager_hpp_ */
