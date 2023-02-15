#ifndef INDEX_H_
#define INDEX_H_

#include <string>
#include <queue>

#include "InvertedLists.hpp"
#include "L2Space.hpp"

namespace ann_dkvs
{
  typedef pair<distance_t, vector_id_t> vector_distance_id_t;
  class VectorDistanceIdMaxHeapCompare
  {
  public:
    bool operator()(vector_distance_id_t below, vector_distance_id_t above)
    {
      if (below.first > above.first)
      {
        return true;
      }
      else if (below.first == above.first && below.second < above.second)
      {
        return true;
      }
      return false;
    }
  };
  typedef priority_queue<vector_distance_id_t, vector<vector_distance_id_t>, VectorDistanceIdMaxHeapCompare> heap_t;

  class StorageIndex
  {
  private:
    InvertedLists *lists;
    distance_func_t distance_func;
    vector<vector_distance_id_t> extract_results(heap_t *candidates);
    void search_preassigned_list(
        list_id_t list_id,
        vector_el_t *query,
        size_t k,
        heap_t *candidates);

  public:
    StorageIndex(InvertedLists *lists);
    vector<vector_distance_id_t> search_preassigned(
        list_id_t *list_ids,
        size_t n_lists,
        vector_el_t *query,
        size_t k);
  };
}
#endif // INDEX_H_