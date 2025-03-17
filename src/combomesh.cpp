#include "combomesh.hpp"

#include "utils.hpp"
#include "exporter.hpp"
#include "combomesh.hpp"
#include "objwavefront.hpp"
#include "workpool.hpp"

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
		this->cmesh[color_uid].material = node.mesh.getSurfaceMaterials(0)[0];
	}
	this->cmesh[color_uid].append(node);

	return true;
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

template <class T>
void copyVectorArray(T* dest, T* src, int size, int dest_offset) {
	for (int i = 0; i < size; i++) {
		dest[dest_offset + i] = src[i];
	}
}

void copyFaceArray(Face* dest, Face* src, int size, int dest_offset) {
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < 3; j++) {
			dest[dest_offset + i].vert_index[j] = src[i].vert_index[j];
			dest[dest_offset + i].norm_index[j] = src[i].norm_index[j];
			dest[dest_offset + i].uv_index[j] = src[i].uv_index[j];
		}
	}
}

MeshObj* ComboMesh::commitToMesh() {
	int i, j, k;
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
		order[i]->material->name = "mat_" + std::to_string(i);
		(*combined->mesh.surfaces[i].material_refs)[0] = order[i]->material->name;
		combined->mesh.materials[order[i]->material->name] = *order[i]->material;
	}

	// Must wait on workpool tasks to finish before pulling results
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
			copyVectorArray<Vector3>(
				combined->mesh.verts, obj->verts,
				obj->vert_count, full_vert_index
			);
			copyVectorArray<Vector3>(
				combined->mesh.norms, obj->norms,
				obj->norm_count, full_norm_index
			);
			copyVectorArray<Vector2>(
				combined->mesh.uvs, obj->uvs,
				obj->uv_count, full_uv_index
			);
			for (k = 0; k < obj->surface_count; k++) {
				copyFaceArray(
					surface->faces, obj->surfaces[k].faces,
					obj->surfaces[k].face_count, face_index
				);
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

void buildComboFromSceneChildren(ComboMesh& combo, Node& root) {
	// TODO: single combo should be it's own function; this should only combine by siblings
	for (int i = 0; i < root.children.size(); i++) {
		if (root.children[i]->type == NodeType::Node) {
			buildComboFromSceneChildren(combo, *root.children[i]);
			continue;
		}
		combo.append(*(MeshObj*)root.children[i]);
	}
}

ComboMesh* createComboFromScene(Node& scene) {
	ComboMesh* combo = new ComboMesh();
	buildComboFromSceneChildren(*combo, scene);
	return combo;
}