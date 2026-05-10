#pragma once
#include <cstdint>
#include <SFML/Graphics.hpp>
#include <thread>
#include <barrier>
#include <vector>
#include "Rasterizer.h"

class Renderer {
public:
    int numThreads;
    int width, height;
    uint32_t* framebuffer;
    float* depthBuffer;
	uint32_t* idBuffer;
	uint32_t nextSubmitId = 0;

    std::vector<DrawCall> drawCalls;

    void submitDrawCall(const DrawCall& drawcall);
    void flush();

    Renderer(int width, int height);
    ~Renderer();

    void clear(uint32_t color);
    void present(sf::RenderWindow& window);

    inline int index(int x, int y) const { return y * width + x; }

private:
    std::vector<std::thread> workers;
    std::barrier<> syncStart;
    std::barrier<> syncEnd;
    bool running = true;

    void workerLoop(int threadIdx);
    sf::Texture texture;
    sf::Sprite sprite;
};
