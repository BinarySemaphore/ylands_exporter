#ifndef SPACE_H
#define SPACE_H

#define PI 3.14159265358979323846264338327950
const double TAU = PI * 2.0;
const double PI_HALF = PI * 0.5;
const double TURN_FULL = TAU;
const double TURN_THREEQ = PI * 1.5;
const double TURN_TWOQ = PI;
const double TURN_HALF = PI;
const double TURN_ONEQ = PI_HALF;

class Vector2 {
public:
	float x, y;
	Vector2();
	Vector2(float x, float y);
};

class Vector3 {
public:
	float x, y, z;
	Vector3();
	Vector3(float x, float y, float z);
	float dot(const Vector3& v) const;
	Vector3 cross(const Vector3& v) const;
	Vector3 operator+(const Vector3& v) const;
	Vector3 operator*(float scalar) const;
	Vector3 operator*(const Vector3& v) const;
};

static Vector3 operator*(float scalar, const Vector3& v);

class Quaternion {
public:
	float w, x, y, z;
	Quaternion();
	Quaternion(const Vector3& v);
	Quaternion(float rads, const Vector3& axis);
	Quaternion inverse() const;
	Quaternion operator*(const Quaternion& q) const;
	Vector3 operator*(const Vector3& v) const;
};

class Node {
public:
	Vector3 position;
	Quaternion rotation;
};

#endif // SPACE_H