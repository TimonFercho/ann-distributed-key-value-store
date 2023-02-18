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
    bool operator()(const QueryResult &parent, const QueryResult &child)
    {
      bool swapParentWithChild = parent < child;
      return swapParentWithChild;
    }
  };
  typedef std::priority_queue<QueryResult, std::vector<QueryResult>, VectorDistanceIdMaxHeapCompare> heap_t;
  typedef std::pair<len_t, list_id_t> QueryListPair;
  typedef std::vector<QueryListPair> QueryListPairs;

  class StorageIndex
  {

  private:
    StorageLists *lists;
    distance_func_t distance_func;
    QueryResults extract_results(heap_t *candidates);
    void search_preassigned_list(
        const Query *query,
        list_id_t list_id,
        heap_t &candidates);
    QueryListPairs get_work_items(QueryBatch queries);
    void add_candidate(const Query *query, const QueryResult &candidate, heap_t &candidates);

  public:
    StorageIndex(StorageLists *lists);
    QueryResults search_preassigned(Query *query);
    QueryResultsBatch batch_search_preassigned(const QueryBatch &queries);
  };
}
#endif // INDEX_HPP_