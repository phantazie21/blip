#include "Renderer.h"

Renderer::Renderer(int width, int height) : width(width), height(height), texture(sf::Vector2u(width, height)), sprite(texture), numThreads(std::max(1u, std::thread::hardware_concurrency())), syncStart(numThreads + 1), syncEnd(numThreads + 1) {
	drawCalls.reserve(10000);
	for (int i = 0; i < numThreads; i++) {
		workers.emplace_back(&Renderer::workerLoop, this, i);
	}
	framebuffer = new uint32_t[width * height];
	depthBuffer = new float[width * height];
	idBuffer = new uint32_t[width * height];
	assert(texture.getNativeHandle() != 0);
}

Renderer::~Renderer() {
	running = false;
	syncStart.arrive_and_wait();
	for (std::thread& t : workers) t.join();
	delete[] framebuffer;
	delete[] depthBuffer;
}

void Renderer::clear(uint32_t color) {
	std::fill(framebuffer, framebuffer + width * height, color);
	std::fill(depthBuffer, depthBuffer + width * height, 0.0f);
	std::fill(idBuffer, idBuffer + width * height, 0);
	nextSubmitId = 0;
}

void Renderer::present(sf::RenderWindow& window) {
	texture.update((const uint8_t*)framebuffer);
	window.draw(sprite);
}

void Renderer::workerLoop(int t) {
	int bandH = height / numThreads;
	int yMin = t * bandH;
	int yMax = (t == numThreads - 1) ? height - 1 : yMin + bandH - 1;

	while (true) {
		syncStart.arrive_and_wait();
		if (!running) return;

		for (const auto& dc : drawCalls) {
			drawTriangle(*this, dc.a, dc.b, dc.c,
				dc.uv0, dc.uv1, dc.uv2,
				dc.texture, dc.color,
				dc.i0, dc.i1, dc.i2,
				yMin, yMax, dc.submitId);
		}

		syncEnd.arrive_and_wait();
	}
}

void Renderer::submitDrawCall(const DrawCall& dc) {
	DrawCall dcCopy = dc;
	dcCopy.submitId = nextSubmitId++;
	drawCalls.push_back(dcCopy);
}

void Renderer::flush() {
	syncStart.arrive_and_wait();
	syncEnd.arrive_and_wait();
	drawCalls.clear();
}