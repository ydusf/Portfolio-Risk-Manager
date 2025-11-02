#pragma once

#include <vector>
#include <string>

namespace PortfolioOptimisation
{
    struct OptimisationResult 
    {
        std::vector<double> weights;
        double expectedReturn;
        double volatility;
        double sharpeRatio;
    };

    struct EfficientFrontier 
    {
        std::vector<double> returns;
        std::vector<double> volatilities;
        std::vector<std::vector<double>> weights;
        int maxSharpeIndex;
        int minVolIndex;
    };

    OptimisationResult MinimiseVolatility(const std::vector<std::vector<double>>& covMatrix, const std::vector<double>& expectedReturns, bool allowNegativeWeights);
    OptimisationResult MaximiseSharpeRatio(const std::vector<std::vector<double>>& covMatrix, const std::vector<double>& expectedReturns, bool allowNegativeWeights, double riskFreeRate = 0.0);
    OptimisationResult OptimiseForTargetReturn(const std::vector<std::vector<double>>& covMatrix, const std::vector<double>& expectedReturns, double targetReturn, bool allowNegativeWeights);
    EfficientFrontier ComputeEfficientFrontier(const std::vector<std::vector<double>>& covMatrix, const std::vector<double>& expectedReturns, int numPoints);
    
    Eigen::MatrixXd GetCholeskyMatrix(const std::vector<std::vector<double>>& covMatrix);
    std::vector<std::vector<double>> CalculateCovarianceMatrix(const std::vector<std::vector<double>>& logReturnsMat);
    std::vector<double> CalculateExpectedAssetReturns(const std::vector<std::string>& tickers, bool annualise);
    double CalculatePortfolioVariance(const std::vector<double>& weights, const std::vector<std::vector<double>>& covMatrix);
    double CalculatePortfolioReturn(const std::vector<double>& weights, const std::vector<double>& expectedReturns);
};