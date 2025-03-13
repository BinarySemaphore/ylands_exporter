#include "exporter.hpp"

#include <iostream>
#include <fstream>

#include "utils.hpp"
#include "extractor.hpp"
#include "scene.hpp"

int extractAndExport(Config& config) {
	json data;

	// Load Ylands data
	if (!config.has_input) {
		// From Ylands directly
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
		// From input arg (Ylands JSON)
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
	MeshObj* mesh;
	try {
		mesh = createSceneFromJson(config, data);
	} catch (CustomException& e) {
		std::cerr << "Error creating scene: " << e.what() << std::endl;
		return 3;
	}
	// Process scene using config flags

	// OBJ export
	if (config.export_type == ExportType::OBJ) {
		double s = timerStart();
		std::cout << "Exporting [OBJ] file \"" << config.output_filename << "\"..." << std::endl;
		mesh->mesh.save((config.output_filename + ".obj").c_str());
		std::cout << "Export complete" << std::endl;
		timerStopMsAndPrint(s);
		std::cout << std::endl;
	}
	// GLTF export
	// GLB export

	// Finish
	return 0;
}

void exportAsJson(const char* filename, const json& data, bool pprint) {
	char filename_ext[200] = "";
	std::strcat(filename_ext, filename);
	std::strcat(filename_ext, ".json");

	double s = timerStart();
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
	std::cout << "Export complete" << std::endl;
	timerStopMsAndPrint(s);
	std::cout << std::endl;
}