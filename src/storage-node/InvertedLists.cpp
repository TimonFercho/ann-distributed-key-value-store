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
    FILE *f = fopen(filename.c_str(), "r+");
    if (f == nullptr)
    {
      throw "could not open file";
    }
    base_ptr = (uint8_t *)mmap(nullptr,
                               total_size,
                               PROT_READ | PROT_WRITE,
                               MAP_SHARED,
                               fileno(f),
                               0);
    if (base_ptr == MAP_FAILED)
    {
      throw "could not mmap file";
    }
    fclose(f);
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

  void InvertedLists::resize_file(size_t size)
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
    resize_file(total_size);
    mmap_region();
  }

  bool InvertedLists::does_list_need_reallocation(InvertedList *list, size_t new_size)
  {
    return new_size <= list->capacity / 2 || new_size > list->capacity;
  }

  void InvertedLists::copy_shared_data(InvertedList *dst, InvertedList *src)
  {
    size_t n_entries_to_copy = min(dst->size, src->size);
    if (n_entries_to_copy == 0)
    {
      return;
    }
    size_t n_bytes_vectors = n_entries_to_copy * vector_size;
    size_t n_bytes_ids = n_entries_to_copy * sizeof(vector_id_t);
    memcpy(get_vectors_by_list(dst), get_vectors_by_list(src), n_bytes_vectors);
    memcpy(get_ids_by_list(dst), get_ids_by_list(src), n_bytes_ids);
  }

  void InvertedLists::resize_list(list_id_t list_id, size_t new_size)
  {
    InvertedList *list = &id_to_list_map[list_id];
    if (!does_list_need_reallocation(list, new_size))
    {
      list->size = new_size;
      return;
    }
    free_slot(list);
    InvertedList new_list;
    new_list.size = new_size;
    if (new_size == 0)
    {
      new_list.capacity = 0;
    }
    else
    {
      Slot new_slot = alloc_slot(new_size);
      new_list.capacity = new_slot.capacity;
      new_list.offset = new_slot.offset;
      if (new_list.offset != list->offset)
      {
        copy_shared_data(&new_list, list);
      }
    }
    id_to_list_map[list_id] = new_list;
  }

  InvertedLists::slot_it_t InvertedLists::find_large_enough_slot_index(size_t capacity)
  {
    for (auto it = free_slots.begin(); it != free_slots.end(); it++)
    {
      if (it->capacity >= capacity)
      {
        return it;
      }
    }
    return free_slots.end();
  }

  void InvertedLists::grow_region_until_free_capacity(size_t capacity)
  {
    size_t new_size = total_size;
    while (new_size - total_size < capacity)
    {
      new_size *= 2;
    }
    resize_region(new_size);
  }

  InvertedLists::Slot InvertedLists::alloc_slot(size_t size)
  {
    auto slot_it = find_large_enough_slot_index(size);
    if (slot_it == free_slots.end())
    {
      grow_region_until_free_capacity(size);
      slot_it = find_large_enough_slot_index(size);
    }
    Slot *slot = &(*slot_it);
    Slot alloced_slot;
    alloced_slot.offset = slot->offset;
    alloced_slot.capacity = size;
    if (slot->capacity == size)
    {
      free_slots.erase(slot_it);
    }
    else
    {
      slot->offset += size;
      slot->capacity -= size;
    }
    return alloced_slot;
  }

  InvertedLists::slot_it_t InvertedLists::find_next_slot_to_right(Slot *slot)
  {
    for (auto it = free_slots.begin(); it != free_slots.end(); it++)
    {
      if (it->offset > slot->offset)
      {
        return it;
      }
    }
    return free_slots.end();
  }

  InvertedLists::slot_it_t InvertedLists::find_next_slot_to_left(
      slot_it_t next_slot_right)
  {
    if (next_slot_right == free_slots.begin())
    {
      return free_slots.end();
    }
    return prev(next_slot_right);
  }

  bool InvertedLists::are_slots_adjacent(
      Slot *slot_left, Slot *slot_right)
  {
    if (slot_left == nullptr || slot_right == nullptr)
    {
      return false;
    }
    return slot_left->offset + slot_left->capacity == slot_right->offset;
  }

  InvertedLists::Slot *InvertedLists::to_slot(slot_it_t it)
  {
    if (it == free_slots.end())
    {
      return nullptr;
    }
    return &(*it);
  }

  void InvertedLists::free_slot(Slot *slot)
  {
    slot_it_t slot_right_it = find_next_slot_to_right(slot);
    slot_it_t slot_left_it = find_next_slot_to_left(slot_right_it);
    Slot *slot_right = to_slot(slot_right_it);
    Slot *slot_left = to_slot(slot_left_it);
    bool has_adjacent_slot_left = are_slots_adjacent(slot_left, slot);
    bool has_adjacent_slot_right = are_slots_adjacent(slot, slot_right);
    if (has_adjacent_slot_left && has_adjacent_slot_right)
    {
      slot_left->capacity += slot->capacity + slot_right->capacity;
      free_slots.erase(slot_right_it);
    }
    else if (has_adjacent_slot_left && !has_adjacent_slot_right)
    {
      slot_left->capacity += slot->capacity;
    }
    else if (!has_adjacent_slot_left && has_adjacent_slot_right)
    {
      slot_right->offset -= slot->capacity;
      slot_right->capacity += slot->capacity;
    }
    else
    {
      free_slots.insert(slot_right_it, *slot);
    }
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
      throw "list already exists";
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
    throw "delete_list() not implemented";
  }

  void InvertedLists::update_entries(
      list_id_t list_id,
      vector_el_t *vectors,
      vector_id_t *ids,
      size_t n_entries,
      size_t offset)
  {
    InvertedList *list = &id_to_list_map[list_id];
    vector_el_t *list_vectors = get_vectors_by_list(list);
    vector_id_t *list_ids = get_ids_by_list(list);
    memcpy(list_vectors + offset, vectors, n_entries * vector_size);
    memcpy(list_ids + offset, ids, n_entries * sizeof(vector_id_t));
  }

  void InvertedLists::add_entries(
      list_id_t list_id,
      vector_el_t *vectors,
      vector_id_t *ids,
      size_t n_entries)
  {
    InvertedList *list = &id_to_list_map[list_id];
    resize_list(list_id, list->size + n_entries);
    update_entries(list_id, vectors, ids, n_entries, list->size);
  }
}