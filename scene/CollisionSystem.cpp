#include "CollisionSystem.h"
#include <algorithm>

static WorldBox getWorldBox(Object* obj, BoxCollider* bc) {
    return { obj->transform.position, bc->halfExtents };
}

static WorldSphere getWorldSphere(Object* obj, SphereCollider* sc) {
    return { obj->transform.position, sc->radius };
}

static bool testSphereSphere(const WorldSphere& a, const WorldSphere& b, ContactInfo& out) {
    Vec3 d = a.center - b.center;
    float distSq = dot(d, d);
    float r = a.radius + b.radius;
    if (distSq >= r * r) return false;

    float dist = std::sqrtf(distSq);
    if (dist < 1e-6f) {
        // exactly overlapping centers — pick arbitrary normal
        out.normal = { 0, 1, 0 };
        out.penetration = r;
    }
    else {
        out.normal = d / dist;
        out.penetration = r - dist;
    }
    return true;
}

static bool testBoxBox(const WorldBox& a, const WorldBox& b, ContactInfo& out) {
    Vec3 d = a.center - b.center;
    Vec3 overlap = {
        a.halfExtents.x + b.halfExtents.x - std::abs(d.x),
        a.halfExtents.y + b.halfExtents.y - std::abs(d.y),
        a.halfExtents.z + b.halfExtents.z - std::abs(d.z)
    };
    if (overlap.x <= 0 || overlap.y <= 0 || overlap.z <= 0) return false;

    // separate along the axis of smallest overlap (minimum translation vector)
    if (overlap.x < overlap.y && overlap.x < overlap.z) {
        out.normal = { d.x < 0 ? -1.f : 1.f, 0, 0 };
        out.penetration = overlap.x;
    }
    else if (overlap.y < overlap.z) {
        out.normal = { 0, d.y < 0 ? -1.f : 1.f, 0 };
        out.penetration = overlap.y;
    }
    else {
        out.normal = { 0, 0, d.z < 0 ? -1.f : 1.f };
        out.penetration = overlap.z;
    }
    return true;
}

static bool testBoxSphere(const WorldBox& box, const WorldSphere& sph, ContactInfo& out) {
    // closest point on box to sphere center
    Vec3 d = sph.center - box.center;
    Vec3 clamped = {
        std::clamp(d.x, -box.halfExtents.x, box.halfExtents.x),
        std::clamp(d.y, -box.halfExtents.y, box.halfExtents.y),
        std::clamp(d.z, -box.halfExtents.z, box.halfExtents.z)
    };
    Vec3 closest = box.center + clamped;
    Vec3 toSph = sph.center - closest;
    float distSq = dot(toSph, toSph);
    if (distSq >= sph.radius * sph.radius) return false;

    float dist = std::sqrtf(distSq);
    if (dist < 1e-6f) {
        out.normal = { 0, 1, 0 };  // sphere center inside box, arbitrary normal
        out.penetration = sph.radius;
    }
    else {
        out.normal = toSph / dist;       // pointing from box to sphere
        out.penetration = sph.radius - dist;
    }
    return true;
}

void CollisionSystem::detectCollisions(
    const std::vector<std::unique_ptr<Object>>& objects,
    std::vector<ContactInfo>& outContacts)
{
    outContacts.clear();

    // collect all colliders with their object owners
    struct ColliderRef {
        Object* obj;
        BoxCollider* box = nullptr;
        SphereCollider* sphere = nullptr;
    };
    std::vector<ColliderRef> refs;
    for (const auto& obj : objects) {
        ColliderRef ref{ obj.get() };
        ref.box = obj->getComponent<BoxCollider>();
        ref.sphere = obj->getComponent<SphereCollider>();
        if (ref.box || ref.sphere) refs.push_back(ref);
    }

    // O(n^2) pair test — fine for now, optimize with broadphase later
    for (size_t i = 0; i < refs.size(); i++) {
        for (size_t j = i + 1; j < refs.size(); j++) {
            ContactInfo c;
            c.a = refs[i].obj;
            c.b = refs[j].obj;
            bool hit = false;

            if (refs[i].box && refs[j].box) {
                hit = testBoxBox(getWorldBox(refs[i].obj, refs[i].box),
                    getWorldBox(refs[j].obj, refs[j].box), c);
            }
            else if (refs[i].sphere && refs[j].sphere) {
                hit = testSphereSphere(getWorldSphere(refs[i].obj, refs[i].sphere),
                    getWorldSphere(refs[j].obj, refs[j].sphere), c);
            }
            else if (refs[i].box && refs[j].sphere) {
                hit = testBoxSphere(getWorldBox(refs[i].obj, refs[i].box),
                    getWorldSphere(refs[j].obj, refs[j].sphere), c);
            }
            else if (refs[i].sphere && refs[j].box) {
                hit = testBoxSphere(getWorldBox(refs[j].obj, refs[j].box),
                    getWorldSphere(refs[i].obj, refs[i].sphere), c);
                if (hit) {
                    // we asked the test as (box=j, sphere=i), so swap a/b and flip normal
                    std::swap(c.a, c.b);
                    c.normal = c.normal * -1.0f;
                }
            }

            if (hit) outContacts.push_back(c);
        }
    }
}
