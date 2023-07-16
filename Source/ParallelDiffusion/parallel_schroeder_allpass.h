//
// Created by ihorv on 16.07.2023.
//

#ifndef REFERBISH_PARALLEL_SCHROEDER_H
#define REFERBISH_PARALLEL_SCHROEDER_H

#include "../Utils.h"
#include "single_channel_schroeder_allpass.h"
#include <JuceHeader.h>
#include <thread_scheduler.hpp>

namespace ihpedals
{

template <uint8_t thread_pool_size, std::floating_point SampleType>
class parallel_schroeder_allpass
{
public:
    void prepare(const dsp::ProcessSpec& spec) noexcept
    {
        if (!mp_th_sched)
        {
            m_sc_schroeder = single_channel_schroeder_allpass<SampleType>(spec);
            mp_th_sched = std::make_unique<thread_scheduler<thread_pool_size, SampleType>>(m_sc_schroeder);
        }
    }

    void set_parameters(single_channel_schroeder_allpass<SampleType>::parameters const& params)
    {
        m_sc_schroeder.set_parameters(params);
    }

    void process_buffer(gsl::span<SampleType> buffer, uint16_t channels)
    {
        if (mp_th_sched)
        {
            mp_th_sched->process_buffer(buffer, channels);
        }
    }

private:
    std::unique_ptr<thread_scheduler<thread_pool_size, SampleType>> mp_th_sched;
    single_channel_schroeder_allpass<SampleType> m_sc_schroeder;
};

} // namespace ihpedals

#endif // REFERBISH_PARALLEL_SCHROEDER_H
