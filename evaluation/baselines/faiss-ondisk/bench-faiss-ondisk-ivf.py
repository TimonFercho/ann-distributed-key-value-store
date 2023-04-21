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
    cfg['n_probes'] = [1, 2, 4, 8, 16, 32, 64, 128]
    cfg['n_runs'] = 3
    cfg['n_splits'] = cfg['total_dataset_size'] // cfg['dataset_size']
    cfg['batch_size'] = cfg['dataset_size'] // cfg['n_batches']
    cfg['output_file'] = f'results/results-{cfg["n_lists"]}.csv'

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

            xq = cfg['dataset'].get_queries()

            measurements = []
            with open(cfg['output_file'], 'a') as f:
                header = 'n_probes,parallel_mode,run,queries_per_s,latency_50th,latency_95th'
                f.write(header + '\n')

                for nprobe in cfg['n_probes']:
                    index.nprobe = nprobe
                    print("nprobe = %d" % nprobe)
                    for pmode in [0, 1, 2]:
                        index.parallel_mode = pmode
                        print("parallel_mode = %d" % pmode)

                        for run in range(cfg['n_runs']):
                            # measure qps
                            start_time = time.time()
                            D, I = index.search(xq, 10)
                            end_time = time.time()
                            runtime = end_time - start_time
                            qps = cfg['n_queries'] / runtime

                            # measure latency
                            latencies = []
                            n_queries = xq.shape[0]
                            for i in range(n_queries):
                                start_time = time.time()
                                D, I = index.search(xq[i:i + 1], 10)
                                end_time = time.time()
                                latency_s = end_time - start_time
                                latencies.append(latency_s)

                            latencies = np.array(latencies)
                            latency_50 = np.percentile(latencies, 50)
                            latency_95 = np.percentile(latencies, 95)

                            line = f'{nprobe},{pmode},{run},{qps},{latency_50},{latency_95}'

                            f.write(line + '\n')
                            f.flush()


if __name__ == "__main__":
    main()
