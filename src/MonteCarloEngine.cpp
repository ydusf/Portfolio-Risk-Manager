#include <vector>
#include <numeric>
#include <cassert>
#include <random>
#include <cmath>
#include <algorithm>
#include <functional>

#include "../include/MonteCarloEngine.hpp"
#include "../include/DataHandler.hpp"

/* -------------------------- PUBLIC METHODS ------------------------------ */

MonteCarloEngine::MonteCarloEngine()
{
    for(std::size_t i = 0; i < m_NUM_THREADS; ++i)
    {
        m_normal_rngs.emplace_back();
        m_threadPool.emplace_back();
    }
}

MonteCarloEngine::~MonteCarloEngine()
{}

Returns MonteCarloEngine::GenerateReturns
(
    double drift,
    double volatility,
    std::size_t numPaths/*1000000*/, 
    std::size_t numDays/*252*/
)
{
    Returns returns;
    returns.m_returns.resize(numPaths * numDays);
    returns.m_blockSize = numDays;

    const double dt = 1.0 / 252.0;
    const double dailyVolatility = volatility * std::sqrt(dt);
    const double dailyDrift = drift * dt;

    std::vector<std::thread> threads;
    threads.reserve(m_NUM_THREADS);

    const std::size_t pathsPerThread = numPaths / m_NUM_THREADS;

    for(std::size_t threadIdx = 0; threadIdx < m_NUM_THREADS; ++threadIdx)
    {
        const std::size_t startPath = threadIdx * pathsPerThread;
        const std::size_t endPath = (threadIdx == m_NUM_THREADS - 1) ? numPaths : (threadIdx+ 1) * pathsPerThread;

        threads.emplace_back([&, threadIdx, startPath, endPath, dailyDrift, dailyVolatility]() {
            GenerateReturnsJob(returns.m_returns, threadIdx, startPath, endPath, numDays, dailyDrift, dailyVolatility);
        });
    }

    for(std::thread& th : threads)
    {
        if(th.joinable())
        {
            th.join();
        }
    }

    return returns;
}

Returns MonteCarloEngine::BuildPricePaths
(
    const Returns& returns,
    double initialPrice
) 
{
    std::size_t n = returns.m_returns.size();
    
    Returns prices;
    prices.m_returns.resize(n);
    prices.m_blockSize = returns.m_blockSize;

    double S = initialPrice;
    std::size_t idx = 0;
    while(idx < n)
    {
        if(idx % returns.m_blockSize == 0)
        {
            S = initialPrice;
        }

        S *= std::exp(returns.m_returns[idx]);
        prices.m_returns[idx] = S;
        idx++;
    }

    return prices;
}


std::pair<double, double> MonteCarloEngine::CalculatePortfolioStatistics
(
    const std::vector<std::vector<double>>& returns, 
    const Portfolio& portfolio
)
{
    const std::vector<double>& weights = portfolio.GetWeights();
    const std::vector<std::string>& tickers = portfolio.GetTickers();
    
    assert(weights.size() == tickers.size());
    assert(!returns.empty() && returns[0].size() == tickers.size());

    std::vector<double> portfolioReturns;
    portfolioReturns.reserve(returns.size());

    for(std::size_t day = 0; day < returns.size(); ++day)
    {
        double portfolioReturn = 0.0;
        for(std::size_t asset = 0; asset < returns[day].size(); ++asset)
        {
            portfolioReturn += weights[asset] * returns[day][asset];
        }
        portfolioReturns.push_back(portfolioReturn);
    }

    double totalRet = std::accumulate(portfolioReturns.begin(), portfolioReturns.end(), 0.0);
    double meanRet = totalRet / portfolioReturns.size();

    double sumSquaredDiffs = 0.0;
    for(double ret : portfolioReturns)
    {
        sumSquaredDiffs += std::pow(ret - meanRet, 2);
    }
    
    double variance = sumSquaredDiffs / (portfolioReturns.size() - 1);
    double stddev = std::sqrt(variance);

    return std::make_pair(meanRet, stddev);
}

/* -------------------------- PRIVATE METHODS ------------------------------ */

void MonteCarloEngine::GenerateReturnsJob
( 
    std::vector<double>& returns,
    const std::size_t threadIdx, 
    const std::size_t pStartIdx, 
    const std::size_t pEndIdx, 
    const std::size_t numDays,
    const double dDrift, 
    const double dVol
)
{
    NormalRNG& localRng = m_normal_rngs[threadIdx];

    for(std::size_t path = pStartIdx; path < pEndIdx; ++path)
    {                
        std::size_t baseIdx = path * numDays;
        std::generate(returns.begin() + baseIdx, returns.begin() + baseIdx + numDays, [&]() 
            { return dDrift + dVol * localRng(); });
    }
}
