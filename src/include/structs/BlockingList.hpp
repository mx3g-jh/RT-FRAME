#pragma once

#include "IntrusiveSortedList.hpp"
#include "LockGuard.hpp"
#include <zephyr/kernel.h>
#include <stdlib.h>

template<class T>
class BlockingList : public IntrusiveSortedList<T>
{
public:
	BlockingList()
	{
		k_mutex_init(&_mutex);
	}

	void add(T newNode)
	{
		LockGuard lg{_mutex};
		IntrusiveSortedList<T>::add(newNode);
	}

	bool remove(T removeNode)
	{
		LockGuard lg{_mutex};
		return IntrusiveSortedList<T>::remove(removeNode);
	}

	size_t size()
	{
		LockGuard lg{_mutex};
		return IntrusiveSortedList<T>::size();
	}

	void clear()
	{
		LockGuard lg{_mutex};
		IntrusiveSortedList<T>::clear();
	}

	struct k_mutex &mutex() { return _mutex; }

private:
	struct k_mutex _mutex = Z_MUTEX_INITIALIZER(_mutex);
};
