#include <zephyr/sys/printk.h>
#include <zephyr/zbus/zbus.h>
#include "vwork.h"
#include "hrt.h"
#include "task_register.h"
#include "rtframe_channels.h"

/* 周期模型示例：每 500ms 发布一次 zbus 消息。 */
class DemoPub : public vwork::Periodic
{
public:
	DemoPub() : Periodic(vwork::configs::sensor) {}

protected:
	void init() override
	{
		printk("[pub] init\n");
		_last_us = hrt_absolute_time();
	}

	void callback() override
	{
		hrt_abstime now = hrt_absolute_time();
		hrt_abstime interval = now - _last_us;
		_last_us = now;

		_msg.seq++;
		_msg.value = static_cast<int32_t>(now / 1000);
		zbus_chan_pub(&rtframe_demo_chan, &_msg, K_MSEC(10));

		printk("[pub] seq=%u interval=%llu us\n", _msg.seq, interval);
	}

private:
	struct rtframe_msg _msg {};
	hrt_abstime _last_us{0};
};

RTFRAME_TASK_REGISTER(DemoPub, vwork::configs::sensor, INIT_LEVEL_APP, 500000);
