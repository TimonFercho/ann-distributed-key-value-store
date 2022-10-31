#include "StorageNode.h"

namespace ann_dkvs
{
  StorageNode::StorageNode(Index index) : index(index) {}

  vector<result_t> StorageNode::receive_knn_request(
      list_id_t *list_ids,
      size_t nlist,
      vector_el_t *query,
      size_t k)
  {
    vector<result_t> results = index.search_preassigned(list_ids, nlist, query, k);
    return results;
  }
}