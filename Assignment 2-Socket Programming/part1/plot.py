import subprocess
import json
import time
import matplotlib.pyplot as plt
import numpy as np
from scipy import stats

# Function to calculate mean and confidence interval
def mean_confidence_interval(data, confidence=0.95):
    data = np.array(data)
    n = len(data)
    mean = np.mean(data)
    se = stats.sem(data)
    h = se * stats.t.ppf((1 + confidence) / 2., n-1)
    return mean, mean - h, mean + h

# Function to run the server
def run_server(p):
    # Update the config file with the new value of p
    with open('config.json', 'r') as f:
        config = json.load(f)
    config['p'] = p

    with open('config.json', 'w') as f:
        json.dump(config, f, indent=4)

    # Rebuild and run the server
    subprocess.run(['make', 'build'])  # Build the server
    return subprocess.Popen(['./server'])  # Start the server

# Function to run the client and measure execution time
def run_client():
    times = []
    for _ in range(50):  # Run 50 times for each p value
        start_time = time.time()
        subprocess.run(['./client'])  # Run the client
        elapsed_time = time.time() - start_time
        times.append(elapsed_time)
    return times  # Return all times for further processing

# Main function to run the experiment
def main():
    p_values = list(range(1, 11))  # p from 1 to 10
    completion_times = []
    ci_lower = []
    ci_upper = []

    for p in p_values:
        print(f"Running for p = {p}")

        # Start the server
        server_process = run_server(p)

        # Run the client and get execution times
        times = run_client()
        
        # Calculate mean and confidence interval
        mean_time, lower_ci, upper_ci = mean_confidence_interval(times)
        completion_times.append(mean_time)
        ci_lower.append(lower_ci)
        ci_upper.append(upper_ci)
        
        print(f"Average time for p = {p}: {mean_time:.4f} seconds (95% CI: {lower_ci:.4f} to {upper_ci:.4f})")

        # Stop the server after client runs
        server_process.terminate()
        server_process.wait()  # Wait for the server to properly terminate

    # Plotting the results
    plt.figure(figsize=(10, 6))
    plt.errorbar(p_values, completion_times, yerr=[np.array(completion_times) - np.array(ci_lower), 
                                                    ci_upper - np.array(completion_times)],
                 marker='o', linestyle='-', color='b', capsize=5, label='Average Time ± 95% CI')
    plt.xlabel('p (Number of words per packet)')
    plt.ylabel('Average Completion Time (seconds)')
    plt.title('Completion Time vs p')
    plt.grid(True)
    plt.legend()
    plt.savefig('plot.png')
    plt.show()  # Display the plot

if __name__ == '__main__':
    main()
