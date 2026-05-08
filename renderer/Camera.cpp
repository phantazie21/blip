#include "Camera.h"

Mat4 Camera::viewMatrix() const {
    return rotateYZ(pitch) * rotateXZ(-yaw) * translation(-position);
}

bool Camera::project(Vec3 camSpaceVert, int screenW, int screenH, ScreenVert& out) const {
	if (camSpaceVert.z <= 0.01f) return false;
	const float invZ = 1.f / camSpaceVert.z;
	const float aspect = (float)screenW / screenH;
	const float pX = (camSpaceVert.x * invZ) * focal / aspect;
	const float pY = (camSpaceVert.y * invZ) * focal;
	out.x = (pX + 1) * 0.5f * screenW;
	out.y = (1 - pY) * 0.5f * screenH;
	out.z = camSpaceVert.z;
	return true;
}

void Camera::setFov(float f) {
	fov = f;
	focal = 1.0f / std::tanf(fov * 0.5f);
}