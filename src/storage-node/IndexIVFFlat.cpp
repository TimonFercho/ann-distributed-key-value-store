#include "IndexIVFFlat.h"

using namespace std;

namespace ann_dkvs
{
  static float L2Sqr(
      const void *pVect1v,
      const void *pVect2v,
      const void *qty_ptr)
  {
    float *pVect1 = (float *)pVect1v;
    float *pVect2 = (float *)pVect2v;
    size_t qty = *((size_t *)qty_ptr);

    float res = 0;
    for (size_t i = 0; i < qty; i++)
    {
      float t = *pVect1 - *pVect2;
      pVect1++;
      pVect2++;
      res += t * t;
    }
    return (res);
  }

  vector<result_t> IndexIVFFlat::extract_results(heap_t candidates)
  {
    vector<result_t> results;
    while (!candidates.empty())
    {
      results.push_back(candidates.top());
      candidates.pop();
    }
    return results;
  }

  void IndexIVFFlat::search_list(
      list_id_t list_id,
      vector_el_t *query,
      size_t k,
      heap_t candidates)
  {
    vector_el_t *vectors = lists.get_vectors(list_id);
    vector_id_t *ids = lists.get_ids(list_id);
    size_t list_size = lists.get_list_size(list_id);
    size_t vector_size = lists.get_vector_size();
    for (size_t j = 0; j < list_size; j++)
    {
      vector_el_t *vector = vectors + j * vector_size;
      float distance = L2Sqr(vector, query, &list_size);
      vector_id_t id = ids[j];
      result_t result = {distance, id};
      if (candidates.size() < k)
      {
        candidates.push(result);
      }
      else if (distance < candidates.top().first)
      {
        candidates.pop();
        candidates.push(result);
      }
    }
  }

  IndexIVFFlat::IndexIVFFlat(size_t vector_dims)
      : vector_dims(vector_dims),
        lists(vector_dims * sizeof(vector_el_t)) {}

  vector<result_t> IndexIVFFlat::search(
      list_id_t *list_ids,
      size_t nlist,
      vector_el_t *query,
      size_t k)
  {
    heap_t knn;
    for (size_t i = 0; i < nlist; i++)
    {
      search_list(list_ids[i], query, k, knn);
    }
    return extract_results(knn);
  }
}