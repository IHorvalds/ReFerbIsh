#pragma once

#include <vector>
#include <random>

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
    T pivot;

    srand(time(NULL));

    for (int i = top; i > 0; --i)
    {
        int pivot = rand() % i;
        swap(vec[pivot], vec[i]);
    }
}

template <>
float random_between(float a, float b)
{
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_real_distribution<float> dist(a, b);
    return dist(generator);
}

template <typename T>
T random_between(T a, T b)
{
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<T> dist(a, b);
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

template <typename T, int size>
void InPlaceHouseholderMix(std::array<T, size>& vec)
{
    double sum = 0.0;
    double multiplier = -2.0 / (double)size;

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

float softclipper(float x)
{
    // basically sigmoid, centered around 0, doubled and softened
    return 2.f * (1.f/(1.f + std::exp2f(-x * 0.2f)) - 0.5f);
}