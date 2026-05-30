#pragma once

/**
 * px4_atomic.h — C++11 atomics abstraction layer.
 *
 * Architecture rule 2: src/ may only include via platform/include/os/.
 * This header is the OS-agnostic facade over C11 <atomic>.  Both
 * FreeRTOS (ARM Cortex-M7) and pthread (Linux x86_64) provide
 * lock-free std::atomic for sizes ≤ 64-bit on the targets we support.
 *
 * Memory ordering used in ringbuffer code:
 *
 *   Producer writes data → release store to _end  → consumer acquire load sees data
 *   Consumer reads  data → release store to _start → producer acquire load sees progress
 *
 * On a single-core Cortex-M7, memory_order_acquire/release compile to
 * ZERO CPU instructions (compiler-only barrier).  On multi-core (future)
 * the hardware ensures cache coherence so the same code is still correct.
 */

#include <atomic>

namespace px4 {

template<typename T>
using atomic = std::atomic<T>;

using atomic_bool = std::atomic<bool>;
using atomic_int  = std::atomic<int>;
using atomic_uint32 = std::atomic<uint32_t>;
using atomic_uint64 = std::atomic<uint64_t>;

// ---------------------------------------------------------------------------
// Explicit memory-order helpers — used by ringbuffer SPSC
// ---------------------------------------------------------------------------

/** Consumer side: acquire load of peer's index.  Pairs with producer's
 *  release store.  On single-core ARM: zero CPU instructions (compiler barrier).
 */
template<typename T>
inline T atomic_load_acquire(const atomic<T> *obj) {
    return std::atomic_load_explicit(obj, std::memory_order_acquire);
}

/** Producer side: release store of own index.  Pairs with consumer's
 *  acquire load.
 */
template<typename T>
inline void atomic_store_release(atomic<T> *obj, T val) {
    std::atomic_store_explicit(obj, val, std::memory_order_release);
}

/** Relaxed load — only safe when reading your OWN index (no cross-thread
 *  synchronisation needed).  Used for reading _end inside a producer that
 *  has exclusive access.
 */
template<typename T>
inline T atomic_load_relaxed(const atomic<T> *obj) {
    return std::atomic_load_explicit(obj, std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// CAS (compare-and-swap) — MPMC ringbuffer core primitive
// ---------------------------------------------------------------------------

/**
 * Attempt to atomically replace *obj with desired if *obj == expected.
 * Returns true on success (slot acquired); false on failure (retry).
 *
 * Uses weak CAS: on Cortex-M7 LDREX/STREX this always succeeds on the first
 * try unless there is genuine contention.  The retry loop in the caller
 * handles the failure path.
 */
template<typename T>
inline bool atomic_compare_exchange_weak(
    atomic<T> *obj, T *expected, T desired) {
    return std::atomic_compare_exchange_weak_explicit(
        obj, expected, desired,
        std::memory_order_relaxed,
        std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// Fetch-add / fetch-sub — for head/tail increment in MPMC
// ---------------------------------------------------------------------------

template<typename T>
inline T atomic_fetch_add(atomic<T> *obj, T arg) {
    return std::atomic_fetch_add_explicit(obj, arg, std::memory_order_relaxed);
}

template<typename T>
inline T atomic_fetch_sub(atomic<T> *obj, T arg) {
    return std::atomic_fetch_sub_explicit(obj, arg, std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// Compile-time lock-free query — useful for assertions
// ---------------------------------------------------------------------------

template<typename T>
inline bool is_lock_free(const atomic<T> *obj) {
    return std::atomic_is_lock_free(obj);
}

} // namespace px4
