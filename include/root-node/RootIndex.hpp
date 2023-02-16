#ifndef ROOT_INDEX_HPP_
#define ROOT_INDEX_HPP_

#include <vector>
#include <queue>

namespace ann_dkvs
{
  typedef std::pair<distance_t, list_id_t> centroid_distance_id_t;
  typedef std::priority_queue<centroid_distance_id_t> list_id_heap_t;
  class RootIndex
  {
  private:
    vector_el_t *centroids;
    len_t n_centroids;
    std::vector<list_id_t> extract_list_ids(list_id_heap_t *nearest_centroids);

  public:
    RootIndex(vector_el_t *centroids, len_t n_centroids);
    std::vector<list_id_t> find_nearest_centroids(vector_el_t *query_vector, len_t vector_dim, len_t number_of_nearest_centroids);
  };
} // namespace ann_dkvs
#endif // ROOT_INDEX_HPP_