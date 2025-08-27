#pragma once

#include <cstddef>

#include "../include/Portfolio.hpp"

namespace PortfolioUtils
{
    double GetMeanReturnOfSegment(const Portfolio& portfolio, std::size_t segmentDays);
    double GetMeanDailyReturn(const Portfolio& portfolio);
    double GetStandardDeviation(const Portfolio& portfolio);
    double GetVaR(const Portfolio& portfolio, double confidence = 0.95);
    double GetCVaR(const Portfolio& portfolio, double confidence = 0.95);
    double GetSharpeRatio(const Portfolio& portfolio);
}