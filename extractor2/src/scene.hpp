#ifndef SCENE_H
#define SCENE_H
#pragma once

#include <vector>

#include "config.hpp"
#include "space.hpp"
#include "objwavefront.hpp"
#include "json.hpp"
using json = nlohmann::json;

class ComboMesh;

enum class NodeType {
	Node,
	MeshObj
};

class Node {
public:
	NodeType type;
	std::string name;
	Vector3 position;
	Vector3 scale;
	Quaternion rotation;
	std::vector<Node*> children;
	Node();
};

class MeshObj : public Node {
public:
	Vector3 offset;
	ObjWavefront mesh;
	MeshObj();
};

MeshObj* createSceneFromJson(const Config& config, const json& data);
void buildScene(Node* parent, const json& root, ComboMesh* combo);
void createNodeFromItem(Node* parent, const json& item, ComboMesh* combo);
MeshObj* createMeshFromRef(const char* ref_key);

#endif // SCENE_H