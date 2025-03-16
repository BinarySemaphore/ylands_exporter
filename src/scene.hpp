#ifndef SCENE_H
#define SCENE_H
#pragma once

#include <vector>

#include "space.hpp"
#include "objwavefront.hpp"
#include "json.hpp"
using json = nlohmann::json;

// Forward declaration to avoid using headers and getting multiple redefines
class Config;
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

Node* createSceneFromJson(const Config& config, const json& data);
void buildScene(Node* parent, const json& root);
void createNodeFromItem(Node* parent, const json& item);
MeshObj* createMeshFromRef(const char* ref_key);

#endif // SCENE_H