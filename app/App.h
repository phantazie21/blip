#pragma once
#include "../input/Input.h"
#include "../renderer/Renderer.h"
#include "../scene/Scene.h"
#include "../editor/Editor.h"

class App {
public:
    App();
    void run();
private:
    sf::RenderWindow window;
    Renderer         renderer;
    Scene            scene;
	Editor           editor;
    Input            input;
    sf::Font         font;
    sf::Text         fpsText;
    float            acc = 0.0f;
    sf::Clock        clock;
    Vec3             prevPosition;
	float 		     prevYaw = 0.0f, prevPitch = 0.0f;

    void handleEvents();
    void fixedUpdate(float dt);
    void render();
    void applyMouseLook();
	void setVSync(bool enabled);
};
