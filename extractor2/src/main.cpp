#include "exporter.hpp"
#include <time.h>
#include <iostream>

int main(int argc, char** argv) {
	clock_t start = clock();
	Config config = getConfigFromArgs(argc, argv);
	int status = extractAndExport(config);
	double duration = clock() - start;
	duration = duration * 1000.0 / CLOCKS_PER_SEC;
	std::cout << "Time taken: " << duration << " ms" << std::endl;
	return status;
}