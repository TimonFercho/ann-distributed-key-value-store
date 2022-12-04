import faiss
import numpy as np
from os.path import join, exists
from os import makedirs, rename, remove, environ
import urllib.request
import tarfile

try:
    from faiss.contrib.datasets_fb import DatasetSIFT1M
except ImportError:
    from faiss.contrib.datasets import DatasetSIFT1M


def download_data():
    sift1M_link = "ftp://ftp.irisa.fr/local/texmex/corpus/sift.tar.gz"
    data_dir = "data"
    sift1M_dir = join(data_dir, "sift1M")
    sift1M_tar = "sift.tar.gz"
    if exists(sift1M_dir):
        print("Directory already exists", sift1M_dir)
        return
    if not exists(data_dir):
        print("Creating data directory", data_dir)
        makedirs(data_dir)
    sift1M_tar_path = join(data_dir, sift1M_tar)
    print("Downloading SIFT1M")
    urllib.request.urlretrieve(sift1M_link, sift1M_tar_path)
    print("Extracting SIFT1M")
    with tarfile.open(sift1M_tar_path, "r:gz") as tar:
        tar.extractall(path=data_dir)
    sift_dir = join(data_dir, "sift")
    rename(sift_dir, sift1M_dir)
    print("Removing temporary files")
    remove(sift1M_tar_path)


def import_data():
    # Assumes that the dataset is in data/sift1M
    # fvecs format:
    # n * [[int] + d * [float32]]
    # where n is the number of vectors,
    # d is the dimension (128),
    # the int is the vector dimension
    # and the float32 is one vector component
    ds = DatasetSIFT1M()
    xq = ds.get_queries()
    xb = ds.get_database()
    gt = ds.get_groundtruth()
    xt = ds.get_train()
    return xq, xb, gt, xt


def build_IVFFlat(xb, xt, d, npartitions):
    quantizer = faiss.IndexFlatL2(d)
    index = faiss.IndexIVFFlat(quantizer, d, npartitions)
    index.train(xt)
    index.add(xb)
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
    N_LISTS = 1024
    VECTORS_FILE = "vectors.bin"
    VECTOR_IDS_FILE = "vector_ids.bin"
    LIST_IDS_FILE = "list_ids.bin"
    OUTPUT_DIR = join("out", "sift1M")
    if exists(OUTPUT_DIR):
        print("Output directory already exists", OUTPUT_DIR)
        return
    print("Creating output directory", OUTPUT_DIR)
    makedirs(OUTPUT_DIR)

    try:
        _, xb, _, xt = import_data()
        n, d = xb.shape
    except FileNotFoundError:
        download_data()
        _, xb, _, xt = import_data()
        n, d = xb.shape

    if not (environ.get('LD_PRELOAD') and 'libmkl_core.so' in environ.get('LD_PRELOAD') and 'libmkl_sequential.so' in environ.get('LD_PRELOAD')):
        print('Please run using make to set the environment variables')
        return

    print("Clustering SIFT1M using IVFFlat")
    index = build_IVFFlat(xb, xt, d, N_LISTS)
    vector_id_to_list_id_map = get_vector_id_to_list_id_map(index)

    print("Writing vectors, vector ids and list ids to file")
    write_vectors(xb, join(OUTPUT_DIR, VECTORS_FILE))
    write_vector_ids(n, join(OUTPUT_DIR, VECTOR_IDS_FILE))
    write_list_ids(vector_id_to_list_id_map, join(OUTPUT_DIR, LIST_IDS_FILE))


if __name__ == "__main__":
    pipeline()
