#include <vector>
#include "../include/L2Space.hpp"
#include "../include/root-node/RootIndex.hpp"

namespace ann_dkvs
{
  RootIndex::RootIndex(vector_el_t *centroids, len_t n_centroids)
      : centroids(centroids), n_centroids(n_centroids) {}

  std::vector<list_id_t> RootIndex::extract_list_ids(list_id_heap_t *nearest_centroids)
  {
    len_t k = nearest_centroids->size();
    std::vector<list_id_t> results(k);
    for (size_t i = 0; i < k; i++)
    {
      results[k - i - 1] = nearest_centroids->top().second;
      nearest_centroids->pop();
    }
    return results;
  }

  std::vector<list_id_t> RootIndex::find_nearest_centroids(vector_el_t *query_vector, len_t vector_dim, len_t number_of_nearest_centroids)
  {
    list_id_heap_t nearest_centroids;
    distance_func_t distance_func = L2Space(vector_dim).get_distance_func();

    for (list_id_t i = 0; i < (list_id_t)n_centroids; i++)
    {
      vector_el_t *centroid = &centroids[i * vector_dim];
      float distance = distance_func(centroid, query_vector, &vector_dim);
      centroid_distance_id_t result = {distance, i};
      if (nearest_centroids.size() < number_of_nearest_centroids)
      {
        nearest_centroids.push(result);
      }
      else if (distance < nearest_centroids.top().first || (distance == nearest_centroids.top().first && i < nearest_centroids.top().second))
      {
        nearest_centroids.pop();
        nearest_centroids.push(result);
      }
    }
    return extract_list_ids(&nearest_centroids);
  }
}