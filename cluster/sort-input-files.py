import argparse
import os
from os.path import join
from tqdm import tqdm

VECTOR_BYTES = 128 * 4
VECTOR_ID_BYTES = 8
LIST_ID_BYTES = 8


def sort(vectors_file, vector_ids_file, list_ids_file, vectors_file_sorted,
         vector_ids_file_sorted, list_ids_file_sorted, dataset_size):
    list_id_entry_id_pairs = []
    print("Reading list ids")
    with open(list_ids_file, "rb") as f:
        for _ in tqdm(range(dataset_size)):
            list_id = f.read(LIST_ID_BYTES)
            if not list_id:
                print("Unexpected end of file")
                exit(1)
            list_id_entry_id_pairs.append(
                (list_id, len(list_id_entry_id_pairs)))

    print("Sorting list ids")
    list_id_entry_id_pairs.sort()

    print(f"Writing sorted list ids to {list_ids_file_sorted}")
    if not os.path.isfile(list_ids_file_sorted):
        with open(list_ids_file_sorted, "wb") as f:
            for list_id, _ in tqdm(list_id_entry_id_pairs):
                f.write(list_id)
    else:
        print(f"\tFile {list_ids_file_sorted} already exists, skipping")
    assert (os.path.getsize(list_ids_file_sorted) ==
            get_expected_list_ids_file_size(dataset_size))

    print(f"Writing sorted vector ids to {vector_ids_file_sorted}")
    if not os.path.isfile(vector_ids_file_sorted):
        with open(vector_ids_file, "rb") as f_in, open(vector_ids_file_sorted,
                                                       "wb") as f_out:
            for _, entry_id in tqdm(list_id_entry_id_pairs):
                f_in.seek(VECTOR_ID_BYTES * entry_id)
                vector_id = f_in.read(VECTOR_ID_BYTES)
                f_out.write(vector_id)
    else:
        print(f"\tFile {vector_ids_file_sorted} already exists, skipping")
    assert (os.path.getsize(vector_ids_file_sorted) ==
            get_expected_vector_ids_file_size(dataset_size))

    print(f"Writing sorted vectors to {vectors_file_sorted}")
    if not os.path.isfile(vectors_file_sorted):
        with open(vectors_file, "rb") as f_in, open(vectors_file_sorted,
                                                    "wb") as f_out:
            for _, entry_id in tqdm(list_id_entry_id_pairs):
                f_in.seek(VECTOR_BYTES * entry_id)
                vector = f_in.read(VECTOR_BYTES)
                f_out.write(vector)
    else:
        print(f"\tFile {vectors_file_sorted} already exists, skipping")
    assert (os.path.getsize(vectors_file_sorted) ==
            get_expected_vectors_file_size(dataset_size))


def get_expected_vector_ids_file_size(dataset_size):
    return dataset_size * VECTOR_ID_BYTES


def get_expected_vectors_file_size(dataset_size):
    return dataset_size * VECTOR_BYTES


def get_expected_list_ids_file_size(dataset_size):
    return dataset_size * LIST_ID_BYTES


def user_confirmation(dataset, n_lists, vectors_file, vector_ids_file,
                      list_ids_file, vectors_file_sorted,
                      vector_ids_file_sorted, list_ids_file_sorted, base_dir):
    print(f"Sorting clustered dataset with the following config:")
    print(f"\tDataset: {dataset}")
    print(f"\tNumber of clusters: {n_lists}")
    print(f"\tBase directory: {base_dir}")
    print(f"Sorting the following files:")
    print(f"\tVectors: {vectors_file}")
    print(f"\tVector ids: {vector_ids_file}")
    print(f"\tList ids: {list_ids_file}")
    print(f"Output files:")
    print(f"\tVectors: {vectors_file_sorted}")
    print(f"\tVector ids: {vector_ids_file_sorted}")
    print(f"\tList ids: {list_ids_file_sorted}")
    if not input("Continue? [Y/N] ").lower().startswith("y"):
        print("Aborting")
        exit(1)


def get_dataset_size(dataset):
    if dataset == "SIFT1M":
        return 1_000_000
    elif dataset == "SIFT10M":
        return 10_000_000
    elif dataset == "SIFT100M":
        return 100_000_000
    elif dataset == "SIFT1000M" or dataset == "SIFT1B":
        return 1_000_000_000
    else:
        raise ValueError(f"Unknown dataset {dataset}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog="Sort clustered SIFT dataset",
        description=
        "Sort pre-clustered SIFT dataset by list ids to test potential performance improvements when used as input for bulk insertion"
    )
    parser.add_argument(
        "--dataset",
        type=str,
        default="SIFT1M",
        help=
        "Dataset to cluster, one of SIFT1M, SIFT10M, SIFT100M, SIFT1000M/1B",
        choices=["SIFT1M", "SIFT10M", "SIFT100M", "SIFT1000M", "SIFT1B"])
    parser.add_argument("--n_lists",
                        type=int,
                        default=2**10,
                        help="Number of clusters")
    parser.add_argument("--vectors_file",
                        type=str,
                        default="vectors",
                        help="File to output vectors to")
    parser.add_argument("--vector_ids_file",
                        type=str,
                        default="vector_ids",
                        help="File to output vector ids to")
    parser.add_argument("--list_ids_file",
                        type=str,
                        default="list_ids",
                        help="File to output list ids to")
    parser.add_argument(
        "--output_dir",
        type=str,
        default="./out",
        help=
        "Directory where the clustered data is located and where to store output files in"
    )
    args = parser.parse_args()

    base_dir = join(args.output_dir, args.dataset)
    vectors_file = join(base_dir, args.vectors_file + ".bin")
    vector_ids_file = join(base_dir, args.vector_ids_file + ".bin")
    list_ids_file = join(base_dir, f"{args.list_ids_file}_{args.n_lists}.bin")

    suffix = f"_{args.n_lists}_sorted.bin"
    vectors_file_sorted = join(base_dir, args.vectors_file + suffix)
    vector_ids_file_sorted = join(base_dir, args.vector_ids_file + suffix)
    list_ids_file_sorted = join(base_dir, args.list_ids_file + suffix)

    user_confirmation(args.dataset, args.n_lists, vectors_file,
                      vector_ids_file, list_ids_file, vectors_file_sorted,
                      vector_ids_file_sorted, list_ids_file_sorted, base_dir)
    dataset_size = get_dataset_size(args.dataset)

    # ensure input files exist
    for file in [vectors_file, vector_ids_file, list_ids_file]:
        if not os.path.isfile(file):
            print(f"Input file {file} does not exist")
            exit(1)

    sort(vectors_file, vector_ids_file, list_ids_file, vectors_file_sorted,
         vector_ids_file_sorted, list_ids_file_sorted, dataset_size)
