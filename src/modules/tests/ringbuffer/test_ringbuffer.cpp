#include "test_ringbuffer.h"
#include <zephyr/logging/log.h>
#include "Ringbuffer.hpp"

LOG_MODULE_REGISTER(test_ringbuffer, LOG_LEVEL_INF);

static Ringbuffer     s_rb;
static uint8_t        s_rb_buf[256];

void TestRingbuffer::init()
{
	s_rb.init_static(s_rb_buf, sizeof(s_rb_buf));

	LOG_INF("init: rb cap=%u",(unsigned)s_rb.capacity());
}

void TestRingbuffer::callback()
{
	/* ── SPSC Ringbuffer ─────────────────────────────────────────────── */
	const char *msg = "hello ringbuffer";
	const size_t len = strlen(msg);

	bool ok = s_rb.push_back((const uint8_t *)msg, len);
	LOG_INF("spsc push %u bytes: %s  used=%u",
		(unsigned)len, ok ? "ok" : "FAIL",
		(unsigned)s_rb.space_used());

	uint8_t out[64] = {};
	size_t got = s_rb.pop_front(out, sizeof(out));
	LOG_INF("spsc pop  %u bytes: \"%.*s\"", (unsigned)got, (int)got, out);
}

// RTFRAME_TASK_REGISTER(TestRingbuffer, vwork::configs::sensor, INIT_LEVEL_APP, 1_hz);
