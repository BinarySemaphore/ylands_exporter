#include <iostream>
#include "json.hpp"
#include "workpool.hpp"
#include "objwavefront.hpp"

using json = nlohmann::json;

int main(int argc, char** argv) {
	try {
		ObjWavefront test("models/musket_ball.obj");
		test.save("copy_obj.obj");
		test.free();
	} catch (std::exception& e) {
		std::cout << "Exception: " << e.what() << std::endl;
	}
	return 0;
}