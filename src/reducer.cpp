#include "reducer.hpp"

#include <vector>
#include <unordered_set>
#include <unordered_map>

#include "utils.hpp"
#include "scene.hpp"
#include "space.hpp"
#include "octree.hpp"

const float MIN_TRIANGLE_AREA_SQ = NEAR_ZERO * NEAR_ZERO;

int reduceFaces(ObjWavefront& mesh, Surface& surface) {
	int i, remove_count;
	float squared_area;
	Vector3 *p1, *p2, *p3;
	Face* hold;
	Vector3 area;

	// Ignore degenerate triangle faces, shifting to remove
	// Use squared area to check
	remove_count = 0;
	for (i = 0; i < surface.face_count; i++) {
		p1 = &mesh.verts[surface.faces[i].vert_index[0] - 1];
		p2 = &mesh.verts[surface.faces[i].vert_index[1] - 1];
		p3 = &mesh.verts[surface.faces[i].vert_index[2] - 1];
		area = (*p2 - *p1).cross(*p3 - *p1);
		squared_area = area.dot(area) * 0.25f;
		if (squared_area <= MIN_TRIANGLE_AREA_SQ) {
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
		reduceFaces(mesh, mesh.surfaces[i]);
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

// class VertData {
// public:
// 	int index;
// };

int joinVertInMesh(Octree<FaceData>& octree, std::unordered_set<ObjWavefront*>& meshes, float min_dist) {
	int i, j, k, idx1, idx2, remove_count, avg_count;
	float min_dist_sq, dist_sq;
	Vector3 avg, diff;
	Vector3* hold;
	Surface* surface;
	Face* face;
	Face* nbr_face;
	ObjWavefront* mesh;
	std::vector<bool> keep;
	std::unordered_set<int> visited_idx, unique_neighbors_idx;
	std::unordered_set<OctreeItem<FaceData>*> visited, unique_neighbors;
	std::unordered_map<int, int> index_remap;

	if (octree.children.size() == 0) return 0;

	min_dist_sq = min_dist * min_dist;
	mesh = octree.children[0]->data->mesh_ref;
	if(!meshes.insert(mesh).second) return 0;
	keep.reserve(mesh->vert_count);
	for (i = 0; i < mesh->vert_count; i++) {
		keep.push_back(true);
	}

	for (OctreeItem<FaceData>* item : octree.children) {
		visited.insert(item);
		// mesh = item->data->mesh_ref;
		face = item->data->face_ref;
		for (i = 0; i < 3; i++) {
			idx1 = face->vert_index[i] - 1;
			visited_idx.insert(idx1);
			unique_neighbors.clear();
			unique_neighbors_idx.clear();
			if (!keep[idx1]) continue;
			avg = mesh->verts[idx1];
			avg_count = 1;
			// TODO: check own verts too
			for (j = i + 1; j < 3; j++) {
				idx2 = face->vert_index[j] - 1;
				if (!unique_neighbors_idx.insert(idx2).second) continue;
				diff = mesh->verts[idx1]
					 - mesh->verts[idx2];
				dist_sq = diff.dot(diff);
				if (dist_sq <= min_dist_sq) {
					index_remap[idx2] = idx1;
					keep[idx2] = false;
					avg = avg + mesh->verts[idx2];
					avg_count += 1;
				}
			}
			for (Octree<FaceData>* parent : item->parents) {
				for (OctreeItem<FaceData>* neighbor : parent->children) {
					if (visited.find(neighbor) != visited.end()) continue;
					if (!unique_neighbors.insert(neighbor).second) continue;
					if (neighbor->data->mesh_ref != mesh) {
						throw GeneralException("Mismatch meshes in joinVertInMesh: unexpected");
					}
					nbr_face = neighbor->data->face_ref;
					for (k = 0; k < 3; k++) {
						idx2 = nbr_face->vert_index[k] - 1;
						if (visited_idx.find(idx2) != visited_idx.end()) continue;
						if (!unique_neighbors_idx.insert(idx2).second) continue;
						diff = mesh->verts[idx1]
							 - mesh->verts[idx2];
						dist_sq = diff.dot(diff);
						if (dist_sq <= min_dist_sq) {
							index_remap[idx2] = idx1;
							keep[idx2] = false;
							avg = avg + mesh->verts[idx2];
							avg_count += 1;
						}
					}
				}
			}
			if (avg_count > 1) {
				mesh->verts[idx1] = avg * (1.0f / avg_count);
			}
		}
	}
	
	// Update vertices then realloc
	remove_count = 0;
	for (i = 0; i < mesh->vert_count; i++) {
		// Increment remove_count for vertices marked as removed (not kept).
		// Use i - remove_count to shift the original vertex into the correct position.
		if (!keep[i]) remove_count += 1;
		else mesh->verts[i - remove_count] = mesh->verts[i];
		// For any vertex already in index_remap (merged to point at earlier vertex)
		// update its mapping (due to shift in reference)
		// If a vertex was merged (not kept and already in index_remap),
		// update its mapping to the final reference index, accounting for previous shift.
		if (index_remap.find(i) != index_remap.end()) {
			index_remap[i] = index_remap[index_remap[i]];
		// For untouched vertices, set their mapping based on the current shift.
		} else index_remap[i] = i - remove_count;
	}
	if (remove_count > 0) {
		mesh->vert_count = mesh->vert_count - remove_count;
		hold = (Vector3*)realloc(mesh->verts, mesh->vert_count * sizeof(Vector3));
		if (hold == NULL) {
			throw ReallocException("vertices post-join", mesh->vert_count);
		}
		mesh->verts = hold;
		keep.clear();

		// Update surface[].faces indices
		for (i = 0; i < mesh->surface_count; i++) {
			surface = &mesh->surfaces[i];
			for (j = 0; j < surface->face_count; j++) {
				face = &surface->faces[j];
				for(k = 0; k < 3; k++) {
					face->vert_index[k] = index_remap[face->vert_index[k] - 1] + 1;
				}
			}
		}
	}
	index_remap.clear();
	/*
	// Find joinable vertices and build index remapping based on future shift down
	// Use squared distance to check
	min_dist = min_dist * min_dist;
	for (OctreeItem<VertData>* item : items) {
		// Mark item as visted and reset neighbors checked
		visited.insert(item);
		unique_neighbors.clear();
		avg_count = 1;
		avg = mesh.verts[item->data->index];
		for (Octree<VertData>* parent : item->parents) {
			for (OctreeItem<VertData>* neighbor : parent->children) {
				// Ignore self / redundant checks
				if (visited.find(neighbor) != visited.end()) continue;
				if (!unique_neighbors.insert(neighbor).second) continue;
				// Check if neighbor position is within min dist (squared distances).
				diff = mesh.verts[item->data->index] - mesh.verts[neighbor->data->index];
				dist = diff.dot(diff);
				if (dist <= min_dist) {
					index_remap[neighbor->data->index] = item->data->index;
					keep[neighbor->data->index] = false;
					avg_count += 1;
					avg = avg + mesh.verts[neighbor->data->index];
				}
			}
		}
		// Merge to avg
		if (avg_count > 1) {
			mesh.verts[item->data->index] = avg * (1.0f / avg_count);
		}
	}
	visited.clear();
	unique_neighbors.clear();
	items.clear();
	delete octree;

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

	*/
	return remove_count;
}

// TODO: replace scene with octscene
int joinSceneRelatedVerts(TNode<Octree<FaceData>>& octscene, float min_dist) {
	int remove_count = 0;
	MeshObj* mnode;
	std::unordered_set<ObjWavefront*> meshes_reduced;
	std::vector<int> empty_meshes;

	if (octscene.data != nullptr) {
		remove_count += joinVertInMesh(*octscene.data, meshes_reduced, min_dist);
	}
	for (TNode<Octree<FaceData>>* node : octscene.children) {
		remove_count += joinSceneRelatedVerts(*node, min_dist);
	}
	for (ObjWavefront* mesh : meshes_reduced) {
		reduceSurfaces(*mesh);
	}
	// Remove child mesh objects that were reduced to nothing
	// for (i = empty_meshes.size() - 1; i >= 0; i--) {
	// 	delete scene.children[empty_meshes[i]];
	// 	scene.children.erase(scene.children.begin() + empty_meshes[i]);
	// }
	return remove_count;
}