import os
import time
import numpy as np

# Set the size of the file to read and write (in bytes)
FILE_SIZE = 16 * 1024 * 1024 * 1024  # 16 GB

# Set the block size for the I/O operations (in bytes)
BLOCK_SIZE = 4 * 1024  # 4 KB

# Initialize a list to store the throughput measurements
throughputs = []

# Check if the file exists
if not os.path.exists("tmp/testfile"):
    # Create the file with the required size
    with open("tmp/testfile", "wb") as f:
        f.truncate(FILE_SIZE)

# Sample the throughput 10 times
for i in range(10):
    # Open the file for reading and writing
    with open("tmp/testfile", "r+b") as f:
        # Read and write the file in blocks
        start_time = time.time()
        while f.tell() < FILE_SIZE:
            f.read(BLOCK_SIZE)
            f.write(b"\0" * BLOCK_SIZE)
        end_time = time.time()

    # Calculate the elapsed time and throughput
    elapsed_time = end_time - start_time
    throughput = FILE_SIZE / elapsed_time

    # Store the throughput measurement
    throughputs.append(throughput)

# Convert the list of throughput measurements to a NumPy array
throughputs_array = np.array(throughputs)

# Calculate the mean and standard deviation of the throughput
mean_throughput_mbs = np.mean(throughputs_array) / 1024 / 1024
std_throughput_mbs = np.std(throughputs_array) / 1024 / 1024

# Print the results
print(f"Throughput: {round(mean_throughput_mbs, 2)} +/- {round(std_throughput_mbs, 2)} MB/s")
