#pragma once

#include <vector>
#include <thread>

#include "Eigen/Dense"

#include "Portfolio.hpp"
#include "RandomGenerator.hpp"

static const std::size_t NUM_THREADS = std::thread::hardware_concurrency();

struct Returns
{
    std::vector<double> m_returns;
    std::size_t m_blockSize;
};

class MonteCarloEngine 
{
public:
    MonteCarloEngine();
    ~MonteCarloEngine();
    
    std::pair<double, double> ComputeAssetStatistics(const std::size_t assetIdx, const std::vector<std::vector<double>>& assetReturns);
    std::vector<std::pair<double, double>> ComputeMultiAssetStatistics(const std::vector<std::vector<double>>& returns);
    std::vector<double> CombineAssetReturns(const Portfolio& portfolio);
    Returns GenerateReturnsForMultiAsset(const Eigen::MatrixXd& choleskyMatrix, const std::vector<std::pair<double, double>>& assetStatistics, std::size_t numPaths = 1000000, std::size_t numDays = 252) const;
    Returns GenerateReturnsForSingleAsset(double drift, double volatility, std::size_t numPaths = 1000000, std::size_t numDays = 252) const;
    Returns BuildPricePaths(const Returns& returns, double initialPrice);
};