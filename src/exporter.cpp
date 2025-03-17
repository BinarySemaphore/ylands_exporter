#include "exporter.hpp"

#include <iostream>
#include <fstream>

#include "utils.hpp"
#include "config.hpp"
#include "extractor.hpp"
#include "scene.hpp"
#include "combomesh.hpp"
#include "workpool.hpp"

// IMPORTANT: When in debug, make sure no_threads is true
Workpool* wp = new Workpool(std::thread::hardware_concurrency() * 50, false, false);

int extractAndExport(Config& config) {
	Node* scene;
	ComboMesh combo;
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

	// Validate and Setup to continue
	if (!isDataValidScene(data)) {
		std::cerr << "Invalid JSON data: expected Ylands SCENE data."
				  << std::endl;
		return 2;
	}
	wp->start();

	// Convert data into 3D Scene
	try {
		scene = createSceneFromJson(config, data);
	} catch (CustomException& e) {
		std::cerr << "Error creating scene: " << e.what() << std::endl;
		return 4;
	}

	// Process scene using config flags

	// OBJ export
	if (config.export_type == ExportType::OBJ) {
		try {
			exportAsObj(config.output_filename.c_str(), *scene);
		} catch (CustomException& e) {
			std::cerr << "Error exporting OBJ file \""
					  << config.output_filename << "\": "
					  << e.what() << std::endl;
			return 3;
		}
	}

	// GLTF export
	
	// GLB export

	// Finish
	return 0;
}

void exportAsJson(const char* filename, const json& data, bool pprint) {
	double s;
	char filename_ext[200] = "";
	
	std::strcat(filename_ext, filename);
	std::strcat(filename_ext, ".json");

	s = timerStart();
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

void exportAsObj(const char* filename, Node& scene) {
	double s;
	MeshObj* combined;
	char filename_ext[200] = "";

	if (scene.type != NodeType::MeshObj) {
		combined = combineMeshFromScene(scene);
	} else {
		combined = (MeshObj*)&scene;
	}

	std::strcat(filename_ext, filename);
	std::strcat(filename_ext, ".obj");

	s = timerStart();
	std::cout << "Exporting [OBJ] file \"" << filename_ext << "\"..." << std::endl;
	combined->mesh.save(filename_ext);
	std::cout << "Export complete" << std::endl;
	timerStopMsAndPrint(s);
	std::cout << std::endl;
}

MeshObj* combineMeshFromScene(Node& scene) {
	ComboMesh* combo;
	MeshObj* combined;

	double s = timerStart();
	std::cout << "Creating single mesh..." << std::endl;
	combo = createComboFromScene(scene);
	combined = combo->commitToMesh();
	std::cout << "Mesh created" << std::endl;
	timerStopMsAndPrint(s);
	std::cout << std::endl;
	/*
	Workpool::shutex[1].lock();
	combo->append(*(MeshObj*)node);
	Workpool::shutex[1].unlock();
	*/
	return combined;
}