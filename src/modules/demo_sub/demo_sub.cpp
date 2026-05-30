#include <zephyr/sys/printk.h>
#include <zephyr/zbus/zbus.h>
#include "vwork.h"
#include "hrt.h"
#include "task_register.h"
#include "rtframe_channels.h"

ZBUS_OBS_DECLARE(demo_sub);

/* 线程模型示例：阻塞等待 zbus 消息，收到即打印。 */
class DemoSub : public vwork::Thread
{
public:
	DemoSub() : Thread(vwork::configs::sub) {}

protected:
	void run() override
	{
		printk("[sub] run\n");
		const struct zbus_channel *chan;

		while (true) {
			if (zbus_sub_wait(&demo_sub, &chan, K_FOREVER) == 0) {
				struct rtframe_msg msg;
				zbus_chan_read(chan, &msg, K_MSEC(10));
				printk("[sub] now=%llu us seq=%u value=%d\n",
				       hrt_absolute_time(), msg.seq, msg.value);
			}
		}
	}
};

RTFRAME_TASK_REGISTER(DemoSub, vwork::configs::sub, INIT_LEVEL_APP, 0);
