#include "ylands.hpp"

#include <fstream>
#include <filesystem>

#include "utils.hpp"
#include "objwavefront.hpp"

// Add definition as nlohmann::json requires more than just declaration (without inline static)
json YlandStandard::lookup;

void YlandStandard::preloadLookups(const char* filename) {
	std::filesystem::path base_dir;
	std::filesystem::path ref_path;

	std::ifstream f(filename);
	if (!f.is_open()) {
		throw LoadException("Failed to open file");
	}
	try {
		YlandStandard::lookup = json::parse(f);
	} catch (json::parse_error& e) {
		f.close();
		throw ParseException("Failed to parse lookup JSON: " + std::string(e.what()));
	}
	f.close();

	if (YlandStandard::lookup.contains("base-dir")) {
		base_dir = (std::string)YlandStandard::lookup["base-dir"];
	} else {
		base_dir = ".";
	}

	for (auto& [k1, item] : YlandStandard::lookup.items()) {
		if(!item.is_object()) continue;
		for (auto& [k2, path] : item.items()) {
			if(!path.is_string()) continue;
			ref_path = (std::string)path;
			item[k2] = base_dir / ref_path;
		}
	}
}

Material* getEntitySurfaceMaterial(MeshObj& entity) {
	Material* mat = NULL;
	std::vector<Material*> mats = entity.mesh.getSurfaceMaterials(0);
	if (mats.size() > 0) mat = mats[0];
	return mat;
}