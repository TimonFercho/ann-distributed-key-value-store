#include <iostream>

#include "InvertedLists.h"

using namespace std;
using namespace ann_dkvs;

int main(int argc, char const *argv[])
{
	size_t vector_dim = 1;
	string filename = "lists.bin";
	InvertedLists lists = InvertedLists(vector_dim, filename);
	return 0;
}
