#ifndef SPACE_H
#define SPACE_H

extern const double PI;
extern const double TAU;
extern const double PI_HALF;
extern const double TURN_FULL;
extern const double TURN_THREEQ;
extern const double TURN_TWOQ;
extern const double TURN_HALF;
extern const double TURN_ONEQ;

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
	bool operator==(const Vector3& v) const;
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

#endif // SPACE_H