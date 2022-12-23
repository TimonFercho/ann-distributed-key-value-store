#!/usr/bin/env python
import faiss
import numpy as np
from os.path import join, exists, getsize
from os import makedirs, rename, remove, chdir, getcwd
from faiss.contrib.ondisk import merge_ondisk
import argparse

try:
    from faiss.contrib.datasets_fb import DatasetSIFT1M, DatasetBigANN
except ImportError:
    from faiss.contrib.datasets import DatasetSIFT1M, DatasetBigANN

DATASET_LINK = "http://corpus-texmex.irisa.fr/"


#################################################################
# Preparing the data
#################################################################

def load_dataset(cfg):
    n_splits = cfg.total_dataset_size // cfg.dataset_size 
    xb = cfg.dataset.database_iterator(bs=cfg.batch_size, split=(n_splits, 0)) 
    return xb


#################################################################
# Training the index
#################################################################

def get_trained_index(cfg):
    if exists(cfg.trained_index_file):
        print(f"\tLoading trained index from {cfg.trained_index_file}")
        index = faiss.read_index(cfg.trained_index_file)
    else:
        print("\tTraining index")
        xt = self.dataset.get_train()
        quantizer = faiss.IndexFlatL2(cfg.dimension)
        index = faiss.IndexIVFFlat(quantizer, cfg.dimension, cfg.n_lists)
        index.train(xt)
        print(f"\tWriting trained index to {cfg.trained_index_file}")
        faiss.write_index(index, cfg.trained_index_file)
    return index


#################################################################
# Building the index
#################################################################

def write_batch_index(cfg, batch, batch_no):
    index = None
    if exists(cfg.get_batch_index_file(batch_no)):
        print(f"\tIndex file {cfg.get_batch_index_file(batch_no)} already populated, skipping")
    else:
        index = get_trained_index(cfg)
        print(f"\tAdding vectors to index")
        index.add(batch)
        print(f"\tWriting index to {cfg.get_batch_index_file(batch_no)}")
        faiss.write_index(index, cfg.get_batch_index_file(batch_no))
    return index

def write_batch_indices(cfg):
    xb = load_dataset(cfg)
    index_files = []
    for batch_no, batch in enumerate(xb):
        print(f"Building index for batch {batch_no + 1} of {cfg.n_batches}")
        index_files.append(cfg.get_batch_index_file(batch_no))
        write_vectors(cfg, batch, batch_no)
        index = write_batch_index(cfg, batch, batch_no)
        write_list_ids(cfg, index, batch_no)
    assert getsize(cfg.vectors_file) == cfg.get_expected_vectors_file_size(), "Vectors file does not have the expected size"
    return index_files

def get_merged_index(cfg, index_files):
    if exists(cfg.merged_index_file):
        print(f"Loading merged index from {cfg.merged_index_file}")
        return faiss.read_index(cfg.merged_index_file)
    else:
        print("Merging indices")
        merged_index = get_trained_index(cfg)
        print("\t", end="")
        merge_ondisk(merged_index, index_files, cfg.merged_index_file)
        faiss.write_index(merged_index, cfg.merged_index_file)
        return merged_index


#################################################################
# Extracting clusters from the index
#################################################################

def get_vector_ids(index, list_id):
    list_length = index.invlists.list_size(list_id)
    vector_ids_ptr = index.invlists.get_ids(list_id)
    vector_ids = faiss.rev_swig_ptr(vector_ids_ptr, list_length)
    return vector_ids


def get_ids_map(cfg, index, batch_no):
    ids_map = {}
    start_id = batch_no * cfg.batch_size
    for list_id in range(cfg.n_lists):
        vector_ids = get_vector_ids(index, list_id)
        for vector_id in vector_ids:
            ids_map[start_id + vector_id] = list_id
    return ids_map

def get_centroids(index):
    return index.quantizer.reconstruct_n(0, index.nlist)

#################################################################
# Writing test files to disk
#################################################################

def write_vectors(cfg, batch, batch_no):
    if getsize(cfg.vectors_file) >= cfg.get_expected_partial_vectors_file_size(batch_no):
        print(f"\tVectors file {cfg.vectors_file} is already populated, skipping")
    else:
        expected_size = cfg.get_expected_partial_vectors_file_size(batch_no - 1)
        with open(cfg.vectors_file, "ab") as f:
            if getsize(cfg.vectors_file) > expected_size:
                f.truncate(expected_size)
            print(f"\tWriting vectors to {cfg.vectors_file}")
            batch.tofile(f)
    assert getsize(cfg.vectors_file) >= cfg.get_expected_partial_vectors_file_size(batch_no), "Vectors file does not have the expected size"

def write_vector_ids(cfg):
    if exists(cfg.vector_ids_file) and getsize(cfg.vector_ids_file) == cfg.get_expected_vector_ids_file_size():
            print(f"\tVector ids file already exists, skipping")
            return
    ids = np.arange(cfg.dataset_size, dtype=np.int64)
    with open(cfg.vector_ids_file, "wb") as f:
        ids.tofile(f)
    assert getsize(cfg.vector_ids_file) >= cfg.get_expected_vector_ids_file_size(), "Vector ids file does not have the expected size"

def write_list_ids(cfg, index, batch_no):
    if getsize(cfg.list_ids_file) >= cfg.get_expected_partial_list_ids_file_size(batch_no):
            print(f"\tList ids file {cfg.list_ids_file} already populated, skipping")
    else:
        if not index:
            print(f"\tLoading index from {cfg.get_batch_index_file(batch_no)}")
            index = faiss.read_index(cfg.get_batch_index_file(batch_no))
        expected_size = cfg.get_expected_partial_list_ids_file_size(batch_no - 1)
        with open(cfg.list_ids_file, "ab") as f:
            if getsize(cfg.list_ids_file) > expected_size:
                f.truncate(expected_size)
            start_id = batch_no * cfg.batch_size
            end_id = start_id + cfg.batch_size
            print(f"\tPopulating ids map for vector ids {start_id} to {end_id}")
            ids_map = get_ids_map(cfg, index, batch_no)
            map_all_to_list_ids = np.vectorize(lambda i: ids_map[i])
            ids = np.arange(start=start_id, stop=end_id, dtype=np.int64)
            list_ids = map_all_to_list_ids(ids)
            print(f"\tWriting list ids to {cfg.list_ids_file}")
            list_ids.tofile(f)
    assert getsize(cfg.list_ids_file) >= cfg.get_expected_partial_list_ids_file_size(batch_no), "List ids file does not have the expected size"

def write_centroids(cfg, index):
    if exists(cfg.centroids_file):
        print(f"\tCentroids file already exists, skipping")
    else:
        print(f"\tWriting centroids to {cfg.centroids_file}")
        centroids = get_centroids(index)
        with open(cfg.centroids_file, "wb") as f:
            centroids.tofile(f)

#################################################################
# Main
#################################################################

class Config:
    def __init__(self, args):
        self.dataset_size_millions = 1000 if args.dataset.lower().endswith("1b") else int(args.dataset[4:-1])
        self.indices_dir = join(args.temp_dir, f"SIFT{self.dataset_size_millions}M")
        self.output_dir = join(args.output_dir, f"SIFT{self.dataset_size_millions}M")
        self.indices_base = join(self.indices_dir, f"SIFT{self.dataset_size_millions}M")
        self.dataset_size = self.dataset_size_millions * 10**6
        self.n_lists=args.n_lists
        self.trained_index_file=join(args.temp_dir, "SIFT1000M_trained.index")
        self.merged_index_file=f"{self.indices_base}_merged.index"
        self.vectors_file=join(self.output_dir, args.vectors_file)
        self.vector_ids_file=join(self.output_dir, args.vector_ids_file)
        self.list_ids_file=join(self.output_dir, args.list_ids_file)
        self.centroids_file=join(self.output_dir, args.centroids_file)
        self.batch_size = min(args.batch_size, self.dataset_size)
        self.n_batches = self.dataset_size // self.batch_size
        self.reconstruct_centroids = args.reconstruct_centroids
        self.user_confirmation()
        self.prepare_dataset()

    def user_confirmation(self):
        print(f"Clustering with the following config:")
        print(f"\tDataset: SIFT{self.dataset_size_millions}M")
        print(f"\tNumber of clusters: {self.n_lists}")
        print(f"\tBatch size: {self.batch_size}")
        print(f"\tNumber of batches: {self.n_batches}")
        print(f"\tReconstruct centroids from index: {self.reconstruct_centroids}")
        print(f"\tTemporary directory: {self.indices_dir}")
        print(f"\tOutput directory: {self.output_dir}")
        if not input("Continue? [Y/N] ").lower().startswith("y"):
            print("Aborting")
            exit(1)


    def prepare_dataset(self):
        try:
            # Assumes that the dataset is in data/{dataset}
            # fvecs format:
            # n * [[int] + d * [float32]]
            # where n is the number of vectors,
            # d is the dimension (128),
            # the int is the vector dimension
            # and the float32 is one vector component
            self.dataset = DatasetBigANN()
        except FileNotFoundError:
            print(
                f"Could not find bigann dataset in data/bigann, please download it first from", DATASET_LINK)
            exit(1)
        self.dimension = self.dataset.d
        self.total_dataset_size = self.dataset.nb

    def get_batch_index_file(self, batch_no):
        return f"{self.indices_base}_{batch_no + 1}_of_{self.n_batches}.index"

    def get_expected_partial_vectors_file_size(self, batch_no):
        vector_size = self.dimension * 4
        no_vectors = (batch_no + 1) * self.batch_size
        return no_vectors * vector_size

    def get_expected_vectors_file_size(self):
        return self.get_expected_partial_vectors_file_size(self.n_batches - 1)

    def get_expected_partial_list_ids_file_size(self, batch_no):
        return (batch_no + 1) * self.batch_size * 8

    def get_expected_list_ids_file_size(self):
        return self.get_expected_partial_list_ids_file_size(self.n_batches - 1)

    def get_expected_vector_ids_file_size(self):
        return self.dataset_size * 8

def cluster_dataset(cfg):
    makedirs(cfg.output_dir, exist_ok=True)
    makedirs(cfg.indices_dir, exist_ok=True)
    if not exists(cfg.vectors_file):
        open(cfg.vectors_file, "w").close()
    assert exists(cfg.vectors_file), "Vectors file does not exist"
    if not exists(cfg.list_ids_file):
        open(cfg.list_ids_file, "w").close()
    assert exists(cfg.list_ids_file), "List ids file does not exist"

    index_files = write_batch_indices(cfg)

    if cfg.reconstruct_centroids:
        if len(index_files) > 1:
            index = get_merged_index(cfg, index_files)
        else:
            print("Only one index file, skipping merge and loading last index")
            index = faiss.read_index(index_files[0])
        write_centroids(cfg, index)

    print(f"Writing vector ids to {cfg.vector_ids_file}")
    write_vector_ids(cfg)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog="SIFT Dataset Clustering", description="Cluster subsets of SIFT dataset and output as lists of vectors, vector ids and list ids")
    parser.add_argument("--dataset", type=str, default="SIFT100M", help="Dataset to cluster, one of SIFT1M, SIFT10M, SIFT100M, SIFT1000M/1B", choices=["SIFT1M", "SIFT10M", "SIFT100M", "SIFT1000M", "SIFT1B"])
    parser.add_argument("--batch_size", type=int, default=10**7, help="Batch size")
    parser.add_argument("--n_lists", type=int, default=2**10, help="Number of clusters")
    parser.add_argument("--output_dir", type=str, default="./out", help="Directory to store output files in")
    parser.add_argument("--temp_dir", type=str, default="./tmp", help="Directory to store temporary indices in")
    parser.add_argument("--vectors_file", type=str, default="vectors.bin", help="File to output vectors to")
    parser.add_argument("--vector_ids_file", type=str, default="vector_ids.bin", help="File to output vector ids to")
    parser.add_argument("--list_ids_file", type=str, default="list_ids.bin", help="File to output list ids to")
    parser.add_argument("--centroids_file", type=str, default="centroids.bin", help="File to output centroids to")
    parser.add_argument("--reconstruct_centroids", action="store_true", help="Construct centroids from built index")

    args = parser.parse_args()

    cfg = Config(args)
    # make sure working directory is tests/clustering
    if not getcwd().endswith("tests/clustering"):
        chdir("tests/clustering")
    
    cluster_dataset(cfg)
