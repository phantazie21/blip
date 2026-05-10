#pragma once

#include "../math/Vec3.h"
#include "../math/Mat4.h"
#include "Rasterizer.h"

struct Camera {
	Vec3 position;
	float yaw;
	float pitch;
	float fov;
	float focal;
	float nearZ = 0.01f;
	Mat4 viewMatrix() const;
	bool project(Vec3 v, int screenW, int screenH, ScreenVert& out) const;
	void setFov(float f);
};
