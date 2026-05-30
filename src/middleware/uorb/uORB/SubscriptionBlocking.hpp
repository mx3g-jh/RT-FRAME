/****************************************************************************
 *
 *   Copyright (c) 2019 PX4 Development Team. All rights reserved.
 *   Copyright (c) 2025 rtframe contributors. Zephyr port.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#pragma once

#include "SubscriptionCallback.hpp"
#include <zephyr/kernel.h>

namespace uORB
{

/**
 * Blocking subscription — waits on k_sem until new data arrives.
 */
template<typename T>
class SubscriptionBlocking : public SubscriptionCallback
{
public:
	SubscriptionBlocking(const orb_metadata *meta, uint32_t interval_us = 0, uint8_t instance = 0) :
		SubscriptionCallback(meta, interval_us, instance)
	{
		k_sem_init(&_sem, 0, 1);
	}

	~SubscriptionBlocking() override = default;

	void call() override
	{
		if ((_interval_us == 0) || (hrt_elapsed_time(&_last_update) >= _interval_us)) {
			k_sem_give(&_sem);
		}
	}

	/**
	 * Block until updated.
	 * @param timeout_us  0 = wait forever
	 * @return true if topic was updated
	 */
	bool updatedBlocking(uint32_t timeout_us = 0)
	{
		if (!_registered) {
			registerCallback();
		}

		if (updated()) {
			return true;
		}

		k_timeout_t timeout = (timeout_us == 0) ? K_FOREVER : K_USEC(timeout_us);

		if (k_sem_take(&_sem, timeout) == 0) {
			return updated();
		}

		return false;
	}

	bool updateBlocking(T &data, uint32_t timeout_us = 0)
	{
		if (updatedBlocking(timeout_us)) {
			return copy(&data);
		}

		return false;
	}

private:
	struct k_sem _sem;
};

} // namespace uORB
