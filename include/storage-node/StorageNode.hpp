#ifndef STORAGE_NODE_HPP_
#define STORAGE_NODE_HPP_

#include "StorageIndex.hpp"

namespace ann_dkvs
{
  class StorageNode
  {
  private:
    const StorageIndex &index;

  public:
    StorageNode(const StorageIndex &index);
  };
}

#endif // STORAGE_NODE_HPP_