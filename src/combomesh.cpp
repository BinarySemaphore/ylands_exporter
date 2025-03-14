#include "combomesh.hpp"

#include "utils.hpp"
#include "scene.hpp"

ComboMeshItem::ComboMeshItem() {
	this->vert_count = 0;
	this->norm_count = 0;
	this->uv_count = 0;
	this->face_count = 0;
}

bool ComboMeshItem::append(MeshObj& node) {
	this->vert_index_offs.push_back(this->vert_count);
	this->norm_index_offs.push_back(this->norm_count);
	this->uv_index_offs.push_back(this->uv_count);
	this->meshes.push_back(&node);
	this->vert_count += node.mesh.vert_count;
	this->norm_count += node.mesh.norm_count;
	this->uv_count += node.mesh.uv_count;
	for (int i = 0; i < node.mesh.surface_count; i++) {
		this->face_count += node.mesh.surfaces[i].face_count;
	}
	
	return true;
}

ComboMesh::ComboMesh() {
	this->surface_count = 0;
}

bool ComboMesh::append(MeshObj& node) {
	std::string color_uid = ComboMesh::getEntityColorUid(node);

	if (this->cmesh.find(color_uid) == this->cmesh.end()) {
		this->cmesh[color_uid].material = *node.mesh.getSurfaceMaterials(0)[0];
	}
	this->cmesh[color_uid].append(node);

	return true;
}

void globalizeSpacials(MeshObj* mesh) {
	int i;
	for (i = 0; i < mesh->mesh.vert_count; i++) {
		mesh->mesh.verts[i] = mesh->position
							+ (mesh->rotation
								* (mesh->offset
									+ (mesh->scale * mesh->mesh.verts[i])));
	}
	for (i = 0; i < mesh->mesh.norm_count; i++) {
		// TODO: Honor scaling with rotation * (norm * scale).normalized
		mesh->mesh.norms[i] = mesh->rotation * mesh->mesh.norms[i];
	}
}

void globalizeIndecies(const ComboMeshItem& item, MeshObj* mesh, int mesh_index, int vert_off, int norm_off, int uv_off) {
	int i, j, k;
	int total_vert_offset = vert_off + item.vert_index_offs[mesh_index];
	int total_norm_offset = norm_off + item.norm_index_offs[mesh_index];
	int total_uv_offset = uv_off + item.uv_index_offs[mesh_index];
	for (i = 0; i < mesh->mesh.surface_count; i++) {
		for (j = 0; j < mesh->mesh.surfaces[i].face_count; j++) {
			for (k = 0; k < 3; k++) {
				mesh->mesh.surfaces[i].faces[j].vert_index[k] += total_vert_offset;
				mesh->mesh.surfaces[i].faces[j].norm_index[k] += total_norm_offset;
				mesh->mesh.surfaces[i].faces[j].uv_index[k] += total_uv_offset;
			}
		}
	}
}

MeshObj* ComboMesh::commitToMesh(Workpool* wp) {
	int i, j, k, l, m;
	const int batch = 100;
	int total_vert_count = 0;
	int total_norm_count = 0;
	int total_uv_count = 0;
	int total_surface_count = 0;
	MeshObj* combined = new MeshObj();
	std::vector<ComboMeshItem*> order;

	bool t = true;
	for (std::pair<std::string, ComboMeshItem> kv : this->cmesh) {
		for (i = 0; i < kv.second.meshes.size(); i++) {
			wp->addTask(std::bind(globalizeSpacials, kv.second.meshes[i]), NULL, NULL);
			wp->addTask(std::bind(
				globalizeIndecies,
				kv.second,
				kv.second.meshes[i],
				i,
				total_vert_count,
				total_norm_count,
				total_uv_count
			), NULL, NULL);
		}
		order.push_back(&this->cmesh.find(kv.first)->second);
		total_vert_count += kv.second.vert_count;
		total_norm_count += kv.second.norm_count;
		total_uv_count += kv.second.uv_count;
		total_surface_count += 1;
	}

	if (total_vert_count > 0) {
		combined->mesh.vert_count = total_vert_count;
		combined->mesh.verts = (Vector3*)malloc(sizeof(Vector3) * total_vert_count);
		if (combined->mesh.verts == NULL) {
			throw AllocationException("vertices", total_vert_count);
		}
	}
	if (total_norm_count > 0) {
		combined->mesh.norm_count = total_norm_count;
		combined->mesh.norms = (Vector3*)malloc(sizeof(Vector3) * total_norm_count);
		if (combined->mesh.norms == NULL) {
			throw AllocationException("normals", total_norm_count);
		}
	}
	if (total_uv_count > 0) {
		combined->mesh.uv_count = total_uv_count;
		combined->mesh.uvs = (Vector2*)malloc(sizeof(Vector2) * total_uv_count);
		if (combined->mesh.uvs == NULL) {
			throw AllocationException("UVs", total_uv_count);
		}
	}
	if (total_surface_count > 0) {
		combined->mesh.surface_count = total_surface_count;
		combined->mesh.surfaces = (Surface*)malloc(sizeof(Surface) * total_surface_count);
		if (combined->mesh.surfaces == NULL) {
			throw AllocationException("surfaces", total_surface_count);
		}
	}
	for (i = 0; i < total_surface_count; i++) {
		combined->mesh.surfaces[i].face_count = order[i]->face_count;
		if (order[i]->face_count > 0) {
			combined->mesh.surfaces[i].faces = (Face*)malloc(sizeof(Face) * order[i]->face_count);
			if (combined->mesh.surfaces[i].faces == NULL) {
				throw AllocationException("surface faces", order[i]->face_count);
			}
		}
		combined->mesh.surfaces[i].material_refs = new std::unordered_map<int, std::string>();
		order[i]->material.name = "mat_" + std::to_string(i);
		(*combined->mesh.surfaces[i].material_refs)[0] = order[i]->material.name;
		combined->mesh.materials[order[i]->material.name] = order[i]->material;
	}

	wp->wait();

	ObjWavefront* obj;
	Surface* surface;
	int full_vert_index;
	int full_norm_index;
	int full_uv_index;
	int face_index;
	int vert_index = 0;
	int norm_index = 0;
	int uv_index = 0;
	for (i = 0; i < order.size(); i++) {
		surface = &combined->mesh.surfaces[i];
		face_index = 0;
		for (j = 0; j < order[i]->meshes.size(); j++) {
			obj = &order[i]->meshes[j]->mesh;
			full_vert_index = vert_index + order[i]->vert_index_offs[j];
			full_norm_index = norm_index + order[i]->norm_index_offs[j];
			full_uv_index = uv_index + order[i]->uv_index_offs[j];
			for (k = 0; k < obj->vert_count; k++) {
				combined->mesh.verts[full_vert_index + k] = obj->verts[k];
			}
			for (k = 0; k < obj->norm_count; k++) {
				combined->mesh.norms[full_norm_index + k] = obj->norms[k];
			}
			for (k = 0; k < obj->uv_count; k++) {
				combined->mesh.uvs[full_uv_index + k] = obj->uvs[k];
			}
			for (k = 0; k < obj->surface_count; k++) {
				for (l = 0; l < obj->surfaces[k].face_count; l++) {
					for (m = 0; m < 3; m++) {
						surface->faces[face_index + l].vert_index[m] = obj->surfaces[k].faces[l].vert_index[m];
						surface->faces[face_index + l].norm_index[m] = obj->surfaces[k].faces[l].norm_index[m];
						surface->faces[face_index + l].uv_index[m] = obj->surfaces[k].faces[l].uv_index[m];
					}
					//combined->mesh.surfaces[j].faces[face_index + l] = obj->surfaces[k].faces[l];
				}
				face_index += obj->surfaces[k].face_count;
			}
		}
		vert_index += order[i]->vert_count;
		norm_index += order[i]->norm_count;
		uv_index += order[i]->uv_count;
	}

	combined->mesh.name = "CominedMesh";
	return combined;
}

std::string ComboMesh::getEntityColorUid(MeshObj& entity) {
	std::string color_uid = "";
	Material* mat = entity.mesh.getSurfaceMaterials(0)[0];
	color_uid += Material::getColorHashString(mat->diffuse);
	color_uid += "_em" + Material::getColorHashString(mat->emissive);
	color_uid += "_d" + std::to_string(mat->dissolve);
	return color_uid;
}