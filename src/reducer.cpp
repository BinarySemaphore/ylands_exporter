#include "reducer.hpp"

#include <iostream>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include "utils.hpp"
#include "scene.hpp"
#include "space.hpp"
#include "octree.hpp"
#include "bmp.hpp"

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

class FlatFace {
public:
	FaceData* ref;
	Vector2 points[3];
	std::vector<std::pair<int, int>> area_points;
};

bool pointWithin2dTriangle(const Vector2& check, const Vector2& p1, const Vector2& p2, const Vector2& p3) {
	float c1, c2, c3;
	c1 = (check - p1).cross(p2 - p1);
	c2 = (check - p2).cross(p3 - p2);
	c3 = (check - p3).cross(p1 - p3);
	return (c1 >= 0.0f && c2 >= 0.0f && c3 >= 0.0f) || (c1 <= 0.0f && c2 <= 0.0f && c3 <= 0.0f);
}

ObjWavefront* candCreateMesh() {
	ObjWavefront* mesh = new ObjWavefront();
	mesh->name = "Candidates";
	mesh->vert_count = 0;
	mesh->norm_count = 0;
	mesh->uv_count = 0;
	mesh->surface_count = 0;
	mesh->verts = (Vector3*)malloc(sizeof(Vector3) * 3);
	mesh->norms = (Vector3*)malloc(sizeof(Vector3));
	mesh->surfaces = (Surface*)malloc(sizeof(Surface));
	return mesh;
}

void candSurfaceAddToMesh(const Vector3& color, ObjWavefront* mesh) {
	Material* mat = new Material(("mat_" + std::to_string(mesh->materials.size())).c_str());
	mat->ambient = color;
	mat->diffuse = color;
	mat->emissive = color;
	mat->dissolve = 1.0f;
	mesh->materials[mat->name] = *mat;
	mesh->surface_count += 1;
	mesh->surfaces = (Surface*)realloc(mesh->surfaces, sizeof(Surface) * mesh->surface_count);
	mesh->surfaces[mesh->surface_count - 1].material_refs = new std::unordered_map<int, std::string>();
	(*mesh->surfaces[mesh->surface_count - 1].material_refs)[0] = mat->name;
	mesh->surfaces[mesh->surface_count - 1].face_count = 0;
	mesh->surfaces[mesh->surface_count - 1].faces = (Face*)malloc(sizeof(Face));
}

void candFaceAddToMesh(const FaceData* face, int rel_surface_index, ObjWavefront* mesh) {
	Surface* surface = &mesh->surfaces[mesh->surface_count - rel_surface_index];
	mesh->vert_count += 3;
	mesh->norm_count += 1;
	mesh->verts = (Vector3*)realloc(mesh->verts, sizeof(Vector3) * mesh->vert_count);
	mesh->norms = (Vector3*)realloc(mesh->norms, sizeof(Vector3) * mesh->norm_count);
	mesh->verts[mesh->vert_count - 3] = *face->points[0];
	mesh->verts[mesh->vert_count - 2] = *face->points[1];
	mesh->verts[mesh->vert_count - 1] = *face->points[2];
	mesh->norms[mesh->norm_count - 1] = face->normal;
	surface->face_count += 1;
	surface->faces = (Face*)realloc(surface->faces, sizeof(Face) * surface->face_count);
	surface->faces[surface->face_count - 1].vert_index[0] = mesh->vert_count - 2;
	surface->faces[surface->face_count - 1].vert_index[1] = mesh->vert_count - 1;
	surface->faces[surface->face_count - 1].vert_index[2] = mesh->vert_count;
	surface->faces[surface->face_count - 1].norm_index[0] = mesh->norm_count;
	surface->faces[surface->face_count - 1].norm_index[1] = mesh->norm_count;
	surface->faces[surface->face_count - 1].norm_index[2] = mesh->norm_count;
}

int removeFacesInMesh(ObjWavefront& mesh) {
	bool area_occulded;
	int i, j, k, remove_count, coord_x, coord_y, surface_remove_count;
	float plane_dist;
	float one_third = 1.0f / 3.0f;
	Surface* surface;
	FaceData* face_data;
	FlatFace* fface;
	Vector3 *normal_filter, *opposing_normal;
	Vector2 res, half_res, res_off, res_check, res_min, res_max;
	Vector3 u_axis, v_axis, not_norm, plane, normal, projection;
	Vector3 min, max, center, grp_max_diff, grp_min_diff, area_diff;
	Vector3 points[3];
	std::string cand_key;
	std::unordered_map<std::string, std::unordered_set<FaceData*>> candidates;
	std::unordered_map<Surface*, std::unordered_set<int>> faces_to_keep_debug;
	std::unordered_map<Surface*, std::unordered_set<int>> faces_to_remove;
	std::unordered_set<OctreeItem<FaceData>*> visited;
	std::unordered_set<OctreeItem<FaceData>*> unique_neighbors;
	std::vector<OctreeItem<FaceData>*> items;
	Octree<FaceData>* octree;
	std::unordered_set<std::pair<int, int>, PairHash> grp_1_area;
	std::unordered_set<std::pair<int, int>, PairHash> grp_2_area;
	std::unordered_set<FlatFace*> grp_1;
	std::unordered_set<FlatFace*> grp_2;

	ObjWavefront* debug_mesh = octreeDebugPrepareMesh();
	ObjWavefront* debug_cand_mesh = candCreateMesh();
	float amount_blue = 0.0f;

	remove_count = 0;
	for (i = 0; i < mesh.surface_count; i++) {
		std::cout << "On surface " << i + 1 << " of " << mesh.surface_count << "." << std::endl;
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
			face_data->normal = (points[1] - points[0]).cross(points[2] - points[1]);
			face_data->normal = face_data->normal * (1.0f / sqrtf(face_data->normal.dot(face_data->normal)));
			getBounds<Vector3>(points, 3, min, max);
			center = (points[0] + points[1] + points[2]) * one_third;
			items.push_back(new OctreeItem<FaceData>(center, max - min));
			items.back()->data = face_data;
		}
		octree = new Octree<FaceData>(items.data(), items.size());
		octree->subdivide(20, 0);
		items.clear();
		octreeDebugAddToMesh<FaceData>(octree, debug_mesh);

		// Use octree to find neighboring candidates for removal
		for (OctreeItem<FaceData>* item : octree->children) {
			// Mark item as visted and reset neighbors checked
			//visited.insert(item);
			unique_neighbors.clear();
			cand_key = "";
			for (Octree<FaceData>* parent : item->parents) {
				for (OctreeItem<FaceData>* neighbor : parent->children) {
					// Ignore self / redundant checks
					//if (visited.find(neighbor) != visited.end()) continue;
					if (item == neighbor) continue;
					if (!unique_neighbors.insert(neighbor).second) continue;
					// Check near each other
					if (!item->overlap(*neighbor)) continue;
					// Check opposite normals
					if (item->data->normal.dot(neighbor->data->normal) >= -1.0f + NEAR_ZERO) continue;
					// Check coplanar
					plane_dist = std::abs((*neighbor->data->points[0] - *item->data->points[0]).dot(item->data->normal));
					if (plane_dist > NEAR_ZERO) continue;
					// Generate cand_key if it hasn't been created already
					if (cand_key.empty()) {
						// plane = *item->data->points[0] / std::sqrtf(item->data->points[0]->dot(*item->data->points[0]));
						// plane = plane - (plane * plane.dot(item->data->normal));
						// normal.x = std::abs(item->data->normal.x);
						// normal.y = std::abs(item->data->normal.y);
						// normal.z = std::abs(item->data->normal.z);
						cand_key = hexFromPtr(item) + "_" + item->data->normal.str();
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
			opposing_normal = nullptr;
			// Clean up
			for (FlatFace* fface : grp_1) {
				delete fface;
			}
			for (FlatFace* fface : grp_2) {
				delete fface;
			}
			grp_1.clear();
			grp_2.clear();
			grp_1_area.clear();
			grp_2_area.clear();
			res.x = std::numeric_limits<float>::max();
			res.y = std::numeric_limits<float>::max();
			// Construct flat faces from face candidates and
			// collect into two opposing groups by normals
			// Identify min resolution for area sampling later
			for (FaceData* fd : grp.second) {
				if (normal_filter == nullptr) {
					normal_filter = &fd->normal;
					// Get UV axis vectors to convert 3D points into 2D space
					not_norm = Vector3(1.0f, 0.0f, 0.0f);
					if (std::abs(normal_filter->dot(not_norm)) >= 1.0f - NEAR_ZERO) {
						not_norm = Vector3(0.0f, 1.0f, 0.0f);
					}
					u_axis = normal_filter->cross(not_norm);
					u_axis = u_axis / std::sqrtf(u_axis.dot(u_axis));
					v_axis = normal_filter->cross(u_axis);
				}
				// Build flat face
				fface = new FlatFace();
				fface->ref = fd;
				for (j = 0; j < 3; j++) {
					projection = fd->points[j]->projectOntoPlane(fd->normal);
					fface->points[j].x = u_axis.dot(projection);
					fface->points[j].y = v_axis.dot(projection);
				}
				// Check for smallest resolution
				for (j = 0; j < 3; j++) {
					k = (j + 1) % 3;
					res_check.x = std::abs(fface->points[j].x - fface->points[k].x);
					res_check.y = std::abs(fface->points[j].y - fface->points[k].y);
					if (res_check.x > NEAR_ZERO && res_check.x < res.x) res.x = res_check.x;
					if (res_check.y > NEAR_ZERO && res_check.y < res.y) res.y = res_check.y;
				}
				// Filter to group
				if (normal_filter->dot(fd->normal) >= 1.0f - NEAR_ZERO) {
					grp_1.insert(fface);
				} else if (opposing_normal == nullptr || opposing_normal->dot(fd->normal) >= 1.0f - NEAR_ZERO) {
					grp_2.insert(fface);
					if (opposing_normal == nullptr) {
						opposing_normal = &fd->normal;
					}
				} else {
					throw GeneralException("Bad candidate group: multiple nomals (more than 2) detected");
				}
			}
			// Rasterize each flat face area using resolution coordiants
			// Keep track of coords for each group
			// TODO: min resolution isn't being computed for smallest triangle properly, fix it
			res = res * 0.5f;
			half_res = res * 0.5f;
			int start_x, end_x, start_y, end_y;
			for (FlatFace* fface : grp_1) {
				getBounds<Vector2>(fface->points, 3, res_min, res_max);
				start_x = std::round((res_min.x) / res.x);
				end_x = std::round((res_max.x) / res.x);
				start_y = std::round((res_min.y) / res.y);
				end_y = std::round((res_max.y) / res.y);
				if (end_x < start_x) std::swap(start_x, end_x);
				if (end_y < start_y) std::swap(start_y, end_y);
				for (coord_x = start_x; coord_x <= end_x; coord_x++) {
					for (coord_y = start_y; coord_y <= end_y; coord_y++) {
						res_check.x = coord_x * res.x + half_res.x;
						res_check.y = coord_y * res.y + half_res.y;
						if (pointWithin2dTriangle(res_check, fface->points[0], fface->points[1], fface->points[2])) {
							fface->area_points.push_back(std::pair<int, int>(coord_x, coord_y));
							grp_1_area.insert(std::pair<int, int>(coord_x, coord_y));
						}
					}
				}
				if (fface->area_points.size() == 0) {
					throw GeneralException("Bad resolution");
				}
			}
			for (FlatFace* fface : grp_2) {
				getBounds<Vector2>(fface->points, 3, res_min, res_max);
				start_x = std::round((res_min.x) / res.x);
				end_x = std::round((res_max.x) / res.x);
				start_y = std::round((res_min.y) / res.y);
				end_y = std::round((res_max.y) / res.y);
				if (end_x < start_x) std::swap(start_x, end_x);
				if (end_y < start_y) std::swap(start_y, end_y);
				for (coord_x = start_x; coord_x <= end_x; coord_x++) {
					for (coord_y = start_y; coord_y <= end_y; coord_y++) {
						res_check.x = coord_x * res.x + half_res.x;
						res_check.y = coord_y * res.y + half_res.y;
						if (pointWithin2dTriangle(res_check, fface->points[0], fface->points[1], fface->points[2])) {
							fface->area_points.push_back(std::pair<int, int>(coord_x, coord_y));
							grp_2_area.insert(std::pair<int, int>(coord_x, coord_y));
						}
					}
				}
				if (fface->area_points.size() == 0) {
					throw GeneralException("Bad resolution");
				}
			}
			// Finally check if any face's rasterred area is fully occluded by the opposition
			for (FlatFace* fface : grp_1) {
				area_occulded = true;
				for (std::pair<int, int>& coords : fface->area_points) {
					// Check if any raster coord is not in opposing area
					if (grp_2_area.find(coords) == grp_2_area.end()) {
						area_occulded = false;
						break;
					}
				}
				// Mark face for removal
				if (area_occulded) {
					faces_to_remove[fface->ref->surface_ref].insert(fface->ref->face_index);
				}
			}
			for (FlatFace* fface : grp_2) {
				area_occulded = true;
				for (std::pair<int, int>& coords : fface->area_points) {
					// Check if any raster coord is not in opposing area
					if (grp_1_area.find(coords) == grp_1_area.end()) {
						area_occulded = false;
						break;
					}
				}
				// Mark face for removal
				if (area_occulded) {
					faces_to_remove[fface->ref->surface_ref].insert(fface->ref->face_index);
				}
			}
		}
		candidates.clear();
		grp_1_area.clear();
		grp_2_area.clear();
		for (FlatFace* fface : grp_1) {
			delete fface;
		}
		for (FlatFace* fface : grp_2) {
			delete fface;
		}
		grp_1.clear();
		grp_2.clear();

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
	}

	debug_mesh->save("octree_debug.obj");
	debug_cand_mesh->save("cand_debug.obj");

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