#include "test_perf.h"
#include <zephyr/logging/log.h>
#include "perf_counter.h"
#include "hrt.h"

LOG_MODULE_REGISTER(test_perf, LOG_LEVEL_INF);

static perf_counter_t s_count    = nullptr;
static perf_counter_t s_elapsed  = nullptr;
static perf_counter_t s_interval = nullptr;

void TestPerf::init()
{
	s_count    = perf_alloc(PC_COUNT,    "test_count");
	s_elapsed  = perf_alloc(PC_ELAPSED,  "test_elapsed");
	s_interval = perf_alloc(PC_INTERVAL, "test_interval");
	LOG_INF("init");
}

void TestPerf::callback()
{
	/* PC_COUNT */
	perf_count(s_count);

	/* PC_ELAPSED: measure a small busy loop */
	perf_begin(s_elapsed);
	volatile uint32_t x = 0;
	for (int i = 0; i < 1000; i++) { x += i; }
	(void)x;
	perf_end(s_elapsed);

	/* PC_INTERVAL: record each callback invocation */
	perf_count(s_interval);

	/* Print all counters every callback */
	perf_print_all();
}

RTFRAME_TASK_REGISTER(TestPerf, vwork::configs::sensor, INIT_LEVEL_APP, 1_hz);
