#include "../lib/catch.hpp"

#include "../include/tests/InvertedListsTestUtils.hpp"
#include "../include/storage-node/Index.hpp"

using namespace ann_dkvs;

SCENARIO("Index can be queried", "[Index]")
{
  GIVEN("an InvertedLists object and a list of 1D vectors, corresponding vector ids and list ids")
  {
    len_t vector_dim = 1;
    InvertedLists lists = get_inverted_lists_object(vector_dim);
    len_t list1_length = 2;
    vector_el_t vectors_list1[] = {2, 4};
    vector_id_t ids_list1[] = {1, 2};

    len_t list2_length = 2;
    vector_el_t vectors_list2[] = {1, 3};
    vector_id_t ids_list2[] = {3, 4};

    list_id_t list_ids[] = {1, 2};

    AND_GIVEN("A query vector, the list ids to search and a value of k")
    {
      vector_el_t query[] = {0};
      size_t k = 3;
      list_id_t list_ids_to_search[] = {1, 2};
      len_t n_lists_to_search = 2;

      WHEN("the InvertedLists object is populated with the vectors, ids and list ids and used to initialize an Index object")
      {
        lists.insert_entries(list_ids[0], vectors_list1, ids_list1, list1_length);
        lists.insert_entries(list_ids[1], vectors_list2, ids_list2, list2_length);

        Index index(&lists);

        WHEN("the Index is queried with a query vector")
        {
          vector<result_t> results = index.search_preassigned(list_ids_to_search, n_lists_to_search, query, k);

          THEN("the results are correct")
          {
            CHECK(results.size() == k);
            CHECK(results[0].first == 1 * 1);
            CHECK(results[0].second == 3);
            CHECK(results[1].first == 2 * 2);
            CHECK(results[1].second == 1);
            CHECK(results[2].first == 3 * 3);
            CHECK(results[2].second == 4);
          }
        }
      }
    }
  }
}