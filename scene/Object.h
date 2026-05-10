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
};
