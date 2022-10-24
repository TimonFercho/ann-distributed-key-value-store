#include <IndexIVFFlat.h>

namespace AnnKvs
{
  class StorageNode
  {
  private:
    IndexIVFFlat index;

  public:
    StorageNode(size_t vector_dims, string base_path) : index(vector_dims, base_path) {}

    vector<result_t> receive_knn_request(
        list_id_t *list_ids,
        size_t nlist,
        vector_el_t *query,
        size_t k)
    {
      vector<result_t> results = index.search(list_ids, nlist, query, k);
      return results;
    }
  };
}