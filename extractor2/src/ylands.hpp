#ifndef YLANDS_H
#define YLANDS_H

#include "json.hpp"

using json = nlohmann::json;

class YlandStandard {
public:
	const float unit = 0.375f;
	const float half_unit = 0.1875f;
	static json lookup;  // Without inline static, requires def in cpp

	static void preload_lookups(const char* filename);
};

class YlandScene {
private:
	json block_def;
	json scene;
public:
};

#endif // YLANDS_H