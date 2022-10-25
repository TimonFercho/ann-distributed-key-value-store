#ifndef INVERTED_LISTS_H_
#define INVERTED_LISTS_H_

#include <unordered_map>

#include "../types.h"

#define PPROT_READ 0x1
#define FLAG_READ "r"

using namespace std;

namespace ann_dkvs
{
  struct InvertedList
  {
    size_t id;
    size_t size;
    vector_el_t *data;
  };
  typedef std::unordered_map<list_id_t, InvertedList> hash_map_t;

  class InvertedLists
  {
  private:
    size_t vector_size;
    hash_map_t id_to_list_map;
    string base_path;
    string get_filename(InvertedList *list) const;
    void mmap_list(InvertedList *list);
    vector_el_t *get_vectors_by_list(InvertedList *list) const;
    vector_id_t *get_ids_by_list(InvertedList *list) const;
    size_t get_total_size(InvertedList *list) const;

  public:
    InvertedLists(size_t vector_size, string base_path);
    size_t get_list_count() const;
    size_t get_vector_size() const;
    vector_el_t *get_vectors(list_id_t list_no);
    vector_id_t *get_ids(list_id_t list_no);
    size_t get_size(list_id_t list_no);
    void insert_list(
        list_id_t list_id,
        vector_el_t *vectors,
        vector_id_t *ids,
        size_t size);
    void delete_list(list_id_t list_id);
    InvertedList *get_list(list_id_t id) const;
  };
};

#endif // INVERTED_LISTS_H_