#include "extractor.hpp"

#include <iostream>
#include <fstream>

#include "utils.hpp"

const char* DATA_TYPE_BLOCKDEF = "BLOCKDEF";
const char* DATA_TYPE_SCENE = "SCENE";

const char* LOG_STRIP = "# ";
const char* DATA_INDICATOR_START = "# }";
const char* DATA_INDICATOR_BLOCKDEF = "Export Block Ref (below):";
const char* DATA_INDICATOR_SCENE = "Export Scene (below):";

void loadFromFile(const char* filename, json& data) {
	double s = timerStart();
	std::cout << "Loading from JSON file \"" << filename << "\"..." << std::endl;
	std::ifstream f(filename);
	if (!f.is_open()) {
		throw LoadException("Failed to open file");
	}
	try {
		data = json::parse(f);
	} catch (json::parse_error& e) {
		f.close();
		throw ParseException("Failed to parse JSON file: " + std::string(e.what()));
	}
	f.close();
	std::cout << "Loaded from JSON file" << std::endl;
	timerStopMsAndPrint(s);
	std::cout << std::endl;
}

void extractFromYlands(Config& config, json& data) {
	bool data_type_set;
	int i;
	int data_start = -1;
	char confirm[2];
	char data_type[20] = "";
	std::filesystem::path ylands_install_dir;
	std::filesystem::path log_fullpath;
	std::string line;
	std::string raw_data;
	std::vector<std::string> log_lines;
	std::vector<std::string> raw_data_lines;

	std::cout << "Extracting JSON directly from Ylands..." << std::endl;
	updateConfigFromFile(config, CONFIG_FILE);
	double s = timerStart();

	ylands_install_dir = std::filesystem::path(config.ylands_install_dir);
	log_fullpath = std::filesystem::path(config.ylands_log_path);
	log_fullpath = ylands_install_dir / log_fullpath;

	std::cout << "Loading log file \"" << log_fullpath.string() << "\"..." << std::endl;
	std::ifstream f(log_fullpath);
	if (!f.is_open()) {
		throw LoadException("Failed to open \"" + log_fullpath.string() + "\".");
	}
	while (std::getline(f, line)) {
		log_lines.push_back(line);
	}
	if (log_lines.size() == 0) {
		throw ParseException("Log file is empty");
	}
	f.close();
	std::cout << "Loaded" << std::endl << std::endl;

	std::cout << "Searching for exported data in log file..." << std::endl;
	data_type_set = false;
	for (i = log_lines.size() - 1; i >= 0; i--) {
		line = log_lines[i];
		if (line.rfind(DATA_INDICATOR_START, 0) == 0) {
			data_start = i;
		}
		if (data_start >= 0 && !data_type_set) {
			if (line.find(DATA_INDICATOR_BLOCKDEF) != std::string::npos) {
				std::strcpy(data_type, DATA_TYPE_BLOCKDEF);
				data_type_set = true;
			} else if (line.find(DATA_INDICATOR_SCENE) != std::string::npos) {
				std::strcpy(data_type, DATA_TYPE_SCENE);
				data_type_set = true;
			}
		}
		if (data_type_set) {
			raw_data_lines = std::vector<std::string>(
				log_lines.begin() + i + 1,
				log_lines.begin() + data_start + 1
			);
			log_lines.clear();
			break;
		}
	}
	if (!data_type_set || raw_data_lines.size() == 0) {
		throw ParseException(
			"Could not find any exported data, please try restarting"
			" Ylands and rerunning the export tool"
		);
	}
	std::cout << "Found " << data_type << " exported data"
			  << std::endl << std::endl;
	
	std::cout << "Processing " << data_type << " data to JSON..." << std::endl;
	for (i = 0; i < raw_data_lines.size(); i++) {
		raw_data_lines[i] = string_replace(raw_data_lines[i], LOG_STRIP, "");
	}
	raw_data = string_join(raw_data_lines, "");
	raw_data = string_ascii(raw_data);
	try {
		data = json::parse(raw_data);
	} catch (json::parse_error& e) {
		std::cerr << "Failed to parse JSON data: " << e.what() << std::endl;
		std::cerr << "Storing raw data in \"error.txt\"" << std::endl;
		std::ofstream ef("error.txt");
		if (!ef.is_open()) {
			throw SaveException("Failed to open \"error.txt\" for writing");
		}
		ef << raw_data;
		ef.close();
		throw ParseException(
			"Failed to parse JSON data: see error/log for details"
		);
	}
	raw_data_lines.clear();
	raw_data.clear();
	std::cout << "JSON ready" << std::endl << std::endl;

	if (std::strcmp(data_type, DATA_TYPE_SCENE) == 0 && config.ext_auto_nest) {
		std::cout << "Reformat " << data_type << " JSON from flat to nested..."
				  << std::endl;
		try {
			reformatSceneFlatToNested(data);
		} catch (ParseException& e) {
			std::cerr << "Failed to reformat data: " << e.what() << std::endl;
			if (config.no_interact) {
				confirm[0] = '\0';
			} else {
				std::cout << "Continue without nesting? (y/n): ";
				std::cin >> confirm;
			}
			std::cout << std::endl;
			if (confirm[0] != 'y') {
				throw ParseException(
					"Failed to reformat data (user prompted: abort and exit):"
					" see error/log for details"
				);
			}
		}
		std::cout << "JSON data reformatted" << std::endl << std::endl;
	}
	std::cout << "JSON extracted from Ylands" << std::endl;
	timerStopMsAndPrint(s);
	std::cout << std::endl;
}

void reformatSceneFlatToNested(json& data) {
	std::string parent_key;
	std::vector<std::string> keys_to_remove;
	json* parent;

	for (auto& [key, item] : data.items()) {
		if (item.contains("parent")) {
			parent_key = item["parent"];
			if (!data.contains(parent_key)) {
				throw ParseException(
					"Cannot find parent \"" + parent_key + "\" in JSON data"
				);
			}
			parent = &data[parent_key];
			if (!parent->contains("children")) {
				(*parent)["children"] = json {};
			}
			(*parent)["children"][key] = item;
			keys_to_remove.push_back(key);
		}
	}

	for (int i = 0; i < keys_to_remove.size(); i++) {
		data.erase(keys_to_remove[i]);
	}
}

bool isDataBlockDef(const json& data) {
	bool is_blockdef = false;
	if (!data.is_object()) return false;
	for (auto& [key, item] : data.items()) {
		if (!item.is_object()) break;
		if (item.contains("shape") || item.contains("NONE")) {
			is_blockdef = true;
		}
		break;
	}
	return is_blockdef;
}

bool isDataValidScene(const json& data) {
	bool is_validscene = false;
	if (!data.is_object()) return false;
	for (auto& [key, item] : data.items()) {
		if (!item.is_object()) break;
		if (item.contains("position") || item.contains("rotation")) {
			is_validscene = true;
		}
		break;
	}
	return is_validscene;
}