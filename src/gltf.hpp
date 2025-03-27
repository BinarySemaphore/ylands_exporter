// Based on KHRONOS Group GLTF 2.0
// Specs: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
// Quick Ref: https://www.khronos.org/files/gltf20-reference-guide.pdf
#ifndef GLTF_H
#define GLTF_H

#include <any>
#include <vector>

// Forward declaration to avoid using headers and getting multiple redefines
class Vector3;
class Quaternion;
class Node;

enum class GLTFBVTarget {
	ARRAY_BUFFER,
	ELEMENT_ARRAY_BUFFER
};

enum class GLTFAlphaMode {
	NONE,
	BLEND
};

enum class GLTFAccType {
	SCALAR,
	VEC2,
	VEC3,
	VEC4
};

enum class GLTFCompType {
	BYTE,
	UNSIGNED_BYTE,
	SHORT,
	UNSIGNED_SHORT,
	UNSIGNED_INT,
	FLOAT
};

enum class GLTFTopoTypes {
	POINTS,
	LINE_STRIPS,
	LINE_LOOPS,
	LINES,
	TRIANGLES,
	TRIANGLE_STRIPS,
	TIRANGLE_FANS
};

/// URI and Byte Length auto generated on save
class GLBuffer {
public:
	// int byte_length;
	// char uri[128];
	std::vector<std::byte> data;

	GLBuffer() = default;
	int addData(std::byte* data, int size);
};

class GLBufferView {
public:
	int buffer_index;
	int byte_offset;
	int byte_length;
	int byte_stride;
	GLTFBVTarget target;

	GLBufferView();
	GLBufferView(int buffer_idx, int offset, int length, int stride);
};

class GLAccessor {
public:
	int bufferview_index;
	int byte_offset;
	int count;
	GLTFAccType type;
	GLTFCompType component_type;
	uint32_t* min;
	uint32_t* max;

	GLAccessor();
	GLAccessor(GLTFAccType type, GLTFCompType comp_type);
	~GLAccessor();
};

// Note: Actual materials contain textures and support other implementations
// This is a simplified PBR default material model with textures ignored
class GLMaterial {
public:
	GLTFAlphaMode alpha_mode;
	float matalic;
	float roughness;
	float base_color[4];
	float emissive[3];

	GLMaterial();
};

// Note: Actual mesh attributes can contain additional textcoord and tangent
class GLMeshAttrs {
public:
	int position_index;
	int normal_index;
	int texcoord_0_index;

	GLMeshAttrs();
	GLMeshAttrs(int position_idx, int normal_idx, int texcoord_0_idx);
};

class GLPrimitive {
public:
	int indices;
	int material_index;
	GLTFTopoTypes mode;
	GLMeshAttrs attributes;

	GLPrimitive();
	GLPrimitive(int indices_idx, int mat_idx, GLTFTopoTypes mode);
};

class GLMesh {
public:
	std::vector<GLPrimitive*> primitives;
};

class GLNode {
public:
	Vector3* translation;
	Vector3* scale;
	Quaternion* rotation;
	char name[128];
	std::vector<int> child_indicies;
	int mesh_index;

	GLNode();
	GLNode(const char* name, Vector3* position, Vector3* scale, Quaternion* rotation);
	void addChild(int node_idx);
};

class GLScene {
public:
	std::vector<int> node_indicies;

	void addNode(int node_idx);
};

class GLTF {
public:
	int default_scene_index;
	std::vector<GLScene*> scenes;
	std::vector<GLNode*> nodes;
	std::vector<GLMesh*> meshes;
	std::vector<GLMaterial*> materials;
	std::vector<GLAccessor*> accessors;
	std::vector<GLBufferView*> buffer_views;
	std::vector<GLBuffer*> buffers;

	GLTF();
	~GLTF();
	void save(const char* filename, bool single_glb);
};

GLTF* createGLTFFromScene(Node& scene);

#endif // GLTF_H