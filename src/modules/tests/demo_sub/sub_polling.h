#pragma once

#include "vwork.h"
#include "hrt.h"

#include <uORB/Subscription.hpp>
#include <uORB/topics/sensor_accel.h>

class SubPolling : public vwork::Periodic
{
public:
	SubPolling() : Periodic(vwork::configs::sensor) {}

protected:
	void init() override;
	void callback() override;

private:
	uORB::Subscription _sub0{ORB_ID(sensor_accel), 0};
	uORB::Subscription _sub1{ORB_ID(sensor_accel), 1};
};
