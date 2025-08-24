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

    double GetWeight(const std::string& ticker) const;
    const std::vector<std::string>& GetTickers() const;
    const std::vector<double>& GetWeights() const;
    const std::map<std::string, double>& GetAssets() const;

    double GetVaR(double confidence = 0.95) const;
    double GetStandardDeviation() const;
    double GetMeanReturn(std::size_t segmentDays) const;

    ~Portfolio();

private:
    std::vector<double> GetReturns();
};