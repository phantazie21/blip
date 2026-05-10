#pragma once
#include "Object.h"
#include "components/Collider.h"
#include "components/Rigidbody.h"
struct ContactInfo {
    Object* a;
    Object* b;
    Vec3    normal;
    float   penetration;
};

struct WorldBox {
    Vec3 center;
    Vec3 halfExtents;
};

struct WorldSphere {
    Vec3 center;
    float radius;
};

class CollisionSystem {
public:
    void detectCollisions(const std::vector<std::unique_ptr<Object>>& objects,
        std::vector<ContactInfo>& outContacts);
};
