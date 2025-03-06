#include "scene.hpp"

#include <fstream>

#include "utils.hpp"
#include "ylands.hpp"

json BLOCKDEF;

Node::Node() {
	this->position = Vector3();
	this->rotation = Quaternion();
}

Node createFromJson(const Config& config, const json& data) {
	Node root = Node();

	try {
		std::ifstream f(config.blockdef_filename);
		if (!f.is_open()) {
			throw LoadException("Failed to open file");
		}
		try {
			BLOCKDEF = json::parse(f);
		} catch (json::parse_error& e) {
			f.close();
			throw ParseException("Failed to parse: " + std::string(e.what()));
		}
		f.close();
	} catch (CustomException& e) {
		throw LoadException(
			"Failed to load BlockDef \"" + config.blockdef_filename + "\": " + e.what()
		);
	}

	try {
		YlandStandard::preloadLookups("lookup.json");
	} catch (CustomException& e) {
		throw LoadException("Failed to load \"lookup.json\": " + std::string(e.what()));
	}

	return root;
}