import faiss
import numpy as np
from os.path import join, exists
from os import makedirs, rename, remove

try:
    from faiss.contrib.datasets_fb import DatasetSIFT1M, DatasetBigANN
except ImportError:
    from faiss.contrib.datasets import DatasetSIFT1M, DatasetBigANN

datasets_link = "http://corpus-texmex.irisa.fr/"


def import_data_SITF1M():
    # Assumes that the dataset is in data/sift1M
    # fvecs format:
    # n * [[int] + d * [float32]]
    # where n is the number of vectors,
    # d is the dimension (128),
    # the int is the vector dimension
    # and the float32 is one vector component

    return xb, xt


def import_data(dataset="sift1M"):
    # Assumes that the dataset is in data/{dataset}
    # fvecs format:
    # n * [[int] + d * [float32]]
    # where n is the number of vectors,
    # d is the dimension (128),
    # the int is the vector dimension
    # and the float32 is one vector component
    ds, xb = None, None
    try:
        if dataset == "sift1M":
            ds = DatasetSIFT1M()
            xb = ds.get_database()
        elif dataset == "bigann":
            ds = DatasetBigANN()
            # first 10 batches of 1M vectors each => 10M out of 1B
            xb = ds.database_iterator(bs=10**6, split=(10**2, 0)) 
        else:
            raise ValueError("Unknown dataset: {}".format(dataset))
    except FileNotFoundError:
        print(
            f"Could not find {dataset} dataset in data/{dataset}, please download it first from", datasets_link)
        exit(1)
    xt = ds.get_train()
    return xb, xt, ds.d, ds.nb


def build_IVFFlat_and_write_vectors(xb, xt, d, npartitions, filename):
    quantizer = faiss.IndexFlatL2(d)
    index = faiss.IndexIVFFlat(quantizer, d, npartitions)
    index.train(xt)
    with open(filename, "wb") as f:
        if isinstance(xb, np.ndarray):
            index.add(xb)
            xb.tofile(f)
        else:
            i = 0
            for x in xb:
                print(f"Adding {i}'th batch")
                index.add(x)
                x.tofile(f)
                i += 1
    return index


def get_invlist(index, id):
    invlists = index.invlists
    list_len = invlists.list_size(id)
    list_ids = invlists.get_ids(id)
    list_vec_ids = faiss.rev_swig_ptr(list_ids, list_len)
    return list_vec_ids


def get_vector_id_to_list_id_map(index):
    vector_id_to_list_id_map = {}
    for i in range(index.nlist):
        invlist = get_invlist(index, i)
        for vec_id in invlist:
            vector_id_to_list_id_map[vec_id] = i
    return vector_id_to_list_id_map


def write_vectors(vectors, filename):
    if isinstance(vectors, np.ndarray):
        with open(filename, "wb") as f:
            vectors.tofile(f)

def write_vector_ids(n, filename):
    ids = np.arange(n, dtype=np.int64)
    with open(filename, "wb") as f:
        ids.tofile(f)


def write_list_ids(vector_id_to_list_id_map, filename):
    ids = np.arange(len(vector_id_to_list_id_map), dtype=np.int64)
    map_all_to_list_ids = np.vectorize(lambda i: vector_id_to_list_id_map[i])
    list_ids = map_all_to_list_ids(ids)
    with open(filename, "wb") as f:
        list_ids.tofile(f)


def pipeline():
    dataset = "bigann"
    N_LISTS = 1024
    VECTORS_FILE = "vectors.bin"
    VECTOR_IDS_FILE = "vector_ids.bin"
    LIST_IDS_FILE = "list_ids.bin"
    OUTPUT_DIR = join("out", dataset)
    if exists(OUTPUT_DIR):
        print("Output directory already exists", OUTPUT_DIR)
        return

    print("Importing data")
    xb, xt, d, n = import_data(dataset)

    print("Creating output directory", OUTPUT_DIR)
    makedirs(OUTPUT_DIR)

    print(f"Clustering {dataset} using IVFFlat and writing vectors to {join(OUTPUT_DIR, VECTORS_FILE)}")
    index = build_IVFFlat_and_write_vectors(xb, xt, d, N_LISTS, join(OUTPUT_DIR, VECTORS_FILE))

    print("Constructing vector id to list id map")
    vector_id_to_list_id_map = get_vector_id_to_list_id_map(index)

    print("Writing vector ids and list ids to file")
    write_vector_ids(n, join(OUTPUT_DIR, VECTOR_IDS_FILE))
    write_list_ids(vector_id_to_list_id_map, join(OUTPUT_DIR, LIST_IDS_FILE))


if __name__ == "__main__":
    pipeline()
