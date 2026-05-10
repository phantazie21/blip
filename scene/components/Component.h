#pragma once
#include "imgui.h"
#include "json.hpp"

using json = nlohmann::json;

struct Object;
struct Component {
	Object* owner = nullptr;
	virtual ~Component() = default;
	virtual void update(float dt) = 0;
	virtual void fixedUpdate(float dt) = 0;
	virtual const char* typeName() const = 0;
	virtual void drawInspector() {}
	virtual json toJson() const { return {}; }
	virtual void fromJson(const json& j) {}
};

std::unique_ptr<Component> createComponent(const std::string& typeName);
