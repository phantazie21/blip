#pragma once

#include "Component.h"
#include "../../math/Vec3.h"
struct BoxCollider : Component {
	Vec3 halfExtents = { 0.5f, 0.5f, 0.5f };
	void update(float dt) override {}
	void fixedUpdate(float dt) override {}
};

struct SphereCollider : Component {
	float radius = 0.5f;
	void update(float dt) override {}
	void fixedUpdate(float dt) override {}
};
