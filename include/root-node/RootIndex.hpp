#ifndef ROOT_INDEX_HPP_
#define ROOT_INDEX_HPP_

#include <vector>
#include <queue>

#include "types.hpp"
#include "Query.hpp"

namespace ann_dkvs
{
  struct CentroidsResult
  {
    distance_t distance;
    list_id_t list_id;
    friend bool operator<(const CentroidsResult &a, const CentroidsResult &b)
    {
      bool closer = a.distance < b.distance ||
                    (a.distance == b.distance && a.list_id < b.list_id);
      return closer;
    }
  };
  class CentroidsDistanceIdMaxHeapCompare
  {
  public:
    bool operator()(const CentroidsResult &parent, const CentroidsResult &child)
    {
      bool swapParentWithChild = parent < child;
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
    void allocate_list_ids(Query *query, centroids_heap_t *nearest_centroids);
    void add_candidate(const Query *query, const CentroidsResult &candidate, centroids_heap_t &candidates);

  public:
    RootIndex(len_t vector_dim, vector_el_t *centroids, len_t n_centroids);
    void preassign_query(Query *query);
    void batch_preassign_queries(QueryBatch queries);
  };
} // namespace ann_dkvs
#endif // ROOT_INDEX_HPP_