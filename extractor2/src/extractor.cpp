#include "extractor.hpp"

#include <iostream>
#include <fstream>
#include <filesystem>

#include "utils.hpp"
#include "json.hpp"
using json = nlohmann::json;

const char* PGM_VERSION = "0.0.1";
const char* PGM_DESCRIPTION = ""
"\n"
"Description:\n"
"  Ylands Export Extractor v2\n"
"  Used with Ylands editor tools:\n"
"    - \"EXPORTBLOCKDEFS.ytool\"\n"
"    - \"EXPORTSCENE.ytool\"\n"
"  to pull JSON data from log file \"log_userscript_ct.txt\".\n"
"  Accepts existing JSON exports for direct conversion.\n"
"  Depending on options can output:\n"
"    - Clean JSON ready for custom scene reconstruction or later conversion.\n"
"    - 3D model (GLB, GLTF, or OBJ).\n"
"  Configure with accompanying file \"extract_config.json\" and \"lookup.json\".\n"
"  See README for more details.\n"
"  Authors: BinarySemaphore\n"
"  Updated: 2025-03-04\n";
const char* PGM_OPTIONS_HELP = ""
"\n"
"Options:\n"
"          -v, --version : Show info.\n"
"             -h, --help : Show info and options help.\n"
"-i, --input <JSON-file> : (optional) Read from existing JSON.\n"
"                          Takes existing JSON export from Ylands.\n"
"                          If not given, will attempt to extract from Ylands\n"
"                          directly using settings in \"extract_config.json\".\n"
"    -o, --output <name> : (optional) Output name [no file extension]\n"
"                          (defualt: \"output\").\n"
"                          Output files will automatically have appropriate\n"
"                          extensions based on TYPE.\n"
"      -t, --type <TYPE> : (optional) Output type\n"
"                          (default: JSON).\n"
"                          Supported TYPEs : file(s) created.\n"
"                             GLB : <name>.glb\n"
"                            GLTF : <name>.gltf and <name>.bin\n"
"                             OBJ : <name>.obj and <name>.mtl\n"
"                            JSON : <name>.json\n"
"                          See \"Output Options\" for more additional options.\n"
"                          If input data is Block Definitions from\n"
"                          \"EXPORTBLOCKDEFS.ytool\" output TYPE forced as JSON.\n"
"\n"
"Output Options (only applies to TYPEs: GLB, GLTF, and JSON):\n"
"  -m : Merge into single geometry.\n"
"       Same as using '-rja'\n"
"  -r : Remove internal faces within same material (unless using -a).\n"
"       Any faces adjacent and opposite another face are removed.\n"
"       This includes their opposing neighbor's face.\n"
"  -j : Join verticies within same material (unless using -a)\n"
"       Any vertices sharing a location with another, or within a very small\n"
"       distance, will be reduced to a single vertex.\n"
"       This efectively \"hardens\" or \"joins\" Yland blocks into a single\n"
"       geometry.\n"
"  -a : Apply to all.\n"
"       For any Join (-j) or Internal Face Removal (-r).\n"
"       Applies to all regardless of material grouping.\n";
/*
Hidden Options (for post-build):
--preload <file> : Run YlandStandard::preload_lookups(<file>).
				   Iterates through refs, loading *.obj files.
				   Substitues the lookup with loaded objects.
				   Serializes the result into a binary file
				   to be loaded by enduser at runtime.
*/
const char* CONFIG_FILE = "./extract_config.json";
const char* LOG_STRIP = "# ";
const char* DATA_INDICATOR_START = "# }";
const char* DATA_INDICATOR_BLOCKDEF = "Export Block Ref (below):";
const char* DATA_INDICATOR_SCENE = "Export Scene (below):";
const char* DATA_TYPE_BLOCKDEF = "BLOCKDEF";
const char* DATA_TYPE_SCENE = "SCENE";
const char* CF_KEY_INSTALL_DIR = "Ylands Install Location";
const char* CF_KEY_LOG_PATH = "Log Location";
const char* CF_KEY_AUTO_NEST = "Auto Nest Scenes";
const char* CF_KEY_PPRINT = "Output Pretty";

void printHelp(const char* pgm_name) {
	printHelp(pgm_name, false);
}

void printHelp(const char* pgm_name, bool version_only) {
	std::cout << "Program: " << pgm_name << std::endl << std::endl;
	std::cout << "Version: " << PGM_VERSION << std::endl;
	std::cout << PGM_DESCRIPTION;
	if (!version_only) {
		std::cout << PGM_OPTIONS_HELP;
	}
}

Config getConfigFromArgs(int argc, char** argv) {
	bool get_preload_filename = false;
	bool get_input_filename = false;
	bool get_output_filename = false;
	bool get_export_type = false;
	bool missing_arg = false;
	char missing_arg_name[25];
	char missing_arg_expects[100];
	char pgm_name[250];
	Config config;

	// Defaults
	config.output_filename = "output";
	config.export_type = ExportType::JSON;
	config.preload = false;
	config.has_input = false;
	config.remove_faces = false;
	config.join_verts = false;
	config.apply_all = false;

	for (int i = 0; i < argc; i++) {
		if (i == 0) {
			std::strcpy(pgm_name, argv[0]);
			continue;
		}

		// Get secondary args
		if (get_preload_filename) {
			config.preload_filename = argv[i];
			get_preload_filename = false;
		} else if (get_input_filename) {
			config.input_filename = argv[i];
			get_input_filename = false;
		} else if (get_output_filename) {
			config.output_filename = argv[i];
			get_output_filename = false;
		} else if (get_export_type) {
			if (std::strcmp(argv[i], "GLB") == 0 || std::strcmp(argv[i], "glb") == 0) {
				config.export_type = ExportType::GLB;
				get_export_type = false;
			} else if (std::strcmp(argv[i], "GLTF") == 0 || std::strcmp(argv[i], "gltf") == 0) {
				config.export_type = ExportType::GLTF;
				get_export_type = false;
			} else if (std::strcmp(argv[i], "OBJ") == 0 || std::strcmp(argv[i], "obj") == 0) {
				config.export_type = ExportType::OBJ;
				get_export_type = false;
			} else if (std::strcmp(argv[i], "JSON") == 0 || std::strcmp(argv[i], "json") == 0) {
				config.export_type = ExportType::JSON;
				get_export_type = false;
			}
		}

		// Catch primary args
		else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
			printHelp(pgm_name);
			exit(0);
		} else if (std::strcmp(argv[i], "--version") == 0 || std::strcmp(argv[i], "-v") == 0) {
			printHelp(pgm_name, true);
			exit(0);
		} else if (std::strcmp(argv[i], "--preload") == 0) {
			config.preload = true;
			get_preload_filename = true;
		} else if (std::strcmp(argv[i], "--input") == 0 || std::strcmp(argv[i], "-i") == 0) {
			config.has_input = true;
			get_input_filename = true;
		} else if (std::strcmp(argv[i], "--output") == 0 || std::strcmp(argv[i], "-o") == 0) {
			get_output_filename = true;
		} else if (std::strcmp(argv[i], "--type") == 0 || std::strcmp(argv[i], "-t") == 0) {
			get_export_type = true;
		} else if (std::strcmp(argv[i], "-r") == 0) {
			config.remove_faces = true;
		} else if (std::strcmp(argv[i], "-j") == 0) {
			config.join_verts = true;
		} else if (std::strcmp(argv[i], "-a") == 0) {
			config.apply_all = true;
		} else if (std::strcmp(argv[i], "-m") == 0) {
			config.remove_faces = true;
			config.join_verts = true;
			config.apply_all = true;
		} else {
			std::cerr << "Unrecongnized argument \"" << argv[i] << "\". "
					  << "Use -h or --help to show options." << std::endl;
			exit(1);
		}
	}

	// Check if secondary args missing
	missing_arg = get_preload_filename || get_input_filename ||
				  get_output_filename || get_export_type;
	if (get_preload_filename) {
		std::strcpy(missing_arg_name, "--preload");
		std::strcpy(missing_arg_expects, "a filename");
	} else if (get_input_filename) {
		std::strcpy(missing_arg_name, "--input (-i)");
		std::strcpy(missing_arg_expects, "a filename");
	} else if (get_output_filename) {
		std::strcpy(missing_arg_name, "--output (-o)");
		std::strcpy(missing_arg_expects, "a filename");
	} else if (get_export_type) {
		std::strcpy(missing_arg_name, "--type (-t)");
		std::strcpy(missing_arg_expects, "a Type (JSON, GLB, GLTF, or OBJ)");
	}
	if (missing_arg) {
		std::cerr << "Arguement missing: \"" << missing_arg_name
				  << "\" expects " << missing_arg_expects << std::endl;
		exit(1);
	}

	return config;
}

int extractAndExport(Config& config) {
	json data;

	if (!config.has_input) {
		try {
			extractFromYlands(config, data);
		} catch (CustomException& e) {
			std::cerr << "Error while extrating from Ylands: " << e.what()
					  << std::endl;
			return 1;
		}
		if (isDataBlockDef(data) && config.export_type != ExportType::JSON) {
			std::cout << "Extracted " << DATA_TYPE_BLOCKDEF
					  << " from Ylands, forcing JSON export." << std::endl;
			config.export_type = ExportType::JSON;
		}
	} else {
		try {
			loadFromFile(config.input_filename.c_str(), data);
		} catch (CustomException& e) {
			std::cerr << "Error while loading from JSON file \""
					  << config.input_filename << "\": " << e.what()
					  << std::endl;
			return 1;
		}
		if (config.export_type == ExportType::JSON) {
			std::cout << "Nothing to do:"
					  << " Loading from JSON file and exporting to JSON."
					  << std::endl;
			return 0;
		}
		if (isDataBlockDef(data)) {
			std::cerr << "Error: Given Ylands BLOCKDEF data." << std::endl
					  << "BLOCKDEF is for referencing only, not processing."
					  << std::endl
					  << "To set BLOCKDEF update \"lookup.json\" file."
					  << std::endl;
			return 2;
		}
	}

	// JSON export
	if (config.export_type == ExportType::JSON) {
		try {
			exportAsJson(config.output_filename.c_str(), data, config.ext_pprint);
		} catch(CustomException& e) {
			std::cerr << "Error exporting JSON file \""
					  << config.output_filename << "\": "
					  << e.what() << std::endl;
			return 3;
		}
		return 0;
	}

	if (!isDataValidScene(data)) {
		std::cerr << "Invalid JSON data: expected Ylands SCENE data."
				  << std::endl;
		return 2;
	}

	// Convert data into 3D Scene
	// Process scene using config flags

	// OBJ export
	// GLTF export
	// GLB export

	// Finish
	return 0;
}

void exportAsJson(const char* filename, const json& data, bool pprint) {
	char filename_ext[200] = "";
	std::strcat(filename_ext, filename);
	std::strcat(filename_ext, ".json");

	std::cout << "Exporting [JSON] file \"" << filename_ext << "\"..." << std::endl;
	std::ofstream f(filename_ext);
	if (!f.is_open()) {
		throw SaveException("Failed to open file");
	}
	if (pprint) {
		f << data.dump(4);
	} else {
		f << data.dump();
	}
	f.close();
	std::cout << "Export complete" << std::endl << std::endl;
}

void loadFromFile(const char* filename, json& data) {
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
	std::cout << "Loaded from JSON file" << std::endl << std::endl;
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

	std::cout << "Extracting directly from Ylands..." << std::endl;
	updateConfigFromFile(config, CONFIG_FILE);

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
	try {
		data = json::parse(raw_data);
	} catch (json::parse_error& e) {
		std::cerr << "Failed to parse JSON data: " << e.what() << std::endl;
		std::cerr << "Storing raw data in \"error.txt\"" << std::endl;
		std::ofstream f("error.txt");
		if (!f.is_open()) {
			throw SaveException("Failed to open \"error.txt\" for writing");
		}
		f << raw_data;
		f.close();
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
			std::cout << "Continue without nesting? (y/n): ";
			std::cin >> confirm;
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
	std::cout << "JSON extracted from Ylands" << std::endl << std::endl;
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

void updateConfigFromFile(Config& config, const char* filename) {
	json data;

	std::cout << "Loading config file: \"" << filename << "\"..." << std::endl;
	std::ifstream f(filename);
	if (!f.is_open()) {
		throw LoadException("Failed to open \"" + std::string(filename)
		+ "\".");
	}
	try {
		data = json::parse(f);
	} catch (json::parse_error& e) {
		f.close();
		throw ParseException("Failed to parse JSON file: " + std::string(e.what()));
	}
	f.close();
	if (data.contains(CF_KEY_INSTALL_DIR)) {
		config.ylands_install_dir = data[CF_KEY_INSTALL_DIR];
	} else config.ylands_install_dir = "";
	if (data.contains(CF_KEY_LOG_PATH)) {
		config.ylands_log_path = data[CF_KEY_LOG_PATH];
	} else config.ylands_log_path = "";
	if (data.contains(CF_KEY_AUTO_NEST)) {
		config.ext_auto_nest = data[CF_KEY_AUTO_NEST];
	} else config.ext_auto_nest = true;
	if (data.contains(CF_KEY_PPRINT)) {
		config.ext_pprint = data[CF_KEY_PPRINT];
	} else config.ext_pprint = false;

	validateConfigAndPromptForFixes(config, filename, false);

	// Update data with any changes to config from validating
	data[CF_KEY_INSTALL_DIR] = config.ylands_install_dir;
	data[CF_KEY_LOG_PATH] = config.ylands_log_path;
	data[CF_KEY_AUTO_NEST] = config.ext_auto_nest;
	data[CF_KEY_PPRINT] = config.ext_pprint;
	std::cout << "Config loaded: " << data.dump(4).c_str() << std::endl;
}

void validateConfigAndPromptForFixes(Config& config, const char* filename, bool config_changed) {
	bool valid = true;
	char confirm[2];

	std::filesystem::path install_dir(config.ylands_install_dir);
	std::filesystem::path log_path(config.ylands_log_path);
	std::filesystem::path log_fullpath = install_dir / log_path;

	if (!std::filesystem::exists(install_dir) ||
		!std::filesystem::is_directory(install_dir)) {
		std::cout << "\nInvalid Config \"" << CF_KEY_INSTALL_DIR
				  << "\", directory does not exist." << std::endl;
		std::cout << "Path was: \"" << config.ylands_install_dir
				  << "\"." << std::endl;
		std::cout << "Enter new path: ";
		getline(std::cin, config.ylands_install_dir);
		std::cout << std::endl;
		config_changed = true;
		valid = false;
	} else if (!std::filesystem::exists(log_fullpath) ||
		!std::filesystem::is_regular_file(log_fullpath)) {
		std::cout << "\nInvalid Config \"" << CF_KEY_LOG_PATH
				  << "\", file does not exist." << std::endl;
		std::cout << "Path was: \"" << config.ylands_log_path
				  << "\"." << std::endl;
		std::cout << "Enter new path: ";
		getline(std::cin, config.ylands_log_path);
		std::cout << std::endl;
		config_changed = true;
		valid = false;
	}

	if (!valid) {
		return validateConfigAndPromptForFixes(config, filename, config_changed);
	}

	if (config_changed) {
		std::cout << "Config was updated, save to config file? (y/n): ";
		std::cin >> confirm;
		std::cout << std::endl;
		if (confirm[0] == 'y' || confirm[0] == 'Y') {
			std::ofstream f(filename);
			if (!f.is_open()) {
				throw SaveException(
					"Failed to open \"" + std::string(filename)
					+ "\" for writing."
				);
			}
			f << "{\n"
			  << "    \"" << CF_KEY_INSTALL_DIR << "\": \""
			  << config.ylands_install_dir << "\",\n"
			  << "    \"" << CF_KEY_LOG_PATH << "\": \""
			  << config.ylands_log_path << "\",\n"
			  << "    \"" << CF_KEY_AUTO_NEST << "\": ";
			if (config.ext_auto_nest) f << "true";
			else f << "false";
			f << ",\n";
			f << "    \"" << CF_KEY_PPRINT << "\": ";
			if (config.ext_pprint) f << "true";
			else f << "false";
			f << "\n";
			f << "}";
			f.close();
			std::cout << "Updated config saved to \"" << filename
					  << "\"" << std::endl;
		}
	}
}

bool isDataBlockDef(const json& data) {
	bool is_blockdef = false;
	for (auto& [key, value] : data.items()) {
		if (value.contains("shape") || value.contains("NONE")) {
			is_blockdef = true;
		}
		break;
	}
	return is_blockdef;
}


bool isDataValidScene(const json& data) {
	bool is_validscene = false;
	for (auto& [key, value] : data.items()) {
		if (value.contains("position") || value.contains("rotation")) {
			is_validscene = true;
		}
		break;
	}
	return is_validscene;
}