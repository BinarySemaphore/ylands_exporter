#include "objwavefront.hpp"

#include <fstream>
#include <iomanip>

#include "utils.hpp"

const int INITIAL_BUFFER = 64;
const char* DEFAULT_NAME = "Unnamed";
const char* HEADER_LINE = "# Yland Extractor v2\n"
						  "# https://github.com/BinarySemaphore/ylands_exporter";

Vector2::Vector2() {
	this->x = 0.0f;
	this->y = 0.0f;
}

Vector2::Vector2(float x, float y) {
	this->x = x;
	this->y = y;
}

Vector3::Vector3() {
	this->x = 0.0f;
	this->y = 0.0f;
	this->z = 0.0f;
}

Vector3::Vector3(float x, float y, float z) {
	this->x = x;
	this->y = y;
	this->z = z;
}

Material::Material() {
	this->illum_model = IllumModel::HIGHLIGHT_ON;
	this->dissolve = 1.0f;
	this->optical_density = 1.0f;
	this->spec_exp = 50.0f;
	this->ambient = Vector3(1.0f, 1.0f, 1.0f);
	this->diffuse = Vector3(1.0f, 1.0f, 1.0f);
	this->specular = Vector3(0.5f, 0.5f, 0.5f);
	this->emissive = Vector3();
	this->name = DEFAULT_NAME;
}

void Material::save(const char* filename, const std::vector<Material>& materials) {
	std::ofstream f(filename);
	if (!f.is_open()) {
		throw SaveException(
			"Cannot open file for writing \"" + std::string(filename) + "\""
		);
	}

	f << std::fixed << std::setprecision(6);
	f << HEADER_LINE;

	for (int i=0; i < materials.size(); i++) {
		f << "\n\nnewmtl " << materials[i].name;
		f << "\nKa "
		 << materials[i].ambient.x << " "
		 << materials[i].ambient.y << " "
		 << materials[i].ambient.z;
		f << "\nKd "
		 << materials[i].diffuse.x << " "
		 << materials[i].diffuse.y << " "
		 << materials[i].diffuse.z;
		f << "\nKs "
		 << materials[i].specular.x << " "
		 << materials[i].specular.y << " "
		 << materials[i].specular.z;
		f << "\nKe "
		 << materials[i].emissive.x << " "
		 << materials[i].emissive.y << " "
		 << materials[i].emissive.z;
		f << "\nNs " << materials[i].spec_exp;
		f << "\nNi " << materials[i].optical_density;
		f << "\nd " << materials[i].dissolve;
		f << "\nillum " << (int)materials[i].illum_model;
	}

	f.close();
}

std::vector<Material> Material::load(const char* filename) {
	int line_count = 0;
	int mat_count = 0;
	Material* cur_material = NULL;
	std::string line;
	// Substring
	std::vector<std::string> line_s;
	std::vector<Material> materials;

	std::ifstream f(filename);
	if (!f.is_open()) {
		throw LoadException(
			"Cannot open material file \"" + std::string(filename) + "\""
		);
	}

	try {
	while (std::getline(f, line)) {
		line_count += 1;
		if (line.size() <= 2) continue;
		if (line[0] == '#') continue;
		if (cur_material != NULL) {
			// Specular Exp
			if (line[0] == 'N' && line[1] == 's') {
				line_s = string_split(line, ' ');
				if (line_s.size() != 2) throw ParseException("Invalid specular exponent");
				cur_material->spec_exp = std::stof(line_s[1]);
				continue;
			}
			// Optical Density
			if (line[0] == 'N' && line[1] == 'i') {
				line_s = string_split(line, ' ');
				if (line_s.size() != 2) throw ParseException("Invalid optical density");
				cur_material->optical_density = std::stof(line_s[1]);
				continue;
			}
			// Dissolve
			if (line[0] == 'd') {
				line_s = string_split(line, ' ');
				if (line_s.size() != 2) throw ParseException("Invalid dissolve");
				cur_material->dissolve = std::stof(line_s[1]);
				continue;
			}
			// Ambient Color
			if (line[0] == 'K' && line[1] == 'a') {
				line_s = string_split(line, ' ');
				if (line_s.size() != 4) throw ParseException("Invalid ambient color");
				cur_material->ambient.x = std::stof(line_s[1]);
				cur_material->ambient.y = std::stof(line_s[2]);
				cur_material->ambient.z = std::stof(line_s[3]);
				continue;
			}
			// Diffuse Color
			if (line[0] == 'K' && line[1] == 'd') {
				line_s = string_split(line, ' ');
				if (line_s.size() != 4) throw ParseException("Invalid diffuse color");
				cur_material->diffuse.x = std::stof(line_s[1]);
				cur_material->diffuse.y = std::stof(line_s[2]);
				cur_material->diffuse.z = std::stof(line_s[3]);
				continue;
			}
			// Specular Color
			if (line[0] == 'K' && line[1] == 's') {
				line_s = string_split(line, ' ');
				if (line_s.size() != 4) throw ParseException("Invalid specular color");
				cur_material->specular.x = std::stof(line_s[1]);
				cur_material->specular.y = std::stof(line_s[2]);
				cur_material->specular.z = std::stof(line_s[3]);
				continue;
			}
			// Emissive Color
			if (line[0] == 'K' && line[1] == 'e') {
				line_s = string_split(line, ' ');
				if (line_s.size() != 4) throw ParseException("Invalid emissive color");
				cur_material->emissive.x = std::stof(line_s[1]);
				cur_material->emissive.y = std::stof(line_s[2]);
				cur_material->emissive.z = std::stof(line_s[3]);
				continue;
			}
			// Illumination Model
			if (line.rfind("illum", 0) == 0) {
				line_s = string_split(line, ' ');
				if (line_s.size() != 2) throw ParseException("Invalid illumination model");
				cur_material->illum_model = (IllumModel)std::stoi(line_s[1]);
				continue;
			}
		}
		// New Material
		if (line.rfind("newmtl ", 0) == 0) {
			line = line.substr(7, line.size() - 7);
			if (line.size() == 0) throw ParseException("Invalid new material (missing name)"); 
			materials.emplace_back();
			cur_material = &materials[mat_count];
			cur_material->name = string_replace(line, " ", "_");
			mat_count += 1;
		}
	}
	} catch (std::invalid_argument& e) {
		throw LoadException(
			"Invalid argument at line (" + std::to_string(line_count)
			+ ") in file \"" + std::string(filename) + "\""
		);
	} catch (std::out_of_range& e) {
		throw LoadException(
			"Invalid argument range at line (" + std::to_string(line_count)
			+ ") in file \"" + std::string(filename) + "\""
		);
	} catch (std::bad_alloc& e) {
		throw LoadException(
			"Failed to allocate enough memory for materials while "
			"loading file \"" + std::string(filename) + "\""
		);
	} catch (std::length_error& e) {
		throw LoadException(
			"Failed to allocate enough initial memory for materials while "
			"loading file \"" + std::string(filename) + "\""
		);
	} catch (ParseException& e) {;
		throw LoadException(
			std::string(e.what()) + " at line (" + std::to_string(line_count)
			+ ") in file \"" + std::string(filename) + "\""
		);
	}

	f.close();
	return materials;
}

void ObjWavefront::save(const char* filename) {
	int i;
	std::vector<std::string> material_files;

	// Save Material Library
	if (this->materials.size() > 0) {
		std::string base_dir = f_base_dir(filename);
		std::string mtl_filename = f_base_filename_no_ext(filename) + ".mtl";
		std::vector<Material> materials_flat;
		for (std::pair<std::string, Material> kv : this->materials) {
			materials_flat.push_back(kv.second);
		}
		Material::save((base_dir + mtl_filename).c_str(), materials_flat);
		material_files.push_back(mtl_filename);
	}

	std::ofstream f(filename);
	if (!f.is_open()) {
		throw SaveException(std::strcat("Cannot open file for writing ", filename));
	}

	f << std::fixed << std::setprecision(6);
	f << HEADER_LINE;

	// Material Library
	for (i = 0; i < material_files.size(); i++) {
		f << "\nmtllib " << material_files[i];
	}

	// Object
	f << "\no " << this->name;

	// Vectors
	for (i = 0; i < this->vert_count; i++) {
		f << "\nv "
		 << this->verts[i].x << " "
		 << this->verts[i].y << " "
		 << this->verts[i].z;
	}
	// Normals
	for (i = 0; i < this->norm_count; i++) {
		f << "\nvn "
		 << this->norms[i].x << " "
		 << this->norms[i].y << " "
		 << this->norms[i].z;
	}
	// UVs
	for (i = 0; i < this->uv_count; i++) {
		f << "\nvt "
		 << this->uvs[i].x << " "
		 << this->uvs[i].y;
	}
	// Surfaces
	for (i = 0; i < this->surface_count; i++) {
		f << "\ns " << i;
		// Faces
		for (int j = 0; j < this->surfaces[i].face_count; j++) {
			// Material Reference
			if (this->surfaces[i].material_refs->find(j) != this->surfaces[i].material_refs->end()) {
				f << "\nusemtl " << (*this->surfaces[i].material_refs)[j];
			}

			f << "\nf";  // Note: space moved to forward of face data in loop
			for (int k = 0; k < 3; k++) {
				f << " " << this->surfaces[i].faces[j].vert_index[k] << "/";
				if (this->uv_count != 0) {
					f << this->surfaces[i].faces[j].uv_index[k];
				}
				f << "/" << this->surfaces[i].faces[j].norm_index[k];
			}
		}
	}

	f.close();
}

ObjWavefront::ObjWavefront(const char* filename) {
	int line_count = 0;
	int face_count = 0;
	int current_buffer = INITIAL_BUFFER;
	ObjReadState state = ObjReadState::OBJECT;
	Surface* cur_surface = NULL;
	Vector3* hold_v3;
	Vector2* hold_v2;
	Surface* hold_s;
	Face* hold_f;
	std::string base_dir;
	std::string line;
	// Substring and sub-substring
	std::vector<std::string> line_s;
	std::vector<std::string> line_s_s;
	std::vector<Material> new_materials;

	base_dir = f_base_dir(filename);
	std::ifstream f(filename);
	if (!f.is_open()) {
		throw LoadException(
			"Cannot open Obj Wavefront file \"" + std::string(filename) + "\""
		);
	}

	this->vert_count = 0;
	this->norm_count = 0;
	this->uv_count = 0;
	this->surface_count = 0;
	this->name = DEFAULT_NAME;
	this->norms = NULL;
	this->uvs = NULL;
	this->surfaces = NULL;

	try {
	this->verts = (Vector3*)malloc(sizeof(Vector3) * current_buffer);
	if (this->verts == NULL) throw AllocationException("vertices", current_buffer);

	while (std::getline(f, line)) {
		line_count += 1;
		if (line.size() <= 2) continue;
		if (line[0] == '#') continue;
		// Material Library
		if (state == ObjReadState::OBJECT && line.rfind("mtllib ", 0) == 0) {
			line = line.substr(7, line.size() - 7);
			if (line.size() == 0) throw ParseException("Invalid material library");
			
			new_materials = Material::load((base_dir + line).c_str());
			for (int i = 0; i < new_materials.size(); i++) {
				this->materials[new_materials[i].name] = new_materials[i];
			}
			continue;
		}
		// Object
		if (state == ObjReadState::OBJECT && line[0] == 'o' && line[1] == ' ') {
			this->name = line.substr(2, line.length() - 2);
			state = ObjReadState::VERTICIES;
			continue;
		}
		// Verticies
		if (state == ObjReadState::VERTICIES) {
			if (line[0] == 'v' && line[1] == ' ') {
				line_s = string_split(line, ' ');
				if (line_s.size() != 4) throw ParseException("Invalid vertex");
				if (this->vert_count == current_buffer) {
					current_buffer *= 2;
					hold_v3 = (Vector3*)realloc(this->verts, sizeof(Vector3) * current_buffer);
					if (hold_v3 == NULL) throw ReallocException("verticies", current_buffer);
					this->verts = hold_v3;
				}
				this->verts[this->vert_count].x = std::stof(line_s[1]);
				this->verts[this->vert_count].y = std::stof(line_s[2]);
				this->verts[this->vert_count].z = std::stof(line_s[3]);
				this->vert_count += 1;
				continue;
			} else {
				state = ObjReadState::NORMALS;
				if (this->vert_count == 0) throw ParseException("No vertex data found");
				hold_v3 = (Vector3*)realloc(this->verts, sizeof(Vector3) * this->vert_count);
				if (hold_v3 == NULL) throw ReallocException("verticies", this->vert_count);
				this->verts = hold_v3;
				current_buffer = INITIAL_BUFFER;
				this->norms = (Vector3*)malloc(sizeof(Vector3) * current_buffer);
				if (this->norms == NULL) throw AllocationException("normals", current_buffer);
			}
		}
		// Normals
		if (state == ObjReadState::NORMALS) {
			if (line[0] == 'v' && line[1] == 'n') {
				line_s = string_split(line, ' ');
				if (line_s.size() != 4) throw ParseException("Invalid normal");
				if (this->norm_count == current_buffer) {
					current_buffer *= 2;
					hold_v3 = (Vector3*)realloc(this->norms, sizeof(Vector3) * current_buffer);
					if (hold_v3 == NULL) throw ReallocException("normals", current_buffer);
					this->norms = hold_v3;
				}
				this->norms[this->norm_count].x = std::stof(line_s[1]);
				this->norms[this->norm_count].y = std::stof(line_s[2]);
				this->norms[this->norm_count].z = std::stof(line_s[3]);
				this->norm_count += 1;
				continue;
			} else {
				state = ObjReadState::UVS;
				if (this->norm_count == 0) throw ParseException("No normal data found");
				hold_v3 = (Vector3*)realloc(this->norms, sizeof(Vector3) * this->norm_count);
				if (hold_v3 == NULL) throw ReallocException("normals", this->norm_count);
				this->norms = hold_v3;
				current_buffer = INITIAL_BUFFER;
				this->uvs = (Vector2*)malloc(sizeof(Vector2) * current_buffer);
				if (this->uvs == NULL) throw AllocationException("UVs", current_buffer);
			}
		}
		// UVs
		if (state == ObjReadState::UVS) {
			if (line[0] == 'v' && line[1] == 't') {
				line_s = string_split(line, ' ');
				if (line_s.size() != 3) throw ParseException("Invalid UV");
				if (this->uv_count == current_buffer) {
					current_buffer *= 2;
					hold_v2 = (Vector2*)realloc(this->uvs, sizeof(Vector2) * current_buffer);
					if (hold_v2 == NULL) throw ReallocException("UVs", current_buffer);
					this->uvs = hold_v2;
				}
				this->uvs[this->uv_count].x = std::stof(line_s[1]);
				this->uvs[this->uv_count].y = std::stof(line_s[2]);
				this->uv_count += 1;
				continue;
			} else {
				state = ObjReadState::SURFACES;
				if (this->uv_count > 0) {
					hold_v2 = (Vector2*)realloc(this->uvs, sizeof(Vector2) * this->uv_count);
					if (hold_v2 == NULL) throw ReallocException("UVs", this->uv_count);
					this->uvs = hold_v2;
				} else std::free(this->uvs);
				current_buffer = INITIAL_BUFFER;
				this->surfaces = (Surface*)malloc(sizeof(Surface) * current_buffer);
				if (this->surfaces == NULL) throw AllocationException("surfaces", current_buffer);
			}
		}
		// Surfaces
		if (state >= ObjReadState::SURFACES) {
			if (state == ObjReadState::SURFACES && line[0] == 's') {
				if (this->surface_count == current_buffer) {
					hold_s = (Surface*)realloc(this->surfaces, sizeof(Surface) * current_buffer);
					if (hold_s == NULL) throw ReallocException("surfaces", current_buffer);
					this->surfaces = hold_s;
				}
				cur_surface = &this->surfaces[this->surface_count];
				cur_surface->face_count = 0;
				cur_surface->faces = (Face*)malloc(sizeof(Face) * current_buffer);
				if (cur_surface->faces == NULL) throw AllocationException("surface faces", current_buffer);
				cur_surface->material_refs = new std::unordered_map<int, std::string>();
				this->surface_count += 1;
				face_count = 0;
				state = ObjReadState::FACES;
				continue;
			}
			// Material Reference
			if (state == ObjReadState::FACES && line.rfind("usemtl ", 0) == 0) {
				line = line.substr(7, line.size() - 7);
				if (line.size() == 0) throw ParseException("Invalid material reference");
				line = string_replace(line, " ", "_");
				if (this->materials.find(line) == this->materials.end()) {
					throw ParseException(
						"Invalid material reference (\""
						+ line + "\" not loaded)"
					);
				}
				(*cur_surface->material_refs)[face_count] = line;
				continue;
			}
			// Faces
			if (state == ObjReadState::FACES && line[0] == 'f') {
				line_s = string_split(line, ' ');
				if (line_s.size() != 4) throw ParseException("Invalid face (expected triangle data)");
				if (face_count == current_buffer) {
					current_buffer *= 2;
					hold_f = (Face*)realloc(cur_surface->faces, sizeof(Face) * current_buffer);
					if (hold_f == NULL) throw ReallocException("surface faces", current_buffer);
					cur_surface->faces = hold_f;
				}
				for (int i = 0; i < 3; i++) {
					line_s_s = string_split(line_s[i+1], '/');
					if (line_s_s.size() != 3) throw ParseException("Invalid face point (expected 2 slashes '/')");
					cur_surface->faces[face_count].vert_index[i] = std::stoi(line_s_s[0]);
					if (line_s_s[1].size() > 0) cur_surface->faces[face_count].uv_index[i] = std::stoi(line_s_s[1]);
					cur_surface->faces[face_count].norm_index[i] = std::stoi(line_s_s[2]);
				}
				face_count += 1;
			} else {
				state = ObjReadState::SURFACES;
				hold_f = (Face*)realloc(cur_surface->faces, sizeof(Face) * face_count);
				if (hold_f == NULL) throw ReallocException("surface faces", face_count);
				cur_surface->faces = hold_f;
				cur_surface->face_count = face_count;
				current_buffer = INITIAL_BUFFER;
			}
		}
	}
	} catch (std::invalid_argument& e) {
		this->free();
		throw LoadException(
			"Invalid argument at line (" + std::to_string(line_count)
			+ ") in file \"" + filename + "\""
		);
	} catch (std::out_of_range& e) {
		this->free();
		throw LoadException(
			"Invalid argument range at line (" + std::to_string(line_count)
			+ ") in file \"" + filename + "\""
		);
	} catch (AllocationException& e) {
		this->free();
		throw LoadException(
			std::string(e.what()) + " while loading file \"" + filename + "\""
		);
	} catch (ParseException& e) {
		this->free();
		throw LoadException(
			std::string(e.what()) + " at line (" + std::to_string(line_count)
			+ ") in file \"" + filename + "\""
		);
	}

	// Finish Face
	if (state == ObjReadState::FACES && cur_surface != NULL) {
		state = ObjReadState::SURFACES;
		hold_f = (Face*)realloc(cur_surface->faces, sizeof(Face) * face_count);
		if (hold_f == NULL) {
			this->free();
			throw LoadException(
				"Unable to reallocate memory (" + std::to_string(face_count)
				+ ") for final faces of final surface "
				+ std::to_string(this->surface_count - 1)
			 	+ " while loading file " + filename
			);
		}
		cur_surface->faces = hold_f;
		cur_surface->face_count = face_count;
	}

	// Finish Surface
	if (state == ObjReadState::SURFACES) {
		hold_s = (Surface*)realloc(this->surfaces, sizeof(Surface) * this->surface_count);
		if (hold_s == NULL) {
			this->free();
			throw LoadException(
				"Unable to reallocate memory (" + std::to_string(this->surface_count)
				+ ") for surfaces while loading file " + filename
			);
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
			delete this->surfaces[i].material_refs;
		}
		std::free(this->surfaces);
	}
	this->name.clear();
	this->materials.clear();
	this->vert_count = 0;
	this->norm_count = 0;
	this->uv_count = 0;
	this->surface_count = 0;
}