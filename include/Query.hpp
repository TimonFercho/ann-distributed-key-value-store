#ifndef QUERY_HPP_
#define QUERY_HPP_

namespace ann_dkvs
{
  struct Query
  {
  private:
    vector_el_t *query_vector;
    list_id_t *list_to_probe;
    len_t n_results;
    len_t n_probe;
    bool free_list_to_probe = false;

  public:
    Query(vector_el_t *query_vector, len_t n_results, len_t n_probe) : query_vector(query_vector), list_to_probe(nullptr), n_results(n_results), n_probe(n_probe)
    {
      list_to_probe = new list_id_t[n_probe];
      free_list_to_probe = true;
    }
    Query(vector_el_t *query_vector, list_id_t *list_to_probe, len_t n_results, len_t n_probe) : query_vector(query_vector), list_to_probe(list_to_probe), n_results(n_results), n_probe(n_probe) {}
    vector_el_t *get_query_vector() const { return query_vector; }
    list_id_t get_list_to_probe(len_t i) const { return list_to_probe[i]; }
    len_t get_n_results() const { return n_results; }
    len_t get_n_probe() const { return n_probe; }
    void set_list_to_probe(len_t offset, list_id_t list_id)
    {
      assert(offset < n_probe);
      list_to_probe[offset] = list_id;
    }
    ~Query()
    {
      if (free_list_to_probe)
      {
        delete[] list_to_probe;
      }
    }
  };

  struct QueryResult
  {
    distance_t distance;
    vector_id_t vector_id;
    friend bool operator<(const QueryResult &a, const QueryResult &b)
    {
      bool closer = a.distance < b.distance ||
                    (a.distance == b.distance && a.vector_id < b.vector_id);
      return closer;
    }
  };

  typedef std::vector<Query *> QueryBatch;
  typedef std::vector<QueryResult> QueryResults;
  typedef std::vector<QueryResults> QueryResultsBatch;
} // namespace ann_dkvs
#endif // QUERY_HPP_