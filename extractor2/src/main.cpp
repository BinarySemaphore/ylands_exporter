#include <iostream>

#include "ylands.hpp"
#include "space.hpp"

int main(int argc, char** argv) {
	// try {
	// 	ObjWavefront test("models/musket_ball.obj");
	// 	test.save("copy_obj.obj");
	// 	test.free();
	// } catch (std::exception& e) {
	// 	std::cerr << "Exception: " << e.what() << std::endl;
	// }
	YlandStandard::preload_lookups("./lookup.json");
	Quaternion rot(TURN_ONEQ, Vector3(0.0f, 1.0f, 0.0f));
	Quaternion ident = Quaternion();
	rot = rot * ident;  // rot should be same after
	Quaternion rot2(-TURN_ONEQ, Vector3(0.0f, 1.0f, 0.0f));
	Vector3 test(2.0f, 0.0f, 0.0f);
	test = rot * rot2 * test; // test should be same after
	test = rot * test;
	test = rot2 * test;
	return 0;
}

/*
Options: (TODO: make interactive option selection)
 -i <JSON-file> : Read from existing JSON export from Ylands.
				  If not given, will attempt to extract from
				  Ylands directly (setup in config.json).
	  -o <name> : Output name (extensionless - file extensions are auto applied)
	  -t <TYPE> : Output type (default: OBJ <- for now - will be GLB).
				  Supported TYPEs : what they produce as output.
					 GLB : <name>.glb
					GLTF : <name>.gltf and <name>.bin
					 OBJ : <name>.obj and <name>.mtl
			 -m : Merge into single geometry.
				  Same as using '-rja'.
			 -r : Remove internal faces within same material (unless using -a)
				  Any faces adjacent and opposite another face is removed.
				  This includes their opposing neighbor.
			 -j : Join verticies within same material (unless using -a)
				  Any vertices sharing a location with another,
				  or within a very small distance, will be
				  reduced to a single vertex.
				  This efectively "hardens" or "joins" Yland blocks
				  into a single geometry.
			 -a : Apply to all.
				  For any Join (-j) or Internal Face Removal (-r).
				  Applies to all regardless of material grouping.
*/