#include <cmath>
#include <iostream>

#include "../lib/catch.hpp"

#include "../include/storage-node/InvertedLists.hpp"

using namespace ann_dkvs;
using Catch::Matchers::Contains;
using namespace std;

#define FILE_NAME "test_lists.bin"

#define MAX_VECTOR_DIM 1
#define MAX_LIST_ID 10000
#define MAX_VECTOR_ID 10000
#define MAX_LIST_LENGTH 10000
#define MIN_VECTOR_VAL -1E10F
#define MAX_VECTOR_VAL 1E10F

#define N_VECTOR_DIMS 15
#define N_LIST_IDS 15
#define N_LIST_LENGTHS 15
#define N_VECTORS 5

#define gen_random_values(T, MIN_VAL, MAX_VAL, N_CHUNKS, CHUNK_LEN, EXCLUDE_SET) (GENERATE(take(N_CHUNKS, chunk(CHUNK_LEN, map([](T val) { return (T)val; }, filter([&](T val) {vector<T> exclude{EXCLUDE_SET};return find(exclude.begin(), exclude.end(), val) == exclude.end(); }, random((int)MIN_VAL, (int)MAX_VAL)))))))

#define gen_ranged_values(T, MIN_VAL, MAX_VAL, N_CHUNKS, CHUNK_LEN, EXCLUDE_SET) (GENERATE(take(N_CHUNKS, chunk(CHUNK_LEN, map([](T val) { return (T)val; }, filter([&](T val) {vector<T> exclude{EXCLUDE_SET};return find(exclude.begin(), exclude.end(), val) == exclude.end(); }, range(MIN_VAL, MAX_VAL)))))))

#define gen_random_value(T, MIN_VAL, MAX_VAL, N_VALS, EXCLUDE_SET) (gen_random_values(T, MIN_VAL, MAX_VAL, N_VALS, 1, EXCLUDE_SET)[0])

#define to_ptr(T, V) ((T *)V.data())

#define gen_vector_dim(EXCLUDE_SET) (gen_random_value(len_t, 0, MAX_VECTOR_DIM, N_VECTOR_DIMS, EXCLUDE_SET))

#define gen_list_ids(CHUNK_LEN) gen_ranged_values(list_id_t, 0, MAX_LIST_ID, N_LIST_IDS, CHUNK_LEN, {})

#define gen_list_id() gen_list_ids(1)[0]

#define gen_list_lengths(CHUNK_LEN, EXCLUDE_SET) gen_random_values(len_t, 0, MAX_LIST_LENGTH, N_LIST_LENGTHS, CHUNK_LEN, EXCLUDE_SET)

#define gen_list_lengths_random_length() gen_list_lengths(random_range(1, N_LIST_LENGTHS), {})

#define gen_list_length(EXCLUDE_SET) gen_list_lengths(1, EXCLUDE_SET)[0]

#define gen_vector_ids_fixed(CHUNK_LEN) gen_ranged_values(vector_id_t, 0, MAX_VECTOR_ID, N_VECTORS, CHUNK_LEN, {})

#define gen_random_vector_values_fixed(CHUNK_LEN) gen_random_values(vector_el_t, MIN_VECTOR_VAL, MAX_VECTOR_VAL, N_VECTORS, CHUNK_LEN, {})

#define gen_vectors_fixed(CHUNK_LEN, DIM) make_pair(gen_random_vector_values_fixed((CHUNK_LEN) * (DIM)), gen_vector_ids_fixed(CHUNK_LEN))

#define random_range(MIN, MAX) (rand() % (MAX - MIN + 1) + MIN)

#define gen_vectors(dim) gen_vectors_fixed(random_range(1, MAX_LIST_LENGTH), dim)

#define get_clustered_vectors_length(data, list_ids, dim) (min(get_vector_length(data, dim), list_ids.size()))

#define get_vector_length(data, dim) (min(data.first.size() / dim, data.second.size()))

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

auto get_vector_size = [](len_t vector_dim)
{
  return vector_dim * sizeof(vector_el_t);
};

auto get_list_size = [](len_t vector_dim, len_t n_entries)
{
  len_t entries_allocated = round_up_to_next_power_of_two(n_entries);
  size_t list_size = entries_allocated * (get_vector_size(vector_dim) + sizeof(vector_id_t));
  return list_size;
};

auto get_total_size = [](size_t used_space)
{
  size_t total_size = max((size_t)32, round_up_to_next_power_of_two(used_space));
  return total_size;
};

auto write_to_file = [](string filename, void *data, size_t size)
{
  FILE *file = fopen(filename.c_str(), "w");
  fwrite(data, size, 1, file);
  fclose(file);
};

auto get_inverted_lists_object(len_t vector_dim)
{
  remove(FILE_NAME);
  InvertedLists lists(vector_dim, FILE_NAME);
  return lists;
};

template <typename T>
bool is_unique(vector<T> vec)
{
  sort(vec.begin(), vec.end());
  return unique(vec.begin(), vec.end()) == vec.end();
}

auto print_vector = [](vector_el_t *vector, len_t vector_dim, len_t list_length)
{
  for (len_t i = 0; i < list_length; i++)
  {
    cout << " | ";
    for (len_t j = 0; j < vector_dim; j++)
    {
      cout << vector[i * vector_dim + j] << " ";
    }
  }
  cout << endl;
};

auto are_vectors_equal = [](vector_el_t *actual, vector_el_t *expected, len_t vector_dim, len_t n_entries)
{
  for (len_t i = 0; i < n_entries * vector_dim; i++)
  {
    REQUIRE(actual[i] == expected[i]);
  }
};

auto are_ids_equal = [](vector_id_t *actual, vector_id_t *expected, len_t n_entries)
{
  for (len_t i = 0; i < n_entries; i++)
  {
    REQUIRE(actual[i] == expected[i]);
  }
};

SCENARIO("InvertedLists(): an InvertedLists object can be constructed", "[.InvertedLists]")
{
  GIVEN("a nonzero vector dimension")
  {
    len_t vector_dim = gen_vector_dim({0});

    CHECK(vector_dim > 0);

    WHEN("an InvertedLists object is created")
    {
      InvertedLists lists = get_inverted_lists_object(vector_dim);

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
        REQUIRE(lists.get_filename() == FILE_NAME);
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
  GIVEN("the vector dimension 0")
  {
    len_t vector_dim = 0;

    WHEN("an InvertedLists object is created")
    {
      THEN("an exception is thrown")
      {
        REQUIRE_THROWS_AS(get_inverted_lists_object(vector_dim), out_of_range);
      }
    }
  }
}

SCENARIO("create_list(): inverted lists can be created", "[.InvertedLists]")
{

  GIVEN("an InvertedLists object storing vectors of some dimension")
  {
    len_t vector_dim = gen_vector_dim({0});
    InvertedLists lists = get_inverted_lists_object(vector_dim);

    WHEN("a list of length 0 is created")
    {
      len_t list_id = gen_list_id();
      len_t list_length = 0;
      THEN("an exception is thrown")
      {
        REQUIRE_THROWS_AS(lists.create_list(list_id, list_length), out_of_range);
      }
    }

    WHEN("a list of nonzero length is created")
    {
      auto list_ids = gen_list_ids(2);
      auto list_lengths = gen_list_lengths(2, {0});
      CHECK(list_ids[0] != list_ids[1]);
      CHECK(list_lengths[0] > 0);
      CHECK(list_lengths[1] > 0);

      lists.create_list(list_ids[0], list_lengths[0]);

      THEN("the number of lists is 1")
      {
        REQUIRE(lists.get_length() == 1);
      }

      THEN("the list length is correct")
      {
        REQUIRE(lists.get_list_length(list_ids[0]) == list_lengths[0]);
      }

      THEN("the total size is correct")
      {
        size_t used_space = get_list_size(vector_dim, list_lengths[0]);
        REQUIRE(lists.get_total_size() == get_total_size(used_space));
      }

      AND_WHEN("another list of nonzero length with the same index is created")
      {

        THEN("an exception is thrown")
        {
          REQUIRE_THROWS_AS(lists.create_list(list_ids[0], list_lengths[1]), invalid_argument);
        }

        THEN("the number of lists is still 1")
        {
          REQUIRE(lists.get_length() == 1);
        }

        THEN("the first list has the correct length")
        {
          REQUIRE(lists.get_list_length(list_ids[0]) == list_lengths[0]);
        }

        THEN("the total size is still correct")
        {
          size_t used_space = get_list_size(vector_dim, list_lengths[0]);
          REQUIRE(lists.get_total_size() == get_total_size(used_space));
        }
      }

      AND_WHEN("another list of nonzero length with a different index is created")
      {
        lists.create_list(list_ids[1], list_lengths[1]);

        THEN("the number of lists is 2")
        {
          REQUIRE(lists.get_length() == 2);
        }

        THEN("the first list has the correct length")
        {
          REQUIRE(lists.get_list_length(list_ids[0]) == list_lengths[0]);
        }

        THEN("the second list has the correct length")
        {
          REQUIRE(lists.get_list_length(list_ids[1]) == list_lengths[1]);
        }

        THEN("the total size is correct")
        {
          size_t used_space = get_list_size(vector_dim, list_lengths[0]) + get_list_size(vector_dim, list_lengths[1]);
          REQUIRE(lists.get_total_size() == get_total_size(used_space));
        }

        THEN("the free space is correct")
        {
          size_t used_space = get_list_size(vector_dim, list_lengths[0]) + get_list_size(vector_dim, list_lengths[1]);
          size_t total_size = get_total_size(used_space);
          REQUIRE(lists.get_free_space() == total_size - used_space);
        }
      }
    }
    WHEN("a list of size 0 is created")
    {
      list_id_t list_id = 1;
      len_t list_length = 0;

      THEN("an exception is thrown")
      {
        REQUIRE_THROWS_AS(lists.create_list(list_id, list_length), out_of_range);
      }
    }

    WHEN("a list of length 0 is created")
    {
      list_id_t list_id = 1;
      len_t list_length = 0;

      THEN("an exception is thrown")
      {
        REQUIRE_THROWS_AS(lists.create_list(list_id, list_length), out_of_range);
      }
    }
  }
  GIVEN("an InvertedLists object storing vectors of dimension 1")
  {
    size_t vector_dim = 1;
    InvertedLists lists = get_inverted_lists_object(vector_dim);

    WHEN("a list of size < 32B is created")
    {
      list_id_t list_id = 1;
      len_t list_length = 1;
      lists.create_list(list_id, list_length);

      size_t list_size = get_list_size(vector_dim, list_length);

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
  }
}

SCENARIO("get_free_space(): the free space of an InvertedLists object is as expected", "[InvertedLists]")
{

  GIVEN('an InvertedLists object storing vectors of some dimension')
  {
    len_t vector_dim = gen_vector_dim({0});
    InvertedLists lists = get_inverted_lists_object(vector_dim);

    WHEN("no lists have been created")
    {
      THEN("the free space is 0")
      {
        REQUIRE(lists.get_free_space() == 0);
      }
    }
  }

  GIVEN("an InvertedLists object storing vectors of some dimension > 2")
  {
    len_t vector_dim = gen_vector_dim(vector<len_t>({0, 1, 2}));
    InvertedLists lists = get_inverted_lists_object(vector_dim);

    WHEN("a list is created which has a size > 32B")
    {
      list_id_t list_id = gen_list_id();
      len_t list_length = gen_list_length();
      lists.create_list(list_id, list_length);

      THEN("the free space is the difference between the list size and the next power of two")
      {
        len_t list_size = get_list_size(vector_dim, list_length);
        len_t next_power_of_two = round_up_to_next_power_of_two(list_size);
        REQUIRE(lists.get_free_space() == next_power_of_two - list_size);
      }
    }
  }

  GIVEN("an InvertedLists object storing vectors of dimension 2")
  {
    len_t vector_dim = 2;
    InvertedLists lists = get_inverted_lists_object(vector_dim);

    WHEN("a list is created with size < 32B")
    {
      list_id_t list_id = gen_list_id();
      len_t list_length = 1;
      lists.create_list(list_id, list_length);

      len_t list_size = get_list_size(vector_dim, list_length);

      CHECK(list_size < 32);

      THEN("the free space is 32 - list size")
      {
        REQUIRE(lists.get_free_space() == 32 - list_size);
      }
    }
  }
}

SCENARIO("update_entries(): multiple entries of a list can be updated", "[.InvertedLists]")
{
  GIVEN("an InvertedLists object and two lists of 128D vectors and corresponding ids")
  {
    size_t vector_dim = 128;
    InvertedLists lists = get_inverted_lists_object(vector_dim);

    auto data = gen_vectors(128);
    len_t list_length = get_vector_length(data, vector_dim);
    vector_el_t *vectors = to_ptr(vector_el_t, data.first);
    vector_id_t *ids = to_ptr(vector_id_t, data.second);

    REQUIRE(list_length == min(data.first.size() / vector_dim, data.second.size()));

    auto data2 = gen_vectors(128);
    len_t list_length2 = get_vector_length(data2, vector_dim);
    vector_el_t *vectors2 = to_ptr(vector_el_t, data2.first);
    vector_id_t *ids2 = to_ptr(vector_id_t, data2.second);

    REQUIRE(list_length2 == min(data2.first.size() / vector_dim, data2.second.size()));

    WHEN("an inverted list is created")
    {
      list_id_t list_id = gen_list_id();
      lists.create_list(list_id, list_length);
      size_t total_size_before_update = lists.get_total_size();

      AND_WHEN("all entries of the list are updated with the vectors and ids of the first list")
      {
        lists.update_entries(list_id, vectors, ids, 0, list_length);

        THEN("all vectors are updated")
        {
          vector_el_t *list_vectors = lists.get_vectors(list_id);
          for (len_t i = 0; i < list_length * vector_dim; i++)
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
          len_t update_length = random_range(1, min(list_length, list_length2));

          REQUIRE(update_length <= list_length);
          REQUIRE(update_length <= list_length2);

          lists.update_entries(list_id, vectors2, ids2, 0, update_length);

          THEN("only the first few entries are updated with the vectors from the second list")
          {
            vector_el_t *list_vectors = lists.get_vectors(list_id);

            are_vectors_equal(list_vectors, vectors2, vector_dim, update_length);

            are_vectors_equal(list_vectors + update_length * vector_dim, vectors + update_length * vector_dim, vector_dim, list_length - update_length);
          }

          THEN("only the first few entries are updated with the ids from the second list")
          {
            list_id_t *list_ids = lists.get_ids(list_id);

            are_ids_equal(list_ids, ids2, update_length);

            are_ids_equal(list_ids + update_length, ids + update_length, list_length - update_length);
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
          len_t update_length = random_range(1, min(list_length, list_length2));

          REQUIRE(update_length <= list_length);
          REQUIRE(update_length <= list_length2);

          lists.update_entries(list_id, vectors2, ids2, list_length - update_length, update_length);

          THEN("only the last few entries are updated with the vectors from the second list")
          {
            vector_el_t *list_vectors = lists.get_vectors(list_id);

            are_vectors_equal(list_vectors, vectors, vector_dim, list_length - update_length);

            are_vectors_equal(list_vectors + (list_length - update_length) * vector_dim, vectors2, vector_dim, update_length);
          }

          THEN("only the last few entries are updated with the ids from the second list")
          {
            list_id_t *list_ids = lists.get_ids(list_id);

            are_ids_equal(list_ids, ids, list_length - update_length);

            are_ids_equal(list_ids + (list_length - update_length), ids2, update_length);
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

      AND_WHEN("the list is updated with more entries than it has")
      {
        len_t update_length = list_length + 1;

        THEN("an exception is thrown")
        {
          REQUIRE_THROWS_AS(lists.update_entries(list_id, vectors, ids, 0, update_length), out_of_range);
        }
      }

      AND_WHEN("a list is updated which does not exist")
      {
        list_id_t list_id2 = list_id + 1;
        len_t update_length = 5;
        THEN("an exception is thrown")
        {
          REQUIRE_THROWS_AS(lists.update_entries(list_id2, vectors, ids, 0, update_length), invalid_argument);
        }
      }
    }
  }
}

SCENARIO("insert_entries(): entries can be appended to an inverted list", "[InvertedLists]")
{
  GIVEN("an InvertedLists object and two lists of 1D vectors and corresponding ids")
  {
    size_t vector_dim = 128;
    InvertedLists lists = get_inverted_lists_object(vector_dim);

    auto data = gen_vectors(128);
    len_t list1_length = get_vector_length(data, vector_dim);
    vector_el_t *vectors = to_ptr(vector_el_t, data.first);
    vector_id_t *ids = to_ptr(vector_id_t, data.second);

    auto data2 = gen_vectors(128);
    len_t list2_length = get_vector_length(data2, vector_dim);
    vector_el_t *vectors2 = to_ptr(vector_el_t, data2.first);
    vector_id_t *ids2 = to_ptr(vector_id_t, data2.second);

    len_t total_list_length = list1_length + list2_length;

    WHEN("an inverted list is created")
    {
      list_id_t list_id = gen_list_id();
      lists.create_list(list_id, list1_length);
      size_t total_size_before_update = lists.get_total_size();

      AND_WHEN("all entries of the list are updated with the vectors and ids of the first list")
      {
        lists.update_entries(list_id, vectors, ids, 0, list1_length);

        THEN("all vectors are updated")
        {
          vector_el_t *list_vectors = lists.get_vectors(list_id);
          are_vectors_equal(list_vectors, vectors, vector_dim, list1_length);
        }

        THEN("all ids are updated")
        {
          list_id_t *list_ids = lists.get_ids(list_id);
          are_ids_equal(list_ids, ids, list1_length);
        }

        THEN("the list length is unaffected")
        {
          REQUIRE(lists.get_list_length(list_id) == list1_length);
        }

        THEN("the total size is unaffected")
        {
          REQUIRE(lists.get_total_size() == total_size_before_update);
        }

        AND_WHEN("the list is appended with the entries of the second list")
        {
          lists.insert_entries(list_id, vectors2, ids2, list2_length);

          vector_el_t *list_vectors = lists.get_vectors(list_id);

          THEN("all vectors of the first list are still present")
          {
            are_vectors_equal(list_vectors, vectors, vector_dim, list1_length);
          }

          THEN("all vectors of the second list are appended")
          {
            are_vectors_equal(list_vectors + list1_length * vector_dim, vectors2, vector_dim, list2_length);
          }

          list_id_t *list_ids = lists.get_ids(list_id);

          THEN("all ids of the first list are still present")
          {
            are_ids_equal(list_ids, ids, list1_length);
          }

          THEN("all ids of the second list are correctly appended")
          {
            are_ids_equal(list_ids + list1_length, ids2, list2_length);
          }

          THEN("the list length is updated")
          {
            REQUIRE(lists.get_list_length(list_id) == total_list_length);
          }

          THEN("the total size is updated")
          {
            size_t list_size_after_append = get_list_size(vector_dim, total_list_length);
            size_t total_size_after_append = get_total_size(list_size_after_append);
            REQUIRE(lists.get_total_size() == total_size_after_append);
          }
        }
      }
    }
  }
}

SCENARIO("resize_list(): an inverted list can be resized", "[.InvertedLists]")
{
  GIVEN("an InvertedLists object and a list of 1D vectors and corresponding ids")
  {
    size_t vector_dim = 1;
    InvertedLists lists = get_inverted_lists_object(vector_dim);

    auto data = gen_vectors(1);
    len_t list_length = get_vector_length(data, vector_dim);
    vector_el_t *vectors = to_ptr(vector_el_t, data.first);
    vector_id_t *ids = to_ptr(vector_id_t, data.second);

    WHEN("an inverted list is created")
    {
      list_id_t list_id = 1;
      lists.create_list(list_id, list_length);

      AND_WHEN("all entries of the list are updated with the vectors and ids of the first list")
      {
        lists.update_entries(list_id, vectors, ids, 0, list_length);
        size_t total_size_before_resizing = lists.get_total_size();

        if (list_length >= 2)
        {
          AND_WHEN("the list is resized to a smaller size")
          {
            len_t new_list_length = random_range(1, list_length - 1);

            REQUIRE(new_list_length < list_length);
            lists.resize_list(list_id, new_list_length);

            vector_el_t *list_vectors = lists.get_vectors(list_id);
            THEN("all vectors of the first list are unchanged")
            {
              for (len_t i = 0; i < new_list_length; i++)
              {
                REQUIRE(list_vectors[i] == vectors[i]);
              }
            }
            list_id_t *list_ids = lists.get_ids(list_id);
            THEN("all ids of the first list are unchanged")
            {
              for (len_t i = 0; i < new_list_length; i++)
              {
                REQUIRE(list_ids[i] == ids[i]);
              }
            }
            THEN("the list length is updated")
            {
              REQUIRE(lists.get_list_length(list_id) == new_list_length);
            }

            THEN("the total size is unaffected")
            {
              REQUIRE(lists.get_total_size() == total_size_before_resizing);
            }
          }
        }
        if (list_length < MAX_LIST_LENGTH)
        {
          AND_THEN("the list is resized to a larger size")
          {
            len_t new_list_length = random_range(list_length + 1, MAX_LIST_LENGTH);
            REQUIRE(new_list_length > list_length);
            lists.resize_list(list_id, new_list_length);

            vector_el_t *list_vectors = lists.get_vectors(list_id);
            THEN("all vectors of the first list are unchanged")
            {
              for (len_t i = 0; i < list_length; i++)
              {
                REQUIRE(list_vectors[i] == vectors[i]);
              }
            }
            list_id_t *list_ids = lists.get_ids(list_id);
            THEN("all ids of the first list are unchanged")
            {
              for (len_t i = 0; i < list_length; i++)
              {
                REQUIRE(list_ids[i] == ids[i]);
              }
            }
            THEN("the list length is updated")
            {
              REQUIRE(lists.get_list_length(list_id) == new_list_length);
            }

            THEN("the total size is updated")
            {
              size_t list_size_after_resize = get_list_size(vector_dim, new_list_length);
              size_t total_size_after_resize = get_total_size(list_size_after_resize);
              REQUIRE(lists.get_total_size() == total_size_after_resize);
            }
          }
        }
        AND_WHEN("the list is resized to the same size")
        {
          len_t new_list_length = list_length;
          lists.resize_list(list_id, new_list_length);

          vector_el_t *list_vectors = lists.get_vectors(list_id);
          THEN("all vectors of the first list are unchanged")
          {
            for (len_t i = 0; i < list_length; i++)
            {
              REQUIRE(list_vectors[i] == vectors[i]);
            }
          }
          list_id_t *list_ids = lists.get_ids(list_id);
          THEN("all ids of the first list are unchanged")
          {
            for (len_t i = 0; i < list_length; i++)
            {
              REQUIRE(list_ids[i] == ids[i]);
            }
          }
          THEN("the list length is updated")
          {
            REQUIRE(lists.get_list_length(list_id) == new_list_length);
          }

          THEN("the total size is unaffected")
          {
            REQUIRE(lists.get_total_size() == total_size_before_resizing);
          }
        }
        AND_WHEN("the list is resized to a size of 0")
        {
          len_t new_list_length = 0;

          THEN("an expected exception is thrown")
          {
            REQUIRE_THROWS_AS(lists.resize_list(list_id, new_list_length), out_of_range);
          }
        }
      }
      AND_WHEN("a list is resized which does not exist")
      {
        len_t new_list_length = 10;
        list_id_t non_existing_list_id = 2;
        THEN("an exception is thrown")
        {
          REQUIRE_THROWS_AS(lists.resize_list(non_existing_list_id, new_list_length), invalid_argument);
        }
      }
    }
  }
}

SCENARIO("get_list_length(): the length of an inverted list can be retrieved", "[InvertedLists]")
{
  GIVEN("an InvertedLists object of 1D vectors")
  {
    size_t vector_dim = 128;
    InvertedLists lists = get_inverted_lists_object(vector_dim);

    WHEN("an inverted list is created")
    {
      len_t list_length = gen_list_length();
      auto list_ids = gen_list_ids(2);
      lists.create_list(list_ids[0], list_length);

      THEN("the list length is correct")
      {
        REQUIRE(lists.get_list_length(list_ids[0]) == list_length);
      }

      AND_WHEN("the length of a list is retrieved which does not exist")
      {
        THEN("an exception is thrown")
        {
          REQUIRE_THROWS_AS(lists.get_list_length(list_ids[1]), invalid_argument);
        }
      }
    }
  }
}

SCENARIO("get_vectors(): the vectors of an inverted list can be retrieved", "[.InvertedLists]")
{
  GIVEN("an InvertedLists object and a list of 128D vectors and corresponding ids")
  {
    size_t vector_dim = 128;
    InvertedLists lists = get_inverted_lists_object(vector_dim);

    auto data = gen_vectors(128);
    len_t list_length = get_vector_length(data, vector_dim);
    vector_el_t *vectors = to_ptr(vector_el_t, data.first);
    vector_id_t *ids = to_ptr(vector_id_t, data.second);

    WHEN("an inverted list is created")
    {
      auto list_ids = gen_list_ids(2);
      lists.create_list(list_ids[0], list_length);

      AND_WHEN("all entries of the list are updated with the vectors and ids of the first list")
      {
        lists.update_entries(list_ids[0], vectors, ids, 0, list_length);

        vector_el_t *list_vectors = lists.get_vectors(list_ids[0]);
        THEN("all vectors of the first list are correctly retrieved")
        {
          for (len_t i = 0; i < list_length; i++)
          {
            REQUIRE(list_vectors[i] == vectors[i]);
          }
        }

        AND_WHEN("the vectors of a list are retrieved which does not exist")
        {
          THEN("an exception is thrown")
          {
            REQUIRE_THROWS_AS(lists.get_vectors(list_ids[1]), invalid_argument);
          }
        }
      }
    }
  }
}

SCENARIO("get_ids(): the ids of an inverted list can be retrieved", "[InvertedLists]")
{
  GIVEN("an InvertedLists object and a list of 128D vectors and corresponding ids")
  {
    size_t vector_dim = 128;
    InvertedLists lists = get_inverted_lists_object(vector_dim);

    auto data = gen_vectors(128);
    len_t list_length = get_vector_length(data, vector_dim);
    vector_el_t *vectors = to_ptr(vector_el_t, data.first);
    vector_id_t *ids = to_ptr(vector_id_t, data.second);

    WHEN("an inverted list is created")
    {
      auto list_ids = gen_list_ids(2);
      lists.create_list(list_ids[0], list_length);

      AND_WHEN("all entries of the list are updated with the vectors and ids of the first list")
      {
        lists.update_entries(list_ids[0], vectors, ids, 0, list_length);

        list_id_t *actual_ids = lists.get_ids(list_ids[0]);
        THEN("all ids of the first list are correctly retrieved")
        {
          for (len_t i = 0; i < list_length; i++)
          {
            REQUIRE(actual_ids[i] == ids[i]);
          }
        }

        AND_WHEN("the ids of a list are retrieved which does not exist")
        {
          THEN("an exception is thrown")
          {
            REQUIRE_THROWS_AS(lists.get_ids(list_ids[1]), invalid_argument);
          }
        }
      }
    }
  }
}

SCENARIO("bulk_insert_entries(): load entries belonging to different lists from files (vectors, vector_ids, list_ids)", "[.InvertedLists]")
{
  GIVEN("an InvertedLists object, a list of vectors, ids and list ids")
  {
    size_t vector_dim = 1;
    InvertedLists lists = get_inverted_lists_object(vector_dim);

    auto data = gen_vectors(1);
    auto list_ids_data = gen_list_lengths_random_length();
    len_t list_length = get_clustered_vectors_length(data, list_ids_data, vector_dim);
    vector_el_t *vectors = to_ptr(vector_el_t, data.first);
    vector_id_t *ids = to_ptr(vector_id_t, data.second);
    list_id_t *list_ids = to_ptr(list_id_t, list_ids_data);

    unordered_map<list_id_t, vector<vector_el_t>>
        list_vectors_map;
    unordered_map<list_id_t, vector<list_id_t>> list_ids_map;
    for (len_t i = 0; i < list_length; i++)
    {
      list_vectors_map[list_ids[i]].push_back(vectors[i]);
      list_ids_map[list_ids[i]].push_back(ids[i]);
    }

    AND_GIVEN("the vectors, ids and list ids are written to files")
    {
      string vectors_filename = "bulk_vectors.bin";
      size_t vectors_size = list_length * vector_dim * sizeof(vector_el_t);
      string ids_filename = "bulk_vector_ids.bin";
      size_t ids_size = list_length * sizeof(list_id_t);
      string list_ids_filename = "bulk_list_ids.bin";
      size_t list_ids_size = list_length * sizeof(list_id_t);
      write_to_file(vectors_filename, vectors, vectors_size);
      write_to_file(ids_filename, ids, ids_size);
      write_to_file(list_ids_filename, list_ids, list_ids_size);

      WHEN("the entries are bulk inserted")
      {
        lists.bulk_insert_entries(vectors_filename, ids_filename, list_ids_filename, list_length);

        THEN("the inverted lists exist and have the correct length")
        {
          for (auto list : list_vectors_map)
          {
            REQUIRE(lists.get_list_length(list.first) == list.second.size());
          }
        }
        THEN("the inverted lists have the correct vectors")
        {
          for (auto list : list_vectors_map)
          {
            vector_el_t *actual_vectors = lists.get_vectors(list.first);
            for (len_t i = 0; i < list.second.size(); i++)
            {
              REQUIRE(actual_vectors[i] == list.second[i]);
            }
          }
        }
        THEN("the inverted lists have the correct ids")
        {
          for (auto list : list_ids_map)
          {
            list_id_t *actual_ids = lists.get_ids(list.first);
            for (len_t i = 0; i < list.second.size(); i++)
            {
              REQUIRE(actual_ids[i] == list.second[i]);
            }
          }
          THEN("the total size is updated")
          {
            size_t total_size = 0;
            for (auto list : list_vectors_map)
            {
              total_size += get_list_size(vector_dim, list.second.size());
            }
            REQUIRE(lists.get_total_size() == get_total_size(total_size));
          }
          AND_WHEN("the entries are bulk inserted again")
          {
            THEN("an exception is thrown")
            {
              REQUIRE_THROWS_AS(lists.bulk_insert_entries(vectors_filename, ids_filename, list_ids_filename, list_length), runtime_error);
            }
          }
        }
      }
    }
  }
}