#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t hrt_abstime;

/* 当前绝对时间（微秒）。基于 cycle counter 直读，纳秒级精度，零中断开销。 */
hrt_abstime hrt_absolute_time();

/* 自 *then 起经过的时间（微秒）。 */
static inline hrt_abstime hrt_elapsed_time(const hrt_abstime *then)
{
	return hrt_absolute_time() - *then;
}

#ifdef __cplusplus
}

namespace time_literals
{
constexpr hrt_abstime operator""_s(unsigned long long s)  { return hrt_abstime(s  * 1000000ULL); }
constexpr hrt_abstime operator""_ms(unsigned long long ms) { return hrt_abstime(ms * 1000ULL); }
constexpr hrt_abstime operator""_us(unsigned long long us) { return hrt_abstime(us); }
} /* namespace time_literals */
#endif
