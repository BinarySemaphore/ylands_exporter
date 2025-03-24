#include "space.hpp"

#include <math.h>

const double PI = 3.14159265358979323846264338327950;
const double TAU = PI * 2.0;
const double PI_HALF = PI * 0.5;
const double TURN_FULL = TAU;
const double TURN_THREEQ = PI * 1.5;
const double TURN_TWOQ = PI;
const double TURN_HALF = PI;
const double TURN_ONEQ = PI_HALF;
const double DEG_TO_RAD = PI / 180.0;
const double RAD_TO_DEG = 180.0 / PI;

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

float Vector3::dot(const Vector3& v) const {
	return this->x * v.x + this->y * v.y + this->z * v.z;
}

Vector3 Vector3::cross(const Vector3& v) const {
	Vector3 result;
	result.x = this->y * v.z - this->z * v.y;
	result.y = this->z * v.x - this->x * v.z;
	result.z = this->x * v.y - this->y * v.x;
	return result;
}

Vector3 Vector3::operator+(const Vector3& v) const {
	Vector3 result;
	result.x = this->x + v.x;
	result.y = this->y + v.y;
	result.z = this->z + v.z;
	return result;
}

Vector3 Vector3::operator-(const Vector3& v) const {
	Vector3 result;
	result.x = this->x - v.x;
	result.y = this->y - v.y;
	result.z = this->z - v.z;
	return result;
}

Vector3 Vector3::operator*(float scalar) const {
	Vector3 result;
	result.x = scalar * this->x;
	result.y = scalar * this->y;
	result.z = scalar * this->z;
	return result;
}

Vector3 Vector3::operator*(const Vector3& v) const {
	Vector3 result;
	result.x = this->x * v.x;
	result.y = this->y * v.y;
	result.z = this->z * v.z;
	return result;
}

bool Vector3::operator==(const Vector3& v) const {
	return this->x == v.x && this->y == v.y && this->z == v.z;
}

Vector3 operator*(float scalar, const Vector3& v) {
	return Vector3(v * scalar);
}

Quaternion::Quaternion() {
	this->euler_order = RotationOrder::YXZ;
	this->w = 1.0f;
	this->x = 0.0f;
	this->y = 0.0f;
	this->z = 0.0f;
	this->cache_inverse = NULL;
}

Quaternion::Quaternion(float radians, const Vector3& axis) : Quaternion() {
	this->rotate(radians, axis);
}

Quaternion::Quaternion(const Vector3& v) : Quaternion() {
	this->w = 0.0f;
	this->x = v.x;
	this->y = v.y;
	this->z = v.z;
}

Quaternion::Quaternion(const Quaternion& q) {
	this->w = q.w;
	this->x = q.x;
	this->y = q.y;
	this->z = q.z;
	this->euler_order = q.euler_order;
	this->cache_inverse = NULL;
}

Quaternion::~Quaternion() {
	if (this->cache_inverse != NULL) delete this->cache_inverse;
}

void Quaternion::rotate(float radians, const Vector3& axis) {
	radians = radians * 0.5f;
	this->w = cos(radians);
	this->x = axis.x * sin(radians);
	this->y = axis.y * sin(radians);
	this->z = axis.z * sin(radians);
}

Vector3 rotateAxisOrderHelper(const RotationOrder& ord, int step) {
	if (step == 0) {
		if (ord == RotationOrder::XYZ || ord == RotationOrder::XZY) {
			return Vector3(1.0f, 0.0f, 0.0f);
		}
		if (ord == RotationOrder::YXZ || ord == RotationOrder::YZX) {
			return Vector3(0.0f, 1.0f, 0.0f);
		}
		if (ord == RotationOrder::ZXY || ord == RotationOrder::ZYX) {
			return Vector3(0.0f, 0.0f, 1.0f);
		}
	}
	if (step == 1) {
		if (ord == RotationOrder::YXZ || ord == RotationOrder::ZXY) {
			return Vector3(1.0f, 0.0f, 0.0f);
		}
		if (ord == RotationOrder::XYZ || ord == RotationOrder::ZYX) {
			return Vector3(0.0f, 1.0f, 0.0f);
		}
		if (ord == RotationOrder::XZY || ord == RotationOrder::YZX) {
			return Vector3(0.0f, 0.0f, 1.0f);
		}
	}
	if (step == 2) {
		if (ord == RotationOrder::YZX || ord == RotationOrder::ZYX) {
			return Vector3(1.0f, 0.0f, 0.0f);
		}
		if (ord == RotationOrder::XZY || ord == RotationOrder::ZXY) {
			return Vector3(0.0f, 1.0f, 0.0f);
		}
		if (ord == RotationOrder::XYZ || ord == RotationOrder::YXZ) {
			return Vector3(0.0f, 0.0f, 1.0f);
		}
	}
	return Vector3();
}

void Quaternion::rotate(const Vector3& euler_radians) {
	// TODO: better way to compound the axis, I suspect
	float radians;
	Vector3 axis;
	Quaternion qs[3];
	for (int i = 0; i < 3; i++) {
		axis = rotateAxisOrderHelper(this->euler_order, i);
		if (axis.x == 1.0f) radians = euler_radians.x;
		else if (axis.y == 1.0f) radians = euler_radians.y;
		else if (axis.z == 1.0f) radians = euler_radians.z;
		qs[i].rotate(radians, axis);
	}
	*this = qs[0] * qs[1] * qs[2];
}

void Quaternion::rotate_degrees(float degrees, const Vector3& axis) {
	this->rotate(degrees * DEG_TO_RAD, axis);
}

void Quaternion::rotate_degrees(const Vector3& euler_degrees) {
	Vector3 euler_radians = euler_degrees * DEG_TO_RAD;
	this->rotate(euler_radians);
}

Quaternion Quaternion::inverse() {
	this->qm.lock();
	if (this->cache_inverse == NULL) {
		this->qm.unlock();
		Quaternion* result = new Quaternion();
		float qlength = this->w * this->w + this->x * this->x
					+ this->y * this->y + this->z * this->z;
		result->w = this->w / qlength;
		result->x = -this->x / qlength;
		result->y = -this->y / qlength;
		result->z = -this->z / qlength;
		this->qm.lock();
		this->cache_inverse = result;
	}
	this->qm.unlock();
	return *this->cache_inverse;
}

Quaternion& Quaternion::operator=(const Quaternion& q) {
	if (this == &q) return *this;
	this->w = q.w;
	this->x = q.x;
	this->y = q.y;
	this->z = q.z;
	this->euler_order = q.euler_order;
	if (this->cache_inverse != NULL) delete this->cache_inverse;
	this->cache_inverse = q.cache_inverse;
	return *this;
}

Quaternion Quaternion::operator*(const Quaternion& q) const {
	Quaternion result;
	result.w = this->w * q.w - this->x * q.x - this->y * q.y - this->z * q.z;
	result.x = this->w * q.x + this->x * q.w + this->y * q.z - this->z * q.y;
	result.y = this->w * q.y - this->x * q.z + this->y * q.w + this->z * q.x;
	result.z = this->w * q.z + this->x * q.y - this->y * q.x + this->z * q.w;
	return result;
}

Vector3 Quaternion::operator*(const Vector3& v) {
	Vector3 result;
	// Full and accurate (slow)
	Quaternion pure_v(v);
	Quaternion rv = *this * pure_v * this->inverse();
	result.x = rv.x;
	result.y = rv.y;
	result.z = rv.z;
	Vector3 qv(this->x, this->y, this->z);
	// // Simplified mostly accurate (fast) (credit: GLM https://github.com/g-truc/glm)
	// // Note: maybe better with doubles than floats
	// Vector3 qv(this->x, this->y, this->z);
	// Vector3 qv_x_v = qv.cross(v);
	// result = v + 2.0f * (this->w * qv_x_v + qv.cross(qv_x_v));
	return result;
}