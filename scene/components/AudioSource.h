#pragma once
#include "Component.h"
#include <string>

struct AudioSource : Component {
    std::string path;
    float volume = 1.0f;
    float pitch = 1.0f;
    bool  playOnStart = false;

    int handle = -1;
    bool wasStarted = false;

    void update(float dt) override;
    void fixedUpdate(float dt) override {}
    const char* typeName() const override { return "AudioSource"; }
    void drawInspector() override;

	json toJson() const override {
		return { {"path", path}, {"volume", volume}, {"pitch", pitch}, {"playOnStart", playOnStart} };
	}

	void fromJson(const json& j) override {
		path = j.value("path", "");
		volume = j.value("volume", 1.0f);
		pitch = j.value("pitch", 1.0f);
		playOnStart = j.value("playOnStart", false);
	}

    void play();
};