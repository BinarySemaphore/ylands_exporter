#include <iostream>

#include "utils.hpp"
#include "config.hpp"
#include "exporter.hpp"

int main(int argc, char** argv) {
	double s = timerStart();
	Config config = getConfigFromArgs(argc, argv);
	int status = extractAndExport(config);
	std::cout << "Done" << std::endl;
	std::cout << "Total time taken: " << timerStopMs(s) << " ms" << std::endl;
	return status;
}