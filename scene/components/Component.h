#pragma once
struct Object;
struct Component {
	Object* owner = nullptr;
	virtual ~Component() = default;
	virtual void update(float dt) = 0;
	virtual void fixedUpdate(float dt) = 0;
};
