#include "../include/PortfolioUtils.hpp"

#include <vector>
#include <numeric>
#include <cmath>
#include <algorithm>

namespace PortfolioUtils
{
    double GetMeanReturnOfSegment(const Portfolio& portfolio, std::size_t segmentDays)
    {
        const std::vector<double>& dailyReturnSeries = portfolio.GetReturnSeries();

        if (segmentDays == 0 || dailyReturnSeries.empty())
            return 0.0;

        std::vector<double> segmentReturns;
        double segmentProduct = 1.0;
        std::size_t currentSegSize = 0;

        for (const double dailyReturn : dailyReturnSeries)
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


    double GetMeanDailyReturn(const Portfolio& portfolio)
    {
        return PortfolioUtils::GetMeanReturnOfSegment(portfolio, 1);
    }

    double GetStandardDeviation(const Portfolio& portfolio)
    {
        const std::vector<double>& dailyReturnSeries = portfolio.GetReturnSeries();
        if (dailyReturnSeries.empty()) 
            return 0.0;

        const double mean = PortfolioUtils::GetMeanDailyReturn(portfolio);

        double sqDiffSum = 0.0;
        for (double value : dailyReturnSeries) 
        {
            sqDiffSum += std::pow(value - mean, 2);
        }

        return std::sqrt(sqDiffSum / dailyReturnSeries.size());
    }

    double GetVaR(const Portfolio& portfolio, double confidence)
    {
        const std::vector<double>& dailyReturnSeries = portfolio.GetReturnSeries();
        if(dailyReturnSeries.empty())
            return 0.0;
        
        std::vector<double> sorted = dailyReturnSeries;
        std::sort(sorted.begin(), sorted.end());
        
        std::size_t index = static_cast<std::size_t>((1.0 - confidence) * sorted.size());
        return -sorted[index];
    }

    double GetCVaR(const Portfolio& portfolio, double confidence)
    {
        const std::vector<double>& dailyReturnSeries = portfolio.GetReturnSeries();
        if(dailyReturnSeries.empty())
            return 0.0;
        
        std::vector<double> sorted = dailyReturnSeries;
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

    double GetSharpeRatio(const Portfolio& portfolio)
    {
        double meanDailyReturn = PortfolioUtils::GetMeanDailyReturn(portfolio);
        double volatility = PortfolioUtils::GetStandardDeviation(portfolio);

        return (meanDailyReturn / volatility) * std::sqrt(252);
    };
}