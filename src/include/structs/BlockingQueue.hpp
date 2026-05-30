#pragma once

#include <zephyr/kernel.h>
#include <stdlib.h>

template<class T, size_t N>
class BlockingQueue
{
public:
	BlockingQueue()
	{
		k_sem_init(&_sem_head, (unsigned)N, (unsigned)N);
		k_sem_init(&_sem_tail, 0, (unsigned)N);
	}

	void push(T newItem)
	{
		k_sem_take(&_sem_head, K_FOREVER);
		_data[_tail] = newItem;
		_tail = (_tail + 1) % N;
		k_sem_give(&_sem_tail);
	}

	T pop()
	{
		k_sem_take(&_sem_tail, K_FOREVER);
		T ret = _data[_head];
		_head = (_head + 1) % N;
		k_sem_give(&_sem_head);
		return ret;
	}

private:
	struct k_sem _sem_head;
	struct k_sem _sem_tail;

	T      _data[N]{};
	size_t _head{0};
	size_t _tail{0};
};
