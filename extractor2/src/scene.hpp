#ifndef SCENE_H
#define SCENE_H

#include <vector>

#include "config.hpp"
#include "space.hpp"
#include "objwavefront.hpp"
#include "json.hpp"
using json = nlohmann::json;

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
	ObjWavefront mesh;
	MeshObj();
};

Node createSceneFromJson(const Config& config, const json& data);
void buildScene(Node* parent, const json& root);
void createNodeFromItem(Node* parent, const json& item);
MeshObj* createMeshFromRef(const char* ref_key);

#endif // SCENE_H