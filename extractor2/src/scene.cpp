#include "scene.hpp"

#include <iostream>
#include <fstream>

#include "utils.hpp"
#include "ylands.hpp"
#include "workpool.hpp"

Workpool* wp;
std::string reported_error;

Node::Node() {
	this->type = NodeType::Node;
	this->name = "New Node";
	this->position = Vector3();
	this->scale = Vector3();
	this->rotation = Quaternion();
}

MeshObj::MeshObj() : Node() {
	this->type = NodeType::MeshObj;
}

void errorHandler(std::exception& e) {
	Workpool::shutex.lock();
	if (reported_error.size() == 0) {
		reported_error = e.what();
	}
	Workpool::shutex.unlock();
}

Node createSceneFromJson(const Config& config, const json& data) {
	wp = new Workpool(100, false, true);
	Node scene = Node();

	try {
		YlandStandard::preloadLookups("lookup.json");
	} catch (CustomException& e) {
		throw LoadException("Failed to load \"lookup.json\": " + std::string(e.what()));
	}

	wp->start();
	buildScene(&scene, data);
	wp->stop();

	delete wp;
	if (reported_error.size()) {
		throw GeneralException(reported_error);
	}

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
			// Set color
		}
	} else if (item["type"] == "group") {
		node = new Node();
	}

	if (node != NULL) {
		node->position = Vector3(
			(float)item["position"][0],
			(float)item["position"][1],
			(float)item["position"][2]
		);
		node->rotation.rotate_degrees(
			Vector3(
				(float)item["rotation"][0],
				(float)item["rotation"][1],
				(float)item["rotation"][2]
			)
		);

		if (item.contains("children") && item["children"].size() > 0) {
			buildScene(node, item["children"]);
		}
		Workpool::shutex.lock();
		parent->children.push_back(node);
		Workpool::shutex.unlock();
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

	if (YlandStandard::lookup["ids"].contains(ref_key)) {
		mesh = new MeshObj();
		Workpool::shutex.lock();
		mesh->mesh.load(((std::string)YlandStandard::lookup["ids"][ref_key]).c_str(), true);
		Workpool::shutex.unlock();
	} else if (YlandStandard::lookup["types"].contains(block_ref["type"])) {
		mesh = new MeshObj();
		Workpool::shutex.lock();
		mesh->mesh.load(((std::string)YlandStandard::lookup["types"][block_ref["type"]]).c_str(), true);
		Workpool::shutex.unlock();
	} else if (YlandStandard::lookup["shapes"].contains(block_ref["shape"])) {
		mesh = new MeshObj();
		Workpool::shutex.lock();
		mesh->mesh.load(((std::string)YlandStandard::lookup["shapes"][block_ref["shape"]]).c_str(), true);
		Workpool::shutex.unlock();
		mesh->scale = Vector3(
			(float)block_ref["size"][0],
			(float)block_ref["size"][1],
			(float)block_ref["size"][2]
		);
	}

	if (mesh != NULL) {
		mesh->mesh.setMaterial(mat);
		setEntityColor(*mesh, (std::vector<float>)block_ref["colors"].array());
	}

	return mesh;
}