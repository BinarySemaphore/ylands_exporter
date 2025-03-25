#ifndef CONFIG_H
#define CONFIG_H

#include <string>

extern char PGM_NAME[128];
extern const char* PGM_NAME_READABLE;
extern const char* PGM_VERSION;
extern const char* PGM_REF_LINK;
extern const char* CONFIG_FILE;

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
	bool combine;
	bool preload;
	bool has_input;
	bool draw_bb;
	ExportType export_type;
	float draw_bb_transparency;
	std::string preload_filename;
	std::string input_filename;
	std::string output_filename;
	std::string ylands_install_dir;
	std::string ylands_log_path;
};

void printHelp();
void printHelp(bool version_only);

Config getConfigFromArgs(int argc, char** argv);

void updateConfigFromFile(Config& config, const char* filename);
void validateConfigAndPromptForFixes(Config& config, const char* filename, bool config_changed);

#endif // CONFIG_H