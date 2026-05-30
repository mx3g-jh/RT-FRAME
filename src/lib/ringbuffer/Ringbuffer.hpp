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

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <px4_atomic.h>


/**
 * Ringbuffer — FIFO ringbuffer: single-producer / single-consumer, lock-free.
 *
 * Concurrency contract:
 *   * Exactly one producer thread/ISR calls push_back / push_back_partial.
 *   * Exactly one consumer thread/ISR calls pop_front.
 *   * space_available() may only be called by the producer.
 *   * space_used()      may only be called by the consumer.
 *   * Multiple producers (or multiple consumers) require external
 *     serialisation by the caller.
 *
 * Capacity is buffer_size - 1 (one slot kept free so empty/full are distinct).
 *
 * Performance: indices use power-of-2 bitwise masking (buffer size must be a
 * power of 2).  On Cortex-M7 this avoids the costly DIV instruction.
 */
class Ringbuffer
{
public:
	Ringbuffer() = default;

	/** Frees heap allocation if allocate() was used. */
	~Ringbuffer();

	/**
	 * Heap-allocate a buffer of buffer_size bytes.
	 * buffer_size MUST be a power of 2 (asserted).
	 */
	bool allocate(size_t buffer_size);

	/**
	 * Use a caller-supplied static buffer.  Does not take ownership.
	 * buffer_size MUST be a power of 2 (asserted).
	 */
	bool init_static(uint8_t *buf, size_t buffer_size);

	void deallocate();

	/** Bytes available for the producer to write. */
	size_t space_available() const;

	/** Bytes currently held in the buffer (consumer view). */
	size_t space_used() const;

	/** All-or-nothing push. Returns false if buf_len would not fit. */
	bool push_back(const uint8_t *buf, size_t buf_len);

	/** Pushes as many bytes as fit, returns the number actually pushed. */
	size_t push_back_partial(const uint8_t *buf, size_t buf_len);

	/** Pops up to buf_max_len bytes, returns the number actually popped. */
	size_t pop_front(uint8_t *buf, size_t buf_max_len);

	/** Maximum usable bytes (one slot is always kept free). */
	size_t capacity() const { return _mask; }

private:
	size_t                 _size{0};
	uint8_t              *_ringbuffer{nullptr};
	bool                   _owns_buffer{false};

	/**
	 * _start: consumer writes, producer reads  (acquire on producer side)
	 * _end:   producer writes, consumer reads  (acquire on consumer side)
	 *
	 * Both must be std::atomic_size_t — volatile is insufficient on
	 * cores with data cache (Cortex-M7 L1 DCache).
	 */
	px4::atomic<size_t>   _start{0};
	px4::atomic<size_t>   _end{0};

	/**
	 * Bitmask for power-of-2 wraparound: index & _mask == index % _size.
	 * _mask = _size - 1.  _size must be power of 2, verified in init.
	 */
	size_t                 _mask{0};
};
