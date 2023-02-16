#include "StorageIndex.hpp"
#include "L2Space.hpp"

namespace ann_dkvs
{

  std::vector<vector_distance_id_t> StorageIndex::extract_results(heap_t *candidates)
  {
    len_t k = candidates->size();
    std::vector<vector_distance_id_t> results(k);
    for (size_t i = 0; i < k; i++)
    {
      results[k - i - 1] = candidates->top();
      candidates->pop();
    }
    return results;
  }

  void StorageIndex::search_preassigned_list(
      list_id_t list_id,
      vector_el_t *query,
      size_t k,
      heap_t *candidates)
  {
    vector_el_t *vectors = lists->get_vectors(list_id);
    vector_id_t *ids = lists->get_ids(list_id);
    size_t list_size = lists->get_list_length(list_id);
    size_t vector_dim = lists->get_vector_dim();
    for (size_t j = 0; j < list_size; j++)
    {
      vector_el_t *vector = &vectors[j * vector_dim];
      float distance = distance_func(vector, query, &vector_dim);
      vector_id_t id = ids[j];
      vector_distance_id_t result = {distance, id};
      if (candidates->size() < k)
      {
        candidates->push(result);
      }
      else if (distance < candidates->top().first || (distance == candidates->top().first && id < candidates->top().second))
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

  std::vector<vector_distance_id_t> StorageIndex::search_preassigned(list_id_t *list_ids, size_t nlist, vector_el_t *query, size_t k)
  {
    heap_t knn;
    for (size_t i = 0; i < nlist; i++)
    {
      search_preassigned_list(list_ids[i], query, k, &knn);
    }
    return extract_results(&knn);
  }
}