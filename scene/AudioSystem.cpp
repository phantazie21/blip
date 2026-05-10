#define MINIAUDIO_IMPLEMENTATION
#include "../third_party/miniaudio.h"
#include "AudioSystem.h"

AudioSystem::AudioSystem() {
    engine = std::make_unique<ma_engine>();
    if (ma_engine_init(nullptr, engine.get()) != MA_SUCCESS) {
        engine.reset();
    }
}

AudioSystem::~AudioSystem() {
    for (auto& s : sounds) {
        if (s) ma_sound_uninit(s.get());
    }
    if (engine) ma_engine_uninit(engine.get());
}

int AudioSystem::loadSound(const std::string& path) {
    if (!engine) return -1;
    auto it = pathToHandle.find(path);
    if (it != pathToHandle.end()) return it->second;

    auto sound = std::make_unique<ma_sound>();
    if (ma_sound_init_from_file(engine.get(), path.c_str(), 0, nullptr, nullptr, sound.get()) != MA_SUCCESS) {
        return -1;
    }
    int handle = (int)sounds.size();
    sounds.push_back(std::move(sound));
    pathToHandle[path] = handle;
    return handle;
}

void AudioSystem::play(int handle, float volume, float pitch) {
    if (handle < 0 || handle >= (int)sounds.size() || !sounds[handle]) return;
    ma_sound_set_volume(sounds[handle].get(), volume);
    ma_sound_set_pitch(sounds[handle].get(), pitch);
    ma_sound_seek_to_pcm_frame(sounds[handle].get(), 0);
    ma_sound_start(sounds[handle].get());
}

void AudioSystem::stop(int handle) {
    if (handle < 0 || handle >= (int)sounds.size() || !sounds[handle]) return;
    ma_sound_stop(sounds[handle].get());
}