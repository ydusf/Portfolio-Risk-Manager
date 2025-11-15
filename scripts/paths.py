import pandas as pd
import matplotlib.pyplot as plt
import argparse
import sys

def plot_paths_from_csv(filepath, sample_size, ci_bands):
    try:
        df = pd.read_csv(filepath, header=None)
    except FileNotFoundError:
        print(f"Error: The file '{filepath}' was not found.")
        sys.exit(1)
    except pd.errors.EmptyDataError:
        print(f"Error: The file '{filepath}' is empty.")
        sys.exit(1)

    paths_df = df.T
    n_paths = len(paths_df.columns)

    plt.figure(figsize=(12, 7))
    plt.style.use('seaborn-v0_8-darkgrid')
    
    actual_sample_size = min(sample_size, n_paths)
    if actual_sample_size > 0:
        sample_paths = paths_df.sample(n=actual_sample_size, axis=1)
        plt.plot(sample_paths.index, 
                 sample_paths, 
                 color='blue', 
                 alpha=0.3, 
                 lw=0.5, 
                 label='_nolegend_')

    if ci_bands:
        q_lower_outer = paths_df.quantile(0.05, axis=1)
        q_upper_outer = paths_df.quantile(0.95, axis=1)
        
        q_lower_inner = paths_df.quantile(0.25, axis=1)
        q_upper_inner = paths_df.quantile(0.75, axis=1)
    
    median_path = paths_df.quantile(0.5, axis=1)
    
    if ci_bands:
        plt.fill_between(paths_df.index, 
                        q_lower_outer, 
                        q_upper_outer, 
                        color='C0', 
                        alpha=0.2, 
                        label='90% Interval (5th-95th percentile)')
    
        plt.fill_between(paths_df.index, 
                        q_lower_inner, 
                        q_upper_inner, 
                        color='C0', 
                        alpha=0.4, 
                        label='50% Interval (IQR: 25th-75th percentile)')

    plt.plot(median_path.index, 
             median_path, 
             color='black', lw=2, 
             linestyle='--', 
             label='Median Path (50th percentile)')

    plt.title("Monte Carlo Simulation")
    plt.xlabel("Step (e.g., Day)")
    plt.ylabel("Value (e.g., Return)")
    plt.legend()
    plt.grid(True, linestyle='--', alpha=0.6)
    plt.tight_layout()
    plt.show()


def main():
    parser = argparse.ArgumentParser(description="Plot simulation paths from a CSV file.",
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    
    parser.add_argument(
        "filepath", 
        help="Path to the CSV file containing simulation paths."
    )
    parser.add_argument(
        "-n", "--sample_size", 
        type=int, 
        default=50,
        help="Number of random sample paths to plot for background texture."
    )
    parser.add_argument(
        "--no-bands", 
        action="store_false",
        dest="ci_bands",
        help="Do not plot the 50%/90% quantile bands."
    )

    args = parser.parse_args()
    
    plot_paths_from_csv(args.filepath, 
                        args.sample_size, 
                        args.ci_bands)
    
if __name__ == "__main__":
    main()