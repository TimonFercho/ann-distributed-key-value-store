#ifndef INVERTED_LISTS_TEST_UTILS_HPP
#define INVERTED_LISTS_TEST_UTILS_HPP

#include <iostream>
#include <fstream>

#include "../../lib/catch.hpp"

#include "TestConfig.hpp"
#include "../../include/storage-node/InvertedLists.hpp"

using namespace ann_dkvs;

#define gen_random_ints(T, MIN_VAL, MAX_VAL, N_CHUNKS, CHUNK_LEN, EXCLUDE_SET) (GENERATE(take(N_CHUNKS, chunk(CHUNK_LEN, Catch::Generators::map([](T val) { return (T)val; }, filter([&](T val) {std::vector<T> exclude{EXCLUDE_SET};return find(exclude.begin(), exclude.end(), val) == exclude.end(); }, random(MIN_VAL, MAX_VAL)))))))

#define gen_random_floats(T, MIN_VAL, MAX_VAL, N_CHUNKS, CHUNK_LEN, EXCLUDE_SET) (GENERATE(take(N_CHUNKS, chunk(CHUNK_LEN, Catch::Generators::map([](T val) { return (T)val; }, filter([&](T val) {std::vector<T> exclude{EXCLUDE_SET};return find(exclude.begin(), exclude.end(), val) == exclude.end(); }, random(MIN_VAL, MAX_VAL)))))))

#define gen_ranged_values(T, MIN_VAL, MAX_VAL, N_CHUNKS, CHUNK_LEN, EXCLUDE_SET) (GENERATE(take(N_CHUNKS, chunk(CHUNK_LEN, Catch::Generators::map([](T val) { return (T)val; }, filter([&](T val) {std::vector<T> exclude{EXCLUDE_SET};return find(exclude.begin(), exclude.end(), val) == exclude.end(); }, range(MIN_VAL, MAX_VAL)))))))

#define gen_random_int(T, MIN_VAL, MAX_VAL, N_VALS, EXCLUDE_SET) (gen_random_ints(T, MIN_VAL, MAX_VAL, N_VALS, 1, EXCLUDE_SET)[0])

#define to_ptr(T, V) ((T *)V.data())

#define gen_vector_dim(EXCLUDE_SET) (gen_random_int(len_t, 0, MAX_VECTOR_DIM, N_VECTOR_DIM_SAMPLES, EXCLUDE_SET))

#define gen_list_ids(CHUNK_LEN) gen_ranged_values(list_id_t, 0, MAX_LIST_ID, N_LIST_ID_SAMPLES, CHUNK_LEN, {})

#define gen_list_id() gen_list_ids(1)[0]

#define gen_list_lengths(CHUNK_LEN, EXCLUDE_SET) gen_random_ints(len_t, MIN_LIST_LENGTH, MAX_LIST_LENGTH, N_LIST_LENGTH_SAMPLES, CHUNK_LEN, EXCLUDE_SET)

#define gen_list_lengths_random_length() gen_list_lengths(random_range(1, MAX_N_LISTS), {})

#define gen_list_length(EXCLUDE_SET) gen_list_lengths(1, EXCLUDE_SET)[0]

#define gen_vector_ids_fixed(CHUNK_LEN) gen_ranged_values(vector_id_t, 0, MAX_VECTOR_ID, N_VECTOR_SAMPLES, CHUNK_LEN, {})

#define gen_random_vector_values_fixed(CHUNK_LEN) gen_random_floats(vector_el_t, MIN_VECTOR_VAL, MAX_VECTOR_VAL, N_VECTOR_SAMPLES, CHUNK_LEN, {})

#define gen_vectors_fixed(CHUNK_LEN, DIM) make_pair(gen_random_vector_values_fixed((CHUNK_LEN) * (DIM)), gen_vector_ids_fixed(CHUNK_LEN))

#define random_range(MIN, MAX) (rand() % (MAX - MIN + 1) + MIN)

#define gen_vectors(dim) gen_vectors_fixed(random_range(MIN_LIST_LENGTH, MAX_LIST_LENGTH), dim)

#define get_clustered_vectors_length(data, list_ids, dim) (std::min(get_vector_length(data, dim), list_ids.size()))

#define get_vector_length(data, dim) (std::min(data.first.size() / dim, data.second.size()))

#define UNUSED(x) (void)(x)

namespace ann_dkvs
{
  size_t round_up_to_next_power_of_two(size_t value);
  size_t is_power_of_two(size_t value);
  size_t get_vector_size(len_t vector_dim);
  size_t get_list_size(len_t vector_dim, len_t n_entries);
  size_t get_total_size(size_t used_space);
  void write_to_file(std::string filename, void *data, size_t size);
  void read_from_file(std::string filename, void *data, size_t size);
  void *mmap_file(std::string filename, size_t size);
  bool file_exists(std::string filename);
  std::string join(const std::string &a, const std::string &b);
  InvertedLists get_inverted_lists_object(len_t vector_dim);
  void print_vector(vector_el_t *vector, len_t vector_dim, len_t n_entries);
  void are_vectors_equal(vector_el_t *actual, vector_el_t *expected, len_t vector_dim, len_t n_entries);
  void are_ids_equal(vector_id_t *actual, vector_id_t *expected, len_t n_entries);
  std::string get_lists_filename();
  std::string get_vectors_filename();
  std::string get_vector_ids_filename();
  std::string get_list_ids_filename(len_t n_lists);
  void setup_run_teardown_bulk_insert_entries_dataset(
      len_t n_entries,
      len_t vector_dim,
      std::string vectors_filepath,
      std::string vector_ids_filepath,
      std::string list_ids_filepath,
      std::function<void(len_t, size_t, vector_el_t *, vector_id_t *, list_id_t *, std::string, std::string, std::string)> run_bulk_insert_entries);
}

#endif // INVERTED_LISTS_TEST_UTILS_HPP