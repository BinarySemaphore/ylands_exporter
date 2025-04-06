#include "octree.hpp"

#include "utils.hpp"
#include "objwavefront.hpp"

const Vector3 NEAR_ZERO_V3(NEAR_ZERO, NEAR_ZERO, NEAR_ZERO);

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