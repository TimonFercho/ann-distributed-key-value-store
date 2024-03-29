#pragma once

#define TMP_DIR "tests/tmp"
#define SIFT_OUTPUT_DIR "cluster/out"
#define SIFT_GROUNDTRUTH_DIR "cluster/data/bigann/gnd"
#define SIFT_QUERY_VECTORS_FILEPATH "cluster/data/bigann/bigann_query.bvecs"
#define LISTS_FILENAME "lists"
#define VECTORS_FILENAME "vectors"
#define VECTOR_IDS_FILENAME "vector_ids"
#define LIST_IDS_FILENAME "list_ids"
#define CENTROIDS_FILENAME "centroids"
#define FILE_EXT ".bin"

#define MAX_VECTOR_DIM 128
#define MIN_LIST_LENGTH 1
#define MAX_LIST_LENGTH (int)1E3
#define MAX_N_LISTS 100

#define N_VECTOR_DIM_SAMPLES 5
#define N_LIST_ID_SAMPLES 5
#define N_LIST_LENGTH_SAMPLES 5
#define N_VECTOR_SAMPLES 10

#define MAX_VECTOR_ID std::numeric_limits<int>::max()
#define MAX_LIST_ID std::numeric_limits<int>::max()
#define MIN_VECTOR_VAL -3.40282e+38
#define MAX_VECTOR_VAL 3.40282e+38