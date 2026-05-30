#pragma once

#include "vwork.h"
#include "task_register.h"

class TestPerf : public vwork::Periodic
{
public:
	TestPerf() : Periodic(vwork::configs::sensor) {}

protected:
	void init() override;
	void callback() override;
};
