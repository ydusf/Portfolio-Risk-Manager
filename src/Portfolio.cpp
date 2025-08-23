#include <cmath>
#include <algorithm>
#include <numeric>
#include <cassert>

#include "../include/Portfolio.hpp"
#include "../include/DataHandler.hpp"

/* -------------------------- PUBLIC METHODS ------------------------------ */

Portfolio::Portfolio(const std::vector<std::string>& tickers, const std::vector<double>& weights)
    : m_tickers(tickers), m_weights(weights)
{
    assert(m_tickers.size() == m_weights.size());

    for(int i = 0; i < m_tickers.size(); ++i)
    {
        m_assets.insert({m_tickers[i], m_weights[i]});
    }
}

Portfolio::~Portfolio()
{};

double Portfolio::GetWeight(const std::string& ticker)
{
    auto itr = m_assets.find(ticker);
    if(itr != m_assets.end())
    {
        return itr->second;
    }

    return 0.0;
}

const std::vector<std::string>& Portfolio::GetTickers()
{
    return m_tickers;
}

const std::vector<double>& Portfolio::GetWeights()
{
    return m_weights;
}


const std::map<std::string, double>& Portfolio::GetAssets()
{
    return m_assets;
}

double Portfolio::GetVaR(double confidence)
{
    std::vector<double> returns = GetReturns();
    if(returns.empty())
        return 0.0;

    std::vector<double> sorted = returns;
    std::sort(sorted.begin(), sorted.end());

    std::size_t idx = static_cast<std::size_t>(std::floor((1.0 - confidence) * sorted.size()));

    idx = std::min(idx, sorted.size()-1);

    return sorted[idx];
}

double Portfolio::GetStandardDeviation()
{
    std::vector<double> returns = GetReturns();
    if (returns.empty()) 
        return 0.0;

    double sum = std::accumulate(returns.begin(), returns.end(), 0.0);
    double mean = sum / returns.size();

    double sqDiffSum = 0.0;
    for (double value : returns) 
    {
        sqDiffSum += std::pow(value - mean, 2);
    }

    return std::sqrt(sqDiffSum / returns.size());
}


/* -------------------------- PRIVATE METHODS ------------------------------ */

std::vector<double> Portfolio::GetReturns()
{
    const std::vector<std::vector<double>>& returnsMat = DataHandler::GetReturnsMat(m_tickers);

    std::map<std::size_t, std::string> idxTickerMap;
    for(std::size_t i = 0; i < m_tickers.size(); ++i)
    {
        idxTickerMap.insert({i, m_tickers[i]});
    }

    std::vector<double> returns;
    for(std::size_t row = 0; row < returnsMat.size(); ++row)
    {
        double weightedReturnT = 0.0;
        for(std::size_t col = 0; col < returnsMat[row].size(); ++col)
        {
            auto tickerItr = idxTickerMap.find(col);
            if(tickerItr == idxTickerMap.end())
                continue;

            auto weightItr = m_assets.find(tickerItr->second);
            if(weightItr == m_assets.end())
                continue;

            weightedReturnT += weightItr->second * returnsMat[row][col];
        }

        returns.push_back(weightedReturnT);
    }

    return returns;
}