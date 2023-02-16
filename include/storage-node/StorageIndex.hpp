#ifndef INDEX_HPP_
#define INDEX_HPP_

#include <string>
#include <queue>

#include "InvertedLists.hpp"
#include "L2Space.hpp"

namespace ann_dkvs
{
  typedef std::pair<distance_t, vector_id_t> vector_distance_id_t;
  class VectorDistanceIdMaxHeapCompare
  {
  public:
    bool operator()(vector_distance_id_t parent, vector_distance_id_t child)
    {
      bool swapParentWithChild = (parent.first < child.first ||
                                  (parent.first == child.first && parent.second < child.second));
      return swapParentWithChild;
    }
  };
  typedef std::priority_queue<vector_distance_id_t, std::vector<vector_distance_id_t>, VectorDistanceIdMaxHeapCompare> heap_t;

  class StorageIndex
  {
  private:
    InvertedLists *lists;
    distance_func_t distance_func;
    std::vector<vector_distance_id_t> extract_results(heap_t *candidates);
    void search_preassigned_list(
        list_id_t list_id,
        vector_el_t *query,
        size_t k,
        heap_t *candidates);

  public:
    StorageIndex(InvertedLists *lists);
    std::vector<vector_distance_id_t> search_preassigned(
        list_id_t *list_ids,
        size_t n_lists,
        vector_el_t *query,
        size_t k);
  };
}
#endif // INDEX_HPP_