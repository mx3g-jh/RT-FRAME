#pragma once

#include <zephyr/kernel.h>
#include "vwork.h"

using namespace freq_literals;

/* ============================================================
 *  任务启动分级（init_level）
 *
 *  task_init 按 level 从低到高排序启动。
 *  同级内顺序不保证 —— 同级任务间若有依赖，应分到不同 level，
 *  或用消息总线的数据就绪状态门控（推荐）。
 * ============================================================ */
enum rtframe_init_level : uint8_t {
	INIT_LEVEL_DRIVER   = 0,   /* 驱动：传感器、执行器 */
	INIT_LEVEL_ESTIMATE = 1,   /* 估计器：依赖传感器数据 */
	INIT_LEVEL_CONTROL  = 2,   /* 控制器：依赖估计器 */
	INIT_LEVEL_APP      = 3,   /* 应用层 */
};

/*
 * 注册表项。start_fn 是类型擦除的启动 thunk，
 * 其余字段供汇总查看（rtframe task）。
 */
struct task_entry {
	bool          (*start_fn)(void);
	const char     *name;
	const char     *cfg_name;   /* 所属 config 坑位名 */
	uint32_t        period_us;
	uint8_t         init_level;
	vwork::Model    model;
	uint16_t        stacksize;
	int8_t          priority;
};

/*
 * 统一注册宏。模型由 config 坑位决定（config.model），
 * 任务基类（Periodic/Thread/Event）的 MODEL 必须与之匹配，否则编译报错。
 *
 *   _type   —— 任务类（继承 Periodic/Thread/Event 之一）
 *   _cfg    —— config 坑位（vwork::configs::xxx）
 *   _level  —— INIT_LEVEL_*
 *   _period —— 周期（微秒）。Thread/Event 仅作记录，可填 0。
 *
 * 用法：
 *   RTFRAME_TASK_REGISTER(DemoPub, vwork::configs::sensor, INIT_LEVEL_APP, 500000);
 */
#define RTFRAME_TASK_REGISTER(_type, _cfg, _level, _period)                  \
	static _type _rtf_##_type;                                           \
	static_assert(_type::MODEL == (_cfg).model,                          \
		      #_type " base class model does not match config '"     \
		      #_cfg "' model");                                      \
	static bool _rtf_##_type##_start(void) { return _rtf_##_type.start(_period); } \
	static const STRUCT_SECTION_ITERABLE(task_entry, _rtf_##_type##_entry) = { \
		.start_fn   = _rtf_##_type##_start,                          \
		.name       = #_type,                                        \
		.cfg_name   = (_cfg).name,                                   \
		.period_us  = (_period),                                     \
		.init_level = (_level),                                      \
		.model      = (_cfg).model,                                  \
		.stacksize  = (_cfg).stacksize,                              \
		.priority   = (_cfg).priority,                               \
	}
