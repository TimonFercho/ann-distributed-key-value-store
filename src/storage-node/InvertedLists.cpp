#include <sys/mman.h>
#include <string.h>
#include <string>
#include <iostream>

#include "InvertedLists.h"

using namespace std;

namespace ann_dkvs
{
  void InvertedLists::mmap_region()
  {
    throw "mmap_region() not implemented";
  }

  vector_el_t *InvertedLists::get_vectors_by_list(InvertedList *list) const
  {
    return (vector_el_t *)base_ptr + list->offset;
  }

  vector_id_t *InvertedLists::get_ids_by_list(InvertedList *list) const
  {
    vector_el_t *vector_ptr = get_vectors_by_list(list);
    vector_id_t *id_ptr = (vector_id_t *)(vector_ptr + vector_size * list->capacity);
    return id_ptr;
  }

  size_t InvertedLists::get_total_list_size(InvertedList *list) const
  {
    return list->size * (vector_size + sizeof(vector_id_t));
  }

  InvertedLists::InvertedLists(size_t vector_dim, string filename) : vector_dim(vector_dim), filename(filename), vector_size(vector_dim * sizeof(vector_el_t)), total_size(0)
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

  size_t InvertedLists::get_vector_dim() const
  {
    return vector_dim;
  }

  string InvertedLists::get_filename() const
  {
    return filename;
  }

  InvertedLists::Slot InvertedLists::alloc_slot(size_t size)
  {
    throw "alloc_slot() not implemented";
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

  size_t InvertedLists::round_up_to_next_power_of_two(size_t n)
  {
    size_t power = 1;
    while (power < n)
    {
      power *= 2;
    }
    return power;
  }

  void InvertedLists::insert_list(
      list_id_t list_id,
      size_t size)
  {
    if (id_to_list_map.find(list_id) != id_to_list_map.end())
    {
      throw "List already exists";
    }
    Slot slot = alloc_slot(size);
    InvertedList list;
    list.offset = slot.offset;
    list.capacity = slot.capacity;
    list.size = size;
    id_to_list_map[list_id] = list;
  }

  void InvertedLists::delete_list(list_id_t list_id)
  {
  }
}