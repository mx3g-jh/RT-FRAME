#include "rtframe_channels.h"
#include <zephyr/zbus/zbus.h>

ZBUS_SUBSCRIBER_DEFINE(demo_sub, 4);

ZBUS_CHAN_DEFINE(rtframe_demo_chan,
                struct rtframe_msg,
                NULL,
                NULL,
                ZBUS_OBSERVERS(demo_sub),
                ZBUS_MSG_INIT(.seq = 0, .value = 0));
