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

    m_dailyReturnSeries = GetReturns();
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

double Portfolio::GetMeanReturnOfSegment(std::size_t segmentDays) const
{
    if (segmentDays == 0 || m_dailyReturnSeries.empty())
        return 0.0;

    std::vector<double> segmentReturns;
    double segmentProduct = 1.0;
    std::size_t currentSegSize = 0;

    for (const double dailyReturn : m_dailyReturnSeries)
    {
        segmentProduct *= (1.0 + dailyReturn);
        ++currentSegSize;

        if (currentSegSize == segmentDays)
        {
            segmentReturns.push_back(segmentProduct - 1.0);

            segmentProduct = 1.0;
            currentSegSize = 0;
        }
    }

    if (currentSegSize != 0)
    {
        segmentReturns.push_back(segmentProduct - 1.0);
    }

    if (segmentReturns.empty())
        return 0.0;

    const double total = std::accumulate(segmentReturns.begin(), segmentReturns.end(), 0.0);
    return total / static_cast<double>(segmentReturns.size());
}


double Portfolio::GetMeanDailyReturn() const
{
    return GetMeanReturnOfSegment(1);
}

double Portfolio::GetStandardDeviation() const
{
    if (m_dailyReturnSeries.empty()) 
        return 0.0;

    const double mean = GetMeanDailyReturn();

    double sqDiffSum = 0.0;
    for (double value : m_dailyReturnSeries) 
    {
        sqDiffSum += std::pow(value - mean, 2);
    }

    return std::sqrt(sqDiffSum / m_dailyReturnSeries.size());
}

double Portfolio::GetVaR(double confidence) const
{
    if(m_dailyReturnSeries.empty())
        return 0.0;
    
    std::vector<double> sorted = m_dailyReturnSeries;
    std::sort(sorted.begin(), sorted.end());
    
    std::size_t index = static_cast<std::size_t>((1.0 - confidence) * sorted.size());
    return -sorted[index];
}

double Portfolio::GetCVaR(double confidence) const
{
    if(m_dailyReturnSeries.empty())
        return 0.0;
    
    std::vector<double> sorted = m_dailyReturnSeries;
    std::sort(sorted.begin(), sorted.end());
    
    std::size_t index = static_cast<std::size_t>((1.0 - confidence) * sorted.size());
    double var_threshold = sorted[index];
    
    double sum = 0.0;
    std::size_t count = 0;
    for(double r : sorted) 
    {
        if(r <= var_threshold) 
        {
            sum += r;
            ++count;
        }
    }
    
    return -sum / count;
}   

double Portfolio::GetSharpeRatio() const
{
    double meanDailyReturn = GetMeanDailyReturn();
    double volatility = GetStandardDeviation();

    return (meanDailyReturn / volatility) * std::sqrt(252);
};


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