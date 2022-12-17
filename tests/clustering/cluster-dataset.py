import faiss
import numpy as np
from os.path import join, exists, getsize
from os import makedirs, rename, remove
from faiss.contrib.ondisk import merge_ondisk

try:
    from faiss.contrib.datasets_fb import DatasetSIFT1M, DatasetBigANN
except ImportError:
    from faiss.contrib.datasets import DatasetSIFT1M, DatasetBigANN

DATASET_LINK = "http://corpus-texmex.irisa.fr/"


#################################################################
# Preparing the data
#################################################################

def load_dataset(cfg):
    # Assumes that the dataset is in data/{dataset}
    # fvecs format:
    # n * [[int] + d * [float32]]
    # where n is the number of vectors,
    # d is the dimension (128),
    # the int is the vector dimension
    # and the float32 is one vector component
    try:
        ds = DatasetBigANN()
    except FileNotFoundError:
        print(
            f"Could not find bigann dataset in data/bigann, please download it first from", DATASET_LINK)
        exit(1)
    total_dataset_size = ds.nb
    dataset_size = cfg.dataset_size_millions * 10**6
    cfg.batch_size = min(cfg.batch_size, dataset_size)
    n_splits = total_dataset_size // dataset_size 
    cfg.n_batches = dataset_size // cfg.batch_size
    xb = ds.database_iterator(bs=cfg.batch_size, split=(n_splits, 0)) 
    cfg.dimension = ds.d
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
        try:
            ds = DatasetBigANN()
        except FileNotFoundError:
            print(
                f"\tCould not find bigann dataset in data/bigann, please download it first from", DATASET_LINK)
            exit(1)
        xt = ds.get_train()
        quantizer = faiss.IndexFlatL2(cfg.dimension)
        index = faiss.IndexIVFFlat(quantizer, cfg.dimension, cfg.n_lists)
        index.train(xt)
        print(f"\tWriting trained index to {cfg.trained_index_file}")
        faiss.write_index(index, cfg.trained_index_file)
    return index


#################################################################
# Building the index
#################################################################

def populate_and_write_index(cfg, batch, batch_no):
    if cfg.get_vectors_file_size() >= cfg.get_expected_vectors_file_size(batch_no):
        print(f"\tVectors file {cfg.vectors_file} is already populated, skipping")
    else:
        assert cfg.get_vectors_file_size() == cfg.get_expected_vectors_file_size(batch_no - 1), f"Vectors file {cfg.vectors_file} does not have the expected size"
        with open(cfg.vectors_file, "wb") as f:
            print(f"\tWriting vectors to {cfg.vectors_file}")
            batch.tofile(f)
    if exists(cfg.get_batch_index_file(batch_no)):
        print(f"\tIndex file {cfg.get_batch_index_file(batch_no)} already exists, skipping")
    else:
        index = get_trained_index(cfg)
        print(f"\tAdding vectors to index")
        index.add(batch)
        print(f"\tWriting index to {cfg.get_batch_index_file(batch_no)}")
        faiss.write_index(index, cfg.get_batch_index_file(batch_no))

def build_batch_indices(cfg):
    xb = load_dataset(cfg)
    index_files = []
    for batch_no, batch in enumerate(xb):
        print(f"Building index for batch {batch_no + 1} of {cfg.n_batches}")
        index_files.append(cfg.get_batch_index_file(batch_no))
        populate_and_write_index(cfg, batch, batch_no)
    return index_files

def get_merged_index(cfg):
    if exists(cfg.merged_index_file):
        print(f"Loading merged index from {cfg.merged_index_file}")
        return faiss.read_index(cfg.merged_index_file)
    else:
        print("Merging indices")
        merged_index = get_trained_index(cfg)
        merge_ondisk(merged_index, index_files, cfg.merged_index_file)
        faiss.write_index(merged_index, cfg.merged_index_file)
        return merged_index


#################################################################
# Extracting clusters from the index
#################################################################

def get_vector_ids(index, list_id):
    invlists = index.invlists
    list_len = invlists.list_size(list_id)
    list_ids = invlists.get_ids(list_id)
    list_vec_ids = faiss.rev_swig_ptr(list_ids, list_len)
    return list_vec_ids


def get_vector_id_to_list_id_map(index):
    vector_id_to_list_id_map = {}
    for list_id in range(index.nlist):
        vector_ids = get_vector_ids(index, list_id)
        for vector_id in vector_ids:
            vector_id_to_list_id_map[vector_id] = list_id
    return vector_id_to_list_id_map


#################################################################
# Writing test files to disk
#################################################################

def write_vector_ids(cfg, vector_id_to_list_id_map):
    ids = np.arange(len(vector_id_to_list_id_map), dtype=np.int64)
    with open(cfg.vector_ids_file, "wb") as f:
        ids.tofile(f)


def write_list_ids(cfg, vector_id_to_list_id_map):
    ids = np.arange(len(vector_id_to_list_id_map), dtype=np.int64)
    map_all_to_list_ids = np.vectorize(lambda i: vector_id_to_list_id_map[i])
    list_ids = map_all_to_list_ids(ids)
    with open(cfg.list_ids_file, "wb") as f:
        list_ids.tofile(f)


#################################################################
# Main
#################################################################

def pipeline():
    cfg = Config(
        dataset_size_millions=1,
        n_lists=1024,
        vectors_file="vectors.bin",
        vector_ids_file="vector_ids.bin",
        list_ids_file= "list_ids.bin",
        output_dir="./out",
        temp_dir="./tmp", 
        batch_size=10**8
    )
    assert cfg.dataset_size_millions in [1, 10, 100, 1000], "Only SIFT1M, SIFT10M, SIFT100M, SIFT1B are supported"

    if exists(cfg.vector_ids_file) or exists(cfg.list_ids_file):
        print(f"Some output files already exist in {cfg.output_dir}, override? [y/n]")
        if input() != "y":
            exit(1)
        for f in [cfg.vector_ids_file, cfg.list_ids_file]:
            if exists(f):
                remove(f)
    makedirs(cfg.output_dir, exist_ok=True)
    makedirs(cfg.indices_dir, exist_ok=True)
    if not exists(cfg.vectors_file):
        open(cfg.vectors_file, "w").close()

    index_files = build_batch_indices(cfg)

    if len(index_files) > 1:
        print(f"Merging indices")
        index = get_merged_index(cfg)
    else:
        print(f"Only one index file, skipping merge")
        index = faiss.read_index(index_files[0])

    print("Constructing vector-id-to-list-ids map")
    vector_id_to_list_id_map = get_vector_id_to_list_id_map(index)
    print(f"Writing vector ids to {cfg.vector_ids_file}")
    write_vector_ids(cfg, vector_id_to_list_id_map)
    print(f"Writing list ids to {cfg.list_ids_file}")
    write_list_ids(cfg, vector_id_to_list_id_map)

class Config:
    def __init__(self, dataset_size_millions, batch_size, n_lists,  vectors_file, vector_ids_file, list_ids_file, temp_dir, output_dir):
        self.indices_dir = join(temp_dir, f"SIFT{dataset_size_millions}M")
        self.output_dir = join(output_dir, f"SIFT{dataset_size_millions}M")
        self.indices_base = join(self.indices_dir, f"SIFT{dataset_size_millions}M")
        self.dataset_size_millions=dataset_size_millions
        self.n_lists=n_lists
        self.trained_index_file=join(temp_dir, "SIFT1000M_trained.index")
        self.merged_index_file=f"{self.indices_base}_merged.index"
        self.vectors_file=join(self.output_dir, vectors_file)
        self.vector_ids_file=join(self.output_dir, vector_ids_file)
        self.list_ids_file=join(self.output_dir, list_ids_file)
        self.batch_size=batch_size

    def get_batch_index_file(self, batch_no):
        return f"{self.indices_base}_{batch_no + 1}_of_{self.n_batches}.index"
        
    def get_vectors_file_size(self):
        return getsize(self.vectors_file)

    def get_expected_vectors_file_size(self, batch_no):
        vector_size = self.dimension * 4
        no_vectors = (batch_no + 1) * self.batch_size
        return no_vectors * vector_size

if __name__ == "__main__":
    pipeline()
