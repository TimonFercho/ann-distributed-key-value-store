#ifndef STORAGE_NODE_HPP_
#define STORAGE_NODE_HPP_

#include "StorageIndex.hpp"

namespace ann_dkvs
{
  class StorageNode
  {
  private:
    StorageIndex index;

  public:
    StorageNode(StorageIndex index);
  };
}

#endif // STORAGE_NODE_HPP_