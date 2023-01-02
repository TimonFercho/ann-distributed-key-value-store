# I/O Throughput Benchmarks

| Configuration      | Value     |
|--------------|-----------|
| Script      | `ann-dkvs/benchmarks/bench_io.py` |
| File size   | 16000 MB |
| #Samples    | 10      |


## 1. Copying file from /mnt/scratch/ to /mnt/scratch/


```
Benchmarking copying data from /pub/scratch/tfercho/ann-dkvs/benchmarks/io-throughput/tmp/src to /pub/scratch/tfercho/ann-dkvs/benchmarks/io-throughput/tmp/dst
Creating temporary file of size 16000 MB
Run 1 of 10
Run 2 of 10
Run 3 of 10
Run 4 of 10
Run 5 of 10
Run 6 of 10
Run 7 of 10
Run 8 of 10
Run 9 of 10
Run 10 of 10
Mean throughput: 205.19 MB/s
Std throughput: 34.9 MB/s
```

## 2. Copying file from /home/tfercho/ to /home/tfercho/

```
Benchmarking copying data from /home/tfercho/tmp/ to /home/tfercho/tmp/
Creating temporary file of size 16000 MB
Run 1 of 10
Run 2 of 10
Run 3 of 10
Run 4 of 10
Run 5 of 10
Run 6 of 10
Run 7 of 10
Run 8 of 10
Run 9 of 10
Run 10 of 10
Mean throughput: 223.31 MB/s
Std throughput: 60.11 MB/s
```