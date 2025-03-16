#include "scene.hpp"

#include <iostream>
#include <fstream>

#include "utils.hpp"
#include "config.hpp"
#include "exporter.hpp"
#include "objwavefront.hpp"
#include "ylands.hpp"
#include "workpool.hpp"

bool draw_bb;
float draw_bb_transparency;
std::string reported_error;

Node::Node() {
	this->type = NodeType::Node;
	this->name = "New Node";
	this->position = Vector3();
	this->scale = Vector3(1.0f, 1.0f, 1.0f);
	this->rotation = Quaternion();
}

MeshObj::MeshObj() : Node() {
	this->offset = Vector3();
	this->type = NodeType::MeshObj;
}

void errorHandler(std::exception& e) {
	Workpool::shutex[0].lock();
	if (reported_error.size() == 0) {
		reported_error = e.what();
	}
	Workpool::shutex[0].unlock();
}

Node* createSceneFromJson(const Config& config, const json& data) {
	Node* scene = new Node();

	double s = timerStart();
	std::cout << "Building scene..." << std::endl;

	draw_bb = config.draw_bb;
	draw_bb_transparency = config.draw_bb_transparency;

	try {
		YlandStandard::preloadLookups("lookup.json");
	} catch (CustomException& e) {
		throw LoadException("Failed to load \"lookup.json\": " + std::string(e.what()));
	}

	buildScene(scene, data);
	// Must wait on workpool tasks before returning
	wp->wait();

	if (reported_error.size()) {
		throw GeneralException(reported_error);
	}
	std::cout << "Scene built" << std::endl;
	timerStopMsAndPrint(s);
	std::cout << std::endl;

	return scene;
}

void buildScene(Node* parent, const json& root) {
	for (auto& [key, item] : root.items()) {
		wp->addTask(std::bind(createNodeFromItem, parent, item), NULL, errorHandler);
	}
}

void createNodeFromItem(Node* parent, const json& item) {
	Node* node = NULL;

	if (item["type"] == "entity") {
		node = createMeshFromRef(((std::string)item["blockdef"]).c_str());
		if (node != NULL) {
			setEntityColor(*(MeshObj*)node, item["colors"][0].get<std::vector<float>>());
		}
	} else if (item["type"] == "group") {
		node = new Node();
	}

	if (node != NULL) {
		node->position = Vector3(
			(float)item["position"][0],
			(float)item["position"][1],
			-(float)item["position"][2]
		);
		node->rotation.rotate_degrees(
			Vector3(
				-(float)item["rotation"][0],
				-(float)item["rotation"][1],
				(float)item["rotation"][2]
			)
		);

		if (item.contains("children") && item["children"].size() > 0) {
			Workpool::shutex[4].lock();
			buildScene(node, item["children"]);
			Workpool::shutex[4].unlock();
		}
		Workpool::shutex[2].lock();
		parent->children.push_back(node);
		Workpool::shutex[2].unlock();
	}
}

MeshObj* createMeshFromRef(const char* ref_key) {
	MeshObj* mesh = NULL;
	Material mat;
	json block_ref;

	if (!YlandStandard::blockdef.contains(ref_key)) {
		std::cout << "No block reference for \"" << ref_key << "\"" << std::endl;
		return NULL;
	}
	block_ref = YlandStandard::blockdef[ref_key];

	// Note: All models should be build z-inverted
	// Recommend using existing models as reference
	// For blender: models will extend +Z, -X, -Y)

	if (YlandStandard::lookup["ids"].contains(ref_key)) {
		mesh = new MeshObj();
		Workpool::shutex[3].lock();
		mesh->mesh.load(((std::string)YlandStandard::lookup["ids"][ref_key]).c_str(), true);
		Workpool::shutex[3].unlock();
	} else if (YlandStandard::lookup["types"].contains(block_ref["type"])) {
		mesh = new MeshObj();
		Workpool::shutex[3].lock();
		mesh->mesh.load(((std::string)YlandStandard::lookup["types"][block_ref["type"]]).c_str(), true);
		Workpool::shutex[3].unlock();
	} else if (YlandStandard::lookup["shapes"].contains(block_ref["shape"])) {
		mesh = new MeshObj();
		Workpool::shutex[3].lock();
		mesh->mesh.load(((std::string)YlandStandard::lookup["shapes"][block_ref["shape"]]).c_str(), true);
		Workpool::shutex[3].unlock();
		mesh->scale = Vector3(
			(float)block_ref["size"][0],
			(float)block_ref["size"][1],
			(float)block_ref["size"][2]
		);
	} else if (draw_bb) {
		mesh = new MeshObj();
		Workpool::shutex[3].lock();
		mesh->mesh.load(((std::string)YlandStandard::lookup["shapes"]["CCUBE"]).c_str(), true);
		Workpool::shutex[3].unlock();
		// TODO: confirm not scaling negative x and y is okay, it should be fine.
		mesh->scale = Vector3(
			(float)block_ref["bb-dimensions"][0],
			(float)block_ref["bb-dimensions"][1],
			(float)block_ref["bb-dimensions"][2]
		);
		mesh->offset = Vector3(
			(float)block_ref["bb-center-offset"][0],
			(float)block_ref["bb-center-offset"][1],
			-(float)block_ref["bb-center-offset"][2]
		);
		mat.dissolve = draw_bb_transparency;
	}

	if (mesh != NULL) {
		mat.specular = Vector3(0.0f, 0.0f, 0.0f);
		mesh->mesh.setMaterial(mat);
		setEntityColor(*mesh, block_ref["colors"][0].get<std::vector<float>>());
	}

	return mesh;
}