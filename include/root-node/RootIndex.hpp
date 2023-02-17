#ifndef ROOT_INDEX_HPP_
#define ROOT_INDEX_HPP_

#include <vector>
#include <queue>

#include "types.hpp"

namespace ann_dkvs
{
  typedef struct
  {
    distance_t distance;
    list_id_t list_id;
  } CentroidsResult;
  class CentroidsDistanceIdMaxHeapCompare
  {
  public:
    bool operator()(CentroidsResult parent, CentroidsResult child)
    {
      bool swapParentWithChild = (parent.distance > child.distance ||
                                  (parent.distance == child.distance && parent.list_id < child.list_id));
      return swapParentWithChild;
    }
  };
  typedef std::priority_queue<CentroidsResult, std::vector<CentroidsResult>, CentroidsDistanceIdMaxHeapCompare> centroids_heap_t;
  class RootIndex
  {
  private:
    len_t vector_dim;
    vector_el_t *centroids;
    len_t n_centroids;
    list_ids_t extract_list_ids(centroids_heap_t *nearest_centroids);

  public:
    RootIndex(len_t vector_dim, vector_el_t *centroids, len_t n_centroids);
    list_ids_t preassign_query(Query *query);
  };
} // namespace ann_dkvs
#endif // ROOT_INDEX_HPP_