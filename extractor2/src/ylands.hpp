#ifndef YLANDS_H
#define YLANDS_H

#include <vector>

#include "scene.hpp"
#include "json.hpp"
using json = nlohmann::json;

class YlandStandard {
public:
	static float unit;
	static float half_unit;
	static json lookup;
	static json blockdef;

	static void preloadLookups(const char* filename);
};

void setEntityColor(MeshObj& entity, const std::vector<float>& colors);

#endif // YLANDS_H