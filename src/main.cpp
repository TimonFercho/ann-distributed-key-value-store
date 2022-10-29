#include <iostream>

#include "InvertedLists.h"

using namespace std;
using namespace ann_dkvs;

void print_list(InvertedLists ivls, list_id_t id)
{
	vector_el_t *vectors = ivls.get_vectors(id);
	string filename = ivls.get_filename(id);
	size_t size = ivls.get_list_size(id);
	for (size_t i = 0; i < size; i++)
	{
		cout << filename << "[" << i << "]"
				 << ": " << vectors[i] << endl;
	}
}

int main(int argc, char const *argv[])
{
	size_t vector_size = 1 * sizeof(vector_el_t); // 1D
	InvertedLists ivls = InvertedLists(vector_size);

	string il1_filename = "inverted_list1.bin";
	vector_el_t il1_vectors[5] = {1.0, 2.0, 3.0, 4.0, 5.0};
	list_id_t il1_ids[5] = {1, 2, 3, 4, 5};
	ivls.write_list("inverted_list1.bin", il1_vectors, il1_ids, 5);
	cout << "wrote list 1" << endl;

	string il2_filename = "inverted_list2.bin";
	vector_el_t il2_vectors[3] = {6.0, 7.0, 8.0};
	list_id_t il2_ids[3] = {6, 7, 8};
	ivls.write_list("inverted_list2.bin", il2_vectors, il2_ids, 3);
	cout << "wrote list 2" << endl;

	cout << "number of lists: " << ivls.get_size() << endl;

	cout << "inserting list 1" << endl;
	ivls.insert_list(1, il1_filename, 5);
	cout << "number of lists: " << ivls.get_size() << endl;
	cout << "number of elements in list 1: " << ivls.get_list_size(1) << endl;
	print_list(ivls, 1);

	cout << "inserting list 2" << endl;
	ivls.insert_list(2, il2_filename, 3);
	cout << "number of lists: " << ivls.get_size() << endl;
	cout << "number of elements in list 2: " << ivls.get_list_size(2) << endl;
	print_list(ivls, 2);
	return 0;
}
