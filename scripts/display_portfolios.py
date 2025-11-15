import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
from matplotlib.ticker import PercentFormatter
from pathlib import Path
import argparse
import sys

def load_data_with_pandas(filepath: str):
    portfolio_file = Path(filepath)
    if not portfolio_file.exists():
        print(f"Error: File not found at path: {filepath}")
        return None, None

    try:
        df = pd.read_csv(portfolio_file)

        df_metrics = df.iloc[:, 0:4]
        df_metrics.columns = ['Type', 'Expected Return', 'Volatility', 'Sharpe Ratio']
        
        df_weights = df.iloc[:, 4:]
        df_weights.columns = [col.split('_', 1)[1] for col in df_weights.columns]

        tickers = list(df_weights.columns)

        df_full_weights = pd.concat([df_metrics['Type'], df_weights], axis=1)
        df_full_weights = df_full_weights.set_index('Type')
        
        return df_metrics, df_full_weights, tickers

    except pd.errors.EmptyDataError:
        print("Error: The CSV file is empty.")
        return None, None
    except (IOError, IndexError, Exception) as e:
        print(f"An unexpected error occurred while loading the file: {e}")
        return None, None

def plot_weights_heatmap(df_weights):    
    if df_weights.empty:
        print("No weights to plot.")
        return

    plt.figure(figsize=(12, max(6, len(df_weights) * 0.8)))
    
    sns.heatmap(df_weights, 
                annot=True, 
                fmt='.1%', 
                cmap='vlag', 
                center=0,
                linewidths=.5,
                cbar_kws={'label': 'Weight in Portfolio', 'format': PercentFormatter(xmax=1)})
    
    plt.title("Portfolio Allocation Weights Comparison", fontsize=16, pad=20)
    plt.ylabel("Portfolio Type")
    plt.xlabel("Asset Ticker")
    plt.xticks(rotation=45)
    plt.yticks(rotation=0)  
    
    plt.tight_layout()
    plt.show()

def main():
    parser = argparse.ArgumentParser(description="Display and plot optimised portfolios from CSV file",
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    
    parser.add_argument(
        "filepath", 
        help="Path to the CSV file containing optimised portfolios"
    )

    args = parser.parse_args()
    
    df_metrics, df_weights, _ = load_data_with_pandas(args.filepath)
    
    if df_metrics is None:
        sys.exit(1)
    
    plot_weights_heatmap(df_weights)
    

if __name__ == "__main__":
    main()