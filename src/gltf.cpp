#include "gltf.hpp"

#include <type_traits>
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

int GLBuffer::addData(std::byte* data, int size) {
	int i;
	int index = this->data.size();
	for (i = 0; i < size; i++) {
		this->data.push_back(data[i]);
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

void GLTF::save(const char* filename) {
	int i, j, limit;
	int bf_count = 0;
	std::string base_dir;
	std::string bin_filename;
	json data;
	json* subdata;

	base_dir = f_base_dir(filename);

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
		data["scenes"].push_back({
			{"nodes", this->scenes[i]->node_indicies}
		});
	}

	// Nodes
	if (this->nodes.size() > 0) data["nodes"] = json::array();
	for (i = 0; i < this->nodes.size(); i++) {
		subdata = new json();
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
			subdata = new json();
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
		subdata = new json();
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
			(*subdata)["pbrMetallicRoughness"]["emissiveFactor"] = this->materials[i]->emissive;
		}
		data["materials"].push_back(*subdata);
	}

	// Accessors
	if (this->accessors.size() > 0) data["accessors"] = json::array();
	for (i = 0; i < this->accessors.size(); i++) {
		subdata = new json();
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
		subdata = new json();
		(*subdata)["buffer"] = this->buffer_views[i]->buffer_index;
		(*subdata)["byteLength"] = this->buffer_views[i]->byte_length;
		(*subdata)["byteOffset"] = this->buffer_views[i]->byte_offset;
		if (this->buffer_views[i]->byte_stride != 0) {
			(*subdata)["byteStride"] = this->buffer_views[i]->byte_stride;
		}
		(*subdata)["target"] = GLTFBVTargetToInt(this->buffer_views[i]->target);
		data["bufferViews"].push_back(*subdata);
	}

	// Buffers
	if (this->buffers.size() > 0) data["buffers"] = json::array();
	for (i = 0; i < this->buffers.size(); i++) {
		bin_filename = f_base_filename_no_ext(filename)
					 + "_" + std::to_string(bf_count) + ".bin";
		bf_count += 1;

		subdata = new json();
		(*subdata)["byteLength"] = this->buffers[i]->data.size();
		(*subdata)["uri"] = bin_filename;
		data["buffers"].push_back(*subdata);

		// // Setup byte array for little endian write
		// std::reverse(this->buffers[i]->data.begin(), this->buffers[i]->data.end());

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

	// Write GLTF JSON file
	std::ofstream f(filename);
	if (!f.is_open()) {
		throw SaveException("Cannot open file for writing \"" + std::string(filename) + "\"");
	}
	f << data.dump();
	f.close();
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
void getBounds(T* values, int count, uint32_t* min, uint32_t* max);

void buildGLTFFromSceneChildren(GLTF& gltf, Node& root, GLNode* parent_node) {
	int mesh_index;
	GLNode* node;
	GLScene* scene = gltf.scenes[0];

	node = new GLNode(root.name.c_str(), &root.position, &root.scale, &root.rotation);
	gltf.nodes.push_back(node);
	if (parent_node == NULL) scene->addNode(gltf.nodes.size() - 1);
	else parent_node->addChild(gltf.nodes.size() - 1);

	if (root.type == NodeType::MeshObj) {
		mesh_index = addMesh(gltf, *(MeshObj*)&root);
		node->mesh_index = mesh_index;
	}

	for (int i = 0; i < root.children.size(); i++) {
		buildGLTFFromSceneChildren(gltf, *root.children[i], node);
	}
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
		mesh->primitives;

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
		
			// Normals
			attr_index = addBufferWithViewAndAccessor<Vector3>(gltf, groups[i].norms);
			mprim->attributes.normal_index = attr_index;

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
	int vert_idx, norm_idx;
	int count;
	MeshGroup* cur_grp = nullptr;
	
	for (i = 0; i < mnode.mesh.surface_count; i++) {
		for (j = 0; j < mnode.mesh.surfaces[i].face_count; j++) {
			if (mnode.mesh.surfaces[i].material_refs->find(j) != mnode.mesh.surfaces[i].material_refs->end()) {
				count = 0;
				groups.emplace_back();
				cur_grp = &groups[groups.size() - 1];
				cur_grp->material = &mnode.mesh.materials[(*mnode.mesh.surfaces[i].material_refs)[j]];
			}
			if (cur_grp == nullptr) {
				throw GeneralException("Unable to get group from mesh surfaces (missing material)");
			}
			for (k = 0; k < 3; k++) {
				cur_grp->indices.push_back(count);
				vert_idx = mnode.mesh.surfaces[i].faces[j].vert_index[k] - 1;
				norm_idx = mnode.mesh.surfaces[i].faces[j].norm_index[k] - 1;
				cur_grp->verts.push_back(mnode.mesh.verts[vert_idx]);
				cur_grp->norms.push_back(mnode.mesh.norms[norm_idx]);
				count += 1;
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
	getBounds<T>(source.data(), size, accessor->min, accessor->max);
	gltf.accessors.push_back(accessor);

	return gltf.accessors.size() - 1;
}

template <typename T>
void getBounds(T* values, int count, uint32_t* min, uint32_t* max) {
	int i;
	if (std::is_same<T, Vector2>::value) {
		Vector2& vval = (Vector2&)values[0];
		float fmin[2], fmax[2];
		fmin[0] = fmin[0] = vval.x;
		fmin[1] = fmax[1] = vval.y;
		for (i = 1; i < count; i++) {
			vval = (Vector2&)values[i];
			if (vval.x > fmax[0]) fmax[0] = vval.x;
			if (vval.y > fmax[1]) fmax[1] = vval.y;
			if (vval.x < fmin[0]) fmin[0] = vval.x;
			if (vval.y < fmin[1]) fmin[1] = vval.y;
		}
		for (i = 0; i < 2; i++) {
			std::memcpy(&min[i], &fmin[i], sizeof(uint32_t));
			std::memcpy(&max[i], &fmax[i], sizeof(uint32_t));
		}
	} else if (std::is_same<T, Vector3>::value) {
		Vector3& vval = (Vector3&)values[0];
		float fmin[3], fmax[3];
		fmin[0] = fmax[0] = vval.x;
		fmin[1] = fmax[1] = vval.y;
		fmin[2] = fmax[2] = vval.z;
		for (i = 1; i < count; i++) {
			vval = (Vector3&)values[i];
			if (vval.x > fmax[0]) fmax[0] = vval.x;
			if (vval.y > fmax[1]) fmax[1] = vval.z;
			if (vval.z > fmax[2]) fmax[2] = vval.y;
			if (vval.x < fmin[0]) fmin[0] = vval.x;
			if (vval.y < fmin[1]) fmin[1] = vval.z;
			if (vval.z < fmin[2]) fmin[2] = vval.y;
		}
		for (i = 0; i < 3; i++) {
			std::memcpy(&min[i], &fmin[i], sizeof(uint32_t));
			std::memcpy(&max[i], &fmax[i], sizeof(uint32_t));
		}
	} else if (std::is_same<T, Quaternion>::value) {
		Quaternion& qval = (Quaternion&)values[0];
		float fmin[4], fmax[4];
		fmin[0] = fmax[0] = qval.x;
		fmin[1] = fmax[1] = qval.y;
		fmin[2] = fmax[2] = qval.z;
		fmin[3] = fmax[3] = qval.w;
		for (i = 1; i < count; i++) {
			qval = (Quaternion&)values[i];
			if (qval.x > fmax[0]) fmax[0] = qval.x;
			if (qval.y > fmax[1]) fmax[1] = qval.y;
			if (qval.z > fmax[2]) fmax[2] = qval.z;
			if (qval.w > fmax[3]) fmax[3] = qval.w;
			if (qval.x < fmin[0]) fmin[0] = qval.x;
			if (qval.y < fmin[1]) fmin[1] = qval.y;
			if (qval.z < fmin[2]) fmin[2] = qval.z;
			if (qval.w < fmin[3]) fmin[3] = qval.w;
		}
		for (i = 0; i < 4; i++) {
			std::memcpy(&min[i], &fmin[i], sizeof(uint32_t));
			std::memcpy(&max[i], &fmax[i], sizeof(uint32_t));
		}
	} else {
		uint32_t& val = (uint32_t&)values[0];
		min[0] = max[0] = val;
		for (i = 1; i < count; i++) {
			val = (uint32_t&)values[i];
			if (val > max[0]) max[0] = val;
			if (val < min[0]) min[0] = val;
		}
	}
}