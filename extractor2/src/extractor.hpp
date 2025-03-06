#ifndef EXPORTER_H
#define EXPORTER_H

#include <string>

#include "json.hpp"
using json = nlohmann::json;

enum class ExportType {
	JSON,
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
	std::string blockdef_filename;
	std::string ylands_install_dir;
	std::string ylands_log_path;
};

void printHelp(const char* pgm_name);
void printHelp(const char* pgm_name, bool version_only);
Config getConfigFromArgs(int argc, char** argv);

int extractAndExport(Config& config);
void exportAsJson(const char* filename, const json& data, bool pprint);
// More export options here...

void loadFromFile(const char* filename, json& data);
void extractFromYlands(Config& config, json& data);

void reformatSceneFlatToNested(json& data);
void updateConfigFromFile(Config& config, const char* filename);
void validateConfigAndPromptForFixes(Config& config, const char* filename, bool config_changed);

bool isDataBlockDef(const json& data);
bool isDataValidScene(const json& data);

#endif // EXPORTER_H