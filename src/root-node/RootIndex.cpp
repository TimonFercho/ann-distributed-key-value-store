#include <vector>
#include "../include/L2Space.hpp"
#include "../include/root-node/RootIndex.hpp"

namespace ann_dkvs
{
  RootIndex::RootIndex(len_t vector_dim, vector_el_t *centroids, len_t n_centroids)
      : vector_dim(vector_dim), centroids(centroids), n_centroids(n_centroids) {}

  list_ids_t RootIndex::extract_list_ids(centroids_heap_t *nearest_centroids)
  {
    len_t n_probe = nearest_centroids->size();
    list_ids_t results(n_probe);
    for (size_t i = 0; i < n_probe; i++)
    {
      results[n_probe - i - 1] = nearest_centroids->top().list_id;
      nearest_centroids->pop();
    }
    return results;
  }

  list_ids_t RootIndex::preassign_query(Query *query)
  {
    centroids_heap_t nearest_centroids;
    distance_func_t distance_func = L2Space(vector_dim).get_distance_func();

    for (list_id_t list_id = 0; list_id < (list_id_t)n_centroids; list_id++)
    {
      vector_el_t *centroid = &centroids[list_id * vector_dim];
      float distance = distance_func(centroid, query->query_vector, &vector_dim);
      CentroidsResult result = {.distance = distance, .list_id = list_id};
      if (nearest_centroids.size() < query->n_probe)
      {
        nearest_centroids.push(result);
      }
      else if (distance < nearest_centroids.top().distance || (distance == nearest_centroids.top().distance && list_id < nearest_centroids.top().list_id))
      {
        nearest_centroids.pop();
        nearest_centroids.push(result);
      }
    }
    return extract_list_ids(&nearest_centroids);
  }
}