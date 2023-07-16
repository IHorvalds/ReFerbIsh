//
// Created by ihorv on 16.07.2023.
//

#ifndef REFERBISH_THREAD_UTILITIES_H
#define REFERBISH_THREAD_UTILITIES_H

#include <atomic>
#include <concepts>
#include <exception>

namespace ihpedals::utilities
{

// Concept: invocable object wth specified argument type and return type
template <typename Func, typename ReturnType, typename... Args>
concept invocable_r = requires(Func f, Args&&... args) {
    {
        f(args...)
    } -> std::same_as<ReturnType>;
};

// Simple barrier for synchronising a pool of threads
// The same thread can arrive multiple times (e.g. if it got scheduled twice)
static_assert(std::atomic_uint8_t::is_always_lock_free);

struct barrier
{
    [[nodiscard]] explicit barrier(uint8_t thread_count) : m_thread_count(thread_count), m_counter(thread_count)
    {
    }

    void arrive()
    {
        --m_counter;
        if (m_counter > m_thread_count) // overflow
            throw std::runtime_error("A thread arrived unexpectedly");
    }

    void reset() noexcept
    {
        m_counter.store(m_thread_count);
    }

    void reset(uint8_t new_val)
    {
        m_thread_count.store(new_val);
        reset();
    }

    void wait()
    {
        while (m_thread_count.load(std::memory_order_relaxed) != 0)
            ; // busy wait, but I don't have a better idea
    }

private:
    std::atomic_uint8_t m_thread_count;
    std::atomic_uint8_t m_counter;
};

} // namespace ihpedals::utilities

#endif // REFERBISH_THREAD_UTILITIES_H
