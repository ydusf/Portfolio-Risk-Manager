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

    m_dailyReturnSeries = CreateReturnSeries();
}

Portfolio::~Portfolio()
{};

double Portfolio::GetWeight(const std::string& ticker) const
{
    auto itr = m_assets.find(ticker);
    if(itr != m_assets.end())
    {
        return itr->second;
    }

    return 0.0;
}

const std::vector<std::string>& Portfolio::GetTickers() const
{
    return m_tickers;
}

const std::vector<double>& Portfolio::GetWeights() const
{
    return m_weights;
}


const std::map<std::string, double>& Portfolio::GetAssets() const
{
    return m_assets;
}

const std::vector<double>& Portfolio::GetReturnSeries() const
{
    return m_dailyReturnSeries;
}


/* -------------------------- PRIVATE METHODS ------------------------------ */

std::vector<double> Portfolio::CreateReturnSeries() const 
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