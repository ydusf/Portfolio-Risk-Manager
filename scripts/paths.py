import sys
import pandas as pd
import matplotlib.pyplot as plt
import os

def plot_paths_from_csv(filename):
    df = pd.read_csv(filename, header=None)
    
    plt.figure(figsize=(10,6))
    for i in range(len(df)):
        plt.plot(df.iloc[i], color='blue', alpha=0.3, lw=0.5) 

    plt.title("Portfolio Return Paths")
    plt.xlabel("Day")
    plt.ylabel("Return")
    plt.grid(True)
    plt.show()


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python plot_paths.py <filename>")
    else:
        filename = sys.argv[1]

        script_path = os.path.abspath(__file__)
        script_dir = os.path.dirname(script_path)
        project_root = os.path.abspath(os.path.join(script_dir, '..'))
        full_path = os.path.join(project_root, "data", filename)

        plot_paths_from_csv(full_path)