#include <sys/mman.h>
#include <string.h>
#include <string>

#include "InvertedLists.h"

using namespace std;

namespace ann_dkvs
{
  string InvertedLists::get_filename(InvertedList *list) const
  {
    return base_path + "/" + to_string(list->id);
  }

  void InvertedLists::mmap_list(InvertedList *list)
  {
    size_t total_size = get_total_size(list);
    const char *filename = get_filename(list).c_str();
    FILE *f = fopen(filename, FLAG_READ);
    vector_el_t *data =
        (vector_el_t *)mmap(nullptr, total_size, PROT_READ, MAP_SHARED, fileno(f), 0);
    if (data == MAP_FAILED)
    {
      perror("mmap");
      exit(1);
    }
    list->data = data;
  }

  vector_el_t *InvertedLists::get_vectors_by_list(InvertedList *list) const
  {
    return list->data;
  }

  vector_id_t *InvertedLists::get_ids_by_list(InvertedList *list) const
  {
    return (vector_id_t *)list->data + list->size * vector_size;
  }

  size_t InvertedLists::get_total_size(InvertedList *list) const
  {
    return list->size * (vector_size + sizeof(vector_id_t));
  }

  InvertedLists::InvertedLists(size_t vector_size, string base_path) : vector_size(vector_size), base_path(base_path)
  {
  }

  size_t InvertedLists::get_list_count() const
  {
    return id_to_list_map.size();
  }

  size_t InvertedLists::get_vector_size() const
  {
    return vector_size;
  }

  vector_el_t *InvertedLists::get_vectors(list_id_t list_no)
  {
    InvertedList *list = &id_to_list_map[list_no];
    return get_vectors_by_list(list);
  }

  vector_id_t *InvertedLists::get_ids(list_id_t list_no)
  {
    InvertedList *list = &id_to_list_map[list_no];
    return get_ids_by_list(list);
  }

  size_t InvertedLists::get_size(list_id_t list_no)
  {
    InvertedList *list = &id_to_list_map[list_no];
    return list->size;
  }

  void InvertedLists::insert_list(
      list_id_t list_id,
      vector_el_t *vectors,
      vector_id_t *ids,
      size_t size)
  {
    InvertedList list;
    list.id = list_id;
    list.size = size;
    mmap_list(&list);
    memcpy(get_vectors_by_list(&list), vectors, size * vector_size);
    memcpy(get_ids_by_list(&list), ids, size * sizeof(vector_id_t));
    id_to_list_map[list_id] = list;
  }

  void InvertedLists::delete_list(list_id_t list_id)
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

  InvertedList *InvertedLists::get_list(list_id_t id) const
  {
    hash_map_t::const_iterator it = id_to_list_map.find(id);
    if (it == id_to_list_map.end())
    {
      return nullptr;
    }
    return (InvertedList *)&it->second;
  }
}