#include "objload.hpp"

#include <vector>
#include <sstream>
#include <fstream>
#include <format>

const int INITIAL_BUFFER = 64;

std::vector<std::string> split_string(const std::string& str, char delimiter) {
	std::vector<std::string> tokens;
	std::stringstream ss(str);
	std::string token;
	while (std::getline(ss, token, delimiter)) {
		tokens.push_back(token);
	}
	return tokens;
}

void ObjWavefront::save(const char* filename) {
	int i;
	std::ofstream f(filename);
	if (!f.is_open()) {
		throw std::exception(std::strcat("Cannot open file for writing ", filename));
	}

	f << "# Yland Extractor v2" << std::endl;
	f << "# https://github.com/BinarySemaphore/ylands_exporter" << std::endl;

	if (this->name.size() > 0) {
		f << "o " << this->name << std::endl;
	} else {
		f << "o Unknown" << std::endl;
	}

	for (i = 0; i < this->vert_count; i++) {
		f << "v " << this->verts[i].x << " " << this->verts[i].y << " " << this->verts[i].z << std::endl;
	}
	for (i = 0; i < this->norm_count; i++) {
		f << "vn " << this->norms[i].x << " " << this->norms[i].y << " " << this->norms[i].z << std::endl;
	}
	for (i = 0; i < this->uv_count; i++) {
		f << "vt " << this->uvs[i].x << " " << this->uvs[i].y << std::endl;
	}
	for (i = 0; i < this->surface_count; i++) {
		f << "s " << i << std::endl;
		for (int j = 0; j < this->surfaces[i].face_count; j++) {
			f << "f";
			for (int k = 0; k < 3; k++) {
				f << " " << this->surfaces[i].faces[j].vert_index[k] << "/";
				if (this->uv_count != 0) f << this->surfaces[i].faces[j].uv_index[k];
				f << "/" << this->surfaces[i].faces[j].norm_index[k];
			}
			f << std::endl;
		}
	}

	f.close();
}

ObjWavefront::ObjWavefront(const char* filename) {
	int state = 0;
	int line_count = 0;
	int face_count = 0;
	int current_buffer = INITIAL_BUFFER;
	Vector3* hold_v3;
	Vector2* hold_v2;
	Surface* hold_s;
	Face* hold_f;
	std::string line;
	std::vector<std::string> line_s;
	std::vector<std::string> line_s_s;
	std::stringstream errmsg;
	std::ifstream f(filename);
	if (!f.is_open()) {
		throw std::exception(std::strcat("Could not open file ", filename));
	}

	this->vert_count = 0;
	this->norm_count = 0;
	this->uv_count = 0;
	this->surface_count = 0;
	this->name = "OBJ";
	this->norms = NULL;
	this->uvs = NULL;
	this->surfaces = NULL;
	this->verts = (Vector3*)malloc(sizeof(Vector3) * current_buffer);
	if (this->verts == NULL) {
		errmsg << "Unable to allocate enough initial memory (" << current_buffer << ") for vertices while loading file " << filename;
		throw std::exception(errmsg.str().c_str());
	}

	while (std::getline(f, line)) {
		line_count += 1;
		if (line[0] == '#') continue;
		if (state == 0 && line[0] == 'o') {
			this->name = line.substr(2, line.length()-2);
			state = 1;
			continue;
		}
		/*
		VERTICIES
		*/
		if (state == 1) {
			if (line[0] == 'v' && line[1] == ' ') {
				line_s = split_string(line, ' ');
				if (line_s.size() != 4) {
					this->free();
					errmsg << "Invalid vertex at line (" << line_count << ") in file " << filename;
					throw std::exception(errmsg.str().c_str());
				}
				if (this->vert_count == current_buffer) {
					current_buffer *= 2;
					hold_v3 = (Vector3*)realloc(this->verts, sizeof(Vector3) * current_buffer);
					if (hold_v3 == NULL) {
						this->free();
						errmsg << "Unable to allocate enough additional memory (" << current_buffer << ") for vertices while loading file " << filename;
						throw std::exception(errmsg.str().c_str());
					}
					this->verts = hold_v3;
				}
				this->verts[this->vert_count].x = std::stof(line_s[1]);
				this->verts[this->vert_count].y = std::stof(line_s[2]);
				this->verts[this->vert_count].z = std::stof(line_s[3]);
				this->vert_count += 1;
				continue;
			} else {
				state = 2;
				if (this->vert_count == 0) {
					this->free();
					errmsg << "No vertex data found in file " << filename;
					throw std::exception(errmsg.str().c_str());
				}
				hold_v3 = (Vector3*)realloc(this->verts, sizeof(Vector3) * this->vert_count);
				if (hold_v3 == NULL) {
					this->free();
					errmsg << "Unable to reallocate memory (" << this->vert_count << ") for verticies while loading file " << filename;
					throw std::exception(errmsg.str().c_str());
				}
				this->verts = hold_v3;
				current_buffer = INITIAL_BUFFER;
				this->norms = (Vector3*)malloc(sizeof(Vector3) * current_buffer);
				if (this->norms == NULL) {
					this->free();
					errmsg << "Unable to allocate enough initial memory (" << current_buffer << ") for vertex normals while loading file " << filename;
					throw std::exception(errmsg.str().c_str());
				}
			}
		}
		/*
		Normals
		*/
		if (state == 2) {
			if (line[0] == 'v' && line[1] == 'n') {
				line_s = split_string(line, ' ');
				if (line_s.size() != 4) {
					this->free();
					errmsg << "Invalid vertex normal at line (" << line_count << ") in file " << filename;
					throw std::exception(errmsg.str().c_str());
				}
				if (this->norm_count == current_buffer) {
					current_buffer *= 2;
					hold_v3 = (Vector3*)realloc(this->norms, sizeof(Vector3) * current_buffer);
					if (hold_v3 == NULL) {
						this->free();
						errmsg << "Unable to allocate enough additional memory (" << current_buffer << ") for vertex normals while loading file " << filename;
						throw std::exception(errmsg.str().c_str());
					}
					this->norms = hold_v3;
				}
				this->norms[this->norm_count].x = std::stof(line_s[1]);
				this->norms[this->norm_count].y = std::stof(line_s[2]);
				this->norms[this->norm_count].z = std::stof(line_s[3]);
				this->norm_count += 1;
				continue;
			} else {
				state = 3;
				if (this->norm_count == 0) {
					this->free();
					errmsg << "No vertex normal data found in file " << filename;
					throw std::exception(errmsg.str().c_str());
				}
				hold_v3 = (Vector3*)realloc(this->norms, sizeof(Vector3) * this->norm_count);
				if (hold_v3 == NULL) {
					this->free();
					errmsg << "Unable to reallocate memory (" << this->norm_count << ") for vertex normals while loading file " << filename;
					throw std::exception(errmsg.str().c_str());
				}
				this->norms = hold_v3;
				current_buffer = INITIAL_BUFFER;
				this->uvs = (Vector2*)malloc(sizeof(Vector2) * current_buffer);
				if (this->uvs == NULL) {
					this->free();
					errmsg << "Unable to allocate enough initial memory (" << current_buffer << ") for vertex UVs while loading file " << filename;
					throw std::exception(errmsg.str().c_str());
				}
			}
		}
		/*
		UVs
		*/
		if (state == 3) {
			if (line[0] == 'v' && line[1] == 't') {
				line_s = split_string(line, ' ');
				if (line_s.size() != 3) {
					this->free();
					errmsg << "Invalid vertex UV at line (" << line_count << ") in file " << filename;
					throw std::exception(errmsg.str().c_str());
				}
				if (this->uv_count == current_buffer) {
					current_buffer *= 2;
					hold_v2 = (Vector2*)realloc(this->uvs, sizeof(Vector2) * current_buffer);
					if (hold_v2 == NULL) {
						this->free();
						errmsg << "Unable to allocate enough additional memory (" << current_buffer << ") for vertex UVs while loading file " << filename;
						throw std::exception(errmsg.str().c_str());
					}
					this->uvs = hold_v2;
				}
				this->uvs[this->uv_count].x = std::stof(line_s[1]);
				this->uvs[this->uv_count].y = std::stof(line_s[2]);
				this->uv_count += 1;
				continue;
			} else {
				state = 4;
				if (this->uv_count > 0) {
					hold_v2 = (Vector2*)realloc(this->uvs, sizeof(Vector2) * this->uv_count);
					if (hold_v2 == NULL) {
						this->free();
						errmsg << "Unable to reallocate memory (" << this->uv_count << ") for vertex UVs while loading file " << filename;
						throw std::exception(errmsg.str().c_str());
					}
					this->uvs = hold_v2;
				} else std::free(this->uvs);
				current_buffer = INITIAL_BUFFER;
				this->surfaces = (Surface*)malloc(sizeof(Surface) * current_buffer);
				if (this->surfaces == NULL) {
					this->free();
					errmsg << "Unable to allocate enough initial memory (" << current_buffer << ") for surfaces while loading file " << filename;
					throw std::exception(errmsg.str().c_str());
				}
			}
		}
		/*
		Surfaces
		*/
		if (state > 3) {
			if (state == 4 && line[0] == 's') {
				if (this->surface_count == current_buffer) {
					hold_s = (Surface*)realloc(this->surfaces, sizeof(Surface) * current_buffer);
					if (hold_s == NULL) {
						this->free();
						errmsg << "Unable to allocate enough additional memory (" << current_buffer << ") for surfaces while loading file " << filename;
						throw std::exception(errmsg.str().c_str());
					}
					this->surfaces = hold_s;
				}
				this->surfaces[this->surface_count].face_count = 0;
				this->surfaces[this->surface_count].faces = (Face*)malloc(sizeof(Face) * current_buffer);
				if (this->surfaces == NULL) {
					this->free();
					errmsg << "Unable to allocate enough initial memory (" << current_buffer << ") for surface faces while loading file " << filename;
					throw std::exception(errmsg.str().c_str());
				}
				this->surface_count += 1;
				face_count = 0;
				state = 5;
				continue;
			}
			/*
			Faces
			*/
			if (state == 5 && line[0] == 'f') {
				line_s = split_string(line, ' ');
				if (line_s.size() != 4) {
					this->free();
					errmsg << "Invalid Face (expected triangle data) at line (" << line_count << ") in file " << filename;
					throw std::exception(errmsg.str().c_str());
				}
				if (face_count == current_buffer) {
					current_buffer *= 2;
					hold_f = (Face*)realloc(this->surfaces[this->surface_count-1].faces, sizeof(Face) * current_buffer);
					if (hold_f == NULL) {
						this->free();
						errmsg << "Unable to allocate enough additional memory (" << current_buffer << ") for faces of surface " << this->surface_count - 1 << " while loading file " << filename;
						throw std::exception(errmsg.str().c_str());
					}
					this->surfaces[this->surface_count-1].faces = hold_f;
				}
				for (int i = 0; i < 3; i++) {
					line_s_s = split_string(line_s[i+1], '/');
					if (line_s_s.size() != 3) {
						this->free();
						errmsg << "Invalid Face point (expected 2 slashes '/') at line (" << line_count << ") in file " << filename;
						throw std::exception(errmsg.str().c_str());
					}
					this->surfaces[this->surface_count-1].faces[face_count].vert_index[i] = std::stoi(line_s_s[0]);
					if (line_s_s[1].size() > 0) this->surfaces[this->surface_count-1].faces[face_count].uv_index[i] = std::stoi(line_s_s[1]);
					this->surfaces[this->surface_count-1].faces[face_count].norm_index[i] = std::stoi(line_s_s[2]);
				}
				face_count += 1;
			} else {
				state = 4;
				hold_f = (Face*)realloc(this->surfaces[this->surface_count-1].faces, sizeof(Face) * face_count);
				if (hold_f == NULL) {
					this->free();
					errmsg << "Unable to reallocate memory (" << face_count << ") for faces of surface " << this->surface_count - 1 << " while loading file " << filename;
					throw std::exception(errmsg.str().c_str());
				}
				this->surfaces[this->surface_count-1].faces = hold_f;
				this->surfaces[this->surface_count-1].face_count = face_count;
				current_buffer = INITIAL_BUFFER;
			}
		}
	}

	// Finish face
	if (state == 5) {
		state = 4;
		hold_f = (Face*)realloc(this->surfaces[this->surface_count-1].faces, sizeof(Face) * face_count);
		if (hold_f == NULL) {
			this->free();
			errmsg << "Unable to reallocate memory (" << face_count << ") for final faces of final surface " << this->surface_count - 1 << " while loading file " << filename;
			throw std::exception(errmsg.str().c_str());
		}
		this->surfaces[this->surface_count-1].faces = hold_f;
		this->surfaces[this->surface_count-1].face_count = face_count;
	}

	// Finish surface
	if (state == 4) {
		hold_s = (Surface*)realloc(this->surfaces, sizeof(Surface) * this->surface_count);
		if (hold_s == NULL) {
			this->free();
			errmsg << "Unable to reallocate memory (" << this->surface_count << ") for surfaces while loading file " << filename;
			throw std::exception(errmsg.str().c_str());
		}
		this->surfaces = hold_s;
	}

	f.close();
}

void ObjWavefront::free() {
	if (this->verts != NULL) std::free(this->verts);
	if (this->norms != NULL) std::free(this->norms);
	if (this->uvs != NULL) std::free(this->uvs);
	if (this->surfaces != NULL) {
		for (int i = 0; i < this->surface_count; i++) {
			std::free(this->surfaces[i].faces);
		}
		std::free(this->surfaces);
	}
}