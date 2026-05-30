#pragma once

#include "vwork.h"
#include "task_register.h"

#include <uORB/SubscriptionCallback.hpp>
#include <uORB/topics/vehicle_attitude.h>

class SubCallback : public vwork::Event
{
public:
	SubCallback() : Event(vwork::configs::sensor),
	                _sub_cb(work_ptr(), ORB_ID(vehicle_attitude)) {}

protected:
	void init() override;
	void callback() override;

private:
	uORB::SubscriptionCallbackWorkItem _sub_cb;
};
