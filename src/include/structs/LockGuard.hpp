#pragma once

#include <zephyr/kernel.h>

class LockGuard
{
public:
	explicit LockGuard(struct k_mutex &mutex) : _mutex(mutex)
	{
		k_mutex_lock(&_mutex, K_FOREVER);
	}

	LockGuard(const LockGuard &) = delete;
	LockGuard &operator=(const LockGuard &) = delete;

	~LockGuard() { k_mutex_unlock(&_mutex); }

private:
	struct k_mutex &_mutex;
};
