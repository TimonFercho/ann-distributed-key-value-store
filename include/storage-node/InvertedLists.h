#ifndef INVERTED_LISTS_H_
#define INVERTED_LISTS_H_

#include <unordered_map>
#include <string>

#include "types.h"

#define PPROT_READ 0x1
#define PROT_WRITE 0x2
#define FLAG_READ "r"
#define FLAG_READ_WRITE "r+"

using namespace std;

namespace ann_dkvs
{
  struct InvertedList
  {
    size_t id;
    size_t size;
    string filename;
    vector_el_t *data;
  };
  typedef std::unordered_map<list_id_t, InvertedList> hash_map_t;

  class InvertedLists
  {
  private:
    size_t vector_size;
    hash_map_t id_to_list_map;
    void mmap_list(InvertedList *list);
    vector_el_t *get_vectors_by_list(InvertedList *list) const;
    vector_id_t *get_ids_by_list(InvertedList *list) const;
    size_t get_total_size(InvertedList *list) const;

  public:
    InvertedLists(size_t vector_size);
    size_t get_size() const;
    size_t get_vector_size() const;
    vector_el_t *get_vectors(list_id_t list_id);
    vector_id_t *get_ids(list_id_t list_id);
    string get_filename(list_id_t list_id);
    size_t get_list_size(list_id_t list_id);
    void insert_list(
        list_id_t list_id,
        string filename,
        size_t size);
    void delete_list(list_id_t list_id);
    void write_list(
        string filename,
        vector_el_t *vectors,
        vector_id_t *ids,
        size_t size);
  };
}

#endif // INVERTED_LISTS_H_