#ifndef TYPES_HPP_
#define TYPES_HPP_

#include <vector>
#include <cassert>

namespace ann_dkvs
{
  // 1 byte
  typedef unsigned char uint8_t;
  // 4 bytes
  typedef float vector_el_t;
  typedef float distance_t;
  // 8 bytes
  typedef unsigned long len_t;
  typedef unsigned long size_t;
  typedef signed long int64_t;
  typedef int64_t list_id_t;
  typedef int64_t vector_id_t;

  typedef std::vector<list_id_t> list_ids_t;
}
#endif // TYPES_HPP_
