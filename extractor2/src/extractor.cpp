#include "extractor.hpp"

#include <iostream>
#include <filesystem>

#include "json.hpp"
using json = nlohmann::json;

const char* PGM_VERSION = "0.0.1";
const char* PGM_DESCRIPTION = ""
"\n"
"Description:\n"
"  Ylands Export Extractor v2\n"
"  Used with Ylands editor tools (\"EXPORTBLOCKDEFS.ytool\" and \"EXPORTSCENE.ytool\")\n"
"  to pull JSON data from log file \"log_userscript_ct.txt\".\n"
"  Also accepts existing JSON exports for direct conversion.\n"
"  Depending on options can produce:\n"
"    - Clean JSON ready for custom scene reconstruction or later conversion.\n"
"    - 3D model (GLB, GLTF, or OBJ).\n"
"  Configure with accompanying file \"extract_config.json\".\n"
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
"                          If not given, will attempt to extract\n"
"                          from Ylands directly using setup\n"
"                          in \"extract_config.json\".\n"
"    -o, --output <name> : Output name (without file extension).\n"
"                          Output files will automatically get\n"
"                          appropriate extensions.\n"
"      -t, --type <TYPE> : (optional) Output type (default: OBJ).\n"
"                          Supported TYPEs : file(s) created.\n"
"                             GLB : <name>.glb\n"
"                            GLTF : <name>.gltf and <name>.bin\n"
"                             OBJ : <name>.obj and <name>.mtl\n"
"                            JSON : <name>.json\n"
"                          See \"Output Options\" for more options.\n"
"                          If input data is Block Definitions from\n"
"                          \"EXPORTBLOCKDEFS.ytool\" type will\n"
"                          automatically be set to JSON.\n"
"\n"
"Output Options (only applies to TYPEs: GLB, GLTF, and JSON):\n"
"  -m : Merge into single geometry.\n"
"       Same as using '-rja'\n"
"  -r : Remove internal faces within same material (unless using -a).\n"
"       Any faces adjacent and opposite another face are removed.\n"
"       This includes their opposing neighbor's face.\n"
"  -j : Join verticies within same material (unless using -a)\n"
"       Any vertices sharing a location with another, or within\n"
"       a very small distance, will be reduced to a single vertex.\n"
"       This efectively \"hardens\" or \"joins\" Yland blocks into\n"
"       a single geometry.\n"
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
	config.export_type = ExportType::OBJ;  // TODO: Switch to GLB or GLTF
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
		std::strcpy(missing_arg_expects, "a Type (GLB, GLTF, or OBJ)");
	}
	if (missing_arg) {
		std::cerr << "Arguement missing: \"" << missing_arg_name
				  << "\" expects " << missing_arg_expects << std::endl;
	}

	return config;
}

void doStuff(const Config& config) {
	if (!config.has_input) {
		
	}
}

void extract(const Config& config) {

}

json reformatSceneFlatToNested(const json& data) {

}

void getConfigFromFile(Config& config, const char* filename) {

}

void validateConfigAndPromptForFixes(Config& config, const char* filename) {
	bool config_changed = false;

	std::filesystem::path install_dir(config.ylands_install_dir);
	std::filesystem::path log_path(config.ylands_log_path);
	std::filesystem::path log_fullpath = install_dir / log_path;

	if (!std::filesystem::exists(install_dir) ||
		!std::filesystem::is_directory(install_dir)) {
		std::cout << "\nInvalid Config \"" << CF_KEY_INSTALL_DIR
				  << "\", directory does not exist." << std::endl;
		std::cout << "Enter new path: ";
		std::cin >> config.ylands_install_dir;
	}

	if (!std::filesystem::exists(log_fullpath) ||
		!std::filesystem::is_regular_file(log_fullpath)) {
		std::cout << "\nInvalid Config \""
	}
}
