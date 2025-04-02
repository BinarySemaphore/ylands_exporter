#ifndef OCTREE_H
#define OCTREE_H

#include <vector>
#include <unordered_set>

#include "utils.hpp"
#include "space.hpp"

template <typename T>
class Octree;

class AABB {
public:
	Vector3 center;
	Vector3 dims;
	Vector3 left;
	Vector3 right;

	AABB();
	AABB(const Vector3& center, const Vector3& dims);

	bool overlap(const AABB& other);
};

template <typename T>
class OctreeItem : public AABB {
public:
	T* data;
	std::unordered_set<Octree<T>*> parents;

	OctreeItem(const Vector3& center, const Vector3& dims);
	OctreeItem(const Vector3& center);
};

template <typename T>
class Octree {
public:
	int max_children;
	Octree* parent;
	AABB space;
	std::vector<Octree<T>> subdivisions;
	std::vector<OctreeItem<T>*> children;
	
	Octree();
	Octree(const AABB& presize);
	Octree(OctreeItem<T>* items, size_t count);
	~Octree();

	void clear();
	void addItem(OctreeItem<T>* new_item);
	void subdivide(const Vector3& min_dims, int max_depth, int depth);
};

/*
Template Definitions
*/

const float OCTREE_SUBDIV_DIR[2] = {-1.0f, 1.0f};

template <typename T>
OctreeItem<T>::OctreeItem(const Vector3& center, const Vector3& dims) : AABB(center, dims) {
	// Empty
}

template <typename T>
OctreeItem<T>::OctreeItem(const Vector3& center) : AABB(center, Vector3(1.0f, 1.0f, 1.0f)) {
	// Empty
}

template <typename T>
Octree<T>::Octree() {
	this->max_children = 5;
	this->parent = nullptr;
	this->space = AABB(Vector3(), Vector3());
}

template <typename T>
Octree<T>::Octree(const AABB& presize) : Octree() {
	this->space = presize;
}

template <typename T>
Octree<T>::Octree(OctreeItem<T>* items, size_t count) : Octree() {
	Vector3 min, max;
	std::vector<Vector3> check(count * 2);
	for (int i = 0; i < count; i++) {
		items[i].parents.insert(this);
		check.push_back(items[i].left);
		check.push_back(items[i].right);
		this->children.push_back(&items[i]);
	}
	getBounds<Vector3>(check.data(), check.size(), min, max);
	this->space = AABB((max + min) * 0.5f, max - min);
}

template <typename T>
Octree<T>::~Octree() {
	this->clear();
}

template <typename T>
void Octree<T>::clear() {
	this->subdivisions.clear();
	this->children.clear();
}

template <typename T>
void Octree<T>::addItem(OctreeItem<T>* new_item) {
	// If root, resize if out of bounds
	if (this->parent == nullptr) {
		Vector3 min, max;
		std::vector<Vector3> check = {
			this->space.left,
			this->space.right,
			new_item->left,
			new_item->right
		};
		getBounds<Vector3>(check.data(), check.size(), min, max);
		this->space = AABB((max + min) * 0.5f, max - min);
		// TODO: update subdivisions space
	}
	new_item->parents.insert(this);
	this->children.push_back(new_item);
	// Add octree to item's parents
	// TODO: walk item down to furthest subdivision then call on that division only
	//this->subdivide();
}

template <typename T>
void Octree<T>::subdivide(const Vector3& min_dims, int max_depth, int depth) {
	if (depth >= max_depth) return;
	if (this->children.size() <= this->max_children) return;
	if (this->space.dims.x < min_dims.x ||
		this->space.dims.y < min_dims.y ||
		this->space.dims.z < min_dims.z) return;
	
	int i, j, k;

	// Create 8 new Octrees subdivisions
	Vector3 center;
	Vector3 half_dim = this->space.dims * 0.5f;
	Vector3 quarter_dim = this->space.dims * 0.25f;

	for (i = 0; i <= 1; i++) {
		for (j = 0; j <= 1; j++) {
			for (k = 0; k <= 1; k++) {
				center = this->space.center
							+ Vector3(
								quarter_dim.x * OCTREE_SUBDIV_DIR[i],
								quarter_dim.y * OCTREE_SUBDIV_DIR[j],
								quarter_dim.z * OCTREE_SUBDIV_DIR[k]
							);
				this->subdivisions.emplace_back(AABB(center, half_dim));
				this->subdivisions.back().parent = this;
			}
		}
	}

	// Distribute childring into subdivisions
	for (OctreeItem<T>* item : this->children) {
		item->parents.erase(this);
		for (Octree<T>& div : this->subdivisions) {
			if (item->overlap(div.space)) {
				div.children.push_back(item);
				item->parents.insert(&div);
			}
		}
	}

	// Continue subdividing
	for (Octree& div : this->subdivisions) {
		div.subdivide(min_dims, max_depth, depth + 1);
	}
}

#endif // OCTREE_H