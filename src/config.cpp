#include "config.hpp"

#include <iostream>
#include <fstream>

#include "utils.hpp"
#include "json.hpp"
using json = nlohmann::json;

char PGM_NAME[128];
const char* PGM_NAME_READABLE = "Ylands Extractor";
const char* PGM_VERSION = "0.2.1";
const char* PGM_REF_LINK = "https://github.com/BinarySemaphore/ylands_exporter";
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
"  Configure with accompanying file \"config.json\".\n"
"  See README for more details.\n"
"  Authors: BinarySemaphore\n"
"  Updated: 2025-03-26\n";
const char* PGM_OPTIONS_HELP = ""
"\n"
"Options:\n"
"          -v, --version : Show info.\n"
"             -h, --help : Show info and options help.\n"
"          --no-interact : Non interactive mode.\n"
"                          Will exit on errors or prompts.\n"
"-i, --input <JSON-FILE> : (optional) Read from existing JSON file.\n"
"                          Takes existing JSON export from Ylands.\n"
"                          If not given, will attempt to extract from Ylands\n"
"                          directly using settings in \"config.json\".\n"
"    -o, --output <NAME> : (optional) Output name [no file extension]\n"
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
"                          Recommend GLTF wanting to preserve build groups\n"
"                          If input data is Block Definitions from\n"
"                          \"EXPORTBLOCKDEFS.ytool\" output TYPE forced as JSON.\n"
"\n"
"Output Options (only applies to TYPEs: GLB, GLTF, and JSON):\n"
"  -u <VISIBILITY> : Draw unsupported Yland entities.\n"
"                    Ylands has 5k+ entities and not all geometry is supported\n"
"                    by this program, but bounding boxes are known.\n"
"                    This option will draw transparent bounding boxes for any\n"
"                    unsupported entities.\n"
"                    Transparency is set by VISIBILITY percent:\n"
"                    0.0 to 1.0 and anything inbetween.\n"
"                    0.0 being invisible and 1.0 being fully opaque.\n"
"               -c : Combine geometry by shared group and material.\n"
"                    Recommended for large builds: reduces export complexity.\n"
"                    Unless using Join verticies (-j), individual entity\n"
"                    geometry will still be retained.\n"
"                    Note: OBJ exports only supports single objects; a combine\n"
"                    is always done for OBJ export (grouping in surfaces).\n"
"               -m : Merge into single geometry.\n"
"                    Same as using '-rja'.\n"
"                    Warning: materials will switch to default.\n"
"               -r : Remove internal faces.\n"
"                    Only within same material (unless TYPE OBJ or using -a).\n"
"                    Any faces adjacent and opposite another face are removed.\n"
"                    This includes their opposing neighbor's face.\n"
"               -j : Join verticies.\n"
"                    Only within same material (unless TYPE OBJ or using -a).\n"
"                    Any vertices sharing a location with another, or within\n"
"                    a very small distance, will be reduced to a single vertex.\n"
"                    This efectively \"hardens\" or \"joins\" Yland entities\n"
"                    into a single geometry.\n"
"               -a : Apply to all.\n"
"                    For any Join Verticies (-j) or Internal Face Removal (-r).\n"
"                    Applies to all regardless of material grouping.\n"
"                    Ignored if both -r and -j are not used.\n"
"\n"
"Example \"config.json\":\n"
"{\n"
"    \"Ylands Install Location\": \"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Ylands\",\n"
"    \"Log Location\": \"Ylands_Data\\log_userscript_ct.txt\",\n"
"    \"Auto Nest Scenes\": true,\n"
"    \"Output Pretty\": false,\n"
"    \"Block Def JSON\": \"blockdef_2025_02.json\"\n"
"}\n";
/*
Hidden Options (for post-build):
--preload <file> : Run YlandStandard::preload_lookups(<file>).
				   Iterates through refs, loading *.obj files.
				   Substitues the lookup with loaded objects.
				   Serializes the result into a binary file
				   to be loaded by enduser at runtime.
*/

const char* CONFIG_FILE = "./config.json";

const char* CF_KEY_INSTALL_DIR = "Ylands Install Location";
const char* CF_KEY_LOG_PATH = "Log Location";
const char* CF_KEY_AUTO_NEST = "Auto Nest Scenes";
const char* CF_KEY_PPRINT = "Output Pretty";

void printHelp() {
	printHelp(false);
}

void printHelp(bool version_only) {
	std::cout << "Program: " << PGM_NAME << std::endl << std::endl;
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
	bool get_bb_transparency = false;
	bool missing_arg = false;
	char missing_arg_name[25];
	char missing_arg_expects[100];
	Config config;

	// Defaults (args)
	config.output_filename = "output";
	config.export_type = ExportType::JSON;
	config.preload = false;
	config.has_input = false;
	config.remove_faces = false;
	config.join_verts = false;
	config.apply_all = false;
	config.combine = false;
	config.draw_bb = false;
	config.draw_bb_transparency = 0.5f;

	// Defaults (config.json)
	config.ylands_install_dir = "";
	config.ylands_log_path = "";
	config.ext_auto_nest = true;
	config.ext_pprint = false;

	for (int i = 0; i < argc; i++) {
		if (i == 0) {
			std::strcpy(PGM_NAME, argv[0]);
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
		} else if (get_bb_transparency) {
			try {
				config.draw_bb_transparency = std::stof(argv[i]);
				if (config.draw_bb_transparency >= 0.0f && config.draw_bb_transparency <= 1.0f) {
					get_bb_transparency = false;
				} else break;
			} catch (std::exception) {
				break;
			}
		} else if (get_export_type) {
			if (std::strcmp(argv[i], "GLB") == 0 || std::strcmp(argv[i], "glb") == 0) {
				config.export_type = ExportType::GLB;
				get_export_type = false;
			} else if (std::strcmp(argv[i], "GLTF") == 0 || std::strcmp(argv[i], "gltf") == 0) {
				config.export_type = ExportType::GLTF;
				get_export_type = false;
			} else if (std::strcmp(argv[i], "OBJ") == 0 || std::strcmp(argv[i], "obj") == 0) {
				config.combine = true;
				config.export_type = ExportType::OBJ;
				get_export_type = false;
			} else if (std::strcmp(argv[i], "JSON") == 0 || std::strcmp(argv[i], "json") == 0) {
				config.export_type = ExportType::JSON;
				get_export_type = false;
			} else {
				break;
			}
		}

		// Catch primary args
		else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
			printHelp();
			exit(0);
		} else if (std::strcmp(argv[i], "--version") == 0 || std::strcmp(argv[i], "-v") == 0) {
			printHelp(true);
			exit(0);
		} else if (std::strcmp(argv[i], "--preload") == 0) {
			config.preload = true;
			get_preload_filename = true;
		} else if (std::strcmp(argv[i], "--no-interact") == 0) {
			config.no_interact = true;
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
		} else if (std::strcmp(argv[i], "-c") == 0) {
			config.combine = true;
		} else if (std::strcmp(argv[i], "-u") == 0) {
			config.draw_bb = true;
			get_bb_transparency = true;
		} else {
			std::cerr << "Unrecongnized argument \"" << argv[i] << "\". "
					  << "Use -h or --help to show options." << std::endl;
			exit(1);
		}
	}

	// Check if secondary args missing
	missing_arg = get_preload_filename || get_input_filename ||
				  get_output_filename || get_export_type ||
				  get_bb_transparency;
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
	} else if (get_bb_transparency) {
		std::strcpy(missing_arg_name, "-u");
		std::strcpy(missing_arg_expects, "a decimal value (between 0.0 and 1.0)");
	}
	if (missing_arg) {
		std::cerr << "Arguement missing: \"" << missing_arg_name
				  << "\" expects " << missing_arg_expects << std::endl;
		exit(1);
	}

	return config;
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
	}
	if (data.contains(CF_KEY_LOG_PATH)) {
		config.ylands_log_path = data[CF_KEY_LOG_PATH];
	}
	if (data.contains(CF_KEY_AUTO_NEST)) {
		config.ext_auto_nest = data[CF_KEY_AUTO_NEST];
	}
	if (data.contains(CF_KEY_PPRINT)) {
		config.ext_pprint = data[CF_KEY_PPRINT];
	}

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
		if (config.no_interact) exit(1);
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
		if (config.no_interact) exit(1);
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
			  << "    \"" << CF_KEY_INSTALL_DIR << "\": "
			  << install_dir << ",\n"
			  << "    \"" << CF_KEY_LOG_PATH << "\": "
			  << log_path << ",\n"
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