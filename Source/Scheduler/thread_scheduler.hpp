//
// Created by ihorv on 16.07.2023.
//

#ifndef REFERBISH_THREAD_SCHEDULER_HPP
#define REFERBISH_THREAD_SCHEDULER_HPP

#include "audio_child_thread.h"
#include "thread_utilities.h"
#include <thread>
#include <vector>

namespace ihpedals
{

/// Class to schedule processing of a buffer
/// by assigning portions of the buffer to
/// multiple threads (one portion <-> one thread)
/// until the entire buffer has been processed.
///
/// Each thread will run the same function,
/// which must be known when creating the scheduler
template <uint8_t thread_pool_size, std::floating_point SampleType>
class thread_scheduler
{
public:
    explicit thread_scheduler(utilities::invocable_r<void, gsl::span<SampleType>> auto&& func);
    ~thread_scheduler();

    //! The size of the buffer must be a multiple of channels
    void process_buffer(gsl::span<SampleType> buffer, uint16_t channels);

private:
    std::vector<audio_child_thread<SampleType>> m_thread_pool;
    utilities::barrier m_barrier;
};

} // namespace ihpedals

#include "thread_scheduler.inl"

#endif // REFERBISH_THREAD_SCHEDULER_HPP
