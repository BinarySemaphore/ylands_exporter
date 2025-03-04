#include "ylands.hpp"

#include <fstream>

#include "json.hpp"

using json = nlohmann::json;

// Add definition as nlohmann::json requires more than just declaration (without inline static)
json YlandStandard::lookup;

void YlandStandard::preloadLookups(const char* filename) {
	std::fstream f(filename);
	YlandStandard::lookup = json::parse(f);
	f.close();
}

