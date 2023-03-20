import subprocess
import statistics
import re

pattern = r'READ: bw=(\d+\.\d+)MiB/s'

num_runs = 3
results = []

for i in range(num_runs):
    output = subprocess.check_output(
        ['/home/tfercho/.local/bin/fio', 'bigann-10M.fio']).decode('utf-8')
    match = re.search(pattern, output)
    if match:
        bandwidth = float(match.group(1))
        results.append(bandwidth)
    else:
        print("Error: bandwidth not found in output")

avg_throughput = statistics.mean(results)
std_dev = statistics.stdev(results)

print(f"Average throughput: {avg_throughput} MB/s")
print(f"Standard deviation: {std_dev} MB/s")
