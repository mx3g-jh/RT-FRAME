/****************************************************************************
 *
 *   Copyright (c) 2012-2015 PX4 Development Team. All rights reserved.
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

#include "uORB.h"
#include "uORBManager.hpp"
#include "uORBMessageFields.hpp"
#include "Subscription.hpp"
#include <uORB/topics/uORBTopics.hpp>
#include <inttypes.h>
#include "hrt.h"
#include <zephyr/kernel.h>
#include <log.h>

orb_advert_t orb_advertise(const struct orb_metadata *meta, const void *data)
{
	return uORB::Manager::orb_advertise(meta, data);
}

orb_advert_t orb_advertise_multi(const struct orb_metadata *meta, const void *data, int *instance)
{
	return uORB::Manager::orb_advertise_multi(meta, data, instance);
}

orb_advert_t orb_advertise_queue(const struct orb_metadata *meta, const void *data, unsigned int queue_size)
{
	return uORB::Manager::orb_advertise_queue(meta, data, queue_size);
}

orb_advert_t orb_advertise_multi_queue(const struct orb_metadata *meta, const void *data,
				       int *instance, unsigned int queue_size)
{
	return uORB::Manager::orb_advertise_multi_queue(meta, data, instance, queue_size);
}

int orb_unadvertise(orb_advert_t handle)
{
	return uORB::Manager::orb_unadvertise(handle);
}

int orb_publish(const struct orb_metadata *meta, orb_advert_t handle, const void *data)
{
	return uORB::Manager::orb_publish(meta, handle, data);
}

int orb_exists(const struct orb_metadata *meta, int instance)
{
	return uORB::Manager::orb_exists(meta, instance);
}

/* ── fd-style handle slot 表（和 freertos 版结构一致）─────────────────── */

namespace
{

constexpr int MAX_HANDLES = CONFIG_UORB_SUBSCRIBE_HANDLE_MAX;

struct HandleSlot {
	bool              in_use{false};
	uORB::Subscription *sub{nullptr};
	uint32_t          interval_ms{0};
};

HandleSlot          g_slots[MAX_HANDLES];
struct k_mutex      g_slots_mutex = Z_MUTEX_INITIALIZER(g_slots_mutex);

int alloc_slot(uORB::Subscription *sub)
{
	k_mutex_lock(&g_slots_mutex, K_FOREVER);

	for (int i = 0; i < MAX_HANDLES; i++) {
		if (!g_slots[i].in_use) {
			g_slots[i].in_use      = true;
			g_slots[i].sub         = sub;
			g_slots[i].interval_ms = 0;
			k_mutex_unlock(&g_slots_mutex);
			return i;
		}
	}

	k_mutex_unlock(&g_slots_mutex);
	return -1;
}

HandleSlot *get_slot(int handle)
{
	if (handle < 0 || handle >= MAX_HANDLES) { return nullptr; }
	if (!g_slots[handle].in_use)             { return nullptr; }
	return &g_slots[handle];
}

void free_slot(int handle)
{
	HandleSlot *s = get_slot(handle);
	if (!s) { return; }
	k_mutex_lock(&g_slots_mutex, K_FOREVER);
	delete s->sub;
	s->sub    = nullptr;
	s->in_use = false;
	k_mutex_unlock(&g_slots_mutex);
}

} // anonymous namespace

int orb_subscribe(const struct orb_metadata *meta)
{
	return orb_subscribe_multi(meta, 0);
}

int orb_subscribe_multi(const struct orb_metadata *meta, unsigned instance)
{
	if (!meta || instance >= ORB_MULTI_MAX_INSTANCES) { return -1; }

	auto *sub = new uORB::Subscription(meta, (uint8_t)instance);
	if (!sub) { return -1; }

	int h = alloc_slot(sub);
	if (h < 0) { delete sub; return -1; }

	return h;
}

int orb_unsubscribe(int handle)
{
	if (!get_slot(handle)) { return -1; }
	free_slot(handle);
	return 0;
}

int orb_copy(const struct orb_metadata *meta, int handle, void *buffer)
{
	(void)meta;
	HandleSlot *s = get_slot(handle);
	if (!s || !buffer) { return -1; }
	return s->sub->copy(buffer) ? 0 : -1;
}

int orb_check(int handle, bool *updated)
{
	HandleSlot *s = get_slot(handle);
	if (!s || !updated) { return -1; }
	*updated = s->sub->updated();
	return 0;
}

int orb_set_interval(int handle, unsigned interval_ms)
{
	HandleSlot *s = get_slot(handle);
	if (!s) { return -1; }
	s->interval_ms = interval_ms;
	return 0;
}

int orb_get_interval(int handle, unsigned *interval_ms)
{
	HandleSlot *s = get_slot(handle);
	if (!s || !interval_ms) { return -1; }
	*interval_ms = s->interval_ms;
	return 0;
}

uint8_t orb_get_queue_size(const struct orb_metadata *meta)
{
	if (meta) {
		return meta->o_queue;
	}

	return 0;
}

const char *orb_get_c_type(unsigned char short_type)
{
	switch (short_type) {
	case 0x82: return "int8_t";
	case 0x83: return "int16_t";
	case 0x84: return "int32_t";
	case 0x85: return "int64_t";
	case 0x86: return "uint8_t";
	case 0x87: return "uint16_t";
	case 0x88: return "uint32_t";
	case 0x89: return "uint64_t";
	case 0x8a: return "float";
	case 0x8b: return "double";
	case 0x8c: return "bool";
	case 0x8d: return "char";
	}
	return nullptr;
}

void orb_print_message_internal(const orb_metadata *meta, const void *data, bool print_topic_name)
{
	if (print_topic_name) {
		PX4_INFO_RAW(" %s\n", meta->o_name);
	}

	const hrt_abstime now = hrt_absolute_time();
	hrt_abstime topic_timestamp = 0;

	const uint8_t *data_ptr = (const uint8_t *)data;
	int data_offset = 0;

	char format_buffer[128];
	uORB::MessageFormatReader format_reader(format_buffer, sizeof(format_buffer));

	if (!format_reader.readUntilFormat(meta->o_id)) {
		PX4_ERR("Failed to get uorb format");
		return;
	}

	int field_length = 0;

	while (format_reader.readNextField(field_length)) {
		const char *c_type = orb_get_c_type(format_buffer[0]);

		int array_idx = -1;
		int field_name_idx = -1;

		for (int field_idx = 0; field_idx < field_length; ++field_idx) {
			if (format_buffer[field_idx] == '[') {
				array_idx = field_idx + 1;
			} else if (format_buffer[field_idx] == ' ') {
				field_name_idx = field_idx + 1;
				break;
			}
		}

		int array_size = 1;
		if (array_idx >= 0) {
			array_size = strtol(format_buffer + array_idx, nullptr, 10);
		}

		const char *field_name = format_buffer + field_name_idx;

		if (c_type) {
			bool dont_print = (strncmp(field_name, "_padding", 8) == 0);

			if (!dont_print && strcmp(c_type, "char") == 0 && array_size > 1) {
				PX4_INFO_RAW("    %s: \"%.*s\"\n", field_name, array_size, (char *)(data_ptr + data_offset));
				data_offset += array_size;
				continue;
			}

			if (!dont_print) { PX4_INFO_RAW("    %s: ", field_name); }
			if (!dont_print && array_size > 1) { PX4_INFO_RAW("["); }

			const int previous_data_offset = data_offset;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
			for (int i = 0; i < array_size; ++i) {
				if (strcmp(c_type, "int8_t") == 0) {
					if (!dont_print) { PX4_INFO_RAW("%" PRIi8, *(int8_t *)(data_ptr + data_offset)); }
					data_offset += sizeof(int8_t);
				} else if (strcmp(c_type, "int16_t") == 0) {
					if (!dont_print) { PX4_INFO_RAW("%" PRIi16, *(int16_t *)(data_ptr + data_offset)); }
					data_offset += sizeof(int16_t);
				} else if (strcmp(c_type, "int32_t") == 0) {
					if (!dont_print) { PX4_INFO_RAW("%" PRIi32, *(int32_t *)(data_ptr + data_offset)); }
					data_offset += sizeof(int32_t);
				} else if (strcmp(c_type, "int64_t") == 0) {
					if (!dont_print) { PX4_INFO_RAW("%" PRIi64, *(int64_t *)(data_ptr + data_offset)); }
					data_offset += sizeof(int64_t);
				} else if (strcmp(c_type, "uint8_t") == 0) {
					if (!dont_print) { PX4_INFO_RAW("%" PRIu8, *(uint8_t *)(data_ptr + data_offset)); }
					data_offset += sizeof(uint8_t);
				} else if (strcmp(c_type, "uint16_t") == 0) {
					if (!dont_print) { PX4_INFO_RAW("%" PRIu16, *(uint16_t *)(data_ptr + data_offset)); }
					data_offset += sizeof(uint16_t);
				} else if (strcmp(c_type, "uint32_t") == 0) {
					if (!dont_print) { PX4_INFO_RAW("%" PRIu32, *(uint32_t *)(data_ptr + data_offset)); }
					data_offset += sizeof(uint32_t);
				} else if (strcmp(c_type, "uint64_t") == 0) {
					if (!dont_print) { PX4_INFO_RAW("%" PRIu64, *(uint64_t *)(data_ptr + data_offset)); }
					data_offset += sizeof(uint64_t);
				} else if (strcmp(c_type, "float") == 0) {
					if (!dont_print) { PX4_INFO_RAW("%.5f", (double)*(float *)(data_ptr + data_offset)); }
					data_offset += sizeof(float);
				} else if (strcmp(c_type, "double") == 0) {
					if (!dont_print) { PX4_INFO_RAW("%.6f", *(double *)(data_ptr + data_offset)); }
					data_offset += sizeof(double);
				} else if (strcmp(c_type, "bool") == 0) {
					if (!dont_print) { PX4_INFO_RAW("%s", *(bool *)(data_ptr + data_offset) ? "True" : "False"); }
					data_offset += sizeof(bool);
				} else if (strcmp(c_type, "char") == 0) {
					if (!dont_print) { PX4_INFO_RAW("%i", (int)*(char *)(data_ptr + data_offset)); }
					data_offset += sizeof(char);
				}

				if (!dont_print && i < array_size - 1) { PX4_INFO_RAW(", "); }
			}
#pragma GCC diagnostic pop

			if (!dont_print && array_size > 1) { PX4_INFO_RAW("]"); }

			if (array_size == 1 && strcmp(c_type, "uint64_t") == 0 && strcmp(field_name, "timestamp") == 0) {
				topic_timestamp = *(uint64_t *)(data_ptr + previous_data_offset);
				if (topic_timestamp != 0) {
					PX4_INFO_RAW(" (%.6f seconds ago)", (double)((now - topic_timestamp) / 1e6f));
				}
			}

			if (!dont_print) { PX4_INFO_RAW("\n"); }

		} else {
			const size_t topic_name_len = array_size > 1 ? array_idx - 1 : field_name_idx - 1;
			format_buffer[topic_name_len] = '\0';
			const char *topic_name = format_buffer;

			const orb_metadata *const *topics = orb_get_topics();
			const orb_metadata *found_topic = nullptr;

			for (size_t i = 0; i < orb_topics_count(); i++) {
				if (strcmp(topics[i]->o_name, topic_name) == 0) {
					found_topic = topics[i];
					break;
				}
			}

			if (!found_topic) {
				PX4_ERR("Topic %s did not match any known topics", topic_name);
				return;
			}

			for (int i = 0; i < array_size; ++i) {
				PX4_INFO_RAW("  %s", field_name);
				if (array_size > 1) { PX4_INFO_RAW("[%i]", i); }
				PX4_INFO_RAW(" (%s):\n", topic_name);
				orb_print_message_internal(found_topic, data_ptr + data_offset, false);
				data_offset += found_topic->o_size;
			}
		}
	}
}
