#include <vector>
#include <cassert>
#include <iostream>

#include "../include/L2Space.hpp"
#include "../include/root-node/RootIndex.hpp"

namespace ann_dkvs
{
  RootIndex::RootIndex(len_t vector_dim, vector_el_t *centroids, len_t n_centroids)
      : vector_dim(vector_dim), centroids(centroids), n_centroids(n_centroids) {}

  void RootIndex::allocate_list_ids(Query *query, centroids_heap_t *nearest_centroids)
  {
    for (size_t query_id = 0; query_id < query->get_n_probe(); query_id++)
    {
      size_t insertion_index = query->get_n_probe() - query_id - 1;
      list_id_t list_id = nearest_centroids->top().list_id;
      query->set_list_to_probe(insertion_index, list_id);
      nearest_centroids->pop();
    }
  }

  void RootIndex::preassign_query(Query *query)
  {
    centroids_heap_t nearest_centroids;
    distance_func_t distance_func = L2Space(vector_dim).get_distance_func();

    for (list_id_t list_id = 0; list_id < (list_id_t)n_centroids; list_id++)
    {
      vector_el_t *centroid = &centroids[list_id * vector_dim];
      float distance = distance_func(centroid, query->get_query_vector(), &vector_dim);
      CentroidsResult result = {.distance = distance, .list_id = list_id};
      if (nearest_centroids.size() < query->get_n_probe())
      {
        nearest_centroids.push(result);
      }
      else if (distance < nearest_centroids.top().distance || (distance == nearest_centroids.top().distance && list_id < nearest_centroids.top().list_id))
      {
        nearest_centroids.pop();
        nearest_centroids.push(result);
      }
    }
    allocate_list_ids(query, &nearest_centroids);
  }

  void RootIndex::batch_preassign_queries(QueryBatch queries)
  {
#if PMODE == 1
#pragma omp parallel for
#endif
    for (len_t i = 0; i < queries.size(); i++)
    {
      preassign_query(queries[i]);
    }
  }
}