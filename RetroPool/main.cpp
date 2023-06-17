#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

sf::Font font;

class Pin;

class Gate {
private:

public:
    virtual bool tryClick(sf::Vector2f pos) = 0;
    virtual bool tryRightClick(sf::Vector2f pos) = 0;
    virtual bool pinHover(sf::Vector2f pos) = 0;
    virtual void position(sf::Vector2f pos) = 0;
    virtual bool isInBounds(float x, float y) = 0;
    virtual void draw(sf::RenderTarget& target) = 0;
    virtual void updateState() = 0;
};

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
    Pin* connectedTo; // for input pins
    std::vector<Pin*> outputs; // for output pins
    Gate* inputFor;
    bool cachedState;

    void update(bool state);
    void static connectPins(Pin* A, Pin* B);
    void static onPinClicked(Pin* pin);
    void static onPinRightClicked(Pin* pin);
    void disconnectAll();
    Pin(PinType pt);
    static void drawTempConnection(sf::RenderTarget& target, sf::Vector2f mousePos);
    void drawConnection(sf::RenderTarget& target);
    void setOffset(sf::Vector2f off);
    sf::Vector2f getPosition();
    void setPosition(sf::Vector2f pos);
    void draw(sf::RenderTarget& target);
    bool pinHover(sf::Vector2f pos);
    bool tryClick(sf::Vector2f pos);
    bool tryRightClick(sf::Vector2f pos);
};




struct SimulationUpdate {
public:
    Pin * affectedPin;
    bool newState;
    SimulationUpdate(Pin* pin, bool state)// : affectedPin(pin), newState(state)
    {
        affectedPin = pin;
        newState = state;
    }
};

// not working
#define TRY_RUN_EVERYTHING_ONCE

class Simulation {
private:
    Simulation();

public:
    static std::queue<SimulationUpdate> updateQueue;

#ifdef TRY_RUN_EVERYTHING_ONCE
    static int updatesThisFrame;
#endif

    static void queueUpdate(Pin * pin, bool newState) {
        updateQueue.emplace(pin, newState);

#ifdef TRY_RUN_EVERYTHING_ONCE
        updatesThisFrame++;
#endif

        std::cout << "Size of queue " << updateQueue.size() << std::endl;
    }
    
    static void processTick() {
#ifdef TRY_RUN_EVERYTHING_ONCE
        int i = updatesThisFrame;
        updatesThisFrame = 0;
        for (; i > 0; i--) {
#endif
            if (updateQueue.size() == 0) { return; }
            updateQueue.front().affectedPin->update(updateQueue.front().newState);
            updateQueue.pop();
        }
#ifdef TRY_RUN_EVERYTHING_ONCE
    }
#endif
};
std::queue<SimulationUpdate> Simulation::updateQueue;
int Simulation::updatesThisFrame = 0;














void Pin::update(bool state) {
    if (cachedState == state) { return; }

    cachedState = state;
        
    if (pinType == PinType::Input) {
        if (inputFor != nullptr) {
            inputFor->updateState();// propagate into component
        }
    }
    if (pinType == PinType::Output) {
        for (auto other : outputs) {
            Simulation::queueUpdate(other, state);
        }
    }
}

void Pin::connectPins(Pin* A, Pin* B) {
    if (A->pinType == PinType::Output) {
        std::swap(A, B);
    }

    // A is input, B is output

    A->disconnectAll();

    A->connectedTo = B;

    B->outputs.push_back(A);

    if (B->cachedState) {
        A->update(B->cachedState); // TODO : should it be and immediate ->update on the pin ? or queue an update for later ?
    }
}

void Pin::onPinClicked(Pin * pin) {
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

            /*
            if (firstPinSelected->connectedTo != nullptr) {
                firstPinSelected->connectedTo->connectedTo = nullptr; //on firstPinSelected unhook
            }

            if (pin->connectedTo != nullptr) {
                pin->connectedTo->connectedTo = nullptr; // on pin unhook
            }

            firstPinSelected->connectedTo = pin;
            pin->connectedTo = firstPinSelected;
            */
            connectPins(firstPinSelected, pin);
        }
            

        firstPinSelected = nullptr;
    }
}

void Pin::onPinRightClicked(Pin* pin) {
    pin->disconnectAll();
}

void Pin::disconnectAll() {
    if (pinType == PinType::Input) {
        if (connectedTo == nullptr) { return; }
        connectedTo->outputs.erase(std::remove(connectedTo->outputs.begin(), connectedTo->outputs.end(), this), connectedTo->outputs.end());
        connectedTo = nullptr;
        update(false); // TODO : should it be and immediate ->update on the pin ? or queue an update for later ?
        return;
    }
    if (pinType == PinType::Output) {
        for (auto other : outputs) {
            other->connectedTo = nullptr;
            other->update(false); // TODO : should it be and immediate ->update on the pin ? or queue an update for later ?
        }
        outputs.clear();
        return;
    }
}

Pin::Pin(PinType pt) {
    connectedTo = nullptr;

    shape.setSize(sf::Vector2f(10, 10));
    shape.setFillColor(sf::Color(200, 200, 200));
    shape.setOrigin(5, 5);
    shape.setPosition(-25 + 5, 25 + 5);

    pinType = pt;
}

void Pin::drawTempConnection(sf::RenderTarget& target, sf::Vector2f mousePos) {
    if (firstPinSelected == nullptr) { return; }

    sf::Vertex line[2];
    line[0].position = firstPinSelected->getPosition();
    line[0].color = sf::Color(255,127,0);
    line[1].position = mousePos;
    line[1].color = sf::Color(255, 127, 0);

    target.draw(line, 2, sf::Lines);
}

void Pin::drawConnection(sf::RenderTarget& target) {
        
    // if (connectedTo == nullptr) { return; }

    for (auto pin : outputs) {
        sf::Vertex line[2];
        line[0].position = shape.getPosition();
        line[0].color = sf::Color(3, 127, 252);
        line[1].position = pin->getPosition();
        line[1].color = sf::Color(3, 127, 252);

        target.draw(line, 2, sf::Lines);
    }
}

void Pin::setOffset(sf::Vector2f off) {
    offset = off;
}

sf::Vector2f Pin::getPosition() {
    return shape.getPosition();
}

void Pin::setPosition(sf::Vector2f pos) {
    shape.setPosition(pos + offset);
}

void Pin::draw(sf::RenderTarget& target) {
    target.draw(shape);

    if (pinType == PinType::Output) {
        drawConnection(target);
    }
}

bool Pin::pinHover(sf::Vector2f pos) {
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

bool Pin::tryClick(sf::Vector2f pos) {
    bool truth = shape.getGlobalBounds().contains(pos);

    if (truth) {
        onPinClicked(this);
    }
        
    //std::cout << shape.getGlobalBounds().left << " " << shape.getGlobalBounds().top << "   " << pos.x << " " << pos.y << std::endl;

    return truth;
}

bool Pin::tryRightClick(sf::Vector2f pos) {
    bool truth = shape.getGlobalBounds().contains(pos);

    if (truth) {
        onPinRightClicked(this);
    }

    //std::cout << shape.getGlobalBounds().left << " " << shape.getGlobalBounds().top << "   " << pos.x << " " << pos.y << std::endl;

    return truth;
}



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

        inputA.inputFor = this;
        inputB.inputFor = this;
    }

    void updateState() {
        output.update(inputA.cachedState || inputB.cachedState);
    }

    bool tryClick(sf::Vector2f pos) {
        return inputA.tryClick(pos) || inputB.tryClick(pos) || output.tryClick(pos);
    }

    bool tryRightClick(sf::Vector2f pos) {
        return inputA.tryRightClick(pos) || inputB.tryRightClick(pos) || output.tryRightClick(pos);
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

        inputA.inputFor = this;
        inputB.inputFor = this;
    }

    void updateState() {
        output.update(inputA.cachedState && inputB.cachedState);
    }

    bool tryClick(sf::Vector2f pos) {
        return inputA.tryClick(pos) || inputB.tryClick(pos) || output.tryClick(pos);
    }

    bool tryRightClick(sf::Vector2f pos) {
        return inputA.tryRightClick(pos) || inputB.tryRightClick(pos) || output.tryRightClick(pos);
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

class Switch : public Gate {
private:
    sf::RectangleShape body;
    sf::RectangleShape indicator;
    Pin output;
    sf::Text text;
    bool state = false;
public:

    static Switch* clickedOn;

    Switch() : output(PinType::Output) {
        body.setSize(sf::Vector2f(50, 50));
        body.setFillColor(sf::Color(64, 64, 64));
        body.setOrigin(25, 25);

        output.setOffset(sf::Vector2f(0, -25 - 5));

        text.setFont(font);
        text.setString("Switch");
        text.setCharacterSize(12);

        indicator.setSize(sf::Vector2f(5, 5));
        indicator.setFillColor(sf::Color::Red);
        indicator.setOrigin(2.5f, 2.5f + 10.f);

        position(sf::Vector2f(0, 0));
    }

    void updateState() {
        // ...
    }

    void toggle() {
        std::cout << "TOGGLED SWITCH" << std::endl;

        state = !state;

        indicator.setFillColor(state ? sf::Color::Green : sf::Color::Red);

        output.update(state);
    }

    bool tryClick(sf::Vector2f pos) {
        return output.tryClick(pos);
    }

    bool tryRightClick(sf::Vector2f pos) {
        return output.tryRightClick(pos);
    }

    bool pinHover(sf::Vector2f pos) {
        return output.pinHover(pos);
    }

    void position(sf::Vector2f pos) {
        body.setPosition(pos);
        output.setPosition(pos);
        indicator.setPosition(pos);

        sf::FloatRect textRect = text.getGlobalBounds();
        text.setPosition(pos - sf::Vector2f(textRect.width, textRect.height) / 2.0f);
    }

    bool isInBounds(float x, float y) {
        return body.getGlobalBounds().contains(sf::Vector2f(x, y));
    }

    void draw(sf::RenderTarget& target) {
        target.draw(body);
        output.draw(target);
        target.draw(indicator);
        target.draw(text);
    }
};
Switch* Switch::clickedOn = nullptr;


class Light : public Gate {
private:
    sf::RectangleShape body;
    sf::RectangleShape indicator;
    Pin input;
    sf::Text text;
    bool state = false;
public:

    static Switch* clickedOn;

    Light() : input(PinType::Input) {
        body.setSize(sf::Vector2f(50, 50));
        body.setFillColor(sf::Color(64, 64, 64));
        body.setOrigin(25, 25);

        input.setOffset(sf::Vector2f(0, 25 + 5));

        text.setFont(font);
        text.setString("Light");
        text.setCharacterSize(12);

        indicator.setSize(sf::Vector2f(5, 5));
        indicator.setFillColor(sf::Color::Red);
        indicator.setOrigin(2.5f, 2.5f + 10.f);

        position(sf::Vector2f(0, 0));

        input.inputFor = this;
    }

    void updateState() {
        indicator.setFillColor(input.cachedState ? sf::Color::Green : sf::Color::Red);
    }

    bool tryClick(sf::Vector2f pos) {
        return input.tryClick(pos);
    }

    bool tryRightClick(sf::Vector2f pos) {
        return input.tryRightClick(pos);
    }

    bool pinHover(sf::Vector2f pos) {
        return input.pinHover(pos);
    }

    void position(sf::Vector2f pos) {
        body.setPosition(pos);
        input.setPosition(pos);
        indicator.setPosition(pos);

        sf::FloatRect textRect = text.getGlobalBounds();
        text.setPosition(pos - sf::Vector2f(textRect.width, textRect.height) / 2.0f);
    }

    bool isInBounds(float x, float y) {
        return body.getGlobalBounds().contains(sf::Vector2f(x, y));
    }

    void draw(sf::RenderTarget& target) {
        target.draw(body);
        input.draw(target);
        target.draw(indicator);
        target.draw(text);
    }
};

class NOTGate : public Gate {
private:
    sf::RectangleShape body;
    Pin input, output;
    sf::Text text;
public:
    NOTGate() : input(PinType::Input), output(PinType::Output) {
        body.setSize(sf::Vector2f(50, 50));
        body.setFillColor(sf::Color(64, 64, 64));
        body.setOrigin(25, 25);

        input.setOffset(sf::Vector2f(0, 25 + 5));
        output.setOffset(sf::Vector2f(0, -25 - 5));

        text.setFont(font);
        text.setString("NOT");

        position(sf::Vector2f(0, 0));

        input.inputFor = this;
        output.update(true);
    }

    void updateState() {
        output.update(!input.cachedState);
    }

    bool tryClick(sf::Vector2f pos) {
        return input.tryClick(pos) || output.tryClick(pos);
    }

    bool tryRightClick(sf::Vector2f pos) {
        return input.tryRightClick(pos)|| output.tryRightClick(pos);
    }

    bool pinHover(sf::Vector2f pos) {
        return input.pinHover(pos) || output.pinHover(pos);
    }

    void position(sf::Vector2f pos) {
        body.setPosition(pos);
        input.setPosition(pos);
        output.setPosition(pos);

        sf::FloatRect textRect = text.getGlobalBounds();
        text.setPosition(pos - sf::Vector2f(textRect.width, textRect.height) / 2.0f);
    }

    bool isInBounds(float x, float y) {
        return body.getGlobalBounds().contains(sf::Vector2f(x, y));
    }

    void draw(sf::RenderTarget& target) {
        target.draw(body);
        input.draw(target);
        output.draw(target);
        target.draw(text);
    }
};

class XORGate : public Gate {
private:
    sf::RectangleShape body;
    Pin inputA, inputB, output;
    sf::Text text;
public:
    XORGate() : inputA(PinType::Input), inputB(PinType::Input), output(PinType::Output) {
        body.setSize(sf::Vector2f(50, 50));
        body.setFillColor(sf::Color(64, 64, 64));
        body.setOrigin(25, 25);

        inputA.setOffset(sf::Vector2f(-25 + 5, 25 + 5));
        inputB.setOffset(sf::Vector2f(+25 - 5, 25 + 5));
        output.setOffset(sf::Vector2f(0, -25 - 5));

        text.setFont(font);
        text.setString("XOR");

        position(sf::Vector2f(0, 0));

        inputA.inputFor = this;
        inputB.inputFor = this;
    }

    void updateState() {
        output.update(inputA.cachedState != inputB.cachedState);
    }

    bool tryClick(sf::Vector2f pos) {
        return inputA.tryClick(pos) || inputB.tryClick(pos) || output.tryClick(pos);
    }

    bool tryRightClick(sf::Vector2f pos) {
        return inputA.tryRightClick(pos) || inputB.tryRightClick(pos) || output.tryRightClick(pos);
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
    window.setFramerateLimit(60);

    Gate* held = nullptr;


    std::vector<Gate*> gates;

    auto starterGate = new ORGate();
    starterGate->position(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT)/2.0f);
    gates.push_back(starterGate);

    font.loadFromFile("assets/roboto.ttf"); // TODO : better font loading

    sf::Color clearColor = sf::Color(32, 32, 32);

    sf::View board(sf::Vector2f(0, 0), sf::Vector2f(600, 600));
    board.setViewport(sf::FloatRect(0,0,0.5,1));

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
                if (event.key.code == sf::Keyboard::E) {
                    auto gate = new Switch();
                    gate->position(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT) / 2.0f);
                    gates.push_back(gate);
                }
                if (event.key.code == sf::Keyboard::R) {
                    auto gate = new Light();
                    gate->position(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT) / 2.0f);
                    gates.push_back(gate);
                }
                if (event.key.code == sf::Keyboard::W) {
                    auto gate = new NOTGate();
                    gate->position(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT) / 2.0f);
                    gates.push_back(gate);
                }
                if (event.key.code == sf::Keyboard::S) {
                    auto gate = new XORGate();
                    gate->position(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT) / 2.0f);
                    gates.push_back(gate);
                }
            }

            if (event.type == event.MouseButtonPressed) {
                for (auto gate : gates) {
                    if (gate->isInBounds(event.mouseButton.x, event.mouseButton.y)) {
                        std::cout << "Click";
                        held = gate;

                        Switch* sw = dynamic_cast<Switch*>(gate);

                        if (sw != nullptr) {
                            Switch::clickedOn = sw;
                        }

                        break;
                    }

                    if (event.mouseButton.button == sf::Mouse::Left) {
                        if (gate->tryClick(sf::Vector2f(event.mouseButton.x, event.mouseButton.y))) {
                            std::cout << "Pin click" << std::endl;

                            break;
                        }
                    }
                    else if (event.mouseButton.button == sf::Mouse::Right) {
                        firstPinSelected = nullptr;
                        if (gate->tryRightClick(sf::Vector2f(event.mouseButton.x, event.mouseButton.y))) {
                            std::cout << "Pin right click" << std::endl;

                            break;
                        }
                    }

                }
            }

            if (event.type == event.MouseButtonReleased) {
                held = nullptr;

                if (Switch::clickedOn != nullptr) {
                    Switch::clickedOn->toggle();
                    Switch::clickedOn = nullptr;
                }
            }

            if (event.type == event.MouseMoved) {

                Switch::clickedOn = nullptr;

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

        Simulation::processTick();

        window.clear(clearColor);
        //window.setView(board);
        for (auto gate : gates) {
            gate->draw(window);
        }

        Pin::drawTempConnection(window, sf::Vector2f(sf::Mouse::getPosition(window)));
        window.display();
    }
    return 0;
}