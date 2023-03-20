#!/usr/bin/env python3

import time
import numpy as np
import faiss
from faiss.contrib.ondisk import merge_ondisk
try:
    from faiss.contrib.datasets_fb import DatasetSIFT1M, DatasetBigANN
except ImportError:
    from faiss.contrib.datasets import DatasetSIFT1M, DatasetBigANN

# def load_dataset(cfg):
#     return cfg['dataset'].database_iterator(bs=cfg['batch_size'],
# split=(cfg['n_splits'], 0))


def main():
    tmpdir = 'tmp/'
    stage = 3

    cfg = {}
    cfg['dataset'] = DatasetSIFT1M()
    cfg['total_dataset_size'] = cfg['dataset'].nb
    cfg['dataset_size'] = 10**6
    # cfg['batch_size'] = 10**5
    # cfg['n_splits'] = cfg['total_dataset_size'] // cfg['dataset_size']
    cfg['n_batches'] = 1
    cfg['n_queries'] = 10**5
    cfg['dimension'] = 128
    cfg['n_lists'] = 1024

    if stage == 0:
        quantizer = faiss.IndexFlatL2(cfg['dimension'])
        index = faiss.IndexIVFFlat(quantizer, cfg['dimension'], cfg['n_lists'])
        print("training index")
        xt = cfg['dataset'].get_train()
        index.train(xt)
        print("write " + tmpdir + "trained.index")
        faiss.write_index(index, tmpdir + "trained.index")

    if stage == 1:
        xb = cfg['dataset'].get_database()
        for batch_no in range(cfg['n_batches']):
            index = faiss.read_index(tmpdir + "trained.index")
            index.add(xb)
            faiss.write_index(index, tmpdir + "block_%s.index" % batch_no)

    if stage == 2:
        print('loading trained index')
        index = faiss.read_index(tmpdir + "trained.index")
        block_fnames = [
            tmpdir + "block_%d.index" % bno for bno in range(cfg['n_batches'])
        ]
        merge_ondisk(index, block_fnames, tmpdir + "merged_index.ivfdata")
        print("write " + tmpdir + "populated.index")
        faiss.write_index(index, tmpdir + "populated.index")

    if stage == 3:
        print("read " + tmpdir + "populated.index")
        index = faiss.read_index(tmpdir + "populated.index")
        index.nprobe = 16

        xq = cfg['dataset'].get_queries()

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


if __name__ == "__main__":
    main()
