#include "Query.hpp"

namespace ann_dkvs
{
  Query::Query(vector_el_t *query_vector, const len_t n_results, const len_t n_probes)
      : query_vector(query_vector), lists_to_probe(new list_id_t[n_probes]), n_results(n_results), n_probes(n_probes), free_lists_to_probe(true) {}
  Query::Query(vector_el_t *query_vector, list_id_t *lists_to_probe, const len_t n_results, const len_t n_probes)
      : query_vector(query_vector), lists_to_probe(lists_to_probe), n_results(n_results), n_probes(n_probes) {}
  vector_el_t *Query::get_query_vector() const { return query_vector; }
  list_id_t Query::get_list_to_probe(const len_t i) const
  {
    return lists_to_probe[i];
  }
  len_t Query::get_n_results() const
  {
    return n_results;
  }
  len_t Query::get_n_probe() const
  {
    return n_probes;
  }
  void Query::set_list_to_probe(const len_t offset, const list_id_t list_id) const
  {
    assert(offset < n_probes);
    lists_to_probe[offset] = list_id;
  }
  Query::~Query()
  {
    if (free_lists_to_probe)
    {
      delete[] lists_to_probe;
    }
  }
}