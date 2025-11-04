from typing import List
import yfinance as yf
import os
import sys
import requests
import csv    

from typing import Optional

def get_ticker_from_isin(isin: str) -> Optional[str]:
    url = "https://query2.finance.yahoo.com/v1/finance/search"
    headers = {
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36'
    }
    params = {'q': isin, 'quotesCount': 1, 'newsCount': 0}

    try:
        response = requests.get(url, params=params, headers=headers, timeout=5)
        response.raise_for_status() 
        
        data = response.json()
        
        if 'quotes' in data and data['quotes']:
            symbol = data['quotes'][0].get('symbol')
            return symbol
        else:
            print(f"Warning: No quote found for ISIN {isin}")
            return None

    except requests.exceptions.RequestException as e:
        print(f"Error fetching data for {isin}: {e}")
        return None
    except (IndexError, KeyError, TypeError, ValueError) as e:
        print(f"Error parsing response for {isin}: {e}")
        return None


def download_assets(tickers: List[tuple], start_date="2020-11-12", end_date="2025-08-21"):
    save_to = "assets/"
    os.makedirs(save_to, exist_ok=True)  # Create folder if it doesn't exist

    for tikr, name in tickers:
        if os.path.exists("assets/" + name + ".csv"):
            print(f"CSV file with data for {name} already exists")
            continue

        print(f"Downloading {tikr}...")
        data = yf.download(tikr, start=start_date, end=end_date, progress=False)
        
        if data.empty:
            print(f"Warning: No data found for {tikr}")
            continue
        
        df_to_save = data[['Close']]

        df_to_save.to_csv(os.path.join(save_to, f"{name}.csv"))
        print(f"Saved {name}.csv to {save_to}")

def parse_tickers_out_of_portfolio(filename: str) -> List[str]:
    tickers: List[tuple] = []
    
    filepath = os.path.join("data", filename + ".csv")
    with open(filepath, newline='') as csvfile:
        reader = csv.reader(csvfile, delimiter=',', quotechar='|')
        firstRow: bool = True
        for row in reader:
            if firstRow:
                firstRow = False
                continue

            ticker = get_ticker_from_isin(row[0])
            tickers.append((ticker, row[0]))

    return tickers


if __name__ == "__main__":
    tickers = []

    n_args = len(sys.argv)
    if n_args > 3:
        for arg in sys.argv[1:n_args-2]:
            if '=' in arg:
                ticker = arg.split('=')[0].upper()
                tickers.append((ticker, ticker))
    else:
        tickers = parse_tickers_out_of_portfolio("current_portfolio")
                
    if tickers:
        download_assets(tickers, sys.argv[n_args-2], sys.argv[n_args-1])

