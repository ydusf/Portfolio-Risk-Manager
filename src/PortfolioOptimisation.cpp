#include <cmath>
#include <numeric>
#include <algorithm>
#include <stdexcept>

#include <Eigen/Dense>

#include "../include/PortfolioOptimisation.hpp"
#include "../include/DataHandler.hpp"

namespace PortfolioOptimisation
{
    PortfolioOptimisation::OptimisationResult MinimiseVolatility(const std::vector<std::vector<double>>& covMatrix, const std::vector<double>& expectedReturns)
    {
        PortfolioOptimisation::OptimisationResult result;
        const std::size_t n = covMatrix.size();
        
        assert(n != 0 && covMatrix[0].size() == n);
        assert(expectedReturns.size() == n);
        
        Eigen::MatrixXd sigma(n, n);
        for (int i = 0; i < n; ++i)
        {
            for (int j = 0; j < n; ++j)
            {
                sigma(i, j) = covMatrix[i][j];
            }
        }
        
        const Eigen::LLT<Eigen::MatrixXd> llt(sigma);
        assert(llt.info() != Eigen::NumericalIssue);

        const Eigen::VectorXd ones = Eigen::VectorXd::Ones(n);
        const Eigen::VectorXd sigmaInvOnes = sigma.llt().solve(ones);
        const double denominator = ones.transpose() * sigmaInvOnes;
        assert(std::abs(denominator) >= 1e-10);
        
        const Eigen::VectorXd w = sigmaInvOnes / denominator;
        
        result.weights.resize(n);
        for (int i = 0; i < n; ++i)
        {
            result.weights[i] = w(i);
        }
        
        const double sumWeights = std::accumulate(result.weights.begin(), result.weights.end(), 0.0);
        assert(std::abs(sumWeights - 1.0) <= 1e-6);
        
        const double variance = (w.transpose() * sigma * w)(0);
        result.volatility = std::sqrt(variance);
        
        result.expectedReturn = 0.0;
        for (int i = 0; i < n; ++i)
        {
            result.expectedReturn += result.weights[i] * expectedReturns[i];
        }
        
        result.sharpeRatio = result.expectedReturn / result.volatility;
            
        return result;
    }

    PortfolioOptimisation::OptimisationResult MaximiseSharpeRatio(const std::vector<std::vector<double>>& covMatrix, const std::vector<double>& expectedReturns, double riskFreeRate)
    {
        PortfolioOptimisation::OptimisationResult result;
        const std::size_t n = covMatrix.size();
        
        assert(n != 0 && covMatrix[0].size() == n);
        assert(expectedReturns.size() == n);
        
        Eigen::MatrixXd sigma(n, n);
        Eigen::VectorXd mu(n);
        
        for (int i = 0; i < n; ++i)
        {
            mu(i) = expectedReturns[i];
            for (int j = 0; j < n; ++j)
            {
                sigma(i, j) = covMatrix[i][j];
            }
        }
        
        const Eigen::LLT<Eigen::MatrixXd> llt(sigma);
        assert(llt.info() != Eigen::NumericalIssue);
        
        const Eigen::VectorXd excessReturns = mu.array() - riskFreeRate;
        assert(excessReturns.norm() >= 1e-10);
        
        const Eigen::VectorXd w_unnormalized = sigma.llt().solve(excessReturns);
        const double sumWeights = w_unnormalized.sum();
        assert(std::abs(sumWeights) >= 1e-10);
        
        const Eigen::VectorXd w = w_unnormalized / sumWeights;
        
        result.weights.resize(n);
        for (int i = 0; i < n; ++i)
        {
            result.weights[i] = w(i);
        }
        
        const double weightsSum = std::accumulate(result.weights.begin(), result.weights.end(), 0.0);
        if (std::abs(weightsSum - 1.0) > 1e-6)
        {
            for (int i = 0; i < n; ++i)
            {
                result.weights[i] /= weightsSum;
            }
        }
        
        const double variance = (w.transpose() * sigma * w)(0);
        result.volatility = std::sqrt(variance);
        
        result.expectedReturn = 0.0;
        for (int i = 0; i < n; ++i)
        {
            result.expectedReturn += result.weights[i] * expectedReturns[i];
        }
        
        result.sharpeRatio = (result.expectedReturn - riskFreeRate) / result.volatility;            

        return result;
    }

    PortfolioOptimisation::OptimisationResult OptimiseForTargetReturn(const std::vector<std::vector<double>>& covMatrix, const std::vector<double>& expectedReturns, double targetReturn)
    {
        PortfolioOptimisation::OptimisationResult result;
        const std::size_t n = covMatrix.size();

        assert(n != 0 && covMatrix[0].size() == n);

        Eigen::MatrixXd sigma(n, n);
        Eigen::VectorXd mu(n);
        
        for (int i = 0; i < n; ++i)
        {
            mu(i) = expectedReturns[i];
            for (int j = 0; j < n; ++j)
            {
                sigma(i, j) = covMatrix[i][j];
            }
        }
        
        Eigen::MatrixXd A(n + 2, n + 2);
        A.setZero();
        
        A.block(0, 0, n, n) = 2.0 * sigma;
        
        A.block(0, n, n, 1) = mu;
        A.block(0, n + 1, n, 1) = Eigen::VectorXd::Ones(n);
        
        A.block(n, 0, 1, n) = mu.transpose();
        A.block(n + 1, 0, 1, n) = Eigen::VectorXd::Ones(n).transpose();
        
        Eigen::VectorXd b(n + 2);
        b.setZero();
        b(n) = targetReturn;
        b(n + 1) = 1.0;
        
        const Eigen::VectorXd solution = A.colPivHouseholderQr().solve(b);
        const Eigen::VectorXd w = solution.head(n);
        
        result.weights.resize(n);
        for (int i = 0; i < n; ++i)
        {
            result.weights[i] = w(i);
        }
        
        const double variance = (w.transpose() * sigma * w)(0);
        result.volatility = std::sqrt(variance);
        result.expectedReturn = targetReturn;
        result.sharpeRatio = result.expectedReturn / result.volatility;
        
        return result;
    }
    
    EfficientFrontier ComputeEfficientFrontier(const std::vector<std::vector<double>>& covMatrix, const std::vector<double>& expectedReturns, int numPoints)
    {
        EfficientFrontier frontier;
        assert(numPoints > 0);
        
        const double minReturn = *std::min_element(expectedReturns.begin(), expectedReturns.end());
        const double maxReturn = *std::max_element(expectedReturns.begin(), expectedReturns.end());
        
        frontier.returns.reserve(numPoints);
        frontier.volatilities.reserve(numPoints);
        frontier.weights.reserve(numPoints);
        
        double maxSharpe = -std::numeric_limits<double>::max();
        double minVol = std::numeric_limits<double>::max();
        frontier.maxSharpeIndex = -1;
        frontier.minVolIndex = -1;
        
        for (int i = 0; i < numPoints; ++i)
        {
            const double targetReturn = minReturn + i * (maxReturn - minReturn) / (numPoints - 1);
            const PortfolioOptimisation::OptimisationResult result = OptimiseForTargetReturn(covMatrix, expectedReturns, targetReturn);
            
            frontier.returns.push_back(result.expectedReturn);
            frontier.volatilities.push_back(result.volatility);
            frontier.weights.push_back(result.weights);
            
            if (result.sharpeRatio > maxSharpe)
            {
                maxSharpe = result.sharpeRatio;
                frontier.maxSharpeIndex = static_cast<int>(frontier.returns.size()) - 1;
            }
            
            if (result.volatility < minVol)
            {
                minVol = result.volatility;
                frontier.minVolIndex = static_cast<int>(frontier.returns.size()) - 1;
            }
        }
        
        return frontier;
    }

    Eigen::MatrixXd GetCholeskyMatrix(const std::vector<std::vector<double>>& covMatrix)
    {
        const std::size_t n = covMatrix.size();
        
        Eigen::MatrixXd sigma(n, n);
        for (std::size_t i = 0; i < n; ++i)
        {
            for (std::size_t j = 0; j < n; ++j)
            {
                sigma(i, j) = covMatrix[i][j];
            }
        }
        
        Eigen::LLT<Eigen::MatrixXd> llt(sigma);
        assert(llt.info() == Eigen::Success);
        
        return llt.matrixL();
    }

    std::vector<std::vector<double>> CalculateCovarianceMatrix(const std::vector<std::vector<double>>& logReturnsMat)
    {
        assert(logReturnsMat.size() > 0);

        const std::size_t numAssets = logReturnsMat[0].size();
        assert(numAssets > 0);
                
        const std::size_t numPeriods = logReturnsMat.size();
        
        std::vector<double> means(numAssets, 0.0);
        for (std::size_t asset = 0; asset < numAssets; ++asset) 
        {
            double sum = 0.0;
            for (std::size_t t = 0; t < numPeriods; ++t) 
            {
                sum += logReturnsMat[t][asset];
            }
            means[asset] = sum / numPeriods;
        }
        
        /*
        covMatrix is in the form:
            [ [Cov(A1,A1), Cov(A2,A1), Cov(A3,A1), ..., Cov(An, A1)],
              [​Cov(A1,A2), Cov(A2,A2), Cov(A3,A2), ..., Cov(An, A2)],
              [​Cov(A1,A3), Cov(A2,A3), Cov(A3,A3), ..., Cov(An, A3)],
              ......................................................
              [​Cov(A1,An), Cov(A2,An), Cov(A3,An), ..., Cov(An, An)]​ ]
        where A1, A2, A3, ..., An are individual assets in a portfolio
        */
        std::vector<std::vector<double>> covMatrix(numAssets, std::vector<double>(numAssets, 0.0));
        
        for (std::size_t i = 0; i < numAssets; ++i) 
        {
            for (std::size_t j = i; j < numAssets; ++j) 
            {
                double covariance = 0.0;
                for (std::size_t t = 0; t < numPeriods; ++t) 
                {
                    const double dev_i = logReturnsMat[t][i] - means[i];
                    const double dev_j = logReturnsMat[t][j] - means[j];
                    covariance += dev_i * dev_j;
                }
                covariance /= (numPeriods - 1);
                covariance *= 252.0;
                
                covMatrix[i][j] = covariance;
                covMatrix[j][i] = covariance;
            }
        }
        
        return covMatrix;
    }

    std::vector<double> CalculateExpectedAssetReturns(const std::vector<std::string>& tickers, bool annualise)
    {
        std::vector<std::vector<double>> logReturnsMat = DataHandler::GetLogReturnsMat(tickers);
        assert(logReturnsMat.size() > 0);
        
        const std::size_t numPeriods = logReturnsMat.size();
        const std::size_t numAssets = tickers.size();
        
        std::vector<double> expectedReturns(numAssets, 0.0);
        for (std::size_t asset = 0; asset < numAssets; ++asset)
        {
            double sum = 0.0;
            for (std::size_t t = 0; t < numPeriods; ++t)
            {
                sum += logReturnsMat[t][asset];
            }
            
            const double meanReturn = sum / numPeriods;
            expectedReturns[asset] = annualise ? meanReturn * 252.0 : meanReturn;
        }
        
        return expectedReturns;
    }

    double CalculatePortfolioVariance(const std::vector<double>& weights, const std::vector<std::vector<double>>& covMatrix)
    {
        const std::size_t n = weights.size();
        assert(covMatrix.size() == n && covMatrix[0].size() == n);
        
        double variance = 0.0;
        for (std::size_t i = 0; i < n; ++i)
        {
            for (std::size_t j = 0; j < n; ++j)
            {
                variance += weights[i] * weights[j] * covMatrix[i][j];
            }
        }
        
        return variance;
    }

    double CalculatePortfolioReturn(const std::vector<double>& weights, const std::vector<double>& expectedLogReturns)
    {
        assert(weights.size() == expectedLogReturns.size());
        
        double portfolioReturn = 0.0;
        for (std::size_t i = 0; i < weights.size(); ++i)
        {
            portfolioReturn += weights[i] * expectedLogReturns[i];
        }
        
        return portfolioReturn;
    }
};