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
    font.openFromFile("C:/Windows/Fonts/arial.ttf");

    fpsText.setFillColor(sf::Color::Yellow);
    fpsText.setPosition({ 10.f, 10.f });
    auto floor = createObjectWithMesh(createBox(0.5f, 0xFF888888), { 0, -2, 5 }, false);
    floor->getComponent<BoxCollider>()->halfExtents = { 10, 0.5f, 10 };
    scene.objects.push_back(std::move(floor));
    scene.objects.push_back(createObjectWithMesh(createBox(0.5f, 0xFFFFFFFF), {0.f, 3.f, 5.f}, true));
    auto camObj = std::make_unique<Object>();
    camObj->transform.position = { 0, 0, 0 };
    auto* cam = camObj->addComponent<Camera>();
    cam->setFov(80.0f * 3.14159265f / 180.0f);
    scene.objects.push_back(std::move(camObj));
	input.setMouseLocked(true, window, sf::Vector2i(window.getSize().x / 2, window.getSize().y / 2 ));
	ImGui::SFML::Init(window);
}

void App::applyMouseLook() {
    Object* cameraObj = scene.findMainCameraObject();  // helper
    if (!cameraObj) return;
    sf::Vector2i md = input.getMouseDelta();
    input.resetMouseDelta();
    cameraObj->transform.yaw += md.x * 0.003f;
    cameraObj->transform.pitch += md.y * 0.003f;
    cameraObj->transform.pitch = std::clamp(cameraObj->transform.pitch, -1.57f, 1.57f);
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
        auto cameraObj = scene.findMainCameraObject();
        if (!cameraObj) {
			auto camObj = std::make_unique<Object>();
			camObj->transform.position = { 0, 0, 0 };
			auto* cam = camObj->addComponent<Camera>();
			cam->setFov(80.0f * 3.14159265f / 180.0f);
			scene.objects.push_back(std::move(camObj));
        }
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
			prevPosition = cameraObj->transform.position;
			prevYaw = cameraObj->transform.yaw;
			prevPitch = cameraObj->transform.pitch;
			applyMouseLook();
			fixedUpdate(fixedDT);
            acc -= fixedDT;
        }

		scene.update(dt);

        bool mouseDown = input.isMouseButtonDown(sf::Mouse::Button::Left);
        bool mousePressed = input.isMouseButtonPressed(sf::Mouse::Button::Left);  // edge: was up, now down
        bool wantUI = ImGui::GetIO().WantCaptureMouse;

        auto mp = input.getMousePosition();
        int mx = mp.x, my = mp.y;
        int sw = window.getSize().x, sh = window.getSize().y;

        if (mousePressed && !wantUI) {
            GizmoAxis axis = editor.pickGizmoAxis(scene, mx, my, sw, sh, cameraObj);
            if (axis != GizmoAxis::None) {
                editor.activeGizmoAxis = axis;
                editor.gizmoLastMousePos = ImVec2((float)mx, (float)my);
            }
            else {
                editor.handleMouseClick(scene, mx, my, sw, sh, cameraObj);
            }
        }

        if (mouseDown && editor.activeGizmoAxis != GizmoAxis::None) {
            editor.updateGizmoDrag(scene, mx, my, sw, sh, cameraObj);
        }

        if (!mouseDown) {
            editor.activeGizmoAxis = GizmoAxis::None;
        }
        ImGui::SFML::Update(window, restart);
		editor.draw(scene);
		editor.drawGizmo(scene, window.getSize().x, window.getSize().y, cameraObj);

		float alpha = acc / fixedDT;

        Transform realTransform = cameraObj->transform;

        cameraObj->transform.position = lerp(prevPosition, realTransform.position, alpha);
        cameraObj->transform.yaw = prevYaw + (realTransform.yaw - prevYaw) * alpha;
        cameraObj->transform.pitch = prevPitch + (realTransform.pitch - prevPitch) * alpha;

        render();

        cameraObj->transform = realTransform;
    }
    ImGui::SFML::Shutdown();
}

void App::fixedUpdate(float dt) {
	auto cameraObj = scene.findMainCameraObject();
    if (!cameraObj) return;
    scene.fixedUpdate(dt);
	const float step = 0.05f;
	Vec3 forward = { std::sinf(-cameraObj->transform.yaw), 0, std::cosf(-cameraObj->transform.yaw) };
	Vec3 right = { forward.z, 0, -forward.x };
	Vec3 move = {};
	if (input.isKeyDown(sf::Keyboard::Scancode::W)) move = move + forward;
	if (input.isKeyDown(sf::Keyboard::Scancode::S)) move = move - forward;
	if (input.isKeyDown(sf::Keyboard::Scancode::A)) move = move - right;
	if (input.isKeyDown(sf::Keyboard::Scancode::D)) move = move + right;

	float len = length(move);
	if (len > 0.0f) cameraObj->transform.position = cameraObj->transform.position + (move / len) * step;

	if (input.isKeyDown(sf::Keyboard::Scancode::E)) cameraObj->transform.position.y += step;
	if (input.isKeyDown(sf::Keyboard::Scancode::Q)) cameraObj->transform.position.y -= step;
}

void App::render() {
	renderer.clear(0xFF7D7D7Du);
    scene.render(renderer);
	renderer.present(window);
	window.draw(fpsText);
	ImGui::SFML::Render(window);
	window.display();
}
