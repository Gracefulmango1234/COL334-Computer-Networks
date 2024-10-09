import subprocess
import time
import json
import matplotlib.pyplot as plt
import numpy as np


# Function to update only the 'num_clients' field in the client configuration
def update_client_config(num_clients):
    # Load the existing configuration from the file
    with open('config.json', 'r') as config_file:
        config_data = json.load(config_file)
    
    # Update only the 'num_clients' field
    config_data['num_clients'] = num_clients
    
    # Write the updated configuration back to the file
    with open('config.json', 'w') as config_file:
        json.dump(config_data, config_file, indent=4)

# Function to run the server (fixed server)
def run_server():
    # Rebuild and run the server
    subprocess.run(['make', 'build'])  # Build the server
    return subprocess.Popen(['./server'])  # Start the server

# Function to run the client and measure execution time for a given number of clients
def run_client(num_clients):
    # Update the client config before running
    

    start_time = time.time()
    
    # Compile and run the client once
    subprocess.run(['./client'])  # Run the client once
    
    elapsed_time = time.time() - start_time
    # avg_time_per_client = elapsed_time / num_clients
    return elapsed_time  # Return the average time per client

# Main function to run the experiment
def main():
    num_clients_values = list(range(1, 33, 4))  # Number of clients: 1, 5, 9, ..., 29
    avg_completion_times = []

    # Start the server once
    print("Starting the server...")
    server_process = run_server()

    # Iterate over different numbers of clients
    for num_clients in num_clients_values:
        print(f"Running with {num_clients} clients...")

        # Run the client and measure the average completion time
        times = []
        update_client_config(num_clients)
        subprocess.run(['make', 'client'])  # Compile the client
        for _ in range(10):  # Repeat 10 times for averaging
            avg_time = run_client(num_clients)
            times.append(avg_time)
        
        # Calculate the mean time
        mean_time = np.mean(times)
        avg_completion_times.append(mean_time)

        print(f"Average time for {num_clients} clients: {mean_time:.4f} seconds")

    # Stop the server
    server_process.terminate()
    server_process.wait()  # Ensure server has terminated

    # Plotting the results
    plt.figure(figsize=(10, 6))
    plt.plot(num_clients_values, avg_completion_times, marker='o', linestyle='-', color='b', label='Avg Completion Time')
    plt.xlabel('Number of Clients')
    plt.ylabel('Average Completion Time per Client (seconds)')
    plt.title('Completion Time vs Number of Clients')
    plt.grid(True)
    plt.legend()
    plt.savefig('plot.png')
    plt.show()  # Display the plot

if __name__ == '__main__':
    main()
