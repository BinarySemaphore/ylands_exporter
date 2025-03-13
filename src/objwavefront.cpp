#include "objwavefront.hpp"

#include <fstream>
#include <sstream>
#include <iomanip>

#include "utils.hpp"

const int INITIAL_BUFFER = 64;
const char* DEFAULT_NAME = "Unnamed";
const char* HEADER_LINE = "# Yland Extractor v2\n"
						  "# https://github.com/BinarySemaphore/ylands_exporter";

std::unordered_map<std::string, ObjWavefront> CACHE_OBJWF_LOAD;

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

Material::Material(const char* name) {
	Material();
	this->name = name;
}

std::string Material::getColorHashString(Vector3 color) {
	Vector3 expanded = 255 * color;
	std::string result = hexFromInt(expanded.x)
					   + hexFromInt(expanded.y)
					   + hexFromInt(expanded.z);
	return result;
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
	} catch (std::invalid_argument) {
		f.close();
		throw LoadException(
			"Invalid argument at line (" + std::to_string(line_count)
			+ ") in file \"" + std::string(filename) + "\""
		);
	} catch (std::out_of_range) {
		f.close();
		throw LoadException(
			"Invalid argument range at line (" + std::to_string(line_count)
			+ ") in file \"" + std::string(filename) + "\""
		);
	} catch (std::bad_alloc) {
		f.close();
		throw LoadException(
			"Failed to allocate enough memory for materials while "
			"loading file \"" + std::string(filename) + "\""
		);
	} catch (std::length_error) {
		f.close();
		throw LoadException(
			"Failed to allocate enough initial memory for materials while "
			"loading file \"" + std::string(filename) + "\""
		);
	} catch (ParseException& e) {
		f.close();
		throw LoadException(
			std::string(e.what()) + " at line (" + std::to_string(line_count)
			+ ") in file \"" + std::string(filename) + "\""
		);
	}

	f.close();

	return materials;
}

void Material::save(const char* filename, const std::vector<const Material*>& materials) {
	std::ofstream f(filename);
	if (!f.is_open()) {
		throw SaveException(
			"Cannot open file for writing \"" + std::string(filename) + "\""
		);
	}

	f << std::fixed << std::setprecision(6);
	f << HEADER_LINE;

	for (int i=0; i < materials.size(); i++) {
		f << "\n\nnewmtl " << materials[i]->name;
		f << "\nKa "
		 << materials[i]->ambient.x << " "
		 << materials[i]->ambient.y << " "
		 << materials[i]->ambient.z;
		f << "\nKd "
		 << materials[i]->diffuse.x << " "
		 << materials[i]->diffuse.y << " "
		 << materials[i]->diffuse.z;
		f << "\nKs "
		 << materials[i]->specular.x << " "
		 << materials[i]->specular.y << " "
		 << materials[i]->specular.z;
		f << "\nKe "
		 << materials[i]->emissive.x << " "
		 << materials[i]->emissive.y << " "
		 << materials[i]->emissive.z;
		f << "\nNs " << materials[i]->spec_exp;
		f << "\nNi " << materials[i]->optical_density;
		f << "\nd " << materials[i]->dissolve;
		f << "\nillum " << (int)materials[i]->illum_model;
	}

	f.close();
}

bool Material::operator==(const Material& mat) const {
	return this->ambient == mat.ambient &&
		   this->diffuse == mat.diffuse &&
		   this->dissolve == mat.dissolve &&
		   this->emissive == mat.emissive &&
		   this->illum_model == mat.illum_model &&
		   this->optical_density == mat.optical_density &&
		   this->spec_exp == mat.spec_exp &&
		   this->specular == mat.specular;
}

bool Material::operator!=(const Material& mat) const {
	return !(*this == mat);
}

ObjWavefront::ObjWavefront() {
	this->name = DEFAULT_NAME;
	this->vert_count = 0;
	this->norm_count = 0;
	this->uv_count = 0;
	this->surface_count = 0;
	this->verts = NULL;
	this->norms = NULL;
	this->uvs = NULL;
	this->surfaces = NULL;
}

ObjWavefront::~ObjWavefront() {
	this->clear();
}

void ObjWavefront::load(const char* filename, bool cache) {
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
	std::ifstream f;

	try {
	if (cache && CACHE_OBJWF_LOAD.find(filename) != CACHE_OBJWF_LOAD.end()) {
		try {
			*this = CACHE_OBJWF_LOAD[filename];
		} catch (AllocationException& e) {
			throw LoadException(
				"Cache load error \"" + std::string(filename) + "\": " + e.what()
			);
		}
		return;
	}

	base_dir = f_base_dir(filename);
	f = std::ifstream(filename);
	if (!f.is_open()) {
		throw LoadException(
			"Cannot open Obj Wavefront file \"" + std::string(filename) + "\""
		);
	}

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
	} catch (std::invalid_argument) {
		f.close();
		this->clear();
		throw LoadException(
			"Invalid argument at line (" + std::to_string(line_count)
			+ ") in file \"" + filename + "\""
		);
	} catch (std::out_of_range) {
		f.close();
		this->clear();
		throw LoadException(
			"Invalid argument range at line (" + std::to_string(line_count)
			+ ") in file \"" + filename + "\""
		);
	} catch (AllocationException& e) {
		f.close();
		this->clear();
		throw LoadException(
			std::string(e.what()) + " while loading file \"" + filename + "\""
		);
	} catch (ParseException& e) {
		f.close();
		this->clear();
		throw LoadException(
			std::string(e.what()) + " at line (" + std::to_string(line_count)
			+ ") in file \"" + filename + "\""
		);
	} catch (LoadException& e) {
		this->clear();
		throw e;
	}

	f.close();

	// Finish Face
	if (state == ObjReadState::FACES && cur_surface != NULL) {
		state = ObjReadState::SURFACES;
		hold_f = (Face*)realloc(cur_surface->faces, sizeof(Face) * face_count);
		if (hold_f == NULL) {
			this->clear();
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
			this->clear();
			throw LoadException(
				"Unable to reallocate memory (" + std::to_string(this->surface_count)
				+ ") for surfaces while loading file " + filename
			);
		}
		this->surfaces = hold_s;
	}

	if (cache) {
		if (CACHE_OBJWF_LOAD.find(filename) == CACHE_OBJWF_LOAD.end()) {
			try {
				CACHE_OBJWF_LOAD[filename] = *this;
			} catch (AllocationException& e) {
				this->clear();
				throw LoadException("Failed to copy into cache: " + std::string(e.what()));
			}
		}
	}
}

void ObjWavefront::save(const char* filename) const {
	int i;
	std::string base_dir;
	std::string mat_filename;
	std::vector<std::string>::iterator check;
	std::vector<std::string> unique_mats;
	std::vector<std::string> orphan_mats;
	std::vector<const Material*> materials_flat;

	// Save Material Library
	for (i = 0; i < this->surface_count; i++) {
		for (std::pair<int, std::string> kv : (*this->surfaces[i].material_refs)) {
			check = std::find(unique_mats.begin(), unique_mats.end(), kv.second);
			if (check != unique_mats.end()) continue;
			unique_mats.push_back(kv.second);
		}
	}
	if (unique_mats.size() > 0) {
		base_dir = f_base_dir(filename);
		mat_filename = f_base_filename_no_ext(filename) + ".mtl";
		for (i = 0; i < unique_mats.size(); i++) {
			if (this->materials.find(unique_mats[i]) != this->materials.end()) {
				materials_flat.push_back(&this->materials.at(unique_mats[i]));
			} else {
				orphan_mats.push_back(unique_mats[i]);
			}
		}
		if (materials_flat.size() > 0) {
			Material::save((base_dir + mat_filename).c_str(), materials_flat);
		}
		unique_mats.clear();
		materials_flat.clear();
	}

	std::ofstream f(filename);
	if (!f.is_open()) {
		throw SaveException(std::strcat("Cannot open file for writing ", filename));
	}

	f << std::fixed << std::setprecision(6);
	f << HEADER_LINE;

	// Material Library
	if (mat_filename.size() > 0) {
		f << "\nmtllib " << mat_filename;
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
			// If face index is in material reference and material reference is not in ophan materials
			if (this->surfaces[i].material_refs->find(j)
				!= this->surfaces[i].material_refs->end() &&
				std::find(
					orphan_mats.begin(),
					orphan_mats.end(),
					(*this->surfaces[i].material_refs)[j]
				)
				== orphan_mats.end()
			) {
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

std::vector<Material*> ObjWavefront::getSurfaceMaterials(int surface_index) {
	std::vector<Material*> mats;
	std::vector<std::string> unique_mats;

	if (surface_index < 0 || surface_index >= this->surface_count) return mats;

	for (std::pair<int, std::string> kv : (*this->surfaces[surface_index].material_refs)) {
		if (std::find(
				unique_mats.begin(),
				unique_mats.end(),
				kv.second
			)
			!= unique_mats.end()
		) continue;
		unique_mats.push_back(kv.second);
	}

	for (int i = 0; i < unique_mats.size(); i++) {
		if (this->materials.find(unique_mats[i]) == this->materials.end()) continue;
		mats.push_back(&this->materials.at(unique_mats[i]));
	}

	return mats;
}

void ObjWavefront::setSurfaceMaterial(int surface_index, Material& material) {
	if (surface_index < 0 || surface_index >= this->surface_count) return;

	if (this->materials.find(material.name) == this->materials.end()) {
		this->materials[material.name] = material;
	} else if (material != this->materials[material.name]) {
		int name_off = 1;
		std::string mat_new_name = material.name;
		while (this->materials.find(material.name) != this->materials.end()) {
			material.name = mat_new_name + "." + std::to_string(name_off);
			name_off += 1;
			if (name_off == 1000) {
				throw GeneralException("Rename your materials, damn!");
			}
		}
	}

	this->surfaces[surface_index].material_refs->clear();
	(*this->surfaces[surface_index].material_refs)[0] = material.name;
}

void ObjWavefront::setMaterial(Material& material) {
	this->clearMaterials();
	for (int i = 0; i < this->surface_count; i++) {
		this->setSurfaceMaterial(i, material);
	}
}

void ObjWavefront::clearMaterials() {
	this->materials.clear();
	for (int i = 0; i < this->surface_count; i++) {
		this->surfaces[i].material_refs->clear();
	}
}

void ObjWavefront::clear() {
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
	this->materials.clear();
	this->name.clear();
	this->vert_count = 0;
	this->norm_count = 0;
	this->uv_count = 0;
	this->surface_count = 0;
}

void ObjWavefront::operator=(const ObjWavefront& obj) {
	int i, j;

	this->name = obj.name;
	this->vert_count = obj.vert_count;
	this->norm_count = obj.norm_count;
	this->uv_count = obj.uv_count;
	this->surface_count = obj.surface_count;
	this->materials = obj.materials;

	if (this->vert_count > 0) {
		this->verts = (Vector3*)malloc(sizeof(Vector3) * this->vert_count);
		if (this->verts == NULL) {
			this->clear();
			throw AllocationException("vertices", this->vert_count);
		}
	}
	if (this->norm_count > 0) {
		this->norms = (Vector3*)malloc(sizeof(Vector3) * this->norm_count);
		if (this->norms == NULL) {
			this->clear();
			throw AllocationException("normals", this->norm_count);
		}
	}
	if (this->uv_count > 0) {
		this->uvs = (Vector2*)malloc(sizeof(Vector2) * this->uv_count);
		if (this->uvs == NULL) {
			this->clear();
			throw AllocationException("UVs", this->uv_count);
		}
	}
	if (this->surface_count > 0) {
		this->surfaces = (Surface*)malloc(sizeof(Surface) * this->surface_count);
		if (this->surfaces == NULL) {
			this->clear();
			throw AllocationException("normals", this->surface_count);
		}
	}

	for (i = 0; i < this->vert_count; i++) {
		this->verts[i] = obj.verts[i];
	}
	for (i = 0; i < this->norm_count; i++) {
		this->norms[i] = obj.norms[i];
	}
	for (i = 0; i < this->uv_count; i++) {
		this->uvs[i] = obj.uvs[i];
	}
	for (i = 0; i < this->surface_count; i++) {
		this->surfaces[i].material_refs = new std::unordered_map<int, std::string>();
		this->surfaces[i].face_count = obj.surfaces[i].face_count;
		this->surfaces[i].faces = NULL;
		if (this->surfaces[i].face_count > 0) {
			this->surfaces[i].faces = (Face*)malloc(sizeof(Face) * this->surfaces[i].face_count);
			if (this->surfaces[i].faces == NULL) {
				this->clear();
				throw AllocationException("surface faces", this->surfaces[i].face_count);
			}
			for (j = 0; j < this->surfaces[i].face_count; j++) {
				this->surfaces[i].faces[j] = obj.surfaces[i].faces[j];
			}
		}
	}
}