import matplotlib.pyplot as plt
import pandas as pd

def plot_efficient_frontier_from_csv(filename):
    df = pd.read_csv(filename)

    plt.figure(figsize=(10, 6))
    plt.plot(df['Volatility'], df['Return'], 'b-', marker='o', markersize=4)
    plt.title("Efficient Frontier")
    plt.xlabel("Volatility (Standard Deviation)")
    plt.ylabel("Expected Return")
    plt.grid(True)

    max_sharpe_idx = df['SharpeRatio'].idxmax()
    plt.scatter(df['Volatility'][max_sharpe_idx], df['Return'][max_sharpe_idx], color='red', marker='*', s=200, label="Max Sharpe Ratio")

    min_vol_idx = df['Volatility'].idxmin()
    plt.scatter(df['Volatility'][min_vol_idx], df['Return'][min_vol_idx], color='green', marker='s', s=200, label='Min Volatility')
    
    plt.legend()

    plt.show()

if __name__ == "__main__":
    plot_efficient_frontier_from_csv("./data/efficient_frontier.csv")