#ifndef ROOTNODE_HPP_
#define ROOTNODE_HPP_

#include "RootIndex.hpp"

namespace ann_dkvs
{
  class RootNode
  {
  private:
    const RootIndex &index;

  public:
    RootNode(const RootIndex &index);
  };
}

#endif // ROOTNODE_HPP_
