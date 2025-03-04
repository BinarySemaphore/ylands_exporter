#ifndef EXPORTER_H
#define EXPORTER_H

#include <string>

enum class ExportType {
	GLB,
	GLTF,
	OBJ
};

class Config {
public:
	bool remove_faces;
	bool join_verts;
	bool apply_all;
	bool preload;
	bool has_input;
	ExportType export_type;
	std::string preload_filename;
	std::string input_filename;
	std::string output_filename;
};

void printHelp(const char* pgm_name);
void printHelp(const char* pgm_name, bool version_only);
Config getConfigFromArgs(int argc, char** argv);

#endif // EXPORTER_H