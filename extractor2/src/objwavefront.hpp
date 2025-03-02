#ifndef OBJWAVEFRONT_H
#define OBJWAVEFRONT_H

#include <string>

class Vector3 {
public:
	float x, y, z;
};

class Vector2 {
public:
	float x, y;
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
};

class ObjWavefront {
public:
	int vert_count;
	int norm_count;
	int uv_count;
	int surface_count;
	std::string name;
	Vector3* verts;
	Vector3* norms;
	Vector2* uvs;
	Surface* surfaces;
	void save(const char* filename);
	ObjWavefront(const char* filename);
	void free();
};

#endif // OBJWAVEFRONT_H