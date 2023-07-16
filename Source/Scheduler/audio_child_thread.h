//
// Created by ihorv on 16.07.2023.
//

#ifndef REFERBISH_AUDIO_THREAD_H
#define REFERBISH_AUDIO_THREAD_H

#include "thread_utilities.h"
#include <functional>
#include <gsl/span>
#include <stop_token>
#include <thread>

namespace ihpedals
{

/// Wrapper that will store a callable object
/// and will use a barrier to signal when
/// it finished processing.
/// RAII type. Instantiation starts the threads
/// then they wait for a buffer to be set
template <std::floating_point SampleType>
class audio_child_thread
{
public:
    audio_child_thread(
        utilities::invocable_r<void, gsl::span<SampleType>, uint8_t> auto&& processor_func, uint8_t thread_number,
        utilities::barrier& barrier
    )
        : m_thread_number(thread_number)
    {
        mr_process_func = std::ref(processor_func);
        m_thread = std::jthread(
            std::bind(decltype(this)::process_buffer, this, std::placeholders::_1, std::placeholders::_2), barrier
        );
    }

    void set_buffer(gsl::span<SampleType> buffer)
    {
        m_buffer = buffer;
        std::ignore = m_ready.test_and_set(); // set to true, ignore previous value
    }

    // The barrier object must live at least as long as the calling thread.
    // I.e. at least until stop_token has requested to stop
    void process_buffer(std::stop_token stop_token, utilities::barrier& barrier)
    {
        while (!stop_token.stop_requested())
        {
            while (!m_ready.test() && !m_stopping.test())
                ;

            if (m_stopping.test())
            {
                return;
            }

            mr_process_func(m_buffer, m_thread_number);
            barrier.arrive();
            m_ready.clear();
        }
    }

    void stop_thread()
    {
        m_thread.request_stop();
        std::ignore = m_stopping.test_and_set();
    }

private:
    std::reference_wrapper<std::function<void(gsl::span<SampleType>, uint8_t)>> mr_process_func;
    gsl::span<SampleType> m_buffer;
    std::atomic_flag m_ready = ATOMIC_FLAG_INIT;
    std::atomic_flag m_stopping = ATOMIC_FLAG_INIT;
    std::jthread m_thread;
    uint8_t m_thread_number;
};

} // namespace ihpedals

#endif // REFERBISH_AUDIO_THREAD_H
