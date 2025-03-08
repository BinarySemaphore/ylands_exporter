#ifndef COMBOMESH_H
#define COMBOMESH_H
#pragma once

#include <string>
#include <unordered_map>
#include "objwavefront.hpp"
#include "workpool.hpp"

class MeshObj;

class ComboMeshItem {
public:
	int vert_count;
	int norm_count;
	int uv_count;
	int face_count;
	Material material;
	std::vector<int> vert_index_offs;
	std::vector<int> norm_index_offs;
	std::vector<int> uv_index_offs;
	std::vector<MeshObj*> meshes;

	ComboMeshItem();
	bool append(MeshObj& node);
};

class ComboMesh {
public:
	int surface_count;
	std::unordered_map<std::string, ComboMeshItem> cmesh;

	ComboMesh();
	bool append(MeshObj& node);
	MeshObj* commitToMesh(Workpool* wp);

	static std::string getEntityColorUid(MeshObj& entity);
};

#endif // COMBOMESH_H