#include <sys/mman.h>
#include <string.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <fstream>

#include "StorageLists.hpp"

namespace ann_dkvs
{
  void StorageLists::mmap_region()
  {
    FILE *f = fopen(filename.c_str(), "r+");
    if (f == nullptr)
    {
      throw std::runtime_error("Could not open file " + filename);
    }
    base_ptr = (uint8_t *)mmap(
        nullptr,
        total_size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fileno(f),
        0);
    if (base_ptr == MAP_FAILED)
    {
      throw std::runtime_error("Could not mmap file " + filename);
    }
    fclose(f);
  }

  vector_el_t *StorageLists::get_vectors_by_list(InvertedList *list) const
  {
    return (vector_el_t *)(base_ptr + list->offset);
  }

  vector_id_t *StorageLists::get_ids_by_list(InvertedList *list) const
  {
    vector_el_t *vector_ptr = get_vectors_by_list(list);
    vector_id_t *id_ptr = (vector_id_t *)((size_t)vector_ptr + vector_size * list->allocated_entries);
    return id_ptr;
  }

  size_t StorageLists::get_vectors_size(len_t n_entries) const
  {
    return n_entries * vector_size;
  }

  size_t StorageLists::get_ids_size(len_t n_entries) const
  {
    return n_entries * sizeof(vector_id_t);
  }

  size_t StorageLists::get_total_list_size(InvertedList *list) const
  {
    len_t n_entries = list->allocated_entries;
    return get_vectors_size(n_entries) + get_ids_size(n_entries);
  }

  size_t StorageLists::get_free_space() const
  {
    size_t free_space = 0;
    for (auto slot : free_slots)
    {
      free_space += slot.size;
    }
    return free_space;
  }

  size_t StorageLists::get_largest_continuous_free_space() const
  {
    size_t max_free_space = 0;
    for (auto slot : free_slots)
    {
      if (slot.size > max_free_space)
      {
        max_free_space = slot.size;
      }
    }
    return max_free_space;
  }

  StorageLists::StorageLists(len_t vector_dim, std::string filename) : filename(filename), vector_dim(vector_dim), vector_size(vector_dim * sizeof(vector_el_t)), total_size(0)
  {
    if (vector_dim == 0)
    {
      throw std::out_of_range("Vector dimension must be greater than 0");
    }
  }

  StorageLists::~StorageLists()
  {
    if (base_ptr != nullptr)
    {
      munmap(base_ptr, total_size);
    }
  }

  len_t StorageLists::get_length() const
  {
    return id_to_list_map.size();
  }

  size_t StorageLists::get_total_size() const
  {
    return total_size;
  }

  size_t StorageLists::get_vector_size() const
  {
    return vector_size;
  }

  len_t StorageLists::get_vector_dim() const
  {
    return vector_dim;
  }

  std::string StorageLists::get_filename() const
  {
    return filename;
  }

  bool StorageLists::has_free_slot_at_end()
  {
    if (free_slots.size() == 0)
      return false;
    Slot last_slot = free_slots.back();
    return last_slot.offset + last_slot.size == total_size;
  }

  void StorageLists::ensure_file_created_and_region_unmapped()
  {
    if (total_size == 0)
    {
      FILE *f = fopen(filename.c_str(), "r+");
      if (f == nullptr)
      {
        f = fopen(filename.c_str(), "w+");
        if (f == nullptr)
        {
          throw std::runtime_error("Could not create file " + filename);
        }
      }
      fclose(f);
    }
    else
    {
      munmap(base_ptr, total_size);
    }
  }

  void StorageLists::resize_file(size_t size)
  {
    FILE *f = fopen(filename.c_str(), "r+");
    if (f == nullptr)
    {
      throw std::runtime_error("Could not open file " + filename);
    }
    if (ftruncate(fileno(f), size) == -1)
    {
      throw std::runtime_error("Could not resize file " + filename);
    }
    fclose(f);
  }

  void StorageLists::resize_region(size_t new_size)
  {
    if (new_size < total_size)
    {
      throw std::runtime_error("Cannot shrink region");
    }
    if (new_size == total_size)
    {
      return;
    }
    ensure_file_created_and_region_unmapped();
    size_t size_to_grow = new_size - total_size;
    if (has_free_slot_at_end())
    {
      free_slots.back().size += size_to_grow;
    }
    else
    {
      Slot new_slot;
      new_slot.offset = total_size;
      new_slot.size = size_to_grow;
      free_slots.push_back(new_slot);
    }
    total_size = new_size;
    resize_file(total_size);
    mmap_region();
  }

  bool StorageLists::does_list_need_reallocation(InvertedList *list, len_t n_entries)
  {
    return n_entries <= list->allocated_entries / 2 || n_entries > list->allocated_entries;
  }

  void StorageLists::copy_shared_data(InvertedList *dst, InvertedList *src)
  {
    len_t n_entries_to_copy = std::min(dst->used_entries, src->used_entries);
    if (n_entries_to_copy == 0)
    {
      return;
    }
    memcpy(get_vectors_by_list(dst), get_vectors_by_list(src), get_vectors_size(n_entries_to_copy));
    memcpy(get_ids_by_list(dst), get_ids_by_list(src), get_ids_size(n_entries_to_copy));
  }

  StorageLists::Slot StorageLists::list_to_slot(InvertedList *list)
  {
    Slot slot;
    slot.offset = list->offset;
    slot.size = get_total_list_size(list);
    return slot;
  }

  void StorageLists::resize_list(list_id_t list_id, len_t n_entries)
  {
    list_id_list_map_t::iterator list_it = id_to_list_map.find(list_id);
    if (list_it == id_to_list_map.end())
    {
      throw std::invalid_argument("List " + std::to_string(list_id) + " does not exist");
    }
    if (n_entries == 0)
    {
      throw std::out_of_range("Cannot resize list to 0 entries");
    }
    InvertedList *list = &list_it->second;
    if (!does_list_need_reallocation(list, n_entries))
    {
      list->used_entries = n_entries;
      return;
    }
    Slot slot = list_to_slot(list);
    free_slot(&slot);
    InvertedList new_list;
    new_list = alloc_list(n_entries);
    if (new_list.offset == list->offset)
    {
      memmove(get_ids_by_list(&new_list), get_ids_by_list(list), get_ids_size(list->used_entries));
    }
    else
    {
      copy_shared_data(&new_list, list);
    }
    id_to_list_map[list_id] = new_list;
  }

  StorageLists::InvertedList StorageLists::alloc_list(len_t n_entries)
  {
    InvertedList list;
    list.used_entries = n_entries;
    len_t allocated_entries = round_up_to_next_power_of_two(n_entries);
    list.allocated_entries = allocated_entries;
    size_t list_size = get_total_list_size(&list);
    list.offset = alloc_slot(list_size);
    return list;
  }

  StorageLists::slot_it_t StorageLists::find_large_enough_slot_index(size_t size)
  {
    for (auto it = free_slots.begin(); it != free_slots.end(); it++)
    {
      if (it->size >= size)
      {
        return it;
      }
    }
    return free_slots.end();
  }

  void StorageLists::grow_region_until_enough_space(size_t size)
  {
    size_t new_size = total_size == 0 ? min_total_size : total_size;
    if (has_free_slot_at_end())
    {
      size -= free_slots.back().size;
    }
    while (new_size - total_size < size)
    {
      new_size *= 2;
    }
    resize_region(new_size);
  }

  size_t StorageLists::alloc_slot(size_t size)
  {
    auto slot_it = find_large_enough_slot_index(size);
    if (slot_it == free_slots.end())
    {
      grow_region_until_enough_space(size);
      slot_it = find_large_enough_slot_index(size);
    }
    Slot *slot = &(*slot_it);
    size_t offset = slot->offset;
    if (slot->size == size)
    {
      free_slots.erase(slot_it);
    }
    else
    {
      slot->offset += size;
      slot->size -= size;
    }
    return offset;
  }

  StorageLists::slot_it_t StorageLists::find_next_slot_to_right(Slot *slot)
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

  StorageLists::slot_it_t StorageLists::find_next_slot_to_left(
      slot_it_t next_slot_right)
  {
    if (next_slot_right == free_slots.begin())
    {
      return free_slots.end();
    }
    return prev(next_slot_right);
  }

  bool StorageLists::are_slots_adjacent(
      Slot *slot_left, Slot *slot_right)
  {
    if (slot_left == nullptr || slot_right == nullptr)
    {
      return false;
    }
    return slot_left->offset + slot_left->size == slot_right->offset;
  }

  StorageLists::Slot *StorageLists::to_slot(slot_it_t it)
  {
    if (it == free_slots.end())
    {
      return nullptr;
    }
    return &(*it);
  }

  void StorageLists::free_slot(Slot *slot)
  {
    slot_it_t slot_right_it = find_next_slot_to_right(slot);
    slot_it_t slot_left_it = find_next_slot_to_left(slot_right_it);
    Slot *slot_right = to_slot(slot_right_it);
    Slot *slot_left = to_slot(slot_left_it);
    bool has_adjacent_slot_left = are_slots_adjacent(slot_left, slot);
    bool has_adjacent_slot_right = are_slots_adjacent(slot, slot_right);
    if (has_adjacent_slot_left && has_adjacent_slot_right)
    {
      slot_left->size += slot->size + slot_right->size;
      free_slots.erase(slot_right_it);
    }
    else if (has_adjacent_slot_left && !has_adjacent_slot_right)
    {
      slot_left->size += slot->size;
    }
    else if (!has_adjacent_slot_left && has_adjacent_slot_right)
    {
      slot_right->offset -= slot->size;
      slot_right->size += slot->size;
    }
    else
    {
      free_slots.insert(slot_right_it, *slot);
    }
  }

  vector_el_t *StorageLists::get_vectors(list_id_t list_id)
  {
    list_id_list_map_t::iterator list_it = id_to_list_map.find(list_id);
    if (list_it == id_to_list_map.end())
    {
      throw std::invalid_argument("List not found");
    }
    return get_vectors_by_list(&list_it->second);
  }

  vector_id_t *StorageLists::get_ids(list_id_t list_id)
  {
    list_id_list_map_t::iterator list_it = id_to_list_map.find(list_id);
    if (list_it == id_to_list_map.end())
    {
      throw std::invalid_argument("List not found");
    }
    return get_ids_by_list(&list_it->second);
  }

  len_t StorageLists::get_list_length(list_id_t list_id)
  {
    list_id_list_map_t::iterator list_it = id_to_list_map.find(list_id);
    if (list_it == id_to_list_map.end())
    {
      throw std::invalid_argument("List not found");
    }
    return list_it->second.used_entries;
  }

  size_t StorageLists::round_up_to_next_power_of_two(size_t n)
  {
    size_t power = 1;
    while (power < n)
    {
      power *= 2;
    }
    return power;
  }

  void StorageLists::create_list(
      list_id_t list_id,
      len_t n_entries)
  {
    if (id_to_list_map.find(list_id) != id_to_list_map.end())
    {
      throw std::invalid_argument("List already exists");
    }
    if (n_entries == 0)
    {
      throw std::out_of_range("List must have at least one entry");
    }
    InvertedList list = alloc_list(n_entries);
    id_to_list_map[list_id] = list;
  }

  void StorageLists::update_entries(
      list_id_t list_id,
      vector_el_t *vectors,
      vector_id_t *ids,
      size_t offset,
      len_t n_entries)
  {
    list_id_list_map_t::iterator list_it = id_to_list_map.find(list_id);
    if (list_it == id_to_list_map.end())
    {
      throw std::invalid_argument("List not found");
    }
    InvertedList *list = &list_it->second;
    if (offset + n_entries > list->used_entries)
    {
      throw std::out_of_range("updating more entries than list has");
    }
    vector_el_t *list_vectors = get_vectors_by_list(list);
    vector_id_t *list_ids = get_ids_by_list(list);
    memcpy(list_vectors + offset * vector_dim, vectors, get_vectors_size(n_entries));
    memcpy(list_ids + offset, ids, get_ids_size(n_entries));
  }

  void StorageLists::insert_entries(
      list_id_t list_id,
      vector_el_t *vectors,
      vector_id_t *ids,
      len_t n_entries)
  {
    InvertedList *list = &id_to_list_map[list_id];
    resize_list(list_id, list->used_entries + n_entries);
    update_entries(list_id, vectors, ids, list->used_entries - n_entries, n_entries);
  }

  void StorageLists::reserve_space(len_t n_entries)
  {
    if (n_entries == 0)
    {
      throw std::runtime_error("Cannot reserve 0 entries");
    }
    size_t size_to_reserve = get_vectors_size(n_entries) + get_ids_size(n_entries);
    grow_region_until_enough_space(size_to_reserve);
  }

  std::ifstream StorageLists::open_filestream(std::string filename)
  {
    std::ifstream filestream(filename, std::ios::in | std::ios::binary);
    if (!filestream.is_open())
    {
      throw std::runtime_error("Could not open file " + filename);
    }
    return filestream;
  }

  StorageLists::list_id_counts_map_t StorageLists::bulk_create_lists(
      std::string list_ids_filename,
      len_t n_entries)
  {
    list_id_counts_map_t lists_counts;
    std::ifstream list_ids_file = open_filestream(list_ids_filename);
    list_id_t list_id;
    len_t n_entries_read = 0;
    while (list_ids_file.read((char *)&list_id, sizeof(list_id_t)))
    {
      lists_counts[list_id]++;
      n_entries_read++;
    }
    list_ids_file.close();
    if (list_ids_file.bad())
    {
      throw std::runtime_error("Error reading list ids file");
    }
    if (n_entries_read != n_entries)
    {
      throw std::runtime_error("Number of entries in list ids file does not match n_entries");
    }

    for (auto list_it : lists_counts)
    {
      create_list(list_it.first, list_it.second);
    }
    return lists_counts;
  }

  void StorageLists::bulk_insert_entries(
      std::string vectors_filename,
      std::string ids_filename,
      std::string list_ids_filename,
      len_t n_entries)
  {
    if (total_size != 0)
    {
      throw std::runtime_error("bulk_insert_entries() can only be called on an empty inverted lists");
    }
    reserve_space(n_entries);
    list_id_counts_map_t entries_left = bulk_create_lists(list_ids_filename, n_entries);

    std::ifstream vectors_file = open_filestream(vectors_filename);
    std::ifstream ids_file = open_filestream(ids_filename);
    std::ifstream list_ids_file = open_filestream(list_ids_filename);

    vector_el_t *cur_vector = new vector_el_t[vector_dim];
    len_t n_entries_read = 0;
    vector_id_t cur_id;
    list_id_t cur_list_id;

    while (n_entries_read < n_entries)
    {
      if (!vectors_file.read((char *)cur_vector, sizeof(vector_el_t) * vector_dim))
      {
        throw std::runtime_error("Error reading vectors file");
      }
      if (!ids_file.read((char *)&cur_id, sizeof(vector_id_t)))
      {
        throw std::runtime_error("Error reading ids file");
      }
      if (!list_ids_file.read((char *)&cur_list_id, sizeof(list_id_t)))
      {
        throw std::runtime_error("Error reading list ids file");
      }
      len_t list_length = get_list_length(cur_list_id);
      len_t cur_list_offset = list_length - entries_left[cur_list_id];
      update_entries(cur_list_id, cur_vector, &cur_id, cur_list_offset, 1);
      entries_left[cur_list_id]--;
      n_entries_read++;
    }

    if (vectors_file.fail() || ids_file.fail() || list_ids_file.fail())
    {
      throw std::runtime_error("Error reading files");
    }

    vectors_file.close();
    ids_file.close();
    list_ids_file.close();
  }
}