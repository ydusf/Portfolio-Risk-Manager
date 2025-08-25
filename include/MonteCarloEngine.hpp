#pragma once

#include <vector>
#include <thread>
#include <random>

#include "Portfolio.hpp"

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

struct Returns
{
    std::vector<double> m_returns;
    std::size_t m_blockSize;
};

class MonteCarloEngine 
{
private:
    const std::size_t m_NUM_THREADS = std::thread::hardware_concurrency();
    std::vector<NormalRNG> m_normal_rngs;

public:
    MonteCarloEngine();
    ~MonteCarloEngine();
    
    std::pair<double, double> CalculatePortfolioStatistics
    (
        const std::vector<std::vector<double>>& returns, 
        const Portfolio& portfolio
    );
    Returns GenerateReturns
    (        
        double drift,
        double volatility,
        std::size_t numPaths = 1000000, 
        std::size_t numDays = 252
    );

private:
};