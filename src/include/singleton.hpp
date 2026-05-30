#pragma once

#include <cstddef>

/**
 * @file Singleton.hpp
 *
 * CRTP singleton helper supporting both static and dynamic instantiation.
 *
 * Default mode — Meyers static local (lazy, zero-cost):
 *
 *   class MyService : public px4::Singleton<MyService> {
 *       friend class px4::Singleton<MyService>;
 *       MyService() = default;
 *   public:
 *       void hello();
 *   };
 *
 *   MyService::GetInstance().hello();   // first call constructs the static
 *
 * Dynamic mode — caller controls lifetime via new/delete:
 *
 *   MyService::SetInstance(new MyService());
 *   MyService::GetInstance().hello();   // returns the heap object
 *   MyService::DestroyInstance();       // delete + clear pointer
 *
 * Once SetInstance() has been called, GetInstance() always returns the
 * dynamic object; the static local is never constructed. Calling
 * SetInstance(nullptr) reverts to the static fallback on the next
 * GetInstance() call.
 */
namespace px4
{

template<typename T>
class Singleton
{
public:
	static T &GetInstance()
	{
		if (_instance != nullptr) {
			return *_instance;
		}

		static T s_instance;
		return s_instance;
	}

	/** Returns the dynamic instance pointer, or nullptr if none was set. */
	static T *TryGetInstance() { return _instance; }

	/** Adopt a heap-allocated instance. Pass nullptr to clear. */
	static void SetInstance(T *p) { _instance = p; }

	/** Delete the dynamic instance (if any) and clear the pointer. */
	static void DestroyInstance()
	{
		if (_instance != nullptr) {
			delete _instance;
			_instance = nullptr;
		}
	}

protected:
	Singleton() = default;
	~Singleton() = default;

	Singleton(const Singleton &)            = delete;
	Singleton &operator=(const Singleton &) = delete;
	Singleton(Singleton &&)                 = delete;
	Singleton &operator=(Singleton &&)      = delete;

private:
	static T *_instance;
};

template<typename T>
T *Singleton<T>::_instance = nullptr;

} // namespace px4
