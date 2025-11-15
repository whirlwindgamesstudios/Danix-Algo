#pragma once
#include <string>
#include <vector>
#include <memory>
#include <imgui.h>
#include <nlohmann/json.hpp>
#include "../../GUI/Color.h"

class BlueprintManager;
class BasicNode;
class PinIn;
class PinOut;

enum class NodeHeaderColor {
    DEFAULT,          
    EXECUTION,         
    MATH,                
    LOGIC,             
    CONVERSION,        
    CONSTANT,         
    VARIABLE,         
    CHART,              
    ARRAY,            
    FLOW               
};

enum class PinType {
    EXEC,
    INT,
    FLOAT,
    DOUBLE,
    BOOL,
    STRING,
    ARRAY_INT,
    ARRAY_FLOAT,
    ARRAY_DOUBLE,
    ARRAY_BOOL,
    ARRAY_STRING
};


class Pin {
public:
    std::string pin_guid;
    std::string owner_guid;
    PinType type;
    std::string name;
    BasicNode* owner;
    ImVec2 position;
    std::string description;

    Pin(const std::string& id, const std::string& og, PinType t, const std::string& n)
        : pin_guid(id), owner_guid(og), type(t), name(n), owner(nullptr), position(0, 0) {
    }

    virtual ~Pin() = default;
};

class PinIn : public Pin {
public:
    std::string linked_to_guid;
    PinOut* linked_to;

    float stored_float = 0.0f;
    bool stored_bool = false;
    int stored_int = 0;
    std::string stored_string = "";
    long double stored_double = 0.0L;

    float default_stored_float = 0.0f;
    bool default_stored_bool = false;
    int default_stored_int = 0;
    std::string default_stored_string = "";
    long double default_stored_double = 0.0L;
    std::vector<float>* array_default_stored_float;
    std::vector<bool>* array_default_stored_bool;
    std::vector<int>* array_default_stored_int;
    std::vector<std::string>* array_default_stored_string;
    std::vector<long double>* array_default_stored_double;

    std::vector<float>* array_stored_float;
    std::vector<bool>* array_stored_bool;
    std::vector<int>* array_stored_int;
    std::vector<std::string>* array_stored_string;
    std::vector<long double>* array_stored_double;

    PinIn(const std::string& guid, const std::string& og, PinType t, const std::string& n)
        : Pin(guid, og, t, n), linked_to(nullptr), stored_float(0.0f), stored_bool(false), stored_int(0), stored_string(""), stored_double(0.0L) {
    }

    template<typename T>
    T getValue();

    template<typename T>
    T getDefaultValue();

    template<typename T>
    void setValue(T value);

    template<typename T>
    void setDefaultValue(T value);
};

class PinOut : public Pin {
public:
    std::vector<std::string> connected_pin_guids;
    std::vector<PinIn*> connected_pins;

    float stored_float;
    bool stored_bool;
    int stored_int;
    std::string stored_string;
    long double stored_double;
    std::vector<float>* array_stored_float;
    std::vector<bool>* array_stored_bool;
    std::vector<int>* array_stored_int;
    std::vector<std::string>* array_stored_string;
    std::vector<long double>* array_stored_double;

    PinOut(const std::string& guid, const std::string& og, PinType t, const std::string& n)
        : Pin(guid, og, t, n), stored_float(0.0f), stored_bool(false), stored_int(0), stored_string(""), stored_double(0.0L) {
    }

    template<typename T>
    void setValue(T value) {
        if constexpr (std::is_same_v<T, bool>) {
            stored_bool = value;
        }
        else if constexpr (std::is_same_v<T, float>) {
            stored_float = value;
        }
        else if constexpr (std::is_same_v<T, int>) {
            stored_int = value;
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            stored_string = value;
        }
        else if constexpr (std::is_same_v<T, long double>) {
            stored_double = value;
        }
        else if constexpr (std::is_same_v<T, std::vector<float>*>) {
            array_stored_float = value;
        }
        else if constexpr (std::is_same_v<T, std::vector<bool>*>) {
            array_stored_bool = value;
        }
        else if constexpr (std::is_same_v<T, std::vector<int>*>) {
            array_stored_int = value;
        }
        else if constexpr (std::is_same_v<T, std::vector<std::string>*>) {
            array_stored_string = value;
        }
        else if constexpr (std::is_same_v<T, std::vector<long double>*>) {
            array_stored_double = value;
        }
        propagateValue();
    }

    template<typename T>
    T getValue() {
        if constexpr (std::is_same_v<T, bool>) {
            return stored_bool;
        }
        else if constexpr (std::is_same_v<T, float>) {
            return stored_float;
        }
        else if constexpr (std::is_same_v<T, int>) {
            return stored_int;
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            return stored_string;
        }
        else if constexpr (std::is_same_v<T, long double>) {
            return stored_double;
        }
        else if constexpr (std::is_same_v<T, std::vector<float>*>) {
            return array_stored_float;
        }
        else if constexpr (std::is_same_v<T, std::vector<bool>*>) {
            return array_stored_bool;
        }
        else if constexpr (std::is_same_v<T, std::vector<int>*>) {
            return array_stored_int;
        }
        else if constexpr (std::is_same_v<T, std::vector<std::string>*>) {
            return array_stored_string;
        }
        else if constexpr (std::is_same_v<T, std::vector<long double>*>) {
            return array_stored_double;
        }
        return T{};
    }

    void propagateValue() {
        for (auto* connected_pin : connected_pins) {
            if (type == PinType::BOOL) {
                connected_pin->setValue(stored_bool);
            }
            else if (type == PinType::FLOAT) {
                connected_pin->setValue(stored_float);
            }
            else if (type == PinType::INT) {
                connected_pin->setValue(stored_int);
            }
            else if (type == PinType::STRING) {
                connected_pin->setValue(stored_string);
            }
            else if (type == PinType::DOUBLE) {
                connected_pin->setValue(stored_double);
            }
            if (type == PinType::ARRAY_BOOL) {
                connected_pin->setValue(array_stored_bool);
            }
            else if (type == PinType::ARRAY_FLOAT) {
                connected_pin->setValue(array_stored_float);
            }
            else if (type == PinType::ARRAY_INT) {
                connected_pin->setValue(array_stored_int);
            }
            else if (type == PinType::ARRAY_STRING) {
                connected_pin->setValue(array_stored_string);
            }
            else if (type == PinType::ARRAY_DOUBLE) {
                connected_pin->setValue(array_stored_double);
            }
        }
    }
};

class BasicNode {
public:
    std::string node_guid;
    std::string name;
    std::vector<std::unique_ptr<PinIn>> inputs;
    std::vector<std::unique_ptr<PinOut>> outputs;
    ImVec2 position;
    ImVec2 size;
    bool selected;

    BlueprintManager* blueprintManager;

    std::string description;
    BasicNode(const std::string& guid, const std::string& n, BlueprintManager* dManager = nullptr)
        : node_guid(guid), name(n), blueprintManager(dManager), position(0, 0), size(150, 100), selected(false) {
    }

    virtual ~BasicNode() = default;
    virtual void execute() = 0;
    virtual bool isPure() const = 0;
    virtual bool isLoop() const = 0;
    virtual std::string getNodeType() const = 0;

    ImVec2 getInputPinPos(int index) const {
        return ImVec2(position.x + 8, position.y + 35 + index * 25);
    }

    ImVec2 getOutputPinPos(int index) const {
        return ImVec2(position.x + size.x - 8, position.y + 35 + index * 25);
    }

    bool isClicked(ImVec2 mouse_pos) const {
        return mouse_pos.x >= position.x && mouse_pos.x <= position.x + size.x &&
            mouse_pos.y >= position.y && mouse_pos.y <= position.y + size.y;
    }

    PinIn* getInputPin(int index) {
        if (index >= 0 && index < inputs.size()) {
            return inputs[index].get();
        }
        return nullptr;
    }

    PinOut* getOutputPin(int index) {
        if (index >= 0 && index < outputs.size()) {
            return outputs[index].get();
        }
        return nullptr;
    }

    virtual void drawNodeToDrawList(ImDrawList* draw_list);
    virtual void drawPinsToDrawList(ImDrawList* draw_list);
    virtual void BeginNodeDraw(const std::string& title, ImU32 bg_color = IM_COL32(20, 20, 20, 255));
    virtual void EndNodeDraw(std::map<int, bool> not_show_inpin_names_map = {}, std::map<int, bool> not_show_outpin_names_map = {});
    virtual void drawPins(std::vector<bool> not_show_inpin_names = {}, std::vector<bool> not_show_outpin_names = {});
    virtual nlohmann::json serializeData() const { return nlohmann::json{}; }
    virtual void deserializeData(const nlohmann::json& data) {}
    ImU32 getHeaderColorValue(NodeHeaderColor color) const;
    virtual NodeHeaderColor getHeaderColor() const {
        return NodeHeaderColor::DEFAULT;
    }
};

struct Variable {
    std::string name;
    std::string guid;
    PinType type;
    bool global = false;
    int intValue;
    float floatValue;
    long double doubleValue;
    bool boolValue;
    std::string stringValue;

    std::vector<int> arrayIntValue;
    std::vector<float> arrayFloatValue;
    std::vector<long double> arrayDoubleValue;
    std::vector<bool> arrayBoolValue;
    std::vector<std::string> arrayStringValue;

    int def_intValue = 0;
    float def_floatValue = 0.0f;
    long double def_doubleValue = 0.0L;
    bool def_boolValue = false;
    std::string def_stringValue = "";

    std::vector<int> def_arrayIntValue;
    std::vector<float> def_arrayFloatValue;
    std::vector<long double> def_arrayDoubleValue;
    std::vector<bool> def_arrayBoolValue;
    std::vector<std::string> def_arrayStringValue;

    Variable(const std::string& n, const std::string& g, PinType t, bool globalActive)
        : name(n), guid(g), type(t),global(globalActive), intValue(0), floatValue(0.0f),
        doubleValue(0.0L), boolValue(false), stringValue("") {
    }

    template<typename T>
    T getValue() const {
        if constexpr (std::is_same_v<T, int>) {
            return intValue;
        }
        else if constexpr (std::is_same_v<T, float>) {
            return floatValue;
        }
        else if constexpr (std::is_same_v<T, long double>) {
            return doubleValue;
        }
        else if constexpr (std::is_same_v<T, bool>) {
            return boolValue;
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            return stringValue;
        }
        else if constexpr (std::is_same_v<T, std::vector<int>*>) {
            return arrayIntValue;
        }
        else if constexpr (std::is_same_v<T, std::vector<float>*>) {
            return arrayFloatValue;
        }
        else if constexpr (std::is_same_v<T, std::vector<long double>*>) {
            return arrayDoubleValue;
        }
        else if constexpr (std::is_same_v<T, std::vector<bool>*>) {
            return arrayBoolValue;
        }
        else if constexpr (std::is_same_v<T, std::vector<std::string>*>) {
            return arrayStringValue;
        }
        return T{};
    }

    template<typename T>
    void setValue(const T& value) {
        if constexpr (std::is_same_v<T, int>) {
            intValue = value;
        }
        else if constexpr (std::is_same_v<T, float>) {
            floatValue = value;
        }
        else if constexpr (std::is_same_v<T, long double>) {
            doubleValue = value;
        }
        else if constexpr (std::is_same_v<T, bool>) {
            boolValue = value;
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            stringValue = value;
        }
        else if constexpr (std::is_same_v<T, std::vector<int>*>) {
            arrayIntValue = value;
        }
        else if constexpr (std::is_same_v<T, std::vector<float>*>) {
            arrayFloatValue = value;
        }
        else if constexpr (std::is_same_v<T, std::vector<long double>*>) {
            arrayDoubleValue = value;
        }
        else if constexpr (std::is_same_v<T, std::vector<bool>*>) {
            arrayBoolValue = value;
        }
        else if constexpr (std::is_same_v<T, std::vector<std::string>*>) {
            arrayStringValue = value;
        }
    }

    void setDefaultValue() {
        switch (type) {
        case PinType::INT:
            intValue = def_intValue;
            break;
        case PinType::FLOAT:
            floatValue = def_floatValue;
            break;
        case PinType::DOUBLE:
            doubleValue = def_doubleValue;
            break;
        case PinType::BOOL:
            boolValue = def_boolValue;
            break;
        case PinType::STRING:
            stringValue = def_stringValue;
            break;
        case PinType::ARRAY_INT:
            arrayIntValue.clear();
            break;
        case PinType::ARRAY_FLOAT:
            arrayFloatValue.clear();
            break;
        case PinType::ARRAY_DOUBLE:
            arrayDoubleValue.clear();
            break;
        case PinType::ARRAY_BOOL:
            arrayBoolValue.clear();
            break;
        case PinType::ARRAY_STRING:
            arrayStringValue.clear();
            break;
        }
    }
};

template<typename T>
T PinIn::getValue() {
    if (linked_to && linked_to->owner) {   

        if(!linked_to->owner->isLoop() && linked_to->owner->isPure()) linked_to->owner->execute();

        if constexpr (std::is_same_v<T, bool>) {
            return linked_to->stored_bool;
        }
        else if constexpr (std::is_same_v<T, float>) {
            return linked_to->stored_float;
        }
        else if constexpr (std::is_same_v<T, int>) {
            return linked_to->stored_int;
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            return linked_to->stored_string;
        }
        else if constexpr (std::is_same_v<T, long double>) {
            return linked_to->stored_double;
        }
        if constexpr (std::is_same_v<T, std::vector<bool>*>) {
            return linked_to->array_stored_bool;
        }
        if constexpr (std::is_same_v<T, std::vector<float>*>) {
            return linked_to->array_stored_float;
        }
        if constexpr (std::is_same_v<T, std::vector<int>*>) {
            return linked_to->array_stored_int;
        }
        if constexpr (std::is_same_v<T, std::vector<std::string>*>) {
            return linked_to->array_stored_string;
        }
        if constexpr (std::is_same_v<T, std::vector<long double>*>) {
            return linked_to->array_stored_double;
        }
    }

    if constexpr (std::is_same_v<T, bool>) {
        return default_stored_bool;
    }
    else if constexpr (std::is_same_v<T, float>) {
        return default_stored_float;
    }
    else if constexpr (std::is_same_v<T, int>) {
        return default_stored_int;
    }
    else if constexpr (std::is_same_v<T, std::string>) {
        return default_stored_string;
    }
    else if constexpr (std::is_same_v<T, long double>) {
        return default_stored_double;
    }
    if constexpr (std::is_same_v<T, std::vector<bool>*>) {
        return array_default_stored_bool;
    }
    if constexpr (std::is_same_v<T, std::vector<float>*>) {
        return array_default_stored_float;
    }
    if constexpr (std::is_same_v<T, std::vector<int>*>) {
        return array_default_stored_int;
    }
    if constexpr (std::is_same_v<T, std::vector<std::string>*>) {
        return array_default_stored_string;
    }
    if constexpr (std::is_same_v<T, std::vector<long double>*>) {
        return array_default_stored_double;
    }
   
    return T{};
}

template<typename T>
T PinIn::getDefaultValue() {

    if constexpr (std::is_same_v<T, bool>) {
        return default_stored_bool;
    }
    else if constexpr (std::is_same_v<T, float>) {
        return default_stored_float;
    }
    else if constexpr (std::is_same_v<T, int>) {
        return default_stored_int;
    }
    else if constexpr (std::is_same_v<T, std::string>) {
        return default_stored_string;
    }
    else if constexpr (std::is_same_v<T, long double>) {
        return default_stored_double;
    }
    if constexpr (std::is_same_v<T, std::vector<bool>*>) {
        return array_default_stored_bool;
    }
    if constexpr (std::is_same_v<T, std::vector<float>*>) {
        return array_default_stored_float;
    }
    if constexpr (std::is_same_v<T, std::vector<int>*>) {
        return array_default_stored_int;
    }
    if constexpr (std::is_same_v<T, std::vector<std::string>*>) {
        return array_default_stored_string;
    }
    if constexpr (std::is_same_v<T, std::vector<long double>*>) {
        return array_default_stored_double;
    }

    return T{};
}

template<typename T>
void PinIn::setValue(T value) {
    if constexpr (std::is_same_v<T, bool>) {
        stored_bool = value;
    }
    else if constexpr (std::is_same_v<T, float>) {
        stored_float = value;
    }
    else if constexpr (std::is_same_v<T, int>) {
        stored_int = value;
    }
    else if constexpr (std::is_same_v<T, std::string>) {
        stored_string = value;
    }
    else if constexpr (std::is_same_v<T, long double>) {
        stored_double = value;
    }
    if constexpr (std::is_same_v<T, std::vector<bool>*>) {
        array_stored_bool = value;
    }
    if constexpr (std::is_same_v<T, std::vector<float>*>) {
        array_stored_float = value;
    }
    if constexpr (std::is_same_v<T, std::vector<int>*>) {
        array_stored_int = value;
    }
    if constexpr (std::is_same_v<T, std::vector<std::string>*>) {
        array_stored_string = value;
    }
    if constexpr (std::is_same_v<T, std::vector<long double>*>) {
        array_stored_double = value;
    }
}

template<typename T>
void PinIn::setDefaultValue(T value) {
    if constexpr (std::is_same_v<T, bool>) {
        default_stored_bool = value;
    }
    else if constexpr (std::is_same_v<T, float>) {
        default_stored_float = value;
    }
    else if constexpr (std::is_same_v<T, int>) {
        default_stored_int = value;
    }
    else if constexpr (std::is_same_v<T, std::string>) {
        default_stored_string = value;
    }
    else if constexpr (std::is_same_v<T, long double>) {
        default_stored_double = value;
    }
    if constexpr (std::is_same_v<T, std::vector<bool>*>) {
        array_default_stored_bool = value;
    }
    if constexpr (std::is_same_v<T, std::vector<float>*>) {
        array_default_stored_float = value;
    }
    if constexpr (std::is_same_v<T, std::vector<int>*>) {
        array_default_stored_int = value;
    }
    if constexpr (std::is_same_v<T, std::vector<std::string>*>) {
        array_default_stored_string = value;
    }
    if constexpr (std::is_same_v<T, std::vector<long double>*>) {
        array_default_stored_double = value;
    }
}

class PureNode : public BasicNode {
public:
    PureNode(const std::string& guid, const std::string& n, BlueprintManager* dManager) : BasicNode(guid, n, dManager) {}

    bool isPure() const override { return true; }
    bool isLoop() const override { return false; }

    virtual void execute() override = 0;
};

class ConstValueNode : public PureNode {
public:
    PinType type;

    int intValue = 0;
    float floatValue = 0.f;
    long double doubleValue = 0.0L;
    bool boolValue = false;
    std::string stringValue = "";

    ConstValueNode(const std::string& guid, const std::string& n, BlueprintManager* dManager) : type(PinType::INT), intValue(0), floatValue(0.0f),
        doubleValue(0.0L), boolValue(false), stringValue(""), PureNode(guid, n, dManager) {}

    bool isPure() const override { return true; }
    bool isLoop() const override { return false; }

    virtual void execute() override = 0;
    void drawNodeToDrawList(ImDrawList* draw_list) override;
};

class ExecNode : public BasicNode {
public:
    ExecNode(const std::string& guid, const std::string& n, BlueprintManager* dManager) : BasicNode(guid, n, dManager) {}

    bool isPure() const override { return false; }
    bool isLoop() const override { return false; }

    virtual void execute() override = 0;
};
