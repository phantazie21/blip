#pragma once
#include <SFML/Window.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <unordered_map>
class Input {
public:
    Input() = default;
    void beginFrame();           // call once per frame before polling
    void onEvent(const std::optional<sf::Event>& event, sf::RenderWindow& window);

    bool isKeyDown(sf::Keyboard::Scancode key) const;
    bool isKeyPressed(sf::Keyboard::Scancode key) const;  // true only on first frame
    bool isKeyReleased(sf::Keyboard::Scancode key) const;

    bool isMouseButtonDown(sf::Mouse::Button button) const;
    bool isMouseLocked() const;
    void setMouseLocked(bool locked, sf::RenderWindow& window, sf::Vector2i center);
	sf::Vector2i getMouseDelta() const { return mouseDelta; }
	sf::Vector2i getMousePosition() const { return mousePosition; }
	bool shouldClose() const { return shouldCloseFlag; }
    void resetMouseDelta();

private:
    std::unordered_map<sf::Keyboard::Scancode, bool> current;
    std::unordered_map<sf::Keyboard::Scancode, bool> previous;
    sf::Vector2i  windowCenter;
    bool          mouseLocked = false;
    sf::Vector2i mouseDelta;
	sf::Vector2i mousePosition;
	bool 		shouldCloseFlag = false;
};
