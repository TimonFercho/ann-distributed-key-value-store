#include "../include/tests/InvertedListsTestUtils.hpp"
#include <sys/mman.h>

namespace ann_dkvs
{

  size_t round_up_to_next_power_of_two(size_t value)
  {
    size_t power = 1;
    while (power < value)
    {
      power *= 2;
    }
    return power;
  }

  size_t is_power_of_two(size_t value)
  {
    return (value & (value - 1)) == 0;
  }

  size_t get_vector_size(len_t vector_dim)
  {
    return vector_dim * sizeof(vector_el_t);
  }

  size_t get_list_size(len_t vector_dim, len_t n_entries)
  {
    len_t entries_allocated = round_up_to_next_power_of_two(n_entries);
    size_t list_size = entries_allocated * (get_vector_size(vector_dim) + sizeof(vector_id_t));
    return list_size;
  }

  size_t get_total_size(size_t used_space)
  {
    size_t total_size = std::max((size_t)32, round_up_to_next_power_of_two(used_space));
    return total_size;
  }

  void write_to_file(std::string filename, void *data, size_t size)
  {
    FILE *file = fopen(filename.c_str(), "w");
    fwrite(data, size, 1, file);
    fclose(file);
  }

  void read_from_file(std::string filename, void *data, size_t size)
  {
    FILE *file = fopen(filename.c_str(), "r");
    fread(data, size, 1, file);
    fclose(file);
  }

  void *mmap_file(std::string filename, size_t size)
  {
    FILE *file = fopen(filename.c_str(), "r");
    void *data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fileno(file), 0);
    fclose(file);
    return data;
  }

  bool file_exists(std::string filename)
  {
    std::ifstream f(filename.c_str());
    if (!f.is_open())
    {
      return false;
    }
    f.close();
    return true;
  }

  std::string join(const std::string &a, const std::string &b)
  {
    return a + "/" + b;
  }

  InvertedLists get_inverted_lists_object(len_t vector_dim)
  {
    std::string file = join(TMP_DIR, get_lists_filename());
    remove(file.c_str());
    InvertedLists lists(vector_dim, file);
    return lists;
  }

  void print_vector(vector_el_t *vector, len_t vector_dim, len_t n_entries)
  {
    for (len_t i = 0; i < n_entries; i++)
    {
      std::cout << " | ";
      for (len_t j = 0; j < vector_dim; j++)
      {
        std::cout << vector[i * vector_dim + j] << " ";
      }
    }
    std::cout << std::endl;
  }

  void are_vectors_equal(vector_el_t *actual, vector_el_t *expected, len_t vector_dim, len_t n_entries)
  {
    for (len_t i = 0; i < n_entries * vector_dim; i++)
    {
      REQUIRE(actual[i] == expected[i]);
    }
  }

  void are_ids_equal(vector_id_t *actual, vector_id_t *expected, len_t n_entries)
  {
    for (len_t i = 0; i < n_entries; i++)
    {
      REQUIRE(actual[i] == expected[i]);
    }
  }

  std::string get_lists_filename()
  {
    std::string filename = LISTS_FILENAME;
    std::string file_ext = FILE_EXT;
    return filename + file_ext;
  }

  std::string get_vectors_filename()
  {
    std::string filename = VECTORS_FILENAME;
    std::string file_ext = FILE_EXT;
    return filename + file_ext;
  }

  std::string get_vector_ids_filename()
  {
    std::string filename = VECTOR_IDS_FILENAME;
    std::string file_ext = FILE_EXT;
    return filename + file_ext;
  }

  std::string get_list_ids_filename(len_t n_lists)
  {
    std::string filename = LIST_IDS_FILENAME;
    std::string separator = "_";
    std::string file_ext = FILE_EXT;
    return filename + separator + std::to_string(n_lists) + file_ext;
  }

  void setup_run_teardown_bulk_insert_entries_dataset(
      len_t n_entries,
      len_t vector_dim,
      std::string vectors_filepath,
      std::string vector_ids_filepath,
      std::string list_ids_filepath,
      std::function<void(len_t, size_t, vector_el_t *, vector_id_t *, list_id_t *, std::string, std::string, std::string)> run_bulk_insert_entries)
  {
    THEN("the vector, vector id and list id files are already present")
    {
      REQUIRE(file_exists(vectors_filepath));
      REQUIRE(file_exists(vector_ids_filepath));
      REQUIRE(file_exists(list_ids_filepath));

      AND_GIVEN("the files are mapped to memory")
      {
        size_t vectors_size = n_entries * vector_dim * sizeof(vector_el_t);
        size_t ids_size = n_entries * sizeof(list_id_t);
        size_t list_ids_size = n_entries * sizeof(list_id_t);

        vector_el_t *vectors = (vector_el_t *)mmap_file(vectors_filepath, vectors_size);
        vector_id_t *ids = (vector_id_t *)mmap_file(vector_ids_filepath, ids_size);
        list_id_t *list_ids = (list_id_t *)mmap_file(list_ids_filepath, list_ids_size);

        run_bulk_insert_entries(n_entries, vector_dim, vectors, ids, list_ids, vectors_filepath, vector_ids_filepath, list_ids_filepath);

        munmap(vectors, vectors_size);
        munmap(ids, ids_size);
        munmap(list_ids, list_ids_size);
      }
    }
  }
}