#ifndef INVERTED_LISTS_H_
#define INVERTED_LISTS_H_

#include <unordered_map>
#include <vector>
#include <string>

#include "types.h"

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
      size_t capacity;
    };
    struct InvertedList : Slot
    {
      size_t size;
    };

    typedef std::unordered_map<list_id_t, InvertedList> hash_map_t;
    const size_t vector_dim;
    const string filename;
    const size_t vector_size;

    size_t total_size;
    uint8_t *base_ptr;
    hash_map_t id_to_list_map;
    vector<Slot> free_slots;

    void mmap_region();
    vector_el_t *get_vectors_by_list(InvertedList *list) const;
    vector_id_t *get_ids_by_list(InvertedList *list) const;
    size_t get_total_list_size(InvertedList *list) const;
    void resize_region(size_t new_size);
    Slot *alloc_slot(size_t size);
    void free_slot(Slot *slot);
    size_t round_up_to_next_power_of_two(size_t n);
    bool has_free_slot_at_end();
    void ensure_file_created_and_region_unmapped();
    void resize_file(size_t size);
    bool does_list_need_reallocation(InvertedList *list, size_t new_size);
    void copy_shared_data(InvertedList *dst, InvertedList *src);
    size_t find_large_enough_slot_index(size_t capacity);
    void grow_region_until_free_capacity(size_t capacity);

  public:
    InvertedLists(size_t vector_dim, string filename);
    ~InvertedLists();
    size_t get_size() const;
    size_t get_vector_size() const;
    size_t get_vector_dim() const;
    string get_filename() const;
    vector_el_t *get_vectors(list_id_t list_id);
    vector_id_t *get_ids(list_id_t list_id);
    size_t get_list_size(list_id_t list_id);
    void resize_list(list_id_t list_id, size_t new_size);
    void add_entries(list_id_t list_id, vector_el_t *vectors, vector_id_t *ids, size_t n_entries);
    void update_entries(list_id_t list_id, vector_el_t *vectors, vector_id_t *ids, size_t n_entries, size_t offset);
    void insert_list(
        list_id_t list_id,
        size_t size);
    void delete_list(list_id_t list_id);
    void reserve_space(size_t n_entries);
  };
}

#endif // INVERTED_LISTS_H_