#pragma once

#include <vector>
#include <thread>

#include "Portfolio.hpp"
#include "RandomGenerator.hpp"

struct Returns
{
    std::vector<double> m_returns;
    std::size_t m_blockSize;
};

class MonteCarloEngine 
{
private:
    const std::size_t m_NUM_THREADS = std::thread::hardware_concurrency();

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
    Returns BuildPricePaths
    (
        const Returns& returns,
        double initialPrice
    );
};