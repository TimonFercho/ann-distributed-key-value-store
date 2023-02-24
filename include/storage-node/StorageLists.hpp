#ifndef INVERTED_LISTS_HPP_
#define INVERTED_LISTS_HPP_

#include <unordered_map>
#include <vector>
#include <string>

#include "types.hpp"

#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define FLAG_READ "r"
#define FLAG_READ_WRITE "r+"

namespace ann_dkvs
{
  class StorageLists
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

    typedef std::unordered_map<list_id_t, InvertedList> list_id_list_map_t;
    typedef std::unordered_map<list_id_t, len_t> list_id_counts_map_t;
    typedef std::vector<StorageLists::Slot>::iterator slot_it_t;
    const size_t min_total_size = 32;
    const std::string filename;
    const size_t vector_dim;
    const size_t vector_size;

    size_t total_size;
    uint8_t *base_ptr;
    list_id_list_map_t id_to_list_map;
    std::vector<Slot> free_slots;

    void mmap_region();
    vector_el_t *get_vectors_by_list(const InvertedList *list) const;
    vector_id_t *get_ids_by_list(const InvertedList *list) const;
    size_t get_ids_size(const len_t n_entries) const;
    size_t get_vectors_size(const len_t n_entries) const;
    size_t get_total_list_size(const InvertedList *list) const;
    Slot list_to_slot(const InvertedList *list);
    void resize_region(const size_t new_size);
    InvertedList alloc_list(const len_t n_entries);
    size_t alloc_slot(const size_t size);
    void free_slot(const Slot *slot);
    size_t round_up_to_next_power_of_two(const size_t n) const;
    bool has_free_slot_at_end() const;
    void ensure_file_created_and_region_unmapped() const;
    void resize_file(const size_t size);
    bool does_list_need_reallocation(const InvertedList *list, const len_t new_length) const;
    void copy_shared_data(const InvertedList *dst, const InvertedList *src) const;
    slot_it_t find_large_enough_slot_index(const size_t size);
    void grow_region_until_enough_space(size_t size);
    slot_it_t find_next_slot_to_right(const Slot *slot);
    slot_it_t find_next_slot_to_left(const slot_it_t next_slot_right);
    bool are_slots_adjacent(const Slot *slot_left, const Slot *slot_right) const;
    Slot *to_slot(const slot_it_t it) const;
    void reserve_space(const len_t n_entries);
    list_id_counts_map_t bulk_create_lists(const std::string &list_ids_filename, const len_t n_entries);
    std::ifstream open_filestream(const std::string &filename) const;

  public:
    StorageLists(const len_t vector_dim, const std::string &filename);
    ~StorageLists();
    len_t get_length() const;
    size_t get_total_size() const;
    size_t get_free_space() const;
    size_t get_largest_continuous_free_space() const;
    size_t get_vector_size() const;
    len_t get_vector_dim() const;
    std::string get_filename() const;
    vector_el_t *get_vectors(const list_id_t list_id) const;
    vector_id_t *get_ids(const list_id_t list_id) const;
    len_t get_list_length(const list_id_t list_id) const;
    void resize_list(const list_id_t list_id, const len_t length);
    void insert_entries(const list_id_t list_id, const vector_el_t *vectors, const vector_id_t *ids, const len_t n_entries);
    void update_entries(const list_id_t list_id, const vector_el_t *vectors, const vector_id_t *ids, const size_t offset, const len_t n_entries) const;
    void create_list(
        const list_id_t list_id,
        const len_t n_entries);
    void bulk_insert_entries(const std::string &vectors_filename, const std::string &vector_ids_filename, const std::string &list_ids_filename, const len_t n_entries);
  };
}

#endif // INVERTED_LISTS_HPP_