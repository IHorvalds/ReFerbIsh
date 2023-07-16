//
// Created by ihorv on 16.07.2023.
//

namespace ihpedals
{

template <uint8_t thread_pool_size, std::floating_point SampleType>
thread_scheduler<thread_pool_size, SampleType>::thread_scheduler(
    utilities::invocable_r<void, gsl::span<SampleType>> auto&& func
)
    : m_barrier(thread_pool_size)
{
    for (size_t i = 0; i < thread_pool_size; ++i)
    {
        m_thread_pool.emplace_back(audio_child_thread<SampleType>(func, i, m_barrier));
    }
}

template <uint8_t thread_pool_size, std::floating_point SampleType>
thread_scheduler<thread_pool_size, SampleType>::~thread_scheduler()
{
    for (auto child_thread : m_thread_pool)
    {
        child_thread.stop_thread();
    }

    /// when the threads go out of scope, they will join
    /// but they **should** be done by then because
    /// stop_thread() will both request the stop and
    /// break out of the spinlock.
}

template <uint8_t thread_pool_size, std::floating_point SampleType>
void thread_scheduler<thread_pool_size, SampleType>::process_buffer(gsl::span<SampleType> buffer, uint16_t channels)
{
    if (buffer.size() % channels)
        throw std::runtime_error("The size of the buffer must be a multiple of channels");
    uint16_t number_of_samples = buffer.size() / channels;

    uint16_t channels_left = channels;
    while (channels_left)
    {
        uint8_t number_of_threads = std::min(channels_left, (uint16_t)thread_pool_size);
        m_barrier.reset(number_of_threads);

        for (auto th = 0; th < number_of_threads; ++th)
        {
            m_thread_pool[th].set_buffer(buffer.make_subspan(number_of_samples * th, number_of_samples));
        }

        m_barrier.wait();
        channels_left -= number_of_threads;
    }
}

} // namespace ihpedals