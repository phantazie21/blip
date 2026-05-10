#pragma once

#include "Component.h"
#include "../../math/Vec3.h"
struct BoxCollider : Component {
	Vec3 halfExtents = { 0.5f, 0.5f, 0.5f };
	void update(float dt) override {}
	void fixedUpdate(float dt) override {}
	const char* typeName() const override { return "BoxCollider"; }
	void drawInspector() override {
		ImGui::DragFloat3("Half Extents", &halfExtents.x, 0.01f, 0.f, 100.f);
	}
	json toJson() const override {
		return { {"halfExtents", {halfExtents.x, halfExtents.y, halfExtents.z}} };
	}
	void fromJson(const json& j) override {
		auto h = j["halfExtents"];
		halfExtents = { h[0], h[1], h[2] };
	}
};

struct SphereCollider : Component {
	float radius = 0.5f;
	void update(float dt) override {}
	void fixedUpdate(float dt) override {}
	const char* typeName() const override { return "SphereCollider"; }
	void drawInspector() override {
		ImGui::DragFloat("Radius", &radius, 0.01f, 0.f, 100.f);
	}
	json toJson() const override { return { {"radius", radius} }; }
	void fromJson(const json& j) override { radius = j.value("radius", 0.5f); }
};
