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
	if (mesh.vert_count == 0) return 0;

	int i, j, k, avg_count, remove_count;
	float min_dist_sq, dist_sq;
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

	// Some distance checks use squared distance
	min_dist_sq = min_dist * min_dist;

	// TODO: Move octree instances outside of joinVertInMesh (mirror of scene)
	// TODO: Switch octree to be mesh > surface > material > faces (not individual verts)
	for (i = 0; i < mesh.vert_count; i++) {
		items.push_back(new OctreeItem<VertData>(mesh.verts[i], Vector3()));
		items.back()->data = new VertData();
		items.back()->data->index = i;
	}
	octree = new Octree<VertData>(items.data(), items.size());
	octree->min_dim = min_dist_sq;
	octree->subdivide(20, 0);

	// Find joinable vertices and build index remapping based on future shift down
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
				dist_sq = diff.dot(diff);
				if (dist_sq <= min_dist_sq) {
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

enum class FOStatus {
	Same,
	Valid,
	Invalid
};

FOStatus inline getFaceOverlapStatus(OctreeItem<FaceData>* item, OctreeItem<FaceData>* neighbor, float min_dist_sq) {
	static float edge_1d_c1, edge_1d_c2, edge_1d_prod;
	static Vector3 diff, edge_ndir, edge_norm, edge_rel_orig;
	static std::vector<Vector3*> shared_points(3);

	// Get shared points
	shared_points.clear();
	for (Vector3* npoint : neighbor->data->points) {
		for (Vector3* point : item->data->points) {
			diff = *npoint - *point;
			if (diff.dot(diff) <= min_dist_sq) {
				shared_points.push_back(npoint);
			}
		}
	}

	// Prechecks
	if (shared_points.size() <= 1) return FOStatus::Invalid;
	if (shared_points.size() == 3) return FOStatus::Same;

	// Two shared points:
	// Edge defines 1D space, project both centers into space
	edge_ndir = *shared_points[0] - *shared_points[1];
	edge_ndir = edge_ndir / edge_ndir.dot(edge_ndir);
	edge_rel_orig = edge_ndir * edge_ndir.dot(item->center);
	edge_norm = item->center - edge_rel_orig;
	edge_norm = edge_norm / edge_norm.dot(edge_norm);
	edge_1d_c1 = edge_norm.dot(item->center - edge_rel_orig);
	edge_1d_c2 = edge_norm.dot(neighbor->center - edge_rel_orig);
	// If 1D centers mismatch about origin (the edge) loop-continue
	// Check mismatch using product (needs to be positive)
	edge_1d_prod = edge_1d_c1 * edge_1d_c2;
	if (edge_1d_prod < 0.0f) return FOStatus::Invalid;

	return FOStatus::Valid;
}

int removeFacesInMesh(ObjWavefront& mesh, float min_dist) {
	if (mesh.vert_count == 0 || mesh.surface_count == 0) return 0;

	bool covered, shared_point;
	int i, j, k, remove_count, surface_remove_count;
	float min_dist_sq, point_dist, plane_dist;
	float one_third = 1.0f / 3.0f;
	Surface* surface;
	FaceData* face_data;
	FOStatus fol_status;
	Vector3 min, max, center, diff;
	Vector3 points[3];
	std::unordered_map<Surface*, std::unordered_set<int>> faces_to_remove;
	std::unordered_set<Vector3*> opposing_points;
	std::unordered_set<OctreeItem<FaceData>*> not_covered;
	std::unordered_set<OctreeItem<FaceData>*> unique_neighbors;
	std::vector<OctreeItem<FaceData>*> items;
	Octree<FaceData>* octree;

	// Some distance checks use squared distance
	min_dist_sq = min_dist * min_dist;

	remove_count = 0;
	for (i = 0; i < mesh.surface_count; i++) {
		// Create an octree of face data to localize the mesh surface
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

		// Use octree to find neighboring candidates for removal
		for (OctreeItem<FaceData>* item : octree->children) {
			// Check if item has already been marked for removal
			if (faces_to_remove.find(item->data->surface_ref) != faces_to_remove.end()) {
				if (faces_to_remove[item->data->surface_ref].find(item->data->face_index) != faces_to_remove[item->data->surface_ref].end()) {
					continue;
				}
			}
			// Reset neighbors checked
			unique_neighbors.clear();
			for (Octree<FaceData>* parent : item->parents) {
				for (OctreeItem<FaceData>* neighbor : parent->children) {
					// Ignore self / redundant checks
					if (item == neighbor) continue;
					if (!unique_neighbors.insert(neighbor).second) continue;
					// Check near each other
					if (!item->overlap(*neighbor)) continue;
					// Check opposite normals
					if (item->data->normal.dot(neighbor->data->normal) > -1.0f + NEAR_ZERO) continue;
					// Check coplanar
					plane_dist = std::abs((neighbor->center - item->center).dot(item->data->normal));
					if (plane_dist > min_dist) continue;
					// Check overlapping status
					fol_status = getFaceOverlapStatus(item, neighbor, min_dist_sq);
					if (fol_status == FOStatus::Invalid) continue;
					if (fol_status == FOStatus::Same) {
						// Clean opposing_points
						opposing_points.clear();
						// Add both to faces_to_remove
						faces_to_remove[item->data->surface_ref].insert(item->data->face_index);
						faces_to_remove[neighbor->data->surface_ref].insert(neighbor->data->face_index);
						// Continue to next Octree item ()
						break;
					}
					// Keep points for opposing checks later
					for (Vector3* point : neighbor->data->points) {
						opposing_points.insert(point);
					}
				}
			}
			// Check if face's points are fully covered by opposing points
			if (opposing_points.size() > 0) {
				covered = true;
				for (const Vector3* point : item->data->points) {
					// Check if any opposing points share position with this point
					shared_point = false;
					for (const Vector3* opoint : opposing_points) {
						diff = *point - *opoint;
						point_dist = diff.dot(diff);
						if (point_dist > min_dist_sq) continue;
						shared_point = true;
						break;
					}
					// If point doesn't share, then face isn't covered
					if (!shared_point) {
						covered = false;
						break;
					}
				}
				// If covered, mark face for removal
				if (covered) {
					faces_to_remove[item->data->surface_ref].insert(item->data->face_index);
				// If uncovered, add it to list of neighbors to ignore
				} else {
					not_covered.insert(item);
				}
				opposing_points.clear();
			}
		}
		unique_neighbors.clear();
		delete octree;
	}

	// Remove faces marked for removal (do not remove vertices)
	for (auto& fr : faces_to_remove) {
		surface = fr.first;
		surface_remove_count = 0;
		// Iterate each surface's face, shift back on removal
		for (j = 0; j < surface->face_count; j++) {
			if (fr.second.find(j) != fr.second.end()) {
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
	}
	faces_to_remove.clear();

	return remove_count;
}

int removeSceneInternalFaces(Node& scene, float min_dist) {
	int i;
	int remove_count = 0;
	MeshObj* mnode;
	std::vector<int> empty_meshes;
	for (i = 0; i < scene.children.size(); i++) {
		if (scene.children[i]->type == NodeType::MeshObj) {
			mnode = ((MeshObj*)scene.children[i]);
			remove_count += removeFacesInMesh(mnode->mesh, min_dist);
			if (mnode->mesh.surface_count == 0) empty_meshes.push_back(i);
		} else {
			remove_count += removeSceneInternalFaces(*scene.children[i], min_dist);
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