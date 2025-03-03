#ifndef OBJWAVEFRONT_H
#define OBJWAVEFRONT_H

#include <string>
#include <vector>
#include <unordered_map>

enum class IllumModel {
	COLOR_ON_AMBIENT_OFF,
	COLOR_ON_AMBIENT_ON,
	HIGHLIGHT_ON,
	REFLECT_ON_RAYTRACE_ON,
	TRANSP_GLASS_ON_REFLECT_RAYTRACE_ON,
	REFLECT_FRESNEL_ON_RAYTRACE_ON,
	TRANSP_REFRACT_ON_REFLECT_FRESNEL_OFF_RAYTRACE_ON,
	TRANSP_REFRACT_ON_REFLECT_FRESNEL_ON_RAYTRACE_ON,
	REFLECT_ON_RAYTRACE_OFF,
	TRANSP_GLASS_ON_REFLECT_RAYTRACE_OFF,
	CAST_SHADOWS_ONTO_INVIS_SURFACES
};

enum class ObjReadState {
	OBJECT,
	VERTICIES,
	NORMALS,
	UVS,
	SURFACES,
	FACES
};

class Vector2 {
public:
	float x, y;
	Vector2();
	Vector2(float x, float y);
};

class Vector3 {
public:
	float x, y, z;
	Vector3();
	Vector3(float x, float y, float z);
};

class Material {
public:
	IllumModel illum_model;
	float dissolve;
	float optical_density;
	float spec_exp;
	Vector3 ambient;
	Vector3 diffuse;
	Vector3 specular;
	Vector3 emissive;
	std::string name;
	Material();
	static void save(const char* filename, const std::vector<Material>& materials);
	static std::vector<Material> load(const char* filename);
};

class Face {
public:
	int vert_index[3];
	int norm_index[3];
	int uv_index[3];
};

class Surface {
public:
	int face_count;
	Face* faces;
	std::unordered_map<int, std::string>* material_refs;
};

class ObjWavefront {
public:
	int vert_count;
	int norm_count;
	int uv_count;
	int surface_count;
	Vector3* verts;
	Vector3* norms;
	Vector2* uvs;
	Surface* surfaces;
	std::string name;
	std::unordered_map<std::string, Material> materials;
	void save(const char* filename);
	ObjWavefront(const char* filename);
	void free();
};

#endif // OBJWAVEFRONT_H