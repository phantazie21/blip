#pragma once
#include "imgui.h"
#include "json.hpp"

using json = nlohmann::json;

struct Object;
struct Vec3;
struct Component {
	Object* owner = nullptr;
	bool hasStarted = false;
	virtual ~Component() = default;
	virtual void update(float dt) = 0;
	virtual void fixedUpdate(float dt) = 0;
	virtual const char* typeName() const = 0;
	virtual void drawInspector() {}
	virtual json toJson() const { return {}; }
	virtual void fromJson(const json& j) {}

	virtual void onCollisionEnter(Object* other, const Vec3& normal, float penetration) {}
	virtual void onCollisionExit(Object* other) {}
	virtual void onStart() {}
	virtual void onDestroy() {}
};

std::unique_ptr<Component> createComponent(const std::string& typeName);
