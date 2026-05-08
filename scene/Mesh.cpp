#include "Mesh.h"

BoundingSphere computeBounds(const std::vector<Vec3>& vertices) {
	float minX = std::numeric_limits<float>::max(), minY = std::numeric_limits<float>::max(), minZ = std::numeric_limits<float>::max();
	float maxX = std::numeric_limits<float>::lowest(), maxY = std::numeric_limits<float>::lowest(), maxZ = std::numeric_limits<float>::lowest();

	for (const auto& v : vertices) {
		if (v.x < minX) minX = v.x;
		if (v.y < minY) minY = v.y;
		if (v.z < minZ) minZ = v.z;
		if (v.x > maxX) maxX = v.x;
		if (v.y > maxY) maxY = v.y;
		if (v.z > maxZ) maxZ = v.z;
	}

	Vec3 center = {(minX + maxX) / 2, (minY + maxY) / 2, (minZ + maxZ) / 2};
	float radius = 0;
	for (const auto& v : vertices) {
		float dist = length(v - center);
		if (dist > radius) radius = dist;
	}

	return {center, radius};
}

void computeVertexNormals(Mesh& mesh) {
	mesh.vertexNormals.assign(mesh.vertices.size(), { 0.0f, 0.0f, 0.0f });
	for (const auto& face : mesh.faces) {
		const Vec3& v0 = mesh.vertices[face.idx[0]];
		const Vec3& v1 = mesh.vertices[face.idx[1]];
		const Vec3& v2 = mesh.vertices[face.idx[2]];
		Vec3 normal = cross(v1 - v0, v2 - v0);
		normal = normalize(normal);
		mesh.vertexNormals[face.idx[0]] = mesh.vertexNormals[face.idx[0]] + normal;
		mesh.vertexNormals[face.idx[1]] = mesh.vertexNormals[face.idx[1]] + normal;
		mesh.vertexNormals[face.idx[2]] = mesh.vertexNormals[face.idx[2]] + normal;
	}
	for (auto& n : mesh.vertexNormals) {
		n = normalize(n);
	}
}
