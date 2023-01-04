#include "../include/storage-node/InvertedLists.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
#include <chrono>
#include <functional>
#include <numeric>
#include <cmath>

#define TMP_DIR "benchmarks/tmp"
#define SIFT1M_OUTPUT_DIR "cluster/out/sift1M"
#define LISTS_FILE_NAME "lists.bin"
#define VECTORS_FILE_NAME "vectors.bin"
#define VECTOR_IDS_FILE_NAME "vector_ids.bin"
#define LIST_IDS_FILE_NAME "list_ids.bin"

using namespace ann_dkvs;

auto read_from_file = [](string filename, void *data, size_t size)
{
  FILE *file = fopen(filename.c_str(), "r");
  fread(data, size, 1, file);
  fclose(file);
};

auto file_exists = [](string filename)
{
  ifstream f(filename.c_str());
  if (!f.is_open())
  {
    return false;
  }
  f.close();
  return true;
};

auto join = [](const string &a, const string &b)
{
  return a + "/" + b;
};

struct BulkInsertEntriesContext
{
  InvertedLists *lists;
  vector_el_t *vectors;
  vector_id_t *ids;
  list_id_t *list_ids;
  size_t n_entries;
  size_t vector_dim;
  string vectors_filename;
  string ids_filename;
  string list_ids_filename;
  string lists_filename;
};

void setup_benchmark(BulkInsertEntriesContext *c)
{
  assert(file_exists(c->vectors_filename));
  assert(file_exists(c->ids_filename));
  assert(file_exists(c->list_ids_filename));

  size_t vectors_size = c->n_entries * c->vector_dim * sizeof(vector_el_t);
  size_t ids_size = c->n_entries * sizeof(list_id_t);
  size_t list_ids_size = c->n_entries * sizeof(list_id_t);

  vector_el_t *vectors = (vector_el_t *)malloc(vectors_size);
  vector_id_t *ids = (vector_id_t *)malloc(ids_size);
  list_id_t *list_ids = (list_id_t *)malloc(list_ids_size);

  read_from_file(c->vectors_filename, vectors, vectors_size);
  read_from_file(c->ids_filename, ids, ids_size);
  read_from_file(c->list_ids_filename, list_ids, list_ids_size);

  c->lists = new InvertedLists(c->vector_dim, c->lists_filename);
}

std::chrono::duration<double> run_benchmark(std::function<void()> const &func)
{
  std::chrono::time_point<std::chrono::system_clock> start, end;
  start = std::chrono::system_clock::now();

  func();

  end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = end - start;
  return elapsed_seconds;
}

void teardown_benchmark(BulkInsertEntriesContext *c)
{
  free(c->vectors);
  free(c->ids);
  free(c->list_ids);
  delete c->lists;
  remove(c->lists_filename.c_str());
}

void report_results(vector<double> samples, double total_elapsed_seconds)
{
  std::cout << "------------------" << std::endl;
  std::cout << "Benchmark results:" << std::endl;
  std::cout << "------------------" << std::endl;
  double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
  double mean = sum / samples.size();
  double min = *std::min_element(samples.begin(), samples.end());
  double max = *std::max_element(samples.begin(), samples.end());
  double std = 0.0;
  for (size_t i = 0; i < samples.size(); i++)
  {
    std += pow((samples[i] - mean), 2);
  }
  std = sqrt(std / samples.size());

  std::cout << "Samples: " << samples.size() << std::endl;
  std::cout << "Elapsed minutes: " << total_elapsed_seconds / 60 << std::endl;
  std::cout << "Min: " << min << std::endl;
  std::cout << "Mean: " << mean << std::endl;
  std::cout << "Max: " << max << std::endl;
  std::cout << "Std: " << std << std::endl;
}

int main()
{
  BulkInsertEntriesContext *c = (BulkInsertEntriesContext *)malloc(sizeof(BulkInsertEntriesContext));

  c->vector_dim = 128;
  c->vectors_filename = join(SIFT1M_OUTPUT_DIR, VECTORS_FILE_NAME);
  c->ids_filename = join(SIFT1M_OUTPUT_DIR, VECTOR_IDS_FILE_NAME);
  c->list_ids_filename = join(SIFT1M_OUTPUT_DIR, LIST_IDS_FILE_NAME);
  c->lists_filename = join(TMP_DIR, LISTS_FILE_NAME);
  c->n_entries = (len_t)1E6;

  auto func = [&]()
  {
    c->lists->bulk_insert_entries(
        c->vectors_filename,
        c->ids_filename,
        c->list_ids_filename,
        c->n_entries);
  };

  size_t n_samples = 10;

  std::cout << "Benchmarking bulk_insert_entries() using SIFT1M dataset" << std::endl;

  vector<double>
      samples(n_samples);

  std::chrono::time_point<std::chrono::system_clock> start, end;
  start = std::chrono::system_clock::now();

  for (size_t i = 0; i < n_samples; i++)
  {
    setup_benchmark(c);
    std::chrono::duration elapsed_seconds = run_benchmark(func);
    samples[i] = elapsed_seconds.count();
    teardown_benchmark(c);
  }

  end = std::chrono::system_clock::now();
  std::chrono::duration<double> total_elapsed = end - start;

  report_results(samples, total_elapsed.count());
}
