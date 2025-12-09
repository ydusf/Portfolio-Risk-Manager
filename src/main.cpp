#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cassert>
#include <chrono>
#include <iomanip>

#include "../include/Globals.hpp"
#include "../include/Portfolio.hpp"
#include "../include/MonteCarloEngine.hpp"
#include "../include/DataHandler.hpp"
#include "../include/PortfolioUtils.hpp"
#include "../include/PortfolioOptimisation.hpp"

// EXAMPLE USAGE:
// ./run.sh NVDA=0.15 GOOGL=0.1 AGYS=0.08 AMZN=0.03 MU=0.06 MSFT=0.03 NU=0.04 LLY=0.085 UNH=0.2 NVO=0.225 2024-11-12 2025-10-18 

int main(int argc, char* argv[]) 
{
    std::pair<std::vector<std::string>, std::vector<double>> assetData = DataHandler::ParseAssetDataOutOfArguments(argc, argv);
    const std::vector<std::string>& tickers = assetData.first;
    const std::vector<double>& weights = assetData.second;

    Portfolio portfolio(assetData.first, assetData.second);

    const double totalReturn = PortfolioUtils::GetMeanReturnOfSegment(portfolio, portfolio.GetReturnSeries().size());
    const double mean10R     = PortfolioUtils::GetMeanReturnOfSegment(portfolio, 10);
    const double stddev      = PortfolioUtils::GetStandardDeviation(portfolio);
    const double VaR         = PortfolioUtils::GetVaR(portfolio);
    const double CVaR        = PortfolioUtils::GetCVaR(portfolio);
    const double sharpe      = PortfolioUtils::GetSharpeRatio(portfolio);

    std::cout << "\nCurrent Portfolio Risk Metrics:" << '\n';
    std::cout << "  Total Return:        " << std::fixed << std::setprecision(2) << totalReturn * 100 << "%" << '\n';
    std::cout << "  Mean 10-Day Return:  " << std::fixed << std::setprecision(2) << mean10R * 100 << "%" << '\n';
    std::cout << "  Volatility (STD):    " << stddev * 100 << "%" << '\n';
    std::cout << "  Value-at-Risk (VaR): " << VaR * 100 << "%" << '\n';
    std::cout << "  Conditional VaR:     " << CVaR * 100 << "%" << '\n';
    std::cout << "  Sharpe Ratio:        " << sharpe << "" << '\n';
    
    const std::vector<std::vector<double>> portfolioReturns = DataHandler::GetLogReturnsMat(portfolio.GetTickers());

    const std::vector<std::vector<double>> covMatrix = PortfolioOptimisation::CalculateCovarianceMatrix(portfolioReturns);
    const std::vector<double> expectedReturns = PortfolioOptimisation::CalculateExpectedAssetReturns(tickers, true);
    
    std::cout << "\nExpected Annual Returns:" << '\n';
    for (std::size_t i = 0; i < tickers.size(); ++i)
    {
        std::cout << "  " << tickers[i] << ": " << std::setprecision(2) << expectedReturns[i] * 100 << "%" << '\n';
    }

    const bool allowNegativeWeights = true;
    
    const PortfolioOptimisation::OptimisationResult minVolPortfolio = PortfolioOptimisation::MinimiseVolatility(covMatrix, expectedReturns, allowNegativeWeights);
    
    std::cout << "\nMinimum Volatility Portfolio:" << '\n';
    for (std::size_t i = 0; i < tickers.size(); ++i)
    {
        std::cout << "  " << tickers[i] << ": " << minVolPortfolio.weights[i] * 100 << "%" << '\n';
    }
    std::cout << "  Expected Return: " << minVolPortfolio.expectedReturn * 100 << "%" << '\n';
    std::cout << "  Volatility: " << minVolPortfolio.volatility * 100 << "%" << '\n';
    std::cout << "  Sharpe Ratio: " << minVolPortfolio.sharpeRatio << '\n';

    constexpr double riskFreeRate = 0;
    const PortfolioOptimisation::OptimisationResult maxSharpePortfolio = PortfolioOptimisation::MaximiseSharpeRatio(covMatrix, expectedReturns, allowNegativeWeights, riskFreeRate);
    
    std::cout << "\nMaximum Sharpe Ratio Portfolio:" << '\n';
    for (std::size_t i = 0; i < tickers.size(); ++i)
    {
        std::cout << "  " << tickers[i] << ": " << maxSharpePortfolio.weights[i] * 100 << "%" << '\n';
    }
    std::cout << "  Expected Return: " << maxSharpePortfolio.expectedReturn * 100 << "%" << '\n';
    std::cout << "  Volatility: " << maxSharpePortfolio.volatility * 100 << "%" << '\n';
    std::cout << "  Sharpe Ratio: " << maxSharpePortfolio.sharpeRatio << '\n';
    
    const double currentVar = PortfolioOptimisation::CalculatePortfolioVariance(weights, covMatrix);
    const double currentVol = std::sqrt(currentVar);
    const double currentRet = PortfolioOptimisation::CalculatePortfolioReturn(weights, expectedReturns);
    
    std::cout << "\nCurrent Portfolio: " << '\n';
    for (std::size_t i = 0; i < tickers.size(); ++i)
    {
        std::cout << "  " << tickers[i] << ": " << weights[i] * 100 << "%" << '\n';
    }
    std::cout << "  Expected Return: " << currentRet * 100 << "%" << '\n';
    std::cout << "  Volatility: " << currentVol * 100 << "%" << '\n';
    std::cout << "  Sharpe Ratio: " << (currentRet - riskFreeRate) / currentVol << '\n';
    
    const std::string portfolioFilename = "../data/optimised_portfolios.csv";
    DataHandler::WritePortfoliosToCSV(portfolioFilename,
                                      tickers,
                                      currentRet,
                                      currentVol,
                                      riskFreeRate,
                                      weights,
                                      minVolPortfolio,
                                      maxSharpePortfolio);

    std::cout << "\nComputing Efficient Frontier (50 points):" << '\n';
    const PortfolioOptimisation::EfficientFrontier frontier = PortfolioOptimisation::ComputeEfficientFrontier(covMatrix, expectedReturns, 50);
    if (frontier.returns.size() > 0)
    {
        std::cout << "  Generated " << frontier.returns.size() << " efficient portfolios" << '\n';
        std::cout << "  Max Sharpe portfolio is at index " << frontier.maxSharpeIndex << '\n';
        std::cout << "  Min volatility portfolio is at index " << frontier.minVolIndex << '\n';
        DataHandler::WriteEfficientFrontierToCSV(frontier, "../data/efficient_frontier.csv");
    }

    MonteCarloEngine mce;

    const std::vector<std::pair<double, double>> assetStatistics = mce.ComputeMultiAssetStatistics(portfolioReturns);

    const Eigen::MatrixXd choleskyMatrix = PortfolioOptimisation::GetCholeskyMatrix(covMatrix);

    constexpr std::size_t NUM_SIMS = 1000000;
    constexpr std::size_t NUM_DAYS = 252;
    auto start = std::chrono::high_resolution_clock::now();
    Returns returns = mce.GenerateReturnsForMultiAsset(choleskyMatrix, assetStatistics, NUM_SIMS, NUM_DAYS);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // Returns pricePaths = mce.BuildPricePaths(returns, 100);
    // DataHandler::WritePathsToCSV(pricePaths, "../data/multi_assets_paths.csv");

    std::cout << "\nMonte Carlo Simulation:" << '\n';
    std::cout << "  Simulated paths: " << NUM_SIMS << "" << '\n';
    std::cout << "  Days per path: " << NUM_DAYS << "" << '\n';
    std::cout << "  Number of assets: " << tickers.size() << "" << '\n';
    std::cout << "  Time taken: " << duration << " ms" << '\n';
}
