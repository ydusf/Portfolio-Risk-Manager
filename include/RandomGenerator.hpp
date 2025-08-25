#pragma once

#include <random>

struct NormalRNG
{
    std::mt19937 m_gen;
    std::normal_distribution<double> m_dist;

    NormalRNG() 
        : NormalRNG(0.0, 1.0, std::random_device{}()) {}

    NormalRNG(double mean, double stddev, unsigned seed = std::random_device{}())
        : m_gen(seed), m_dist(mean, stddev) {}

    double operator()()
    {
        return m_dist(m_gen);
    }
};