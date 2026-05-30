#include "sub_blocking.h"
#include "task_register.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sub_blocking, LOG_LEVEL_INF);

void SubBlocking::run()
{
	/* 等待 advertise 完成后再注册回调，避免时序问题 */
	while (!_sub.registerCallback()) {
		k_msleep(1);
	}
	LOG_INF("run");

	sensor_accel_s accel{};

	while (true) {
		if (_sub.updateBlocking(accel, 1000U)) {
			// LOG_INF("t=%llu x=%.3f y=%.3f z=%.3f",
			// 	accel.timestamp,
			// 	(double)accel.x, (double)accel.y, (double)accel.z);
		} else {
			// LOG_WRN("timeout (no accel for 1 s)");
		}
	}
}

RTFRAME_TASK_REGISTER(SubBlocking, vwork::configs::sub, INIT_LEVEL_APP, 0);
