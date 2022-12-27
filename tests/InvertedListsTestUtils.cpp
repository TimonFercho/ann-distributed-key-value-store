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
    size_t total_size = max((size_t)32, round_up_to_next_power_of_two(used_space));
    return total_size;
  }

  void write_to_file(string filename, void *data, size_t size)
  {
    FILE *file = fopen(filename.c_str(), "w");
    fwrite(data, size, 1, file);
    fclose(file);
  }

  void read_from_file(string filename, void *data, size_t size)
  {
    FILE *file = fopen(filename.c_str(), "r");
    fread(data, size, 1, file);
    fclose(file);
  }

  void *mmap_file(string filename, size_t size)
  {
    FILE *file = fopen(filename.c_str(), "r");
    void *data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fileno(file), 0);
    fclose(file);
    return data;
  }

  bool file_exists(string filename)
  {
    ifstream f(filename.c_str());
    if (!f.is_open())
    {
      return false;
    }
    f.close();
    return true;
  }

  string join(const string &a, const string &b)
  {
    return a + "/" + b;
  }

  InvertedLists get_inverted_lists_object(len_t vector_dim)
  {
    string file = join(TMP_DIR, LISTS_FILENAME);
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
    std::cout << endl;
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
}