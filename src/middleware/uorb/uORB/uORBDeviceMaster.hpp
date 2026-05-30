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
 * @file uORBDeviceMaster.hpp
 *
 * DeviceMaster — 管理所有 DeviceNode 的注册表。Zephyr port.
 */

#include "uORBCommon.hpp"
#include "uORBDeviceNode.hpp"

#include <uORB/topics/uORBTopics.hpp>
#include <zephyr/kernel.h>
#include <atomic>
#include <cstdint>

namespace uORB
{
class DeviceMaster;
}

class uORB::DeviceMaster
{
public:
	/**
	 * 获取或创建 DeviceNode。
	 * @param meta      topic 元数据
	 * @param instance  实例号（输入/输出）
	 * @param advertiser 是否是发布者（发布者可创建新节点）
	 * @return DeviceNode 指针，失败返回 nullptr
	 */
	UORB_ITCM uORB::DeviceNode *getOrCreateNode(const struct orb_metadata *meta, uint8_t instance);

	/**
	 * 查找已存在的 DeviceNode。
	 */
	UORB_ITCM uORB::DeviceNode *getDeviceNode(const struct orb_metadata *meta, uint8_t instance);

	/**
	 * 按名称字符串查找 DeviceNode（兼容 PX4 接口）。
	 */
	uORB::DeviceNode *getDeviceNode(const char *node_name);

	/**
	 * 检查节点是否存在且已 advertise。
	 */
	bool deviceNodeExists(ORB_ID id, uint8_t instance);

	/**
	 * 打印所有 topic 统计（shell 命令用）
	 */
	void printStatistics();

private:
	friend class uORB::Manager;

	DeviceMaster() = default;
	~DeviceMaster() = default;

	uORB::DeviceNode *getDeviceNodeLocked(const struct orb_metadata *meta, uint8_t instance);

	/* 二维数组索引：O(1) 查找，无需遍历链表 */
	std::atomic<uORB::DeviceNode *> _nodes[ORB_TOPICS_COUNT][ORB_MULTI_MAX_INSTANCES]{};

	/* 原子位图：deviceNodeExists() 无锁读 */
	std::atomic<bool> _node_exists[ORB_TOPICS_COUNT][ORB_MULTI_MAX_INSTANCES]{};

	/* 链表仅用于 printStatistics() 遍历 */
	IntrusiveSortedList<uORB::DeviceNode *> _node_list;

	/* 写锁：仅在创建新节点时加锁 */
	struct k_mutex _lock = Z_MUTEX_INITIALIZER(_lock);

	void lock()   { k_mutex_lock(&_lock, K_FOREVER); }
	void unlock() { k_mutex_unlock(&_lock); }
};
