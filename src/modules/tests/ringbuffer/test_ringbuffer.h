#pragma once

#include "vwork.h"
#include "task_register.h"

class TestRingbuffer : public vwork::Periodic
{
public:
	TestRingbuffer() : Periodic(vwork::configs::sensor) {}

protected:
	void init() override;
	void callback() override;
};
