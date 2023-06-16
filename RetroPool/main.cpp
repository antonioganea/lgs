#include <SFML/Graphics.hpp>
#include <iostream>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

sf::Font font;

class Pin;

Pin* hoveredPin = nullptr;

Pin* firstPinSelected = nullptr;

enum class PinType {
    Input,
    Output
};

class Pin {
private:
    sf::RectangleShape shape;
    sf::Vector2f offset;
    
    
public:
    PinType pinType;
    Pin* connectedTo;

    void static onPinClicked(Pin * pin) {
        if (firstPinSelected == nullptr) {
            firstPinSelected = pin;
        }
        else {
            // both pins selected ...
            std::cout << "Both pins selected" << std::endl;

            if (
                (firstPinSelected->pinType == PinType::Input && pin->pinType == PinType::Output) ||
                (firstPinSelected->pinType == PinType::Output && pin->pinType == PinType::Input)
                ) {

                if (firstPinSelected->connectedTo != nullptr) {
                    firstPinSelected->connectedTo->connectedTo = nullptr; //on firstPinSelected unhook
                }

                if (pin->connectedTo != nullptr) {
                    pin->connectedTo->connectedTo = nullptr; // on pin unhook
                }

                firstPinSelected->connectedTo = pin;
                pin->connectedTo = firstPinSelected;
            }
            

            firstPinSelected = nullptr;
        }
    }

    Pin(PinType pt) {
        connectedTo = nullptr;

        shape.setSize(sf::Vector2f(10, 10));
        shape.setFillColor(sf::Color(200, 200, 200));
        shape.setOrigin(5, 5);
        shape.setPosition(-25 + 5, 25 + 5);

        pinType = pt;
    }

    static void drawTempConnection(sf::RenderTarget& target, sf::Vector2f mousePos) {
        if (firstPinSelected == nullptr) { return; }

        sf::Vertex line[2];
        line[0].position = firstPinSelected->getPosition();
        line[0].color = sf::Color(255,127,0);
        line[1].position = mousePos;
        line[1].color = sf::Color(255, 127, 0);

        target.draw(line, 2, sf::Lines);
    }

    void drawConnection(sf::RenderTarget& target) {
        if (connectedTo == nullptr) { return; }

        sf::Vertex line[2];
        line[0].position = shape.getPosition();
        line[0].color = sf::Color::Red;
        line[1].position = connectedTo->getPosition();
        line[1].color = sf::Color::Red;

        target.draw(line, 2, sf::Lines);
    }

    void setOffset(sf::Vector2f off) {
        offset = off;
    }

    sf::Vector2f getPosition() {
        return shape.getPosition();
    }

    void setPosition(sf::Vector2f pos) {
        shape.setPosition(pos + offset);
    }

    void draw(sf::RenderTarget& target) {
        target.draw(shape);

        if (pinType == PinType::Output) {
            drawConnection(target);
        }
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

class Gate {
private:

public:
    virtual bool tryClick(sf::Vector2f pos) = 0;
    virtual bool pinHover(sf::Vector2f pos) = 0;
    virtual void position(sf::Vector2f pos) = 0;
    virtual bool isInBounds(float x, float y) = 0;
    virtual void draw(sf::RenderTarget& target) = 0;
};

class ORGate : public Gate {
private:
    sf::RectangleShape body;
    Pin inputA, inputB, output;
    sf::Text text;
public:
    ORGate() : inputA(PinType::Input), inputB(PinType::Input), output(PinType::Output) {
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

    void draw(sf::RenderTarget& target) {
        target.draw(body);
        inputA.draw(target);
        inputB.draw(target);
        output.draw(target);
        target.draw(text);
    }
};

class ANDGate : public Gate {
private:
    sf::RectangleShape body;
    Pin inputA, inputB, output;
    sf::Text text;
public:
    ANDGate() : inputA(PinType::Input), inputB(PinType::Input), output(PinType::Output) {
        body.setSize(sf::Vector2f(50, 50));
        body.setFillColor(sf::Color(64, 64, 64));
        body.setOrigin(25, 25);

        inputA.setOffset(sf::Vector2f(-25 + 5, 25 + 5));
        inputB.setOffset(sf::Vector2f(+25 - 5, 25 + 5));
        output.setOffset(sf::Vector2f(0, -25 - 5));

        text.setFont(font);
        text.setString("AND");

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
        return body.getGlobalBounds().contains(sf::Vector2f(x, y));
    }

    void draw(sf::RenderTarget& target) {
        target.draw(body);
        inputA.draw(target);
        inputB.draw(target);
        output.draw(target);
        target.draw(text);
    }
};

int main()
{
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Logic Gate Simulator");

    Gate* held = nullptr;


    std::vector<Gate*> gates;

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
                if (event.key.code == sf::Keyboard::D) {
                    auto gate = new ANDGate();
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
            gate->draw(window);
        }

        Pin::drawTempConnection(window, sf::Vector2f(sf::Mouse::getPosition(window)));
        window.display();
    }
    return 0;
}