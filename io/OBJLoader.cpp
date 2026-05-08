#include "OBJLoader.h"
#include <vector>
#include "../math/Vec3.h"
#include "../math/Vec2.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <array>

struct OBJData {
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;
    std::vector<std::array<int, 3>> facePos;
    std::vector<std::array<int, 3>> faceNorm;
    std::vector<std::array<int, 3>> faceUV;
};

static void parseFaceVertex(const std::string& token, int& pos, int& uv, int& norm) {
    pos = uv = norm = -1;
    std::istringstream ss(token);
    std::string part;
    int idx = 0;
    while (std::getline(ss, part, '/')) {
        if (!part.empty()) {
            int val = std::stoi(part) - 1; // OBJ is 1-indexed
            if (idx == 0) pos = val;
            if (idx == 1) uv = val;
            if (idx == 2) norm = val;
        }
        idx++;
    }
}

static OBJData parseOBJ(const std::string& path) {
    OBJData data;
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "OBJLoader: could not open file: " << path << "\n";
        return data;
    }

    std::string line;
    while (std::getline(file, line)) {
        auto comment = line.find('#');
        if (comment != std::string::npos) line = line.substr(0, comment);

        std::istringstream ss(line);
        std::string token;
        if (!(ss >> token)) continue;

        if (token == "v") {
            Vec3 v{};
            ss >> v.x >> v.y >> v.z;
            data.positions.push_back(v);
        }
        else if (token == "vn") {
            Vec3 n{};
            ss >> n.x >> n.y >> n.z;
            data.normals.push_back(n);
        }
        else if (token == "vt") {
            Vec2 uv{};
            ss >> uv.x >> uv.y;
            data.uvs.push_back(uv);
        }
        else if (token == "f") {
            std::vector<std::array<int, 3>> faceVerts; // [pos, uv, norm]
            std::string vtok;
            while (ss >> vtok) {
                int p, u, n;
                parseFaceVertex(vtok, p, u, n);
                faceVerts.push_back({ p, u, n });
            }
            // fan triangulation from vertex 0
            for (int i = 1; i < (int)faceVerts.size() - 1; i++) {
                data.facePos.push_back({ faceVerts[0][0], faceVerts[i][0], faceVerts[i + 1][0] });
                data.faceUV.push_back({ faceVerts[0][1], faceVerts[i][1], faceVerts[i + 1][1] });
                data.faceNorm.push_back({ faceVerts[0][2], faceVerts[i][2], faceVerts[i + 1][2] });
            }
        }
        // silently skip: o, g, s, usemtl, mtllib, l, etc.
    }

    return data;
}

std::unique_ptr<Mesh> loadOBJ(const std::string& path, uint32_t color, float scale) {
    OBJData raw = parseOBJ(path);
    if (raw.positions.empty()) return nullptr;

    auto mesh = std::make_unique<Mesh>();

    // scale and copy vertices
    mesh->vertices.reserve(raw.positions.size());
    for (const auto& p : raw.positions)
        mesh->vertices.push_back({ p.x * scale, p.y * scale, p.z * scale });

    // copy UVs and normals as-is
    mesh->vertexUVs = raw.uvs;
    mesh->vertexNormals = raw.normals;

    // build faces
    mesh->faces.reserve(raw.facePos.size());
    for (size_t fi = 0; fi < raw.facePos.size(); fi++) {
        Face face{};
        face.idx[0] = raw.facePos[fi][0];
        face.idx[1] = raw.facePos[fi][1];
        face.idx[2] = raw.facePos[fi][2];
        face.uv[0] = raw.faceUV[fi][0];
        face.uv[1] = raw.faceUV[fi][1];
        face.uv[2] = raw.faceUV[fi][2];
        face.normal[0] = raw.faceNorm[fi][0];
        face.normal[1] = raw.faceNorm[fi][1];
        face.normal[2] = raw.faceNorm[fi][2];
        face.color = color;
        face.texture = nullptr;
        mesh->faces.push_back(face);
    }

    // generate normals if OBJ had none
    if (mesh->vertexNormals.empty())
        computeVertexNormals(*mesh);

    mesh->bounds = computeBounds(mesh->vertices);
    return mesh;
}