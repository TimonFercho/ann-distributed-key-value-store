#ifndef QUERY_HPP_
#define QUERY_HPP_

namespace ann_dkvs
{
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

  typedef struct
  {
    distance_t distance;
    vector_id_t vector_id;
  } QueryResult;

  typedef std::vector<QueryResult> QueryResults;
} // namespace ann_dkvs
#endif // QUERY_HPP_