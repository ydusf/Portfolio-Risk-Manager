#include <iostream>
#include <functional>
#include <fstream>
#include <sstream>
#include <string>
#include <cassert>

#include "../include/Globals.hpp"
#include "../include/Portfolio.hpp"
#include "../include/MonteCarloEngine.hpp"
#include "../include/DataHandler.hpp"

// EXAMPLE: ./run.sh NVDA=0.15 GOOGL=0.1 AGYS=0.08 AMZN=0.03 MU=0.06 MSFT=0.03 NU=0.04 LLY=0.085 UNH=0.2 NVO=0.225

int main(int argc, char* argv[]) 
{
    if (argc < 2) 
    {
        std::cerr << "Usage: " << argv[0] << " TICKER=WEIGHT [...]\n";
        return 1;
    }

    std::vector<std::string> tickers;
    std::vector<double> weights;

    for (int i = 1; i < argc; ++i) 
    {
        std::string arg = argv[i];
        auto eqPos = arg.find('=');
        if (eqPos == std::string::npos) 
        {
            std::cerr << "Invalid argument: " << arg << " (expected TICKER=WEIGHT)\n";
            return 1;
        }

        std::string ticker = arg.substr(0, eqPos);
        std::string weightStr = arg.substr(eqPos + 1);

        try 
        {
            double weight = std::stod(weightStr);
            tickers.push_back(ticker);
            weights.push_back(weight);
        } 
        catch (const std::exception&) 
        {
            std::cerr << "Invalid weight for " << ticker << ": " << weightStr << '\n';
            return 1;
        }
    }

    assert(tickers.size() == weights.size());

    std::cout << "Parsed tickers and weights:\n";
    for (std::size_t i = 0; i < tickers.size(); ++i) 
    {
        std::cout << tickers[i] << " -> " << weights[i] << '\n';
    }

    Portfolio portfolio(tickers, weights);

    const double mean10R = portfolio.GetMeanReturnOfSegment(10);
    const double stddev = portfolio.GetStandardDeviation();
    const double VaR = portfolio.GetVaR();
    const double CVaR = portfolio.GetCVaR();
    const double sharpeRatio = portfolio.GetSharpeRatio();

    std::cout << "Mean 10 Day Return: " << mean10R * 100 << "%" << '\n';
    std::cout << "STD: " << stddev * 100 << "%" << '\n';
    std::cout << "Portfolio VaR: " << VaR * 100 << "%" << '\n';
    std::cout << "Portfolio CVaR: " << CVaR * 100 << "%" << '\n';
    std::cout << "Portfolio Sharpe Ratio: " << sharpeRatio << '\n';

    MonteCarloEngine mce;

    std::vector<std::vector<double>> logReturns = DataHandler::GetLogReturnsMat(portfolio.GetTickers());

    auto [portfolioMean, portfolioStd] = mce.CalculatePortfolioStatistics(logReturns, portfolio);

    auto start = std::chrono::system_clock::now();
    Returns returns = mce.GenerateReturns(portfolioMean, portfolioStd, 100);
    auto end = std::chrono::system_clock::now();

    Returns prices = mce.BuildPricePaths(returns, 100);

    DataHandler::WritePathsToCSV(prices, "../paths/monte_carlo_simul6.csv");

    std::cout << "Time taken: " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << "ms" <<'\n';
}