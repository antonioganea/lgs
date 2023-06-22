#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <map>
#include <fstream>
#include <stack>
#include <set>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#define PIN_SIZE 10

sf::Font font;

class Pin;
class Gate;

enum class PinType {
    Input,
    Output
};

enum class GateType {
    OR,
    AND,
    NOT,
    XOR,
    SWITCH,
    LIGHT,
    INTEGRATED
};

Pin* hoveredPin = nullptr;

Pin* firstPinSelected = nullptr;

class Pin {
private:
    sf::RectangleShape shape;
    sf::Vector2f offset;

public:
    PinType pinType;
    Pin* connectedTo; // for input pins
    std::vector<Pin*> outputs; // for output pins
    Gate* parentGate;
    bool cachedState;

    void update(bool state);
    void static connectPins(Pin* A, Pin* B);
    void static onPinClicked(Pin* pin);
    void static onPinRightClicked(Pin* pin);
    void disconnectAll();
    Pin();
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

class Gate {
private:

public:
    virtual bool tryClick(sf::Vector2f pos) = 0;
    virtual bool tryRightClick(sf::Vector2f pos) = 0;
    virtual bool pinHover(sf::Vector2f pos) = 0;
    virtual void position(sf::Vector2f pos) = 0;
    virtual bool isInBounds(float x, float y) = 0;
    virtual void draw(sf::RenderTarget& target) = 0;
    virtual void updateState(Pin * updatedPin) = 0;
    virtual sf::Vector2f getPosition() = 0;
    virtual GateType getGateType() = 0;

    virtual int getInputPinCount() = 0;
    virtual int getOutputPinCount() = 0;

    virtual Pin* getInputPins() = 0;
    virtual Pin* getOutputPins() = 0;

    virtual bool getPinIndex(Pin* pin, PinType pt, int& outIndex) {
        if (pt == PinType::Input) {
            int c = getInputPinCount();
            Pin* pins = getInputPins();
            for (int i = 0; i < c; i++) {
                if ((pins+i) == pin) {
                    outIndex = i;
                    return true;
                }
            }
        }
        else if (pt == PinType::Output) {
            int c = getOutputPinCount();
            Pin* pins = getOutputPins();
            for (int i = 0; i < c; i++) {
                if ((pins+i) == pin) {
                    outIndex = i;
                    return true;
                }
            }
        }

        return false;
    };

    virtual Pin* getPinByIndex(PinType pt, int index) {
        if (pt == PinType::Input) {
            int c = getInputPinCount();
            Pin* pins = getInputPins();
            
            if (index >= 0 && index < c) {
                return pins + index;
            }
        }
        else if (pt == PinType::Output) {
            int c = getOutputPinCount();
            Pin* pins = getOutputPins();
            if (index >= 0 && index < c) {
                return pins + index;
            }
        }

        return nullptr;
    }

    //virtual Pin** serializeInputs() = 0;
    //virtual void deserializeInputs(Pin** pins) = 0;
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
        if (parentGate != nullptr) {
            parentGate->updateState(this);// propagate into component
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

Pin::Pin() : Pin(PinType::Input) {
    //std::cout << "Default constructor";
}

Pin::Pin(PinType pt) {
    connectedTo = nullptr;

    shape.setSize(sf::Vector2f(PIN_SIZE, PIN_SIZE));
    shape.setFillColor(sf::Color(200, 200, 200));
    shape.setOrigin(PIN_SIZE/2.0f, PIN_SIZE/2.0f);
    shape.setPosition(-25 + 5, 25 + 5);

    pinType = pt;

    cachedState = false;
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

    union {
        struct {
            Pin inputA, inputB;
        };
        Pin inputPins[2];
    };

    union {
        Pin output;
        Pin outputPins[1];
    };

    sf::Text text;
public:
    ORGate() : inputA(PinType::Input), inputB(PinType::Input), output(PinType::Output) {
        body.setSize(sf::Vector2f(50, 50));
        body.setFillColor(sf::Color(64, 64, 64));
        body.setOrigin(25, 25);

        inputA.setOffset(sf::Vector2f(-25 + 5, 25 + 5));
        inputB.setOffset(sf::Vector2f(+25 - 5, 25 + 5));
        output.setOffset(sf::Vector2f(0, -25 - 5));

        text.setFont(font);
        text.setString("OR");

        position(sf::Vector2f(0, 0));

        inputA.parentGate = this;
        inputB.parentGate = this;
        output.parentGate = this;
    }

    void updateState(Pin* updatedPin) {
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

    sf::Vector2f getPosition() {
        return body.getPosition();
    }

    GateType getGateType() {
        return GateType::OR;
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

    int getInputPinCount() {
        return 2;
    }

    int getOutputPinCount() {
        return 1;
    }

    Pin* getInputPins() {
        return inputPins;
    }

    Pin* getOutputPins() {
        return outputPins;
    }
};

class ANDGate : public Gate {
private:
    sf::RectangleShape body;

    union {
        struct {
            Pin inputA, inputB;
        };
        Pin inputPins[2];
    };

    union {
        Pin output;
        Pin outputPins[1];
    };

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

        inputA.parentGate = this;
        inputB.parentGate = this;
        output.parentGate = this;
    }

    void updateState(Pin* updatedPin) {
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

    sf::Vector2f getPosition() {
        return body.getPosition();
    }

    GateType getGateType() {
        return GateType::AND;
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

    int getInputPinCount() {
        return 2;
    }

    int getOutputPinCount() {
        return 1;
    }

    Pin* getInputPins() {
        return inputPins;
    }

    Pin* getOutputPins() {
        return outputPins;
    }
};

class Switch : public Gate {
private:
    sf::RectangleShape body;
    sf::RectangleShape indicator;

    union {
        Pin output;
        Pin outputPins[1];
    };

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

        output.parentGate = this;
    }

    void updateState(Pin* updatedPin) {
        // ...
    }

    void toggle() {
        std::cout << "TOGGLED SWITCH" << std::endl;

        state = !state;

        indicator.setFillColor(state ? sf::Color::Green : sf::Color::Red);

        output.update(state);
    }

    void setState(bool _state) {
        if (state != _state) {
            toggle();
        }
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

    sf::Vector2f getPosition() {
        return body.getPosition();
    }

    GateType getGateType() {
        return GateType::SWITCH;
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

    int getInputPinCount() {
        return 0;
    }

    int getOutputPinCount() {
        return 1;
    }

    Pin* getInputPins() {
        return nullptr;
    }

    Pin* getOutputPins() {
        return outputPins;
    }
};
Switch* Switch::clickedOn = nullptr;


class Light : public Gate {
private:
    sf::RectangleShape body;
    sf::RectangleShape indicator;
    union {
        Pin input;
        Pin inputPins[1];
    };

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

        input.parentGate = this;
    }

    void updateState(Pin* updatedPin) {
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

    sf::Vector2f getPosition() {
        return body.getPosition();
    }
    GateType getGateType() {
        return GateType::LIGHT;
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

    int getInputPinCount() {
        return 1;
    }

    int getOutputPinCount() {
        return 0;
    }

    Pin* getInputPins() {
        return inputPins;
    }

    Pin* getOutputPins() {
        return nullptr;
    }
};

class NOTGate : public Gate {
private:
    sf::RectangleShape body;

    union {
        struct {
            Pin input;
        };
        Pin inputPins[1];
    };

    union {
        Pin output;
        Pin outputPins[1];
    };

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

        input.parentGate = this;
        output.parentGate = this;
        output.update(true);
    }

    void updateState(Pin* updatedPin) {
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

    sf::Vector2f getPosition() {
        return body.getPosition();
    }

    GateType getGateType() {
        return GateType::NOT;
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

    int getInputPinCount() {
        return 1;
    }

    int getOutputPinCount() {
        return 1;
    }

    Pin* getInputPins() {
        return inputPins;
    }

    Pin* getOutputPins() {
        return outputPins;
    }
};

class XORGate : public Gate {
private:
    sf::RectangleShape body;

    union {
        struct {
            Pin inputA, inputB;
        };
        Pin inputPins[2];
    };

    union {
        Pin output;
        Pin outputPins[1];
    };

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

        inputA.parentGate = this;
        inputB.parentGate = this;
        output.parentGate = this;
    }

    void updateState(Pin* updatedPin) {
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

    sf::Vector2f getPosition() {
        return body.getPosition();
    }

    GateType getGateType() {
        return GateType::XOR;
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

    int getInputPinCount() {
        return 2;
    }

    int getOutputPinCount() {
        return 1;
    }

    Pin* getInputPins() {
        return inputPins;
    }

    Pin* getOutputPins() {
        return outputPins;
    }
};





/*
class CircuitGraph {
private:

public:
    CircuitGraph() {

    }
};
*/



class IntegratedChip : public Gate {
private:
    sf::RectangleShape body;

    Pin * inputPins, * outputPins;
    int inputPinCount, outputPinCount;

    sf::Text text;

    std::map<Pin*, Switch*> inputToSwitches;

public:
    std::vector<Gate*>* circuit;

    static void getCircuitIOCount(const std::vector<Gate*> & circuit, int & inputs, int & outputs) {
        inputs = 0;
        outputs = 0;
        for (auto gate : circuit) {
            Switch* sw = dynamic_cast<Switch*>(gate);

            if (sw != nullptr) {
                inputs++;
                continue;
            }

            Light* lt = dynamic_cast<Light*>(gate);

            if (lt != nullptr) {
                outputs++;
                continue;
            }
        }
    }

    IntegratedChip(std::vector<Gate*> * _circuit) {

        circuit = _circuit;

        // : inputA(PinType::Input), inputB(PinType::Input), output(PinType::Output)

        IntegratedChip::getCircuitIOCount(*circuit, inputPinCount, outputPinCount);

        inputPins = new Pin[inputPinCount];
        outputPins = new Pin[outputPinCount];

        for (int i = 0; i < outputPinCount; i++) {
            outputPins[i].pinType = PinType::Output;
        }

        sf::Vector2f bodySize(50, 50);
        bodySize.x += PIN_SIZE * inputPinCount; // TODO : make this better

        body.setSize(bodySize);
        body.setFillColor(sf::Color(64, 64, 64));
        body.setOrigin(bodySize/2.0f);

        //std::cout << "integrated chip : " << inputPinCount << " " << outputPinCount << std::endl;

        for (int i = 0; i < inputPinCount; i++) {
            inputPins[i].setOffset(sf::Vector2f((i-inputPinCount/2.0f) * PIN_SIZE * 2.0f + PIN_SIZE, bodySize.y / 2.0f + PIN_SIZE / 2.0f));
        }

        for (int i = 0; i < outputPinCount; i++) {
            outputPins[i].setOffset(sf::Vector2f((i - outputPinCount / 2.0f) * PIN_SIZE * 2.0f + PIN_SIZE, -(bodySize.y / 2.0f + PIN_SIZE / 2.0f) ));
        }

        //inputA.setOffset(sf::Vector2f(-25 + 5, 25 + 5));
        //inputB.setOffset(sf::Vector2f(+25 - 5, 25 + 5));
        //output.setOffset(sf::Vector2f(0, -25 - 5));

        text.setFont(font);
        text.setString("CHIP");

        position(sf::Vector2f(0, 0));

        for (int i = 0; i < inputPinCount; i++) {
            inputPins[i].parentGate = this;
        }
        for (int i = 0; i < outputPinCount; i++) {
            outputPins[i].parentGate = this;
        }




        int iterator = 0;
        // Linking inputs
        for (auto gate : *circuit) {
            Switch* sw = dynamic_cast<Switch*>(gate);

            if (sw != nullptr) {
                inputToSwitches[inputPins+iterator] = sw;
                //sw->setState(false); // not necessary but a good move to set all switches to false upon IC creation
                iterator++;
            }
        }

        iterator = 0;
        // Linking outputs
        for (auto gate : *circuit) {
            Light* lt = dynamic_cast<Light*>(gate);

            if (lt != nullptr) {
                Pin * p = lt->getInputPins();
                Pin * o = p->connectedTo;

                Pin::connectPins(o, outputPins + iterator);
                iterator++;
            }
        }
    }

    void updateState(Pin* updatedPin) {
        Switch* sw = inputToSwitches[updatedPin];

        Pin* switchOutput = sw->getOutputPins();

        for (auto p : switchOutput->outputs) {
            p->update(updatedPin->cachedState);
        }

        //output.update(inputA.cachedState != inputB.cachedState);
    }

    bool tryClick(sf::Vector2f pos) {
        //return inputA.tryClick(pos) || inputB.tryClick(pos) || output.tryClick(pos);
        for (int i = 0; i < inputPinCount; i++) {
            if (inputPins[i].tryClick(pos)) { return true; }
        }
        for (int i = 0; i < outputPinCount; i++) {
            if (outputPins[i].tryClick(pos)) { return true; }
        }

        return false;
    }

    bool tryRightClick(sf::Vector2f pos) {
        //return inputA.tryRightClick(pos) || inputB.tryRightClick(pos) || output.tryRightClick(pos);
        for (int i = 0; i < inputPinCount; i++) {
            if (inputPins[i].tryRightClick(pos)) { return true; }
        }
        for (int i = 0; i < outputPinCount; i++) {
            if (outputPins[i].tryRightClick(pos)) { return true; }
        }

        return false;
    }

    bool pinHover(sf::Vector2f pos) {
        //return inputA.pinHover(pos) || inputB.pinHover(pos) || output.pinHover(pos);
        for (int i = 0; i < inputPinCount; i++) {
            if (inputPins[i].pinHover(pos)) { return true; }
        }
        for (int i = 0; i < outputPinCount; i++) {
            if (outputPins[i].pinHover(pos)) { return true; }
        }

        return false;
    }

    void position(sf::Vector2f pos) {
        body.setPosition(pos);

        for (int i = 0; i < inputPinCount; i++) {
            inputPins[i].setPosition(pos);
        }
        for (int i = 0; i < outputPinCount; i++) {
            outputPins[i].setPosition(pos);
        }

        sf::FloatRect textRect = text.getGlobalBounds();
        text.setPosition(pos - sf::Vector2f(textRect.width, textRect.height) / 2.0f);
    }

    sf::Vector2f getPosition() {
        return body.getPosition();
    }

    GateType getGateType() {
        return GateType::INTEGRATED;
    }

    bool isInBounds(float x, float y) {
        return body.getGlobalBounds().contains(sf::Vector2f(x, y));
    }

    void draw(sf::RenderTarget& target) {
        target.draw(body);

        for (int i = 0; i < inputPinCount; i++) {
            inputPins[i].draw(target);
        }
        for (int i = 0; i < outputPinCount; i++) {
            outputPins[i].draw(target);
        }

        target.draw(text);
    }

    int getInputPinCount() {
        return inputPinCount;
    }

    int getOutputPinCount() {
        return outputPinCount;
    }

    Pin* getInputPins() {
        return inputPins;
    }

    Pin* getOutputPins() {
        return outputPins;
    }
};





typedef std::vector<Gate*>* CircuitPtr;


// should fix the fact that this returns a memory-leaky pointer to a vector
std::vector<CircuitPtr>* topoSort(const CircuitPtr circuit) {
    std::vector<CircuitPtr> * circuitList = new std::vector<CircuitPtr>;

    std::stack<CircuitPtr> stack;

    stack.push(circuit);

    std::set<CircuitPtr> serializedCircuits;

    while (!stack.empty()) {

        CircuitPtr top = stack.top(); stack.pop();

        bool foundNotSerializedYet = false; // not used
        for (Gate* gate : *top) {
            IntegratedChip* ic = dynamic_cast<IntegratedChip*>(gate);
            if (ic != nullptr) {
                if (serializedCircuits.find(ic->circuit) != serializedCircuits.end()) { // if it was already serialized

                }
                else { // if it wasn't previously serialized
                    stack.push(ic->circuit);
                    
                    foundNotSerializedYet = true;
                }
            }
        }
        if (foundNotSerializedYet) {

        }

        serializedCircuits.insert(top);
        circuitList->push_back(top);
    }

    std::reverse(circuitList->begin(), circuitList->end());

    return circuitList;
}


int getIndex(std::vector<CircuitPtr>* v, CircuitPtr K)
{
    auto it = std::find(v->begin(), v->end(), K);

    // If element was found
    if (it != v->end())
    {

        // calculating the index
        // of K
        int index = it - v->begin();
        return index;
    }
    else {
        return -1;
    }
}



Gate* newGateOfType(GateType type) {
    switch (type) {
        case GateType::OR:
            return new ORGate();
        case GateType::AND:
            return new ANDGate();
        case GateType::NOT:
            return new NOTGate();
        case GateType::XOR:
            return new XORGate();
        case GateType::SWITCH:
            return new Switch();
        case GateType::LIGHT:
            return new Light();
    }

    return nullptr;
}

void loadFromFile(std::vector<Gate*>& gates, std::ifstream& inputStream, std::vector<CircuitPtr>* circuitList = nullptr) {
    int gateCount;

    inputStream >> gateCount;

    std::map<int, Gate*> gatesByIndex;

    for (int i = 0; i < gateCount; i++) {
        int gateID; // note : really, can get rid of this because all the id's are in incrementing order 0, 1, 2 ...
        int gateType;
        sf::Vector2f position;

        inputStream >> gateID >> gateType;
        
        int circuitID = -1;
        if (gateType == (int)GateType::INTEGRATED){
            inputStream >> circuitID >> position.x >> position.y;
        }
        else {
            inputStream >> position.x >> position.y;
        }
        
        
        std::cout << gateID << " " << gateType << " " << circuitID << " " << position.x << " " << position.y;

        Gate* newGate;

        if (circuitID != -1) {
            newGate = new IntegratedChip((*circuitList)[circuitID]);
        }
        else {
            newGate = newGateOfType((GateType)gateType);
        }

        
        newGate->position(position);

        gatesByIndex[gateID] = newGate;

        gates.push_back(newGate);
    }

    int connectionCount;

    inputStream >> connectionCount;

    for (int i = 0; i < connectionCount; i++) {
        // gate A -> gate B

        int gateA, gateB, pinA, pinB;

        inputStream >> gateB >> pinB >> gateA >> pinA;

        Pin* outputPin = gatesByIndex[gateA]->getPinByIndex(PinType::Output, pinA);
        Pin* inputPin = gatesByIndex[gateB]->getPinByIndex(PinType::Input, pinB);

        Pin::connectPins(outputPin, inputPin);
    }
}

void saveToFile(const std::vector<Gate*> & gates, std::ostream & outputStream, std::vector<CircuitPtr>* circuitList = nullptr) {
    //std::cout << "Listing pieces" << std::endl;

    std::map<Gate*, int> gateNumbers;

    int counter = 0;
    int totalConnections = 0;
    for (auto gate : gates) {
        gateNumbers[gate] = counter;
        counter++;

        int c = gate->getInputPinCount();
        Pin* pins = gate->getInputPins();
        for (int i = 0; i < c; i++) {
            if (pins[i].connectedTo != nullptr) {
                totalConnections++;
            }
        }
    }

    outputStream << counter << std::endl;

    for (auto gate : gates) {
        auto pos = gate->getPosition();
        //std::cout << "Gate ID : " << gateNumbers[gate] << " type : " << (int)gate->getGateType() << " position : " << pos.x << " " << pos.y << std::endl;

        IntegratedChip* ic = dynamic_cast<IntegratedChip*>(gate);
        if (ic != nullptr) {
            int circuitIndex = getIndex(circuitList, ic->circuit);
            if (circuitIndex == -1) {
                std::cout << "ERROR : index is -1 on lookup circuits" << std::endl;
            }
            outputStream << gateNumbers[gate] << " " << (int)gate->getGateType() << " " << circuitIndex << " " << pos.x << " " << pos.y << std::endl;
        }
        else {
            outputStream << gateNumbers[gate] << " " << (int)gate->getGateType() << " " << pos.x << " " << pos.y << std::endl;
        }
    }

    //std::cout << "Total connections " << totalConnections << std::endl;
    outputStream << totalConnections << std::endl;

    for (auto gate : gates) {
        int c = gate->getInputPinCount();
        Pin* pins = gate->getInputPins();
        for (int i = 0; i < c; i++) {
            Pin* outputPin = pins[i].connectedTo;
            if (outputPin != nullptr) {
                int tempIndex = 0;
                if (outputPin->parentGate->getPinIndex(outputPin, PinType::Output, tempIndex)) {
                    //std::cout << gateNumbers[gate] << " " << i << " " << gateNumbers[outputPin->parentGate] << " " << tempIndex << std::endl;
                    outputStream << gateNumbers[gate] << " " << i << " " << gateNumbers[outputPin->parentGate] << " " << tempIndex << std::endl;
                }
                else {
                    std::cerr << "======FAIL TO REVERSE LOOKUP PIN INDEX========" << std::endl;
                }
            }
        }
    }

    //std::cout << "End of listing" << std::endl;
}



void saveToFileRecursively(std::vector<Gate*>& gates, std::ostream& outputStream) {


    // const std::vector<Gate*>*

    auto list = topoSort(&gates);

    /*
    std::cout << "Performed topo sort on " << &gates << std::endl;

    for (CircuitPtr circuit : *list) {
        std::cout << circuit << std::endl;
    }
    */


    outputStream << list->size() << std::endl; // or capacity ??

    int counter = 0;

    std::cout << "Performed topo sort on " << &gates << std::endl;
    for (CircuitPtr circuit : *list) {
        std::cout << "Serializing circuit " << circuit << std::endl;
        outputStream << counter << std::endl;
        saveToFile(*circuit, outputStream, list);
        counter++;
    }


    delete list;

}

void loadFromFileRecursively(std::vector<Gate*>& gates, std::ifstream& inputStream) {
    int circuitCount;

    inputStream >> circuitCount;

    std::vector<CircuitPtr>* circuitList = new std::vector<CircuitPtr>;

    for (int currentCircuitID = 0; currentCircuitID < circuitCount; currentCircuitID++) {
        int circuitID;
        inputStream >> circuitID;

        if (currentCircuitID == circuitCount - 1) { // if the last one
            loadFromFile(gates, inputStream, circuitList);
        }
        else {
            std::vector<Gate*>* newCircuit = new std::vector<Gate*>();
            loadFromFile(*newCircuit, inputStream, circuitList);
            circuitList->push_back(newCircuit);
        }
    }
}

int main()
{
    //std::cout << "making new pin" << std::endl;
    //Pin mypin;
    //std::cout << "making new pin ---- end" << std::endl;




    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Logic Gate Simulator");
    window.setFramerateLimit(60);

    Gate* held = nullptr;


    std::vector<Gate*> gates;

    //auto starterGate = new ORGate();
    //starterGate->position(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT)/2.0f);
    //gates.push_back(starterGate);

    font.loadFromFile("assets/roboto.ttf"); // TODO : better font loading

    sf::Color clearColor = sf::Color(32, 32, 32);

    sf::View board(sf::Vector2f(0, 0), sf::Vector2f(600, 600));
    board.setViewport(sf::FloatRect(0,0,0.5,1));

    //std::cout << window.getSettings().majorVersion << "." << window.getSettings().majorVersion << std::endl;

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
                if (event.key.code == sf::Keyboard::S && !event.key.control) {
                    auto gate = new XORGate();
                    gate->position(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT) / 2.0f);
                    gates.push_back(gate);
                }
                if (event.key.code == sf::Keyboard::U && !event.key.control) {

                    std::vector<Gate*>* internalCircuit = new std::vector<Gate*>();

                    std::ifstream ifs("4-bit-adder.txt", std::ifstream::in);

                    loadFromFileRecursively(*internalCircuit, ifs);

                    ifs.close();

                    auto gate = new IntegratedChip(internalCircuit);
                    gate->position(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT) / 2.0f);
                    gates.push_back(gate);
                }
                if (event.key.code == sf::Keyboard::K) {
                    auto list = topoSort(&gates);

                    std::cout << "Performed topo sort on " << &gates << std::endl;

                    for (CircuitPtr circuit : *list) {
                        std::cout << circuit << std::endl;
                    }

                    delete list;
                }
                if (event.key.code == sf::Keyboard::Q) {
                    std::cout << "Loading Full Adder circuit..." << std::endl;

                    //gates.clear(); //causes memory leak

                    std::ifstream ifs("full-adder.txt", std::ifstream::in);

                    std::vector<Gate*> * fullBitAdderCircuit = new std::vector<Gate*>(); // memory leak if not deleted elsewhere

                    loadFromFile(*fullBitAdderCircuit, ifs);

                    ifs.close();

                    std::cout << "Loaded!" << std::endl;

                    std::cout << "Creating integrated circuit" << std::endl;

                    auto gate = new IntegratedChip(fullBitAdderCircuit);
                    gate->position(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT) / 2.0f);
                    gates.push_back(gate);

                    std::cout << "Done creating integrated circuit" << std::endl;
                }
                if (event.key.code == sf::Keyboard::S && event.key.control) {
                    std::cout << "Saving ..." << std::endl;
                    
                    std::ofstream ofs("saveFile.txt", std::ofstream::out);
                    saveToFile(gates, ofs);
                    ofs.close();

                    std::cout << "Saved!" << std::endl;
                }
                if (event.key.code == sf::Keyboard::M && event.key.control) {
                    std::cout << "Saving recursively ..." << std::endl;

                    std::ofstream ofs("saveFile-rec.txt", std::ofstream::out);
                    saveToFileRecursively(gates, ofs);
                    ofs.close();

                    std::cout << "Saved!" << std::endl;
                }
                if (event.key.code == sf::Keyboard::B && event.key.control) {

                    std::cout << "Loading recursively ..." << std::endl;

                    std::ifstream ifs("saveFile-rec.txt", std::ifstream::in);

                    loadFromFileRecursively(gates, ifs);

                    ifs.close();

                    std::cout << "Loaded!" << std::endl;
                }
                if (event.key.code == sf::Keyboard::O && event.key.control) {
                    std::cout << "Loading ..." << std::endl;

                    //gates.clear(); //causes memory leak

                    std::ifstream ifs("saveFile.txt", std::ifstream::in);

                    loadFromFile(gates, ifs);

                    ifs.close();

                    std::cout << "Loaded!" << std::endl;
                }
                if (event.key.code == sf::Keyboard::N && event.key.control) {
                    for (auto gate : gates) {
                        delete gate;
                    }
                    gates.clear();
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