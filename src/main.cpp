#include <iostream>
#include <iomanip>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "StorageLists.hpp"

using namespace ann_dkvs;

int main(int argc, char const *argv[])
{
#ifdef _OPENMP
	std::cout << "OpenMP is enabled" << std::endl;
#else
	std::cout << "OpenMP is disabled" << std::endl;
#endif
#ifdef _OPENMP
	omp_set_num_threads(256);
#pragma omp parallel
	{
		int id = omp_get_thread_num();
#pragma omp critical
		{
			std::cout << "Hello World from thread " << std::setfill('0') << std::setw(2) << std::right << id << "\n";
		}
	}
#endif
	return 0;
}
