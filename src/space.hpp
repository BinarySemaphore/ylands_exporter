#ifndef SPACE_H
#define SPACE_H

#include <mutex>

extern const double PI;
extern const double TAU;
extern const double PI_HALF;
extern const double TURN_FULL;
extern const double TURN_THREEQ;
extern const double TURN_TWOQ;
extern const double TURN_HALF;
extern const double TURN_ONEQ;
extern const double DEG_TO_RAD;
extern const double RAD_TO_DEG;

enum class RotationOrder {
	XYZ,
	XZY,
	YXZ,
	YZX,
	ZXY,
	ZYX
};

class Vector2 {
public:
	float x, y;
	/// @brief Zero vector
	Vector2();
	Vector2(float x, float y);
};

class Vector3 {
public:
	float x, y, z;
	/// @brief Zero vector
	Vector3();
	Vector3(float x, float y, float z);
	float dot(const Vector3& v) const;
	Vector3 cross(const Vector3& v) const;
	Vector3 operator+(const Vector3& v) const;
	Vector3 operator-(const Vector3& v) const;
	Vector3 operator*(float scalar) const;
	Vector3 operator*(const Vector3& v) const;
	bool operator==(const Vector3& v) const;
};

extern Vector3 operator*(float scalar, const Vector3& v);

class Quaternion {
private:
	std::mutex qm;
	Quaternion* cache_inverse;
public:
	float w, x, y, z;
	RotationOrder euler_order;
	/// @brief Identity quaternion
	Quaternion();
	Quaternion(float radians, const Vector3& axis);
	/// @brief Generating a pure quaternion, not rotational
	/// @param v vector to embed
	Quaternion(const Vector3& v);
	Quaternion(const Quaternion& q);
	~Quaternion();

	void rotate(float radians, const Vector3& axis);
	void rotate(const Vector3& euler_radians);
	void rotate_degrees(float degrees, const Vector3& axis);
	void rotate_degrees(const Vector3& euler_degrees);
	Quaternion inverse();

	Quaternion& operator=(const Quaternion& q);
	Quaternion operator*(const Quaternion& q) const;
	Vector3 operator*(const Vector3& v);
};

#endif // SPACE_H