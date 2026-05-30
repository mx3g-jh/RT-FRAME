/****************************************************************************
 *
 *   Copyright (c) 2012-2019 PX4 Development Team. All rights reserved.
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

/**
 * @file Subscription.hpp
 * uORB Subscription — Zephyr port
 */

#pragma once

#include <uORB/uORB.h>
#include <uORB/topics/uORBTopics.hpp>

#include "uORBManager.hpp"

namespace uORB
{

class SubscriptionCallback;

/**
 * Base subscription wrapper class
 */
class Subscription
{
public:
	Subscription(ORB_ID id, uint8_t instance = 0) :
		_orb_id(id), _instance(instance)
	{
		subscribe();
	}

	Subscription(const orb_metadata *meta = nullptr, uint8_t instance = 0) :
		_orb_id((meta == nullptr) ? ORB_ID::INVALID : static_cast<ORB_ID>(meta->o_id)),
		_instance(instance)
	{
		subscribe();
	}

	Subscription(const Subscription &other) : _orb_id(other._orb_id), _instance(other._instance) {}
	Subscription(Subscription &&other) noexcept : _orb_id(other._orb_id), _instance(other._instance) {}

	Subscription &operator=(const Subscription &other)
	{
		if (this != &other) {
			unsubscribe();
			_orb_id = other._orb_id;
			_instance = other._instance;
		}
		return *this;
	}

	Subscription &operator=(Subscription &&other) noexcept
	{
		unsubscribe();
		_orb_id = other._orb_id;
		_instance = other._instance;
		return *this;
	}

	~Subscription() { unsubscribe(); }

	bool subscribe();
	void unsubscribe();

	bool valid() const { return _node != nullptr; }

	bool advertised()
	{
		if (subscribe()) {
			return Manager::is_advertised(_node);
		}
		return false;
	}

	bool updated()
	{
		if (subscribe()) {
			return Manager::updates_available(_node, _last_generation) > 0;
		}
		return false;
	}

	bool update(void *dst)
	{
		if (subscribe()) {
			return Manager::orb_data_copy(_node, dst, _last_generation, true);
		}
		return false;
	}

	bool copy(void *dst)
	{
		if (subscribe()) {
			return Manager::orb_data_copy(_node, dst, _last_generation, false);
		}
		return false;
	}

	bool ChangeInstance(uint8_t instance);

	uint8_t  get_instance() const { return _instance; }
	unsigned get_last_generation() const { return _last_generation; }
	orb_id_t get_topic() const { return get_orb_meta(_orb_id); }
	ORB_ID   orb_id() const { return _orb_id; }

protected:
	friend class SubscriptionCallback;

	void *get_node() { return _node; }

	void       *_node{nullptr};
	unsigned    _last_generation{0};
	ORB_ID      _orb_id{ORB_ID::INVALID};
	uint8_t     _instance{0};
};

/**
 * Subscription wrapper with embedded data
 */
template<class T>
class SubscriptionData : public Subscription
{
public:
	SubscriptionData(ORB_ID id, uint8_t instance = 0) :
		Subscription(id, instance) { copy(&_data); }

	SubscriptionData(const orb_metadata *meta, uint8_t instance = 0) :
		Subscription(meta, instance) { copy(&_data); }

	~SubscriptionData() = default;

	SubscriptionData(const SubscriptionData &) = delete;
	SubscriptionData &operator=(const SubscriptionData &) = delete;
	SubscriptionData(SubscriptionData &&) = delete;
	SubscriptionData &operator=(SubscriptionData &&) = delete;

	bool update() { return Subscription::update(&_data); }
	const T &get() const { return _data; }

private:
	T _data{};
};

} // namespace uORB
