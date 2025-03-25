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
	this->inherit = true;
	this->parent = nullptr;
	this->type = NodeType::Node;
	this->name = "New Node";
	this->scale = Vector3(1.0f, 1.0f, 1.0f);
}

Vector3 Node::globalPosition() {
	Node* parent = this->parent;
	Vector3 gpos = this->position;
	while (parent != nullptr) {
		gpos = parent->rotation * gpos;
		gpos = gpos + parent->position;
		parent = parent->parent;
	}
	return gpos;
}

Quaternion Node::globalRotation() {
	Node* parent = this->parent;
	Quaternion grot = this->rotation;
	while (parent != nullptr) {
		grot = parent->rotation * grot;
		parent = parent->parent;
	}
	return grot;
}

void Node::addChild(Node* node) {
	if (node->parent != nullptr) throw GeneralException("Node already belongs to parent Node");
	node->parent = this;
	this->children.push_back(node);
}

MeshObj::MeshObj() : Node() {
	this->type = NodeType::MeshObj;
}

void nodeApplyTransforms(Node* current, Node* parent);
void buildScene(Node* parent, const json& root);
MeshObj* createMeshFromRef(const char* ref_key);

Node* createSceneFromJson(const Config& config, const json& data) {
	Node* scene = new Node();

	double s = timerStart();
	std::cout << "Building scene..." << std::endl;

	scene->name = "Scene";
	draw_bb = config.draw_bb;
	draw_bb_transparency = config.draw_bb_transparency;

	try {
		YlandStandard::preloadLookups("lookup.json");
	} catch (CustomException& e) {
		throw LoadException("Failed to load \"lookup.json\": " + std::string(e.what()));
	}

	buildScene(scene, data);
	
	std::cout << "Scene built" << std::endl;
	timerStopMsAndPrint(s);
	std::cout << std::endl;

	return scene;
}

void transformMeshObj(MeshObj* mesh, bool full_transform) {
	int i;
	if (full_transform) {
		if (mesh->parent != NULL) {
			mesh->position = mesh->parent->globalPosition()
						+ mesh->parent->globalRotation() * mesh->position;
			mesh->rotation = mesh->parent->globalRotation() * mesh->rotation;
		}
	}
	for (i = 0; i < mesh->mesh.vert_count; i++) {
		mesh->mesh.verts[i] = mesh->position
							+ (mesh->rotation
								* (mesh->scale * mesh->mesh.verts[i]));
	}
	for (i = 0; i < mesh->mesh.norm_count; i++) {
		// TODO: Use scaling with rotation * (norm * scale).normalized
		mesh->mesh.norms[i] = mesh->rotation * mesh->mesh.norms[i];
	}
}

void nodeApplyTransforms(Node* current, bool full_transform) {
	if (current->type == NodeType::MeshObj) {
		MeshObj* mnode = (MeshObj*)current;
		transformMeshObj(mnode, full_transform);
	}

	for (int i = 0; i < current->children.size(); i++) {
		nodeApplyTransforms(current->children[i], full_transform);
	}
}

void buildScene(Node* parent, const json& root) {
	Vector3 parent_position = parent->globalPosition();
	Quaternion parent_rotation = parent->globalRotation();

	for (auto& [key, item] : root.items()) {
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

			node->name = "[" + key + "] " + (std::string)item["name"];
			node->position = parent_rotation.inverse() * (node->position - parent_position);
			node->rotation = parent_rotation.inverse() * node->rotation;

			//std::lock_guard lock(Workpool::shutex[1]);
			parent->addChild(node);

			if (item.contains("children") && item["children"].size() > 0) {
				buildScene(node, item["children"]);
			}
		}
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
		mesh->mesh.load(((std::string)YlandStandard::lookup["ids"][ref_key]).c_str(), true);
	} else if (YlandStandard::lookup["types"].contains(block_ref["type"])) {
		mesh = new MeshObj();
		mesh->mesh.load(((std::string)YlandStandard::lookup["types"][block_ref["type"]]).c_str(), true);
	} else if (YlandStandard::lookup["shapes"].contains(block_ref["shape"])) {
		mesh = new MeshObj();
		mesh->mesh.load(((std::string)YlandStandard::lookup["shapes"][block_ref["shape"]]).c_str(), true);
		mesh->scale = Vector3(
			(float)block_ref["size"][0],
			(float)block_ref["size"][1],
			(float)block_ref["size"][2]
		);
	} else if (draw_bb) {
		mesh = new MeshObj();
		mesh->mesh.load(((std::string)YlandStandard::lookup["shapes"]["CCUBE"]).c_str(), true);
		Vector3 offset = Vector3(
			(float)block_ref["bb-center-offset"][0],
			(float)block_ref["bb-center-offset"][1],
			-(float)block_ref["bb-center-offset"][2]
		);
		mesh->scale = Vector3(
			(float)block_ref["bb-dimensions"][0],
			(float)block_ref["bb-dimensions"][1],
			(float)block_ref["bb-dimensions"][2]
		);
		mesh->mesh.offset(offset / mesh->scale, true);
		mat.dissolve = draw_bb_transparency;
	}

	if (mesh != NULL) {
		mat.specular = Vector3(0.0f, 0.0f, 0.0f);
		mesh->mesh.setMaterial(mat);
		setEntityColor(*mesh, block_ref["colors"][0].get<std::vector<float>>());
	}

	return mesh;
}