#include "InvertedLists.h"
#include <queue>
#include "distances.h"

using namespace std;

namespace AnnKvs
{
  typedef float distance_t;
  typedef pair<distance_t, vector_id_t> result_t;
  class IndexIVFFlat
  {
    typedef priority_queue<result_t, vector<result_t>, less<distance_t>> heap_t;

  private:
    InvertedLists lists;

    vector<result_t> extract_results(heap_t knn)
    {
      vector<result_t> results;
      while (!knn.empty())
      {
        results.push_back(knn.top());
        knn.pop();
      }
      return results;
    }

    void search_list(
        list_id_t list_id,
        vector_t *query,
        size_t k,
        heap_t knn)
    {
      vector_t *vectors = lists.get_vectors(list_id);
      vector_id_t *ids = lists.get_ids(list_id);
      size_t list_size = lists.get_size(list_id);
      size_t vector_size = lists.get_vector_size();
      size_t vector_element_size = vector_size / sizeof(vector_t);
      for (int j = 0; j < list_size; j++)
      {
        vector_t *vector = vectors + j * vector_element_size;
        float distance = L2Sqr(vector, query, &list_size);
        vector_id_t id = ids[j];
        result_t result = {distance, id};
        knn.push(result);
      }
    }

  public:
    IndexIVFFlat(size_t vector_size, string base_path) : lists(vector_size, base_path) {}

    vector<result_t> search(
        list_id_t *list_ids,
        size_t nlist,
        vector_t *query,
        size_t k)
    {
      heap_t knn;
      for (int i = 0; i < nlist; i++)
      {
        search_list(list_ids[i], query, k, knn);
      }
      return extract_results(knn);
    }
  };
}