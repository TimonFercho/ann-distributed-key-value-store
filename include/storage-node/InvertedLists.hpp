#ifndef INVERTED_LISTS_H_
#define INVERTED_LISTS_H_

#include <unordered_map>
#include <vector>
#include <string>

#include "types.hpp"

#define PPROT_READ 0x1
#define PROT_WRITE 0x2
#define FLAG_READ "r"
#define FLAG_READ_WRITE "r+"

using namespace std;

namespace ann_dkvs
{
  class InvertedLists
  {
  private:
    struct Slot
    {
      size_t offset;
      size_t size;
    };
    struct InvertedList
    {
      size_t offset;
      len_t allocated_entries;
      len_t used_entries;
    };

    typedef std::unordered_map<list_id_t, InvertedList> hash_map_t;
    typedef vector<InvertedLists::Slot>::iterator slot_it_t;
    const size_t min_total_size = 32;
    const string filename;
    const size_t vector_dim;
    const size_t vector_size;

    size_t total_size;
    uint8_t *base_ptr;
    hash_map_t id_to_list_map;
    vector<Slot> free_slots;

    void mmap_region();
    vector_el_t *get_vectors_by_list(InvertedList *list) const;
    vector_id_t *get_ids_by_list(InvertedList *list) const;
    size_t get_ids_size(len_t n_entries) const;
    size_t get_vectors_size(len_t n_entries) const;
    size_t get_total_list_size(InvertedList *list) const;
    Slot list_to_slot(InvertedList *list);
    void resize_region(size_t new_size);
    InvertedList alloc_list(len_t n_entries);
    size_t alloc_slot(size_t size);
    void free_slot(Slot *slot);
    size_t round_up_to_next_power_of_two(size_t n);
    bool has_free_slot_at_end();
    void ensure_file_created_and_region_unmapped();
    void resize_file(size_t size);
    bool does_list_need_reallocation(InvertedList *list, len_t new_length);
    void copy_shared_data(InvertedList *dst, InvertedList *src);
    slot_it_t find_large_enough_slot_index(size_t size);
    void grow_region_until_enough_space(size_t size);
    slot_it_t find_next_slot_to_right(Slot *slot);
    slot_it_t find_next_slot_to_left(slot_it_t next_slot_right);
    bool are_slots_adjacent(Slot *slot_left, Slot *slot_right);
    Slot *to_slot(slot_it_t it);

  public:
    InvertedLists(len_t vector_dim, string filename);
    ~InvertedLists();
    len_t get_length() const;
    size_t get_total_size() const;
    size_t get_free_space() const;
    size_t get_largest_continuous_free_space() const;
    size_t get_vector_size() const;
    len_t get_vector_dim() const;
    string get_filename() const;
    vector_el_t *get_vectors(list_id_t list_id);
    vector_id_t *get_ids(list_id_t list_id);
    len_t get_list_length(list_id_t list_id);
    void resize_list(list_id_t list_id, len_t length);
    void add_entries(list_id_t list_id, vector_el_t *vectors, vector_id_t *ids, len_t n_entries);
    void update_entries(list_id_t list_id, vector_el_t *vectors, vector_id_t *ids, size_t offset, len_t n_entries);
    void create_list(
        list_id_t list_id,
        len_t n_entries);
    void delete_list(list_id_t list_id);
    void reserve_space(len_t n_entries);
  };
}

#endif // INVERTED_LISTS_H_