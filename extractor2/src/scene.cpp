#include "scene.hpp"

#include <iostream>
#include <fstream>

#include "utils.hpp"
#include "ylands.hpp"
#include "workpool.hpp"

Workpool* wp;

Node::Node() {
	this->position = Vector3();
	this->rotation = Quaternion();
}

MeshObj::MeshObj() {
}

Node createSceneFromJson(const Config& config, const json& data) {
	wp = new Workpool(100);
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
	return scene;
}

void buildScene(Node* parent, const json& root) {
	for (auto& [key, item] : root.items()) {
		wp->addTask(std::bind(createNodeFromItem, parent, item), NULL);
	}
}

void createNodeFromItem(Node* parent, const json& item) {
	Node* node = NULL;

	if (item["type"] == "entity") {
		node = createNewEntityFromRef(((std::string)item["blockdef"]).c_str());
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
			wp->addTask(std::bind(createNodeFromItem, node, item), NULL);
		}
		Workpool::shutex.lock();
		parent->children.push_back(node);
		Workpool::shutex.unlock();
	}
}

Node* createNewEntityFromRef(const char* ref_key) {
	Node* node = NULL;
	MeshObj* mesh = NULL;
	json block_ref;

	if (!YlandStandard::blockdef.contains(ref_key)) {
		std::cout << "No block reference for \"" << ref_key << "\"" << std::endl;
		return NULL;
	}
	block_ref = YlandStandard::blockdef[ref_key];

	if (YlandStandard::lookup["ids"].contains(ref_key)) {
		node = new Node();
		mesh = new MeshObj();
		mesh->mesh.load(((std::string)YlandStandard::lookup["ids"][ref_key]).c_str());
		node->children.push_back(mesh);
	} else if (YlandStandard::lookup["types"].contains(block_ref["type"])) {
		node = new Node();
		mesh = new MeshObj();
		mesh->mesh.load(((std::string)YlandStandard::lookup["types"][block_ref["type"]]).c_str());
		node->children.push_back(mesh);
	} else if (YlandStandard::lookup["shapes"].contains(block_ref["shape"])) {
		node = new Node();
		mesh = new MeshObj();
		mesh->mesh.load(((std::string)YlandStandard::lookup["shapes"][block_ref["shape"]]).c_str());
		node->children.push_back(mesh);
	}

	return node;
}