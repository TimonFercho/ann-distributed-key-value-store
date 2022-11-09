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

SCENARIO("InvertedLists(): an InvertedLists object can be constructed", "[InvertedLists]")
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

SCENARIO("create_list(): inverted lists can be created", "[InvertedLists]")
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

      THEN("an exception is thrown")
      {
        REQUIRE_THROWS(lists.create_list(list_id, list_length));
      }
    }
  }
}

SCENARIO("get_free_space(): the free space of an InvertedLists object is as expected", "[InvertedLists]")
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

SCENARIO("update_entries(): multiple entries of a list can be updated", "[InvertedLists]")
{
  GIVEN("an InvertedLists object and two lists of 1D vectors and corresponding ids")
  {
    size_t vector_dim = 1;
    string filename = "lists.bin";
    InvertedLists lists = InvertedLists(vector_dim, filename);
    vector_el_t vectors[5] = {6.0, 7.0, 8.0, 9.0, 10.0};
    list_id_t ids[5] = {1, 2, 3, 4, 5};
    vector_el_t vectors2[5] = {11.0, 12.0, 13.0, 14.0, 15.0};
    list_id_t ids2[5] = {6, 7, 8, 9, 10};

    WHEN("an inverted list is created")
    {
      list_id_t list_id = 1;
      len_t list_length = 5;
      lists.create_list(list_id, list_length);
      size_t total_size_before_update = lists.get_total_size();

      AND_WHEN("all entries of the list are updated with the vectors and ids of the first list")
      {
        lists.update_entries(list_id, vectors, ids, 0, list_length);

        THEN("all vectors are updated")
        {
          vector_el_t *list_vectors = lists.get_vectors(list_id);
          for (len_t i = 0; i < list_length; i++)
          {
            REQUIRE(list_vectors[i] == vectors[i]);
          }
        }

        THEN("all ids are updated")
        {
          list_id_t *list_ids = lists.get_ids(list_id);
          for (len_t i = 0; i < list_length; i++)
          {
            REQUIRE(list_ids[i] == ids[i]);
          }
        }

        THEN("the list length is unaffected")
        {
          REQUIRE(lists.get_list_length(list_id) == list_length);
        }

        THEN("the total size is unaffected")
        {
          REQUIRE(lists.get_total_size() == total_size_before_update);
        }

        AND_WHEN("only the first few entries are updated with the second list")
        {
          len_t update_length = 3;
          lists.update_entries(list_id, vectors2, ids2, 0, update_length);

          THEN("only the first few entries are updated with the vectors from the second list")
          {
            vector_el_t *list_vectors = lists.get_vectors(list_id);
            for (len_t i = 0; i < update_length; i++)
            {
              REQUIRE(list_vectors[i] == vectors2[i]);
            }
            for (len_t i = update_length; i < list_length; i++)
            {
              REQUIRE(list_vectors[i] == vectors[i]);
            }
          }

          THEN("only the first few entries are updated with the ids from the second list")
          {
            list_id_t *list_ids = lists.get_ids(list_id);
            for (len_t i = 0; i < update_length; i++)
            {
              REQUIRE(list_ids[i] == ids2[i]);
            }
            for (len_t i = update_length; i < list_length; i++)
            {
              REQUIRE(list_ids[i] == ids[i]);
            }
          }

          THEN("the list length is unaffected")
          {
            REQUIRE(lists.get_list_length(list_id) == list_length);
          }

          THEN("the total size is unaffected")
          {
            REQUIRE(lists.get_total_size() == total_size_before_update);
          }
        }

        AND_WHEN("only the last few entries are updated with the second list")
        {
          len_t update_length = 3;
          lists.update_entries(list_id, vectors2, ids2, list_length - update_length, update_length);

          THEN("only the last few entries are updated with the vectors from the second list")
          {
            vector_el_t *list_vectors = lists.get_vectors(list_id);
            for (len_t i = 0; i < list_length - update_length; i++)
            {
              REQUIRE(list_vectors[i] == vectors[i]);
            }
            for (len_t i = list_length - update_length; i < list_length; i++)
            {
              REQUIRE(list_vectors[i] == vectors2[i - (list_length - update_length)]);
            }
          }

          THEN("only the last few entries are updated with the ids from the second list")
          {
            list_id_t *list_ids = lists.get_ids(list_id);
            for (len_t i = 0; i < list_length - update_length; i++)
            {
              REQUIRE(list_ids[i] == ids[i]);
            }
            for (len_t i = list_length - update_length; i < list_length; i++)
            {
              REQUIRE(list_ids[i] == ids2[i - (list_length - update_length)]);
            }
          }

          THEN("the list length is unaffected")
          {
            REQUIRE(lists.get_list_length(list_id) == list_length);
          }

          THEN("the total size is unaffected")
          {
            REQUIRE(lists.get_total_size() == total_size_before_update);
          }
        }
      }
    }
  }
}
