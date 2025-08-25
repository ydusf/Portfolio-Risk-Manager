#include <vector>
#include <numeric>
#include <cassert>
#include <random>
#include <cmath>

#include "../include/MonteCarloEngine.hpp"
#include "../include/DataHandler.hpp"

/* -------------------------- PUBLIC METHODS ------------------------------ */

MonteCarloEngine::MonteCarloEngine()
{
}

MonteCarloEngine::~MonteCarloEngine()
{
}

std::vector<std::vector<double>> MonteCarloEngine::GenerateReturns
(
    const Portfolio& portfolio, 
    double drift,
    double volatility,
    std::size_t numPaths/* = 1000000*/, 
    std::size_t numDays/* = 252*/
)
{
    std::vector<std::vector<double>> paths(numPaths, std::vector<double>(numDays));
    double dt = 1.0 / 252.0;

    std::vector<std::thread> threads;
    threads.reserve(m_numThreads);

    std::size_t pathsPerThread = numPaths / m_numThreads;

    for(std::size_t t = 0; t < m_numThreads; ++t)
    {
        std::size_t startPath = t * pathsPerThread;
        std::size_t endPath = (t == m_numThreads - 1) ? numPaths : (t + 1) * pathsPerThread;

        threads.emplace_back([&, startPath, endPath, drift, volatility, dt]()
        {
            NormalRNG localRng(0.0, 1.0);

            for(std::size_t path = startPath; path < endPath; ++path)
            {
                for(std::size_t day = 0; day < numDays; ++day)
                {        
                    double gbmMulti = localRng();
                    double dailyReturn = drift * dt + volatility * std::sqrt(dt) * gbmMulti;
                    paths[path][day] = dailyReturn;
                }
            }
        });
    }

    for(std::thread& th : threads)
    {
        th.join();
    }

    return paths;
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