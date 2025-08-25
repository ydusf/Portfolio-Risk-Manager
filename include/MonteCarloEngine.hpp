#pragma once

#include <vector>
#include <thread>
#include <random>

#include "Portfolio.hpp"

struct NormalRNG
{
    std::mt19937 m_gen;
    std::normal_distribution<double> m_dist;

    NormalRNG(double mean = 0.0, double stddev = 1.0, unsigned seed = std::random_device{}())
        : m_gen(seed), m_dist(mean, stddev) {}

    double operator()()
    {
        return m_dist(m_gen);
    }
};

class MonteCarloEngine 
{
private:
    std::size_t m_numThreads = std::thread::hardware_concurrency();

public:
    MonteCarloEngine();
    ~MonteCarloEngine();
    
    std::pair<double, double> CalculatePortfolioStatistics
    (
        const std::vector<std::vector<double>>& returns, 
        const Portfolio& portfolio
    );
    std::vector<std::vector<double>> GenerateReturns
    (        
        const Portfolio& portfolio, 
        double drift,
        double volatility,
        std::size_t numPaths = 1000000, 
        std::size_t numDays = 252
    );

private:
};