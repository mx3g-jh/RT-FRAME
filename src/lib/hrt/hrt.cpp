#include <zephyr/kernel.h>
#include "hrt.h"

hrt_abstime hrt_absolute_time()
{
	return k_cyc_to_us_floor64(k_cycle_get_64());
}
