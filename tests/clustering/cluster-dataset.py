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
    if dataset == "sift1M":
        ds = DatasetSIFT1M()
        xb = ds.get_database()
    elif dataset == "bigann":
        ds = DatasetBigANN()
        xb = ds.database_iterator()
    else:
        raise ValueError("Unknown dataset: {}".format(dataset))
    xt = ds.get_train()
    return xb, xt


def build_IVFFlat(xb, xt, d, npartitions):
    quantizer = faiss.IndexFlatL2(d)
    index = faiss.IndexIVFFlat(quantizer, d, npartitions)
    index.train(xt)
    if isinstance(xb, np.ndarray):
        index.add(xb)
    else:
        for x in xb:
            index.add(x)
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
    with open(filename, "wb") as f:
        vectors.tofile(f)


def write_vector_ids(n, filename):
    with open(filename, "wb") as f:
        for i in range(n):
            f.write(i.to_bytes(8, byteorder='little'))


def write_list_ids(vector_id_to_list_id_map, filename):
    with open(filename, "wb") as f:
        for i in range(len(vector_id_to_list_id_map)):
            list_id = vector_id_to_list_id_map[i]
            f.write(list_id.to_bytes(8, byteorder='little'))


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
    print("Creating output directory", OUTPUT_DIR)
    makedirs(OUTPUT_DIR)

    try:
        xb, xt = import_data(dataset)
        d = xt.shape[1]
        n = 10**9 if dataset == "bigann" else xb.shape[0]
    except FileNotFoundError:
        print(
            f"Could not find {dataset} dataset in data/{dataset}, please download it first from", datasets_link)
        return

    print(f"Clustering {dataset} using IVFFlat")
    index = build_IVFFlat(xb, xt, d, N_LISTS)
    vector_id_to_list_id_map = get_vector_id_to_list_id_map(index)

    print("Writing vectors, vector ids and list ids to file")
    write_vectors(xb, join(OUTPUT_DIR, VECTORS_FILE))
    write_vector_ids(n, join(OUTPUT_DIR, VECTOR_IDS_FILE))
    write_list_ids(vector_id_to_list_id_map, join(OUTPUT_DIR, LIST_IDS_FILE))


if __name__ == "__main__":
    pipeline()
