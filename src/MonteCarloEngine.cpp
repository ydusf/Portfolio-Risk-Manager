#include <vector>
#include <numeric>
#include <cassert>
#include <random>
#include <cmath>
#include <algorithm>
#include <functional>

#include "../include/MonteCarloEngine.hpp"
#include "../include/DataHandler.hpp"
#include "../include/PortfolioOptimisation.hpp"

/* -------------------------- PUBLIC METHODS ------------------------------ */

MonteCarloEngine::MonteCarloEngine()
{}

MonteCarloEngine::~MonteCarloEngine()
{}


Returns MonteCarloEngine::GenerateReturnsForMultiAsset(
    const Eigen::MatrixXd& choleskyMatrix, 
    const std::vector<std::pair<double, double>>& assetStatistics,
    const std::vector<double>& weights,
    bool ignoreDrift, 
    std::size_t numPaths, 
    std::size_t numDays) const
{    
    const std::size_t numAssets = assetStatistics.size();
    
    Returns returns;
    returns.m_returns.resize(numPaths * numDays);
    returns.m_blockSize = numDays;

    const double dt = 1.0 / static_cast<double>(numDays);
    const double sqrtDt = std::sqrt(dt);

    double totalDrift = 0.0;
    if(!ignoreDrift)
    {
        for(std::size_t i = 0; i < numAssets; ++i) 
        {
            totalDrift += weights[i] * assetStatistics[i].first * dt;
        }
    }
    else
    {
        // use an arbitrary risk free rate (should probably put this as an argument)
        double riskFreeRate = 0.04;
        totalDrift = riskFreeRate * dt;
    }

    Eigen::VectorXd volatilities(numAssets);
    for(std::size_t i = 0; i < numAssets; ++i) 
    {
        volatilities(i) = weights[i] * assetStatistics[i].second * sqrtDt;
    }

    // pre-calculate (Vol^T * L)
    // inner loop simply needs to perform (effectiveWeights * Z)
    Eigen::VectorXd effectiveWeights = volatilities.transpose() * choleskyMatrix;

    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);
    const std::size_t pathsPerThread = numPaths / NUM_THREADS;

    for(std::size_t threadIdx = 0; threadIdx < NUM_THREADS; ++threadIdx)
    {
        const std::size_t startPath = threadIdx * pathsPerThread;
        const std::size_t endPath = (threadIdx == NUM_THREADS - 1) ? numPaths : (threadIdx + 1) * pathsPerThread;

        threads.emplace_back([&, startPath, endPath, totalDrift, effectiveWeights]() {
            thread_local GenNormalPCG rng; 
        
            Eigen::VectorXd independentShocks(numAssets);

            for(std::size_t path = startPath; path < endPath; ++path)
            {
                std::size_t baseIdx = path * numDays;
                for(std::size_t day = 0; day < numDays; ++day)
                {
                    for(std::size_t i = 0; i < numAssets; ++i) 
                    {
                        independentShocks(i) = rng();
                    }

                    double stochasticComponent = effectiveWeights.dot(independentShocks);             
                    returns.m_returns[baseIdx + day] = totalDrift + stochasticComponent;
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

Returns MonteCarloEngine::GenerateReturnsForSingleAsset(double drift, double volatility, std::size_t numPaths/*1000000*/,  std::size_t numDays/*252*/) const
{
    Returns returns;
    returns.m_returns.resize(numPaths * numDays);
    returns.m_blockSize = numDays;

    const double dt = 1.0 / static_cast<double>(numDays);
    const double dailyVolatility = volatility * std::sqrt(dt);
    const double dailyDrift = drift * dt;

    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);

    const std::size_t pathsPerThread = numPaths / NUM_THREADS;

    for(std::size_t threadIdx = 0; threadIdx < NUM_THREADS; ++threadIdx)
    {
        const std::size_t startPath = threadIdx * pathsPerThread;
        const std::size_t endPath = (threadIdx == NUM_THREADS - 1) ? numPaths : (threadIdx+ 1) * pathsPerThread;

        threads.emplace_back([&, threadIdx, startPath, endPath, dailyDrift, dailyVolatility]() {
            thread_local static GenNormalPCG rng;
            for(std::size_t path = startPath; path < endPath; ++path)
            {                
                std::size_t baseIdx = path * numDays;
                for(std::size_t dayIdx = 0; dayIdx < numDays; ++dayIdx)
                {
                    const std::size_t flatIdx = baseIdx + dayIdx;
                    returns.m_returns[flatIdx] = dailyDrift + dailyVolatility * rng();
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

std::pair<double, double> MonteCarloEngine::ComputeAssetStatistics(const std::size_t assetIdx, const std::vector<std::vector<double>>& assetReturns, bool annualise)
{
    assert(!assetReturns.empty());

    const double n = static_cast<double>(assetReturns.size());

    double sum = 0.0;
    for (const std::vector<double>& tReturns : assetReturns)
    {
        sum += tReturns[assetIdx];
    }
    double mean = sum / n;

    double sumSquaredDiffs = 0.0;
    for (const std::vector<double>& tReturns : assetReturns)
    {
        sumSquaredDiffs += std::pow(tReturns[assetIdx] - mean, 2);
    }

    const double variance = sumSquaredDiffs / (n - 1.0);
    double stddev = std::sqrt(variance);

    if (annualise)
    {
        mean = mean * 252.0;
        stddev = stddev * std::sqrt(252.0);
    }

    return { mean, stddev };
}

std::vector<std::pair<double, double>> MonteCarloEngine::ComputeMultiAssetStatistics(const std::vector<std::vector<double>>& returns, bool annualise)
{
    const std::size_t numDays = returns.size();
    assert(numDays > 0);

    const std::size_t numAssets = returns[0].size();
    assert(numAssets > 0);

    std::vector<std::pair<double, double>> statistics;

    for(std::size_t asset = 0; asset < numAssets; ++asset)
    {
        std::pair<double, double> assetStats = ComputeAssetStatistics(asset, returns, annualise);
        statistics.emplace_back(assetStats);
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
