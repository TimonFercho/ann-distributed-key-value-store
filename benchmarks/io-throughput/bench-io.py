#!/usr/bin/env python
import sys
import time
import argparse
import numpy as np
import os

def measure_time_to_copy(src, dst):
    assert os.path.exists(src)
    assert not os.path.exists(dst)
    start = time.time()
    f = open(src, 'r')
    data = f.read()
    f.close()
    f = open(dst, 'w')
    f.write(data)
    f.close()
    end = time.time()
    return end - start

def create_file_of_size(filepath, file_size_bytes):
    assert not os.path.exists(filepath)
    assert file_size_bytes >= 0
    f = open(filepath, 'w')
    f.write('a' * file_size_bytes)
    f.close()

def measure_thougput(args):
    assert args.num_runs > 0
    src_filepath = os.path.join(args.src_dir, args.src_file)
    dst_filepath = os.path.join(args.dst_dir, args.dst_file)
    execution_times = np.zeros(args.num_runs)
    print(f'Benchmarking copying a file from {src_filepath} to {dst_filepath}')
    print(f'Creating temporary file of size {args.file_size_mb} MB')
    create_file_of_size(filepath=src_filepath, file_size_bytes=args.file_size_mb * 2 ** 20)
    for i in range(args.num_runs):
        print(f'Run {i + 1} of {args.num_runs}')
        execution_times[i] = measure_time_to_copy(src_filepath, dst_filepath)
        os.remove(dst_filepath)
    os.remove(src_filepath)
    throughput = args.file_size_mb / execution_times
    mean_throughput = np.mean(throughput)
    std_throughput = np.std(throughput)
    assert not os.path.exists(src_filepath)
    assert not os.path.exists(dst_filepath)
    return mean_throughput, std_throughput

if __name__ == '__main__':
    self_dir = os.path.dirname(os.path.realpath(__file__))
    parser = argparse.ArgumentParser(prog="Benchmark I/O Throughput", description="Measure the I/O throughput by copying a file of a given size from the specified source directory to the specified destination directory. Creates a file of the specified size in the source directory, copies it to the destination directory, and then deletes both files. The benchmark is repeated the specified number of times and the mean and standard deviation of the throughput is reported.")
    one_gigabyte_in_mega_bytes = 2**10
    parser.add_argument('--file_size_mb', type=int, help='size in MB of the file to be copied', default=one_gigabyte_in_mega_bytes)
    parser.add_argument('--num_runs', type=int, help='number of times to run the test', default=10)
    parser.add_argument('--src_dir', type=str, help='source directory', default=os.path.join(self_dir, 'tmp'))
    parser.add_argument('--dst_dir', type=str, help='destination directory', default=os.path.join(self_dir, 'tmp'))
    parser.add_argument('--src_file', type=str, help='source file name', default='src')
    parser.add_argument('--dst_file', type=str, help='destination file name', default='dst')
    args = parser.parse_args()
    mean_throughput, std_throughput = measure_thougput(args)
    print(f"Mean throughput: {round(mean_throughput, 2)} MB/s")
    print(f"Std throughput: {round(std_throughput, 2)} MB/s")
