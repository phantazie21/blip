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
		obj->projected.resize(mesh->vertices.size());
		bool anyVisible = false;

		for (size_t i = 0; i < mesh->vertices.size(); i++) {
			obj->worldVerts[i] = world * mesh->vertices[i];
			Vec3 camVert = mv * mesh->vertices[i];
			obj->projected[i].visible = cam.project(camVert, r.width, r.height, obj->projected[i]);
			if (obj->projected[i].visible) anyVisible = true;
		}
		if (!anyVisible) continue;
		for (size_t i = 0; i < mesh->vertexNormals.size(); i++) {
			obj->worldNormals[i] = normalize(transformDirection(mesh->vertexNormals[i], world));
		}

		for (const Face& f : mesh->faces) {
			const ScreenVert& sa = obj->projected[f.idx[0]];
			const ScreenVert& sb = obj->projected[f.idx[1]];
			const ScreenVert& sc = obj->projected[f.idx[2]];
			if (!sa.visible || !sb.visible || !sc.visible) continue;
			if (std::abs(sa.x - sb.x) + std::abs(sa.y - sb.y) < 1.0f &&
				std::abs(sb.x - sc.x) + std::abs(sb.y - sc.y) < 1.0f &&
				std::abs(sc.x - sa.x) + std::abs(sc.y - sa.y) < 1.0f) continue;

			const Vec3& v0 = obj->worldVerts[f.idx[0]];
			const Vec3& v1 = obj->worldVerts[f.idx[1]];
			const Vec3& v2 = obj->worldVerts[f.idx[2]];
			Vec3 faceNormal = cross(v1 - v0, v2 - v0);
			if (dot(faceNormal, cam.position - v0) <= 0.0f) continue;
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

			Vec2 uv0 = {}, uv1 = {}, uv2 = {};
			if (!mesh->vertexUVs.empty() && f.uv[0] >= 0) {
				uv0 = mesh->vertexUVs[f.uv[0]];
				uv1 = mesh->vertexUVs[f.uv[1]];
				uv2 = mesh->vertexUVs[f.uv[2]];
			}
			r.submitDrawCall({
				sa, sb, sc,
				uv0, uv1, uv2,
				f.texture,
				f.color,
				i0, i1, i2
			});
		}
	}
	r.flush();
}