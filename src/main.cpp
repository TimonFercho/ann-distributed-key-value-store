#include <iostream>

#include "InvertedLists.h"

using namespace std;
using namespace ann_dkvs;

void print_list(InvertedLists *lists, list_id_t list_id)
{
	vector_el_t *vectors = lists->get_vectors(list_id);
	vector_id_t *ids = lists->get_ids(list_id);
	len_t list_len = lists->get_list_length(list_id);
	cout << "List " << list_id << " (length: " << list_len << ")" << endl;
	for (int i = 0; i < list_len; i++)
	{
		cout << ids[i] << ": " << vectors[i] << endl;
	}
}

int main(int argc, char const *argv[])
{
	size_t vector_dim = 1;
	string filename = "lists.bin";
	InvertedLists lists = InvertedLists(vector_dim, filename);
	lists.create_list(1, 5);
	cout << "number of lists: " << lists.get_length() << endl;
	vector_el_t il1_vectors[5] = {6.0, 7.0, 8.0, 9.0, 10.0};
	list_id_t il1_ids[5] = {1, 2, 3, 4, 5};
	cout << "number of entries in list 1: " << lists.get_list_length(1) << endl;
	lists.update_entries(1, il1_vectors, il1_ids, 0, 5);
	cout << "number of entries in list 1: " << lists.get_list_length(1) << endl;
	print_list(&lists, 1);
	return 0;
}
