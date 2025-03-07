#ifndef YLANDS_H
#define YLANDS_H

#include <vector>

#include "scene.hpp"
#include "json.hpp"
using json = nlohmann::json;

class YlandStandard {
public:
	const float unit = 0.375f;
	const float half_unit = 0.1875f;
	static json lookup;  // Without inline static, requires def in cpp
	static json blockdef;

	static void preloadLookups(const char* filename);
};

void setEntityColor(MeshObj& entity, const std::vector<float>& colors);

#endif // YLANDS_H