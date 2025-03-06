#ifndef YLANDS_H
#define YLANDS_H

#include "scene.hpp"
#include "json.hpp"
using json = nlohmann::json;

class YlandStandard {
public:
	const float unit = 0.375f;
	const float half_unit = 0.1875f;
	static json lookup;  // Without inline static, requires def in cpp

	static void preloadLookups(const char* filename);
};

Material* getEntitySurfaceMaterial(MeshObj& entity);

#endif // YLANDS_H