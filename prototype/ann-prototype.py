import faiss
import numpy as np
import hnswlib
from faiss.contrib import ivf_tools
import os
import argparse

try:
    from faiss.contrib.datasets_fb import DatasetSIFT1M
except ImportError:
    from faiss.contrib.datasets import DatasetSIFT1M


def import_data():
    # Assumes that the dataset is in data/sift1M
    ds = DatasetSIFT1M()
    xq = ds.get_queries()
    xb = ds.get_database()
    gt = ds.get_groundtruth()
    xt = ds.get_train()
    return xq, xb, gt, xt


def get_invlist(index, id):
    invlists = index.invlists
    list_len = invlists.list_size(id)
    list_ids = invlists.get_ids(id)
    list_vec_ids = faiss.rev_swig_ptr(list_ids, list_len)
    return list_vec_ids


def get_centroid(index, id):
    return index.quantizer.reconstruct(id)


def get_centroids(index):
    return index.quantizer.reconstruct_n(0, index.nlist)


def build_IVFFlat(xb, xt, d, npartitions):
    print("Clustering using IVFFlat")
    quantizer = faiss.IndexFlatL2(d)
    index = faiss.IndexIVFFlat(quantizer, d, npartitions)
    index.train(xt)
    index.add(xb)
    centroids = get_centroids(index)
    inverted_lists = []
    for i in range(npartitions):
        inverted_lists.append(get_invlist(index, i))
    return index, centroids, inverted_lists


def verify_centroids(xb, centroids, inverted_lists):
    npartitions = centroids.shape[0]
    assert npartitions == len(inverted_lists)
    closest_centroid = np.zeros(npartitions, dtype=int)

    for i in range(npartitions):
        list_vec_ids = inverted_lists[i]
        list_vecs = xb[list_vec_ids]
        clostest_centroid_id = -1
        clostest_centroid_dist = np.inf
        for j in range(npartitions):
            dist = np.min(np.linalg.norm(list_vecs - centroids[j], axis=1))
            if dist < clostest_centroid_dist:
                clostest_centroid_dist = dist
                clostest_centroid_id = j
        closest_centroid[i] = clostest_centroid_id

    # check that closest centroid of list i is the same as the centroid i
    assert np.all(np.equal(closest_centroid, np.arange(npartitions)))


def build_hnsw(centroids, ef, ef_construction, m):
    print("Creating HNSW index and inserting centroids")
    d = centroids.shape[1]
    npartitions = centroids.shape[0]
    hnsw = hnswlib.Index(space='l2', dim=d)
    hnsw.init_index(max_elements=npartitions,
                    ef_construction=ef_construction,
                    M=m)
    hnsw.add_items(centroids)
    hnsw.set_ef(ef)
    return hnsw


def query(ivfflat, hnsw, xq, k, nprobe):
    print("Querying HNSW index to find nearest centroids")
    nq, d = xq.shape
    partition_I, _ = hnsw.knn_query(xq, k=nprobe)
    # convert data type of partition_ids to int64 to match function signature of faiss::IndexIVF::search_preassigned
    partition_I = partition_I.astype(np.int64)
    ivfflat.nprobe = nprobe
    print("Querying IVF Flat index, restricted to partitions selected by HNSW")
    D, I = ivf_tools.search_preassigned(ivfflat, xq, k, partition_I)
    return D, I


def get_recall(I, gt):
    nq, k = I.shape
    assert gt.shape[0] == nq and gt.shape[1] >= k
    n_correct = np.sum(I == gt[:, :k])
    recall = n_correct / float(nq * k)
    return recall


def pipeline(args):
    xq, xb, gt, xt = import_data()
    nq, d = xq.shape
    ivfflat, centroids, inverted_lists = build_IVFFlat(xb, xt, d,
                                                       args.npartitions)
    if VERIFY_CENTROIDS:
        verify_centroids(xb, centroids, inverted_lists)
    hnsw = build_hnsw(centroids, args.ef, args.ef_construction, args.m)
    D, I = query(ivfflat, hnsw, xq, args.k, args.nprobe)
    recall = get_recall(I, gt)
    for arg_key, arg_value in vars(args).items():
        print(arg_key, ":= ", arg_value)
    print("recall := ", recall)


if __name__ == "__main__":
    if os.path.basename(os.getcwd()) != "prototype":
        print("please run this script from the prototype directory")
        exit(1)
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        "--verify-centroids",
        action="store_true",
        default=False,
        help=
        "Verify that the centroids are the closest to the vectors in their inverted lists"
    )
    parser.add_argument(
        "--npartitions",
        type=int,
        default=1024,
    )
    parser.add_argument("--ef",
                        type=int,
                        default=10,
                        help="ef parameter for HNSW")
    parser.add_argument("--ef-construction",
                        type=int,
                        default=100,
                        help="ef_construction parameter for HNSW")
    parser.add_argument("--m",
                        type=int,
                        default=16,
                        help="M parameter for HNSW")
    parser.add_argument("--k",
                        type=int,
                        default=1,
                        help="number of results to return")
    parser.add_argument("--nprobe",
                        type=int,
                        default=10,
                        help="nprobe parameter for IVFFlat")
    args = parser.parse_args()
    pipeline(args)