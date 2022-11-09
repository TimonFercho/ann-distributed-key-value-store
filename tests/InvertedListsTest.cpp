#include <cmath>

#include "../lib/catch.hpp"

#include "../include/storage-node/InvertedLists.hpp"

using namespace ann_dkvs;
using Catch::Matchers::Contains;

auto round_up_to_next_power_of_two = [](size_t value)
{
  size_t power = 1;
  while (power < value)
  {
    power *= 2;
  }
  return power;
};

auto is_power_of_two = [](size_t value)
{
  return (value & (value - 1)) == 0;
};

auto get_list_size = [](InvertedLists lists, len_t n_entries)
{
  len_t entries_allocated = round_up_to_next_power_of_two(n_entries);
  size_t list_size = entries_allocated * (lists.get_vector_size() + sizeof(vector_id_t));
  return list_size;
};

auto get_total_size = [](InvertedLists lists, size_t used_space)
{
  size_t total_size = max((size_t)32, round_up_to_next_power_of_two(used_space));
  return total_size;
};

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
        size_t used_space = get_list_size(lists, list_length);
        REQUIRE(lists.get_total_size() == get_total_size(lists, used_space));
      }

      AND_WHEN("another list with the same index is created")
      {
        REQUIRE_THROWS_WITH(lists.create_list(1, 5), Contains("already") && Contains("exist"));

        THEN("the number of lists is still 1")
        {
          REQUIRE(lists.get_length() == 1);
        }

        THEN("the first list has the correct length")
        {
          REQUIRE(lists.get_list_length(1) == 5);
        }

        THEN("the total size is still correct")
        {
          size_t used_space = get_list_size(lists, list_length);
          REQUIRE(lists.get_total_size() == get_total_size(lists, used_space));
        }
      }

      AND_WHEN("another list with a different index is created")
      {
        list_id_t list_id2 = 2;
        len_t list_length2 = 5;
        lists.create_list(list_id2, list_length2);

        THEN("the number of lists is 2")
        {
          REQUIRE(lists.get_length() == 2);
        }

        THEN("the first list has the correct length")
        {
          REQUIRE(lists.get_list_length(1) == 5);
        }

        THEN("the second list has the correct length")
        {
          REQUIRE(lists.get_list_length(2) == 5);
        }

        THEN("the total size is correct")
        {
          size_t used_space = get_list_size(lists, list_length) + get_list_size(lists, list_length2);
          REQUIRE(lists.get_total_size() == get_total_size(lists, used_space));
        }

        THEN("the free space is correct")
        {
          size_t used_space = get_list_size(lists, list_length) + get_list_size(lists, list_length2);
          size_t total_size = get_total_size(lists, used_space);
          REQUIRE(lists.get_free_space() == total_size - used_space);
        }
      }
    }

    WHEN("a list of size < 32B is created")
    {
      list_id_t list_id = 1;
      len_t list_length = 1;
      lists.create_list(list_id, list_length);

      size_t list_size = get_list_size(lists, list_length);

      CHECK(list_size < 32);

      THEN("the total size is 32B")
      {
        REQUIRE(lists.get_total_size() == 32);
      }

      THEN("the number of lists is 1")
      {
        REQUIRE(lists.get_length() == 1);
      }

      THEN("the list length is correct")
      {
        REQUIRE(lists.get_list_length(list_id) == list_length);
      }
    }

    WHEN("a list of length 0 is created")
    {
      list_id_t list_id = 1;
      len_t list_length = 0;

      REQUIRE_THROWS_WITH(lists.create_list(list_id, list_length), Contains("greater than 0"));
    }
  }
}

SCENARIO("the free space of inverted lists can be queried", "[InvertedLists]")
{

  GIVEN("an InvertedLists object storing vectors of dimension 1")
  {
    size_t vector_dim = 1;
    string filename = "lists.bin";
    InvertedLists lists = InvertedLists(vector_dim, filename);

    WHEN("a list is created which has a size != 2^n > 32B")
    {
      list_id_t list_id = 1;
      len_t list_length = 3;
      lists.create_list(list_id, list_length);

      size_t list_size = get_list_size(lists, list_length);

      CHECK(!is_power_of_two(list_size));

      THEN("the free space is the difference between the list size and the next power of two")
      {
        size_t list_size = get_list_size(lists, list_length);
        size_t next_power_of_two = round_up_to_next_power_of_two(list_size);
        REQUIRE(lists.get_free_space() == next_power_of_two - list_size);
      }
    }
  }

  GIVEN("an InvertedLists object storing vectors of dimension 2")
  {
    size_t vector_dim = 2;
    string filename = "lists.bin";
    InvertedLists lists = InvertedLists(vector_dim, filename);

    WHEN("no lists have been created")
    {
      THEN("the free space is 0")
      {
        REQUIRE(lists.get_free_space() == 0);
      }
    }

    WHEN("a list is created which has a size = 2^n > 32B")
    {
      list_id_t list_id = 1;
      len_t list_length = 4;
      lists.create_list(list_id, list_length);

      size_t list_size = get_list_size(lists, list_length);

      CHECK(list_size == 64);
      CHECK(is_power_of_two(list_size));

      THEN("the free space is 0")
      {
        REQUIRE(lists.get_free_space() == 0);
      }
    }

    WHEN("a list is created with size < 32B")
    {
      list_id_t list_id = 1;
      len_t list_length = 1;
      lists.create_list(list_id, list_length);

      size_t list_size = get_list_size(lists, list_length);

      CHECK(list_size < 32);

      THEN("the free space is 32 - list size")
      {
        REQUIRE(lists.get_free_space() == 32 - list_size);
      }
    }
  }
}
