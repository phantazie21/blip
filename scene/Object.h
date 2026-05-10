#pragma once
#include "components/Component.h"
#include "../math/Vec3.h"
#include "../math/Mat4.h"
#include <vector>
#include <memory>

struct Transform {
    Vec3  position = {};
    float yaw = 0, pitch = 0, roll = 0;
};

struct Object {
    Transform           transform;
	std::vector<std::unique_ptr<Component>> components;

    Mat4 worldMatrix() const {
		return translation(transform.position)
			* rotateXY(transform.roll)
			* rotateYZ(transform.pitch)
			* rotateXZ(transform.yaw);
    }
	
    template<typename T, typename... Args>
    T* addComponent(Args&&... args) {
        auto c = std::make_unique<T>(std::forward<Args>(args)...);
        c->owner = this;
        T* ptr = c.get();
        components.push_back(std::move(c));
        return ptr;
    }

    template<typename T>
    T* getComponent() {
        for (auto& c : components)
            if (auto* t = dynamic_cast<T*>(c.get())) return t;
        return nullptr;
    }

    json toJson() const {
        json j;
        j["transform"] = {
            {"position", {transform.position.x, transform.position.y, transform.position.z}},
            {"yaw",   transform.yaw},
            {"pitch", transform.pitch},
            {"roll",  transform.roll}
        };
        json comps = json::array();
        for (const auto& c : components) {
            comps.push_back({
                {"type", c->typeName()},
                {"data", c->toJson()}
                });
        }
        j["components"] = comps;
        return j;
    }
    
    void fromJson(const json& j) {
        auto t = j["transform"];
        auto p = t["position"];
        transform.position = { p[0], p[1], p[2] };
        transform.yaw = t.value("yaw", 0.0f);
        transform.pitch = t.value("pitch", 0.0f);
        transform.roll = t.value("roll", 0.0f);

        components.clear();
        for (const auto& c : j["components"]) {
            std::string type = c["type"];
            auto comp = createComponent(type);
            if (!comp) continue;
            comp->owner = this;
            comp->fromJson(c["data"]);
            components.push_back(std::move(comp));
        }
    }
};
