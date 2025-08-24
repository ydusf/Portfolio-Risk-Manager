from typing import List
import yfinance as yf
import os
import sys

def download_assets(tickers: List[str], start_date="2024-11-12", end_date="2025-08-21"):
    save_to = "assets/"
    os.makedirs(save_to, exist_ok=True)  # Create folder if it doesn't exist

    for tikr in tickers:
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

if __name__ == "__main__":
    tickers = []

    for arg in sys.argv[1:]:
        if '=' in arg:
            ticker = arg.split('=')[0].upper()
            tickers.append(ticker)

    if tickers:
        download_assets(tickers)
