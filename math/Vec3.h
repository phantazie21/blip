#pragma once

#include <cmath>

struct Vec3 {
	float x;
	float y;
	float z;
};

inline Vec3 operator+(const Vec3& a, const Vec3& b) {
	return { a.x + b.x, a.y + b.y, a.z + b.z };
}

inline Vec3 operator-(const Vec3& a, const Vec3& b) {
	return { a.x - b.x, a.y - b.y, a.z - b.z };
}

inline Vec3 operator*(const Vec3& a, float scalar) {
	return { a.x * scalar, a.y * scalar, a.z * scalar };
}

inline Vec3 operator/(const Vec3& a, float scalar) {
	return { a.x / scalar, a.y / scalar, a.z / scalar };
}

inline Vec3 operator-(const Vec3& v) {
	return { -v.x, -v.y, -v.z };
}

inline float dot(const Vec3& a, const Vec3& b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 cross(const Vec3& a, const Vec3& b) {
	return {
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	};
}

inline float length(const Vec3& v) {
	return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline Vec3 normalize(const Vec3& v) {
	float len = length(v);
	if (len < 1e-8f) return { 0, 0, 0 };
	return { v.x / len, v.y / len, v.z / len };
}

inline Vec3 lerp(const Vec3& a, const Vec3& b, float t) {
	return a * (1.0f - t) + b * t;
}