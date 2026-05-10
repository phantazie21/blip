#include "Scene.h"
#include "../renderer/Renderer.h"

static bool isSphereVisible(Vec3 camSpaceCenter, float radius, float fov, float aspect) {
	if (camSpaceCenter.z + radius <= 0.01f) return false;
	float tanY = std::tanf(fov * 0.5f);
	float tanX = tanY * aspect;
	if (std::abs(camSpaceCenter.x) > camSpaceCenter.z * tanX + radius) return false;
	if (std::abs(camSpaceCenter.y) > camSpaceCenter.z * tanY + radius) return false;
	return true;
}

int clipTriangleNearPlane(const ClipVert in[3], ClipVert out[4], float nearZ) {
	int outCount = 0;

	for (int i = 0; i < 3; i++) {
		const ClipVert& a = in[i];
		const ClipVert& b = in[(i + 1) % 3];
		bool aInside = a.camPos.z > nearZ;
		bool bInside = b.camPos.z > nearZ;

		if (aInside) {
			out[outCount++] = a;
		}
		if (aInside != bInside) {
			float t = (nearZ - a.camPos.z) / (b.camPos.z - a.camPos.z);
			out[outCount].camPos = lerp(a.camPos, b.camPos, t);
			out[outCount].worldPos = lerp(a.worldPos, b.worldPos, t);
			out[outCount].uv = a.uv * (1.0f - t) + b.uv * t;
			out[outCount].intensity = a.intensity * (1.0f - t) + b.intensity * t;
			outCount++;
		}
	}
	return outCount; // 0 if all outside, 3 or 4 if mixed/all inside
}

void Scene::processFace(Mesh* mesh, const std::unique_ptr<Object>& obj, const Face& f, Renderer& r, const Camera& cam, const Mat4& mv) {
	ClipVert ca;
	ClipVert cb;
	ClipVert cc;
	ca.camPos = mv * mesh->vertices[f.idx[0]];
	cb.camPos = mv * mesh->vertices[f.idx[1]];
	cc.camPos = mv * mesh->vertices[f.idx[2]];
	ca.worldPos = obj->worldVerts[f.idx[0]];
	cb.worldPos = obj->worldVerts[f.idx[1]];
	cc.worldPos = obj->worldVerts[f.idx[2]];

	const Vec3& v0 = obj->worldVerts[f.idx[0]];
	const Vec3& v1 = obj->worldVerts[f.idx[1]];
	const Vec3& v2 = obj->worldVerts[f.idx[2]];
	Vec3 faceNormal = cross(v1 - v0, v2 - v0);
	if (dot(faceNormal, cam.position - v0) <= 0.0f) return;
	Vec3 fn = normalize(faceNormal);
	float baseDiffuse = std::max(0.0f, dot(fn, lightDir));
	float baseIntensity = ambientLight + (1.0f - ambientLight) * baseDiffuse;

	float i0 = baseIntensity, i1 = baseIntensity, i2 = baseIntensity;
	if (!mesh->vertexNormals.empty() && f.normal[0] >= 0) {
		auto gouraud = [&](int ni) {
			return ambientLight + (1.0f - ambientLight) *
				std::max(0.0f, dot(obj->worldNormals[ni], lightDir));
			};
		i0 = gouraud(f.normal[0]);
		i1 = gouraud(f.normal[1]);
		i2 = gouraud(f.normal[2]);
	}
	ca.intensity = i0;
	cb.intensity = i1;
	cc.intensity = i2;

	Vec2 uv0 = {}, uv1 = {}, uv2 = {};
	if (!mesh->vertexUVs.empty() && f.uv[0] >= 0) {
		uv0 = mesh->vertexUVs[f.uv[0]];
		uv1 = mesh->vertexUVs[f.uv[1]];
		uv2 = mesh->vertexUVs[f.uv[2]];
	}
	ca.uv = uv0;
	cb.uv = uv1;
	cc.uv = uv2;
	ClipVert in[3] = { ca, cb, cc };
	ClipVert clipped[4];
	int clippedCount = clipTriangleNearPlane(in, clipped, cam.nearZ * 1.5f);
	if (clippedCount < 3) return;

	ScreenVert proj[4];
	for (int i = 0; i < clippedCount; i++) {
		cam.project(clipped[i].camPos, r.width, r.height, proj[i]);
		proj[i].visible = true;
	}

	if (std::abs(proj[0].x - proj[1].x) + std::abs(proj[0].y - proj[1].y) < 1.0f &&
		std::abs(proj[1].x - proj[2].x) + std::abs(proj[1].y - proj[2].y) < 1.0f &&
		std::abs(proj[2].x - proj[0].x) + std::abs(proj[2].y - proj[0].y) < 1.0f) return;

	auto submit = [&](int a, int b, int c) {
		r.submitDrawCall({
			proj[a], proj[b], proj[c],
			clipped[a].uv, clipped[b].uv, clipped[c].uv,
			f.texture, f.color,
			clipped[a].intensity, clipped[b].intensity, clipped[c].intensity
			});
		};

	submit(0, 1, 2);
	if (clippedCount == 4) submit(0, 2, 3);
}

void Scene::render(Renderer& r, const Camera& cam) {
	const Mat4 view = cam.viewMatrix();
	const float aspect = (float)r.width / r.height;
	for (const auto& obj : objects) {
		const Mat4 world = obj->worldMatrix();
		Mat4 mv = view * world;
		Vec3 camCenter = mv * obj->mesh->bounds.center;
		if (!isSphereVisible(camCenter, obj->mesh->bounds.radius, cam.fov, aspect)) continue;
		Mesh* mesh = obj->mesh.get();
		if (!obj->lods.empty()) {
			int lodCount = (int)obj->lods.size();
			float pixelRadius = (mesh->bounds.radius / camCenter.z) * cam.focal * (r.height * 0.5f);
			if (pixelRadius < 40 && lodCount > 3) mesh = &obj->lods[3];
			else if (pixelRadius < 80 && lodCount > 2) mesh = &obj->lods[2];
			else if (pixelRadius < 120 && lodCount > 1) mesh = &obj->lods[1];
			else if (lodCount > 0)                      mesh = &obj->lods[0];
		}
		obj->worldVerts.resize(mesh->vertices.size());
		obj->worldNormals.resize(mesh->vertexNormals.size());

		for (size_t i = 0; i < mesh->vertices.size(); i++) {
			obj->worldVerts[i] = world * mesh->vertices[i];
		}
		for (size_t i = 0; i < mesh->vertexNormals.size(); i++) {
			obj->worldNormals[i] = normalize(transformDirection(mesh->vertexNormals[i], world));
		}
		for (const Face& f : mesh->faces) {
			processFace(mesh, obj, f, r, cam, mv);
		}
	}
	r.flush();
}