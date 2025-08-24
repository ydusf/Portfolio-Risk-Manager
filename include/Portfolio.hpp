#pragma once

#include <map>
#include <string>
#include <vector>
#include <memory>

class Portfolio
{
private:
    std::vector<std::string> m_tickers;
    std::vector<double> m_weights;
    std::map<std::string, double> m_assets;

    std::vector<double> m_dailyReturnSeries;

public:
    Portfolio(const std::vector<std::string>& tickers, const std::vector<double>& weights);

    double GetWeight(const std::string& ticker);
    const std::vector<std::string>& GetTickers();
    const std::vector<double>& GetWeights();
    const std::map<std::string, double>& GetAssets();

    double GetVaR(double confidence = 0.95);
    double GetStandardDeviation();
    double GetMeanReturn(std::size_t segmentDays);

    ~Portfolio();

private:
    std::vector<double> GetReturns();
};