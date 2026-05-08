#include "Input.h"

void Input::beginFrame() {
	previous = current;
	//mouseDelta = { 0, 0 };
}

void Input::setMouseLocked(bool locked, sf::RenderWindow& window, sf::Vector2i center) {
	mouseLocked = locked;
	window.setMouseCursorVisible(!mouseLocked);
	if (mouseLocked) {
		windowCenter = center;
		sf::Mouse::setPosition(windowCenter, window);
	}
}

bool Input::isKeyDown(sf::Keyboard::Scancode key) const {
	auto it = current.find(key);
	return it != current.end() && it->second;
}

bool Input::isKeyPressed(sf::Keyboard::Scancode key) const {
	auto it = current.find(key);
	auto prevIt = previous.find(key);
	return (it != current.end() && it->second) && !(prevIt != previous.end() && prevIt->second);
}

bool Input::isKeyReleased(sf::Keyboard::Scancode key) const {
	auto it = current.find(key);
	auto prevIt = previous.find(key);
	return !(it != current.end() && it->second) && (prevIt != previous.end() && prevIt->second);
}

bool Input::isMouseLocked() const {
	return mouseLocked;
}

void Input::resetMouseDelta() { mouseDelta = { 0, 0 }; }

bool Input::isMouseButtonDown(sf::Mouse::Button button) const {
	return sf::Mouse::isButtonPressed(button);
}

void Input::onEvent(const std::optional<sf::Event>& event, sf::RenderWindow& window) {
	if (event->is<sf::Event::Closed>())
	{
		shouldCloseFlag = true;
	}
	else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
	{
		current[keyPressed->scancode] = true;
	}
	else if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>())
	{
		current[keyReleased->scancode] = false;
	}
	else if (event->is<sf::Event::MouseMoved>())
	{
		if (mouseLocked) {
			mouseDelta -= sf::Mouse::getPosition(window) - windowCenter;
			sf::Mouse::setPosition(windowCenter, window);
		}
		else {
			mousePosition = sf::Mouse::getPosition(window);
		}
	}
}