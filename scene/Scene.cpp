#include "Scene.h"
#include "../renderer/Renderer.h"
#include <fstream>
#include "Primitives.h"

AudioSystem* g_audio = nullptr;

Scene::Scene() {
	g_audio = &audioSystem;
}

void Scene::save(const std::string& path) const {
	json j;
	j["objects"] = json::array();
	for (const auto& obj : objects) {
		j["objects"].push_back(obj->toJson());
	}
	std::ofstream out(path);
	out << j.dump(2);  // pretty-print with 2-space indent
}

void Scene::load(const std::string& path) {
	std::ifstream in(path);
	if (!in.is_open()) return;
	json j;
	in >> j;

	objects.clear();
	for (const auto& objJson : j["objects"]) {
		auto obj = std::make_unique<Object>();
		obj->fromJson(objJson);

		// post-load: rebuild MeshRenderer's mesh from its descriptor
		if (auto* mr = obj->getComponent<MeshRenderer>()) {
			if (mr->meshType == "box") {
				mr->mesh = createBox(mr->meshParam, mr->color);
			}
			// add sphere/obj cases as you support them
		}

		objects.push_back(std::move(obj));
	}
}

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

void Scene::processFace(Mesh* mesh, const MeshRenderer* mr, const Face& f, Renderer& r, const Camera& cam, const Mat4& mv) {
	ClipVert ca{};
	ClipVert cb{};
	ClipVert cc{};
	ca.camPos = mv * mesh->vertices[f.idx[0]];
	cb.camPos = mv * mesh->vertices[f.idx[1]];
	cc.camPos = mv * mesh->vertices[f.idx[2]];
	ca.worldPos = mr->worldVerts[f.idx[0]];
	cb.worldPos = mr->worldVerts[f.idx[1]];
	cc.worldPos = mr->worldVerts[f.idx[2]];

	const Vec3& v0 = mr->worldVerts[f.idx[0]];
	const Vec3& v1 = mr->worldVerts[f.idx[1]];
	const Vec3& v2 = mr->worldVerts[f.idx[2]];
	Vec3 faceNormal = cross(v1 - v0, v2 - v0);
	if (dot(faceNormal, cam.position - v0) <= 0.0f) return;
	Vec3 fn = normalize(faceNormal);
	float baseDiffuse = std::max(0.0f, dot(fn, lightDir));
	float baseIntensity = ambientLight + (1.0f - ambientLight) * baseDiffuse;

	float i0 = baseIntensity, i1 = baseIntensity, i2 = baseIntensity;
	if (!mesh->vertexNormals.empty() && f.normal[0] >= 0) {
		auto gouraud = [&](int ni) {
			return ambientLight + (1.0f - ambientLight) *
				std::max(0.0f, dot(mr->worldNormals[ni], lightDir));
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
		MeshRenderer* mr = obj->getComponent<MeshRenderer>();
		if (!mr || !mr->mesh) continue;
		const Mat4 world = obj->worldMatrix();
		Mat4 mv = view * world;
		Vec3 camCenter = mv * mr->mesh->bounds.center;
		if (!isSphereVisible(camCenter, mr->mesh->bounds.radius, cam.fov, aspect)) continue;
		Mesh* mesh = mr->mesh.get();
		if (!mr->lods.empty()) {
			int lodCount = (int)mr->lods.size();
			float pixelRadius = (mesh->bounds.radius / camCenter.z) * cam.focal * (r.height * 0.5f);
			if (pixelRadius < 40 && lodCount > 3) mesh = &mr->lods[3];
			else if (pixelRadius < 80 && lodCount > 2) mesh = &mr->lods[2];
			else if (pixelRadius < 120 && lodCount > 1) mesh = &mr->lods[1];
			else if (lodCount > 0)                      mesh = &mr->lods[0];
		}
		mr->worldVerts.resize(mesh->vertices.size());
		mr->worldNormals.resize(mesh->vertexNormals.size());

		for (size_t i = 0; i < mesh->vertices.size(); i++)
			mr->worldVerts[i] = world * mesh->vertices[i];
		for (size_t i = 0; i < mesh->vertexNormals.size(); i++)
			mr->worldNormals[i] = normalize(transformDirection(mesh->vertexNormals[i], world));

		for (const Face& f : mesh->faces)
			processFace(mesh, mr, f, r, cam, mv);
	}
	r.flush();
}

void Scene::fixedUpdate(float dt) {
	const Vec3 gravity = { 0.f, -9.81f, 0.f };
	const int substeps = 4;
	const float subDT = dt / substeps;
	for (int i = 0; i < substeps; i++) {
		for (const auto& obj : objects) {
			Rigidbody* rb = obj->getComponent<Rigidbody>();
			if (!rb || rb->isStatic) continue;
			if (rb->useGravity) rb->velocity = rb->velocity + gravity * subDT;
			obj->transform.position = obj->transform.position + rb->velocity * subDT;
		}
		collisionSystem.detectCollisions(objects, contacts);
		for (const auto& c : contacts) {
			Rigidbody* rbA = c.a->getComponent<Rigidbody>();
			Rigidbody* rbB = c.b->getComponent<Rigidbody>();
			bool aDyn = rbA && !rbA->isStatic;
			bool bDyn = rbB && !rbB->isStatic;

			Vec3 push = c.normal * c.penetration;
			if (aDyn && bDyn) {
				c.a->transform.position = c.a->transform.position + push * 0.5f;
				c.b->transform.position = c.b->transform.position - push * 0.5f;
			}
			else if (aDyn) {
				c.a->transform.position = c.a->transform.position + push;
			}
			else if (bDyn) {
				c.b->transform.position = c.b->transform.position - push;
			}

			if (aDyn) {
				float vAlongN = dot(rbA->velocity, c.normal);
				if (vAlongN < 0) rbA->velocity = rbA->velocity - c.normal * vAlongN;
			}
			if (bDyn) {
				float vAlongN = dot(rbB->velocity, c.normal);
				if (vAlongN > 0) rbB->velocity = rbB->velocity - c.normal * vAlongN;
			}
		}
	}
}

void Scene::update(float dt) {
	for (const auto& obj : objects) {
		for (const auto& comp : obj->components) {
			comp->update(dt);
		}
	}
}