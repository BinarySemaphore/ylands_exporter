#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

class MeshObj;

const float NEAR_ZERO = 0.00001f;

double timerStart();
double timerStopMs(double start);
void timerStopMsAndPrint(double start);

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

std::string getEntityColorUid(MeshObj& entity);

std::string hexFromInt(int value);

std::string string_ascii(const std::string& str);
std::string string_join(const std::vector<std::string>& str_list, const char* delimiter);
std::vector<std::string> string_split(const std::string& str, char delimiter);
std::string string_replace(const std::string& str, const char* target, const char* repl);

std::string f_base_filename_no_ext(const char* filename);
std::string f_base_dir(const char* filename);

class CustomException : public std::exception {
protected:
	std::string msg;
public:
	CustomException();
	const char* what() const noexcept override;
};

class GeneralException: public CustomException {
public:
	GeneralException(std::string msg);
};

class SaveException : public CustomException {
public:
	SaveException(std::string msg);
};

class LoadException : public CustomException {
public:
	LoadException(std::string msg);
};

class ParseException : public CustomException {
public:
	ParseException(std::string msg);
};

class AllocationException : public CustomException {
public:
	AllocationException();
	AllocationException(const char* who, int size);
};

class ReallocException : public AllocationException {
public:
	ReallocException(const char* who, int size);
};

#endif // UTILS_H