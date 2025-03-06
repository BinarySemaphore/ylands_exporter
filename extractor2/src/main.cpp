#include "extractor.hpp"

int main(int argc, char** argv) {
	Config config = getConfigFromArgs(argc, argv);
	return extractAndExport(config);
}