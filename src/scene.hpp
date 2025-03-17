#ifndef SCENE_H
#define SCENE_H
#pragma once

#include <string>
#include <vector>

#include "space.hpp"
#include "objwavefront.hpp"
#include "json.hpp"
using json = nlohmann::json;

// Forward declaration to avoid using headers and getting multiple redefines
class Config;

enum class NodeType {
	Node,
	MeshObj
};

class Node {
public:
	bool inherit;
	bool has_parent;
	NodeType type;
	std::string name;
	Vector3 position;
	Vector3 scale;
	Quaternion rotation;
	std::vector<Node*> children;
	Node();

	void addChild(Node* node);
};

class MeshObj : public Node {
public:
	Vector3 offset;
	ObjWavefront mesh;
	MeshObj();
};

Node* createSceneFromJson(const Config& config, const json& data);

#endif // SCENE_H