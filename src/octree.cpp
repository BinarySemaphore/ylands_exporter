#include "octree.hpp"

#include "utils.hpp"
#include "objwavefront.hpp"

// Explicitly not using NEAR_ZERO from utils; AABB::overlap wants buffer room
// to handle octree neighbors on nearly perfect divisions.
const Vector3 NEAR_ZERO_V3(0.001f, 0.001f, 0.001f);

AABB::AABB() {
	// Empty
}

AABB::AABB(const Vector3& center, const Vector3& dims) {
	Vector3 half_dims = dims * 0.5f;
	this->center = center;
	this->dims = dims;
	this->left = center - half_dims;
	this->right = center + half_dims;
}

bool AABB::overlap(const AABB& other) {
	Vector3 dist = this->center - other.center;
	Vector3 max_dist = (this->dims + other.dims) * 0.5f + NEAR_ZERO_V3;
	if (std::abs(dist.x) > max_dist.x) return false;
	if (std::abs(dist.y) > max_dist.y) return false;
	if (std::abs(dist.z) > max_dist.z) return false;
	return true;
}

ObjWavefront* octreeDebugPrepareMesh() {
	ObjWavefront* mesh = new ObjWavefront();
	Material* mat = new Material("mat_01");
	mat->diffuse = Vector3(0.25f, 0.9f, 0.9f);
	mat->dissolve = 0.01f;
	mesh->materials[mat->name] = *mat;
	mesh->vert_count = 0;
	mesh->norm_count = 6;
	mesh->norms = (Vector3*)malloc(sizeof(Vector3) * 6);
	mesh->norms[0] = Vector3(0.0f, 1.0f, 0.0f);
	mesh->norms[1] = Vector3(0.0f, -1.0f, 0.0f);
	mesh->norms[2] = Vector3(1.0f, 0.0f, 0.0f);
	mesh->norms[3] = Vector3(-1.0f, 0.0f, 0.0f);
	mesh->norms[4] = Vector3(0.0f, 0.0f, 1.0f);
	mesh->norms[5] = Vector3(0.0f, 0.0f, -1.0f);
	return mesh;
}

template <typename T>
void octreeDebugAddToMesh(const Octree<T>* octree, ObjWavefront* mesh) {
	int i, j, k;
	Vector3 offset;
	Vector3 half_dims = octree->space.dims * 0.5f;
	std::vector<Vector3> nv;
	nv.reserve(8);
	/*
	1 : <0, 0, 0>
	2 : <0, 0, 1>
	3 : <0, 1, 0>
	4 : <0, 1, 1>
	5 : <1, 0, 0>
	6 : <1, 0, 1>
	7 : <1, 1, 0>
	8 : <1, 1, 1>
	*/
	for (i = 0; i < 2; i++) {
		for (j = 0; j < 2; j++) {
			for (k = 0; k < 2; k++) {
				offset.x = half_dims.x * DEBUG_OCTREE_DIRECTION[i];
				offset.y = half_dims.y * DEBUG_OCTREE_DIRECTION[j];
				offset.z = half_dims.z * DEBUG_OCTREE_DIRECTION[k];
				nv.push_back(octree->space.center + offset);
			}
		}
	}
	mesh->verts = (Vector3*)realloc(mesh->verts, sizeof(Vector3) * (mesh->vert_count + 8));
	for (i = 0; i < 8; i++) {
		mesh->verts[i + mesh->vert_count] = nv[i];
	}
	mesh->surfaces = (Surface*)realloc(mesh->surfaces, sizeof(Surface) * (mesh->surface_count + 1));
	mesh->surfaces[mesh->surface_count].material_refs = new std::unordered_map<int, std::string>();
	(*mesh->surfaces[mesh->surface_count].material_refs)[0] = std::string("mat_01");
	mesh->surfaces[mesh->surface_count].faces = (Face*)malloc(sizeof(Face) * 12);
	mesh->surfaces[mesh->surface_count].face_count = 12;
	// Bottom faces
	for (i = 0; i < 2; i++) {
		for (j = 0; j < 3; j++) {
			mesh->surfaces[mesh->surface_count].faces[i].norm_index[j] = 2;
		}
	}
	mesh->surfaces[mesh->surface_count].faces[0].vert_index[0] = 1 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[0].vert_index[1] = 2 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[0].vert_index[2] = 5 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[1].vert_index[0] = 6 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[1].vert_index[1] = 5 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[1].vert_index[2] = 2 + mesh->vert_count;
	// Top faces
	for (i = 2; i < 4; i++) {
		for (j = 0; j < 3; j++) {
			mesh->surfaces[mesh->surface_count].faces[i].norm_index[j] = 1;
		}
	}
	mesh->surfaces[mesh->surface_count].faces[2].vert_index[0] = 3 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[2].vert_index[1] = 4 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[2].vert_index[2] = 7 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[3].vert_index[0] = 8 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[3].vert_index[1] = 7 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[3].vert_index[2] = 4 + mesh->vert_count;
	// Right faces
	for (i = 4; i < 6; i++) {
		for (j = 0; j < 3; j++) {
			mesh->surfaces[mesh->surface_count].faces[i].norm_index[j] = 3;
		}
	}
	mesh->surfaces[mesh->surface_count].faces[4].vert_index[0] = 5 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[4].vert_index[1] = 6 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[4].vert_index[2] = 8 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[5].vert_index[0] = 8 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[5].vert_index[1] = 7 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[5].vert_index[2] = 5 + mesh->vert_count;
	// Left faces
	for (i = 6; i < 8; i++) {
		for (j = 0; j < 3; j++) {
			mesh->surfaces[mesh->surface_count].faces[i].norm_index[j] = 4;
		}
	}
	mesh->surfaces[mesh->surface_count].faces[6].vert_index[0] = 1 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[6].vert_index[1] = 2 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[6].vert_index[2] = 4 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[7].vert_index[0] = 4 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[7].vert_index[1] = 3 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[7].vert_index[2] = 1 + mesh->vert_count;
	// Front faces
	for (i = 8; i < 10; i++) {
		for (j = 0; j < 3; j++) {
			mesh->surfaces[mesh->surface_count].faces[i].norm_index[j] = 5;
		}
	}
	mesh->surfaces[mesh->surface_count].faces[8].vert_index[0] = 2 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[8].vert_index[1] = 6 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[8].vert_index[2] = 8 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[9].vert_index[0] = 8 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[9].vert_index[1] = 4 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[9].vert_index[2] = 2 + mesh->vert_count;
	// Back faces
	for (i = 10; i < 12; i++) {
		for (j = 0; j < 3; j++) {
			mesh->surfaces[mesh->surface_count].faces[i].norm_index[j] = 6;
		}
	}
	mesh->surfaces[mesh->surface_count].faces[10].vert_index[0] = 1 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[10].vert_index[1] = 5 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[10].vert_index[2] = 7 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[11].vert_index[0] = 7 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[11].vert_index[1] = 3 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[11].vert_index[2] = 1 + mesh->vert_count;
	mesh->surface_count += 1;
	mesh->vert_count += 8;
	for (const Octree<FaceData>& div : octree->subdivisions) {
		octreeDebugAddToMesh<T>(&div, mesh);
	}
}