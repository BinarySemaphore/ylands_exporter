#include "extractor.hpp"

int main(int argc, char** argv) {
	Config config = getConfigFromArgs(argc, argv);
	extract(config);
	return 0;
}