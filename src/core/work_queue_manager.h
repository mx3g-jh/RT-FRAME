#pragma once

#include <zephyr/kernel.h>
#include "vwork_config.h"

namespace vwork
{

/*
 * 按 config 懒创建并复用 Zephyr work queue。
 * 同名 config 返回同一个 k_work_q（共享线程）。
 */
struct k_work_q *work_queue_find_or_create(const config_t &cfg);

} // namespace vwork
