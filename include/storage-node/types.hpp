#ifndef TYPES_HPP_
#define TYPES_HPP_

#include <vector>

namespace ann_dkvs
{
  // 1 byte
  typedef unsigned char uint8_t;
  // 4 bytes
  typedef float vector_el_t;
  // 8 bytes
  typedef unsigned long len_t;
  typedef unsigned long size_t;
  typedef signed long int64_t;
  typedef int64_t list_id_t;
  typedef int64_t vector_id_t;

  typedef struct
  {
    vector_el_t *query_vector;
    list_id_t *list_ids = nullptr;
    len_t n_probe = 1;
    len_t n_results = 1;
  } Query;

  typedef std::vector<list_id_t> list_ids_t;

  typedef float distance_t;
  typedef struct
  {
    distance_t distance;
    vector_id_t vector_id;
  } QueryResult;

  typedef std::vector<QueryResult> QueryResults;
}

#endif // TYPES_HPP_