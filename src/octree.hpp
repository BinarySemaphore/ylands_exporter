#ifndef OCTREE_H
#define OCTREE_H

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

	this->min_dim = std::sqrtf(smallest_sq);
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

	// Create 2 to 8 new Octrees subdivisions (yeah yeah I know it's an SVO now, but I'm not changing the name rn)
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
	// No subdivision, just stop
	if (this->subdivisions.size() == 0) return;

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

#endif // OCTREE_H