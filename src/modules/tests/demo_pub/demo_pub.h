#pragma once

#include "vwork.h"
#include "hrt.h"

#include <uORB/Publication.hpp>
#include <uORB/PublicationMulti.hpp>
#include <uORB/topics/sensor_accel.h>
#include <uORB/topics/vehicle_attitude.h>

class DemoPub : public vwork::Thread
{
public:
	DemoPub() : Thread(vwork::configs::pub) {}

protected:
	void init() override;
	void callback() override;

private:
	uint32_t _seq{0};

	uORB::Publication<sensor_accel_s>         _pub_accel{ORB_ID(sensor_accel)};
	uORB::PublicationMulti<sensor_accel_s>    _pub_accel1{ORB_ID(sensor_accel)};
	uORB::PublicationData<vehicle_attitude_s> _pub_att{ORB_ID(vehicle_attitude)};
};
