#pragma once

#include "Vec3.h"

struct Mat4 {
	float m[16];
};

inline Mat4 identity() {
	return {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};
}

inline Mat4 translation(float x, float y, float z) {
	return {
		1, 0, 0, x,
		0, 1, 0, y,
		0, 0, 1, z,
		0, 0, 0, 1
	};
}

inline Mat4 translation(const Vec3& v) {
	return translation(v.x, v.y, v.z);
}

inline Mat4 rotateXZ(float angle) {
	float c = cosf(angle);
	float s = sinf(angle);
	return {
		c, 0, -s, 0,
		0, 1, 0, 0,
		s, 0, c, 0,
		0, 0, 0, 1
	};
}

inline Mat4 rotateYZ(float angle) {
	float c = cosf(angle);
	float s = sinf(angle);
	return {
		1, 0, 0, 0,
		0, c, -s, 0,
		0, s, c, 0,
		0, 0, 0, 1
	};
}

inline Mat4 rotateXY(float angle) {
	float c = cosf(angle);
	float s = sinf(angle);
	return {
		c, -s, 0, 0,
		s, c, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};
}

inline Mat4 scale(float sx, float sy, float sz) {
	return {
		sx, 0, 0, 0,
		0, sy, 0, 0,
		0, 0, sz, 0,
		0, 0, 0, 1
	};
}

inline Mat4 scale(const Vec3& s) {
	return scale(s.x, s.y, s.z);
}

inline Mat4 scale(float s) {
	return scale(s, s, s);
}

inline Mat4 operator*(const Mat4& a, const Mat4& b) {
	Mat4 result = {};
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			result.m[i * 4 + j] =
				a.m[i * 4 + 0] * b.m[0 * 4 + j] +
				a.m[i * 4 + 1] * b.m[1 * 4 + j] +
				a.m[i * 4 + 2] * b.m[2 * 4 + j] +
				a.m[i * 4 + 3] * b.m[3 * 4 + j];
		}
	}
	return result;
}

inline Mat4 operator*(const Mat4& m, float scalar) {
	Mat4 result = {};
	for (int i = 0; i < 16; ++i) {
		result.m[i] = m.m[i] * scalar;
	}
	return result;
}

inline Mat4 operator*(float scalar, const Mat4& m) {
	return m * scalar;
}

inline Vec3 operator*(const Mat4& m, const Vec3& v) {
	Vec3 result = {};
	result.x = m.m[0] * v.x + m.m[1] * v.y + m.m[2] * v.z + m.m[3];
	result.y = m.m[4] * v.x + m.m[5] * v.y + m.m[6] * v.z + m.m[7];
	result.z = m.m[8] * v.x + m.m[9] * v.y + m.m[10] * v.z + m.m[11];
	return result;
}

inline Vec3 transformDirection(const Vec3& v, const Mat4& m) {
	Vec3 result = {};
	result.x = m.m[0] * v.x + m.m[1] * v.y + m.m[2] * v.z;
	result.y = m.m[4] * v.x + m.m[5] * v.y + m.m[6] * v.z;
	result.z = m.m[8] * v.x + m.m[9] * v.y + m.m[10] * v.z;
	return result;
}
