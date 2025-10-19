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
{}

MonteCarloEngine::~MonteCarloEngine()
{}

Returns MonteCarloEngine::GenerateReturns(double drift, double volatility, std::size_t numPaths/*1000000*/,  std::size_t numDays/*252*/)
{
    Returns returns;
    returns.m_returns.resize(numPaths * numDays);
    returns.m_blockSize = numDays;

    constexpr double dt = 1.0 / 252.0;
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
            thread_local static GenNormalPCG rng;
            for(std::size_t path = startPath; path < endPath; ++path)
            {                
                std::size_t baseIdx = path * numDays;
                for(std::size_t day = baseIdx; day < baseIdx + numDays; ++day)
                {
                    returns.m_returns[day] = dailyDrift + dailyVolatility * rng();
                }
            }
        });
    }

    for(std::thread& th : threads)
    {
        th.join();
    }

    return returns;
}

Returns MonteCarloEngine::BuildPricePaths(const Returns& returns, double initialPrice) 
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

std::pair<double, double> MonteCarloEngine::ComputeAssetStatistics(const std::vector<double>& assetReturns)
{
    assert(!assetReturns.empty());

    const double n = static_cast<double>(assetReturns.size());
    const double mean = std::accumulate(assetReturns.begin(), assetReturns.end(), 0.0) / n;

    double sumSquaredDiffs = 0.0;

    for (double r : assetReturns)
    {
        sumSquaredDiffs += std::pow(r - mean, 2);
    }

    const double variance = sumSquaredDiffs / (n - 1.0);
    const double stddev = std::sqrt(variance);

    return { mean, stddev };
}

std::vector<std::pair<double, double>> MonteCarloEngine::ComputeMultiAssetStatistics(const std::vector<std::vector<double>>& returns)
{
    assert(!returns.empty());

    std::vector<std::pair<double, double>> statistics;

    for(const std::vector<double>& assetReturns : returns)
    {
        std::pair<double, double> assetStats = ComputeAssetStatistics(assetReturns);
        statistics.emplace_back(std::move(assetStats));
    }

    return statistics;
}

std::vector<double> MonteCarloEngine::CombineAssetReturns(const Portfolio& portfolio)
{
    const std::vector<double>& weights = portfolio.GetWeights();
    const std::vector<std::string>& tickers = portfolio.GetTickers();
    std::vector<std::vector<double>> returns = DataHandler::GetLogReturnsMat(tickers);

    assert(weights.size() == tickers.size());
    assert(!returns.empty() && returns[0].size() == tickers.size());

    std::vector<double> portfolioReturns;
    portfolioReturns.reserve(returns.size());

    for (std::size_t day = 0; day < returns.size(); ++day)
    {
        double portfolioReturn = 0.0;
        for (std::size_t asset = 0; asset < returns[day].size(); ++asset)
        {
            portfolioReturn += weights[asset] * returns[day][asset];
        }
        portfolioReturns.push_back(portfolioReturn);
    }

    return portfolioReturns;
}
