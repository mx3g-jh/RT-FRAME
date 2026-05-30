#pragma once

#include <zephyr/zbus/zbus.h>

struct rtframe_msg {
    uint32_t seq;
    int32_t  value;
};

ZBUS_CHAN_DECLARE(rtframe_demo_chan);
