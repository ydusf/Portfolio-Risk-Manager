#include <vector>
#include <numeric>
#include <cassert>
#include <random>
#include <cmath>
#include <arm_neon.h>

#include "../include/MonteCarloEngine.hpp"
#include "../include/DataHandler.hpp"

/* -------------------------- PUBLIC METHODS ------------------------------ */

MonteCarloEngine::MonteCarloEngine()
{
    for(std::size_t i = 0; i < m_NUM_THREADS; ++i)
    {
        m_normal_rngs.emplace_back();
    }
}

MonteCarloEngine::~MonteCarloEngine()
{
}

Returns MonteCarloEngine::GenerateReturns
(
    double drift,
    double volatility,
    std::size_t numPaths/* = 1000000*/, 
    std::size_t numDays/* = 252*/
)
{
    Returns returns;
    returns.m_returns.resize(numPaths * numDays);
    returns.m_blockSize = numDays;

    double dt = 1.0 / 252.0;

    std::vector<std::thread> threads;
    threads.reserve(m_NUM_THREADS);

    std::size_t pathsPerThread = numPaths / m_NUM_THREADS;

    for(std::size_t t = 0; t < m_NUM_THREADS; ++t)
    {
        std::size_t startPath = t * pathsPerThread;
        std::size_t endPath = (t == m_NUM_THREADS - 1) ? numPaths : (t + 1) * pathsPerThread;

        threads.emplace_back([&, t, startPath, endPath, drift, volatility, dt]()
        {
            NormalRNG& localRng = m_normal_rngs[t];

            for(std::size_t path = startPath; path < endPath; ++path)
            {
                for(std::size_t day = 0; day < numDays; ++day)
                {        
                    returns.m_returns[path * numDays + day] = drift * dt + volatility * std::sqrt(dt) * localRng();
                }
            }
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