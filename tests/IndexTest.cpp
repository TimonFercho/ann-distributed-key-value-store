#include <sys/mman.h>
#include <unordered_set>

#include "../lib/catch.hpp"

#include "../include/tests/InvertedListsTestUtils.hpp"
#include "../include/storage-node/StorageIndex.hpp"
#include "../include/L2Space.hpp"
#include "../include/root-node/RootIndex.hpp"
#include "../include/Query.hpp"

#define UNUSED(x) (void)(x)
#define N_RESULTS_GROUNDTRUTH 1000

#ifdef _OPENMP
#include <omp.h>
#endif

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
  GIVEN("an StorageLists object and a list of 1D vectors, corresponding vector ids and list ids")
  {
    len_t vector_dim = 1;
    StorageLists lists = get_inverted_lists_object(vector_dim);
    len_t list1_length = 2;
    vector_el_t vectors_list1[] = {2, 4};
    vector_id_t ids_list1[] = {1, 2};

    len_t list2_length = 2;
    vector_el_t vectors_list2[] = {1, 3};
    vector_id_t ids_list2[] = {3, 4};

    list_id_t list_ids[] = {1, 2};

    AND_GIVEN("A query vector, the list ids to search and a value of k")
    {
      vector_el_t query_vector[] = {0};
      len_t n_results = 3;
      list_id_t list_ids_to_probe[] = {1, 2};
      len_t n_probe = 2;
      Query query = Query(query_vector, list_ids_to_probe, n_results, n_probe);

      WHEN("the StorageLists object is populated with the vectors, ids and list ids and used to initialize an StorageIndex object")
      {
        lists.insert_entries(list_ids[0], vectors_list1, ids_list1, list1_length);
        lists.insert_entries(list_ids[1], vectors_list2, ids_list2, list2_length);

        StorageIndex index(&lists);

        WHEN("the StorageIndex is queried with a query vector")
        {
          QueryResults results = index.search_preassigned(&query);

          THEN("the results are correct")
          {
            CHECK(results[0].distance == 1 * 1);
            CHECK(results[0].vector_id == 3);
            CHECK(results[1].distance == 2 * 2);
            CHECK(results[1].vector_id == 1);
            CHECK(results[2].distance == 3 * 3);
            CHECK(results[2].vector_id == 4);
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
                                len_t n_results_groundtruth,
                                len_t vector_dim,
                                bool mmap_groundtruth,
                                std::string dataset,
                                std::string groundtruth_filename,
                                std::function<void(uint8_t *, uint32_t *, StorageIndex *, RootIndex *)> run)
{
  if (n_probe <= n_lists)
  {
    std::string dataset_dir = join(SIFT_OUTPUT_DIR, dataset);
    // groundtruth format: sizeof(uint32_t) + [n_results_groundtruth * sizeof(uint32_t)]
    std::string groundtruth_filepath = join(SIFT_GROUNDTRUTH_DIR, groundtruth_filename);
    // vectors format: n_entries * 128 * sizeof(float)
    std::string vectors_filepath = join(dataset_dir, get_vectors_filename());
    // vector ids format: n_entries * sizeof(int64_t)
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

      WHEN("the files are mapped to memory")
      {
        len_t vectors_size = n_entries * vector_dim * sizeof(vector_el_t);
        len_t ids_size = n_entries * sizeof(list_id_t);
        len_t list_ids_size = n_entries * sizeof(list_id_t);
        len_t centroids_size = n_lists * vector_dim * sizeof(vector_el_t);
        len_t query_vectors_size = n_query_vectors * vector_dim * sizeof(vector_el_t);
        len_t groundtruth_size = n_query_vectors * n_results_groundtruth * sizeof(vector_id_t);

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

        WHEN("the StorageLists object is populated with the vectors, ids and list ids and used to initialize an StorageIndex object")
        {

          StorageLists lists = get_inverted_lists_object(vector_dim);
          lists.bulk_insert_entries(vectors_filepath, vectors_ids_filepath, list_ids_filepath, n_entries);
          StorageIndex storage_index(&lists);
          RootIndex root_index(vector_dim, centroids, n_lists);

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
  }
};

auto prepare_queries = [](uint8_t *query_vectors, len_t n_query_vectors, len_t vector_dim, len_t n_results, len_t n_probe)
{
  QueryBatch queries;
  for (len_t query_id = 0; query_id < n_query_vectors; query_id++)
  {
    uint8_t *query_bytes = &query_vectors[query_id * (vector_dim + 4) + 4];
    vector_el_t *query_vector = alloc_query_as_vector_el(query_bytes, vector_dim);
    Query *query = new Query(query_vector, n_results, n_probe);
    queries.push_back(query);
  }
  return queries;
};

auto free_queries = [](QueryBatch queries)
{
  for (len_t query_id = 0; query_id < queries.size(); query_id++)
  {
    free(queries[query_id]->get_query_vector());
    delete queries[query_id];
  }
};

auto get_recall_at_r = [](const QueryResultsBatch &result_batch, const uint32_t *groundtruth, len_t n_query_vectors, len_t n_results_groundtruth, len_t n_results, len_t r)
{
  REQUIRE(r <= n_results);
  len_t n_correct = 0;
  for (len_t query_id = 0; query_id < n_query_vectors; query_id++)
  {
    const QueryResults results = result_batch[query_id];
    vector_id_t groundtruth_first_nearest_neighbor = groundtruth[query_id * (n_results_groundtruth + 1) + 1];
    for (len_t result_id = 0; result_id < r; result_id++)
    {
      const vector_id_t id = results[result_id].vector_id;
      if (id == groundtruth_first_nearest_neighbor)
      {
        n_correct++;
        break;
      }
    }
  }
  return (float)n_correct / n_query_vectors;
};

SCENARIO("search_preassigned(): test recall with SIFT1M", "[StorageIndex][search_preassigned][test][recall][SIFT1M]")
{
  GIVEN("the SIFT1M dataset")
  {
    len_t vector_dim = 128;
    len_t n_entries = (len_t)1E6;
    len_t n_query_vectors = (len_t)1E4;
    len_t n_results_groundtruth = N_RESULTS_GROUNDTRUTH;
    len_t n_lists = 1024;
    len_t n_probe = GENERATE(1, 2, 4, 8, 16, 32);
    len_t n_results = GENERATE(1, 2, 4, 8, 16, 32);

    auto run = [=](uint8_t *query_vectors, uint32_t *groundtruth, StorageIndex *storage_index, RootIndex *root_index)
    {
      WHEN("for each query vector, the closest centroids are determined, their lists are searched for the nearest n_results neighbors")
      {
#ifdef _OPENMP
        WARN("max_n_threads := " << omp_get_max_threads());
#endif
        WARN("n_lists := " << n_lists);
        WARN("n_probe := " << n_probe);
        WARN("n_results := " << n_results);

        QueryBatch queries = prepare_queries(query_vectors, n_query_vectors, vector_dim, n_results, n_probe);

        root_index->batch_preassign_queries(queries);
        QueryResultsBatch results = storage_index->batch_search_preassigned(queries);

        float recall_at_1 = get_recall_at_r(results, groundtruth, n_query_vectors, n_results_groundtruth, n_results, 1);

        WARN("Recall@1 := " << recall_at_1);
        THEN("the Recall@1 is 100% if we search all lists")
        {
          if (n_probe == n_lists)
          {
            REQUIRE(recall_at_1 == 1.0);
          }
        }

        free_queries(queries);
      }
    };

    setup_indices_and_run(n_probe, n_lists, n_entries, n_query_vectors, n_results_groundtruth, vector_dim, true, "SIFT1M", "idx_1M.ivecs", run);
  }
}

SCENARIO("search_preassigned(): benchmark querying with SIFT1M", "[StorageIndex][search_preassigned][benchmark][SIFT1M]")
{
  len_t vector_dim = 128;
  len_t n_entries = (len_t)1E6;
  len_t n_query_vectors = (len_t)1E4;
  len_t n_results_groundtruth = N_RESULTS_GROUNDTRUTH;
  len_t n_lists = 1024;
  len_t n_probe = GENERATE(1, 2, 4, 8, 16, 32);
  len_t n_results = 1;

  auto run = [=](uint8_t *query_vectors, uint32_t *groundtruth, StorageIndex *storage_index, RootIndex *root_index)
  {
    UNUSED(groundtruth);
    WHEN("for each query vector, the closest centroids are determined, their lists are searched for the nearest n_results neighbors")
    {
#ifdef _OPENMP
      WARN("max_n_threads := " << omp_get_max_threads());
#endif
      WARN("n_lists := " << n_lists);
      WARN("n_probe := " << n_probe);
      WARN("n_results := " << n_results);
      BENCHMARK_ADVANCED("search_preassigned(): measurement excludes preassigning queries")
      (Catch::Benchmark::Chronometer meter)
      {
        QueryBatch queries = prepare_queries(query_vectors, n_query_vectors, vector_dim, n_results, n_probe);
        root_index->batch_preassign_queries(queries);
        meter.measure([&storage_index, &queries]
                      { storage_index->batch_search_preassigned(queries); });
        free_queries(queries);
      };
    }
  };

  setup_indices_and_run(n_probe, n_lists, n_entries, n_query_vectors, n_results_groundtruth, vector_dim, false, "SIFT1M", "idx_1M.ivecs", run);
}

SCENARIO("search_preassigned()", "[StorageIndex][search_preassigned][benchmark][SIFT100M]")
{
  len_t vector_dim = 128;
  len_t n_entries = (len_t)1E8;
  len_t n_query_vectors = (len_t)1E5;
  len_t n_results_groundtruth = N_RESULTS_GROUNDTRUTH;
  len_t n_lists = 1024;
  len_t n_probe = GENERATE(1, 2, 4, 8, 16, 32);
  len_t n_results = GENERATE(1, 2, 4, 8, 16, 32);

  auto run = [=](uint8_t *query_vectors, uint32_t *groundtruth, StorageIndex *storage_index, RootIndex *root_index)
  {
    UNUSED(groundtruth);
    WHEN("for each query vector, the closest centroids are determined, their lists are searched for the nearest n_results neighbors")
    {
#ifdef _OPENMP
      WARN("max_n_threads := " << omp_get_max_threads());
#endif
      WARN("n_lists := " << n_lists);
      WARN("n_probe := " << n_probe);
      WARN("n_results := " << n_results);
      BENCHMARK_ADVANCED("search_preassigned(): measurement excludes preassigning queries")
      (Catch::Benchmark::Chronometer meter)
      {
        QueryBatch queries = prepare_queries(query_vectors, n_query_vectors, vector_dim, n_results, n_probe);
        root_index->batch_preassign_queries(queries);
        meter.measure([&storage_index, &queries]
                      { storage_index->batch_search_preassigned(queries); });
        free_queries(queries);
      };
    }
  };

  setup_indices_and_run(n_probe, n_lists, n_entries, n_query_vectors, n_results_groundtruth, vector_dim, false, "SIFT100M", "idx_100M.ivecs", run);
}

SCENARIO("preassign_query()", "[StorageIndex][preassign_query][benchmark][SIFT1M]")
{
  len_t vector_dim = 128;
  len_t n_entries = (len_t)1E6;
  len_t n_query_vectors = (len_t)1E4;
  len_t n_lists = GENERATE(256, 512, 1024, 2048, 4096);
  len_t n_probe = GENERATE(1, 2, 4, 8, 16, 32, 64, 128);
  len_t n_results_groundtruth = N_RESULTS_GROUNDTRUTH;
  len_t n_results = 1;

  auto run = [=](uint8_t *query_vectors, uint32_t *groundtruth, StorageIndex *storage_index, RootIndex *root_index)
  {
    UNUSED(groundtruth);
    UNUSED(storage_index);
    WHEN("for each query vector, the closest centroids are determined")
    {
#ifdef _OPENMP
      WARN("max_n_threads := " << omp_get_max_threads());
#endif
      WARN("n_lists := " << n_lists);
      WARN("n_probe := " << n_probe);
      WARN("n_results := " << n_results);

      BENCHMARK_ADVANCED("preassign_query(): does not measure search")
      (Catch::Benchmark::Chronometer meter)
      {
        QueryBatch queries = prepare_queries(query_vectors, n_query_vectors, vector_dim, n_results, n_probe);
        meter.measure([&root_index, &queries]
                      { root_index->batch_preassign_queries(queries); });
        free_queries(queries);
      };
    }
  };
  setup_indices_and_run(n_probe, n_lists, n_entries, n_query_vectors, n_results_groundtruth, vector_dim, false, "SIFT1M", "idx_1M.ivecs", run);
}