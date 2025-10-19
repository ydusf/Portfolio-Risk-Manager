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

    OptimisationResult MinimiseVolatility(const std::vector<std::vector<double>>& covMatrix, const std::vector<double>& expectedReturns);
    OptimisationResult MaximiseSharpeRatio(const std::vector<std::vector<double>>& covMatrix, const std::vector<double>& expectedReturns, double riskFreeRate = 0.0);
    OptimisationResult OptimiseForTargetReturn(const std::vector<std::vector<double>>& covMatrix, const std::vector<double>& expectedReturns, double targetReturn);
    EfficientFrontier ComputeEfficientFrontier(const std::vector<std::vector<double>>& covMatrix, const std::vector<double>& expectedReturns, int numPoints);
    
    std::vector<std::vector<double>> CalculateCovarianceMatrix(const std::vector<std::string>& tickers);
    std::vector<double> CalculateExpectedLogReturns(const std::vector<std::string>& tickers, bool annualise);
    double CalculatePortfolioVariance(const std::vector<double>& weights, const std::vector<std::vector<double>>& covMatrix);
    double CalculatePortfolioReturn(const std::vector<double>& weights, const std::vector<double>& expectedReturns);
};