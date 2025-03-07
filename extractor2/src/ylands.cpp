#include "ylands.hpp"

#include <fstream>
#include <filesystem>

#include "utils.hpp"
#include "objwavefront.hpp"

// Add definition as nlohmann::json requires more than just declaration (without inline static)
json YlandStandard::lookup;
json YlandStandard::blockdef;

void YlandStandard::preloadLookups(const char* filename) {
	std::ifstream f;
	std::filesystem::path base_dir;
	std::filesystem::path ref_path;

	f = std::ifstream(filename);
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

	if (!YlandStandard::lookup.is_object()) {
		throw LoadException("Invalid lookup JSON: lookup must be JSON object");
	}
	if (!YlandStandard::lookup.contains("shapes")) {
		throw LoadException("Invalid lookup JSON: lookup must have \"shapes\"");
	}
	if (!YlandStandard::lookup["shapes"].is_object()) {
		throw LoadException("Invalid lookup JSON: \"shapes\" must be JSON object");
	}
	if (!YlandStandard::lookup.contains("types")) {
		throw LoadException("Invalid lookup JSON: lookup must have \"types\"");
	}
	if (!YlandStandard::lookup["types"].is_object()) {
		throw LoadException("Invalid lookup JSON: \"types\" must be JSON object");
	}
	if (!YlandStandard::lookup.contains("ids")) {
		throw LoadException("Invalid lookup JSON: lookup must have \"ids\"");
	}
	if (!YlandStandard::lookup["ids"].is_object()) {
		throw LoadException("Invalid lookup JSON: \"ids\" must be JSON object");
	}
	if (!YlandStandard::lookup.contains("blockdef-file")) {
		throw LoadException("Invalid lookup JSON: lookup must have \"blockdef-file\"");
	}
	if (!YlandStandard::lookup["blockdef-file"].is_string()) {
		throw LoadException("Invalid lookup JSON: \"blockdef-file\" must be string");
	}

	f = std::ifstream((std::string)YlandStandard::lookup["blockdef-file"]);
	if (!f.is_open()) {
		throw LoadException(
			"Failed to open blockdef-file \""
			+ YlandStandard::lookup["blockdef-file"]
			+ std::string("\"")
		);
	}
	try {
		YlandStandard::blockdef = json::parse(f);
	} catch (json::parse_error& e) {
		f.close();
		throw ParseException(
			"Failed to parse blockdef-file JSON \""
			+ YlandStandard::lookup["blockdef-file"]
			+ std::string("\": ") + std::string(e.what())
		);
	}
	f.close();
	YlandStandard::lookup.erase("blockdef-file");

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

void setEntityColor(MeshObj& entity, const std::vector<float>& colors) {
	Material* mat = entity.mesh.getSurfaceMaterials(0)[0];
	mat->diffuse = Vector3(colors[0], colors[1], colors[2]);
	mat->ambient = mat->diffuse;
	if (colors[4] > 0.0f) {
		mat->emissive = mat->diffuse;
	}
}