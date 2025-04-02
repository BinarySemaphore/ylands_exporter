#include "reducer.hpp"

#include <vector>
#include <unordered_set>
#include <unordered_map>

#include "utils.hpp"
#include "scene.hpp"
#include "space.hpp"
#include "octree.hpp"

int reduceFaces(Surface& surface) {
	int i, idx1, idx2, idx3, remove_count;
	Face* hold;

	// Ignore dead faces, shifting down as it goes
	remove_count = 0;
	for (i = 0; i < surface.face_count; i++) {
		idx1 = surface.faces[i].vert_index[0];
		idx2 = surface.faces[i].vert_index[1];
		idx3 = surface.faces[i].vert_index[2];
		if (idx1 == idx2 && idx2 == idx3) {
			remove_count += 1;
			continue;
		}
		surface.faces[i - remove_count] = surface.faces[i];
	}

	// Realloc faces unless empty (delete surface)
	surface.face_count -= remove_count;
	if (surface.face_count == 0) {
		surface.clear();
	} else {
		hold = (Face*)realloc(surface.faces, surface.face_count * sizeof(Face));
		if (hold == NULL) {
			throw ReallocException("faces post-join", surface.face_count);
		}
		surface.faces = hold;
	}
	return remove_count;
}

int reduceSurfaces(ObjWavefront& mesh) {
	int i, offset, removed_surfaces;
	Surface* hold;
	std::vector<bool> keep(mesh.surface_count, true);

	// Reduce surface faces, mark surface as removed if empty
	removed_surfaces = 0;
	for (i = 0; i < mesh.surface_count; i++) {
		reduceFaces(mesh.surfaces[i]);
		if (mesh.surfaces[i].face_count == 0) {
			removed_surfaces += 1;
			keep[i] = false;
		}
	}

	// Update surfaces then realloc or free as needed
	offset = 0;
	for (i = 0; i < mesh.surface_count; i++) {
		if (!keep[i]) offset += 1;
		if (i + offset >= mesh.surface_count) break;
		mesh.surfaces[i] = mesh.surfaces[i + offset];
	}
	mesh.surface_count -= removed_surfaces;
	if (mesh.surface_count == 0) {
		free(mesh.surfaces);
	} else {
		hold = (Surface*)realloc(mesh.surfaces, mesh.surface_count * sizeof(Surface));
		if (hold == NULL) {
			throw ReallocException("surfaces post-join", mesh.surface_count);
		}
		mesh.surfaces = hold;
	}

	return removed_surfaces;
}

class VertData {
public:
	int index;
};

int joinVertInMesh(ObjWavefront& mesh, float min_dist) {
	int i, j, k, avg_count, remove_count;
	float dist;
	Surface* surface;
	Face* face;
	Vector3* hold;
	Vector3 diff;
	Vector3 avg;
	std::vector<bool> keep(mesh.vert_count, true);
	std::unordered_map<int, int> index_remap;
	std::unordered_set<OctreeItem<VertData>*> visited;
	std::unordered_set<OctreeItem<VertData>*> unique_neighbors;
	std::vector<OctreeItem<VertData>> items;
	Octree<VertData> octree;

	// TODO: Move octree instances outside of joinVertInMesh (mirror of scene)
	// TODO: Switch octree to be mesh > surface > material > faces (not individual verts)
	for (i = 0; i < mesh.vert_count; i++) {
		items.emplace_back(mesh.verts[i], Vector3());
		items.back().data = new VertData();
		items.back().data->index = i;
	}
	octree = Octree<VertData>(items.data(), items.size());
	octree.subdivide(Vector3(0.01f, 0.01f, 0.01f), 20, 0);

	// Find joinable vertices and build index remapping based on future shift down
	min_dist = min_dist * min_dist;
	for (OctreeItem<VertData>& item : items) {
		// Mark item as visted and reset neighbors checked
		visited.insert(&item);
		unique_neighbors.clear();
		avg_count = 1;
		avg = mesh.verts[item.data->index];
		for (Octree<VertData>* parent : item.parents) {
			for (OctreeItem<VertData>* neighbor : parent->children) {
				// Ignore self / redundant checks
				if (visited.find(neighbor) != visited.end()) continue;
				if (!unique_neighbors.insert(neighbor).second) continue;
				// Check if neighbor position is within min dist (squared distances).
				diff = mesh.verts[item.data->index] - mesh.verts[neighbor->data->index];
				dist = diff.dot(diff);
				if (dist <= min_dist) {
					index_remap[neighbor->data->index] = item.data->index;
					keep[neighbor->data->index] = false;
					avg_count += 1;
					avg = avg + mesh.verts[neighbor->data->index];
				}
			}
		}
		// Merge to avg
		if (avg_count > 1) {
			mesh.verts[item.data->index] = avg * (1.0f / avg_count);
		}
	}
	visited.clear();
	unique_neighbors.clear();
	items.clear();
	octree.clear();

	// Update vertices then realloc
	remove_count = 0;
	for (i = 0; i < mesh.vert_count; i++) {
		// Increment remove_count for vertices marked as removed (not kept).
		// Use i - remove_count to shift the original vertex into the correct position.
		if (!keep[i]) remove_count += 1;
		else mesh.verts[i - remove_count] = mesh.verts[i];
		// For any vertex already in index_remap (merged to point at earlier vertex)
		// update its mapping (due to shift in reference)
		// If a vertex was merged (not kept and already in index_remap),
		// update its mapping to the final reference index, accounting for previous shift.
		if (index_remap.find(i) != index_remap.end()) {
			index_remap[i] = index_remap[index_remap[i]];
		// For untouched vertices, set their mapping based on the current shift.
		} else index_remap[i] = i - remove_count;
	}
	mesh.vert_count = mesh.vert_count - remove_count;
	hold = (Vector3*)realloc(mesh.verts, mesh.vert_count * sizeof(Vector3));
	if (hold == NULL) {
		throw ReallocException("vertices post-join", mesh.vert_count);
	}
	mesh.verts = hold;
	keep.clear();

	// Update surface[].faces indices
	for (i = 0; i < mesh.surface_count; i++) {
		surface = &mesh.surfaces[i];
		for (j = 0; j < surface->face_count; j++) {
			face = &surface->faces[j];
			for(k = 0; k < 3; k++) {
				face->vert_index[k] = index_remap[face->vert_index[k] - 1] + 1;
			}
		}
	}
	index_remap.clear();

	// Remove faces / surfaces that have collapsed
	reduceSurfaces(mesh);

	return remove_count;
}


int joinSceneMeshVerts(Node& scene, float min_dist) {
	int remove_count = 0;
	for (int i = 0; i < scene.children.size(); i++) {
		if (scene.children[i]->type == NodeType::MeshObj) {
			remove_count += joinVertInMesh(((MeshObj*)scene.children[i])->mesh, min_dist);
		} else {
			remove_count += joinSceneMeshVerts(*scene.children[i], min_dist);
		}
	}
	return remove_count;
}

int joinSceneRelatedVerts(Node& scene, float min_dist) {

	return joinSceneMeshVerts(scene, min_dist);
}