#include <sys/mman.h>
#include <string.h>
#include <string>
#include <iostream>
#include <unistd.h>

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

  bool InvertedLists::has_free_slot_at_end()
  {
    if (free_slots.size() == 0)
      return false;
    Slot last_slot = free_slots.back();
    return last_slot.offset + last_slot.capacity == total_size;
  }

  void InvertedLists::ensure_file_created_and_region_unmapped()
  {
    if (total_size == 0)
    {
      FILE *f = fopen(filename.c_str(), "w");
      if (f == nullptr)
      {
        throw "could not create file";
      }
      fclose(f);
    }
    else
    {
      munmap(base_ptr, total_size);
    }
  }

  void InvertedLists::truncate_file(size_t size)
  {
    FILE *f = fopen(filename.c_str(), "w");
    if (f == nullptr)
    {
      throw "could not create file";
    }
    if (ftruncate(fileno(f), size) == -1)
    {
      throw "could not truncate file";
    }
    fclose(f);
  }

  void InvertedLists::resize_region(size_t new_size)
  {
    if (new_size < total_size)
    {
      throw "cannot shrink region";
    }
    if (new_size == total_size)
    {
      return;
    }
    ensure_file_created_and_region_unmapped();
    size_t size_to_grow = new_size - total_size;
    if (has_free_slot_at_end())
    {
      Slot last_slot = free_slots.back();
      last_slot.capacity += size_to_grow;
    }
    else
    {
      Slot new_slot;
      new_slot.offset = total_size;
      new_slot.capacity = size_to_grow;
      free_slots.push_back(new_slot);
    }
    total_size = new_size;
    truncate_file(total_size);
    mmap_region();
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