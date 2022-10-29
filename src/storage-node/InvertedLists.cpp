#include <sys/mman.h>
#include <string.h>
#include <string>
#include <iostream>

#include "InvertedLists.h"

using namespace std;

namespace ann_dkvs
{
  void InvertedLists::mmap_list(InvertedList *list)
  {
    size_t total_size = get_total_size(list);
    string filename = list->filename;
    FILE *f = fopen(filename.c_str(), FLAG_READ);
    vector_el_t *data =
        (vector_el_t *)mmap(
            nullptr,
            total_size,
            PROT_READ,
            MAP_SHARED,
            fileno(f),
            0);
    if (data == MAP_FAILED)
    {
      cout << "mapping file " << filename << " failed!" << endl;
      perror("mmap");
      exit(1);
    }
    cout << "mapped file " << filename << " of size " << total_size << endl;
    list->data = data;
  }

  vector_el_t *InvertedLists::get_vectors_by_list(InvertedList *list) const
  {
    return list->data;
  }

  vector_id_t *InvertedLists::get_ids_by_list(InvertedList *list) const
  {
    cout << "list data " << list->data << endl;
    cout << "list size " << list->size << endl;
    cout << "vector size " << vector_size << endl;
    vector_el_t *res = list->data + list->size * vector_size;
    cout << "res" << res << endl;
    return (vector_id_t *)res;
  }

  size_t InvertedLists::get_total_size(InvertedList *list) const
  {
    return list->size * (vector_size + sizeof(vector_id_t));
  }

  InvertedLists::InvertedLists(size_t vector_size) : vector_size(vector_size)
  {
  }

  size_t InvertedLists::get_size() const
  {
    return id_to_list_map.size();
  }

  size_t InvertedLists::get_vector_size() const
  {
    return vector_size;
  }

  vector_el_t *InvertedLists::get_vectors(list_id_t list_id)
  {
    InvertedList *list = &id_to_list_map[list_id];
    return get_vectors_by_list(list);
  }

  vector_id_t *InvertedLists::get_ids(list_id_t list_id)
  {
    InvertedList *list = &id_to_list_map[list_id];
    return get_ids_by_list(list);
  }

  size_t InvertedLists::get_list_size(list_id_t list_id)
  {
    InvertedList *list = &id_to_list_map[list_id];
    return list->size;
  }

  string InvertedLists::get_filename(list_id_t list_id)
  {
    InvertedList *list = &id_to_list_map[list_id];
    return list->filename;
  }

  void InvertedLists::insert_list(
      list_id_t list_id,
      string filename,
      size_t size)
  {
    InvertedList list;
    list.id = list_id;
    list.filename = filename;
    list.size = size;
    mmap_list(&list);
    id_to_list_map[list_id] = list;
    cout << "inserted list " << list_id << endl;
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

  void InvertedLists::write_list(string filename, vector_el_t *vectors, vector_id_t *ids, size_t size)
  {
    FILE *f = fopen(filename.c_str(), "w");
    fwrite(vectors, vector_size, size, f);
    fwrite(ids, sizeof(list_id_t), size, f);
    fclose(f);
  }
}