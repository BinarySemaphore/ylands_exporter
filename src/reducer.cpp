#include "reducer.hpp"

#include <vector>
#include <unordered_set>
#include <unordered_map>

#include "utils.hpp"
#include "scene.hpp"
#include "space.hpp"

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

	removed_surfaces = 0;
	for (i = 0; i < mesh.surface_count; i++) {
		reduceFaces(mesh.surfaces[i]);
		if (mesh.surfaces[i].face_count == 0) {
			removed_surfaces += 1;
			keep[i] = false;
		}
	}

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
	int i, j, k, count, remove_count;
	float dist;
	Surface* surface;
	Face* face;
	Vector3* hold;
	Vector3 diff;
	Vector3 avg;
	std::vector<bool> keep(mesh.vert_count, true);
	std::unordered_map<int, int> index_remap;

	// Find joinable vertices and build index remapping based on future shift down
	remove_count = 0;
	min_dist = min_dist * min_dist;
	for (i = 0; i < mesh.vert_count; i++) {  // tempting to exit at vert_count - 1, but required to get correct remove_count
		if (index_remap.find(i) != index_remap.end()) {
			remove_count += 1;
			keep[i] = false;
			continue;
		}
		count = 1;
		avg = mesh.verts[i];
		for (j = i + 1; j < mesh.vert_count; j++) {
			diff = mesh.verts[i] - mesh.verts[j];
			dist = diff.dot(diff);
			if (dist <= min_dist) {
				index_remap[j] = i - remove_count;
				avg = avg + mesh.verts[j];
				count += 1;
			}
		}
		if (count > 1) {
			mesh.verts[i] = avg * (1.0f / count);
		}
		index_remap[i] = i - remove_count;
	}

	// Update vertices then realloc
	for (i = 0; i < mesh.vert_count; i++) {
		if (!keep[i]) continue;
		mesh.verts[index_remap[i]]  = mesh.verts[i];
	}
	mesh.vert_count = mesh.vert_count - remove_count;
	hold = (Vector3*)realloc(mesh.verts, mesh.vert_count * sizeof(Vector3));
	if (hold == NULL) {
		throw ReallocException("vectices post-join", mesh.vert_count);
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