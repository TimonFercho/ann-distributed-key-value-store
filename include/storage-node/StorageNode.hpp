#ifndef STORAGE_NODE_H_
#define STORAGE_NODE_H_

#include "StorageIndex.hpp"

namespace ann_dkvs
{
  class StorageNode
  {
  private:
    StorageIndex index;

  public:
    StorageNode(StorageIndex index);
    std::vector<vector_distance_id_t> receive_knn_request(
        list_id_t *list_ids,
        size_t nlist,
        vector_el_t *query,
        size_t k);
  };
}

#endif // STORAGE_NODE_H_