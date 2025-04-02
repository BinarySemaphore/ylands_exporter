#include "octree.hpp"

#include "utils.hpp"

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