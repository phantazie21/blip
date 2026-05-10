#pragma once
#include <string>
#include <unordered_map>
#include <memory>

struct ma_engine;
struct ma_sound;

class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    int loadSound(const std::string& path);

    void play(int handle, float volume = 1.0f, float pitch = 1.0f);
    void stop(int handle);

private:
    std::unique_ptr<ma_engine> engine;
    std::vector<std::unique_ptr<ma_sound>> sounds;
    std::unordered_map<std::string, int> pathToHandle;
};
