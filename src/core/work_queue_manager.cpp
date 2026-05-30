#include <zephyr/kernel.h>
#include <cstring>
#include "work_queue_manager.h"

namespace vwork
{

namespace
{

constexpr int MAX_QUEUES = 8;

struct queue_slot {
	const char     *name;
	struct k_work_q wq;
	bool            used;
};

queue_slot g_slots[MAX_QUEUES];
struct k_mutex g_lock;
bool g_lock_inited;

void ensure_lock()
{
	if (!g_lock_inited) {
		k_mutex_init(&g_lock);
		g_lock_inited = true;
	}
}

} // namespace

struct k_work_q *work_queue_find_or_create(const config_t &cfg)
{
	ensure_lock();
	k_mutex_lock(&g_lock, K_FOREVER);

	/* 复用同名队列 */
	for (int i = 0; i < MAX_QUEUES; ++i) {
		if (g_slots[i].used && strcmp(g_slots[i].name, cfg.name) == 0) {
			k_mutex_unlock(&g_lock);
			return &g_slots[i].wq;
		}
	}

	/* 找空槽创建 —— 栈由 config 携带（编译期静态分配） */
	for (int i = 0; i < MAX_QUEUES; ++i) {
		if (!g_slots[i].used) {
			g_slots[i].used = true;
			g_slots[i].name = cfg.name;

			struct k_work_queue_config qcfg = {
				.name = cfg.name,
				.no_yield = false,
			};

			k_work_queue_start(&g_slots[i].wq,
					   cfg.stack,
					   cfg.stacksize,
					   cfg.priority,
					   &qcfg);

			k_mutex_unlock(&g_lock);
			return &g_slots[i].wq;
		}
	}

	k_mutex_unlock(&g_lock);
	return nullptr;
}

} // namespace vwork
