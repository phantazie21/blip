#pragma once

struct Vec2 {
	float x;
	float y;
};

inline Vec2 operator*(const Vec2& a, float scalar) {
	return { a.x * scalar, a.y * scalar };
}

inline Vec2 operator+(const Vec2& a, const Vec2& b) {
	return { a.x + b.x, a.y + b.y };
}