#include "../types.h";

#include <unordered_map>;
#include <cstddef>;
#include <sys/mman.h>
#include <string>
#include <string.h>
#include <stdio.h>
// #include <cstdlib>

#define PPROT_READ 0x1
#define FLAG_READ "r"

using namespace std;

namespace AnnKvs
{
  class InvertedLists
  {

    struct InvertedList
    {
      size_t id;
      size_t size;
      vector_t *data;
    };

    typedef std::unordered_map<list_id_t, InvertedList> hash_map_t;

  private:
    size_t vector_size;
    hash_map_t id_to_list_map;
    string base_path;

    string get_filename(InvertedList *list) const
    {
      return base_path + "/" + to_string(list->id);
    }

    void mmap_list(InvertedList *list)
    {
      size_t total_size = get_total_size(list);
      const char *filename = get_filename(list).c_str();
      FILE *f = fopen(filename, FLAG_READ);
      uint8_t *data =
          (uint8_t *)mmap(nullptr, total_size, PROT_READ, MAP_SHARED, fileno(f), 0);
      if (data == MAP_FAILED)
      {
        perror("mmap");
        exit(1);
      }
      list->data = data;
    }

    vector_t *get_vectors(InvertedList *list) const
    {
      return list->data;
    }

    vector_id_t *get_ids(InvertedList *list) const
    {
      return (vector_id_t *)list->data + list->size * vector_size;
    }

    size_t get_total_size(InvertedList *list) const
    {
      return list->size * (vector_size + sizeof(vector_id_t));
    }

  public:
    InvertedLists(size_t vector_size, string base_path) : vector_size(vector_size), base_path(base_path)
    {
    }

    size_t get_list_count() const
    {
      return id_to_list_map.size();
    }

    size_t get_vector_size() const
    {
      return vector_size;
    }

    vector_t *get_vectors(list_id_t list_no)
    {
      InvertedList *list = &id_to_list_map[list_no];
      return get_vectors(list);
    }

    vector_id_t *get_ids(list_id_t list_no)
    {
      InvertedList *list = &id_to_list_map[list_no];
      return get_ids(list);
    }

    size_t get_size(list_id_t list_no)
    {
      InvertedList *list = &id_to_list_map[list_no];
      return list->size;
    }

    void insert_list(
        list_id_t list_id,
        vector_t *vectors,
        vector_id_t *ids,
        size_t size)
    {
      InvertedList list;
      list.id = list_id;
      list.size = size;
      mmap_list(&list);
      memcpy(get_vectors(&list), vectors, size * vector_size);
      memcpy(get_ids(&list), ids, size * sizeof(vector_id_t));
      id_to_list_map[list_id] = list;
    }

    void delete_list(list_id_t list_id)
    {
      InvertedList list = id_to_list_map[list_id];
      size_t total_size = get_total_size(&list);
      int err = munmap(list.data, total_size);
      if (err)
      {
        perror("munmap");
        exit(1);
      }
      id_to_list_map.erase(list_id);
    }

    InvertedList *get_list(list_id_t id) const
    {
      hash_map_t::const_iterator it = id_to_list_map.find(id);
      if (it == id_to_list_map.end())
      {
        return nullptr;
      }
      return (InvertedList *)&it->second;
    }
  };
};