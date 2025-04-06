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
	std::vector<OctreeItem<VertData>*> items;
	Octree<VertData>* octree;

	// TODO: Move octree instances outside of joinVertInMesh (mirror of scene)
	// TODO: Switch octree to be mesh > surface > material > faces (not individual verts)
	for (i = 0; i < mesh.vert_count; i++) {
		items.push_back(new OctreeItem<VertData>(mesh.verts[i], Vector3()));
		items.back()->data = new VertData();
		items.back()->data->index = i;
	}
	octree = new Octree<VertData>(items.data(), items.size());
	octree->min_dims = Vector3(1.0f, 1.0f, 1.0f) * min_dist;
	octree->subdivide(20, 0);

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

	return remove_count;
}

int removeFacesInMesh(ObjWavefront& mesh) {
	int i, j, k, remove_count;
	float plane_dist;
	float one_third = 1.0f / 3.0f;
	FaceData* face_data;
	Vector3* normal_filter;
	Vector3 min, max, center, grp_max_diff, grp_min_diff, area_diff;
	Vector3 points[3];
	std::string cand_key;
	std::vector<Vector3> grp_1;
	std::vector<Vector3> grp_2;
	std::unordered_map<std::string, std::unordered_set<FaceData*>> candidates;
	std::unordered_map<Surface*, std::unordered_set<int>> faces_to_keep_debug;
	std::unordered_map<Surface*, std::unordered_set<int>> faces_to_remove;
	std::unordered_set<OctreeItem<FaceData>*> visited;
	std::unordered_set<OctreeItem<FaceData>*> unique_neighbors;
	std::vector<OctreeItem<FaceData>*> items;
	Octree<FaceData>* octree;

	//ObjWavefront* debug_mesh = octreeDebugPrepareMesh();

	remove_count = 0;
	for (i = 0; i < mesh.surface_count; i++) {
		items.reserve(mesh.surfaces[i].face_count);
		for (j = 0; j < mesh.surfaces[i].face_count; j++) {
			face_data = new FaceData();
			face_data->mesh_ref = &mesh;
			face_data->surface_ref = &mesh.surfaces[i];
			face_data->face_ref = &mesh.surfaces[i].faces[j];
			face_data->face_index = j;
			for (k = 0; k < 3; k++) {
				face_data->points[k] = &mesh.verts[face_data->face_ref->vert_index[k] - 1];
				points[k] = *face_data->points[k];
			}
			face_data->normal = (points[1] - points[0]).cross(points[2] - points[0]);
			face_data->normal = face_data->normal * (1.0f / sqrtf(face_data->normal.dot(face_data->normal)));
			getBounds<Vector3>(points, 3, min, max);
			center = (points[0] + points[1] + points[2]) * one_third;
			items.push_back(new OctreeItem<FaceData>(center, max - min));
			items.back()->data = face_data;
		}
		octree = new Octree<FaceData>(items.data(), items.size());
		octree->subdivide(20, 0);
		items.clear();
		//octreeDebugAddToMesh<FaceData>(octree, debug_mesh);

		for (OctreeItem<FaceData>* item : octree->children) {
			// Mark item as visted and reset neighbors checked
			visited.insert(item);
			unique_neighbors.clear();
			cand_key = "";
			for (Octree<FaceData>* parent : item->parents) {
				for (OctreeItem<FaceData>* neighbor : parent->children) {
					// Ignore self / redundant checks
					if (visited.find(neighbor) != visited.end()) continue;
					if (!unique_neighbors.insert(neighbor).second) continue;
					// Check near each other
					//if (!item->overlap(*neighbor)) continue;
					// Check opposite normals
					if (item->data->normal.dot(neighbor->data->normal) > -1.0f + NEAR_ZERO
						|| item->data->normal.dot(neighbor->data->normal) > 1.0f - NEAR_ZERO) continue;
					// Check coplanar
					plane_dist = fabs((*item->data->points[0] - *neighbor->data->points[0]).dot(item->data->normal));
					if (plane_dist > NEAR_ZERO) continue;
					// Generate cand_key if it hasn't been created already
					if (cand_key[0] == '\0') {
						cand_key = hexFromPtr(parent->parent) + "_" + item->data->normal.str();
					}
					candidates[cand_key].insert(neighbor->data);
				}
			}
			// Append self to candidates if there are any
			if (!cand_key.empty()) {
				candidates[cand_key].insert(item->data);
			}
		}
		visited.clear();
		unique_neighbors.clear();
		delete octree;

		// Check candidates areas
		for (auto& grp : candidates) {
			grp_max_diff = Vector3();
			grp_min_diff = Vector3();
			normal_filter = nullptr;
			grp_1.clear();
			grp_2.clear();
			// Collect points by normal into two groups
			for (FaceData* fd : grp.second) {
				faces_to_keep_debug[fd->surface_ref].insert(fd->face_index);
				if (normal_filter == nullptr) {
					normal_filter = &fd->normal;
				}
				if (fd->normal == *normal_filter) {
					grp_1.push_back(*fd->points[0]);
					grp_1.push_back(*fd->points[1]);
					grp_1.push_back(*fd->points[2]);
				} else {
					grp_2.push_back(*fd->points[0]);
					grp_2.push_back(*fd->points[1]);
					grp_2.push_back(*fd->points[2]);
				}
			}
			// If groups exist, get difference in aggregated areas
			if (grp_1.size() > 0 && grp_2.size() > 0) {
				getBounds<Vector3>(grp_1.data(), grp_1.size(), min, max);
				grp_max_diff = max;
				grp_min_diff = min;
				//grp_1_area = max - min;
				getBounds<Vector3>(grp_2.data(), grp_2.size(), min, max);
				grp_max_diff = grp_max_diff - max;
				grp_min_diff = grp_min_diff - min;
				//grp_2_area = max - min;
				// area_diff = grp_1_area - grp_2_area;
				// area_diff.x = std::abs(area_diff.x);
				// area_diff.y = std::abs(area_diff.y);
				// area_diff.z = std::abs(area_diff.z);
				// Mark faces for removal
				// if (area_diff.x + area_diff.y + area_diff.z < NEAR_ZERO) {
				if (grp_max_diff.dot(grp_max_diff) < NEAR_ZERO && grp_min_diff.dot(grp_min_diff) < NEAR_ZERO) {
					for (FaceData* fd : grp.second) {
						faces_to_remove[fd->surface_ref].insert(fd->face_index);
					}
				}
			}
		}
		grp_1.clear();
		grp_2.clear();
		candidates.clear();

		// Remove faces marked for removal (do not remove vertices)
		int surface_remove_count;
		Surface* surface;
		std::unordered_set<int> surfaces_to_remove;
		for (i = 0; i < mesh.surface_count; i++) {
			surface = &mesh.surfaces[i];
			if (faces_to_keep_debug.find(surface) != faces_to_keep_debug.end()) {
				surface_remove_count = 0;
				std::unordered_set<int> info = faces_to_keep_debug[surface];
				// Iterate each surface's face, shift back on removal
				for (j = 0; j < surface->face_count; j++) {
					if (info.find(j) == info.end()) {
						surface_remove_count += 1;
						continue;
					}
					surface->faces[j - surface_remove_count] = surface->faces[j];
				}
				// Realloc surface
				if (surface_remove_count > 0) {
					surface->face_count -= surface_remove_count;
					surface->faces = (Face*)realloc(surface->faces, sizeof(Face) * surface->face_count);
					if (surface->faces == NULL) {
						throw ReallocException("faces post-face-remove", surface->face_count);
					}
				}
				remove_count += surface_remove_count;
			} else {
				remove_count += surface->face_count;
				surfaces_to_remove.insert(i);
			}
		}
		int surfaces_removed = 0;
		for (i = 0; i < mesh.surface_count; i++) {
			if (surfaces_to_remove.find(i) != surfaces_to_remove.end()) {
				surfaces_removed += 1;
				continue;
			}
			mesh.surfaces[i - surfaces_removed] = mesh.surfaces[i];
		}
		if (surfaces_removed > 0) {
			mesh.surface_count -= surfaces_removed;
			mesh.surfaces = (Surface*)realloc(mesh.surfaces, sizeof(Surface) * mesh.surface_count);
			if (mesh.surfaces == NULL) {
				throw ReallocException("surfaces post-debug-clear-non-candidates", mesh.surface_count);
			}
		}
		faces_to_keep_debug.clear();
		// for (auto& fr : faces_to_remove) {
		// 	surface = fr.first;
		// 	surface_remove_count = 0;
		// 	// Iterate each surface's face, shift back on removal
		// 	for (i = 0; i < surface->face_count; i++) {
		// 		if (fr.second.find(i) != fr.second.end()) {
		// 			surface_remove_count += 1;
		// 			continue;
		// 		}
		// 		surface->faces[i - surface_remove_count] = surface->faces[i];
		// 	}
		// 	// Realloc surface
		// 	if (surface_remove_count > 0) {
		// 		surface->face_count -= surface_remove_count;
		// 		surface->faces = (Face*)realloc(surface->faces, sizeof(Face) * surface->face_count);
		// 		if (surface->faces == NULL) {
		// 			throw ReallocException("faces post-face-remove", surface->face_count);
		// 		}
		// 	}
		// 	remove_count += surface_remove_count;
		// }
		faces_to_remove.clear();
	}

	//debug_mesh->save("octree_debug.obj");

	return remove_count;
}

int removeSceneInternalFaces(Node& scene) {
	int i;
	int remove_count = 0;
	MeshObj* mnode;
	std::vector<int> empty_meshes;
	for (i = 0; i < scene.children.size(); i++) {
		if (scene.children[i]->type == NodeType::MeshObj) {
			mnode = ((MeshObj*)scene.children[i]);
			remove_count += removeFacesInMesh(mnode->mesh);
			if (mnode->mesh.surface_count == 0) empty_meshes.push_back(i);
		} else {
			remove_count += removeSceneInternalFaces(*scene.children[i]);
		}
	}
	// Remove child mesh objects that were reduced to nothing
	for (i = empty_meshes.size() - 1; i >= 0; i--) {
		delete scene.children[empty_meshes[i]];
		scene.children.erase(scene.children.begin() + empty_meshes[i]);
	}
	return remove_count;
}

int joinSceneRelatedVerts(Node& scene, float min_dist) {
	int i;
	int remove_count = 0;
	MeshObj* mnode;
	std::vector<int> empty_meshes;
	for (i = 0; i < scene.children.size(); i++) {
		if (scene.children[i]->type == NodeType::MeshObj) {
			mnode = ((MeshObj*)scene.children[i]);
			remove_count += joinVertInMesh(mnode->mesh, min_dist);
			if (mnode->mesh.surface_count == 0) empty_meshes.push_back(i);
		} else {
			remove_count += joinSceneRelatedVerts(*scene.children[i], min_dist);
		}
	}
	// Remove child mesh objects that were reduced to nothing
	for (i = empty_meshes.size() - 1; i >= 0; i--) {
		delete scene.children[empty_meshes[i]];
		scene.children.erase(scene.children.begin() + empty_meshes[i]);
	}
	return remove_count;
}