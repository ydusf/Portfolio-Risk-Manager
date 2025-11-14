from typing import List
import yfinance as yf
import os
import sys
import csv    


def download_assets(tickers: List[tuple], start_date="2020-11-12", end_date="2025-08-21"):
    save_to = "assets/"
    os.makedirs(save_to, exist_ok=True)  # Create folder if it doesn't exist

    for tikr, _ in tickers:
        if os.path.exists("assets/" + tikr + ".csv"):
            print(f"CSV file with data for {tikr} already exists")
            continue

        print(f"Downloading {tikr}...")
        data = yf.download(tikr, start=start_date, end=end_date, progress=False)
        
        if data.empty:
            print(f"Warning: No data found for {tikr}")
            continue
        
        df_to_save = data[['Close']]

        df_to_save.to_csv(os.path.join(save_to, f"{tikr}.csv"))
        print(f"Saved {tikr}.csv to {save_to}")

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

            tickers.append((row[1], row[0]))

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
                
    if tickers and n_args > 3:
        download_assets(tickers, sys.argv[n_args-2], sys.argv[n_args-1])
    elif tickers:
        download_assets(tickers, "2020-01-01", "2025-11-11")

