#include <iostream>

#include "utils.hpp"
#include "config.hpp"
#include "exporter.hpp"
#include "workpool.hpp"

int main(int argc, char** argv) {
	double s = timerStart();
	Config config = getConfigFromArgs(argc, argv);
	int status = extractAndExport(config);
	// Stop is a forcable stop, so wait first just in case
	//wp->wait();
	//wp->stop();
	std::cout << "Done" << std::endl;
	std::cout << "Total time taken: " << timerStopMs(s) << " ms" << std::endl;
	return status;
}