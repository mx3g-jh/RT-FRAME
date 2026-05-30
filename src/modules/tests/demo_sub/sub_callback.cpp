#include "sub_callback.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sub_callback, LOG_LEVEL_INF);

void SubCallback::init()
{
	LOG_INF("init");
	_sub_cb.registerCallback();
}

void SubCallback::callback()
{
	vehicle_attitude_s att{};
	while (_sub_cb.update(&att)) {
		// LOG_INF("att t=%llu q0=%.4f q3=%.4f",
		// 	att.timestamp,
		// 	(double)att.q[0], (double)att.q[3]);
	}
}

RTFRAME_TASK_REGISTER(SubCallback, vwork::configs::sensor, INIT_LEVEL_APP, 0);
