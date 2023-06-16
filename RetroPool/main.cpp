#include <SFML/Graphics.hpp>
#include <iostream>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

sf::Font font;

class Pin;

Pin* hoveredPin = nullptr;

Pin* firstPinSelected = nullptr;

class Pin : public sf::Drawable {
private:
    sf::RectangleShape shape;
    sf::Vector2f offset;
public:
    void static onPinClicked(Pin * pin) {
        if (firstPinSelected == nullptr) {
            firstPinSelected = pin;
        }
        else {
            // both pins selected ...
            std::cout << "Both pins selected and hooked" << std::endl;
            firstPinSelected = nullptr;
        }
    }

    Pin() {
        shape.setSize(sf::Vector2f(10, 10));
        shape.setFillColor(sf::Color(200, 200, 200));
        shape.setOrigin(5, 5);
        shape.setPosition(-25 + 5, 25 + 5);
    }

    void setOffset(sf::Vector2f off) {
        offset = off;
    }

    void setPosition(sf::Vector2f pos) {
        shape.setPosition(pos + offset);
    }

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const {
        target.draw(shape);
    }

    bool pinHover(sf::Vector2f pos) {
        bool truth = shape.getGlobalBounds().contains(pos);

        //std::cout << shape.getGlobalBounds().left << " " << shape.getGlobalBounds().top << "   " << pos.x << " " << pos.y << std::endl;

        if (truth) {
            shape.setFillColor(sf::Color::Green);
            hoveredPin = this;
        }
        else {
            shape.setFillColor(sf::Color(200, 250, 200));
        }

        return truth;
    }

    bool tryClick(sf::Vector2f pos) {
        bool truth = shape.getGlobalBounds().contains(pos);

        if (truth) {
            onPinClicked(this);
        }
        
        //std::cout << shape.getGlobalBounds().left << " " << shape.getGlobalBounds().top << "   " << pos.x << " " << pos.y << std::endl;

        return truth;
    }
};

class ORGate : public sf::Drawable {
private:
    sf::RectangleShape body;
    Pin inputA, inputB, output;
    sf::Text text;
public:
    ORGate() {
        body.setSize(sf::Vector2f(50, 50));
        body.setFillColor(sf::Color(64,64,64));
        body.setOrigin(25, 25);

        inputA.setOffset(sf::Vector2f( - 25 + 5, 25 + 5));
        inputB.setOffset(sf::Vector2f(+25 - 5, 25 + 5));
        output.setOffset(sf::Vector2f(0, -25 - 5));

        text.setFont(font);
        text.setString("OR");

        position(sf::Vector2f(0, 0));
    }

    bool tryClick(sf::Vector2f pos) {
        return inputA.tryClick(pos) || inputB.tryClick(pos) || output.tryClick(pos);
    }

    bool pinHover(sf::Vector2f pos) {
        return inputA.pinHover(pos) || inputB.pinHover(pos) || output.pinHover(pos);
    }

    void position(sf::Vector2f pos) {
        body.setPosition(pos);
        inputA.setPosition(pos);
        inputB.setPosition(pos);
        output.setPosition(pos);

        sf::FloatRect textRect = text.getGlobalBounds();
        text.setPosition(pos - sf::Vector2f(textRect.width, textRect.height) / 2.0f);
    }

    bool isInBounds(float x, float y) {
        return body.getGlobalBounds().contains(sf::Vector2f(x,y));
    }

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const {
        target.draw(body);
        target.draw(inputA);
        target.draw(inputB);
        target.draw(output);
        target.draw(text);
    }
};

int main()
{
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Logic Gate Simulator");

    ORGate* held = nullptr;


    std::vector<ORGate*> gates;

    gates.push_back(new ORGate());

    font.loadFromFile("assets/roboto.ttf"); // TODO : better font loading

    sf::Color clearColor = sf::Color(32, 32, 32);

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == event.Closed) {
                window.close();
            }

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::F) {
                    auto gate = new ORGate();
                    gate->position(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT) / 2.0f);
                    gates.push_back(gate);
                }
            }

            if (event.type == event.MouseButtonPressed) {
                for (auto gate : gates) {
                    if (gate->isInBounds(event.mouseButton.x, event.mouseButton.y)) {
                        std::cout << "Click";
                        held = gate;

                        break;
                    }

                    if (gate->tryClick(sf::Vector2f(event.mouseButton.x, event.mouseButton.y))) {
                        std::cout << "Pin click" << std::endl;

                        break;
                    }
                }
            }

            if (event.type == event.MouseButtonReleased) {
                held = nullptr;
            }

            if (event.type == event.MouseMoved) {
                if (held != nullptr) {
                    held->position(sf::Vector2f(event.mouseMove.x, event.mouseMove.y));
                }
                else {
                    hoveredPin = nullptr;
                    for (auto gate : gates) {
                        if (gate->pinHover(sf::Vector2f(event.mouseMove.x, event.mouseMove.y))) {
                            break;
                        }
                    }
                }
            }
        }

        window.clear(clearColor);
        for (auto gate : gates) {
            window.draw(*gate);
        }
        window.display();
    }
    return 0;
}