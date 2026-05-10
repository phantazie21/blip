#include "App.h"
#include "../scene/Primitives.h"
#include <iostream>
#include "imgui.h"
#include "imgui-SFML.h"

App::App()
    : window(sf::VideoMode::getFullscreenModes().at(0), "Software Renderer", sf::State::Fullscreen),
    renderer(window.getSize().x, window.getSize().y),
	fpsText(font, "", 20),
    prevPosition({ 0, 0, 0 })
{
    camera.setFov(80.0f * 3.14159265f / 180.0f);
    font.openFromFile("C:/Windows/Fonts/arial.ttf");

    fpsText.setFillColor(sf::Color::Yellow);
    fpsText.setPosition({ 10.f, 10.f });
    auto floor = createObjectWithMesh(createBox(0.5f, 0xFF888888), { 0, -2, 5 }, false);
    floor->getComponent<BoxCollider>()->halfExtents = { 10, 0.5f, 10 };
    scene.objects.push_back(std::move(floor));
    scene.objects.push_back(createObjectWithMesh(createBox(0.5f, 0xFFFFFFFF), {0.f, 3.f, 5.f}, true));
	input.setMouseLocked(true, window, sf::Vector2i(window.getSize().x / 2, window.getSize().y / 2 ));
	ImGui::SFML::Init(window);
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
		sf::Time restart = clock.restart();
        float dt = restart.asSeconds();
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
        while (const std::optional event = window.pollEvent()) {
            input.onEvent(event, window);
			ImGui::SFML::ProcessEvent(window, *event);
        }

		if (input.isKeyPressed(sf::Keyboard::Scancode::Escape)) {
			input.setMouseLocked(!input.isMouseLocked(), window, windowCenter);
        }

        while (acc >= fixedDT) {
			prevPosition = camera.position;
			prevYaw = camera.yaw;
			prevPitch = camera.pitch;
			applyMouseLook();
			fixedUpdate(fixedDT);
            acc -= fixedDT;
        }

        ImGui::SFML::Update(window, restart);
		editor.draw(scene);

		float alpha = acc / fixedDT;

		Camera renderCamera = camera;
		renderCamera.position = lerp(prevPosition, camera.position, alpha);
		renderCamera.yaw = prevYaw + (camera.yaw - prevYaw) * alpha;
		renderCamera.pitch = prevPitch + (camera.pitch - prevPitch) * alpha;

        render(renderCamera);
    }
    ImGui::SFML::Shutdown();
}

void App::fixedUpdate(float dt) {
    scene.fixedUpdate(dt);
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
	ImGui::SFML::Render(window);
	window.display();
}
