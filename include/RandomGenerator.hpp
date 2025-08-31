#pragma once

#include <random>
#include "lib/pcg_random.hpp"

struct GenNormalPCG 
{
    pcg64_fast rng;
    std::normal_distribution<double> dist;

    GenNormalPCG() : GenNormalPCG(0.0, 1.0) {}

    GenNormalPCG(double mean, double stddev)
        : rng(seedFromDevice()), dist(mean, stddev) {}

    double operator()() 
    {
        return dist(rng);
    }

private:
    static uint64_t seedFromDevice() 
    {
        std::random_device rd;
        uint64_t seed = (static_cast<uint64_t>(rd()) << 32) ^ rd();
        return seed;
    }
};