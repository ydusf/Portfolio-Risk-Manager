#pragma once

#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>

#include "Globals.hpp"

struct StockData 
{
    std::string m_ticker;
    std::map<std::string, double> m_prices;
};

namespace DataHandler
{

static StockData ParseStockData(const std::string& ticker)
{
    StockData data;
    
    const std::string csv = Paths::ASSET_DIR + ticker + ".csv";
    std::ifstream file(csv);

    if(!file.is_open())
    {
        throw std::runtime_error("Could not open file: " + csv);
    }

    std::string line;

    if (!std::getline(file, line)) 
    {
        throw std::runtime_error("File format error: missing header line.");
    }

    if (!std::getline(file, line)) 
    {
        throw std::runtime_error("File format error: missing ticker line.");
    }

    {
        std::istringstream ss(line);
        std::string key, value;
        if (std::getline(ss, key, ',') && std::getline(ss, value)) 
        {
            if (key == "Ticker") 
            {
                if(ticker == value)
                {
                    data.m_ticker = value;
                }
                else
                {
                    throw std::runtime_error("Ticker in CSV does not match provided ticker");
                }
                
            } 
            else 
            {
                throw std::runtime_error("Expected 'Ticker' line but got: " + key);
            }
        }
    }

    if (!std::getline(file, line)) 
    {
        throw std::runtime_error("File format error: missing Date line.");
    }

    while (std::getline(file, line)) 
    {
        if (line.empty()) 
            continue;

        std::istringstream ss(line);
        std::string date;
        std::string valueStr;

        if (std::getline(ss, date, ',') && std::getline(ss, valueStr)) 
        {
            double price = std::stod(valueStr);
            data.m_prices[date] = price;
        }
    }

    return data;
}

static std::vector<StockData> ParseStocks(const std::vector<std::string>& tickers)
{
    std::vector<StockData> stocks;
    for (const std::string& ticker : tickers) 
    {
        try 
        {
            StockData stockData = DataHandler::ParseStockData(ticker);
            stocks.push_back(stockData);
        } 
        catch (const std::exception& ex) 
        {
            std::cerr << "Error loading " << ticker << ": " << ex.what() << '\n';
        }
    }

    return stocks;
}

static std::vector<std::vector<double>> GetReturnsMat(const std::vector<std::string>& tickers)
{
    std::vector<StockData> stocks = DataHandler::ParseStocks(tickers);
    if(stocks.empty())
        return {};

    std::size_t numRows = SIZE_MAX;
    for(const auto& stock : stocks)
    {
        if(stock.m_prices.size() < 2)
            continue;
            
        numRows = std::min(numRows, stock.m_prices.size() - 1);
    }

    std::size_t numCols = stocks.size();

    std::vector<std::vector<double>> returnsMat(numRows, std::vector<double>(numCols, 0.0));
    for(std::size_t col = 0; col < numCols; ++col)
    {
        const auto& stock = stocks[col];
        if(stock.m_prices.size() < 2)
            continue;

        std::size_t row = 0;
        for(auto itr = std::next(stock.m_prices.begin()); row < numRows; ++itr, ++row)
        {
            auto prevItr = std::prev(itr);
            if(prevItr->second == 0.0)
            {
                returnsMat[row][col] = 0.0;
                continue;
            }
            double ret = (itr->second - prevItr->second) / prevItr->second;
            returnsMat[row][col] = ret;
        }
    }

    return returnsMat;
}

static std::vector<std::vector<double>> GetLogReturnsMat(const std::vector<std::string>& tickers)
{
    std::vector<StockData> stocks = DataHandler::ParseStocks(tickers);
    if(stocks.empty())
        return {};

    std::size_t numRows = SIZE_MAX;
    for(const auto& stock : stocks)
    {
        if(stock.m_prices.size() < 2)
            continue;
            
        numRows = std::min(numRows, stock.m_prices.size() - 1);
    }

    std::size_t numCols = stocks.size();

    std::vector<std::vector<double>> logReturnsMat(numRows, std::vector<double>(numCols, 0.0));
    
    for(std::size_t col = 0; col < numCols; ++col)
    {
        const auto& stock = stocks[col];
        if(stock.m_prices.size() < 2)
            continue;

        std::size_t row = 0;
        for(auto itr = std::next(stock.m_prices.begin()); row < numRows; ++itr, ++row)
        {
            auto prevItr = std::prev(itr);
            
            // Check for non-positive prices
            if(prevItr->second <= 0.0 || itr->second <= 0.0)
            {
                logReturnsMat[row][col] = std::numeric_limits<double>::quiet_NaN();
                continue;
            }
            
            // Correct log return: log(P_t / P_{t-1})
            double logReturn = std::log(itr->second / prevItr->second);
            logReturnsMat[row][col] = logReturn;
        }
    }

    return logReturnsMat;
}

};