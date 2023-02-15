# `search_preassigned_list`: Correctly finds 1st nearest neighbor
- Dataset: `cluster/out/SIFT1M`
- Groundtruth: `cluster/data/bigann/gnd/idx_1M.ivecs`
- `n_probe = n_lists = 1024`


## 1st NN for query `5759` does not match groundtruth

```
Filters: [StorageIndex][SIFT1M]
[...]
Inserted 1000000 entries of 1000000 (100%)
True nearest neighbor 151761 of query 5759 not found among results
-------------------------------------------------------------------------------
Scenario: search_preassigned_list(): test recall with SIFT1M
      Given: the SIFT1M dataset, n_probe as large as the number of clusters
       When: the files are mapped to memory
       When: the InvertedLists object is populated with the vectors, ids and
             list ids and used to initialize an StorageIndex object
       When: for each query vector, the closest centroids are determined, their
             lists are searched for the nearest R neighbors
       Then: the Recall@1 is 100%
-------------------------------------------------------------------------------
tests/IndexTest.cpp:235
...............................................................................

tests/IndexTest.cpp:237: FAILED:
  REQUIRE( n_correct == n_query_vectors )
with expansion:
  9999 (0x270f) == 10000 (0x2710)

[...]
===============================================================================
test cases:  1 |  0 passed | 1 failed
assertions: 13 | 12 passed | 1 failed
```
- This is due to the fact that
  - Query `5759` has two nearest neighbors with the same distance.
  - The groundtruth has a different ordering of the two nearest neighbors than the one returned by the search.
  - The priority queue used to find the NN has size 1 and we only compare against the first element of minimum distance to the query.
```
Filters: [StorageIndex][SIFT1M]
Inserted 1000000 entries of 1000000 (100%)
query_id: 5759
adding candidate 7446 with distance 170163
replacing candidate 7446 with distance 170163 with candidate 8845 with distance 89078
replacing candidate 8845 with distance 89078 with candidate 12512 with distance 63584
replacing candidate 12512 with distance 63584 with candidate 66043 with distance 62856
replacing candidate 66043 with distance 62856 with candidate 390397 with distance 52826
replacing candidate 390397 with distance 52826 with candidate 576152 with distance 46743
replacing candidate 576152 with distance 46743 with candidate 580949 with distance 46530
checking vector 151761 with distance: 46530
current worst candidate 580949 with distance: 46530
True nearest neighbor 151761 of query 5759 not found among results
```

## Results of updated implementation matches the groundtruth
- The updated implementation ensures that the nearest neighbors are returned in the same order as the groundtruth.
```
Filters: [StorageIndex][SIFT1M]
[...]
Inserted 1000000 entries of 1000000 (100%)
query_id: 5759
adding candidate 7446 with distance 170163
replacing candidate 7446 with distance 170163 with candidate 8845 with distance 89078
replacing candidate 8845 with distance 89078 with candidate 12512 with distance 63584
replacing candidate 12512 with distance 63584 with candidate 66043 with distance 62856
replacing candidate 66043 with distance 62856 with candidate 390397 with distance 52826
replacing candidate 390397 with distance 52826 with candidate 576152 with distance 46743
replacing candidate 576152 with distance 46743 with candidate 580949 with distance 46530
checking vector 151761 with distance: 46530
current worst candidate 580949 with distance: 46530
replacing candidate 580949 with distance 46530 with candidate 151761 with distance 46530
[...]
```
```
Filters: [StorageIndex][SIFT1M]
Inserted 1000000 entries of 1000000 (100%)
===============================================================================
All tests passed (13 assertions in 1 test case)
```
- This is accomplished by
  - Sorting the candidates by their distance to the query vector and then additionally by their id.
  - And adding a custom comparator to the priority queue which ensures this order and which is used to maintain the max heap property.
```c++
  vector_el_t *vector = &vectors[j * vector_dim];
  float distance = L2Sqr(vector, query, &vector_dim);
  vector_id_t id = ids[j];
  vector_distance_id_t result = {distance, id};
  if (candidates->size() < k)
  {
    candidates->push(result);
  }
  else if (distance < candidates->top().first || (distance == candidates->top().first && id < candidates->top().second))
  {
    candidates->pop();
    candidates->push(result);
  }
```
 

```c++
  typedef float distance_t;
  typedef pair<distance_t, vector_id_t> vector_distance_id_t;
  class VectorDistanceIdMaxHeapCompare
  {
    public:
      bool operator()(vector_distance_id_t below, vector_distance_id_t above)
      {
        if (below.first > above.first)
        {
          return true;
        }
        else if (below.first == above.first && below.second < above.second)
        {
          return true;
        }
        return false;
      }
  };
  typedef priority_queue<vector_distance_id_t, vector<vector_distance_id_t>, VectorDistanceIdMaxHeapCompare> heap_t;
```

# `search_preassigned_list`: Recall@1 for SIFT1M
- `n_lists = [256, 512, 1024, 2048, 4096]`
- `n_probe = [1, 2, ..., n_lists]`

![index-recall-at-1](index-recall-at-1.jpg)