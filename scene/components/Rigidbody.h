#pragma once
#include "Component.h"
#include "../../math/Vec3.h"

struct Rigidbody : Component {
    Vec3  velocity = { 0, 0, 0 };
    float mass = 1.0f;
    bool  isStatic = false;  // true = immovable (floor, walls)
    bool  useGravity = true;

    void update(float dt) override {}
    void fixedUpdate(float dt) override {}
};
