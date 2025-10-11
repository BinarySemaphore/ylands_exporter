#ifndef OCTREE_H
#define OCTREE_H

#include <cmath>
#include <vector>
#include <unordered_set>

#include "utils.hpp"
#include "space.hpp"

class ObjWavefront;
class Surface;
class Face;
template <typename T>
class Octree;

class VertData {
public:
	int index;
};

class FaceData {
public:
	int face_index;
	ObjWavefront* mesh_ref;
	Surface* surface_ref;
	Face* face_ref;
	Vector3* points[3];
	Vector3 normal;
};

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
	float min_dim;
	AABB space;
	Octree* parent;
	std::vector<Octree<T>> subdivisions;
	std::vector<OctreeItem<T>*> children;
	
	Octree();
	Octree(const AABB& presize);
	Octree(OctreeItem<T>** items, size_t count);
	~Octree();

	void clear();
	void addItem(OctreeItem<T>* new_item);
	void subdivide(int max_depth, int depth);
};

/*
Template Definitions
*/

const int OCTREE_MIN_CHILDREN = 5;
const float OCTREE_SUBDIV_DIR[3] = {0.0f, -1.0f, 1.0f};

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
	this->parent = nullptr;
	this->space = AABB(Vector3(), Vector3());
}

template <typename T>
Octree<T>::Octree(const AABB& presize) : Octree() {
	this->space = presize;
}

template <typename T>
Octree<T>::Octree(OctreeItem<T>** items, size_t count) : Octree() {
	float smallest_sq, check_sq;
	Vector3 min, max;
	std::vector<Vector3> check;
	if (count == 0) {
		throw GeneralException("Cannot build octree here with zero items");
	}
	check.reserve(count * 2);
	// TODO: min_dims should be pointer, so subdivision children
	// 1) don't use extra memory for no reason and
	// 2) if addItem updates min_dims, they all get updated.
	smallest_sq = items[0]->dims.dot(items[0]->dims);
	for (int i = 0; i < count; i++) {
		items[i]->parents.insert(this);
		check.push_back(items[i]->left);
		check.push_back(items[i]->right);
		this->children.push_back(items[i]);
		check_sq = items[i]->dims.dot(items[i]->dims);
		if (check_sq < smallest_sq) smallest_sq = check_sq;
	}

	this->min_dim = std::sqrt(smallest_sq);
	if (this->min_dim < NEAR_ZERO) this->min_dim = NEAR_ZERO;

	getBounds<Vector3>(check.data(), check.size(), min, max);
	this->space = AABB((max + min) * 0.5f, max - min);
}

template <typename T>
Octree<T>::~Octree() {
	this->clear();
}

template <typename T>
void Octree<T>::clear() {
	int i;
	if (this->parent == nullptr) {
		for (i = 0; i < this->children.size(); i++) {
			if (this->children[i] == nullptr) continue;
			delete this->children[i];
		}
	}
	this->children.clear();
	this->subdivisions.clear();
}

template <typename T>
void Octree<T>::addItem(OctreeItem<T>* new_item) {
	// If root, resize if out of bounds
	// TODO: update min_dims
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
void Octree<T>::subdivide(int max_depth, int depth) {
	if (depth >= max_depth) return;
	if (this->children.size() <= OCTREE_MIN_CHILDREN) return;

	int i, j, k, x_start, y_start, z_start;
	Vector3 center;
	Vector3 half_dim;
	Vector3 quarter_dim;

	// Create 0, 2, 4 or 8 new Octrees subdivisions (yeah yeah I know it's an SVO now, but I'm not changing the name rn)
	// If starts stay zero, that respective dimension will not be subdivided
	x_start = 0;
	y_start = 0;
	z_start = 0;
	half_dim = this->space.dims;
	if (this->space.dims.x > this->min_dim) {
		half_dim.x = this->space.dims.x * 0.5f;
		quarter_dim.x = this->space.dims.x * 0.25f;
		x_start = 1;
	}
	if (this->space.dims.y > this->min_dim) {
		half_dim.y = this->space.dims.y * 0.5f;
		quarter_dim.y = this->space.dims.y * 0.25f;
		y_start = 1;
	}
	if (this->space.dims.z > this->min_dim) {
		half_dim.z = this->space.dims.z * 0.5f;
		quarter_dim.z = this->space.dims.z * 0.25f;
		z_start = 1;
	}

	// No subdivision, just exit branch
	if (x_start == 0 && y_start == 0 && z_start == 0) return;

	// Create, place and size subdivisions, any axis _start must be 1 to apply
	for (i = x_start; i <= 2; i++) {
		for (j = y_start; j <= 2; j++) {
			for (k = z_start; k <= 2; k++) {
				center = this->space.center
							+ Vector3(
								quarter_dim.x * OCTREE_SUBDIV_DIR[i],
								quarter_dim.y * OCTREE_SUBDIV_DIR[j],
								quarter_dim.z * OCTREE_SUBDIV_DIR[k]
							);
				this->subdivisions.emplace_back(AABB(center, half_dim));
				this->subdivisions.back().parent = this;
				this->subdivisions.back().min_dim = this->min_dim;
				if (k == 0) break;
			}
			if (j == 0) break;
		}
		if (i == 0) break;
	}

	// Distribute childring into subdivisions
	for (OctreeItem<T>* item : this->children) {
		//item->parents.erase(this);
		for (Octree<T>& div : this->subdivisions) {
			if (item->overlap(div.space)) {
				div.children.push_back(item);
			}
		}
	}
	// Check if subdivisions actually split up children or not
	bool no_change = true;
	for (Octree<T>& div : this->subdivisions) {
		if (div.children.size() < this->children.size()) {
			no_change = false;
			break;
		}
	}
	// Revert and cancel subdivision
	if (no_change) {
		this->subdivisions.clear();
		return;
	// Commit to subdivisions
	} else {
		for (OctreeItem<T>* item : this->children) {
			item->parents.erase(this);
		}
		for (Octree<T>& div : this->subdivisions) {
			for (OctreeItem<T>* item : div.children) {
				item->parents.insert(&div);
			}
		}
	}

	// Continue subdividing
	for (Octree& div : this->subdivisions) {
		div.subdivide(max_depth, depth + 1);
	}
}

ObjWavefront* octreeDebugPrepareMesh();
const float DEBUG_OCTREE_DIRECTION[2] = {-1.0f, 1.0f};
template <typename T>
void octreeDebugAddToMesh(const Octree<T>* octree, ObjWavefront* mesh);

#endif // OCTREE_H