/**
 * @file log.h
 * PX4 log macros — Zephyr port (printk backend).
 */

#pragma once

#include <zephyr/sys/printk.h>

#define _PX4_LOG_LEVEL_DEBUG    0
#define _PX4_LOG_LEVEL_INFO     1
#define _PX4_LOG_LEVEL_WARN     2
#define _PX4_LOG_LEVEL_ERROR    3
#define _PX4_LOG_LEVEL_PANIC    4

#ifndef MODULE_NAME
#define MODULE_NAME "px4"
#endif

#ifndef __EXPORT
#define __EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

__EXPORT extern const char *__px4_log_level_str[_PX4_LOG_LEVEL_PANIC + 1];
__EXPORT void px4_log_modulename(int level, const char *module_name, const char *fmt, ...)
	__attribute__((format(printf, 3, 4)));
__EXPORT void px4_log_raw(int level, const char *fmt, ...)
	__attribute__((format(printf, 2, 3)));
__EXPORT void px4_log_initialize(void);

#ifdef __cplusplus
}
#endif

#define __px4_log_omit(level, FMT, ...)   ((void)(level))

#define __px4_log_modulename(level, fmt, ...) \
	px4_log_modulename(level, MODULE_NAME, fmt, ##__VA_ARGS__)

#define __px4_log_raw(level, fmt, ...) \
	px4_log_raw(level, fmt, ##__VA_ARGS__)

#define PX4_INFO(FMT, ...)      __px4_log_modulename(_PX4_LOG_LEVEL_INFO,  FMT, ##__VA_ARGS__)
#define PX4_INFO_RAW(FMT, ...)  __px4_log_raw(_PX4_LOG_LEVEL_INFO,         FMT, ##__VA_ARGS__)
#define PX4_WARN(FMT, ...)      __px4_log_modulename(_PX4_LOG_LEVEL_WARN,  FMT, ##__VA_ARGS__)
#define PX4_ERR(FMT, ...)       __px4_log_modulename(_PX4_LOG_LEVEL_ERROR, FMT, ##__VA_ARGS__)
#define PX4_DEBUG(FMT, ...)     __px4_log_omit(_PX4_LOG_LEVEL_DEBUG,       FMT, ##__VA_ARGS__)
#define PX4_PANIC(FMT, ...)     __px4_log_modulename(_PX4_LOG_LEVEL_PANIC, FMT, ##__VA_ARGS__)

#define PX4_LOG_NAMED(name, FMT, ...)       px4_log_raw(_PX4_LOG_LEVEL_INFO, "[%s] " FMT "\n", name, ##__VA_ARGS__)
#define PX4_LOG_NAMED_COND(name, cond, FMT, ...) \
	do { if (cond) px4_log_raw(_PX4_LOG_LEVEL_INFO, "[%s] " FMT "\n", name, ##__VA_ARGS__); } while(0)

#define PX4_ANSI_COLOR_RED     "\x1b[31m"
#define PX4_ANSI_COLOR_GREEN   "\x1b[32m"
#define PX4_ANSI_COLOR_YELLOW  "\x1b[33m"
#define PX4_ANSI_COLOR_BLUE    "\x1b[34m"
#define PX4_ANSI_COLOR_MAGENTA "\x1b[35m"
#define PX4_ANSI_COLOR_CYAN    "\x1b[36m"
#define PX4_ANSI_COLOR_GRAY    "\x1B[37m"
#define PX4_ANSI_COLOR_RESET   "\x1b[0m"
