#include "sub_polling.h"
#include "task_register.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sub_polling, LOG_LEVEL_INF);

void SubPolling::init()
{
	LOG_INF("init");
}

void SubPolling::callback()
{
	sensor_accel_s accel{};

	if (_sub0.update(&accel)) {
		// LOG_INF("inst0 t=%llu x=%.3f y=%.3f z=%.3f",
		// 	accel.timestamp,
		// 	(double)accel.x, (double)accel.y, (double)accel.z);
	}

	if (_sub1.update(&accel)) {
		// LOG_INF("inst1 t=%llu x=%.3f y=%.3f z=%.3f",
		// 	accel.timestamp,
		// 	(double)accel.x, (double)accel.y, (double)accel.z);
	}
}

RTFRAME_TASK_REGISTER(SubPolling, vwork::configs::sensor, INIT_LEVEL_APP, 1000_hz);
