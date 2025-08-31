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

// EXAMPLE USAGE:
// ./run.sh NVDA=0.15 GOOGL=0.1 AGYS=0.08 AMZN=0.03 MU=0.06 MSFT=0.03 NU=0.04 LLY=0.085 UNH=0.2 NVO=0.225

int main(int argc, char* argv[]) 
{
    if (argc < 2) 
    {
        std::cerr << "Usage: " << argv[0] << " TICKER=WEIGHT [...]" << '\n';
        return 1;
    }

    std::vector<std::string> tickers;
    std::vector<double> weights;

    for (std::size_t i = 1; i < argc; ++i) 
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

    std::cout << "Parsed tickers and normalized weights:" << '\n';
    for (std::size_t i = 0; i < tickers.size(); ++i) 
    {
        std::cout << "  " << tickers[i] << " -> " << std::fixed << std::setprecision(3) << weights[i] * 100 << "%" << '\n';
    }

    Portfolio portfolio(tickers, weights);

    const double totalReturn = PortfolioUtils::GetMeanReturnOfSegment(portfolio, portfolio.GetReturnSeries().size());
    const double mean10R     = PortfolioUtils::GetMeanReturnOfSegment(portfolio, 10);
    const double stddev      = PortfolioUtils::GetStandardDeviation(portfolio);
    const double VaR         = PortfolioUtils::GetVaR(portfolio);
    const double CVaR        = PortfolioUtils::GetCVaR(portfolio);
    const double sharpe      = PortfolioUtils::GetSharpeRatio(portfolio);

    std::cout << "\nPortfolio Risk Metrics:" << '\n';
    std::cout << "  Total Return:        " << std::fixed << std::setprecision(2) << totalReturn * 100 << "%" << '\n';
    std::cout << "  Mean 10-Day Return:  " << std::fixed << std::setprecision(2) << mean10R * 100 << "%" << '\n';
    std::cout << "  Volatility (STD):    " << stddev * 100 << "%" << '\n';
    std::cout << "  Value-at-Risk (VaR): " << VaR * 100 << "%" << '\n';
    std::cout << "  Conditional VaR:     " << CVaR * 100 << "%" << '\n';
    std::cout << "  Sharpe Ratio:        " << sharpe << "" << '\n';

    MonteCarloEngine mce;
    std::vector<std::vector<double>> logReturns = DataHandler::GetLogReturnsMat(portfolio.GetTickers());
    auto [portfolioMean, portfolioStd] = mce.CalculatePortfolioStatistics(logReturns, portfolio);

    constexpr int NUM_SIMS = 1'000'000;
    auto start = std::chrono::high_resolution_clock::now();
    Returns returns = mce.GenerateReturns(portfolioMean, portfolioStd, NUM_SIMS);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "\nMonte Carlo Simulation:" << '\n';
    std::cout << "  Runs: " << NUM_SIMS << "" << '\n';
    std::cout << "  Time taken: " << duration << " ms" << '\n';

    return 0;
}
