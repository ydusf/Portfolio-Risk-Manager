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
    if (argc < 2) 
    {
        std::cerr << "Usage: " << argv[0] << " TICKER=WEIGHT [...]" << '\n';
        return 1;
    }

    std::vector<std::string> tickers;
    std::vector<double> weights;

    for (std::size_t i = 1; i < argc-2; ++i) 
    {
        std::string arg = argv[i];
        std::size_t eqPos = arg.find('=');
        if (eqPos == std::string::npos) 
        {
            std::cerr << "Invalid argument: " << arg << " (expected TICKER=WEIGHT)" << '\n';
            return 1;
        }

        std::string ticker = arg.substr(0, eqPos);
        std::string weightStr = arg.substr(eqPos + 1);

        try 
        {
            double weight = std::stod(weightStr);
            if (weight <= 0.0) 
            {   
                std::cerr << "Weight for " << ticker << " must be positive." << '\n';
                return 1;
            }
            tickers.push_back(ticker);
            weights.push_back(weight);
        } 
        catch (const std::exception&) 
        {
            std::cerr << "Invalid weight for " << ticker << ": " << weightStr << '\n';
            return 1;
        }
    }

    if (tickers.size() != weights.size()) 
    {
        std::cerr << "Error: mismatch between tickers and weights." << '\n';
        return 1;
    }

    double totalWeight = 0.0;
    for (double w : weights) 
    {
        totalWeight += w;
    }

    if (totalWeight <= 0.0) 
    {
        std::cerr << "Error: total portfolio weight must be > 0." << '\n';
        return 1;
    }

    for (double& w : weights) 
    {
        w /= totalWeight;
    }

    std::cout << "Parsed tickers and normalised weights:" << '\n';
    for (std::size_t i = 0; i < tickers.size(); ++i) 
    {
        std::cout << "  " << tickers[i] << "  ->  " << std::fixed << std::setprecision(3) << weights[i] * 100 << "%" << '\n';
    }

    Portfolio portfolio(tickers, weights);

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
    
    const PortfolioOptimisation::OptimisationResult minVolPortfolio = PortfolioOptimisation::MinimiseVolatility(covMatrix, expectedReturns);
    
    std::cout << "\nMinimum Volatility Portfolio:" << '\n';
    for (std::size_t i = 0; i < tickers.size(); ++i)
    {
        std::cout << "  " << tickers[i] << ": " << minVolPortfolio.weights[i] * 100 << "%" << '\n';
    }
    std::cout << "  Expected Return: " << minVolPortfolio.expectedReturn * 100 << "%" << '\n';
    std::cout << "  Volatility: " << minVolPortfolio.volatility * 100 << "%" << '\n';
    std::cout << "  Sharpe Ratio: " << minVolPortfolio.sharpeRatio << '\n';

    constexpr double riskFreeRate = 0;
    const PortfolioOptimisation::OptimisationResult maxSharpePortfolio = PortfolioOptimisation::MaximiseSharpeRatio(covMatrix, expectedReturns, riskFreeRate);
    
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
    
    std::cout << "\nComputing Efficient Frontier (50 points):" << '\n';
    const PortfolioOptimisation::EfficientFrontier frontier = PortfolioOptimisation::ComputeEfficientFrontier(covMatrix, expectedReturns, 50);
    if (frontier.returns.size() > 0)
    {
        std::cout << "  Generated " << frontier.returns.size() << " efficient portfolios" << '\n';
        std::cout << "  Max Sharpe portfolio is at index " << frontier.maxSharpeIndex << '\n';
        std::cout << "  Min volatility portfolio is at index " << frontier.minVolIndex << '\n';
        DataHandler::WriteEfficientFrontierToCSV(frontier, "../data/efficient_frontier.csv");
    }

    // MonteCarloEngine mce;
    // const std::vector<double> combinedPortfolioReturns = mce.CombineAssetReturns(portfolio);
    // auto [portfolioMean, portfolioStd] = mce.ComputeAssetStatistics(combinedPortfolioReturns);

    // constexpr int NUM_SIMS = 200;
    // auto start = std::chrono::high_resolution_clock::now();
    // Returns returns = mce.GenerateReturnsForSingleAsset(portfolioMean, portfolioStd, NUM_SIMS);
    // auto end = std::chrono::high_resolution_clock::now();

    // auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // Returns pricePaths = mce.BuildPricePaths(returns, 100);
    // DataHandler::WritePathsToCSV(pricePaths, "../data/single_asset_paths.csv");

    MonteCarloEngine mce;

    const std::vector<std::pair<double, double>> assetStatistics = mce.ComputeMultiAssetStatistics(portfolioReturns);

    const Eigen::MatrixXd choleskyMatrix = PortfolioOptimisation::GetCholeskyMatrix(covMatrix);

    constexpr int NUM_SIMS = 50;
    auto start = std::chrono::high_resolution_clock::now();
    Returns returns = mce.GenerateReturnsForMultiAsset(choleskyMatrix, assetStatistics, NUM_SIMS);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    Returns pricePaths = mce.BuildPricePaths(returns, 100);
    DataHandler::WritePathsToCSV(pricePaths, "../data/multi_assets_paths.csv");

    std::cout << "\nMonte Carlo Simulation:" << '\n';
    std::cout << "  Runs: " << NUM_SIMS << "" << '\n';
    std::cout << "  Time taken: " << duration << " ms" << '\n';
}
