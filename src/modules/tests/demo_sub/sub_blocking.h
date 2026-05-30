#pragma once

#include "vwork.h"

#include <uORB/SubscriptionBlocking.hpp>
#include <uORB/topics/sensor_accel.h>

class SubBlocking : public vwork::Thread
{
public:
	SubBlocking() : Thread(vwork::configs::sub) {}

protected:
	void run() override;

private:
	uORB::SubscriptionBlocking<sensor_accel_s> _sub{ORB_ID(sensor_accel), 0, 0};
};
