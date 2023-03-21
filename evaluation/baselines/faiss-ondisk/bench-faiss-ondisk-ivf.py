#!/usr/bin/env python3

import time
import numpy as np
import faiss
from faiss.contrib.ondisk import merge_ondisk
try:
    from faiss.contrib.datasets_fb import DatasetSIFT1M, DatasetBigANN
except ImportError:
    from faiss.contrib.datasets import DatasetSIFT1M, DatasetBigANN


def main():
    tmpdir = 'tmp/'

    cfg = {}
    cfg['dataset'] = DatasetBigANN()
    cfg['total_dataset_size'] = cfg['dataset'].nb
    cfg['dataset_size'] = 10**6
    cfg['n_batches'] = 10
    cfg['n_queries'] = 10**5
    cfg['dimension'] = 128
    cfg['n_lists'] = 1024
    cfg['n_probe'] = 4
    cfg['n_splits'] = cfg['total_dataset_size'] // cfg['dataset_size']
    cfg['batch_size'] = cfg['dataset_size'] // cfg['n_batches']

    for stage in [3]:
        if stage == 0:
            quantizer = faiss.IndexFlatL2(cfg['dimension'])
            index = faiss.IndexIVFFlat(quantizer, cfg['dimension'],
                                       cfg['n_lists'])
            print("training index")
            xt = cfg['dataset'].get_train()
            index.train(xt)
            print("write " + tmpdir + "trained.index")
            faiss.write_index(index, tmpdir + "trained.index")

        if stage == 1:
            xb = cfg['dataset'].database_iterator(bs=cfg['batch_size'],
                                                  split=(cfg['n_splits'], 0))
            for batch_no, batch in enumerate(xb):
                index = faiss.read_index(tmpdir + "trained.index")
                index.add(batch)
                faiss.write_index(index, tmpdir + "block_%s.index" % batch_no)

        if stage == 2:
            print('loading trained index')
            index = faiss.read_index(tmpdir + "trained.index")
            block_fnames = [
                tmpdir + "block_%d.index" % bno
                for bno in range(cfg['n_batches'])
            ]
            merge_ondisk(index, block_fnames, tmpdir + "merged_index.ivfdata")
            print("write " + tmpdir + "populated.index")
            faiss.write_index(index, tmpdir + "populated.index")

        if stage == 3:
            index = faiss.read_index(tmpdir + "populated.index")
            index.nprobe = cfg['n_probe']

            for pmode in [0, 0, 0, 1, 1, 1, 2, 2, 2]:
                index.parallel_mode = pmode
                print("parallel_mode = %d" % pmode)
                xq = cfg['dataset'].get_queries()

                # measure throughput
                num_runs = 3
                runtimes = []
                for i in range(num_runs):
                    start_time = time.time()
                    D, I = index.search(xq, 10)
                    end_time = time.time()
                    runtime = end_time - start_time
                    runtimes.append(runtime)

                # Compute the average runtime and queries per second
                avg_runtime = sum(runtimes) / num_runs
                queries_per_sec = cfg['n_queries'] / avg_runtime
                print("Average runtime: %.3f seconds" % avg_runtime)
                print("Queries per second: %.3f" % queries_per_sec)

                # measure latency
                latencies = []
                n_queries = xq.shape[0]
                for i in range(n_queries):
                    start_time = time.time()
                    D, I = index.search(xq[i:i + 1], 10)
                    end_time = time.time()
                    latency = end_time - start_time
                    latency_ms = latency * 1000
                    latencies.append(latency_ms)

                # Compute the median and 95th percentile latency
                latencies = np.array(latencies)
                median_latency = np.median(latencies)
                percentile_95th = np.percentile(latencies, 95)
                print("Median latency: %.3f ms" % median_latency)
                print("95th percentile latency: %.3f ms" % percentile_95th)
                print("=====================================")


if __name__ == "__main__":
    main()
