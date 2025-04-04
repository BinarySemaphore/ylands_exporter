#include "exporter.hpp"

#include <iostream>
#include <fstream>

#include "utils.hpp"
#include "config.hpp"
#include "extractor.hpp"
#include "scene.hpp"
#include "combomesh.hpp"
#include "reducer.hpp"
#include "octree.hpp"
#include "objwavefront.hpp"
#include "gltf.hpp"
#include "workpool.hpp"

// IMPORTANT: When in debug, make sure no_threads is true
Workpool* wp = NULL;//new Workpool(std::thread::hardware_concurrency() * 50, false, false);

void combineMeshFromScene(const Config& config, Node* scene);
void vertexJoinMeshInScene(TNode<Octree<FaceData>>* octscene);
TNode<Octree<FaceData>>* createOctreeOfScene(const Config& config, Node* scene);
void debugOctscene(const char* filename, const TNode<Octree<FaceData>>* octscene);

int extractAndExport(Config& config) {
	Node* scene = nullptr;
	TNode<Octree<FaceData>>* octscene = nullptr;
	json data;

	// Load Ylands data
	if (!config.has_input) {
		// From Ylands directly
		try {
			extractFromYlands(config, data);
		} catch (CustomException& e) {
			std::cerr << "Error while extrating from Ylands: " << e.what()
					  << std::endl;
			return 1;
		}
		if (isDataBlockDef(data) && config.export_type != ExportType::JSON) {
			std::cout << "Extracted " << DATA_TYPE_BLOCKDEF
					  << " from Ylands, forcing JSON export." << std::endl;
			config.export_type = ExportType::JSON;
		}
	} else {
		// From input arg (Ylands JSON)
		try {
			loadFromFile(config.input_filename.c_str(), data);
		} catch (CustomException& e) {
			std::cerr << "Error while loading from JSON file \""
					  << config.input_filename << "\": " << e.what()
					  << std::endl;
			return 1;
		}
		if (config.export_type == ExportType::JSON) {
			std::cout << "Nothing to do:"
					  << " Loading from JSON file and exporting to JSON."
					  << std::endl;
			return 0;
		}
		if (isDataBlockDef(data)) {
			std::cerr << "Error: Given Ylands BLOCKDEF data." << std::endl
					  << "BLOCKDEF is for referencing only, not processing."
					  << std::endl
					  << "To set BLOCKDEF update \"lookup.json\" file."
					  << std::endl;
			return 2;
		}
	}

	// JSON export
	if (config.export_type == ExportType::JSON) {
		try {
			exportAsJson(config.output_filename.c_str(), data, config.ext_pprint);
		} catch(CustomException& e) {
			std::cerr << "Error exporting JSON file \""
					  << config.output_filename << "\": "
					  << e.what() << std::endl;
			return 3;
		}
		return 0;
	}

	// Validate and Setup to continue
	if (!isDataValidScene(data)) {
		std::cerr << "Invalid JSON data: expected Ylands SCENE data."
				  << std::endl;
		return 2;
	}
	//wp->start();

	// Convert data into 3D Scene
	try {
		scene = createSceneFromJson(config, data);
	} catch (CustomException& e) {
		std::cerr << "Error creating scene: " << e.what() << std::endl;
		return 4;
	}

	// Process scene using config flags
	if (config.combine) {
		try {
			combineMeshFromScene(config, scene);
		} catch (CustomException& e) {
			std::cerr << "Error combining scene: " << e.what() << std::endl;
		}
	}
	if (config.join_verts || config.remove_faces) {
		octscene = createOctreeOfScene(config, scene);
		//debugOctscene("octree_debug.obj", octscene);
	}
	if (config.join_verts && octscene != nullptr) {
		try {
			vertexJoinMeshInScene(octscene);
		} catch (CustomException& e) {
			std::cerr << "Error joining vertices: " << e.what() << std::endl;
		}
	}

	// OBJ export
	if (config.export_type == ExportType::OBJ) {
		try {
			exportAsObj(config.output_filename.c_str(), *scene);
		} catch (CustomException& e) {
			std::cerr << "Error exporting OBJ file \""
					  << config.output_filename << "\": "
					  << e.what() << std::endl;
			return 3;
		}
	}

	// GLTF export
	if (config.export_type == ExportType::GLTF) {
		try {
			exportAsGLTF(config.output_filename.c_str(), *scene, false);
		} catch (CustomException& e) {
			std::cerr << "Error exporting GLTF file \""
					  << config.output_filename << "\": "
					  << e.what() << std::endl;
			return 3;
		}
	}
	
	// GLB export
	if (config.export_type == ExportType::GLB) {
		try {
			exportAsGLTF(config.output_filename.c_str(), *scene, true);
		} catch (CustomException& e) {
			std::cerr << "Error exporting GLB file \""
					  << config.output_filename << "\": "
					  << e.what() << std::endl;
			return 3;
		}
	}

	// Finish
	return 0;
}

void exportAsJson(const char* filename, const json& data, bool pprint) {
	double s;
	char filename_ext[200] = "";
	
	std::strcat(filename_ext, filename);
	std::strcat(filename_ext, ".json");

	s = timerStart();
	std::cout << "Exporting [JSON] file \"" << filename_ext << "\"..." << std::endl;
	std::ofstream f(filename_ext);
	if (!f.is_open()) {
		throw SaveException("Failed to open file");
	}
	if (pprint) {
		f << data.dump(4);
	} else {
		f << data.dump();
	}
	f.close();
	std::cout << "Export complete" << std::endl;
	timerStopMsAndPrint(s);
	std::cout << std::endl;
}

void exportAsObj(const char* filename, Node& scene) {
	double s;
	char filename_ext[200] = "";

	std::strcat(filename_ext, filename);
	std::strcat(filename_ext, ".obj");

	if (scene.children.size() > 1 || scene.children[0]->type != NodeType::MeshObj) {
		throw GeneralException("OBJ export expects fully combined scene");
	}

	s = timerStart();
	std::cout << "Exporting [OBJ] file \"" << filename_ext << "\"..." << std::endl;
	((MeshObj*)scene.children[0])->mesh.save(filename_ext);
	std::cout << "Export complete" << std::endl;
	timerStopMsAndPrint(s);
	std::cout << std::endl;
}

void exportAsGLTF(const char* filename, Node& scene, bool single_glb) {
	double s;
	GLTF* gltf;
	char filename_ext[200] = "";

	std::strcat(filename_ext, filename);
	if (single_glb) {
		std::strcat(filename_ext, ".glb");
	} else {
		std::strcat(filename_ext, ".gltf");
	}

	s = timerStart();
	std::cout << "Exporting [";
	if (single_glb) {
		std::cout << "GLB";
	} else {
		std::cout << "GLTF";
	}
	std::cout << "] file \"" << filename_ext << "\"..." << std::endl;
	gltf = createGLTFFromScene(scene);
	gltf->save(filename_ext, single_glb);
	std::cout << "Export complete" << std::endl;
	timerStopMsAndPrint(s);
}

void combineMeshFromScene(const Config& config, Node* scene) {
	double s = timerStart();
	std::cout << "Applying config [COMBINE]..." << std::endl;
	if (config.export_type == ExportType::OBJ) {
		comboEntireScene(*scene);
	} else {
		comboSceneMeshes(*scene);
	}
	std::cout << "Applyied" << std::endl;
	timerStopMsAndPrint(s);
	std::cout << std::endl;
}

void vertexJoinMeshInScene(TNode<Octree<FaceData>>* octscene) {
	int count;
	double s = timerStart();
	std::cout << "Applying config [Join Vertices]..." << std::endl;
	count = joinSceneRelatedVerts(*octscene, 0.001f);
	std::cout << "Applied (removed " << count << " vertices)" << std::endl;
	timerStopMsAndPrint(s);
	std::cout << std::endl;
}

TNode<Octree<FaceData>>* createOctreeOfScene(const Config& config, Node* scene) {
	int i, j, k;
	float one_over_three = 1.0f / 3.0f;
	MeshObj* mnode;
	Surface* surface;
	Face* face;
	FaceData* face_data;
	Vector3 min, max, center;
	Vector3 points[3];
	std::vector<OctreeItem<FaceData>*> items;
	TNode<Octree<FaceData>>* child;
	TNode<Octree<FaceData>>* onode;

	onode = new TNode<Octree<FaceData>>();
	for (i = 0; i < scene->children.size(); i++) {
		if (scene->children[i]->type == NodeType::MeshObj) {
			mnode = (MeshObj*)scene->children[i];
			if (!config.apply_all) {
				for (j = 0; j < mnode->mesh.surface_count; j++) {
					surface = &mnode->mesh.surfaces[j];
					for (k = 0; k < surface->face_count; k++) {
						face = &surface->faces[k];
						face_data = new FaceData();
						face_data->mesh_ref = &mnode->mesh;
						face_data->face_ref = face;
						face_data->points[0] = &mnode->mesh.verts[face->vert_index[0] - 1];
						face_data->points[1] = &mnode->mesh.verts[face->vert_index[1] - 1];
						face_data->points[2] = &mnode->mesh.verts[face->vert_index[2] - 1];

						points[0] = *face_data->points[0];
						points[1] = *face_data->points[1];
						points[2] = *face_data->points[2];
						getBounds<Vector3>(points, 3, min, max);
						center = (points[0] + points[1] + points[2]) * one_over_three;

						items.push_back(new OctreeItem<FaceData>(center, max - min));
						items.back()->data = face_data;
					}
					child = new TNode<Octree<FaceData>>();
					child->data = new Octree<FaceData>(items.data(), items.size());
					child->data->subdivide(Vector3(0.01f, 0.01f, 0.01f), 20, 0);
					onode->children.push_back(child);
					items.clear();
				}
			} else {
				throw GeneralException("Not Implemented: createOctreeOfScene with config.apply_all");
			}
		} else {
			child = createOctreeOfScene(config, scene->children[i]);
			onode->children.push_back(child);
		}
	}

	return onode;
}

void debugOctSceneBuildMesh(const TNode<Octree<FaceData>>* octscene, ObjWavefront* mesh);
void debugAddOctreeToMesh(const Octree<FaceData>* octree, ObjWavefront* mesh);

void debugOctscene(const char* filename, const TNode<Octree<FaceData>>* octscene) {
	ObjWavefront* mesh = new ObjWavefront();
	Material* mat = new Material("mat_01");
	mat->diffuse = Vector3(0.25f, 0.9f, 0.9f);
	mat->dissolve = 0.01f;
	mesh->materials[mat->name] = *mat;
	mesh->vert_count = 0;
	mesh->norm_count = 6;
	mesh->norms = (Vector3*)malloc(sizeof(Vector3) * 6);
	mesh->norms[0] = Vector3(0.0f, 1.0f, 0.0f);
	mesh->norms[1] = Vector3(0.0f, -1.0f, 0.0f);
	mesh->norms[2] = Vector3(1.0f, 0.0f, 0.0f);
	mesh->norms[3] = Vector3(-1.0f, 0.0f, 0.0f);
	mesh->norms[4] = Vector3(0.0f, 0.0f, 1.0f);
	mesh->norms[5] = Vector3(0.0f, 0.0f, -1.0f);
	debugOctSceneBuildMesh(octscene, mesh);
	mesh->save(filename);
}

void debugOctSceneBuildMesh(const TNode<Octree<FaceData>>* octscene, ObjWavefront* mesh) {
	if (octscene->data != nullptr) {
		debugAddOctreeToMesh(octscene->data, mesh);
	}
	for (TNode<Octree<FaceData>>* child : octscene->children) {
		debugOctSceneBuildMesh(child, mesh);
	}
}

const float DEBUG_DIR[2] = {-1.0f, 1.0f};
void debugAddOctreeToMesh(const Octree<FaceData>* octree, ObjWavefront* mesh) {
	int i, j, k;
	Vector3 offset;
	Vector3 half_dims = octree->space.dims * 0.5f;
	std::vector<Vector3> nv;
	nv.reserve(8);
	/*
	1 : <0, 0, 0>
	2 : <0, 0, 1>
	3 : <0, 1, 0>
	4 : <0, 1, 1>
	5 : <1, 0, 0>
	6 : <1, 0, 1>
	7 : <1, 1, 0>
	8 : <1, 1, 1>
	*/
	for (i = 0; i < 2; i++) {
		for (j = 0; j < 2; j++) {
			for (k = 0; k < 2; k++) {
				offset.x = half_dims.x * DEBUG_DIR[i];
				offset.y = half_dims.y * DEBUG_DIR[j];
				offset.z = half_dims.z * DEBUG_DIR[k];
				nv.push_back(octree->space.center + offset);
			}
		}
	}
	mesh->verts = (Vector3*)realloc(mesh->verts, sizeof(Vector3) * (mesh->vert_count + 8));
	for (i = 0; i < 8; i++) {
		mesh->verts[i + mesh->vert_count] = nv[i];
	}
	mesh->surfaces = (Surface*)realloc(mesh->surfaces, sizeof(Surface) * (mesh->surface_count + 1));
	mesh->surfaces[mesh->surface_count].material_refs = new std::unordered_map<int, std::string>();
	(*mesh->surfaces[mesh->surface_count].material_refs)[0] = std::string("mat_01");
	mesh->surfaces[mesh->surface_count].faces = (Face*)malloc(sizeof(Face) * 12);
	mesh->surfaces[mesh->surface_count].face_count = 12;
	// Bottom faces
	for (i = 0; i < 2; i++) {
		for (j = 0; j < 3; j++) {
			mesh->surfaces[mesh->surface_count].faces[i].norm_index[j] = 2;
		}
	}
	mesh->surfaces[mesh->surface_count].faces[0].vert_index[0] = 1 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[0].vert_index[1] = 2 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[0].vert_index[2] = 5 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[1].vert_index[0] = 6 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[1].vert_index[1] = 5 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[1].vert_index[2] = 2 + mesh->vert_count;
	// Top faces
	for (i = 2; i < 4; i++) {
		for (j = 0; j < 3; j++) {
			mesh->surfaces[mesh->surface_count].faces[i].norm_index[j] = 1;
		}
	}
	mesh->surfaces[mesh->surface_count].faces[2].vert_index[0] = 3 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[2].vert_index[1] = 4 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[2].vert_index[2] = 7 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[3].vert_index[0] = 8 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[3].vert_index[1] = 7 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[3].vert_index[2] = 4 + mesh->vert_count;
	// Right faces
	for (i = 4; i < 6; i++) {
		for (j = 0; j < 3; j++) {
			mesh->surfaces[mesh->surface_count].faces[i].norm_index[j] = 3;
		}
	}
	mesh->surfaces[mesh->surface_count].faces[4].vert_index[0] = 5 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[4].vert_index[1] = 6 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[4].vert_index[2] = 8 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[5].vert_index[0] = 8 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[5].vert_index[1] = 7 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[5].vert_index[2] = 5 + mesh->vert_count;
	// Left faces
	for (i = 6; i < 8; i++) {
		for (j = 0; j < 3; j++) {
			mesh->surfaces[mesh->surface_count].faces[i].norm_index[j] = 4;
		}
	}
	mesh->surfaces[mesh->surface_count].faces[6].vert_index[0] = 1 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[6].vert_index[1] = 2 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[6].vert_index[2] = 4 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[7].vert_index[0] = 4 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[7].vert_index[1] = 3 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[7].vert_index[2] = 1 + mesh->vert_count;
	// Front faces
	for (i = 8; i < 10; i++) {
		for (j = 0; j < 3; j++) {
			mesh->surfaces[mesh->surface_count].faces[i].norm_index[j] = 5;
		}
	}
	mesh->surfaces[mesh->surface_count].faces[8].vert_index[0] = 2 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[8].vert_index[1] = 6 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[8].vert_index[2] = 8 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[9].vert_index[0] = 8 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[9].vert_index[1] = 4 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[9].vert_index[2] = 2 + mesh->vert_count;
	// Back faces
	for (i = 10; i < 12; i++) {
		for (j = 0; j < 3; j++) {
			mesh->surfaces[mesh->surface_count].faces[i].norm_index[j] = 6;
		}
	}
	mesh->surfaces[mesh->surface_count].faces[10].vert_index[0] = 1 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[10].vert_index[1] = 5 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[10].vert_index[2] = 7 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[11].vert_index[0] = 7 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[11].vert_index[1] = 3 + mesh->vert_count;
	mesh->surfaces[mesh->surface_count].faces[11].vert_index[2] = 1 + mesh->vert_count;
	mesh->surface_count += 1;
	mesh->vert_count += 8;
	for (const Octree<FaceData>& div : octree->subdivisions) {
		debugAddOctreeToMesh(&div, mesh);
	}
}