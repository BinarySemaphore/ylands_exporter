#include "gltf.hpp"

#include <fstream>
#include <unordered_map>

#include "utils.hpp"
#include "config.hpp"
#include "space.hpp"
#include "scene.hpp"
#include "json.hpp"
using json = nlohmann::json;

inline int GLTFBVTargetToInt(GLTFBVTarget target) {
	return 34962 + (int)target;
}

std::string GLTFAlphaModeToString[] = {
	"",  // Don't use
	"BLEND"
};

inline int GLTFAccTypeToInt(GLTFAccType acc_type) {
	return 1 + (int)acc_type;
}

std::string GLTFAccTypeToString[] = {
	"SCALAR",
	"VEC2",
	"VEC3",
	"VEC4"
};

void GLTFCompTypePushBack(const GLTFCompType type, json& arr, const uint32_t& value) {
	switch (type) {
		case GLTFCompType::BYTE:
			arr.push_back((int8_t)value);
			break;
		case GLTFCompType::UNSIGNED_BYTE:
			arr.push_back((uint8_t)value);
			break;
		case GLTFCompType::SHORT:
			arr.push_back((int16_t)value);
			break;
		case GLTFCompType::UNSIGNED_SHORT:
			arr.push_back((uint16_t)value);
			break;
		case GLTFCompType::UNSIGNED_INT:
			arr.push_back(value);
			break;
		case GLTFCompType::FLOAT:
			float fval;
			std::memcpy(&fval, &value, sizeof(float));
			arr.push_back(fval);
			break;
	}
}

int GLTFCompTypeToInt[] = {
	5120,
	5121,
	5122,
	5123,
	5125,
	5126
};

std::string GLTFTopoTypesToString[] = {
	"POINTS",
	"LINE_STRIPS",
	"LINE_LOOPS",
	"LINES",
	"TRIANGLES",
	"TRIANGLE_STRIPS",
	"TIRANGLE_FANS"
};

int GLBuffer::addData(std::byte* new_data, int size) {
	int i;
	int index = this->data.size();
	for (i = 0; i < size; i++) {
		this->data.push_back(new_data[i]);
	}
	return index;
}

GLBufferView::GLBufferView() {
	this->buffer_index = -1;
	this->byte_offset = 0;
	this->byte_length = 0;
	this->byte_stride = 0;
	this->target = GLTFBVTarget::ARRAY_BUFFER;
}

GLBufferView::GLBufferView(int buffer_idx, int offset, int length, int stride) : GLBufferView() {
	this->buffer_index = buffer_idx;
	this->byte_offset = offset;
	this->byte_length = length;
	this->byte_stride = stride;
}

GLAccessor::GLAccessor() {
	this->bufferview_index = -1;
	this->byte_offset = 0;
	this->count = 0;
	this->type = GLTFAccType::SCALAR;
	this->component_type = GLTFCompType::FLOAT;
	this->min = nullptr;
	this->max = nullptr;
}

GLAccessor::GLAccessor(GLTFAccType type, GLTFCompType comp_type) : GLAccessor() {
	int size = GLTFAccTypeToInt(type);
	this->type = type;
	this->component_type = comp_type;
	this->min = new uint32_t[size];
	this->max = new uint32_t[size];
}

GLAccessor::~GLAccessor() {
	if (this->min != nullptr) delete this->min;
	if (this->max != nullptr) delete this->max;
}

GLMaterial::GLMaterial() {
	this->alpha_mode = GLTFAlphaMode::NONE;
	this->matalic = 0.0f;
	this->roughness = 1.0f;
	this->base_color[0] = 0.5f;
	this->base_color[1] = 0.5f;
	this->base_color[2] = 0.5f;
	this->base_color[3] = 1.0f;
	this->emissive[0] = 0.0f;
	this->emissive[1] = 0.0f;
	this->emissive[2] = 0.0f;
}

GLMeshAttrs::GLMeshAttrs() {
	this->position_index = -1;
	this->normal_index = -1;
	this->texcoord_0_index = -1;
}

GLMeshAttrs::GLMeshAttrs(int position_idx, int normal_idx, int texcoord_0_idx) {
	this->position_index = position_idx;
	this->normal_index = normal_idx;
	this->texcoord_0_index = texcoord_0_idx;
}

GLPrimitive::GLPrimitive() {
	this->indices = -1;
	this->material_index = -1;
	this->mode = GLTFTopoTypes::TRIANGLES;
}

GLPrimitive::GLPrimitive(int indices_idx, int mat_idx, GLTFTopoTypes mode) {
	this->indices = indices_idx;
	this->material_index = mat_idx;
	this->mode = mode;
}

GLNode::GLNode() {
	this->translation = nullptr;
	this->scale = nullptr;
	this->rotation = nullptr;
	this->name[0] = '\0';
	this->mesh_index = -1;
}

GLNode::GLNode(const char* name, Vector3* position, Vector3* scale, Quaternion* rotation) : GLNode() {
	this->translation = position;
	this->scale = scale;
	this->rotation = rotation;
	std::strcpy(this->name, name);
}

void GLNode::addChild(int node_idx) {
	this->child_indicies.push_back(node_idx);
}

void GLScene::addNode(int node_idx) {
	this->node_indicies.push_back(node_idx);
}

GLTF::GLTF() {
	this->default_scene_index = -1;
}

GLTF::~GLTF() {
	int i;
	for (i = 0; i < this->scenes.size(); i++) {
		delete this->scenes[i];
	}
	for (i = 0; i < this->nodes.size(); i++) {
		delete this->nodes[i];
	}
	for (i = 0; i < this->meshes.size(); i++) {
		delete this->meshes[i];
	}
	for (i = 0; i < this->materials.size(); i++) {
		delete this->materials[i];
	}
	for (i = 0; i < this->accessors.size(); i++) {
		delete this->accessors[i];
	}
	for (i = 0; i < this->buffer_views.size(); i++) {
		delete this->buffer_views[i];
	}
	for (i = 0; i < this->buffers.size(); i++) {
		delete this->buffers[i];
	}
	this->scenes.clear();
	this->nodes.clear();
	this->meshes.clear();
	this->materials.clear();
	this->accessors.clear();
	this->buffer_views.clear();
	this->buffers.clear();
}

void GLTF::save(const char* filename, bool single_glb) {
	int i, j, limit;
	int buffer_shift;
	std::string base_dir;
	std::string bin_filename;
	json data;
	json* subdata;

	// Get base dir if to setup multifile saving later
	if (!single_glb) {
		base_dir = f_base_dir(filename);
	}

	data["asset"] = {
		{"version", "2.0"},
		{"generator", std::string(PGM_NAME_READABLE) + " v" + PGM_VERSION + " " + PGM_REF_LINK}
	};

	if (this->default_scene_index >= 0) {
		data["scene"] = this->default_scene_index;
	}

	// Scenes
	if (this->scenes.size() > 0) data["scenes"] = json::array();
	for (i = 0; i < this->scenes.size(); i++) {
		subdata = new json({});
		// Leave empty if scene has no nodes
		if (this->scenes[i]->node_indicies.size() > 0) {
			(*subdata)["nodes"] = this->scenes[i]->node_indicies;
		}
		data["scenes"].push_back(*subdata);
	}

	// Nodes
	if (this->nodes.size() > 0) data["nodes"] = json::array();
	for (i = 0; i < this->nodes.size(); i++) {
		subdata = new json({});
		(*subdata)["name"] = this->nodes[i]->name;
		(*subdata)["translation"] = {
			this->nodes[i]->translation->x,
			this->nodes[i]->translation->y,
			this->nodes[i]->translation->z
		};
		(*subdata)["rotation"] = {
			this->nodes[i]->rotation->x,
			this->nodes[i]->rotation->y,
			this->nodes[i]->rotation->z,
			this->nodes[i]->rotation->w
		};
		(*subdata)["scale"] = {
			this->nodes[i]->scale->x,
			this->nodes[i]->scale->y,
			this->nodes[i]->scale->z
		};
		if (this->nodes[i]->child_indicies.size() > 0) {
			(*subdata)["children"] = this->nodes[i]->child_indicies;
		}
		if (this->nodes[i]->mesh_index >= 0) {
			(*subdata)["mesh"] = this->nodes[i]->mesh_index;
		}
		data["nodes"].push_back(*subdata);
	}

	// Meshes
	if (this->meshes.size() > 0) data["meshes"] = json::array();
	for (i = 0; i < this->meshes.size(); i++) {
		if (this->meshes[i]->primitives.size() > 0) data["meshes"].push_back({{"primitives", json::array()}});
		for (j = 0; j < this->meshes[i]->primitives.size(); j++) {
			subdata = new json({});
			(*subdata)["mode"] = (int)this->meshes[i]->primitives[j]->mode;
			if (this->meshes[i]->primitives[j]->material_index >= 0) {
				(*subdata)["material"] = this->meshes[i]->primitives[j]->material_index;
			}
			if (this->meshes[i]->primitives[j]->indices >= 0) {
				(*subdata)["indices"] = this->meshes[i]->primitives[j]->indices;
			}
			(*subdata)["attributes"] = json::object();
			if (this->meshes[i]->primitives[j]->attributes.position_index >= 0) {
				(*subdata)["attributes"]["POSITION"] = this->meshes[i]->primitives[j]->attributes.position_index;
			}
			if (this->meshes[i]->primitives[j]->attributes.normal_index >= 0) {
				(*subdata)["attributes"]["NORMAL"] = this->meshes[i]->primitives[j]->attributes.normal_index;
			}
			if (this->meshes[i]->primitives[j]->attributes.texcoord_0_index >= 0) {
				(*subdata)["attributes"]["TEXCOORD_0"] = this->meshes[i]->primitives[j]->attributes.texcoord_0_index;
			}
			data["meshes"][data["meshes"].size()-1]["primitives"].push_back(*subdata);
		}
	}

	// Materials
	if (this->materials.size() > 0) data["materials"] = json::array();
	for (i = 0; i < this->materials.size(); i++) {
		subdata = new json({});
		if (this->materials[i]->alpha_mode != GLTFAlphaMode::NONE) {
			(*subdata)["alphaMode"] = GLTFAlphaModeToString[(int)this->materials[i]->alpha_mode];
		}
		(*subdata)["pbrMetallicRoughness"] = json::object();
		(*subdata)["pbrMetallicRoughness"]["roughnessFactor"] = this->materials[i]->roughness;
		(*subdata)["pbrMetallicRoughness"]["metallicFactor"] = this->materials[i]->matalic;
		(*subdata)["pbrMetallicRoughness"]["baseColorFactor"] = this->materials[i]->base_color;
		if (this->materials[i]->emissive[0] > 0.0f
			&& this->materials[i]->emissive[1] > 0.0f
			&& this->materials[i]->emissive[2] > 0.0f) {
			(*subdata)["emissiveFactor"] = this->materials[i]->emissive;
		}
		data["materials"].push_back(*subdata);
	}

	// Accessors
	if (this->accessors.size() > 0) data["accessors"] = json::array();
	for (i = 0; i < this->accessors.size(); i++) {
		subdata = new json({});
		(*subdata)["bufferView"] = this->accessors[i]->bufferview_index;
		(*subdata)["byteOffset"] = this->accessors[i]->byte_offset;
		(*subdata)["type"] = GLTFAccTypeToString[(int)this->accessors[i]->type];
		(*subdata)["componentType"] = GLTFCompTypeToInt[(int)this->accessors[i]->component_type];
		(*subdata)["count"] = this->accessors[i]->count;
		limit = GLTFAccTypeToInt(this->accessors[i]->type);

		if (this->accessors[i]->max != nullptr) {
			(*subdata)["max"] = json::array();
			for (j = 0; j < limit; j++) {
				GLTFCompTypePushBack(
					this->accessors[i]->component_type,
					(*subdata)["max"],
					this->accessors[i]->max[j]
				);
			}
		}
		if (this->accessors[i]->min != nullptr) {
			(*subdata)["min"] = json::array();
			for (j = 0; j < limit; j++) {
				GLTFCompTypePushBack(
					this->accessors[i]->component_type,
					(*subdata)["min"],
					this->accessors[i]->min[j]
				);
			}
		}
		data["accessors"].push_back(*subdata);
	}

	// BufferViews
	if (this->buffer_views.size() > 0) data["bufferViews"] = json::array();
	for (i = 0; i < this->buffer_views.size(); i++) {
		subdata = new json({});

		if (!single_glb) {
			(*subdata)["buffer"] = this->buffer_views[i]->buffer_index;
		// GLB writes single buffer chunk, force index to zero (will update offset below)
		} else {
			(*subdata)["buffer"] = 0;
		}

		(*subdata)["byteLength"] = this->buffer_views[i]->byte_length;

		// GLB writes single buffer chunk, adjust offset with padding
		if (single_glb) {
			// Get offset from valid previous index
			if (this->buffer_views[i]->buffer_index - 1 >= 0) {
				buffer_shift = this->buffers[this->buffer_views[i]->buffer_index - 1]->data.size();
			} else buffer_shift = 0;
			(*subdata)["byteOffset"] = this->buffer_views[i]->byte_offset
								     + buffer_shift;
		} else {
			(*subdata)["byteOffset"] = this->buffer_views[i]->byte_offset;
		}

		if (this->buffer_views[i]->byte_stride != 0) {
			(*subdata)["byteStride"] = this->buffer_views[i]->byte_stride;
		}
		(*subdata)["target"] = GLTFBVTargetToInt(this->buffer_views[i]->target);
		data["bufferViews"].push_back(*subdata);
	}

	// Buffers
	if (this->buffers.size() > 0) data["buffers"] = json::array();
	// GLTF, Write individual buffer files
	if (!single_glb) {
		for (i = 0; i < this->buffers.size(); i++) {
			if (this->buffers[i]->data.size() == 0) continue;
			bin_filename = f_base_filename_no_ext(filename)
						+ "_" + std::to_string(i) + ".bin";

			subdata = new json({});
			(*subdata)["byteLength"] = this->buffers[i]->data.size();
			(*subdata)["uri"] = bin_filename;
			data["buffers"].push_back(*subdata);

			// Write binary file data: verts, norms, uvs
			std::ofstream f((base_dir + bin_filename).c_str(), std::ios::binary);
			if (!f.is_open()) {
				throw SaveException("Cannot open file for writing \"" + bin_filename + "\"");
			}
			f.write(
				reinterpret_cast<const char*>(this->buffers[i]->data.data()),
				this->buffers[i]->data.size()
			);
			f.close();
		}
	// GLB, buffers will be appended to single binary file later
	} else {
		uint32_t total_size = 0;
		for (i = 0; i < this->buffers.size(); i++) {
			total_size += this->buffers[i]->data.size();
		}
		if (total_size > 0) {
			subdata = new json({});
			(*subdata)["byteLength"] = total_size;
			data["buffers"].push_back(*subdata);
		}
	}
	// If empty, remove buffers
	if (data["buffers"].size() == 0) data.erase("buffers");

	// Write GLTF JSON file
	if (!single_glb) {
		std::ofstream f(filename);
		if (!f.is_open()) {
			throw SaveException("Cannot open file for writing \"" + std::string(filename) + "\"");
		}
		f << data.dump();
		f.close();
	// Write GLB file
	} else {
		uint32_t size_total_bytes = 0;
		uint32_t json_bytes;
		uint32_t buffer_bytes;
		int json_bytes_padding;
		int buffer_bytes_padding;
		const char bin_version[] = {0x02, 0x00, 0x00, 0x00};
		std::string json_str = data.dump();

		// Get paddings and total size
		json_bytes = json_str.size();
		json_bytes_padding = (4 - (json_bytes % 4)) % 4;
		json_bytes += json_bytes_padding;

		buffer_bytes = 0;
		for (i = 0; i < this->buffers.size(); i++) {
			// Add one byte for trailing 0x00 for padding
			buffer_bytes += this->buffers[i]->data.size();
		}
		buffer_bytes_padding = (4 - (buffer_bytes % 4)) % 4;
		buffer_bytes += buffer_bytes_padding;

		size_total_bytes += 12;  // Header
		size_total_bytes += 8 + json_bytes;
		if (buffer_bytes > 0) size_total_bytes += 8 + buffer_bytes;

		std::ofstream f(filename, std::ios::binary);
		if (!f.is_open()) {
			throw SaveException("Cannot open file for writing \"" + std::string(filename) + "\"");
		}

		// Header (magic, version, length (total size))
		f.write("glTF", 4);
		f.write(bin_version, 4);
		f.write(reinterpret_cast<const char*>(&size_total_bytes), 4);

		// JSON data chunk (length, type, data)
		f.write(reinterpret_cast<const char*>(&json_bytes), 4);
		f.write("JSON", 4);
		f.write(json_str.c_str(), json_str.size());
		while (json_bytes_padding > 0) {
			f.write(" ", 1);
			json_bytes_padding -= 1;
		}

		// Buffer data chunk (length, type, data) <- sad we can't have multiple buffer chunks
		// Ignore chunck if buffers are empty
		if (buffer_bytes > 0) {
			f.write(reinterpret_cast<const char*>(&buffer_bytes), 4);
			f.write("BIN\0", 4);
			for (i = 0; i < this->buffers.size(); i++) {
				f.write(
					reinterpret_cast<const char*>(this->buffers[i]->data.data()),
					this->buffers[i]->data.size()
				);
			}
			while (buffer_bytes_padding > 0) {
				f.write("\0", 1);
				buffer_bytes_padding -= 1;
			}
		}
		f.close();
	}
}

std::unordered_map<std::string, int> glmesh_cache;

class MeshGroup {
public:
	Material* material;
	std::vector<int> indices;
	std::vector<Vector3> verts;
	std::vector<Vector3> norms;
};

int addMesh(GLTF& gltf, MeshObj& mnode);
void buildMeshGroupFromMeshObj(MeshObj& mnode, std::vector<MeshGroup>& groups);
template <typename T>
int addBufferWithViewAndAccessor(GLTF& gltf, std::vector<T>& source);
template <typename T>
void getBoundsArray(T* values, size_t count, uint32_t* min, uint32_t* max);

void buildGLTFFromSceneChildren(GLTF& gltf, Node& root, GLNode* parent_node) {
	int mesh_index;
	GLNode* node;
	GLScene* scene = gltf.scenes[0];

	node = new GLNode(root.name.c_str(), &root.position, &root.scale, &root.rotation);

	if (root.type == NodeType::MeshObj) {
		mesh_index = addMesh(gltf, *(MeshObj*)&root);
		node->mesh_index = mesh_index;
	}

	for (int i = 0; i < root.children.size(); i++) {
		buildGLTFFromSceneChildren(gltf, *root.children[i], node);
	}

	// Screen for and ignore empty nodes
	// This check and following push back must be done after building for children
	if (node->mesh_index == -1 && node->child_indicies.size() == 0) {
		delete node;
		return;
	}

	gltf.nodes.push_back(node);
	if (parent_node == NULL) scene->addNode(gltf.nodes.size() - 1);
	else parent_node->addChild(gltf.nodes.size() - 1);
}

GLTF* createGLTFFromScene(Node& scene) {
	GLTF* gltf = new GLTF();
	gltf->scenes.push_back(new GLScene());
	gltf->buffers.push_back(new GLBuffer());
	buildGLTFFromSceneChildren(*gltf, scene, NULL);
	gltf->default_scene_index = 0;
	return gltf;
}

int addMesh(GLTF& gltf, MeshObj& mnode) {
	Material* mmat;
	GLMaterial* material;
	GLPrimitive* mprim;
	GLMesh* mesh;
	int i;
	int attr_index;
	int glmesh_index = -1;
	std::string cache_key = "";
	// TODO: preallocate vectors in gltf where possible

	if (mnode.mesh.surface_count == 0) return -1;

	// Get cache key (combination of mesh unique load id and material id)
	if(mnode.mesh.ul_id != 0) {
		cache_key = "mesh_" + std::to_string(mnode.mesh.ul_id);
		cache_key += "_" + getEntityColorUid(mnode);
	}

	// Check if cache if mesh was already created
	if (glmesh_cache.find(cache_key) != glmesh_cache.end()) {
		glmesh_index = glmesh_cache[cache_key];
	// Create new mesh
	} else {
		// Unfirl mesh
		std::vector<MeshGroup> groups;
		buildMeshGroupFromMeshObj(mnode, groups);
		mesh = new GLMesh();

		for (i = 0; i < groups.size(); i++) {
			// Indices
			addBufferWithViewAndAccessor<int>(gltf, groups[i].indices);
		
			// Create mesh primitive
			mprim = new GLPrimitive(gltf.accessors.size() - 1, -1, GLTFTopoTypes::TRIANGLES);
			mprim->attributes = GLMeshAttrs();
			mesh->primitives.push_back(mprim);
		
			// Position
			attr_index = addBufferWithViewAndAccessor<Vector3>(gltf, groups[i].verts);
			mprim->attributes.position_index = attr_index;

			// Material
			mmat = groups[i].material;
			material = new GLMaterial();
			material->matalic = 0.0f;
			material->roughness = 1.0f;
			material->base_color[0] = mmat->diffuse.x;
			material->base_color[1] = mmat->diffuse.y;
			material->base_color[2] = mmat->diffuse.z;
			material->base_color[3] = mmat->dissolve;
			if (mmat->dissolve < 1.0f) {
				material->alpha_mode = GLTFAlphaMode::BLEND;
			}
			if (mmat->emissive.x > 0.0f || mmat->emissive.y > 0.0f || mmat->emissive.z > 0.0f) {
				material->emissive[0] = mmat->emissive.x;
				material->emissive[1] = mmat->emissive.y;
				material->emissive[2] = mmat->emissive.z;
			}
			gltf.materials.push_back(material);
			mprim->material_index = gltf.materials.size() - 1;
		}
	
		// Finalize
		gltf.meshes.push_back(mesh);
		glmesh_index = gltf.meshes.size() - 1;
		if (cache_key.size() > 0) {
			glmesh_cache[cache_key] = glmesh_index;
		}
	}

	return glmesh_index;
}

void buildMeshGroupFromMeshObj(MeshObj& mnode, std::vector<MeshGroup>& groups) {
	int i, j, k;
	int vert_idx;
	MeshGroup* cur_grp = nullptr;
	std::unordered_map<int, int> index_map;

	for (i = 0; i < mnode.mesh.surface_count; i++) {
		for (j = 0; j < mnode.mesh.surfaces[i].face_count; j++) {
			// New surface material, setup for new mesh group
			if (mnode.mesh.surfaces[i].material_refs->find(j) != mnode.mesh.surfaces[i].material_refs->end()) {
				index_map.clear();
				groups.emplace_back();
				cur_grp = &groups.back();
				cur_grp->material = &mnode.mesh.materials[(*mnode.mesh.surfaces[i].material_refs)[j]];
			}
			if (cur_grp == nullptr) {
				continue;
				// throw GeneralException("Unable to get group from mesh surfaces (missing material)");
			}
			// Add unique verts only, remap indices to copied verts
			for (k = 0; k < 3; k++) {
				vert_idx = mnode.mesh.surfaces[i].faces[j].vert_index[k] - 1;
				if (index_map.find(vert_idx) == index_map.end()) {
					index_map[vert_idx] = cur_grp->verts.size();
					cur_grp->verts.push_back(mnode.mesh.verts[vert_idx]);
				}
				cur_grp->indices.push_back(index_map[vert_idx]);
			}
		}
	}
}

template <typename T>
int addBufferWithViewAndAccessor(GLTF& gltf, std::vector<T>& source) {
	GLAccessor* accessor;
	GLBufferView* buffer_view;
	GLTFBVTarget view_target;
	GLTFAccType acc_type;
	GLTFCompType acc_comp_type;
	int byte_offset;
	int size = source.size();
	std::byte* byte_ptr = reinterpret_cast<std::byte*>(source.data());
	// TODO: may want to split buffer after certain size
	GLBuffer* buffer = gltf.buffers[0];

	if (std::is_same<T, int>::value) {
		view_target = GLTFBVTarget::ELEMENT_ARRAY_BUFFER;
		acc_type = GLTFAccType::SCALAR;
		acc_comp_type = GLTFCompType::UNSIGNED_INT;
	} else if (std::is_same<T, Vector3>::value) {
		view_target = GLTFBVTarget::ARRAY_BUFFER;
		acc_type = GLTFAccType::VEC3;
		acc_comp_type = GLTFCompType::FLOAT;
	}

	byte_offset = buffer->addData(byte_ptr, size * sizeof(T));
	buffer_view = new GLBufferView(0, byte_offset, size * sizeof(T), 0);
	buffer_view->target = view_target;
	gltf.buffer_views.push_back(buffer_view);

	accessor = new GLAccessor(acc_type, acc_comp_type);
	accessor->bufferview_index = gltf.buffer_views.size() - 1;
	accessor->count = size;
	getBoundsArray<T>(source.data(), size, accessor->min, accessor->max);
	gltf.accessors.push_back(accessor);

	return gltf.accessors.size() - 1;
}

template <typename T>
void getBoundsArray(T* values, size_t count, uint32_t* min, uint32_t* max) {
	if (std::is_same<T, Vector2>::value) {
		Vector2 vmin, vmax;
		getBounds<Vector2>((Vector2*)values, count, vmin, vmax);
		std::memcpy(&min[0], &vmin.x, sizeof(uint32_t));
		std::memcpy(&min[1], &vmin.y, sizeof(uint32_t));
		std::memcpy(&max[0], &vmax.x, sizeof(uint32_t));
		std::memcpy(&max[1], &vmax.y, sizeof(uint32_t));
	} else if (std::is_same<T, Vector3>::value) {
		Vector3 vmin, vmax;
		getBounds<Vector3>((Vector3*)values, count, vmin, vmax);
		std::memcpy(&min[0], &vmin.x, sizeof(uint32_t));
		std::memcpy(&min[1], &vmin.y, sizeof(uint32_t));
		std::memcpy(&min[2], &vmin.z, sizeof(uint32_t));
		std::memcpy(&max[0], &vmax.x, sizeof(uint32_t));
		std::memcpy(&max[1], &vmax.y, sizeof(uint32_t));
		std::memcpy(&max[2], &vmax.z, sizeof(uint32_t));
	} else if (std::is_same<T, Quaternion>::value) {
		Quaternion vmin, vmax;
		getBounds<Quaternion>((Quaternion*)values, count, vmin, vmax);
		std::memcpy(&min[0], &vmin.x, sizeof(uint32_t));
		std::memcpy(&min[1], &vmin.y, sizeof(uint32_t));
		std::memcpy(&min[2], &vmin.z, sizeof(uint32_t));
		std::memcpy(&min[3], &vmin.w, sizeof(uint32_t));
		std::memcpy(&max[0], &vmax.x, sizeof(uint32_t));
		std::memcpy(&max[1], &vmax.y, sizeof(uint32_t));
		std::memcpy(&max[2], &vmax.z, sizeof(uint32_t));
		std::memcpy(&max[3], &vmax.w, sizeof(uint32_t));
	} else {
		getBounds<T>(values, count, *(T*)min, *(T*)max);
	}
}