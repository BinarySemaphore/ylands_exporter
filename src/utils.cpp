#include "utils.hpp"

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <time.h>

#include "scene.hpp"
#include "objwavefront.hpp"

double timerStart() {
	return (double)clock();
}

double timerStopMs(double start) {
	double duration_ms = clock() - (clock_t)start;
	duration_ms *= 1000.0 / (double)CLOCKS_PER_SEC;
	return duration_ms;
}

void timerStopMsAndPrint(double start) {
	double duration_ms = timerStopMs(start);
	std::cout << "Time taken: " << duration_ms << " ms" << std::endl;
}

float roundTo(float value, int places) {
	int base = std::pow(10, places);
	float rf = std::round(value * base) / base;
	return std::abs(rf) < NEAR_ZERO ? 0.0f : rf;
}

std::string getEntityColorUid(MeshObj& entity) {
	std::string color_uid = "";
	Material* mat = entity.mesh.getSurfaceMaterials(0)[0];
	color_uid += Material::getColorHashString(mat->diffuse);
	color_uid += "_em" + Material::getColorHashString(mat->emissive);
	color_uid += "_d" + std::to_string(mat->dissolve);
	return color_uid;
}

std::string hexFromInt(int value) {
	if (value < 0 || value > 255) return "";
	std::stringstream result;
	result << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << value;
	return result.str();
}

std::string string_ascii(const std::string& str) {
	std::string ascii_str = "";
	for (char c : str) {
		if ((unsigned int)c < 128) {
			ascii_str += c;
		} else {
			ascii_str += '?';
		}
	}
	return ascii_str;
}

std::string string_join(const std::vector<std::string>& str_list, const char* delimiter) {
	std::ostringstream combined;
	if (str_list.size() == 1) return str_list[0];
	std::copy(
		str_list.begin(),
		str_list.end() - 1,
		std::ostream_iterator<std::string>(combined, delimiter)
	);
	combined << str_list[str_list.size() - 1];
	return combined.str();
}

std::vector<std::string> string_split(const std::string& str, char delimiter) {
	std::vector<std::string> tokens;
	std::stringstream str_iter(str);
	std::string token;
	while (std::getline(str_iter, token, delimiter)) {
		tokens.push_back(token);
	}
	return tokens;
}

std::string string_replace(const std::string& str, const char* target, const char* repl) {
	int start;
	int target_length = std::strlen(target);
	int repl_length = std::strlen(repl);
	std::string replaced = str;
	start = replaced.find(target);
	while (start != std::string::npos) {
		replaced = replaced.replace(start, target_length, repl);
		start = replaced.find(target, start + repl_length);
	}
	return replaced;
}

std::string f_base_filename_no_ext(const char* filename) {
	int start;
	int end;
	std::string base_filename;
	base_filename = filename;
	start = base_filename.rfind('/');
	if (start == std::string::npos) {
		start = base_filename.rfind('\\');
		if (start == std::string::npos) start = -1;
	}
	end = base_filename.rfind('.');
	if (end == std::string::npos) end = base_filename.size();
	return base_filename.substr(start + 1, end - start - 1);
}

std::string f_base_dir(const char* filename) {
	std::string base_dir = "";
	std::vector<std::string> dir_list;
	dir_list = string_split(filename, '/');
	if (dir_list.size() > 1) {
		dir_list = std::vector<std::string>(dir_list.begin(), dir_list.end() - 1);
		base_dir = string_join(dir_list, "/") + "/";
	} else {
		dir_list = string_split(filename, '\\');
		if (dir_list.size() > 1) {
			dir_list = std::vector<std::string>(dir_list.begin(), dir_list.end() - 1);
			base_dir = string_join(dir_list, "\\") + "\\";
		}
	}
	return base_dir;
}

CustomException::CustomException() {}

const char* CustomException::what() const noexcept {
	return this->msg.c_str();
}

GeneralException::GeneralException(std::string msg) {
	this->msg = msg;
}

SaveException::SaveException(std::string msg) {
	this->msg = msg;
}

LoadException::LoadException(std::string msg) {
	this->msg = msg;
}

ParseException::ParseException(std::string msg) {
	this->msg = msg;
}

AllocationException::AllocationException() {}
AllocationException::AllocationException(const char* who, int size) {
	this->msg = "Unable to allocate enough initial memory ("
	+ std::to_string(size) + ") for " + who;
}

ReallocException::ReallocException(const char* who, int size) {
	this->msg = "Unable to reallocate enough memory ("
	+ std::to_string(size) + ") for " + who;
}

template <typename T>
void getBounds(T* values, size_t count, T& min, T&max) {
	if (count == 0) return;
	max = min = values[0];
	if (std::is_same<T, Vector2>::value) {
		Vector2* vvalues = (Vector2*)values;
		Vector2* vmin = (Vector2*)&min;
		Vector2* vmax = (Vector2*)&max;
		for (int i = 1; i < count; i++) {
			if (vvalues[i].x < vmin->x) vmin->x = vvalues[i].x;
			if (vvalues[i].y < vmin->y) vmin->y = vvalues[i].y;
			if (vvalues[i].x > vmax->x) vmax->x = vvalues[i].x;
			if (vvalues[i].y > vmax->y) vmax->y = vvalues[i].y;
		}
		return;
	} else if (std::is_same<T, Vector3>::value) {
		Vector3* vvalues = (Vector3*)values;
		Vector3* vmin = (Vector3*)&min;
		Vector3* vmax = (Vector3*)&max;
		for (int i = 1; i < count; i++) {
			if (vvalues[i].x < vmin->x) vmin->x = vvalues[i].x;
			if (vvalues[i].y < vmin->y) vmin->y = vvalues[i].y;
			if (vvalues[i].z < vmin->z) vmin->z = vvalues[i].z;
			if (vvalues[i].x > vmax->x) vmax->x = vvalues[i].x;
			if (vvalues[i].y > vmax->y) vmax->y = vvalues[i].y;
			if (vvalues[i].z > vmax->z) vmax->z = vvalues[i].z;
		}
		return;
	} else if (std::is_same<T, Quaternion>::value) {
		Quaternion* vvalues = (Quaternion*)values;
		Quaternion* vmin = (Quaternion*)&min;
		Quaternion* vmax = (Quaternion*)&max;
		for (int i = 1; i < count; i++) {
			if (vvalues[i].x < vmin->x) vmin->x = vvalues[i].x;
			if (vvalues[i].y < vmin->y) vmin->y = vvalues[i].y;
			if (vvalues[i].z < vmin->z) vmin->z = vvalues[i].z;
			if (vvalues[i].w < vmin->w) vmin->w = vvalues[i].w;
			if (vvalues[i].x > vmax->x) vmax->x = vvalues[i].x;
			if (vvalues[i].y > vmax->y) vmax->y = vvalues[i].y;
			if (vvalues[i].z > vmax->z) vmax->z = vvalues[i].z;
			if (vvalues[i].w > vmax->w) vmax->w = vvalues[i].w;
		}
		return;
	} else {
		int* vvalues = (int*)values;
		int* vmin = (int*)&min;
		int* vmax = (int*)&max;
		for (int i = 1; i < count; i++) {
			if (vvalues[i] < *vmin) *vmin = vvalues[i];
			if (vvalues[i] > *vmax) *vmax = vvalues[i];
		}
	}
}
template void getBounds<int>(int* values, size_t count, int& min, int&max);
template void getBounds<float>(float* values, size_t count, float& min, float&max);
template void getBounds<Vector2>(Vector2* values, size_t count, Vector2& min, Vector2&max);
template void getBounds<Vector3>(Vector3* values, size_t count, Vector3& min, Vector3&max);
template void getBounds<Quaternion>(Quaternion* values, size_t count, Quaternion& min, Quaternion&max);
