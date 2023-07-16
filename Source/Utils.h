#pragma once

#include <JuceHeader.h>
#include <chrono>
#include <gsl/span>
#include <random>
#include <vector>

namespace ihpedals::utilities
{

constexpr float primes[16] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53};

template <typename Type>
struct random
{
private:
    std::minstd_rand0 rng;

public:
    void rand_permutation(std::vector<Type>& vec)
    {
        std::shuffle(vec.begin(), vec.end(), rng);
    }

    Type random_between(Type a, Type b)
    {
        return (Type)0;
    };

    Type random_between(Type a, Type b)
        requires std::integral<Type>;

    Type random_between(Type a, Type b)
        requires std::floating_point<Type>;

    bool random_bool()
    {
        std::uniform_int_distribution<int> dist(0, 10'000);
        return bool(dist(rng) % 2);
    }
};

template <typename Type>
Type random<Type>::random_between(Type a, Type b)
    requires std::floating_point<Type>
{
    if (b < a)
        std::swap(a, b);
    std::uniform_real_distribution<Type> dist(a, b);
    return dist(rng);
}

template <typename Type>
Type random<Type>::random_between(Type a, Type b)
    requires std::integral<Type>
{
    if (b < a)
        std::swap(a, b);
    std::uniform_int_distribution<Type> dist(a, b);
    return dist(rng);
}

template <std::floating_point Type, std::floating_point SampleRateType>
Type milliseconds_to_samples(Type duration_ms, SampleRateType sample_rate) noexcept
{
    return duration_ms * gsl::narrow_cast<Type>(0.001 * sample_rate);
}

template <typename T>
void in_place_hadamard_mix(T* vec, size_t start, size_t size)
{
    if (size <= 1)
    {
        return;
    }

    size_t half_size = size / 2;

    in_place_hadamard_mix<T>(vec, start, half_size);
    in_place_hadamard_mix<T>(vec, start + half_size, half_size);

    for (size_t i = 0; i < half_size; ++i)
    {
        T a = vec[start + i];
        T b = vec[start + half_size + i];
        vec[start + i] = a + b;
        vec[start + half_size + i] = a - b;
    }
}

template <std::floating_point Type>
void in_place_householder_mix(gsl::span<Type> s)
{
    Type sum = (Type)0;
    Type multiplier = (Type)-2 / (Type)s.size();

    for (auto& val : s)
    {
        sum += val;
    }

    sum *= multiplier;

    for (auto& val : s)
    {
        val += sum;
    }
}

template <std::floating_point SampleType>
inline constexpr SampleType softclipper(SampleType x)
{
    // basically sigmoid, centered around 0, doubled and softened
    // return 2.f * (1.f/(1.f + std::exp2f(-x * 0.2f)) - 0.5f);

    return static_cast<SampleType>(2) / juce::MathConstants<SampleType>::pi * std::atan(x);
}

/// <summary>
///     Assumes sampleStep (1 sample for each channel we need to multiplex) has at least size equal to permutation.
/// </summary>
/// <typeparam name="Type">Float type</typeparam>
/// <param name="permutation">Generate this before running this function. See rand_permutation()</param>
template <typename Type>
void additive_multiplexer(
    std::vector<int>& permutation, std::vector<Type>& inputSampleStep, std::vector<Type>& outputSampleStep
)
{
    jassert(permutation.size() == inputSampleStep.size() && inputSampleStep.size() == outputSampleStep.size());

    for (size_t ch = 0; ch < permutation.size(); ++ch)
    {
        outputSampleStep[ch] = inputSampleStep[ch] + inputSampleStep[permutation[ch]];
    }
}
}; // namespace ihpedals::utilities