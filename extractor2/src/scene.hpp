#ifndef SCENE_H
#define SCENE_H

#include <vector>

#include "config.hpp"
#include "space.hpp"
#include "objwavefront.hpp"
#include "json.hpp"
using json = nlohmann::json;

class Node {
public:
	Vector3 position;
	Quaternion rotation;
	std::vector<Node> children;
	Node();
};

class MeshObj : Node{
public:
	ObjWavefront mesh;
};

Node createFromJson(const Config& config, const json& data);

#endif // SCENE_H