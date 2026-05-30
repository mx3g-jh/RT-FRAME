/**
 * px4_log.cpp — Zephyr port (Zephyr logging subsystem backend).
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(px4, LOG_LEVEL_DBG);

#ifndef MODULE_NAME
#define MODULE_NAME "log"
#endif

#include "px4_log.h"
#include "hrt.h"
#include <uORB/uORB.h>
#include <uORB/topics/log_message.h>

__EXPORT const char *__px4_log_level_str[_PX4_LOG_LEVEL_PANIC + 1] = {
	"DEBUG", "INFO", "WARN", "ERROR", "PANIC"
};

static orb_advert_t s_log_pub = nullptr;

static void try_advertise()
{
	if (s_log_pub == nullptr) {
		log_message_s seed{};
		seed.timestamp = hrt_absolute_time();
		seed.severity  = 6;
		strncpy((char *)seed.text, "log init", sizeof(seed.text) - 1);
		s_log_pub = orb_advertise(ORB_ID(log_message), &seed);
	}
}

static void publish_uorb(int level, const char *text, size_t text_len)
{
	try_advertise();
	if (s_log_pub == nullptr) { return; }

	log_message_s msg{};
	msg.timestamp = hrt_absolute_time();
	static const uint8_t severity_map[] = { 7, 6, 4, 3, 2 };
	msg.severity = (level >= 0 && level <= _PX4_LOG_LEVEL_PANIC)
		       ? severity_map[level] : 6;
	const size_t copy_len = (text_len < sizeof(msg.text) - 1)
				? text_len : sizeof(msg.text) - 1;
	memcpy(msg.text, text, copy_len);
	msg.text[copy_len] = 0;
	orb_publish(ORB_ID(log_message), s_log_pub, &msg);
}

__EXPORT void px4_log_modulename(int level, const char *module_name, const char *fmt, ...)
{
	char buf[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	switch (level) {
	case _PX4_LOG_LEVEL_DEBUG: LOG_DBG("[%s] %s", module_name, buf); break;
	case _PX4_LOG_LEVEL_INFO:  LOG_INF("[%s] %s", module_name, buf); break;
	case _PX4_LOG_LEVEL_WARN:  LOG_WRN("[%s] %s", module_name, buf); break;
	case _PX4_LOG_LEVEL_ERROR: LOG_ERR("[%s] %s", module_name, buf); break;
	case _PX4_LOG_LEVEL_PANIC: LOG_ERR("[%s] PANIC: %s", module_name, buf); break;
	default:                   LOG_INF("[%s] %s", module_name, buf); break;
	}

	if (level >= _PX4_LOG_LEVEL_INFO) {
		publish_uorb(level, buf, strlen(buf));
	}
}

__EXPORT void px4_log_raw(int level, const char *fmt, ...)
{
	(void)level;
	char buf[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	LOG_INF("%s", buf);
}

__EXPORT void px4_log_initialize(void)
{
	try_advertise();
}
