/**
 * perf_counter.cpp — Zephyr port.
 *
 * Counter math unchanged from PX4 source.
 * Lock layer: FreeRTOS recursive mutex → k_mutex (Zephyr mutex supports recursion).
 * List: NuttX sq_queue → sys_slist (Zephyr intrusive singly-linked list).
 */

#include "perf_counter.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>
#include <zephyr/logging/log.h>

#include "hrt.h"
#include <log/px4_log.h>

LOG_MODULE_REGISTER(perf, LOG_LEVEL_INF);

/* ------------------------------------------------------------------------ */
/* Per-counter records                                                      */
/* ------------------------------------------------------------------------ */

struct perf_ctr_header {
	sys_snode_t           node;
	enum perf_counter_type type;
	const char            *name;
};

struct perf_ctr_count : public perf_ctr_header {
	uint64_t event_count{0};
};

struct perf_ctr_elapsed : public perf_ctr_header {
	uint64_t event_count{0};
	uint64_t time_start{0};
	uint64_t time_total{0};
	uint32_t time_least{0};
	uint32_t time_most{0};
	float    mean{0.0f};
	float    M2{0.0f};
};

struct perf_ctr_interval : public perf_ctr_header {
	uint64_t event_count{0};
	uint64_t time_event{0};
	uint64_t time_first{0};
	uint64_t time_last{0};
	uint32_t time_least{0};
	uint32_t time_most{0};
	float    mean{0.0f};
	float    M2{0.0f};
};

/* ------------------------------------------------------------------------ */
/* Global registry + lock                                                   */
/* ------------------------------------------------------------------------ */

static sys_slist_t s_perf_counters = SYS_SLIST_STATIC_INIT(&s_perf_counters);
static struct k_mutex s_perf_lock;
static bool s_lock_inited = false;

static void perf_lock_init()
{
	if (!s_lock_inited) {
		k_mutex_init(&s_perf_lock);
		s_lock_inited = true;
	}
}

static inline void perf_lock_take()
{
	perf_lock_init();
	k_mutex_lock(&s_perf_lock, K_FOREVER);
}

static inline void perf_lock_give()
{
	k_mutex_unlock(&s_perf_lock);
}

/* ------------------------------------------------------------------------ */
/* Allocation                                                               */
/* ------------------------------------------------------------------------ */

perf_counter_t perf_alloc(enum perf_counter_type type, const char *name)
{
	perf_counter_t ctr = nullptr;

	switch (type) {
	case PC_COUNT:    ctr = new perf_ctr_count();    break;
	case PC_ELAPSED:  ctr = new perf_ctr_elapsed();  break;
	case PC_INTERVAL: ctr = new perf_ctr_interval(); break;
	default: break;
	}

	if (ctr != nullptr) {
		ctr->type = type;
		ctr->name = name;
		perf_lock_take();
		sys_slist_prepend(&s_perf_counters, &ctr->node);
		perf_lock_give();
	}

	return ctr;
}

perf_counter_t perf_alloc_once(enum perf_counter_type type, const char *name)
{
	perf_lock_take();

	sys_snode_t *node;
	SYS_SLIST_FOR_EACH_NODE(&s_perf_counters, node) {
		perf_counter_t handle = CONTAINER_OF(node, struct perf_ctr_header, node);
		if (strcmp(handle->name, name) == 0) {
			perf_lock_give();
			return (type == handle->type) ? handle : nullptr;
		}
	}

	perf_lock_give();
	return perf_alloc(type, name);
}

void perf_free(perf_counter_t handle)
{
	if (handle == nullptr) { return; }

	perf_lock_take();
	sys_slist_find_and_remove(&s_perf_counters, &handle->node);
	perf_lock_give();

	switch (handle->type) {
	case PC_COUNT:    delete (struct perf_ctr_count *)handle;    break;
	case PC_ELAPSED:  delete (struct perf_ctr_elapsed *)handle;  break;
	case PC_INTERVAL: delete (struct perf_ctr_interval *)handle; break;
	default: break;
	}
}

/* ------------------------------------------------------------------------ */
/* Counter operations                                                       */
/* ------------------------------------------------------------------------ */

void perf_count(perf_counter_t handle)
{
	if (handle == nullptr) { return; }

	switch (handle->type) {
	case PC_COUNT:
		((struct perf_ctr_count *)handle)->event_count++;
		break;
	case PC_INTERVAL:
		perf_count_interval(handle, hrt_absolute_time());
		break;
	default: break;
	}
}

void perf_begin(perf_counter_t handle)
{
	if (handle == nullptr) { return; }

	if (handle->type == PC_ELAPSED) {
		((struct perf_ctr_elapsed *)handle)->time_start = hrt_absolute_time();
	}
}

void perf_end(perf_counter_t handle)
{
	if (handle == nullptr) { return; }

	if (handle->type == PC_ELAPSED) {
		struct perf_ctr_elapsed *pce = (struct perf_ctr_elapsed *)handle;
		if (pce->time_start != 0) {
			perf_set_elapsed(handle, hrt_elapsed_time(&pce->time_start));
		}
	}
}

void perf_set_elapsed(perf_counter_t handle, int64_t elapsed)
{
	if (handle == nullptr || handle->type != PC_ELAPSED) { return; }

	struct perf_ctr_elapsed *pce = (struct perf_ctr_elapsed *)handle;

	if (elapsed >= 0) {
		pce->event_count++;
		pce->time_total += elapsed;

		if ((pce->time_least > (uint32_t)elapsed) || (pce->time_least == 0)) {
			pce->time_least = (uint32_t)elapsed;
		}
		if (pce->time_most < (uint32_t)elapsed) {
			pce->time_most = (uint32_t)elapsed;
		}

		const float dt          = elapsed / 1e6f;
		const float delta_intvl = dt - pce->mean;
		pce->mean += delta_intvl / pce->event_count;
		pce->M2   += delta_intvl * (dt - pce->mean);
		pce->time_start = 0;
	}
}

void perf_count_interval(perf_counter_t handle, hrt_abstime now)
{
	if (handle == nullptr || handle->type != PC_INTERVAL) { return; }

	struct perf_ctr_interval *pci = (struct perf_ctr_interval *)handle;

	switch (pci->event_count) {
	case 0:
		pci->time_first = now;
		break;
	case 1:
		pci->time_least = (uint32_t)(now - pci->time_last);
		pci->time_most  = (uint32_t)(now - pci->time_last);
		pci->mean       = pci->time_least / 1e6f;
		pci->M2         = 0;
		break;
	default: {
		const hrt_abstime interval = now - pci->time_last;
		if ((uint32_t)interval < pci->time_least) { pci->time_least = (uint32_t)interval; }
		if ((uint32_t)interval > pci->time_most)  { pci->time_most  = (uint32_t)interval; }
		const float dt          = interval / 1e6f;
		const float delta_intvl = dt - pci->mean;
		pci->mean += delta_intvl / pci->event_count;
		pci->M2   += delta_intvl * (dt - pci->mean);
		break;
	}
	}

	pci->time_last = now;
	pci->event_count++;
}

void perf_set_count(perf_counter_t handle, uint64_t count)
{
	if (handle == nullptr || handle->type != PC_COUNT) { return; }
	((struct perf_ctr_count *)handle)->event_count = count;
}

void perf_cancel(perf_counter_t handle)
{
	if (handle == nullptr || handle->type != PC_ELAPSED) { return; }
	((struct perf_ctr_elapsed *)handle)->time_start = 0;
}

void perf_reset(perf_counter_t handle)
{
	if (handle == nullptr) { return; }

	switch (handle->type) {
	case PC_COUNT:
		((struct perf_ctr_count *)handle)->event_count = 0;
		break;
	case PC_ELAPSED: {
		struct perf_ctr_elapsed *pce = (struct perf_ctr_elapsed *)handle;
		pce->event_count = pce->time_start = pce->time_total = 0;
		pce->time_least  = pce->time_most  = 0;
		pce->mean = pce->M2 = 0.0f;
		break;
	}
	case PC_INTERVAL: {
		struct perf_ctr_interval *pci = (struct perf_ctr_interval *)handle;
		pci->event_count = pci->time_event = pci->time_first = pci->time_last = 0;
		pci->time_least  = pci->time_most  = 0;
		pci->mean = pci->M2 = 0.0f;
		break;
	}
	}
}

/* ------------------------------------------------------------------------ */
/* Printing                                                                 */
/* ------------------------------------------------------------------------ */

void perf_print_counter(perf_counter_t handle)
{
	if (handle == nullptr) { return; }

	switch (handle->type) {
	case PC_COUNT:
		PX4_INFO_RAW("%s: %lu events\n",
			handle->name,
			(unsigned long)((struct perf_ctr_count *)handle)->event_count);
		break;
	case PC_ELAPSED: {
		struct perf_ctr_elapsed *pce = (struct perf_ctr_elapsed *)handle;
		const float rms = (pce->event_count > 1)
				  ? sqrtf(pce->M2 / (pce->event_count - 1)) : 0.0f;
		PX4_INFO_RAW("%s: %lu events, %lu us total, %.2f us avg, "
			"min %lu us max %lu us %5.3f us rms\n",
			handle->name,
			(unsigned long)pce->event_count,
			(unsigned long)pce->time_total,
			(pce->event_count == 0) ? 0.0 : (double)pce->time_total / (double)pce->event_count,
			(unsigned long)pce->time_least,
			(unsigned long)pce->time_most,
			(double)(1e6f * rms));
		break;
	}
	case PC_INTERVAL: {
		struct perf_ctr_interval *pci = (struct perf_ctr_interval *)handle;
		const float rms = (pci->event_count > 1)
				  ? sqrtf(pci->M2 / (pci->event_count - 1)) : 0.0f;
		PX4_INFO_RAW("%s: %lu events, %.2f us avg, min %lu us max %lu us %5.3f us rms\n",
			handle->name,
			(unsigned long)pci->event_count,
			(pci->event_count == 0) ? 0.0
			: (double)(pci->time_last - pci->time_first) / (double)pci->event_count,
			(unsigned long)pci->time_least,
			(unsigned long)pci->time_most,
			(double)(1e6f * rms));
		break;
	}
	default: break;
	}
}

int perf_print_counter_buffer(char *buffer, int length, perf_counter_t handle)
{
	if (handle == nullptr) { return 0; }
	int n = 0;

	switch (handle->type) {
	case PC_COUNT:
		n = snprintf(buffer, length, "%s: %lu events",
			handle->name,
			(unsigned long)((struct perf_ctr_count *)handle)->event_count);
		break;
	case PC_ELAPSED: {
		struct perf_ctr_elapsed *pce = (struct perf_ctr_elapsed *)handle;
		const float rms = (pce->event_count > 1)
				  ? sqrtf(pce->M2 / (pce->event_count - 1)) : 0.0f;
		n = snprintf(buffer, length,
			"%s: %lu events, %lu us total, %.2f us avg, "
			"min %lu us max %lu us %5.3f us rms",
			handle->name,
			(unsigned long)pce->event_count,
			(unsigned long)pce->time_total,
			(pce->event_count == 0) ? 0.0 : (double)pce->time_total / (double)pce->event_count,
			(unsigned long)pce->time_least,
			(unsigned long)pce->time_most,
			(double)(1e6f * rms));
		break;
	}
	case PC_INTERVAL: {
		struct perf_ctr_interval *pci = (struct perf_ctr_interval *)handle;
		const float rms = (pci->event_count > 1)
				  ? sqrtf(pci->M2 / (pci->event_count - 1)) : 0.0f;
		n = snprintf(buffer, length,
			"%s: %lu events, %.2f avg, min %lu us max %lu us %5.3f us rms",
			handle->name,
			(unsigned long)pci->event_count,
			(pci->event_count == 0) ? 0.0
			: (double)(pci->time_last - pci->time_first) / (double)pci->event_count,
			(unsigned long)pci->time_least,
			(unsigned long)pci->time_most,
			(double)(1e6f * rms));
		break;
	}
	default: break;
	}

	if (length > 0) { buffer[length - 1] = 0; }
	return n;
}

uint64_t perf_event_count(perf_counter_t handle)
{
	if (handle == nullptr) { return 0; }

	switch (handle->type) {
	case PC_COUNT:    return ((struct perf_ctr_count *)handle)->event_count;
	case PC_ELAPSED:  return ((struct perf_ctr_elapsed *)handle)->event_count;
	case PC_INTERVAL: return ((struct perf_ctr_interval *)handle)->event_count;
	default: break;
	}
	return 0;
}

float perf_mean(perf_counter_t handle)
{
	if (handle == nullptr) { return 0.0f; }

	switch (handle->type) {
	case PC_ELAPSED:  return ((struct perf_ctr_elapsed *)handle)->mean;
	case PC_INTERVAL: return ((struct perf_ctr_interval *)handle)->mean;
	default: break;
	}
	return 0.0f;
}

void perf_iterate_all(perf_callback cb, void *user)
{
	perf_lock_take();
	sys_snode_t *node;
	SYS_SLIST_FOR_EACH_NODE(&s_perf_counters, node) {
		cb(CONTAINER_OF(node, struct perf_ctr_header, node), user);
	}
	perf_lock_give();
}

void perf_print_all(void)
{
	perf_lock_take();
	sys_snode_t *node;
	SYS_SLIST_FOR_EACH_NODE(&s_perf_counters, node) {
		perf_print_counter(CONTAINER_OF(node, struct perf_ctr_header, node));
	}
	perf_lock_give();
}

void perf_reset_all(void)
{
	perf_lock_take();
	sys_snode_t *node;
	SYS_SLIST_FOR_EACH_NODE(&s_perf_counters, node) {
		perf_reset(CONTAINER_OF(node, struct perf_ctr_header, node));
	}
	perf_lock_give();
}
