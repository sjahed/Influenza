#include <iostream>
#include <algorithm>
#include <vector>

#include <mcstl.h>

const int count = 1000000, num_tests = 10;

int main(int argc, char **argv)
{
	if(argc >= 2)
		mcstl::SETTINGS::num_threads = atoi(argv[1]);
	//take OMP_NUM_THREADS environment variable otherwise
	
	std::vector<int> v(count);
	double total_time = 0.0;

	for (int i = 0; i < num_tests; i++)
	{
		//shared state through random number generator, therefore sequential
		std::generate(v.begin(), v.end(), rand, mcstl::sequential_tag());

		double start = omp_get_wtime();
		std::sort(v.begin(), v.end());	//parallel algorithm
		double end = omp_get_wtime();

		total_time += end - start;
		
		std::cout << "." << std::flush;	//show progress
	}

	std::cout << std::endl << "Sorting " << count << " integers " << num_tests << " times takes " << total_time << " sec, using " << mcstl::SETTINGS::num_threads << " thread(s)." << std::endl;
}
