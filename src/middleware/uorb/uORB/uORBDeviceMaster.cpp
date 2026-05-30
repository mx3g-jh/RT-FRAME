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

#include "uORBDeviceMaster.hpp"

#define MODULE_NAME "uORB"
#include <log.h>
#include <cstring>

uORB::DeviceNode *
uORB::DeviceMaster::getOrCreateNode(const struct orb_metadata *meta, uint8_t instance)
{
	if (!meta || instance >= ORB_MULTI_MAX_INSTANCES) {
		return nullptr;
	}

	const orb_id_size_t idx = meta->o_id;

	if (idx >= ORB_TOPICS_COUNT) {
		return nullptr;
	}

	/* 快速无锁读：已存在则直接返回 */
	DeviceNode *node = _nodes[idx][instance].load(std::memory_order_acquire);

	if (node) {
		return node;
	}

	/* 节点不存在，加锁创建 */
	lock();

	/* double-check：加锁后再检查一次，防止并发创建 */
	node = _nodes[idx][instance].load(std::memory_order_relaxed);

	if (!node) {
		node = new DeviceNode(meta, instance);

		if (node) {
			_nodes[idx][instance].store(node, std::memory_order_release);
			_node_list.add(node);
			_node_exists[idx][instance].store(true, std::memory_order_release);
		} else {
			PX4_ERR("failed to create node for %s:%u", meta->o_name, instance);
		}
	}

	unlock();
	return node;
}

uORB::DeviceNode *
uORB::DeviceMaster::getDeviceNode(const struct orb_metadata *meta, uint8_t instance)
{
	if (!meta || instance >= ORB_MULTI_MAX_INSTANCES) {
		return nullptr;
	}

	const orb_id_size_t idx = meta->o_id;

	if (idx >= ORB_TOPICS_COUNT) {
		return nullptr;
	}

	/* O(1) 无锁读 */
	return _nodes[idx][instance].load(std::memory_order_acquire);
}

uORB::DeviceNode *
uORB::DeviceMaster::getDeviceNodeLocked(const struct orb_metadata *meta, uint8_t instance)
{
	/* 有了数组索引，locked 版本和无锁版本相同 */
	return getDeviceNode(meta, instance);
}

uORB::DeviceNode *
uORB::DeviceMaster::getDeviceNode(const char *node_name)
{
	if (!node_name) {
		return nullptr;
	}

	/* 按名称查找仍需遍历链表，但这是非热路径（仅 shell 命令用） */
	for (DeviceNode *node : _node_list) {
		if (strcmp(node->get_name(), node_name) == 0) {
			return node;
		}
	}

	return nullptr;
}

bool
uORB::DeviceMaster::deviceNodeExists(ORB_ID id, uint8_t instance)
{
	const orb_id_size_t idx = static_cast<orb_id_size_t>(id);

	if (idx >= ORB_TOPICS_COUNT || instance >= ORB_MULTI_MAX_INSTANCES) {
		return false;
	}

	/* 原子读，无锁 */
	return _node_exists[idx][instance].load(std::memory_order_acquire);
}

void
uORB::DeviceMaster::printStatistics()
{
	PX4_INFO_RAW("%-40s %2s %6s %4s %5s\n", "Topic", "In", "Gen", "Subs", "Queue");
	PX4_INFO_RAW("--------------------------------------------------------------\n");

	for (DeviceNode *node : _node_list) {
		node->print_statistics(40);
	}
}
