#pragma once

#include <Eigen/Dense>

#include <random>
#include <thread>
#include "lib/pcg_random.hpp"

class GenNormalPCG 
{
private:
    pcg64_fast rng;
    std::normal_distribution<double> dist;

public:

    GenNormalPCG() : GenNormalPCG(0.0, 1.0) {}

    GenNormalPCG(double mean, double stddev)
        : rng(seedFromDevice()), dist(mean, stddev) {}

    double operator()() 
    {
        return dist(rng);
    }

    Eigen::MatrixXd GenerateRandomMatrixUnsafe(const std::size_t rows, const std::size_t cols)
    {
        const std::size_t NUM_THREADS = std::thread::hardware_concurrency();
        
        Eigen::MatrixXd matrix(rows, cols);

        std::vector<std::thread> threads;
        threads.reserve(NUM_THREADS);

        std::size_t totalCells = rows * cols;
        const std::size_t cellsPerThread = totalCells / NUM_THREADS;

        double* ptr = matrix.data();
        for(std::size_t threadIdx = 0; threadIdx < NUM_THREADS; ++threadIdx)
        {
            const std::size_t start = threadIdx * cellsPerThread;
            const std::size_t end = (threadIdx == NUM_THREADS - 1) ? totalCells : (threadIdx + 1) * cellsPerThread;
            threads.emplace_back([&, ptr, start, end]() {
                for(std::size_t i = start; i < end; ++i)
                {
                    ptr[i] = (*this)();
                }
            });
        }

        for(std::thread& th : threads)
        {
            th.join();
        }

        return matrix;
    }

private:
    static uint64_t seedFromDevice() 
    {
        std::random_device rd;
        uint64_t seed = (static_cast<uint64_t>(rd()) << 32) ^ rd();
        return seed;
    }
};