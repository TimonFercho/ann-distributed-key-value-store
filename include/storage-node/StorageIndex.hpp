#ifndef INDEX_HPP_
#define INDEX_HPP_

#include <string>
#include <queue>

#include "StorageLists.hpp"
#include "L2Space.hpp"
#include "Query.hpp"

namespace ann_dkvs
{
  class VectorDistanceIdMaxHeapCompare
  {
  public:
    bool operator()(QueryResult parent, QueryResult child)
    {
      bool swapParentWithChild = (parent.distance < child.distance ||
                                  (parent.distance == child.distance && parent.vector_id < child.vector_id));
      return swapParentWithChild;
    }
  };
  typedef std::priority_queue<QueryResult, std::vector<QueryResult>, VectorDistanceIdMaxHeapCompare> heap_t;

  class StorageIndex
  {
  private:
    StorageLists *lists;
    distance_func_t distance_func;
    QueryResults extract_results(heap_t *candidates);
    void search_preassigned_list(
        Query *query,
        list_id_t list_id,
        heap_t *candidates);

  public:
    StorageIndex(StorageLists *lists);
    QueryResults search_preassigned(Query *query);
    QueryResultsBatch batch_search_preassigned(QueryBatch queries);
  };
}
#endif // INDEX_HPP_