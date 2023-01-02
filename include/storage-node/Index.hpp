#ifndef INDEX_H_
#define INDEX_H_

#include <string>
#include <queue>

#include "InvertedLists.hpp"

namespace ann_dkvs
{
  typedef float distance_t;
  typedef pair<distance_t, vector_id_t> vector_distance_id_t;
  typedef priority_queue<vector_distance_id_t> heap_t;

  class Index
  {
  private:
    InvertedLists *lists;
    vector<vector_distance_id_t> extract_results(heap_t *candidates);
    void search_preassigned_list(
        list_id_t list_id,
        vector_el_t *query,
        size_t k,
        heap_t *candidates);

  public:
    Index(InvertedLists *lists);
    vector<vector_distance_id_t> search_preassigned(
        list_id_t *list_ids,
        size_t n_lists,
        vector_el_t *query,
        size_t k);
  };
}
#endif // INDEX_H_