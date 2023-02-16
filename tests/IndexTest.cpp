#include <sys/mman.h>
#include <unordered_set>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "../lib/catch.hpp"

#include "../include/tests/InvertedListsTestUtils.hpp"
#include "../include/storage-node/StorageIndex.hpp"
#include "../include/L2Space.hpp"
#include "../include/root-node/RootIndex.hpp"

#define UNUSED(x) (void)(x)

using namespace ann_dkvs;

auto alloc_query_as_vector_el = [](uint8_t *query_vector, len_t vector_dim)
{
  vector_el_t *query_vector_float = (vector_el_t *)malloc(vector_dim * sizeof(vector_el_t));
  for (len_t i = 0; i < vector_dim; i++)
  {
    query_vector_float[i] = (vector_el_t)query_vector[i];
  }
  return query_vector_float;
};

std::string get_centroids_filename(len_t n_lists)
{
  std::string filename = CENTROIDS_FILENAME;
  std::string seperator = "_";
  std::string file_ext = FILE_EXT;
  return filename + seperator + std::to_string(n_lists) + file_ext;
}

SCENARIO("search_preassigned(): use index to find top k ANN of a query vector", "[StorageIndex][search_preassigned][test][hard-coded]")
{
  GIVEN("an InvertedLists object and a list of 1D vectors, corresponding std::vector ids and list ids")
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

      WHEN("the InvertedLists object is populated with the vectors, ids and list ids and used to initialize an StorageIndex object")
      {
        lists.insert_entries(list_ids[0], vectors_list1, ids_list1, list1_length);
        lists.insert_entries(list_ids[1], vectors_list2, ids_list2, list2_length);

        StorageIndex index(&lists);

        WHEN("the StorageIndex is queried with a query vector")
        {
          std::vector<vector_distance_id_t> results = index.search_preassigned(list_ids_to_search, n_lists_to_search, query, k);

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

auto setup_indices_and_run = [](len_t n_probe,
                                len_t n_lists,
                                len_t n_entries,
                                len_t n_query_vectors,
                                len_t groundtruth_k,
                                len_t vector_dim,
                                bool mmap_groundtruth,
                                std::string dataset,
                                std::string groundtruth_filename,
                                std::function<void(uint8_t *, uint32_t *, StorageIndex *, RootIndex *)> run)
{
  if (n_probe <= n_lists)
  {
    std::string dataset_dir = join(SIFT_OUTPUT_DIR, dataset);
    // groundtruth format: sizeof(uint32_t) + [groundtruth_k * sizeof(uint32_t)]
    std::string groundtruth_filepath = join(SIFT_GROUNDTRUTH_DIR, groundtruth_filename);
    // vectors format: n_entries * 128 * sizeof(float)
    std::string vectors_filepath = join(dataset_dir, get_vectors_filename());
    // std::vector ids format: n_entries * sizeof(int64_t)
    std::string vectors_ids_filepath = join(dataset_dir, get_vector_ids_filename());
    // list_ids format: n_entries * sizeof(int64_t)
    std::string list_ids_filepath = join(dataset_dir, get_list_ids_filename(n_lists));
    // centroids format: n_lists * 128 * sizeof(float)
    std::string centroids_filepath = join(dataset_dir, get_centroids_filename(n_lists));
    // query_vectors format: n_query_vectors * [sizeof(uint32_t) + 128 * sizeof(uint8_t)]
    std::string query_vectors_filepath = SIFT_QUERY_VECTORS_FILEPATH;

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
      uint32_t *groundtruth = nullptr;
      if (mmap_groundtruth)
      {
        groundtruth = (uint32_t *)mmap_file(groundtruth_filepath, groundtruth_size);
      }

      THEN("the files are mapped correctly")
      {
        REQUIRE(vectors != nullptr);
        REQUIRE(vector_ids != nullptr);
        REQUIRE(list_ids != nullptr);
        REQUIRE(centroids != nullptr);
        REQUIRE(query_vectors != nullptr);
        if (mmap_groundtruth)
        {
          REQUIRE(groundtruth != nullptr);
        }
      }

      WHEN("the InvertedLists object is populated with the vectors, ids and list ids and used to initialize an StorageIndex object")
      {

        InvertedLists lists = get_inverted_lists_object(vector_dim);
        lists.bulk_insert_entries(vectors_filepath, vectors_ids_filepath, list_ids_filepath, n_entries);
        StorageIndex storage_index(&lists);
        RootIndex root_index(centroids, n_lists);

        run(query_vectors, groundtruth, &storage_index, &root_index);
      }
      munmap(vectors, vectors_size);
      munmap(vector_ids, ids_size);
      munmap(list_ids, list_ids_size);
      munmap(centroids, centroids_size);
      munmap(query_vectors, query_vectors_size);
      if (mmap_groundtruth)
      {
        munmap(groundtruth, groundtruth_size);
      }
    }
  }
};

SCENARIO("search_preassigned(): test recall with SIFT1M", "[StorageIndex][search_preassigned][test][SIFT1M]")
{
  GIVEN("the SIFT1M dataset")
  {
    len_t vector_dim = 128;
    len_t n_entries = (len_t)1E6;
    len_t n_query_vectors = (len_t)1E4;
    len_t groundtruth_k = 1000;
    len_t R = 2;
    len_t n_lists = GENERATE(256, 512, 1024, 2048, 4096);
    len_t n_probe = GENERATE(1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096);

    auto run = [=](uint8_t *query_vectors, uint32_t *groundtruth, StorageIndex *storage_index, RootIndex *root_index)
    {
      WHEN("for each query vector, the closest centroids are determined, their lists are searched for the nearest R neighbors")
      {
        len_t n_correct = 0;
        for (len_t query_id = 0; query_id < n_query_vectors; query_id++)
        {
          uint8_t *query_bytes = &query_vectors[query_id * (vector_dim + 4) + 4];
          vector_el_t *query = alloc_query_as_vector_el(query_bytes, vector_dim);
          std::vector<list_id_t> list_ids_to_search = root_index->find_nearest_centroids(query, vector_dim, n_probe);

          std::vector<vector_distance_id_t> nearest_neighbors = storage_index->search_preassigned(list_ids_to_search.data(), list_ids_to_search.size(), query, R);
          vector_id_t groundtruth_first_nearest_neighbor = groundtruth[query_id * (groundtruth_k + 1) + 1];
          for (len_t i = 0; i < nearest_neighbors.size(); i++)
          {
            if (nearest_neighbors[i].second == groundtruth_first_nearest_neighbor)
            {
              n_correct++;
              break;
            }
          }
          free(query);
        }
        WARN("n_lists := " << n_lists);
        WARN("n_probe := " << n_probe);
        float recall = (float)n_correct / n_query_vectors;
        WARN("Recall@1 := " << recall);
        THEN("the Recall@1 is 100% if we search all lists")
        {
          if (n_probe == n_lists)
          {
            REQUIRE(recall == 1.0);
          }
        }
      }
    };

    setup_indices_and_run(n_probe, n_lists, n_entries, n_query_vectors, groundtruth_k, vector_dim, true, "SIFT1M", "idx_1M.ivecs", run);
  }
}

SCENARIO("search_preassigned(): benchmark querying with SIFT1M", "[StorageIndex][search_preassigned][benchmark][SIFT1M]")
{
  len_t vector_dim = 128;
  len_t n_entries = (len_t)1E6;
  len_t n_query_vectors = (len_t)1E4;
  len_t R = 1;
  len_t groundtruth_k = 1000;
  len_t n_lists = GENERATE(256, 512, 1024, 2048, 4096);
  len_t n_probe = GENERATE(1, 2, 4, 8, 16, 32, 64, 128);

  auto run = [=](uint8_t *query_vectors, uint32_t *groundtruth, StorageIndex *storage_index, RootIndex *root_index)
  {
    UNUSED(groundtruth);
    WHEN("for each query vector, the closest centroids are determined, their lists are searched for the nearest R neighbors")
    {
      WARN("n_lists := " << n_lists);
      WARN("n_probe := " << n_probe);
      BENCHMARK_ADVANCED("search_preassigned(): includes finding the nearest centroids")
      (Catch::Benchmark::Chronometer meter)
      {
        meter.measure([&]
                      {
#pragma omp parallel for
                for (len_t query_id = 0; query_id < n_query_vectors; query_id++)
                {
                  uint8_t *query_bytes = &query_vectors[query_id * (vector_dim + 4) + 4];
                  vector_el_t *query = alloc_query_as_vector_el(query_bytes, vector_dim);
                  std::vector<list_id_t> list_ids_to_search = root_index->find_nearest_centroids(query, vector_dim, n_probe);
                  storage_index->search_preassigned(list_ids_to_search.data(), list_ids_to_search.size(), query, R);
                  free(query);
                } });
      };
    }
  };

  setup_indices_and_run(n_probe, n_lists, n_entries, n_query_vectors, groundtruth_k, vector_dim, false, "SIFT1M", "idx_1M.ivecs", run);
}

SCENARIO("find_nearest_centroids()", "[StorageIndex][find_nearest_centroids][benchmark][SIFT1M]")
{
  len_t vector_dim = 128;
  len_t n_entries = (len_t)1E6;
  len_t n_query_vectors = (len_t)1E4;
  len_t n_lists = GENERATE(256, 512, 1024, 2048, 4096);
  len_t n_probe = GENERATE(1, 2, 4, 8, 16, 32, 64, 128);
  len_t groundtruth_k = 1000;

  auto run = [=](uint8_t *query_vectors, uint32_t *groundtruth, StorageIndex *storage_index, RootIndex *root_index)
  {
    UNUSED(groundtruth);
    UNUSED(storage_index);
    WHEN("for each query vector, the closest centroids are determined")
    {
      WARN("n_lists := " << n_lists);
      WARN("n_probe := " << n_probe);
      BENCHMARK_ADVANCED("find_nearest_centroids(): no search")
      (Catch::Benchmark::Chronometer meter)
      {
        meter.measure([&]
                      {
#pragma omp parallel for
                  for (len_t query_id = 0; query_id < n_query_vectors; query_id++)
                  {
                    uint8_t *query_bytes = &query_vectors[query_id * (vector_dim + 4) + 4];
                    vector_el_t *query = alloc_query_as_vector_el(query_bytes, vector_dim);
                    std::vector<list_id_t> list_ids_to_search = root_index->find_nearest_centroids(query, vector_dim, n_probe);
                    free(query);
                  } });
      };
    }
  };

  setup_indices_and_run(n_probe, n_lists, n_entries, n_query_vectors, groundtruth_k, vector_dim, false, "SIFT1M", "idx_1M.ivecs", run);
}