#include "Component.h"
#include "MeshRenderer.h"
#include "Collider.h"
#include "Rigidbody.h"
#include "AudioSource.h"

std::unique_ptr<Component> createComponent(const std::string& typeName) {
    if (typeName == "MeshRenderer")    return std::make_unique<MeshRenderer>();
    if (typeName == "BoxCollider")     return std::make_unique<BoxCollider>();
    if (typeName == "SphereCollider")  return std::make_unique<SphereCollider>();
    if (typeName == "Rigidbody")       return std::make_unique<Rigidbody>();
	if (typeName == "AudioSource")     return std::make_unique<AudioSource>();
    return nullptr;
}