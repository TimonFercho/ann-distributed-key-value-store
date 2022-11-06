#ifndef INDEX_H_
#define INDEX_H_

#include <string>
#include <queue>

#include "InvertedLists.hpp"

using namespace std;

namespace ann_dkvs
{
  typedef float distance_t;
  typedef pair<distance_t, vector_id_t> result_t;
  typedef priority_queue<result_t> heap_t;

  class Index
  {
  private:
    InvertedLists lists;
    vector<result_t> extract_results(heap_t candidates);
    void search_preassigned_list(
        list_id_t list_id,
        vector_el_t *query,
        size_t k,
        heap_t candidates);

  public:
    Index(InvertedLists lists);
    vector<result_t> search_preassigned(
        list_id_t *list_ids,
        size_t n_lists,
        vector_el_t *query,
        size_t k);
  };
}
#endif // INDEX_H_