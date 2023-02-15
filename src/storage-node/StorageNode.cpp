#include "StorageNode.hpp"

namespace ann_dkvs
{
  StorageNode::StorageNode(StorageIndex index) : index(index) {}

  vector<vector_distance_id_t> StorageNode::receive_knn_request(
      list_id_t *list_ids,
      size_t nlist,
      vector_el_t *query,
      size_t k)
  {
    vector<vector_distance_id_t> results = index.search_preassigned(list_ids, nlist, query, k);
    return results;
  }
}