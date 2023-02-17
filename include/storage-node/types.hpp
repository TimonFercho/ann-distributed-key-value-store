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
  // 8 bytes
  typedef unsigned long len_t;
  typedef unsigned long size_t;
  typedef signed long int64_t;
  typedef int64_t list_id_t;
  typedef int64_t vector_id_t;

  typedef std::vector<list_id_t> list_ids_t;

  class Query
  {
  private:
    vector_el_t *query_vector;
    list_ids_t list_ids;
    len_t n_results;
    len_t n_probe;

  public:
    Query(vector_el_t *query_vector, len_t n_results, len_t n_probe) : query_vector(query_vector), n_results(n_results), n_probe(n_probe) {}
    Query(vector_el_t *query_vector, len_t n_results, list_ids_t list_ids) : query_vector(query_vector), list_ids(list_ids), n_results(n_results), n_probe(list_ids.size())
    {
      assert(list_ids.size() > 0);
    }
    vector_el_t *get_query_vector() { return query_vector; }
    list_ids_t get_list_ids() { return list_ids; }
    len_t get_n_results() { return n_results; }
    len_t get_n_probe() { return n_probe; }
    void preassign(list_ids_t list_ids)
    {
      this->list_ids = list_ids;
      this->n_probe = list_ids.size();
    }
    bool is_preassigned() { return list_ids.size() > 0; }
  };

  typedef float distance_t;
  typedef struct
  {
    distance_t distance;
    vector_id_t vector_id;
  } QueryResult;

  typedef std::vector<QueryResult> QueryResults;
}

#endif // TYPES_HPP_