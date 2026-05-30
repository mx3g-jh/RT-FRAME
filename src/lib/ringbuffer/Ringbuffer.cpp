/****************************************************************************
 *
 *   Copyright (C) 2023 PX4 Development Team. All rights reserved.
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

#include "Ringbuffer.hpp"

#include <assert.h>
#include <cstring>

using px4::atomic_load_acquire;
using px4::atomic_load_relaxed;
using px4::atomic_store_release;

Ringbuffer::~Ringbuffer()
{
	deallocate();
}

bool Ringbuffer::allocate(size_t buffer_size)
{
	assert(_ringbuffer == nullptr);
	assert((buffer_size & (buffer_size - 1)) == 0 && "buffer_size must be power of 2");

	_size = buffer_size;
	_mask = _size - 1;
	_ringbuffer = new uint8_t[_size];
	_owns_buffer = (_ringbuffer != nullptr);
	_start.store(0, std::memory_order_relaxed);
	_end.store(0, std::memory_order_relaxed);
	return _ringbuffer != nullptr;
}

bool Ringbuffer::init_static(uint8_t *buf, size_t buffer_size)
{
	assert(_ringbuffer == nullptr);
	assert((buffer_size & (buffer_size - 1)) == 0 && "buffer_size must be power of 2");

	if (buf == nullptr || buffer_size == 0) {
		return false;
	}

	_ringbuffer = buf;
	_size = buffer_size;
	_mask = _size - 1;
	_owns_buffer = false;
	_start.store(0, std::memory_order_relaxed);
	_end.store(0, std::memory_order_relaxed);
	return true;
}

void Ringbuffer::deallocate()
{
	if (_owns_buffer) {
		delete[] _ringbuffer;
	}

	_ringbuffer = nullptr;
	_size = 0;
	_mask = 0;
	_owns_buffer = false;
	_start.store(0, std::memory_order_relaxed);
	_end.store(0, std::memory_order_relaxed);
}

size_t Ringbuffer::space_available() const
{
	/* Producer view: read own _end relaxed, consumer's _start with acquire. */
	const size_t end   = atomic_load_relaxed(&_end);
	const size_t start = atomic_load_acquire(&_start);

	/* unsigned wrap — works correctly because _size is power of 2 */
	return (start - end - 1) & _mask;
}

size_t Ringbuffer::space_used() const
{
	/* Consumer view: read own _start relaxed, producer's _end with acquire. */
	const size_t start = atomic_load_relaxed(&_start);
	const size_t end   = atomic_load_acquire(&_end);

	return (end - start) & _mask;
}

bool Ringbuffer::push_back(const uint8_t *buf, size_t buf_len)
{
	if (buf_len == 0 || buf == nullptr || _ringbuffer == nullptr) {
		return false;
	}

	const size_t end   = atomic_load_relaxed(&_end);
	const size_t start = atomic_load_acquire(&_start);

	const size_t available = (start - end - 1) & _mask;

	if (available < buf_len) {
		return false;
	}

	/* No wrap in the common case (fast path). */
	const size_t new_end = (end + buf_len) & _mask;

	if (new_end < end) {
		/* Wrap: two memcpy calls. */
		const size_t remaining = _size - end;
		memcpy(&_ringbuffer[end], buf, remaining);
		memcpy(&_ringbuffer[0], buf + remaining, buf_len - remaining);

	} else {
		/* Single memcpy (common case — no wrap). */
		memcpy(&_ringbuffer[end], buf, buf_len);
	}

	atomic_store_release(&_end, new_end);
	return true;
}

size_t Ringbuffer::push_back_partial(const uint8_t *buf, size_t buf_len)
{
	if (buf_len == 0 || buf == nullptr || _ringbuffer == nullptr) {
		return 0;
	}

	const size_t end   = atomic_load_relaxed(&_end);
	const size_t start = atomic_load_acquire(&_start);

	const size_t available = (start - end - 1) & _mask;

	if (available == 0) {
		return 0;
	}

	const size_t to_copy = (buf_len < available) ? buf_len : available;

	const size_t new_end = (end + to_copy) & _mask;

	if (new_end < end) {
		/* Wrap. */
		const size_t remaining = _size - end;
		memcpy(&_ringbuffer[end], buf, remaining);
		memcpy(&_ringbuffer[0], buf + remaining, to_copy - remaining);

	} else {
		memcpy(&_ringbuffer[end], buf, to_copy);
	}

	atomic_store_release(&_end, new_end);
	return to_copy;
}

size_t Ringbuffer::pop_front(uint8_t *buf, size_t buf_max_len)
{
	if (buf == nullptr || _ringbuffer == nullptr) {
		return 0;
	}

	const size_t start = atomic_load_relaxed(&_start);
	const size_t end   = atomic_load_acquire(&_end);

	if (start == end) {
		return 0;
	}

	const size_t used = (end - start) & _mask;

	if (used == 0) {
		return 0;
	}

	const size_t to_copy = (used < buf_max_len) ? used : buf_max_len;

	const size_t new_start = (start + to_copy) & _mask;

	if (new_start < start) {
		/* Wrap. */
		const size_t remaining = _size - start;
		memcpy(buf, &_ringbuffer[start], remaining);
		memcpy(buf + remaining, &_ringbuffer[0], to_copy - remaining);

	} else {
		memcpy(buf, &_ringbuffer[start], to_copy);
	}

	atomic_store_release(&_start, new_start);
	return to_copy;
}
