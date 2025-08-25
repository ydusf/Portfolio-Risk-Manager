import pandas as pd
import matplotlib.pyplot as plt

def plot_paths_from_csv(filename):
    df = pd.read_csv(filename, header=None)
    
    plt.figure(figsize=(10,6))
    for i in range(len(df)):
        plt.plot(df.iloc[i])
    
    plt.title("Portfolio Return Paths")
    plt.xlabel("Day")
    plt.ylabel("Return")
    plt.legend()
    plt.grid(True)
    plt.show()


if __name__ == "__main__":  
    plot_paths_from_csv("./paths/monte_carlo_simul6.csv")
