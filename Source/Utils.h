#pragma once

#include <vector>
#include <random>

constexpr float tenPrimes[10] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29};

template <typename T>
void swap(T& a, T& b)
{
    T c = a;
    a = b;
    b = c;
}

template <typename T>
void rand_permutation(std::vector<T>& vec)
{
    size_t top = vec.size() - 1;

    srand(time(nullptr));

    for (size_t i = top; i > 0; --i)
    {
        int pivot = rand() % i;
        swap(vec[pivot], vec[i]);
    }
}

inline float random_between_float(float a, float b)
{
    std::random_device rd;
    std::mt19937 generator(rd());
    if (b < a)
        swap(a, b);
    std::uniform_real_distribution<float> dist(a, b);
    return dist(generator);
}

inline int random_between_int(int a, int b)
{
    std::random_device rd;
    std::mt19937 generator(rd());
    if (b < a)
        swap(a, b);
    std::uniform_int_distribution<int> dist(a, b);
    return dist(generator);
}

template <typename T>
void InPlaceHadamardMix(T* vec, size_t start, int size)
{
    if (size <= 1)
    {
        return;
    }

    int half_size = size / 2;

    InPlaceHadamardMix<T>(vec, start, half_size);
    InPlaceHadamardMix<T>(vec, start + half_size, half_size);

    for (int i = 0; i < half_size; ++i)
    {
        T a = vec[start + i];
        T b = vec[start + half_size + i];
        vec[start + i] = a + b;
        vec[start + half_size + i] = a - b;
    }
}

template <typename T>
void InPlaceHouseholderMix(T* vec, int size)
{
    float sum = 0.f;
    float multiplier = -2.f / (float)size;

    for (int i = 0; i < size; ++i)
    {
        sum += vec[i];
    }

    sum *= multiplier;

    for (int i = 0; i < size; ++i)
    {
        vec[i] += sum;
    }

}

inline float softclipper(float x)
{
    // basically sigmoid, centered around 0, doubled and softened
    //return 2.f * (1.f/(1.f + std::exp2f(-x * 0.2f)) - 0.5f);

    return 2.f / juce::MathConstants<float>::pi * std::atanf(x);
}