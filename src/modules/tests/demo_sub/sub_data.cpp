#include "sub_data.h"
#include "task_register.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sub_data, LOG_LEVEL_INF);

void SubData::init()
{
	LOG_INF("init");
}

void SubData::callback()
{
	if (_sub_att.update()) {
		// const vehicle_attitude_s &att = _sub_att.get();
		// LOG_INF("att t=%llu q=(%.3f,%.3f,%.3f,%.3f)",
		// 	att.timestamp,
		// 	(double)att.q[0], (double)att.q[1],
		// 	(double)att.q[2], (double)att.q[3]);
	}
}

RTFRAME_TASK_REGISTER(SubData, vwork::configs::sensor, INIT_LEVEL_APP, 1000_hz);
