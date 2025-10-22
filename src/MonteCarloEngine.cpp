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


Returns MonteCarloEngine::GenerateReturnsForMultiAsset(const Eigen::MatrixXd& choleskyMatrix, const std::vector<std::pair<double, double>>& assetStatistics, std::size_t numPaths,  std::size_t numDays)
{    
    const std::size_t numAssets = assetStatistics.size();
    assert(choleskyMatrix.rows() == numAssets && choleskyMatrix.cols() == numAssets);

    constexpr double dt = 1.0 / 252.0;
    
    Eigen::VectorXd dailyDrifts(numAssets);
    Eigen::VectorXd dailyVolatilities(numAssets);
    
    for(std::size_t i = 0; i < numAssets; ++i)
    {
        dailyDrifts(i) = assetStatistics[i].first * dt;
        dailyVolatilities(i) = assetStatistics[i].second * std::sqrt(dt);
    }
    
    const double driftSum = dailyDrifts.sum();
    
    Returns returns;
    returns.m_returns.resize(numPaths * numDays);
    returns.m_blockSize = numDays;

    std::vector<std::thread> threads;
    threads.reserve(m_NUM_THREADS);

    const std::size_t pathsPerThread = numPaths / m_NUM_THREADS;
    constexpr std::size_t BATCH_SIZE = 1000;

    for(std::size_t threadIdx = 0; threadIdx < m_NUM_THREADS; ++threadIdx)
    {
        const std::size_t startPath = threadIdx * pathsPerThread;
        const std::size_t endPath = (threadIdx == m_NUM_THREADS - 1) ? numPaths : (threadIdx + 1) * pathsPerThread;

        threads.emplace_back([&, startPath, endPath]() {
            thread_local static GenNormalPCG rng;
            
            Eigen::MatrixXd independentShocks(numAssets, numDays * BATCH_SIZE);
            Eigen::MatrixXd correlatedShocks(numAssets, numDays * BATCH_SIZE);
            
            for(std::size_t batchStart = startPath; batchStart < endPath; batchStart += BATCH_SIZE)
            {
                const std::size_t batchEnd = std::min(batchStart + BATCH_SIZE, endPath);
                const std::size_t currentBatchSize = batchEnd - batchStart;
                const std::size_t totalSteps = numDays * currentBatchSize;
                
                double* shockData = independentShocks.data();
                for(std::size_t i = 0; i < numAssets * totalSteps; ++i)
                {
                    shockData[i] = rng();
                }
                
                correlatedShocks.leftCols(totalSteps) = choleskyMatrix * independentShocks.leftCols(totalSteps);
                
                for(std::size_t pathIdx = 0; pathIdx < currentBatchSize; ++pathIdx)
                {
                    const std::size_t path = batchStart + pathIdx;
                    const std::size_t baseIdx = path * numDays;
                    const std::size_t columnOffset = pathIdx * numDays;
                    
                    for(std::size_t dayIdx = 0; dayIdx < numDays; ++dayIdx)
                    {
                        const std::size_t colIdx = columnOffset + dayIdx;
                        double totalDailyReturns = driftSum + dailyVolatilities.dot(correlatedShocks.col(colIdx));
                        returns.m_returns[baseIdx + dayIdx] = totalDailyReturns;
                    }
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

Returns MonteCarloEngine::GenerateReturnsForSingleAsset(double drift, double volatility, std::size_t numPaths/*1000000*/,  std::size_t numDays/*252*/)
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

std::pair<double, double> MonteCarloEngine::ComputeAssetStatistics(const std::size_t assetIdx, const std::vector<std::vector<double>>& assetReturns)
{
    assert(!assetReturns.empty());

    const double n = static_cast<double>(assetReturns.size());

    double sum = 0.0;
    for (const std::vector<double>& tReturns : assetReturns)
    {
        sum += tReturns[assetIdx];
    }
    const double mean = sum / n;

    double sumSquaredDiffs = 0.0;
    for (const std::vector<double>& tReturns : assetReturns)
    {
        sumSquaredDiffs += std::pow(tReturns[assetIdx] - mean, 2);
    }

    const double variance = sumSquaredDiffs / (n - 1.0);
    const double stddev = std::sqrt(variance);

    return { mean, stddev };
}

std::vector<std::pair<double, double>> MonteCarloEngine::ComputeMultiAssetStatistics(const std::vector<std::vector<double>>& returns)
{
    const std::size_t numDays = returns.size();
    assert(numDays > 0);

    const std::size_t numAssets = returns[0].size();
    assert(numAssets > 0);

    std::vector<std::pair<double, double>> statistics;

    for(std::size_t asset = 0; asset < numAssets; ++asset)
    {
        std::pair<double, double> assetStats = ComputeAssetStatistics(asset, returns);
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
