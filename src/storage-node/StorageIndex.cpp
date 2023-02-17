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

  void StorageIndex::search_preassigned_list(
      Query *query,
      list_id_t list_id,
      heap_t *candidates)
  {
    vector_el_t *vectors = lists->get_vectors(list_id);
    vector_id_t *ids = lists->get_ids(list_id);
    size_t list_size = lists->get_list_length(list_id);
    size_t vector_dim = lists->get_vector_dim();
    for (size_t j = 0; j < list_size; j++)
    {
      vector_el_t *vector = &vectors[j * vector_dim];
      float distance = distance_func(vector, query->query_vector, &vector_dim);
      vector_id_t vector_id = ids[j];
      QueryResult result = {distance, vector_id};
      if (candidates->size() < query->n_results)
      {
        candidates->push(result);
      }
      else if (distance < candidates->top().distance || (distance == candidates->top().distance && vector_id < candidates->top().vector_id))
      {
        candidates->pop();
        candidates->push(result);
      }
    }
  }

  StorageIndex::StorageIndex(InvertedLists *lists)
      : lists(lists)
  {
    distance_func = L2Space(lists->get_vector_dim()).get_distance_func();
  }

  QueryResults StorageIndex::search_preassigned(Query *query)
  {
    heap_t knn;
    for (size_t i = 0; i < query->n_probe; i++)
    {
      search_preassigned_list(query, query->list_ids[i], &knn);
    }
    return extract_results(&knn);
  }
}