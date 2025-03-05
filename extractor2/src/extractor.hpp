#ifndef EXPORTER_H
#define EXPORTER_H

#include <string>

#include "json.hpp"
using json = nlohmann::json;

enum class ExportType {
	GLB,
	GLTF,
	OBJ
};

class Config {
public:
	bool ext_auto_nest;
	bool ext_pprint;
	bool remove_faces;
	bool join_verts;
	bool apply_all;
	bool preload;
	bool has_input;
	ExportType export_type;
	std::string preload_filename;
	std::string input_filename;
	std::string output_filename;
	std::string ylands_install_dir;
	std::string ylands_log_path;
};

void printHelp(const char* pgm_name);
void printHelp(const char* pgm_name, bool version_only);
Config getConfigFromArgs(int argc, char** argv);

void doStuff(const Config& config);
void extract(const Config& config);
json reformatSceneFlatToNested(const json& data);
void getConfigFromFile(Config& config, const char* filename);
void validateConfigAndPromptForFixes(Config& config, const char* filename);


#endif // EXPORTER_H