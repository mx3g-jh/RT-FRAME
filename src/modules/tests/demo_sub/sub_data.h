#pragma once

#include "vwork.h"
#include "hrt.h"

#include <uORB/Subscription.hpp>
#include <uORB/topics/vehicle_attitude.h>

class SubData : public vwork::Periodic
{
public:
	SubData() : Periodic(vwork::configs::sensor) {}

protected:
	void init() override;
	void callback() override;

private:
	uORB::SubscriptionData<vehicle_attitude_s> _sub_att{ORB_ID(vehicle_attitude)};
};
