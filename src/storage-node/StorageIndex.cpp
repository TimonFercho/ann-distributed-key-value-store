#include "StorageIndex.hpp"
#include "L2Space.hpp"

namespace ann_dkvs
{

  QueryResults StorageIndex::extract_results(heap_t *candidates)
  {
    len_t n_results = candidates->size();
    QueryResults results(n_results);
    for (size_t i = 0; i < n_results; i++)
    {
      results[n_results - i - 1] = candidates->top();
      candidates->pop();
    }
    return results;
  }

  void StorageIndex::add_candidate(const Query *query, const QueryResult &result, heap_t &candidates)
  {
    if (candidates.size() < query->get_n_results())
    {
      candidates.push(result);
    }
    else if (result < candidates.top())
    {
      candidates.pop();
      candidates.push(result);
    }
  }

  void StorageIndex::search_preassigned_list(
      const Query *query,
      list_id_t list_id,
      heap_t &candidates)
  {
    vector_el_t *vectors = lists->get_vectors(list_id);
    vector_id_t *ids = lists->get_ids(list_id);
    size_t list_size = lists->get_list_length(list_id);
    size_t vector_dim = lists->get_vector_dim();
    for (size_t j = 0; j < list_size; j++)
    {
      vector_el_t *vector = &vectors[j * vector_dim];
      float distance = distance_func(vector, query->get_query_vector(), &vector_dim);
      vector_id_t vector_id = ids[j];
      QueryResult result = {distance, vector_id};
      add_candidate(query, result, candidates);
    }
  }

  StorageIndex::StorageIndex(StorageLists *lists)
      : lists(lists)
  {
    distance_func = L2Space(lists->get_vector_dim()).get_distance_func();
  }

  QueryResults StorageIndex::search_preassigned(Query *query)
  {
    heap_t candidates;
    for (len_t i = 0; i < query->get_n_probe(); i++)
    {
      list_id_t list_id = query->get_list_to_probe(i);
      search_preassigned_list(query, list_id, candidates);
    }
    return extract_results(&candidates);
  }

  QueryListPairs StorageIndex::get_work_items(QueryBatch queries)
  {
    QueryListPairs work_items;
    for (len_t i = 0; i < queries.size(); i++)
    {
      for (len_t j = 0; j < queries[i]->get_n_probe(); j++)
      {
        list_id_t list_id = queries[i]->get_list_to_probe(j);
        work_items.push_back({i, list_id});
      }
    }
    return work_items;
  }
  {
    QueryResultsBatch results(queries.size());

#if PMODE == 1
#pragma omp parallel for
#endif
    for (len_t i = 0; i < queries.size(); i++)
    {
      results[i] = search_preassigned(queries[i]);
    }
    return results;
  }
}