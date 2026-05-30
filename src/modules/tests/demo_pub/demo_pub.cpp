#include "demo_pub.h"
#include "task_register.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(demo_pub, LOG_LEVEL_INF);

void DemoPub::init()
{
	LOG_INF("thread start");
}

void DemoPub::callback()
{
	hrt_abstime now = hrt_absolute_time();

	// LOG_INF("callback t=%llu", now);
	_seq++;

	sensor_accel_s accel{};
	accel.timestamp        = now;
	accel.timestamp_sample = now;
	accel.x = 0.1f * static_cast<float>(_seq);
	accel.y = 0.2f * static_cast<float>(_seq);
	accel.z = 9.81f;
	accel.temperature = 25.0f;
	_pub_accel.publish(accel);

	/* sensor_accel instance 1（multi-instance） */
	sensor_accel_s accel1 = accel;
	accel1.x = -accel.x;
	accel1.y = -accel.y;
	_pub_accel1.publish(accel1);

	/* vehicle_attitude（PublicationData 内嵌写法） */
	vehicle_attitude_s &att = _pub_att.get();
	att.timestamp        = now;
	att.timestamp_sample = now;
	float angle = 0.001f * static_cast<float>(_seq);
	att.q[0] = 1.0f;
	att.q[1] = 0.0f;
	att.q[2] = 0.0f;
	att.q[3] = angle;
	_pub_att.update();
}

RTFRAME_TASK_REGISTER(DemoPub, vwork::configs::pub, INIT_LEVEL_APP, 1000_hz);
