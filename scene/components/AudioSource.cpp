#include "AudioSource.h"
#include "../scene/Object.h"
#include "../scene/Scene.h"
#include "../AudioSystem.h"
#include "imgui.h"

extern AudioSystem* g_audio;

void AudioSource::update(float dt) {
    if (playOnStart && !wasStarted) {
        play();
        wasStarted = true;
    }
}

void AudioSource::play() {
    if (handle < 0 && !path.empty()) {
        handle = g_audio->loadSound(path);
    }
    if (handle >= 0) g_audio->play(handle, volume, pitch);
}

void AudioSource::drawInspector() {
    char buf[256];
    strncpy_s(buf, path.c_str(), sizeof(buf) - 1);
    if (ImGui::InputText("Path", buf, sizeof(buf))) {
        path = buf;
        handle = -1;
    }
    ImGui::DragFloat("Volume", &volume, 0.01f, 0.0f, 2.0f);
    ImGui::DragFloat("Pitch##AudioSource", &pitch, 0.01f, 0.1f, 4.0f);
    ImGui::Checkbox("Play on Start", &playOnStart);
    if (ImGui::Button("Play")) play();
}
