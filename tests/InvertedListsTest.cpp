#include <cmath>

#include "../lib/catch.hpp"

#include "../include/storage-node/InvertedLists.hpp"

using namespace ann_dkvs;
using Catch::Matchers::Contains;

SCENARIO("InvertedLists can be constructed", "[InvertedLists]")
{
  GIVEN("a vector dimension and a filename")
  {
    size_t vector_dim = 1;
    string filename = "lists.bin";

    WHEN("an InvertedLists object is created")
    {
      InvertedLists lists = InvertedLists(vector_dim, filename);

      THEN("the number of lists is 0")
      {
        REQUIRE(lists.get_length() == 0);
      }

      THEN("the vector dimension is correct")
      {
        REQUIRE(lists.get_vector_dim() == vector_dim);
      }

      THEN("the filename is correct")
      {
        REQUIRE(lists.get_filename() == filename);
      }

      THEN("the vector size is correct")
      {
        REQUIRE(lists.get_vector_size() == vector_dim * sizeof(vector_el_t));
      }

      THEN("the total size is correct")
      {
        REQUIRE(lists.get_total_size() == 0);
      }
    }
  }
}

SCENARIO("inverted lists can be created", "[InvertedLists]")
{

  GIVEN("an InvertedLists object")
  {
    size_t vector_dim = 1;
    string filename = "lists.bin";
    InvertedLists lists = InvertedLists(vector_dim, filename);

    WHEN("and a list is created")
    {
      list_id_t list_id = 1;
      len_t list_length = 5;
      lists.create_list(list_id, list_length);

      THEN("the number of lists is 1")
      {
        REQUIRE(lists.get_length() == 1);
      }

      THEN("the list length is correct")
      {
        REQUIRE(lists.get_list_length(list_id) == list_length);
      }

      THEN("the total size is correct")
      {
        auto round_up_to_next_power_of_two = [](size_t value)
        {
          size_t power = 1;
          while (power < value)
          {
            power *= 2;
          }
          return power;
        };
        len_t entries_allocated = round_up_to_next_power_of_two(list_length);
        REQUIRE(entries_allocated == 8);
        size_t list_size = entries_allocated * (lists.get_vector_size() + sizeof(vector_id_t));
        size_t expected_size = max((size_t)32, round_up_to_next_power_of_two(list_size));
        REQUIRE(lists.get_total_size() == expected_size);
      }

      AND_WHEN("another list with the same index is created")
      {
        REQUIRE_THROWS_WITH(lists.create_list(1, 5), Contains("already") && Contains("exist"));

        THEN("the number of lists is still 1")
        {
          REQUIRE(lists.get_length() == 1);
        }

        THEN("the list has the correct length")
        {
          REQUIRE(lists.get_list_length(1) == 5);
        }
      }

      AND_WHEN("another list with a different index is created")
      {
        lists.create_list(2, 5);

        THEN("the number of lists is 2")
        {
          REQUIRE(lists.get_length() == 2);
        }

        THEN("the list has the correct length")
        {
          REQUIRE(lists.get_list_length(2) == 5);
        }
      }
    }
  }
}
