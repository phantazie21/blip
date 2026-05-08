#include "App.h"
#include "../scene/Primitives.h"
#include <iostream>

App::App()
    : window(sf::VideoMode::getFullscreenModes().at(0), "Software Renderer", sf::State::Fullscreen),
    renderer(window.getSize().x, window.getSize().y),
	fpsText(font, "", 20)
{
    camera.setFov(80.0f * 3.14159265f / 180.0f);
    font.openFromFile("C:/Windows/Fonts/arial.ttf");

    fpsText.setFillColor(sf::Color::Yellow);
    fpsText.setPosition({ 10.f, 10.f });
    scene.objects.push_back(createObjectWithMesh(createBox(0.5f, 0xFFFFFFFF)));
	input.setMouseLocked(true, window, sf::Vector2i(window.getSize().x / 2, window.getSize().y / 2 ));
}

void App::applyMouseLook() {
    sf::Vector2i md = input.getMouseDelta();
    input.resetMouseDelta();
    camera.yaw += md.x * 0.003f;
    camera.pitch += md.y * 0.003f;
    camera.pitch = std::clamp(camera.pitch, -1.57f, 1.57f);
}

void App::setVSync(bool enabled) {
    window.setVerticalSyncEnabled(enabled);
    if (!enabled) window.setFramerateLimit(0);
}

void App::run() {
    sf::Vector2i windowCenter(window.getSize().x / 2, window.getSize().y / 2);
    const float fixedDT = 1.0f / 60.0f;

    int frameCount = 0;
    float fpsTimer = 0.0f;
    float displayedFps = 0.0f;

    //setVSync(true);

    while (window.isOpen() && !input.shouldClose()) {
        input.beginFrame();
        float dt = clock.restart().asSeconds();
		if (dt > 0.25f) dt = 0.25f;
        acc += dt;
        fpsTimer += dt;
        frameCount++;
        if (fpsTimer >= 0.5f) {
            displayedFps = frameCount / fpsTimer;
            fpsText.setString("FPS: " + std::to_string((int)displayedFps));
            frameCount = 0;
            fpsTimer = 0.0f;
        }
        while (const std::optional event = window.pollEvent())
			input.onEvent(event, window);

		if (input.isKeyDown(sf::Keyboard::Scancode::Escape)) {
            window.close();
            break;
        }

        while (acc >= fixedDT) {
			prevPosition = camera.position;
			prevYaw = camera.yaw;
			prevPitch = camera.pitch;
			applyMouseLook();
			fixedUpdate(fixedDT);
            acc -= fixedDT;
        }
		float alpha = acc / fixedDT;

		Camera renderCamera = camera;
		renderCamera.position = lerp(prevPosition, camera.position, alpha);
		renderCamera.yaw = prevYaw + (camera.yaw - prevYaw) * alpha;
		renderCamera.pitch = prevPitch + (camera.pitch - prevPitch) * alpha;

        render(renderCamera);
    }
}

void App::fixedUpdate(float dt) {
	const float step = 0.05f;
	Vec3 forward = { std::sinf(-camera.yaw), 0, std::cosf(-camera.yaw) };
	Vec3 right = { forward.z, 0, -forward.x };
	Vec3 move = {};
	if (input.isKeyDown(sf::Keyboard::Scancode::W)) move = move + forward;
	if (input.isKeyDown(sf::Keyboard::Scancode::S)) move = move - forward;
	if (input.isKeyDown(sf::Keyboard::Scancode::A)) move = move - right;
	if (input.isKeyDown(sf::Keyboard::Scancode::D)) move = move + right;

	float len = length(move);
	if (len > 0.0f) camera.position = camera.position + (move / len) * step;

	if (input.isKeyDown(sf::Keyboard::Scancode::E)) camera.position.y += step;
	if (input.isKeyDown(sf::Keyboard::Scancode::Q)) camera.position.y -= step;
}

void App::render(Camera currentCamera) {
	renderer.clear(0xFF7D7D7Du);
	scene.render(renderer, currentCamera);
	renderer.present(window);
	window.draw(fpsText);
	window.display();
}
