#include "../lib/catch.hpp"

#include "../include/tests/InvertedListsTestUtils.hpp"
#include "../include/storage-node/Index.hpp"

#include <sys/mman.h>
#include <unordered_set>

using namespace ann_dkvs;

typedef float distance_t;
typedef pair<distance_t, list_id_t> centroid_distance_id_t;
typedef priority_queue<centroid_distance_id_t> list_id_heap_t;

auto L2Sqr = [](
                 const void *pVect1v,
                 const void *pVect2v,
                 const void *qty_ptr)
{
  float *pVect1 = (float *)pVect1v;
  float *pVect2 = (float *)pVect2v;
  size_t qty = *((size_t *)qty_ptr);

  float res = 0;
  for (size_t i = 0; i < qty; i++)
  {
    float t = *pVect1 - *pVect2;
    pVect1++;
    pVect2++;
    res += t * t;
  }
  return (res);
};

auto extract_list_ids = [](list_id_heap_t *candidates)
{
  len_t k = candidates->size();
  vector<list_id_t> results(k);
  for (size_t i = 0; i < k; i++)
  {
    results[k - i - 1] = candidates->top().second;
    candidates->pop();
  }
  return results;
};

auto find_nearest_centroids = [](vector_el_t *centroids, len_t n_centroids, vector_el_t *query_vector, len_t vector_dim, len_t number_of_nearest_centroids)
{
  list_id_heap_t nearest_centroids;

  for (list_id_t i = 0; i < (list_id_t) n_centroids; i++)
  {
    vector_el_t *centroid = &centroids[i * vector_dim];
    float distance = L2Sqr(centroid, query_vector, &vector_dim);
    centroid_distance_id_t result = {distance, i};
    if (nearest_centroids.size() < number_of_nearest_centroids)
    {
      nearest_centroids.push(result);
    }
    else if (distance < nearest_centroids.top().first || (distance == nearest_centroids.top().first && i < nearest_centroids.top().second))
    {
      nearest_centroids.pop();
      nearest_centroids.push(result);
    }
  }
  return extract_list_ids(&nearest_centroids);
};

auto alloc_query_as_vector_el = [](uint8_t *query_vector, len_t vector_dim)
{
  vector_el_t *query_vector_float = (vector_el_t *)malloc(vector_dim * sizeof(vector_el_t));
  for (len_t i = 0; i < vector_dim; i++)
  {
    query_vector_float[i] = (vector_el_t)query_vector[i];
  }
  return query_vector_float;
};

string get_centroids_filename(len_t n_lists)
{
  string filename = CENTROIDS_FILENAME;
  string seperator = "_";
  string file_ext = FILE_EXT;
  return filename + seperator + to_string(n_lists) + file_ext;
}

SCENARIO("search_preassigned_list(): use index to find top k ANN of a query vector", "[Index][search_preassigned_list][test][hard-coded]")
{
  GIVEN("an InvertedLists object and a list of 1D vectors, corresponding vector ids and list ids")
  {
    len_t vector_dim = 1;
    InvertedLists lists = get_inverted_lists_object(vector_dim);
    len_t list1_length = 2;
    vector_el_t vectors_list1[] = {2, 4};
    vector_id_t ids_list1[] = {1, 2};

    len_t list2_length = 2;
    vector_el_t vectors_list2[] = {1, 3};
    vector_id_t ids_list2[] = {3, 4};

    list_id_t list_ids[] = {1, 2};

    AND_GIVEN("A query vector, the list ids to search and a value of k")
    {
      vector_el_t query[] = {0};
      size_t k = 3;
      list_id_t list_ids_to_search[] = {1, 2};
      len_t n_lists_to_search = 2;

      WHEN("the InvertedLists object is populated with the vectors, ids and list ids and used to initialize an Index object")
      {
        lists.insert_entries(list_ids[0], vectors_list1, ids_list1, list1_length);
        lists.insert_entries(list_ids[1], vectors_list2, ids_list2, list2_length);

        Index index(&lists);

        WHEN("the Index is queried with a query vector")
        {
          vector<vector_distance_id_t> results = index.search_preassigned(list_ids_to_search, n_lists_to_search, query, k);

          THEN("the results are correct")
          {
            CHECK(results.size() == k);
            CHECK(results[0].first == 1 * 1);
            CHECK(results[0].second == 3);
            CHECK(results[1].first == 2 * 2);
            CHECK(results[1].second == 1);
            CHECK(results[2].first == 3 * 3);
            CHECK(results[2].second == 4);
          }
        }
      }
    }
  }
}
SCENARIO("search_preassigned_list(): test recall with SIFT1M", "[Index][search_preassigned_list][test][SIFT1M]")
{
  GIVEN("the SIFT1M dataset, n_probe as large as the number of clusters")
  {
    len_t vector_dim = 128;
    len_t n_entries = (len_t)1E6;
    len_t n_lists = 1024;
    len_t n_query_vectors = (len_t)1E4;
    len_t groundtruth_k = 1000;
    len_t n_probe = n_lists;
    len_t R = 1;
    string dataset_dir = join(SIFT_OUTPUT_DIR, "SIFT1M");
    // groundtruth format: sizeof(uint32_t) + [groundtruth_k * sizeof(uint32_t)]
    string groundtruth_filepath = join(SIFT_GROUNDTRUTH_DIR, "idx_1M.ivecs");
    // vectors format: n_entries * 128 * sizeof(float)
    string vectors_filepath = join(dataset_dir, get_vectors_filename());
    // vector ids format: n_entries * sizeof(int64_t)
    string vectors_ids_filepath = join(dataset_dir, get_vector_ids_filename());
    // list_ids format: n_entries * sizeof(int64_t)
    string list_ids_filepath = join(dataset_dir, get_list_ids_filename(n_lists));
    // centroids format: n_lists * 128 * sizeof(float)
    string centroids_filepath = join(dataset_dir, get_centroids_filename(n_lists));
    // query_vectors format: n_query_vectors * [sizeof(uint32_t) + 128 * sizeof(uint8_t)]
    string query_vectors_filepath = SIFT_QUERY_VECTORS_FILEPATH;

    THEN("all required files are present")
    {
      REQUIRE(file_exists(vectors_filepath));
      REQUIRE(file_exists(vectors_ids_filepath));
      REQUIRE(file_exists(list_ids_filepath));
      REQUIRE(file_exists(centroids_filepath));
      REQUIRE(file_exists(query_vectors_filepath));
      REQUIRE(file_exists(groundtruth_filepath));
    }

    WHEN("the files are mapped to memory")
    {
      len_t vectors_size = n_entries * vector_dim * sizeof(vector_el_t);
      len_t ids_size = n_entries * sizeof(list_id_t);
      len_t list_ids_size = n_entries * sizeof(list_id_t);
      len_t centroids_size = n_lists * vector_dim * sizeof(vector_el_t);
      len_t query_vectors_size = n_query_vectors * vector_dim * sizeof(vector_el_t);
      len_t groundtruth_size = n_query_vectors * groundtruth_k * sizeof(vector_id_t);

      vector_el_t *vectors = (vector_el_t *)mmap_file(vectors_filepath, vectors_size);
      vector_id_t *vector_ids = (vector_id_t *)mmap_file(vectors_ids_filepath, ids_size);
      list_id_t *list_ids = (list_id_t *)mmap_file(list_ids_filepath, list_ids_size);
      vector_el_t *centroids = (vector_el_t *)mmap_file(centroids_filepath, centroids_size);
      uint8_t *query_vectors = (uint8_t *)mmap_file(query_vectors_filepath, query_vectors_size);
      uint32_t *groundtruth = (uint32_t *)mmap_file(groundtruth_filepath, groundtruth_size);

      THEN("the files are mapped correctly")
      {
        REQUIRE(vectors != nullptr);
        REQUIRE(vector_ids != nullptr);
        REQUIRE(list_ids != nullptr);
        REQUIRE(centroids != nullptr);
        REQUIRE(query_vectors != nullptr);
        REQUIRE(groundtruth != nullptr);
      }

      WHEN("the InvertedLists object is populated with the vectors, ids and list ids and used to initialize an Index object")
      {

        InvertedLists lists = get_inverted_lists_object(vector_dim);
        lists.bulk_insert_entries(vectors_filepath, vectors_ids_filepath, list_ids_filepath, n_entries);
        Index index(&lists);

        WHEN("for each query vector, the closest centroids are determined, their lists are searched for the nearest R neighbors")
        {
          len_t n_correct = 0;
          for (len_t query_id = 0; query_id < n_query_vectors; query_id++)
          {
            uint8_t *query_bytes = &query_vectors[query_id * (vector_dim + 4) + 4];
            vector_el_t *query = alloc_query_as_vector_el(query_bytes, vector_dim);
            vector<list_id_t> list_ids_to_search = find_nearest_centroids(centroids, n_lists, query, vector_dim, n_probe);
            vector<vector_distance_id_t> nearest_neighbors = index.search_preassigned(list_ids_to_search.data(), list_ids_to_search.size(), query, R);
            vector_id_t groundtruth_first_nearest_neighbor = groundtruth[query_id * (groundtruth_k + 1) + 1];
            bool is_correct = false;
            for (len_t i = 0; i < nearest_neighbors.size(); i++)
            {
              if (nearest_neighbors[i].second == groundtruth_first_nearest_neighbor)
              {
                n_correct++;
                is_correct = true;
                break;
              }
            }
            if (!is_correct)
            {
              cout << "True nearest neighbor " << groundtruth_first_nearest_neighbor << " of query " << query_id << " not found among results" << endl;
            }
            free(query);
          }
          THEN("the Recall@1 is 100%")
          {
            REQUIRE(n_correct == n_query_vectors);
          }
        }
      }
      munmap(vectors, vectors_size);
      munmap(vector_ids, ids_size);
      munmap(list_ids, list_ids_size);
      munmap(centroids, centroids_size);
      munmap(query_vectors, query_vectors_size);
      munmap(groundtruth, groundtruth_size);
    }
  }
}