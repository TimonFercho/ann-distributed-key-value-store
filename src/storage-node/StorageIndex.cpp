#include "StorageIndex.hpp"
#include "L2Space.hpp"

namespace ann_dkvs
{

  QueryResults StorageIndex::extract_results(heap_t &candidates) const
  {
    len_t n_results = candidates.size();
    QueryResults results(n_results);
    for (size_t i = 0; i < n_results; i++)
    {
      results[n_results - i - 1] = candidates.top();
      candidates.pop();
    }
    return results;
  }

  void StorageIndex::add_candidate(const Query *query, const QueryResult &result, heap_t &candidates) const
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
      const list_id_t list_id,
      heap_t &candidates) const
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

  StorageIndex::StorageIndex(const StorageLists *lists)
      : lists(lists), distance_func(L2Space(lists->get_vector_dim()).get_distance_func())
  {
  }

  QueryResults StorageIndex::search_preassigned(const Query *query) const
  {
    heap_t candidates;
    for (len_t i = 0; i < query->get_n_probe(); i++)
    {
      list_id_t list_id = query->get_list_to_probe(i);
      search_preassigned_list(query, list_id, candidates);
    }
    return extract_results(candidates);
  }

  QueryListPairs StorageIndex::get_work_items(const QueryBatch &queries) const
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

  QueryResultsBatch StorageIndex::batch_search_preassigned(const QueryBatch &queries) const
  {
    QueryResultsBatch results(queries.size());

#if PMODE == 0 || PMODE == 1
#if PMODE != 0
#pragma omp parallel for
#endif
    for (len_t i = 0; i < queries.size(); i++)
    {
      results[i] = search_preassigned(queries[i]);
    }
#elif PMODE == 2
    std::vector<heap_t> candidate_lists(queries.size());

    QueryListPairs work_items = get_work_items(queries);

#pragma omp parallel for
    for (len_t i = 0; i < work_items.size(); i++)
    {
      len_t query_index = work_items[i].first;
      const Query *query = queries[query_index];
      list_id_t list_id = work_items[i].second;
      heap_t local_candidates;
      search_preassigned_list(query, list_id, local_candidates);
#pragma omp critical
      {
        while (local_candidates.size() > 0)
        {
          QueryResult result = local_candidates.top();
          local_candidates.pop();
          add_candidate(query, result, candidate_lists[query_index]);
        }
      }
    }
#pragma omp single
    for (len_t j = 0; j < queries.size(); j++)
    {
      results[j] = extract_results(candidate_lists[j]);
    }
#endif
    return results;
  }
}