#include "../../../../Public/Blueprints/Node/Nodes/Nodes.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include "../../../../Public/Blueprints/BlueprintManager.h"
#include "../../../../Public/Chart/CandleChart.h"
#include "../../../../Public/StatsManager/TradingStatsManager.h"

#include "../../../../Public/Exchanges/Bybit/BybitMargin.h"
#include "../../../../Public/Exchanges/Bybit/LiquidationCalculator.h"
#include "../../../../Public/Systems/Config/PlatformConfig.h"
using namespace Bybit;
EntryNode::EntryNode(const std::string& guid, BlueprintManager* dManager) : ExecNode(guid, "Entry", dManager) {
    outputs.push_back(std::make_unique<PinOut>("OutPinEntry", guid, PinType::EXEC, "Entry"));
    outputs[0]->owner = this;
    size = ImVec2(120, 60);
    description = "Entry point of the program. Every blueprint must start with an Entry node.\n\nThis is the first node that executes when the program runs. It has a white execution pin that connects to the next node in the execution chain.\n\nWithout an Entry node, your program won't know where to start execution. Think of it as a 'Start' button.";
}

void EntryNode::execute() {
    if (!outputs[0]->connected_pins.empty()) {
        for (auto* connected_pin : outputs[0]->connected_pins) {
            if (connected_pin->owner != nullptr && !connected_pin->owner->isPure()) {
                static_cast<ExecNode*>(connected_pin->owner)->execute();
            }
        }
    }
}

std::string EntryNode::getNodeType() const {
    return "EntryNode";
}
GetVariableNode::GetVariableNode(const std::string& guid, BlueprintManager* dManager, const std::string& varGuid)
    : PureNode(guid, "Get Variable", dManager), variableGuid(varGuid) {

    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::INT, ""));
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(120, 60);
}

void GetVariableNode::execute() {
    if (blueprintManager && !variableGuid.empty()) {
        Variable* var = blueprintManager->getVariable(variableGuid);

        if (var) {
            switch (var->type) {
            case PinType::INT:
                outputs[0]->setValue(var->intValue);
                break;
            case PinType::FLOAT:
                outputs[0]->setValue(var->floatValue);
                break;
            case PinType::DOUBLE:
                outputs[0]->setValue(var->doubleValue);
                break;
            case PinType::BOOL:
                outputs[0]->setValue(var->boolValue);
                break;
            case PinType::STRING:
                outputs[0]->setValue(var->stringValue);
                break;
            case PinType::ARRAY_INT:
                outputs[0]->setValue(&var->arrayIntValue);
                break;
            case PinType::ARRAY_FLOAT:
                outputs[0]->setValue(&var->arrayFloatValue);
                break;
            case PinType::ARRAY_DOUBLE:
                outputs[0]->setValue(&var->arrayDoubleValue);
                break;
            case PinType::ARRAY_BOOL:
                outputs[0]->setValue(&var->arrayBoolValue);
                break;
            case PinType::ARRAY_STRING:
                outputs[0]->setValue(&var->arrayStringValue);
                break;
            }
        }
    }
}

std::string GetVariableNode::getNodeType() const { return "GetVariableNode"; }

void GetVariableNode::bindToVariable(const std::string& varGuid) {
    variableGuid = varGuid;
    if (blueprintManager) {
        Variable* var = blueprintManager->getVariable(varGuid);
        if (var) {
            name = "Get " + var->name;
            outputs[0]->type = var->type;
        }
    }
}

const std::string& GetVariableNode::getVariableGuid() const { return variableGuid; }

nlohmann::json GetVariableNode::serializeData() const {
    nlohmann::json data;
    data["variableGuid"] = variableGuid;
    return data;
}

void GetVariableNode::deserializeData(const nlohmann::json& data) {
    if (data.contains("variableGuid")) {
        variableGuid = data["variableGuid"];
        bindToVariable(variableGuid);
    }
}


SetVariableNode::SetVariableNode(const std::string& guid, BlueprintManager* dManager, const std::string& varGuid)
    : ExecNode(guid, "Set Variable", dManager), variableGuid(varGuid) {

    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Value"));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(120, 80);
}

void SetVariableNode::execute() {
    if (blueprintManager && !variableGuid.empty()) {
        Variable* var = blueprintManager->getVariable(variableGuid);
        if (var) {
            switch (var->type) {
            case PinType::INT:
                var->intValue = inputs[1]->getValue<int>();
                break;
            case PinType::FLOAT:
                var->floatValue = inputs[1]->getValue<float>();
                break;
            case PinType::DOUBLE:
                var->doubleValue = inputs[1]->getValue<long double>();
                break;
            case PinType::BOOL:
                var->boolValue = inputs[1]->getValue<bool>();
                break;
            case PinType::STRING:
                var->stringValue = inputs[1]->getValue<std::string>();
                break;
            case PinType::ARRAY_INT:
                var->arrayIntValue = inputs[1]->getValue<std::vector<int>>();
                break;
            case PinType::ARRAY_FLOAT:
                var->arrayFloatValue = inputs[1]->getValue<std::vector<float>>();
                break;
            case PinType::ARRAY_DOUBLE:
                var->arrayDoubleValue = inputs[1]->getValue<std::vector<long double>>();
                break;
            case PinType::ARRAY_BOOL:
                var->arrayBoolValue = inputs[1]->getValue<std::vector<bool>>();
                break;
            case PinType::ARRAY_STRING:
                var->arrayStringValue = inputs[1]->getValue<std::vector<std::string>>();
                break;
            }
        }
    }

    if (!outputs.empty() && !outputs[0]->connected_pins.empty()) {
        for (auto* connectedPin : outputs[0]->connected_pins) {
            if (connectedPin->owner) {
                connectedPin->owner->execute();
            }
        }
    }
}

std::string SetVariableNode::getNodeType() const { return "SetVariableNode"; }

void SetVariableNode::bindToVariable(const std::string& varGuid) {
    variableGuid = varGuid;
    if (blueprintManager) {
        Variable* var = blueprintManager->getVariable(varGuid);
        if (var) {
            name = "Set " + var->name;
            inputs[1]->type = var->type;
            inputs[1]->name = var->name;
        }
    }
}

const std::string& SetVariableNode::getVariableGuid() const { return variableGuid; }

nlohmann::json SetVariableNode::serializeData() const {
    nlohmann::json data;
    data["variableGuid"] = variableGuid;
    return data;
}

void SetVariableNode::deserializeData(const nlohmann::json& data) {
    if (data.contains("variableGuid")) {
        variableGuid = data["variableGuid"];
        bindToVariable(variableGuid);
    }
}

IntNotEqualNode::IntNotEqualNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "Not Equal to", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(120, 90);
}

void IntNotEqualNode::execute() {
    int a = inputs[0]->getValue<int>();
    int b = inputs[1]->getValue<int>();
    outputs[0]->setValue(a != b);
}

std::string IntNotEqualNode::getNodeType() const {
    return "IntNotEqualNode";
}

IntEqualNode::IntEqualNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "Equal to", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(120, 90);
}

void IntEqualNode::execute() {
    int a = inputs[0]->getValue<int>();
    int b = inputs[1]->getValue<int>();
    outputs[0]->setValue(a == b);
}

std::string IntEqualNode::getNodeType() const {
    return "IntEqualNode";
}

IntLessThanNode::IntLessThanNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "<", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(120, 90);
}

void IntLessThanNode::execute() {
    int a = inputs[0]->getValue<int>();
    int b = inputs[1]->getValue<int>();
    outputs[0]->setValue(a < b);
}

std::string IntLessThanNode::getNodeType() const {
    return "IntLessThanNode";
}

IntMoreThanNode::IntMoreThanNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, ">", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(120, 90);
}

void IntMoreThanNode::execute() {
    int a = inputs[0]->getValue<int>();
    int b = inputs[1]->getValue<int>();
    outputs[0]->setValue(a > b);
}

std::string IntMoreThanNode::getNodeType() const {
    return "IntMoreThanNode";
}

DoubleNotEqualNode::DoubleNotEqualNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "Not Equal", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(120, 90);
}

void DoubleNotEqualNode::execute() {
    long double a = inputs[0]->getValue<long double>();
    long double b = inputs[1]->getValue<long double>();
    outputs[0]->setValue(a != b);
}

std::string DoubleNotEqualNode::getNodeType() const {
    return "DoubleNotEqualNode";
}

DoubleEqualNode::DoubleEqualNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "Equal", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(120, 90);
}

void DoubleEqualNode::execute() {
    long double a = inputs[0]->getValue<long double>();
    long double b = inputs[1]->getValue<long double>();
    outputs[0]->setValue(a == b);
}

std::string DoubleEqualNode::getNodeType() const {
    return "DoubleEqualNode";
}

DoubleLessThanNode::DoubleLessThanNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "<", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(120, 90);
}

void DoubleLessThanNode::execute() {
    long double a = inputs[0]->getValue<long double>();
    long double b = inputs[1]->getValue<long double>();
    outputs[0]->setValue(a < b);
}

std::string DoubleLessThanNode::getNodeType() const {
    return "DoubleLessThanNode";
}

DoubleMoreThanNode::DoubleMoreThanNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, ">", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(120, 90);
}

void DoubleMoreThanNode::execute() {
    long double a = inputs[0]->getValue<long double>();
    long double b = inputs[1]->getValue<long double>();
    outputs[0]->setValue(a > b);
}

std::string DoubleMoreThanNode::getNodeType() const {
    return "DoubleMoreThanNode";
}

FloatNotEqualNode::FloatNotEqualNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "Not Equal", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(120, 90);
}

void FloatNotEqualNode::execute() {
    float a = inputs[0]->getValue<float>();
    float b = inputs[1]->getValue<float>();
    outputs[0]->setValue(a != b);
}

std::string FloatNotEqualNode::getNodeType() const {
    return "FloatNotEqualNode";
}

FloatEqualNode::FloatEqualNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "Equal", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(120, 90);
}

void FloatEqualNode::execute() {
    float a = inputs[0]->getValue<float>();
    float b = inputs[1]->getValue<float>();
    outputs[0]->setValue(a == b);
}

std::string FloatEqualNode::getNodeType() const {
    return "FloatEqualNode";
}

FloatLessThanNode::FloatLessThanNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "<", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(120, 90);
}

void FloatLessThanNode::execute() {
    float a = inputs[0]->getValue<float>();
    float b = inputs[1]->getValue<float>();
    outputs[0]->setValue(a < b);
}

std::string FloatLessThanNode::getNodeType() const {
    return "FloatLessThanNode";
}

FloatMoreThanNode::FloatMoreThanNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, ">", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(120, 90);
}

void FloatMoreThanNode::execute() {
    float a = inputs[0]->getValue<float>();
    float b = inputs[1]->getValue<float>();
    outputs[0]->setValue(a > b);
}

std::string FloatMoreThanNode::getNodeType() const {
    return "FloatMoreThanNode";
}

BoolAndNode::BoolAndNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "Pure AND", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::BOOL, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::BOOL, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(120, 90);
}

void BoolAndNode::execute() {
    bool a = inputs[0]->getValue<bool>();
    bool b = inputs[1]->getValue<bool>();
    outputs[0]->setValue(a && b);
}

std::string BoolAndNode::getNodeType() const {
    return "BoolAndNode";
}

BoolOrNode::BoolOrNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "Or", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::BOOL, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::BOOL, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(120, 90);
}

void BoolOrNode::execute() {
    bool a = inputs[0]->getValue<bool>();
    bool b = inputs[1]->getValue<bool>();
    outputs[0]->setValue(a || b);
}

std::string BoolOrNode::getNodeType() const {
    return "BoolOrNode";
}

BoolEqualNode::BoolEqualNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "Equal", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::BOOL, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::BOOL, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(120, 90);
}

void BoolEqualNode::execute() {
    bool a = inputs[0]->getValue<bool>();
    bool b = inputs[1]->getValue<bool>();
    outputs[0]->setValue(a == b);
}

std::string BoolEqualNode::getNodeType() const {
    return "BoolEqualNode";
}

BoolNotEqualNode::BoolNotEqualNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "Not Equal", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::BOOL, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::BOOL, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(120, 90);
}

void BoolNotEqualNode::execute() {
    bool a = inputs[0]->getValue<bool>();
    bool b = inputs[1]->getValue<bool>();
    outputs[0]->setValue(a != b);
}

std::string BoolNotEqualNode::getNodeType() const {
    return "BoolNotEqualNode";
}

IntToFloatNode::IntToFloatNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "*", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, ""));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::FLOAT, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(90, 60);
}

void IntToFloatNode::execute() {
    int a = inputs[0]->getValue<int>();
    outputs[0]->setValue(static_cast<float>(a));
}

std::string IntToFloatNode::getNodeType() const {
    return "IntToFloatNode";
}

IntToDoubleNode::IntToDoubleNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "*", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, ""));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(90, 60);
}

void IntToDoubleNode::execute() {
    int a = inputs[0]->getValue<int>();
    outputs[0]->setValue(static_cast<long double>(a));
}


std::string IntToDoubleNode::getNodeType() const {
    return "IntToDoubleNode";
}

IntToTextNode::IntToTextNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "*", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, ""));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::STRING, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(90, 60);
}

void IntToTextNode::execute() {
    int a = inputs[0]->getValue<int>();
    outputs[0]->setValue(std::to_string(a));
}


std::string IntToTextNode::getNodeType() const {
    return "IntToTextNode";
}

DoubleToFloatNode::DoubleToFloatNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "*", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, ""));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::FLOAT, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(90, 60);
}

void DoubleToFloatNode::execute() {
    long double a = inputs[0]->getValue<long double>();
    outputs[0]->setValue(static_cast<float>(a));
}

std::string DoubleToFloatNode::getNodeType() const {
    return "IntToFloatNode";
}

DoubleToIntNode::DoubleToIntNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "*", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, ""));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::INT, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(90, 60);
}

void DoubleToIntNode::execute() {
    long double a = inputs[0]->getValue<long double>();
    outputs[0]->setValue(static_cast<int>(a));
}

std::string DoubleToIntNode::getNodeType() const {
    return "DoubleToIntNode";
}

DoubleToTextNode::DoubleToTextNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "*", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, ""));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::STRING, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(90, 60);
}

void DoubleToTextNode::execute() {
    long double a = inputs[0]->getValue<long double>();

    std::ostringstream oss;
    oss << a;
    outputs[0]->setValue(oss.str());
}

std::string DoubleToTextNode::getNodeType() const {
    return "DoubleToTextNode";
}

FloatToDoubleNode::FloatToDoubleNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "*", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, ""));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(90, 60);
}

void FloatToDoubleNode::execute() {
    float a = inputs[0]->getValue<float>();

    long double result = static_cast<long double>(a);

    outputs[0]->setValue(result);
}

std::string FloatToDoubleNode::getNodeType() const {
    return "FloatToDoubleNode";
}

FloatToIntNode::FloatToIntNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "*", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, ""));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::INT, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(90, 60);
}

void FloatToIntNode::execute() {
    float a = inputs[0]->getValue<float>();
    outputs[0]->setValue(static_cast<int>(a));
}

std::string FloatToIntNode::getNodeType() const {
    return "FloatToIntNode";
}

FloatToTextNode::FloatToTextNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "*", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, ""));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::STRING, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(90, 60);
}

void FloatToTextNode::execute() {
    float a = inputs[0]->getValue<float>();
    outputs[0]->setValue(std::to_string(a));
}

std::string FloatToTextNode::getNodeType() const {
    return "FloatToTextNode";
}

BoolToTextNode::BoolToTextNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "*", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::BOOL, ""));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::STRING, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(90, 60);
}

void BoolToTextNode::execute() {
    bool a = inputs[0]->getValue<bool>();
    std::string result = a == true ? "true" : "false";
    outputs[0]->setValue(result);
}

std::string BoolToTextNode::getNodeType() const {
    return "BoolToTextNode";
}

ConstIntNode::ConstIntNode(const std::string& guid, BlueprintManager* dManager) : ConstValueNode(guid, "Const Int", dManager) {
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::INT, ""));
    outputs[0]->owner = this;
    type = PinType::INT;
    size = ImVec2(150, 80);      
}

void ConstIntNode::execute() {
    outputs[0]->setValue(intValue);    
}


std::string ConstIntNode::getNodeType() const {
    return "ConstIntNode";
}

nlohmann::json ConstIntNode::serializeData() const
{
    nlohmann::json data;
    data["value"] = intValue;
    return data;
}

void ConstIntNode::deserializeData(const nlohmann::json& data)
{
    if (data.contains("value")) {
        intValue = data["value"];
    }
}

ConstFloatNode::ConstFloatNode(const std::string& guid, BlueprintManager* dManager) : ConstValueNode(guid, "Const Float", dManager) {
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::FLOAT, ""));
    outputs[0]->owner = this;
    type = PinType::FLOAT;
    size = ImVec2(150, 80);      
}

void ConstFloatNode::execute() {
    outputs[0]->setValue(floatValue);    
}

std::string ConstFloatNode::getNodeType() const {
    return "ConstFloatNode";
}

nlohmann::json ConstFloatNode::serializeData() const
{
    nlohmann::json data;
    data["value"] = floatValue;
    return data;
}

void ConstFloatNode::deserializeData(const nlohmann::json& data)
{
    if (data.contains("value")) {
        floatValue = data["value"];
    }
}

ConstDoubleNode::ConstDoubleNode(const std::string& guid, BlueprintManager* dManager) : ConstValueNode(guid, "Const Double", dManager) {
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, ""));
    outputs[0]->owner = this;
    type = PinType::DOUBLE;
    size = ImVec2(150, 80);      
}

void ConstDoubleNode::execute() {
    outputs[0]->setValue(doubleValue);    
}

std::string ConstDoubleNode::getNodeType() const {
    return "ConstDoubleNode";
}

nlohmann::json ConstDoubleNode::serializeData() const
{
    nlohmann::json data;
    data["value"] = doubleValue;
    return data;
}

void ConstDoubleNode::deserializeData(const nlohmann::json& data)
{
    if (data.contains("value")) {
        doubleValue = data["value"];
    }
}

ConstBoolNode::ConstBoolNode(const std::string& guid, BlueprintManager* dManager) : ConstValueNode(guid, "Const Bool", dManager) {
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, ""));
    outputs[0]->owner = this;
    type = PinType::BOOL;
    size = ImVec2(150, 80);      
}

void ConstBoolNode::execute() {
    outputs[0]->setValue(boolValue);    
}

std::string ConstBoolNode::getNodeType() const {
    return "ConstBoolNode";
}

nlohmann::json ConstBoolNode::serializeData() const
{
    nlohmann::json data;
    data["value"] = boolValue;
    return data;
}

void ConstBoolNode::deserializeData(const nlohmann::json& data)
{
    if (data.contains("value")) {
        boolValue = data["value"];
    }
}

ConstTextNode::ConstTextNode(const std::string& guid, BlueprintManager* dManager) : ConstValueNode(guid, "Const Text", dManager) {
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::STRING, ""));
    outputs[0]->owner = this;
    type = PinType::STRING;
    size = ImVec2(150, 80);      
}

void ConstTextNode::execute() {
    outputs[0]->setValue(stringValue);    
}

std::string ConstTextNode::getNodeType() const {
    return "ConstTextNode";
}

nlohmann::json ConstTextNode::serializeData() const
{
    nlohmann::json data;
    data["value"] = stringValue;
    return data;
}

void ConstTextNode::deserializeData(const nlohmann::json& data)
{
    if (data.contains("value")) {
        stringValue = data["value"];
    }
}

IntMinusIntNode::IntMinusIntNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "-", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::INT, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}
void IntMinusIntNode::execute() {
    int a = inputs[0]->getValue<int>();
    int b = inputs[1]->getValue<int>();
    outputs[0]->setValue(a - b);
}

std::string IntMinusIntNode::getNodeType() const {
    return "IntMinusIntNode";
}

IntMinusDoubleNode::IntMinusDoubleNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "-", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}

void IntMinusDoubleNode::execute() {
    int a = inputs[0]->getValue<int>();
    long double b = inputs[1]->getValue<long double>();
    outputs[0]->setValue(static_cast<long double>(a) - b);
}

std::string IntMinusDoubleNode::getNodeType() const {
    return "IntMinusDoubleNode";
}

IntMinusFloatNode::IntMinusFloatNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "-", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::FLOAT, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}

void IntMinusFloatNode::execute() {
    int a = inputs[0]->getValue<int>();
    float b = inputs[1]->getValue<float>();
    outputs[0]->setValue(static_cast<float>(a) - b);
}

std::string IntMinusFloatNode::getNodeType() const {
    return "IntMinusFloatNode";
}

IntPlusIntNode::IntPlusIntNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "+", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::INT, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}

void IntPlusIntNode::execute() {
    int a = inputs[0]->getValue<int>();
    int b = inputs[1]->getValue<int>();
    outputs[0]->setValue(a + b);
}

std::string IntPlusIntNode::getNodeType() const {
    return "IntPlusIntNode";
}

IntPlusDoubleNode::IntPlusDoubleNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "+", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}

void IntPlusDoubleNode::execute() {
    int a = inputs[0]->getValue<int>();
    long double b = inputs[1]->getValue<long double>();
    outputs[0]->setValue(static_cast<long double>(a) + b);
}

std::string IntPlusDoubleNode::getNodeType() const {
    return "IntPlusDoubleNode";
}

IntPlusFloatNode::IntPlusFloatNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "+", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::FLOAT, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}
void IntPlusFloatNode::execute() {
    int a = inputs[0]->getValue<int>();
    float b = inputs[1]->getValue<float>();
    outputs[0]->setValue(static_cast<float>(a) + b);
}

std::string IntPlusFloatNode::getNodeType() const {
    return "IntPlusFloatNode";
}

IntDivideIntNode::IntDivideIntNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "/", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::INT, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}

void IntDivideIntNode::execute() {
    int a = inputs[0]->getValue<int>();
    int b = inputs[1]->getValue<int>();
    if (b != 0) {
        outputs[0]->setValue(a / b);
    }
    else {
        outputs[0]->setValue(0);
    }
}

std::string IntDivideIntNode::getNodeType() const {
    return "IntDivideIntNode";
}

IntDivideDoubleNode::IntDivideDoubleNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "/", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}

void IntDivideDoubleNode::execute() {
    int a = inputs[0]->getValue<int>();
    long double b = inputs[1]->getValue<long double>();
    if (b != 0.0) {
        outputs[0]->setValue(static_cast<long double>(a) / b);
    }
    else {
        outputs[0]->setValue(0.0);
    }
}

std::string IntDivideDoubleNode::getNodeType() const {
    return "IntDivideDoubleNode";
}

IntDivideFloatNode::IntDivideFloatNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "/", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::FLOAT, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}

void IntDivideFloatNode::execute() {
    int a = inputs[0]->getValue<int>();
    float b = inputs[1]->getValue<float>();
    if (b != 0.0f) {
        outputs[0]->setValue(static_cast<float>(a) / b);
    }
    else {
        outputs[0]->setValue(0.0f);
    }
}

std::string IntDivideFloatNode::getNodeType() const {
    return "IntDivideFloatNode";
}

IntMultiplyIntNode::IntMultiplyIntNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "*", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::INT, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}
void IntMultiplyIntNode::execute() {
    int a = inputs[0]->getValue<int>();
    int b = inputs[1]->getValue<int>();
    outputs[0]->setValue(a * b);
}

std::string IntMultiplyIntNode::getNodeType() const {
    return "IntMultiplyIntNode";
}

IntMultiplyDoubleNode::IntMultiplyDoubleNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "*", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}
void IntMultiplyDoubleNode::execute() {
    int a = inputs[0]->getValue<int>();
    long double b = inputs[1]->getValue<long double>();
    outputs[0]->setValue(static_cast<long double>(a) * b);
}

std::string IntMultiplyDoubleNode::getNodeType() const {
    return "IntMultiplyDoubleNode";
}

IntMultiplyFloatNode::IntMultiplyFloatNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "*", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::FLOAT, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}
void IntMultiplyFloatNode::execute() {
    int a = inputs[0]->getValue<int>();
    float b = inputs[1]->getValue<float>();
    outputs[0]->setValue(static_cast<float>(a) * b);
}

std::string IntMultiplyFloatNode::getNodeType() const {
    return "IntMultiplyFloatNode";
}

FloatMinusIntNode::FloatMinusIntNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "-", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::FLOAT, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}
void FloatMinusIntNode::execute() {
    float a = inputs[0]->getValue<float>();
    int b = inputs[1]->getValue<int>();
    outputs[0]->setValue(a - static_cast<float>(b));
}

std::string FloatMinusIntNode::getNodeType() const {
    return "FloatMinusIntNode";
}

FloatMinusDoubleNode::FloatMinusDoubleNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "-", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}
void FloatMinusDoubleNode::execute() {
    float a = inputs[0]->getValue<float>();
    long double b = inputs[1]->getValue<long double>();
    outputs[0]->setValue(static_cast<long double>(a) - b);
}

std::string FloatMinusDoubleNode::getNodeType() const {
    return "FloatMinusDoubleNode";
}

FloatMinusFloatNode::FloatMinusFloatNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "-", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::FLOAT, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}
void FloatMinusFloatNode::execute() {
    float a = inputs[0]->getValue<float>();
    float b = inputs[1]->getValue<float>();
    outputs[0]->setValue(a - b);
}

std::string FloatMinusFloatNode::getNodeType() const {
    return "FloatMinusFloatNode";
}

FloatPlusIntNode::FloatPlusIntNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "+", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::FLOAT, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}
void FloatPlusIntNode::execute() {
    float a = inputs[0]->getValue<float>();
    int b = inputs[1]->getValue<int>();
    outputs[0]->setValue(a + static_cast<float>(b));
}

std::string FloatPlusIntNode::getNodeType() const {
    return "FloatPlusIntNode";
}

FloatPlusDoubleNode::FloatPlusDoubleNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "+", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}
void FloatPlusDoubleNode::execute() {
    float a = inputs[0]->getValue<float>();
    long double b = inputs[1]->getValue<long double>();
    outputs[0]->setValue(static_cast<long double>(a) + b);
}

std::string FloatPlusDoubleNode::getNodeType() const {
    return "FloatPlusDoubleNode";
}

FloatPlusFloatNode::FloatPlusFloatNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "+", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::FLOAT, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}
void FloatPlusFloatNode::execute() {
    float a = inputs[0]->getValue<float>();
    float b = inputs[1]->getValue<float>();
    outputs[0]->setValue(a + b);
}

std::string FloatPlusFloatNode::getNodeType() const {
    return "FloatPlusFloatNode";
}

FloatDivideIntNode::FloatDivideIntNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "/", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::FLOAT, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}
void FloatDivideIntNode::execute() {
    float a = inputs[0]->getValue<float>();
    int b = inputs[1]->getValue<int>();
    if (b != 0) {
        outputs[0]->setValue(a / static_cast<float>(b));
    }
    else {
        outputs[0]->setValue(0.0f);
    }
}

std::string FloatDivideIntNode::getNodeType() const {
    return "FloatDivideIntNode";
}

FloatDivideDoubleNode::FloatDivideDoubleNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "/", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}
void FloatDivideDoubleNode::execute() {
    float a = inputs[0]->getValue<float>();
    long double b = inputs[1]->getValue<long double>();
    if (b != 0.0) {
        outputs[0]->setValue(static_cast<long double>(a) / b);
    }
    else {
        outputs[0]->setValue(0.0);
    }
}

std::string FloatDivideDoubleNode::getNodeType() const {
    return "FloatDivideDoubleNode";
}

FloatDivideFloatNode::FloatDivideFloatNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "/", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::FLOAT, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}
void FloatDivideFloatNode::execute() {
    float a = inputs[0]->getValue<float>();
    float b = inputs[1]->getValue<float>();
    if (b != 0.0f) {
        outputs[0]->setValue(a / b);
    }
    else {
        outputs[0]->setValue(0.0f);
    }
}

std::string FloatDivideFloatNode::getNodeType() const {
    return "FloatDivideFloatNode";
}

FloatMultiplyIntNode::FloatMultiplyIntNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "*", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::FLOAT, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}

void FloatMultiplyIntNode::execute() {
    float a = inputs[0]->getValue<float>();
    int b = inputs[1]->getValue<int>();
    outputs[0]->setValue(a * static_cast<float>(b));
}

std::string FloatMultiplyIntNode::getNodeType() const {
    return "FloatMultiplyIntNode";
}

FloatMultiplyDoubleNode::FloatMultiplyDoubleNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "*", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}

void FloatMultiplyDoubleNode::execute() {
    float a = inputs[0]->getValue<float>();
    long double b = inputs[1]->getValue<long double>();
    outputs[0]->setValue(static_cast<long double>(a) * b);
}

std::string FloatMultiplyDoubleNode::getNodeType() const {
    return "FloatMultiplyDoubleNode";
}

FloatMultiplyFloatNode::FloatMultiplyFloatNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "*", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::FLOAT, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}

void FloatMultiplyFloatNode::execute() {
    float a = inputs[0]->getValue<float>();
    float b = inputs[1]->getValue<float>();
    outputs[0]->setValue(a * b);
}

std::string FloatMultiplyFloatNode::getNodeType() const {
    return "FloatMultiplyFloatNode";
}

DoubleMinusIntNode::DoubleMinusIntNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "-", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}

void DoubleMinusIntNode::execute() {
    long double a = inputs[0]->getValue<long double>();
    int b = inputs[1]->getValue<int>();
    outputs[0]->setValue(a - static_cast<long double>(b));
}

std::string DoubleMinusIntNode::getNodeType() const {
    return "DoubleMinusIntNode";
}

DoubleMinusDoubleNode::DoubleMinusDoubleNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "-", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}

void DoubleMinusDoubleNode::execute() {
    long double a = inputs[0]->getValue<long double>();
    long double b = inputs[1]->getValue<long double>();
    outputs[0]->setValue(a - b);
}

std::string DoubleMinusDoubleNode::getNodeType() const {
    return "DoubleMinusDoubleNode";
}

DoubleMinusFloatNode::DoubleMinusFloatNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "-", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}

void DoubleMinusFloatNode::execute() {
    long double a = inputs[0]->getValue<long double>();
    float b = inputs[1]->getValue<float>();
    outputs[0]->setValue(a - static_cast<long double>(b));
}

std::string DoubleMinusFloatNode::getNodeType() const {
    return "DoubleMinusFloatNode";
}

DoublePlusIntNode::DoublePlusIntNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "+", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}

void DoublePlusIntNode::execute() {
    long double a = inputs[0]->getValue<long double>();
    int b = inputs[1]->getValue<int>();
    outputs[0]->setValue(a + static_cast<long double>(b));
}

std::string DoublePlusIntNode::getNodeType() const {
    return "DoublePlusIntNode";
}

DoublePlusDoubleNode::DoublePlusDoubleNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "+", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}

void DoublePlusDoubleNode::execute() {
    long double a = inputs[0]->getValue<long double>();
    long double b = inputs[1]->getValue<long double>();
    outputs[0]->setValue(a + b);
}

std::string DoublePlusDoubleNode::getNodeType() const {
    return "DoublePlusDoubleNode";
}

DoublePlusFloatNode::DoublePlusFloatNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "+", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}

void DoublePlusFloatNode::execute() {
    long double a = inputs[0]->getValue<long double>();
    float b = inputs[1]->getValue<float>();
    outputs[0]->setValue(a + static_cast<long double>(b));
}

std::string DoublePlusFloatNode::getNodeType() const {
    return "DoublePlusFloatNode";
}

DoubleDivideIntNode::DoubleDivideIntNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "/", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}

void DoubleDivideIntNode::execute() {
    long double a = inputs[0]->getValue<long double>();
    int b = inputs[1]->getValue<int>();
    if (b != 0) {
        outputs[0]->setValue(a / static_cast<long double>(b));
    }
    else {
        outputs[0]->setValue(0.0);
    }
}

std::string DoubleDivideIntNode::getNodeType() const {
    return "DoubleDivideIntNode";
}

DoubleDivideDoubleNode::DoubleDivideDoubleNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "/", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}
void DoubleDivideDoubleNode::execute() {
    long double a = inputs[0]->getValue<long double>();
    long double b = inputs[1]->getValue<long double>();
    if (b != 0.0) {
        outputs[0]->setValue(a / b);
    }
    else {
        outputs[0]->setValue(0.0);
    }
}

std::string DoubleDivideDoubleNode::getNodeType() const {
    return "DoubleDivideDoubleNode";
}

DoubleDivideFloatNode::DoubleDivideFloatNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "/", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}
void DoubleDivideFloatNode::execute() {
    long double a = inputs[0]->getValue<long double>();
    float b = inputs[1]->getValue<float>();
    if (b != 0.0f) {
        outputs[0]->setValue(a / static_cast<long double>(b));
    }
    else {
        outputs[0]->setValue(0.0);
    }
}

std::string DoubleDivideFloatNode::getNodeType() const {
    return "DoubleDivideFloatNode";
}

DoubleMultiplyIntNode::DoubleMultiplyIntNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "*", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}

void DoubleMultiplyIntNode::execute() {
    long double a = inputs[0]->getValue<long double>();
    int b = inputs[1]->getValue<int>();
    outputs[0]->setValue(a * static_cast<long double>(b));
}

std::string DoubleMultiplyIntNode::getNodeType() const {
    return "DoubleMultiplyIntNode";
}

DoubleMultiplyDoubleNode::DoubleMultiplyDoubleNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "*", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}

void DoubleMultiplyDoubleNode::execute() {
    long double a = inputs[0]->getValue<long double>();
    long double b = inputs[1]->getValue<long double>();
    outputs[0]->setValue(a * b);
}

std::string DoubleMultiplyDoubleNode::getNodeType() const {
    return "DoubleMultiplyDoubleNode";
}

DoubleMultiplyFloatNode::DoubleMultiplyFloatNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "*", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}

void DoubleMultiplyFloatNode::execute() {
    long double a = inputs[0]->getValue<long double>();
    float b = inputs[1]->getValue<float>();
    outputs[0]->setValue(a * static_cast<long double>(b));
}

std::string DoubleMultiplyFloatNode::getNodeType() const {
    return "DoubleMultiplyFloatNode";
}

TextAppendNode::TextAppendNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "*", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::STRING, "A"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::STRING, "B"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::STRING, ""));
    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(90, 80);
}

void TextAppendNode::execute() {
    std::string a = inputs[0]->getValue<std::string>();
    std::string b = inputs[1]->getValue<std::string>();
    outputs[0]->setValue(a + b);
}


std::string TextAppendNode::getNodeType() const {
    return "TextAppendNode";
}

nlohmann::json TextAppendNode::serializeData() const
{
    nlohmann::json data;
    data["value"] = text;
    return data;
}

void TextAppendNode::deserializeData(const nlohmann::json& data)
{
    if (data.contains("value")) {
        text = data["value"];
    }
}

GetIntArrayElementNode::GetIntArrayElementNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "Get Int Element", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::ARRAY_INT, "Array"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::INT, "Element"));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 90);
    description = "";
}

void GetIntArrayElementNode::execute() {
    std::vector<int>* Array = inputs[0]->getValue<std::vector<int>*>();
    int Index = inputs[1]->getValue<int>();
    if (Array && Index >= 0 && Index < Array->size())
    {
        auto value = Array->at(Index);
        outputs[0]->setValue(value);
    }
    else outputs[0]->setValue(0);
}

std::string GetIntArrayElementNode::getNodeType() const {
    return "GetIntArrayElementNode";
}

AddIntArrayElementNode::AddIntArrayElementNode(const std::string& guid, BlueprintManager* dManager) : ExecNode(guid, "Add Int Element", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::ARRAY_INT, "Array"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Element"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 110);
    description = "";
}

void AddIntArrayElementNode::execute() {
    std::vector<int>* Array = inputs[1]->getValue<std::vector<int>*>();
    if (Array)
    {
        Array->push_back(inputs[2]->getValue<int>());
        int lastIndex = Array->size() - 1;
        outputs[1]->setValue(lastIndex);
    }

    if (!outputs[0]->connected_pins.empty()) {
        for (auto* connected_pin : outputs[0]->connected_pins) {
            if (connected_pin->owner && !connected_pin->owner->isPure()) {
                static_cast<ExecNode*>(connected_pin->owner)->execute();
            }
        }
    }
}

std::string AddIntArrayElementNode::getNodeType() const {
    return "AddIntArrayElementNode";
}

SizeIntArrayElementNode::SizeIntArrayElementNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "Get Size", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::ARRAY_INT, "Array"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::INT, "Size"));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 90);
    description = "";
}

void SizeIntArrayElementNode::execute() {
    std::vector<int>* Array = inputs[0]->getValue<std::vector<int>*>();
    if (Array) {
        outputs[0]->setValue((int)Array->size());
    }
}

std::string SizeIntArrayElementNode::getNodeType() const {
    return "SizeIntArrayElementNode";
}

ClearIntArrayElementNode::ClearIntArrayElementNode(const std::string& guid, BlueprintManager* dManager) : ExecNode(guid, "Clear Array", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::ARRAY_INT, "Array"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 110);
    description = "";
}

void ClearIntArrayElementNode::execute() {
    std::vector<int>* Array = inputs[1]->getValue<std::vector<int>*>();
    if (Array) {
        Array->clear();
    }

    if (!outputs[0]->connected_pins.empty()) {
        for (auto* connected_pin : outputs[0]->connected_pins) {
            if (connected_pin->owner && !connected_pin->owner->isPure()) {
                static_cast<ExecNode*>(connected_pin->owner)->execute();
            }
        }
    }
}

std::string ClearIntArrayElementNode::getNodeType() const {
    return "ClearIntArrayElementNode";
}

RemoveIntArrayElementNode::RemoveIntArrayElementNode(const std::string& guid, BlueprintManager* dManager) : ExecNode(guid, "Remove Element", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::ARRAY_INT, "Array"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 110);
    description = "";
}

void RemoveIntArrayElementNode::execute() {
    std::vector<int>* Array = inputs[1]->getValue<std::vector<int>*>();
    int Index = inputs[2]->getValue<int>();

    if (Array && Index >= 0 && Index < Array->size()) {
        Array->erase(Array->begin() + Index);
    }

    if (!outputs[0]->connected_pins.empty()) {
        for (auto* connected_pin : outputs[0]->connected_pins) {
            if (connected_pin->owner && !connected_pin->owner->isPure()) {
                static_cast<ExecNode*>(connected_pin->owner)->execute();
            }
        }
    }
}

std::string RemoveIntArrayElementNode::getNodeType() const {
    return "RemoveIntArrayElementNode";
}

GetFloatArrayElementNode::GetFloatArrayElementNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "Get Float Element", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::ARRAY_FLOAT, "Array"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::FLOAT, "Element"));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 90);
    description = "";
}

void GetFloatArrayElementNode::execute() {
    std::vector<float>* Array = inputs[0]->getValue<std::vector<float>*>();
    int Index = inputs[1]->getValue<int>();
    if (Array && Index >= 0 && Index < Array->size())
    {
        auto value = Array->at(Index);
        outputs[0]->setValue(value);
    }
    else outputs[0]->setValue(0.0f);
}

std::string GetFloatArrayElementNode::getNodeType() const {
    return "GetFloatArrayElementNode";
}

AddFloatArrayElementNode::AddFloatArrayElementNode(const std::string& guid, BlueprintManager* dManager) : ExecNode(guid, "Add Float Element", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::ARRAY_FLOAT, "Array"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::FLOAT, "Element"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 110);
    description = "";
}

void AddFloatArrayElementNode::execute() {
    std::vector<float>* Array = inputs[1]->getValue<std::vector<float>*>();
    if (Array)
    {
        Array->push_back(inputs[2]->getValue<float>());
        int lastIndex = Array->size() - 1;
        outputs[1]->setValue(lastIndex);
    }

    if (!outputs[0]->connected_pins.empty()) {
        for (auto* connected_pin : outputs[0]->connected_pins) {
            if (connected_pin->owner && !connected_pin->owner->isPure()) {
                static_cast<ExecNode*>(connected_pin->owner)->execute();
            }
        }
    }
}

std::string AddFloatArrayElementNode::getNodeType() const {
    return "AddFloatArrayElementNode";
}

SizeFloatArrayElementNode::SizeFloatArrayElementNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "Get Size", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::ARRAY_FLOAT, "Array"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::INT, "Size"));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 90);
    description = "";
}

void SizeFloatArrayElementNode::execute() {
    std::vector<float>* Array = inputs[0]->getValue<std::vector<float>*>();
    if (Array) {
        outputs[0]->setValue((int)Array->size());
    }
}

std::string SizeFloatArrayElementNode::getNodeType() const {
    return "SizeFloatArrayElementNode";
}

ClearFloatArrayElementNode::ClearFloatArrayElementNode(const std::string& guid, BlueprintManager* dManager) : ExecNode(guid, "Clear Array", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::ARRAY_FLOAT, "Array"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 110);
    description = "";
}

void ClearFloatArrayElementNode::execute() {
    std::vector<float>* Array = inputs[1]->getValue<std::vector<float>*>();
    if (Array) {
        Array->clear();
    }

    if (!outputs[0]->connected_pins.empty()) {
        for (auto* connected_pin : outputs[0]->connected_pins) {
            if (connected_pin->owner && !connected_pin->owner->isPure()) {
                static_cast<ExecNode*>(connected_pin->owner)->execute();
            }
        }
    }
}

std::string ClearFloatArrayElementNode::getNodeType() const {
    return "ClearFloatArrayElementNode";
}

RemoveFloatArrayElementNode::RemoveFloatArrayElementNode(const std::string& guid, BlueprintManager* dManager) : ExecNode(guid, "Remove Element", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::ARRAY_FLOAT, "Array"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 110);
    description = "";
}

void RemoveFloatArrayElementNode::execute() {
    std::vector<float>* Array = inputs[1]->getValue<std::vector<float>*>();
    int Index = inputs[2]->getValue<int>();

    if (Array && Index >= 0 && Index < Array->size()) {
        Array->erase(Array->begin() + Index);
    }

    if (!outputs[0]->connected_pins.empty()) {
        for (auto* connected_pin : outputs[0]->connected_pins) {
            if (connected_pin->owner && !connected_pin->owner->isPure()) {
                static_cast<ExecNode*>(connected_pin->owner)->execute();
            }
        }
    }
}

std::string RemoveFloatArrayElementNode::getNodeType() const {
    return "RemoveFloatArrayElementNode";
}

GetDoubleArrayElementNode::GetDoubleArrayElementNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "Get Double Element", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::ARRAY_DOUBLE, "Array"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Element"));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 90);
    description = "";
}

void GetDoubleArrayElementNode::execute() {
    std::vector<long double>* Array = inputs[0]->getValue<std::vector<long double>*>();
    int Index = inputs[1]->getValue<int>();
    if (Array && Index >= 0 && Index < Array->size())
    {
        auto value = Array->at(Index);
        outputs[0]->setValue(value);
    }
    else outputs[0]->setValue(0.0L);
}

std::string GetDoubleArrayElementNode::getNodeType() const {
    return "GetDoubleArrayElementNode";
}

AddDoubleArrayElementNode::AddDoubleArrayElementNode(const std::string& guid, BlueprintManager* dManager) : ExecNode(guid, "Add Double Element", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::ARRAY_DOUBLE, "Array"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Element"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 110);
    description = "";
}

void AddDoubleArrayElementNode::execute() {
    std::vector<long double>* Array = inputs[1]->getValue<std::vector<long double>*>();
    if (Array)
    {
        Array->push_back(inputs[2]->getValue<long double>());
        int lastIndex = Array->size() - 1;
        outputs[1]->setValue(lastIndex);
    }

    if (!outputs[0]->connected_pins.empty()) {
        for (auto* connected_pin : outputs[0]->connected_pins) {
            if (connected_pin->owner && !connected_pin->owner->isPure()) {
                static_cast<ExecNode*>(connected_pin->owner)->execute();
            }
        }
    }
}

std::string AddDoubleArrayElementNode::getNodeType() const {
    return "AddDoubleArrayElementNode";
}

SizeDoubleArrayElementNode::SizeDoubleArrayElementNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "Get Size", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::ARRAY_DOUBLE, "Array"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::INT, "Size"));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 90);
    description = "";
}

void SizeDoubleArrayElementNode::execute() {
    std::vector<long double>* Array = inputs[0]->getValue<std::vector<long double>*>();
    if (Array) {
        outputs[0]->setValue((int)Array->size());
    }
}

std::string SizeDoubleArrayElementNode::getNodeType() const {
    return "SizeDoubleArrayElementNode";
}

ClearDoubleArrayElementNode::ClearDoubleArrayElementNode(const std::string& guid, BlueprintManager* dManager) : ExecNode(guid, "Clear Array", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::ARRAY_DOUBLE, "Array"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 110);
    description = "";
}

void ClearDoubleArrayElementNode::execute() {
    std::vector<long double>* Array = inputs[1]->getValue<std::vector<long double>*>();
    if (Array) {
        Array->clear();
    }

    if (!outputs[0]->connected_pins.empty()) {
        for (auto* connected_pin : outputs[0]->connected_pins) {
            if (connected_pin->owner && !connected_pin->owner->isPure()) {
                static_cast<ExecNode*>(connected_pin->owner)->execute();
            }
        }
    }
}

std::string ClearDoubleArrayElementNode::getNodeType() const {
    return "ClearDoubleArrayElementNode";
}

RemoveDoubleArrayElementNode::RemoveDoubleArrayElementNode(const std::string& guid, BlueprintManager* dManager) : ExecNode(guid, "Remove Element", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::ARRAY_DOUBLE, "Array"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 110);
    description = "";
}

void RemoveDoubleArrayElementNode::execute() {
    std::vector<long double>* Array = inputs[1]->getValue<std::vector<long double>*>();
    int Index = inputs[2]->getValue<int>();

    if (Array && Index >= 0 && Index < Array->size()) {
        Array->erase(Array->begin() + Index);
    }

    if (!outputs[0]->connected_pins.empty()) {
        for (auto* connected_pin : outputs[0]->connected_pins) {
            if (connected_pin->owner && !connected_pin->owner->isPure()) {
                static_cast<ExecNode*>(connected_pin->owner)->execute();
            }
        }
    }
}

std::string RemoveDoubleArrayElementNode::getNodeType() const {
    return "RemoveDoubleArrayElementNode";
}

GetBoolArrayElementNode::GetBoolArrayElementNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "Get Bool Element", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::ARRAY_BOOL, "Array"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, "Element"));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 90);
    description = "";
}

void GetBoolArrayElementNode::execute() {
    std::vector<bool>* Array = inputs[0]->getValue<std::vector<bool>*>();
    int Index = inputs[1]->getValue<int>();
    if (Array && Index >= 0 && Index < Array->size())
    {
        auto value = Array->at(Index);
        outputs[0]->setValue(value);
    }
    else outputs[0]->setValue(false);
}

std::string GetBoolArrayElementNode::getNodeType() const {
    return "GetBoolArrayElementNode";
}

AddBoolArrayElementNode::AddBoolArrayElementNode(const std::string& guid, BlueprintManager* dManager) : ExecNode(guid, "Add Bool Element", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::ARRAY_BOOL, "Array"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::BOOL, "Element"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 110);
    description = "";
}

void AddBoolArrayElementNode::execute() {
    std::vector<bool>* Array = inputs[1]->getValue<std::vector<bool>*>();
    if (Array)
    {
        Array->push_back(inputs[2]->getValue<bool>());
        int lastIndex = Array->size() - 1;
        outputs[1]->setValue(lastIndex);
    }

    if (!outputs[0]->connected_pins.empty()) {
        for (auto* connected_pin : outputs[0]->connected_pins) {
            if (connected_pin->owner && !connected_pin->owner->isPure()) {
                static_cast<ExecNode*>(connected_pin->owner)->execute();
            }
        }
    }
}

std::string AddBoolArrayElementNode::getNodeType() const {
    return "AddBoolArrayElementNode";
}

SizeBoolArrayElementNode::SizeBoolArrayElementNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "Get Size", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::ARRAY_BOOL, "Array"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::INT, "Size"));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 90);
    description = "";
}

void SizeBoolArrayElementNode::execute() {
    std::vector<bool>* Array = inputs[0]->getValue<std::vector<bool>*>();
    if (Array) {
        outputs[0]->setValue((int)Array->size());
    }
}

std::string SizeBoolArrayElementNode::getNodeType() const {
    return "SizeBoolArrayElementNode";
}

ClearBoolArrayElementNode::ClearBoolArrayElementNode(const std::string& guid, BlueprintManager* dManager) : ExecNode(guid, "Clear Array", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::ARRAY_BOOL, "Array"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 110);
    description = "";
}

void ClearBoolArrayElementNode::execute() {
    std::vector<bool>* Array = inputs[1]->getValue<std::vector<bool>*>();
    if (Array) {
        Array->clear();
    }

    if (!outputs[0]->connected_pins.empty()) {
        for (auto* connected_pin : outputs[0]->connected_pins) {
            if (connected_pin->owner && !connected_pin->owner->isPure()) {
                static_cast<ExecNode*>(connected_pin->owner)->execute();
            }
        }
    }
}

std::string ClearBoolArrayElementNode::getNodeType() const {
    return "ClearBoolArrayElementNode";
}

RemoveBoolArrayElementNode::RemoveBoolArrayElementNode(const std::string& guid, BlueprintManager* dManager) : ExecNode(guid, "Remove Element", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::ARRAY_BOOL, "Array"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 110);
    description = "";
}

void RemoveBoolArrayElementNode::execute() {
    std::vector<bool>* Array = inputs[1]->getValue<std::vector<bool>*>();
    int Index = inputs[2]->getValue<int>();

    if (Array && Index >= 0 && Index < Array->size()) {
        Array->erase(Array->begin() + Index);
    }

    if (!outputs[0]->connected_pins.empty()) {
        for (auto* connected_pin : outputs[0]->connected_pins) {
            if (connected_pin->owner && !connected_pin->owner->isPure()) {
                static_cast<ExecNode*>(connected_pin->owner)->execute();
            }
        }
    }
}

std::string RemoveBoolArrayElementNode::getNodeType() const {
    return "RemoveBoolArrayElementNode";
}

GetStringArrayElementNode::GetStringArrayElementNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "Get String Element", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::ARRAY_STRING, "Array"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::STRING, "Element"));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 90);
    description = "";
}

void GetStringArrayElementNode::execute() {
    std::vector<std::string>* Array = inputs[0]->getValue<std::vector<std::string>*>();
    int Index = inputs[1]->getValue<int>();
    if (Array && Index >= 0 && Index < Array->size())
    {
        auto value = Array->at(Index);
        outputs[0]->setValue(value);
    }
    else outputs[0]->setValue("");
}

std::string GetStringArrayElementNode::getNodeType() const {
    return "GetStringArrayElementNode";
}

AddStringArrayElementNode::AddStringArrayElementNode(const std::string& guid, BlueprintManager* dManager) : ExecNode(guid, "Add String Element", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::ARRAY_STRING, "Array"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::STRING, "Element"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 110);
    description = "";
}

void AddStringArrayElementNode::execute() {
    std::vector<std::string>* Array = inputs[1]->getValue<std::vector<std::string>*>();
    if (Array)
    {
        Array->push_back(inputs[2]->getValue<std::string>());
        int lastIndex = Array->size() - 1;
        outputs[1]->setValue(lastIndex);
    }

    if (!outputs[0]->connected_pins.empty()) {
        for (auto* connected_pin : outputs[0]->connected_pins) {
            if (connected_pin->owner && !connected_pin->owner->isPure()) {
                static_cast<ExecNode*>(connected_pin->owner)->execute();
            }
        }
    }
}

std::string AddStringArrayElementNode::getNodeType() const {
    return "AddStringArrayElementNode";
}

SizeStringArrayElementNode::SizeStringArrayElementNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "Get Size", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::ARRAY_STRING, "Array"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::INT, "Size"));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 90);
    description = "";
}

void SizeStringArrayElementNode::execute() {
    std::vector<std::string>* Array = inputs[0]->getValue<std::vector<std::string>*>();
    if (Array) {
        outputs[0]->setValue((int)Array->size());
    }
}

std::string SizeStringArrayElementNode::getNodeType() const {
    return "SizeStringArrayElementNode";
}

ClearStringArrayElementNode::ClearStringArrayElementNode(const std::string& guid, BlueprintManager* dManager) : ExecNode(guid, "Clear Array", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::ARRAY_STRING, "Array"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 110);
    description = "";
}

void ClearStringArrayElementNode::execute() {
    std::vector<std::string>* Array = inputs[1]->getValue<std::vector<std::string>*>();
    if (Array) {
        Array->clear();
    }

    if (!outputs[0]->connected_pins.empty()) {
        for (auto* connected_pin : outputs[0]->connected_pins) {
            if (connected_pin->owner && !connected_pin->owner->isPure()) {
                static_cast<ExecNode*>(connected_pin->owner)->execute();
            }
        }
    }
}

std::string ClearStringArrayElementNode::getNodeType() const {
    return "ClearStringArrayElementNode";
}

RemoveStringArrayElementNode::RemoveStringArrayElementNode(const std::string& guid, BlueprintManager* dManager) : ExecNode(guid, "Remove Element", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::ARRAY_STRING, "Array"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 110);
    description = "";
}

void RemoveStringArrayElementNode::execute() {
    std::vector<std::string>* Array = inputs[1]->getValue<std::vector<std::string>*>();
    int Index = inputs[2]->getValue<int>();

    if (Array && Index >= 0 && Index < Array->size()) {
        Array->erase(Array->begin() + Index);
    }

    if (!outputs[0]->connected_pins.empty()) {
        for (auto* connected_pin : outputs[0]->connected_pins) {
            if (connected_pin->owner && !connected_pin->owner->isPure()) {
                static_cast<ExecNode*>(connected_pin->owner)->execute();
            }
        }
    }
}

std::string RemoveStringArrayElementNode::getNodeType() const {
    return "RemoveStringArrayElementNode";
}

ForLoopNode::ForLoopNode(const std::string& guid, BlueprintManager* dManager) : ExecNode(guid, "For Loop", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "First index"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Last index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, "Loop body"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, "Completed"));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(200, 110);
    description = "For Loop executes a series of operations a specified number of times.\n\nInputs:\n- First Index: Starting value of the loop counter\n- Last Index: Final value of the loop counter\n\nOutputs:\n- Loop Body: Executes for each iteration\n- Index: Current loop counter value\n- Completed: Executes after loop finishes\n\nThe loop runs from First Index to Last Index (inclusive), incrementing the counter by 1 each iteration.";
}

void ForLoopNode::execute() {
    int firstIndex = inputs[1]->getValue<int>();
    int lastIndex = inputs[2]->getValue<int>();

    for (int i = firstIndex; i <= lastIndex; i++)
    {
        outputs[1]->setValue<int>(i);

        if (!outputs[0]->connected_pins.empty()) {
            for (auto* connected_pin : outputs[0]->connected_pins) {
                if (connected_pin->owner && !connected_pin->owner->isPure()) {
                    static_cast<ExecNode*>(connected_pin->owner)->execute();
                }
            }
        }
    }
    if (!outputs[2]->connected_pins.empty()) {
        for (auto* connected_pin : outputs[2]->connected_pins) {
            if (connected_pin->owner && !connected_pin->owner->isPure()) {
                static_cast<ExecNode*>(connected_pin->owner)->execute();
            }
        }
    }
}

std::string ForLoopNode::getNodeType() const {
    return "ForLoopNode";
}


AddMarkNode::AddMarkNode(const std::string& guid, BlueprintManager* dManager) : ExecNode(guid, "Add Mark", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Candle"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Price"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "R"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "G"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "B"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::STRING, "Label"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(120, 200);
    description = "Adds a circular trading mark on the chart at specified position.\n\nAllows marking important entry/exit points with colored marker and label";
}

void AddMarkNode::execute() {

    int candle = inputs[1]->getValue<int>();
    long double price = inputs[2]->getValue<long double>();
    int R = inputs[3]->getValue<int>();
    int G = inputs[4]->getValue<int>();
    int B = inputs[5]->getValue<int>();
    std::string label = inputs[6]->getValue<std::string>();

    if(blueprintManager && blueprintManager->GetDataManager() && blueprintManager->GetDataManager()->GetChart())
    blueprintManager->GetDataManager()->GetChart()->addTradingMark(candle, price, IM_COL32(R, G, B, 255), label);

    if (!outputs[0]->connected_pins.empty()) {
        for (auto* connected_pin : outputs[0]->connected_pins) {
            if (connected_pin->owner && !connected_pin->owner->isPure()) {
                static_cast<ExecNode*>(connected_pin->owner)->execute();
            }
        }
    }
}

std::string AddMarkNode::getNodeType() const {
    return "AddMarkNode";
}

AddLineNode::AddLineNode(const std::string& guid, BlueprintManager* dManager) : ExecNode(guid, "Add Mark", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Candle 1"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Price 1"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Candle 2"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Price 2"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "R"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "G"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "B"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Thickness"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(120, 250);
    description = "Draws a line between two points on the chart with specified color.\n\nAllows connecting support/resistance levels or showing trends";
}

void AddLineNode::execute() {

    int candle = inputs[1]->getValue<int>();
    long double price = inputs[2]->getValue<long double>();
    int candle2 = inputs[3]->getValue<int>();
    long double price2 = inputs[4]->getValue<long double>();
    int R = inputs[5]->getValue<int>();
    int G = inputs[6]->getValue<int>();
    int B = inputs[7]->getValue<int>();
    long double thickness = inputs[8]->getValue<long double>();

    if (blueprintManager && blueprintManager->GetDataManager() && blueprintManager->GetDataManager()->GetChart())
        blueprintManager->GetDataManager()->GetChart()->addTradingLine(candle, price, candle2, price2, IM_COL32(R,G,B,255), thickness);

    if (!outputs[0]->connected_pins.empty()) {
        for (auto* connected_pin : outputs[0]->connected_pins) {
            if (connected_pin->owner && !connected_pin->owner->isPure()) {
                static_cast<ExecNode*>(connected_pin->owner)->execute();
            }
        }
    }
}

std::string AddLineNode::getNodeType() const {
    return "AddLineNode";
}

GetCurrentCandleDataNode::GetCurrentCandleDataNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "Get Current Candle", dManager) {
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::INT, "Timestamp"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Open"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "High"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Low"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Close"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::FLOAT, "Volume"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::FLOAT, "Mcap"));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(160, 230);
    description = "Retrieves all data from the most recent candle in the chart.\n\nThis node provides real-time access to:\n- OHLC values (Open, High, Low, Close)\n- Trading volume\n- Market cap\n- Timestamp and index\n\nUse this node when you need to analyze the latest price action for making trading decisions.\n\nNote: This always returns the last candle in your dataset. For historical candles, use 'Get Candle At Index' instead.";
}

void GetCurrentCandleDataNode::execute() {

    if (blueprintManager && blueprintManager->GetDataManager() && blueprintManager->GetDataManager()->GetChart())
    {
        if(blueprintManager->GetDataManager()->getPublicData().size() > 0)
        {
            auto candle = blueprintManager->GetDataManager()->getPublicData().back();
            int index = blueprintManager->GetDataManager()->getPublicData().size() - 1;
            outputs[0]->setValue(index);
            outputs[1]->setValue<int>(candle.timestamp);
            outputs[2]->setValue(candle.open);
            outputs[3]->setValue(candle.high);
            outputs[4]->setValue(candle.low);
            outputs[5]->setValue(candle.close);
            outputs[6]->setValue(candle.volume);
            outputs[7]->setValue(candle.mcap);
        }
        else
        {
            outputs[0]->setValue(0);
            outputs[1]->setValue(0.0L);
            outputs[2]->setValue(0.0L);
            outputs[3]->setValue(0.0L);
            outputs[4]->setValue(0.0L);
            outputs[5]->setValue(0.0L);
            outputs[6]->setValue(0.0f);
            outputs[7]->setValue(0.0f);
        }
    }
}

std::string GetCurrentCandleDataNode::getNodeType() const {
    return "GetCurrentCandleDataNode";
}

GetCandleDataAtIndexNode::GetCandleDataAtIndexNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "Get Candle", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::INT, "Timestamp"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Open"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "High"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Low"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Close"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::FLOAT, "Volume"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::FLOAT, "Mcap"));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(120, 230);
    description = "Retrieves complete OHLCV data from a specific candle by its index.\n\nUse this node to:\n- Access historical price data\n- Compare current price with past prices\n- Calculate indicators over multiple candles\n- Build lookback periods for your strategy\n\nCombine with 'For Loop' to iterate through multiple candles and build arrays of historical data.\n\nExample: Get Candle (Index: Current Index - 20) gives you the candle from 20 periods ago.\n\nReturns zero values if the index is out of range.";
}

void GetCandleDataAtIndexNode::execute() {
    int index = inputs[0]->getValue<int>();
    if (blueprintManager && blueprintManager->GetDataManager() && blueprintManager->GetDataManager()->getPublicData().size() > index)
    {
        auto candle = blueprintManager->GetDataManager()->getPublicData().at(index);
        outputs[0]->setValue(index);
        outputs[1]->setValue(candle.timestamp);
        outputs[2]->setValue(candle.open);
        outputs[3]->setValue(candle.high);
        outputs[4]->setValue(candle.low);
        outputs[5]->setValue(candle.close);
        outputs[6]->setValue(candle.volume);
        outputs[7]->setValue(candle.mcap);
    }
    else
    {
        outputs[0]->setValue(0);
        outputs[1]->setValue(0.0L);
        outputs[2]->setValue(0.0L);
        outputs[3]->setValue(0.0L);
        outputs[4]->setValue(0.0L);
        outputs[5]->setValue(0.0L);
        outputs[6]->setValue(0.0f);
        outputs[7]->setValue(0.0f);
    }
}

std::string GetCandleDataAtIndexNode::getNodeType() const {
    return "GetCandleDataAtIndexNode";
}

GetCurrentCandleIndexNode::GetCurrentCandleIndexNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "Get Current Candle Index", dManager) {
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(200, 80);
    description = "Returns the index of the current (most recent) candle in the dataset.\n\nThis is useful when you need to:\n- Calculate relative positions (e.g., Current Index - 10 for 10 candles ago)\n- Know how many candles are available in your dataset\n- Reference the latest candle position for chart annotations\n\nThe index is zero-based, so:\n- Index 0 = first candle\n- Index N-1 = current candle (where N is total candles)\n\nTip: Use this with math nodes to access candles at relative positions like 'Current - 5' for 5 candles ago.";
}

void GetCurrentCandleIndexNode::execute() {

    if (blueprintManager && blueprintManager->GetDataManager() && blueprintManager->GetDataManager()->GetChart())
    {
        if (blueprintManager->GetDataManager()->getPublicData().size() > 0)
        {
            auto candle = blueprintManager->GetDataManager()->getPublicData().back();
            int index = blueprintManager->GetDataManager()->getPublicData().size() - 1;
            outputs[0]->setValue(index);
        }
    }
}

std::string GetCurrentCandleIndexNode::getNodeType() const {
    return "GetCurrentCandleIndexNode";
}

SMANode::SMANode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "SMA", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Period"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "SMA"));

    inputs[0]->setDefaultValue(20);

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(140, 100);
    description = "Simple Moving Average\n\n"
        "Calculates the average of closing prices over a specified period.\n"
        "Good for identifying trends and support/resistance levels.\n\n"
        "- Period: Number of candles (default: 20)\n"
        "- Index: Candle index (default: 0 = last candle)\n\n";

    inputs[0]->description = "Number of candles to calculate the Simple Moving Average (SMA) (default: 20).";

    inputs[1]->description = "Index of the current candle from which the SMA calculation starts.";

    outputs[0]->description = "Calculated Simple Moving Average (SMA) value for the current candle.";

}

void SMANode::execute() {
    int period = inputs[0]->getValue<int>();
    int index = inputs[1]->getValue<int>();

    if (blueprintManager && blueprintManager->GetDataManager()) {
        auto& candles = blueprintManager->GetDataManager()->getPublicData();

        if (index == 0) index = blueprintManager->GetDataManager()->getPublicData().size() - 1;

        if (index < period - 1 || index >= candles.size() || period <= 0) {
            outputs[0]->setValue<long double>(0.0);
            return;
        }

        double sum = 0.0;
        for (int i = 0; i < period; i++) {
            sum += candles[index - i].close;
        }
        outputs[0]->setValue<long double>(sum / period);
    }
}

std::string SMANode::getNodeType() const { return "SMANode"; }

EMANode::EMANode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "EMA", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Period"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "EMA"));

    inputs[0]->setDefaultValue<int>(12);

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(140, 100);
    description = "Exponential Moving Average\n\n"
        "Gives more weight to recent prices, making it more responsive than SMA.\n"
        "Used in MACD and as dynamic support/resistance.\n\n"
        "- Period: Number of candles (default: 12)\n"
        "- Index: Candle index (default: 0 = last candle)\n\n";

    inputs[0]->description = "Number of candles to calculate the Exponential Moving Average (EMA) (default: 12).";
    inputs[1]->description = "Index of the current candle from which the EMA calculation starts (0 = last candle).";
    outputs[0]->description = "Calculated Exponential Moving Average (EMA) value for the current candle.";
}

void EMANode::execute() {
    int period = inputs[0]->getValue<int>();
    int index = inputs[1]->getValue<int>();

    if (blueprintManager && blueprintManager->GetDataManager()) {
        auto& candles = blueprintManager->GetDataManager()->getPublicData();

        if (index == 0)
            index = static_cast<int>(candles.size()) - 1;

        if (index < period - 1 || index >= static_cast<int>(candles.size()) || period <= 0) {
            outputs[0]->setValue<long double>(0.0L);
            return;
        }

        long double multiplier = 2.0L / (period + 1);

        long double sum = 0.0L;
        for (int i = 0; i < period; i++) {
            sum += static_cast<long double>(candles[i].close);
        }
        long double ema = sum / static_cast<long double>(period);

        for (int i = period; i <= index; i++) {
            ema = (static_cast<long double>(candles[i].close) * multiplier) +
                (ema * (1.0L - multiplier));
        }

        outputs[0]->setValue<long double>(ema);
    }
}

std::string EMANode::getNodeType() const { return "EMANode"; }

RSINode::RSINode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "RSI", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Period"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "RSI"));

    inputs[0]->setDefaultValue<int>(14);

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(140, 100);
    description = "Relative Strength Index\n\n"
        "Momentum oscillator (0-100) measuring speed and magnitude of price changes.\n\n"
        "Signals:\n"
        ">70 = Overbought (potential sell)\n"
        "<30 = Oversold (potential buy)\n"
        "50 = Neutral\n\n"
        "Period: Lookback period (default: 14)\n"
        "Index: Candle index (0 = last candle)\n\n";

    inputs[0]->description = "Number of candles to look back when calculating the Relative Strength Index (RSI) (default: 14).";
    inputs[1]->description = "Index of the current candle from which the RSI calculation starts (0 = last candle).";
    outputs[0]->description = "Calculated Relative Strength Index (RSI) value (0-100) indicating overbought, oversold, or neutral conditions.";
}

void RSINode::execute() {
    int period = inputs[0]->getValue<int>();
    int index = inputs[1]->getValue<int>();

    if (blueprintManager && blueprintManager->GetDataManager()) {
        auto& candles = blueprintManager->GetDataManager()->getPublicData();

        if (index == 0)
            index = static_cast<int>(candles.size()) - 1;

        if (index < period || index >= static_cast<int>(candles.size()) || period <= 0) {
            outputs[0]->setValue<long double>(50.0L);
            return;
        }

        long double gains = 0.0L, losses = 0.0L;
        for (int i = 1; i <= period; i++) {
            long double change = static_cast<long double>(candles[index - i + 1].close - candles[index - i].close);
            if (change > 0)
                gains += change;
            else
                losses -= change;
        }

        long double avgGain = gains / static_cast<long double>(period);
        long double avgLoss = losses / static_cast<long double>(period);

        if (avgLoss == 0.0L) {
            outputs[0]->setValue<long double>(100.0L);
        }
        else {
            long double rs = avgGain / avgLoss;
            outputs[0]->setValue<long double>(100.0L - (100.0L / (1.0L + rs)));
        }
    }
}


std::string RSINode::getNodeType() const { return "RSINode"; }

MACDNode::MACDNode(const std::string& guid, BlueprintManager* dManager) : PureNode(guid, "MACD", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Fast"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Slow"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "MACD Line"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Fast EMA"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Slow EMA"));

    inputs[0]->setDefaultValue<int>(12);
    inputs[1]->setDefaultValue<int>(26);

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 140);
    description = "Moving Average Convergence Divergence\n\n"
        "Shows relationship between two moving averages.\n"
        "MACD Line = Fast EMA - Slow EMA\n\n"
        "Signals:\n"
        "- Crossover above 0 = Bullish\n"
        "- Crossover below 0 = Bearish\n\n"
        "Default: Fast=12, Slow=26\n"
        "Index: Candle index (0 = last candle)\n\n";

    inputs[0]->description = "Fast EMA period for MACD calculation (default: 12).";
    inputs[1]->description = "Slow EMA period for MACD calculation (default: 26).";
    inputs[2]->description = "Index of the current candle from which the MACD calculation starts (0 = last candle).";

    outputs[0]->description = "MACD Line value (Fast EMA - Slow EMA) for the current candle.";
    outputs[1]->description = "Calculated Fast EMA value for the current candle.";
    outputs[2]->description = "Calculated Slow EMA value for the current candle.";
}

void MACDNode::execute() {
    int fastPeriod = inputs[0]->getValue<int>();
    int slowPeriod = inputs[1]->getValue<int>();
    int index = inputs[2]->getValue<int>();

    if (blueprintManager && blueprintManager->GetDataManager()) {
        auto& candles = blueprintManager->GetDataManager()->getPublicData();

        if (index == 0)
            index = static_cast<int>(candles.size()) - 1;

        if (index < slowPeriod - 1 || index >= static_cast<int>(candles.size()) ||
            fastPeriod <= 0 || slowPeriod <= 0) {
            outputs[0]->setValue<long double>(0.0L);
            outputs[1]->setValue<long double>(0.0L);
            outputs[2]->setValue<long double>(0.0L);
            return;
        }

        long double fastMultiplier = 2.0L / (fastPeriod + 1);
        long double slowMultiplier = 2.0L / (slowPeriod + 1);

        long double sumFast = 0.0L;
        for (int i = 0; i < fastPeriod; i++) {
            sumFast += static_cast<long double>(candles[i].close);
        }
        long double fastEma = sumFast / static_cast<long double>(fastPeriod);

        for (int i = fastPeriod; i <= index; i++) {
            fastEma = (static_cast<long double>(candles[i].close) * fastMultiplier) +
                (fastEma * (1.0L - fastMultiplier));
        }

        long double sumSlow = 0.0L;
        for (int i = 0; i < slowPeriod; i++) {
            sumSlow += static_cast<long double>(candles[i].close);
        }
        long double slowEma = sumSlow / static_cast<long double>(slowPeriod);

        for (int i = slowPeriod; i <= index; i++) {
            slowEma = (static_cast<long double>(candles[i].close) * slowMultiplier) +
                (slowEma * (1.0L - slowMultiplier));
        }

        outputs[0]->setValue<long double>(fastEma - slowEma);
        outputs[1]->setValue<long double>(fastEma);
        outputs[2]->setValue<long double>(slowEma);
    }
}

std::string MACDNode::getNodeType() const { return "MACDNode"; }

BollingerBandsNode::BollingerBandsNode(const std::string& guid, BlueprintManager* dManager)
    : PureNode(guid, "Bollinger Bands", dManager)
{
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Period"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "StdDev"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Upper"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Middle"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Lower"));

    inputs[0]->setDefaultValue<int>(20);
    inputs[1]->setDefaultValue<long double>(2.0L);

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 140);
    description = "Bollinger Bands\n\n"
        "Volatility indicator with 3 bands:\n"
        "- Upper = SMA + (StdDev × multiplier)\n"
        "- Middle = SMA\n"
        "- Lower = SMA - (StdDev × multiplier)\n\n"
        "Price touching bands often signals reversal.\n"
        "Band squeeze indicates low volatility.\n\n"
        "Defaults: Period=20, StdDev=2.0\n"
        "Index: Candle index (0 = last candle)\n\n";

    inputs[0]->description = "Lookback period for SMA calculation (default: 20).";
    inputs[1]->description = "Standard deviation multiplier for the bands (default: 2.0).";
    inputs[2]->description = "Index of the current candle from which the Bollinger Bands calculation starts (0 = last candle).";

    outputs[0]->description = "Upper Bollinger Band value (SMA + StdDev × multiplier) for the current candle.";
    outputs[1]->description = "Middle Bollinger Band value (SMA) for the current candle.";
    outputs[2]->description = "Lower Bollinger Band value (SMA - StdDev × multiplier) for the current candle.";
}

void BollingerBandsNode::execute() {
    int period = inputs[0]->getValue<int>();
    long double stdDevMultiplier = inputs[1]->getValue<long double>();
    int index = inputs[2]->getValue<int>();

    if (blueprintManager && blueprintManager->GetDataManager()) {
        auto& candles = blueprintManager->GetDataManager()->getPublicData();

        if (index == 0)
            index = static_cast<int>(candles.size()) - 1;

        if (index < period - 1 || index >= static_cast<int>(candles.size()) || period <= 0) {
            outputs[0]->setValue<long double>(0.0L);
            outputs[1]->setValue<long double>(0.0L);
            outputs[2]->setValue<long double>(0.0L);
            return;
        }

        long double sum = 0.0L;
        for (int i = 0; i < period; i++) {
            sum += static_cast<long double>(candles[index - i].close);
        }
        long double sma = sum / static_cast<long double>(period);

        long double sumSquaredDiff = 0.0L;
        for (int i = 0; i < period; i++) {
            long double diff = static_cast<long double>(candles[index - i].close) - sma;
            sumSquaredDiff += diff * diff;
        }
        long double stdDev = sqrt(sumSquaredDiff / static_cast<long double>(period));

        outputs[0]->setValue<long double>(sma + (stdDev * stdDevMultiplier));  
        outputs[1]->setValue<long double>(sma);                                
        outputs[2]->setValue<long double>(sma - (stdDev * stdDevMultiplier));  
    }
}


std::string BollingerBandsNode::getNodeType() const { return "BollingerBandsNode"; }

StochasticNode::StochasticNode(const std::string& guid, BlueprintManager* dManager)
    : PureNode(guid, "Stochastic", dManager)
{
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "K Period"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "D Period"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "%K"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "%D"));

    inputs[0]->setDefaultValue<int>(14);
    inputs[1]->setDefaultValue<int>(3);

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 120);
    description = "Stochastic Oscillator\n\n"
        "Momentum indicator comparing close to price range.\n\n"
        "%K = Fast line (more sensitive)\n"
        "%D = Slow line (smoothed %K)\n\n"
        "Signals:\n"
        ">80 = Overbought\n"
        "<20 = Oversold\n"
        "K/D crossovers = Buy/Sell signals\n\n"
        "Defaults: K=14, D=3";

    inputs[0]->description = "Lookback period for %K calculation (fast line) (default: 14).";
    inputs[1]->description = "Smoothing period for %D calculation (slow line, SMA of %K) (default: 3).";
    inputs[2]->description = "Index of the current candle from which the Stochastic calculation starts (0 = last candle).";

    outputs[0]->description = "Calculated %K value (fast stochastic line) for the current candle.";
    outputs[1]->description = "Calculated %D value (smoothed %K line) for the current candle.";
}

void StochasticNode::execute() {
    int kPeriod = inputs[0]->getValue<int>();
    int dPeriod = inputs[1]->getValue<int>();
    int index = inputs[2]->getValue<int>();

    if (blueprintManager && blueprintManager->GetDataManager()) {
        auto& candles = blueprintManager->GetDataManager()->getPublicData();
        if (candles.empty()) return;

        if (index == 0)
            index = static_cast<int>(candles.size()) - 1;

        if (index < kPeriod - 1 || index >= static_cast<int>(candles.size()) || kPeriod <= 0 || dPeriod <= 0) {
            outputs[0]->setValue<long double>(50.0L);
            outputs[1]->setValue<long double>(50.0L);
            return;
        }

        long double highestHigh = candles[index].high;
        long double lowestLow = candles[index].low;

        for (int i = 1; i < kPeriod; i++) {
            if (candles[index - i].high > highestHigh) highestHigh = candles[index - i].high;
            if (candles[index - i].low < lowestLow) lowestLow = candles[index - i].low;
        }

        long double k;
        if (highestHigh == lowestLow)
            k = 50.0L;
        else
            k = ((candles[index].close - lowestLow) / (highestHigh - lowestLow)) * 100.0L;

        if (index < kPeriod + dPeriod - 2) {
            outputs[0]->setValue<long double>(k);
            outputs[1]->setValue<long double>(50.0L);
            return;
        }

        long double sumK = 0.0L;
        for (int i = 0; i < dPeriod; i++) {
            int currIndex = index - i;
            long double currHighest = candles[currIndex].high;
            long double currLowest = candles[currIndex].low;

            for (int j = 1; j < kPeriod; j++) {
                if (candles[currIndex - j].high > currHighest) currHighest = candles[currIndex - j].high;
                if (candles[currIndex - j].low < currLowest) currLowest = candles[currIndex - j].low;
            }

            if (currHighest != currLowest)
                sumK += ((candles[currIndex].close - currLowest) / (currHighest - currLowest)) * 100.0L;
            else
                sumK += 50.0L;
        }

        outputs[0]->setValue<long double>(k);
        outputs[1]->setValue<long double>(sumK / static_cast<long double>(dPeriod));
    }
}


std::string StochasticNode::getNodeType() const { return "StochasticNode"; }

ATRNode::ATRNode(const std::string& guid, BlueprintManager* dManager)
    : PureNode(guid, "ATR", dManager)
{
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Period"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "ATR"));

    inputs[0]->setDefaultValue<int>(14);

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(140, 100);
    description = "Average True Range\n\n"
        "Measures market volatility.\n"
        "Higher ATR = Higher volatility\n\n"
        "Uses:\n"
        "- Stop-loss placement\n"
        "- Position sizing\n"
        "- Volatility breakouts\n\n"
        "Period: Lookback period (default: 14)\n"
        "Index: Candle index (0 = last candle)\n\n";

    inputs[0]->description = "Number of candles to look back when calculating the Average True Range (ATR) (default: 14).";
    inputs[1]->description = "Index of the current candle from which the ATR calculation starts (0 = last candle).";
    outputs[0]->description = "Calculated Average True Range (ATR) value indicating market volatility over the specified period.";
}

void ATRNode::execute() {
    int period = inputs[0]->getValue<int>();
    int index = inputs[1]->getValue<int>();

    if (blueprintManager && blueprintManager->GetDataManager()) {
        auto& candles = blueprintManager->GetDataManager()->getPublicData();
        if (candles.empty()) return;

        if (index == 0)
            index = static_cast<int>(candles.size()) - 1;

        if (index < period || index >= static_cast<int>(candles.size()) || period <= 0) {
            outputs[0]->setValue<long double>(0.0L);
            return;
        }

        long double sum = 0.0L;
        for (int i = 1; i <= period; i++) {
            long double high = candles[index - i + 1].high;
            long double low = candles[index - i + 1].low;
            long double prevClose = candles[index - i].close;

            long double tr1 = high - low;
            long double tr2 = fabs(high - prevClose);
            long double tr3 = fabs(low - prevClose);

            long double trueRange = tr1;
            if (tr2 > trueRange) trueRange = tr2;
            if (tr3 > trueRange) trueRange = tr3;

            sum += trueRange;
        }

        outputs[0]->setValue<long double>(sum / static_cast<long double>(period));
    }
}


std::string ATRNode::getNodeType() const { 
    return "ATRNode"; 
}

CCINode::CCINode(const std::string& guid, BlueprintManager* dManager)
    : PureNode(guid, "CCI", dManager)
{
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Period"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "CCI"));

    inputs[0]->setDefaultValue<int>(20);

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(140, 100);
    description = "Commodity Channel Index\n\n"
        "Identifies cyclical trends.\n\n"
        "Signals:\n"
        ">+100 = Overbought\n"
        "<-100 = Oversold\n"
        "Zero crossings = Trend changes\n\n"
        "Period: Lookback period (default: 20)\n"
        "Index: Candle index (0 = last candle)\n\n";

    inputs[0]->description = "Number of candles to look back when calculating the Commodity Channel Index (CCI) (default: 20).";
    inputs[1]->description = "Index of the current candle from which the CCI calculation starts (0 = last candle).";
    outputs[0]->description = "Calculated Commodity Channel Index (CCI) value indicating overbought, oversold, or trend change conditions.";
}

void CCINode::execute() {
    int period = inputs[0]->getValue<int>();
    int index = inputs[1]->getValue<int>();

    if (blueprintManager && blueprintManager->GetDataManager()) {
        auto& candles = blueprintManager->GetDataManager()->getPublicData();
        if (candles.empty()) return;

        if (index == 0)
            index = static_cast<int>(candles.size()) - 1;

        if (index < period - 1 || index >= static_cast<int>(candles.size()) || period <= 0) {
            outputs[0]->setValue<long double>(0.0L);
            return;
        }

        long double sumTP = 0.0L;
        for (int i = 0; i < period; i++) {
            int idx = index - i;
            long double tp = (candles[idx].high + candles[idx].low + candles[idx].close) / 3.0L;
            sumTP += tp;
        }
        long double smaTP = sumTP / static_cast<long double>(period);

        long double sumDeviation = 0.0L;
        for (int i = 0; i < period; i++) {
            int idx = index - i;
            long double tp = (candles[idx].high + candles[idx].low + candles[idx].close) / 3.0L;
            sumDeviation += fabs(tp - smaTP);
        }
        long double meanDeviation = sumDeviation / static_cast<long double>(period);

        if (meanDeviation == 0.0L) {
            outputs[0]->setValue<long double>(0.0L);
            return;
        }

        long double currentTP = (candles[index].high + candles[index].low + candles[index].close) / 3.0L;
        long double cci = (currentTP - smaTP) / (0.015L * meanDeviation);

        outputs[0]->setValue<long double>(cci);
    }
}


std::string CCINode::getNodeType() const { return "CCINode"; }

WilliamsRNode::WilliamsRNode(const std::string& guid, BlueprintManager* dManager)
    : PureNode(guid, "Williams %R", dManager)
{
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Period"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "%R"));

    inputs[0]->setDefaultValue<int>(14);

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(140, 100);
    description = "Williams Percent Range\n\n"
        "Momentum indicator showing overbought/oversold levels.\n"
        "Scale: 0 to -100 (inverted)\n\n"
        "Signals:\n"
        ">-20 = Overbought\n"
        "<-80 = Oversold\n\n"
        "Period: Lookback period (default: 14)\n"
        "Index: Candle index (0 = last candle)\n\n";

    inputs[0]->description = "Number of candles to look back when calculating Williams %R (default: 14).";
    inputs[1]->description = "Index of the current candle from which the %R calculation starts (0 = last candle).";
    outputs[0]->description = "Calculated Williams %R value (0 to -100) indicating overbought or oversold conditions.";
}

void WilliamsRNode::execute() {
    int period = inputs[0]->getValue<int>();
    int index = inputs[1]->getValue<int>();

    if (blueprintManager && blueprintManager->GetDataManager()) {
        auto& candles = blueprintManager->GetDataManager()->getPublicData();
        if (candles.empty()) return;

        if (index == 0)
            index = static_cast<int>(candles.size()) - 1;

        if (index < period - 1 || index >= static_cast<int>(candles.size()) || period <= 0) {
            outputs[0]->setValue<long double>(-50.0L);
            return;
        }

        long double highestHigh = candles[index].high;
        long double lowestLow = candles[index].low;

        for (int i = 1; i < period; i++) {
            if (candles[index - i].high > highestHigh) highestHigh = candles[index - i].high;
            if (candles[index - i].low < lowestLow) lowestLow = candles[index - i].low;
        }

        if (highestHigh == lowestLow) {
            outputs[0]->setValue<long double>(-50.0L);
            return;
        }

        long double williamsR = ((highestHigh - candles[index].close) / (highestHigh - lowestLow)) * -100.0L;
        outputs[0]->setValue<long double>(williamsR);
    }
}


std::string WilliamsRNode::getNodeType() const { return "WilliamsRNode"; }

MFINode::MFINode(const std::string& guid, BlueprintManager* dManager)
    : PureNode(guid, "MFI", dManager)
{
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Period"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "MFI"));

    inputs[0]->setDefaultValue<int>(14);

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(140, 100);
    description = "Money Flow Index\n\n"
        "Volume-weighted RSI (0-100).\n"
        "Combines price and volume to measure buying/selling pressure.\n\n"
        "Signals:\n"
        ">80 = Overbought\n"
        "<20 = Oversold\n\n"
        "Period: Lookback period (default: 14)\n"
        "Index: Candle index (0 = last candle)\n\n";

    inputs[0]->description = "Number of candles to look back when calculating the Money Flow Index (default: 14).";
    inputs[1]->description = "Index of the current candle from which the MFI calculation starts (0 = last candle).";
    outputs[0]->description = "Calculated Money Flow Index (MFI) value, ranging from 0 to 100, indicating overbought or oversold conditions.";
}

void MFINode::execute() {
    int period = inputs[0]->getValue<int>();
    int index = inputs[1]->getValue<int>();

    if (blueprintManager && blueprintManager->GetDataManager()) {
        auto& candles = blueprintManager->GetDataManager()->getPublicData();
        if (candles.empty()) return;

        if (index == 0)
            index = static_cast<int>(candles.size()) - 1;

        if (index < period || index >= static_cast<int>(candles.size()) || period <= 0) {
            outputs[0]->setValue<long double>(50.0L);
            return;
        }

        long double positiveFlow = 0.0L;
        long double negativeFlow = 0.0L;

        for (int i = 1; i <= period; i++) {
            int currIdx = index - i + 1;
            int prevIdx = currIdx - 1;

            long double currTP = (candles[currIdx].high + candles[currIdx].low + candles[currIdx].close) / 3.0L;
            long double prevTP = (candles[prevIdx].high + candles[prevIdx].low + candles[prevIdx].close) / 3.0L;
            long double moneyFlow = currTP * candles[currIdx].volume;

            if (currTP > prevTP) {
                positiveFlow += moneyFlow;
            }
            else if (currTP < prevTP) {
                negativeFlow += moneyFlow;
            }
        }

        if (negativeFlow == 0.0L) {
            outputs[0]->setValue<long double>(100.0L);
            return;
        }

        long double moneyFlowRatio = positiveFlow / negativeFlow;
        long double mfi = 100.0L - (100.0L / (1.0L + moneyFlowRatio));

        outputs[0]->setValue<long double>(mfi);
    }
}


std::string MFINode::getNodeType() const { return "MFINode"; }

VWAPNode::VWAPNode(const std::string& guid, BlueprintManager* dManager)
    : PureNode(guid, "VWAP", dManager)
{
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Period"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "VWAP"));

    inputs[0]->setDefaultValue<int>(20);

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(140, 100);
    description = "Volume Weighted Average Price\n\n"
        "Average price weighted by volume.\n"
        "Shows the true average price.\n\n"
        "Uses:\n"
        "- Fair value reference\n"
        "- Support/Resistance\n"
        "- Trend confirmation\n\n"
        "Period: Lookback period (default: 20)\n"
        "Index: Candle index (0 = last candle)\n\n";

    inputs[0]->description = "Number of candles to look back when calculating the Volume Weighted Average Price (default: 20).";
    inputs[1]->description = "Index of the current candle from which the VWAP calculation starts (0 = last candle).";
    outputs[0]->description = "Calculated Volume Weighted Average Price (VWAP) for the specified lookback period.";
}

void VWAPNode::execute() {
    int period = inputs[0]->getValue<int>();
    int index = inputs[1]->getValue<int>();

    if (blueprintManager && blueprintManager->GetDataManager()) {
        auto& candles = blueprintManager->GetDataManager()->getPublicData();
        if (candles.empty()) return;

        if (index == 0)
            index = static_cast<int>(candles.size()) - 1;

        if (index < period - 1 || index >= static_cast<int>(candles.size()) || period <= 0) {
            outputs[0]->setValue<long double>(0.0L);
            return;
        }

        long double totalVolume = 0.0L;
        long double totalPriceVolume = 0.0L;

        for (int i = 0; i < period; i++) {
            int idx = index - i;
            long double typicalPrice = (candles[idx].high + candles[idx].low + candles[idx].close) / 3.0L;
            long double volume = candles[idx].volume;

            totalPriceVolume += typicalPrice * volume;
            totalVolume += volume;
        }

        if (totalVolume > 0.0L) {
            outputs[0]->setValue<long double>(totalPriceVolume / totalVolume);
        }
        else {
            outputs[0]->setValue<long double>(0.0L);
        }
    }
}

std::string VWAPNode::getNodeType() const { return "VWAPNode"; }

HighestNode::HighestNode(const std::string& guid, BlueprintManager* dManager)
    : PureNode(guid, "Highest", dManager)
{
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Period"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Highest"));

    inputs[0]->setDefaultValue<int>(20);

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(140, 100);
    description = "Highest Price\n\n"
        "Finds the highest closing price over the specified period.\n\n"
        "Uses:\n"
        "- Resistance levels\n"
        "- Breakout detection\n"
        "- Used in Stochastic, Williams %R\n\n"
        "Period: Lookback period (default: 20)\n"
        "Index: Candle index (0 = last candle)\n\n";

    inputs[0]->description = "Number of candles to look back when searching for the highest closing price (default: 20).";
    inputs[1]->description = "Index of the current candle from which the lookback period is calculated (0 = last candle).";
    outputs[0]->description = "Highest closing price found within the specified lookback period.";
}

void HighestNode::execute() {
    int period = inputs[0]->getValue<int>();
    int index = inputs[1]->getValue<int>();

    if (blueprintManager && blueprintManager->GetDataManager()) {
        auto& candles = blueprintManager->GetDataManager()->getPublicData();
        if (candles.empty()) return;

        if (index == 0)
            index = static_cast<int>(candles.size()) - 1;

        if (index < period - 1 || index >= static_cast<int>(candles.size()) || period <= 0) {
            outputs[0]->setValue<long double>(0.0L);
            return;
        }

        long double highest = candles[index].close;
        for (int i = 1; i < period; i++) {
            if (candles[index - i].close > highest) {
                highest = candles[index - i].close;
            }
        }

        outputs[0]->setValue<long double>(highest);
    }
}


std::string HighestNode::getNodeType() const { return "HighestNode"; }

LowestNode::LowestNode(const std::string& guid, BlueprintManager* dManager)
    : PureNode(guid, "Lowest", dManager)
{
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Period"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Lowest"));

    inputs[0]->setDefaultValue<int>(20);

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(140, 100);
    description = "Lowest Price\n\n"
        "Finds the lowest closing price over the specified period.\n\n"
        "Uses:\n"
        "- Support levels\n"
        "- Breakdown detection\n"
        "- Used in Stochastic, Williams %R\n\n"
        "Period: Lookback period (default: 20)\n"
        "Index: Candle index (0 = last candle)\n\n";

    inputs[0]->description = "Number of candles to look back when searching for the lowest closing price (default: 20).";
    inputs[1]->description = "Index of the current candle from which the lookback period is calculated (0 = last candle).";
    outputs[0]->description = "Lowest closing price found within the specified lookback period.";
}

void LowestNode::execute() {
    int period = inputs[0]->getValue<int>();
    int index = inputs[1]->getValue<int>();

    if (blueprintManager && blueprintManager->GetDataManager()) {
        auto& candles = blueprintManager->GetDataManager()->getPublicData();
        if (candles.empty()) return;

        if (index == 0)
            index = static_cast<int>(candles.size()) - 1;

        if (index < period - 1 || index >= static_cast<int>(candles.size()) || period <= 0) {
            outputs[0]->setValue<long double>(0.0L);
            return;
        }

        long double lowest = candles[index].close;
        for (int i = 1; i < period; i++) {
            if (candles[index - i].close < lowest) {
                lowest = candles[index - i].close;
            }
        }

        outputs[0]->setValue<long double>(lowest);
    }
}


std::string LowestNode::getNodeType() const { return "LowestNode"; }

DojiPatternNode::DojiPatternNode(const std::string& guid, BlueprintManager* dManager)
    : PureNode(guid, "Doji Pattern", dManager)
{
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Threshold"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, "Is Doji"));

    inputs[1]->setDefaultValue<long double>(0.1);

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(150, 100);
    description = "Doji Candle Pattern\n\n"
        "Detects Doji candles (open ? close).\n"
        "Signals indecision in the market.\n\n"
        "Strong reversal signal when appears at:\n"
        "- Trend tops/bottoms\n"
        "- Support/resistance levels\n\n"
        "Threshold: Body/Range ratio (default: 0.1)\n"
        "Index: Candle index (0 = last candle)\n\n";

    inputs[0]->description = "Index of the candle to check for a Doji pattern (0 = last candle).";
    inputs[1]->description = "Threshold ratio between body size and total candle range (default: 0.1). Smaller values mean stricter Doji detection.";
    outputs[0]->description = "True if the current candle forms a Doji pattern (open and close are nearly equal, indicating market indecision).";
}

void DojiPatternNode::execute() {
    int index = inputs[0]->getValue<int>();
    double threshold = inputs[1]->getValue<long double>();

    if (blueprintManager && blueprintManager->GetDataManager()) {
        auto& candles = blueprintManager->GetDataManager()->getPublicData();
        if (candles.empty()) {
            outputs[0]->setValue<bool>(false);
            return;
        }

        if (index == 0)
            index = static_cast<int>(candles.size()) - 1;

        if (index < 0 || index >= static_cast<int>(candles.size())) {
            outputs[0]->setValue<bool>(false);
            return;
        }

        auto& candle = candles[index];
        double bodySize = fabs(candle.close - candle.open);
        double candleRange = candle.high - candle.low;

        if (candleRange == 0.0) {
            outputs[0]->setValue<bool>(false);
            return;
        }

        bool isDoji = (bodySize / candleRange) <= threshold;
        outputs[0]->setValue<bool>(isDoji);
    }
}


std::string DojiPatternNode::getNodeType() const { return "DojiPatternNode"; }

HammerPatternNode::HammerPatternNode(const std::string& guid, BlueprintManager* dManager)
    : PureNode(guid, "Hammer Pattern", dManager)
{
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, "Is Hammer"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, "Is Shooting Star"));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(160, 100);
    description = "Hammer/Shooting Star Pattern\n\n"
        "Hammer: Bullish reversal pattern\n"
        "- Long lower shadow (2x body)\n"
        "- Small upper shadow\n"
        "- Appears at bottoms\n\n"
        "Shooting Star: Bearish reversal\n"
        "- Long upper shadow (2x body)\n"
        "- Small lower shadow\n"
        "- Appears at tops\n\n"
        "Index: Candle index (0 = last candle)";

    inputs[0]->description = "Index of the candle to check for hammer or shooting star pattern (0 = last candle).";

    outputs[0]->description = "True if the current candle forms a Hammer pattern (long lower shadow, small upper shadow, appears at bottoms).";
    outputs[1]->description = "True if the current candle forms a Shooting Star pattern (long upper shadow, small lower shadow, appears at tops).";
}

void HammerPatternNode::execute() {
    int index = inputs[0]->getValue<int>();

    if (blueprintManager && blueprintManager->GetDataManager()) {
        auto& candles = blueprintManager->GetDataManager()->getPublicData();
        if (candles.empty()) {
            outputs[0]->setValue<bool>(false);
            outputs[1]->setValue<bool>(false);
            return;
        }

        if (index == 0)
            index = static_cast<int>(candles.size()) - 1;

        if (index < 0 || index >= static_cast<int>(candles.size())) {
            outputs[0]->setValue<bool>(false);
            outputs[1]->setValue<bool>(false);
            return;
        }

        auto& candle = candles[index];
        double body = fabs(candle.close - candle.open);
        double upperBase = (candle.close > candle.open) ? candle.close : candle.open;
        double lowerBase = (candle.close < candle.open) ? candle.close : candle.open;

        double upperShadow = candle.high - upperBase;
        double lowerShadow = lowerBase - candle.low;

        bool isHammer = (lowerShadow >= 2.0 * body) && (upperShadow <= body * 0.5);
        bool isShootingStar = (upperShadow >= 2.0 * body) && (lowerShadow <= body * 0.5);

        outputs[0]->setValue<bool>(isHammer);
        outputs[1]->setValue<bool>(isShootingStar);
    }
}


std::string HammerPatternNode::getNodeType() const { return "HammerPatternNode"; }

EngulfingPatternNode::EngulfingPatternNode(const std::string& guid, BlueprintManager* dManager)
    : PureNode(guid, "Engulfing Pattern", dManager)
{
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, "Bullish Engulfing"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, "Bearish Engulfing"));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(200, 100);
    description = "Engulfing Pattern\n\n"
        "Strong reversal patterns:\n\n"
        "Bullish Engulfing:\n"
        "- Current green candle engulfs previous red\n"
        "- Appears at bottoms\n\n"
        "Bearish Engulfing:\n"
        "- Current red candle engulfs previous green\n"
        "- Appears at tops\n\n"
        "Index: Candle index (0 = last candle)";

    inputs[0]->description = "Index of the candle to check for engulfing pattern (0 = last candle).";

    outputs[0]->description = "True if a Bullish Engulfing pattern is detected (green candle fully engulfs the previous red candle).";
    outputs[1]->description = "True if a Bearish Engulfing pattern is detected (red candle fully engulfs the previous green candle).";
}

void EngulfingPatternNode::execute() {
    int index = inputs[0]->getValue<int>();

    if (blueprintManager && blueprintManager->GetDataManager()) {
        auto& candles = blueprintManager->GetDataManager()->getPublicData();
        if (candles.size() < 2) {
            outputs[0]->setValue<bool>(false);
            outputs[1]->setValue<bool>(false);
            return;
        }

        if (index == 0)
            index = static_cast<int>(candles.size()) - 1;

        if (index <= 0 || index >= static_cast<int>(candles.size())) {
            outputs[0]->setValue<bool>(false);
            outputs[1]->setValue<bool>(false);
            return;
        }

        auto& prev = candles[index - 1];
        auto& curr = candles[index];

        bool prevBearish = prev.close < prev.open;
        bool currBullish = curr.close > curr.open;
        bool bullishEngulfs = (curr.open < prev.close) && (curr.close > prev.open);

        bool prevBullish = prev.close > prev.open;
        bool currBearish = curr.close < curr.open;
        bool bearishEngulfs = (curr.open > prev.close) && (curr.close < prev.open);

        outputs[0]->setValue<bool>(prevBearish && currBullish && bullishEngulfs);
        outputs[1]->setValue<bool>(prevBullish && currBearish && bearishEngulfs);
    }
}


std::string EngulfingPatternNode::getNodeType() const { return "EngulfingPatternNode"; }

CenterOfGravityNode::CenterOfGravityNode(const std::string& guid, BlueprintManager* dManager)
    : PureNode(guid, "Center of Gravity", dManager)
{
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Index"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::INT, "Period"));

    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "CoG Value"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, "Cross Up"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, "Cross Down"));

    inputs[1]->setDefaultValue<int>(10);

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(200, 120);

    description = "Center of Gravity (CoG)\n\n"
        "Weighted average price indicator where\n"
        "recent prices have greater weight.\n\n"
        "How it works:\n"
        "Price oscillates around its center of gravity\n"
        "like a pendulum. When price deviates too far\n"
        "from the center, it tends to return back.\n\n"
        "Signals:\n"
        "• Cross Up: Price crossed CoG upward BUY\n"
        "• Cross Down: Price crossed CoG downward SELL\n\n";

    inputs[0]->description = "Candle index (0 = last candle)";
    inputs[1]->description = "Period for calculation (default: 10)";
    outputs[0]->description = "Center of Gravity value";
    outputs[1]->description = "True if price crossed CoG upward";
    outputs[2]->description = "True if price crossed CoG downward";
}

void CenterOfGravityNode::execute() {
    int index = inputs[0]->getValue<int>();
    int period = inputs[1]->getValue<int>();

    if (period <= 0) period = 10;   

    if (blueprintManager && blueprintManager->GetDataManager()) {
        auto& candles = blueprintManager->GetDataManager()->getPublicData();

        if (candles.size() < static_cast<size_t>(period + 1)) {
            outputs[0]->setValue<long double>(0.0);
            outputs[1]->setValue<bool>(false);
            outputs[2]->setValue<bool>(false);
            return;
        }

        if (index == 0)
            index = static_cast<int>(candles.size()) - 1;

        if (index < period || index >= static_cast<int>(candles.size())) {
            outputs[0]->setValue<long double>(0.0);
            outputs[1]->setValue<bool>(false);
            outputs[2]->setValue<bool>(false);
            return;
        }

        double numerator = 0.0;
        double denominator = 0.0;

        for (int i = 0; i < period; i++) {
            double price = candles[index - i].close;
            int weight = period - i;       
            numerator += price * weight;
            denominator += weight;
        }

        double currentCoG = (denominator != 0.0) ? (numerator / denominator) : 0.0;

        numerator = 0.0;
        denominator = 0.0;

        for (int i = 0; i < period; i++) {
            double price = candles[index - 1 - i].close;
            int weight = period - i;
            numerator += price * weight;
            denominator += weight;
        }

        double prevCoG = (denominator != 0.0) ? (numerator / denominator) : 0.0;

        double currentPrice = candles[index].close;
        double prevPrice = candles[index - 1].close;

        bool crossUp = (prevPrice <= prevCoG) && (currentPrice > currentCoG);
        bool crossDown = (prevPrice >= prevCoG) && (currentPrice < currentCoG);

        outputs[0]->setValue<long double>(currentCoG);
        outputs[1]->setValue<bool>(crossUp);
        outputs[2]->setValue<bool>(crossDown);
    }
}

std::string CenterOfGravityNode::getNodeType() const { return "CenterOfGravityNode"; }

BranchNode::BranchNode(const std::string& guid, BlueprintManager* dManager) : ExecNode(guid, "Branch", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::BOOL, ""));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, "True"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, "False"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(120, 110);
    description = "Conditional branch. Checks a boolean condition and executes one of two paths.\n\nIf condition is True - executes the top branch, if False - executes the bottom branch.\n\nUsed to create 'if-then-else' logic in your program.";
}

void BranchNode::execute() {

    bool condition = inputs[1]->getValue<bool>();
    outputs[2]->setValue(condition);

    int branch_index = condition ? 0 : 1;
    if (!outputs[branch_index]->connected_pins.empty()) {
        for (auto* connected_pin : outputs[branch_index]->connected_pins) {
            if (connected_pin->owner && !connected_pin->owner->isPure()) {
                static_cast<ExecNode*>(connected_pin->owner)->execute();
            }
        }
    }
}

std::string BranchNode::getNodeType() const {
    return "BranchNode";
}

SequenceNode::SequenceNode(const std::string& guid, BlueprintManager* dManager) : ExecNode(guid, "Sequence", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, "1"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, "2"));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(120, 110);
    description =
        "Sequence node.\n"
        "Executes multiple output paths in order, one after another.\n\n"
        "When triggered, this node will execute each connected output pin sequentially "
        "from top to bottom.\n\n"
        "Useful for performing a series of actions step-by-step.";

}

void SequenceNode::execute() {

    for (int i = 0; i < outputs.size(); i++)
        if (!outputs[i]->connected_pins.empty()) {
            for (auto* connected_pin : outputs[i]->connected_pins) {
                if (connected_pin->owner && !connected_pin->owner->isPure()) {
                    static_cast<ExecNode*>(connected_pin->owner)->execute();
                }
            }
        }
}

std::string SequenceNode::getNodeType() const {
    return "SequenceNode";
}

PrintString::PrintString(const std::string& guid, BlueprintManager* dManager) : ExecNode(guid, "Print String", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::STRING, "text"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;

    size = ImVec2(140, 100);
}

void PrintString::execute() {
    std::string output_text;
    output_text = inputs[1]->getValue<std::string>();

    std::cout << "[Print String]: " << output_text << std::endl;

    if (!outputs[0]->connected_pins.empty()) {
        for (auto* connected_pin : outputs[0]->connected_pins) {
            if (connected_pin->owner && !connected_pin->owner->isPure()) {
                static_cast<ExecNode*>(connected_pin->owner)->execute();
            }
        }
    }
}

std::string PrintString::getNodeType() const {
    return "PrintString";
}

nlohmann::json PrintString::serializeData() const
{
    nlohmann::json data;
    data["value"] = text;
    return data;
}

void PrintString::deserializeData(const nlohmann::json& data)
{
    if (data.contains("value")) {
        text = data["value"];
    }
}

BybiyMarginLongNode::BybiyMarginLongNode(const std::string& guid, BlueprintManager* dManager) : ExecNode(guid, "Bybit Margin Open Long", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Quantity"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Leverage"));


    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, "Success"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Avg Price"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Mark Price"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Liq Price"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Bust Price"));
    inputs[1]->default_stored_double = 1.0;
    inputs[2]->default_stored_double = 0.0;

    inputs[1]->description = "Quantity to buy";
    inputs[2]->description = "Leverage (0 - doesnt change current)";

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(180, 180);
}

void BybiyMarginLongNode::execute() {

    double quantity = inputs[1]->getValue<long double>();
    double leverage = inputs[2]->getValue<long double>();
    auto candle = blueprintManager->GetDataManager()->getPublicData().back();

    if (blueprintManager->GetDataManager()->RuntimeModeIsActive())
    {
        std::unique_ptr<BybitMargin> margin = std::make_unique<BybitMargin>();
        margin->setApiKeys(Config::getInstance().getBybitAPIKey(), Config::getInstance().getBybitSignature());
        margin->setTestnet(Config::getInstance().bybitIsDemoMode());
        if(leverage != 0.0) margin->setLeverage(blueprintManager->GetDataManager()->GetSymbol(), leverage);
        OrderResult longResult = margin->openLong(blueprintManager->GetDataManager()->GetSymbol(), quantity);
        PositionInfo position = margin->getPosition(blueprintManager->GetDataManager()->GetSymbol());

        outputs[1]->setValue<bool>(longResult.success);
        outputs[2]->setValue<long double>(position.avgPrice);
        outputs[3]->setValue<long double>(position.markPrice);
        outputs[4]->setValue<long double>(position.liqPrice);
        outputs[5]->setValue<long double>(position.bustPrice);

        if(longResult.success)
        {
            if (blueprintManager && blueprintManager->GetDataManager() && blueprintManager->GetDataManager()->GetRuntimeTradingStats()) {
                if (blueprintManager->GetDataManager()->getPublicData().size() > 0) {
                    blueprintManager->GetDataManager()->GetRuntimeTradingStats()->EnterLong(position.createdTime, position.avgPrice, quantity, position.leverage, position.liqPrice);
                }
            }
            
        }

        if (!outputs[0]->connected_pins.empty()) {
            for (auto* connected_pin : outputs[0]->connected_pins) {
                if (connected_pin->owner && !connected_pin->owner->isPure()) {
                    static_cast<ExecNode*>(connected_pin->owner)->execute();
                }
            }
        }
    }
    else
    {
        std::unique_ptr<LiquidationCalculator> calc = std::make_unique<LiquidationCalculator>();
        calc->setApiKeys(Config::getInstance().getBybitAPIKey(), Config::getInstance().getBybitSignature());
        calc->setTestnet(Config::getInstance().bybitIsDemoMode());
        double quickLiq  = calc->quickCalculate(candle.close, leverage, true);

        if (blueprintManager && blueprintManager->GetDataManager() && blueprintManager->GetDataManager()->GetBacktestTradingStats()) {
            if (blueprintManager->GetDataManager()->getPublicData().size() > 0) {
                blueprintManager->GetDataManager()->GetBacktestTradingStats()->EnterLong(candle.timestamp, candle.close, quantity, leverage, quickLiq);
            }
        }

        Position longf = blueprintManager->GetDataManager()->GetBacktestTradingStats()->GetOpenLongPosition();

        outputs[1]->setValue<bool>(true);
        outputs[2]->setValue<long double>(longf.buyPrice);
        outputs[4]->setValue<long double>(longf.liquidationPrice);

        if (!outputs[0]->connected_pins.empty()) {
            for (auto* connected_pin : outputs[0]->connected_pins) {
                if (connected_pin->owner && !connected_pin->owner->isPure()) {
                    static_cast<ExecNode*>(connected_pin->owner)->execute();
                }
            }
        }
    }

}

BybiyMarginShortNode::BybiyMarginShortNode(const std::string& guid, BlueprintManager* dManager) : ExecNode(guid, "Bybit Margin Short", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Quantity"));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Leverage"));

    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, "Success"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Avg Price"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Mark Price"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Liq Price"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Bust Price"));

    inputs[1]->default_stored_double = 1.0;
    inputs[2]->default_stored_double = 0.0;

    inputs[1]->description = "Quantity to sell short";
    inputs[2]->description = "Leverage (0 - doesnt change current)";

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(180, 150);
}

void BybiyMarginShortNode::execute() {

    double quantity = inputs[1]->getValue<long double>();
    double leverage = inputs[2]->getValue<long double>();
    auto candle = blueprintManager->GetDataManager()->getPublicData().back();

    if (blueprintManager->GetDataManager()->RuntimeModeIsActive())
    {
        BybitMargin margin;
        margin.setApiKeys(Config::getInstance().getBybitAPIKey(), Config::getInstance().getBybitSignature());
        margin.setTestnet(Config::getInstance().bybitIsDemoMode());
        if (leverage != 0.0) margin.setLeverage(blueprintManager->GetDataManager()->GetSymbol(), leverage);
        OrderResult shortResult = margin.openShort(blueprintManager->GetDataManager()->GetSymbol(), quantity);
        PositionInfo position = margin.getPosition(blueprintManager->GetDataManager()->GetSymbol());

        outputs[1]->setValue<bool>(shortResult.success);
        outputs[2]->setValue<long double>(position.avgPrice);
        outputs[3]->setValue<long double>(position.markPrice);
        outputs[4]->setValue<long double>(position.liqPrice);
        outputs[5]->setValue<long double>(position.bustPrice);

        if (shortResult.success)
        {
            if (blueprintManager && blueprintManager->GetDataManager() && blueprintManager->GetDataManager()->GetRuntimeTradingStats()) {
                if (blueprintManager->GetDataManager()->getPublicData().size() > 0) {
                    blueprintManager->GetDataManager()->GetRuntimeTradingStats()->EnterShort(position.createdTime, position.avgPrice, quantity, position.leverage, position.liqPrice);
                }
            }
        }

        if (!outputs[0]->connected_pins.empty()) {
            for (auto* connected_pin : outputs[0]->connected_pins) {
                if (connected_pin->owner && !connected_pin->owner->isPure()) {
                    static_cast<ExecNode*>(connected_pin->owner)->execute();
                }
            }
        }
    }
    else
    {
        LiquidationCalculator calc;
        calc.setApiKeys(Config::getInstance().getBybitAPIKey(), Config::getInstance().getBybitSignature());
        calc.setTestnet(Config::getInstance().bybitIsDemoMode());
        double quickLiq = calc.quickCalculate(candle.close, leverage, false);    

        if (blueprintManager && blueprintManager->GetDataManager() && blueprintManager->GetDataManager()->GetBacktestTradingStats()) {
            if (blueprintManager->GetDataManager()->getPublicData().size() > 0) {
                blueprintManager->GetDataManager()->GetBacktestTradingStats()->EnterShort(candle.timestamp, candle.close, quantity, leverage, quickLiq);
            }
        }

        Position shortf = blueprintManager->GetDataManager()->GetBacktestTradingStats()->GetOpenShortPosition();

        outputs[1]->setValue<bool>(true);
        outputs[2]->setValue<long double>(shortf.buyPrice);
        outputs[4]->setValue<long double>(shortf.liquidationPrice);

        if (!outputs[0]->connected_pins.empty()) {
            for (auto* connected_pin : outputs[0]->connected_pins) {
                if (connected_pin->owner && !connected_pin->owner->isPure()) {
                    static_cast<ExecNode*>(connected_pin->owner)->execute();
                }
            }
        }
    }
}

BybiyMarginCloseLongPositionNode::BybiyMarginCloseLongPositionNode(const std::string& guid, BlueprintManager* dManager) : ExecNode(guid, "Bybit Margin Close Long", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Percent"));

    inputs[1]->default_stored_double = 100.0;

    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, "Success"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Price"));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(200, 100);
}

void BybiyMarginCloseLongPositionNode::execute() {

    long double percent = inputs[1]->getValue<long double>();
    auto candle = blueprintManager->GetDataManager()->getPublicData().back();

    if (blueprintManager->GetDataManager()->RuntimeModeIsActive())
    {
        BybitMargin margin;
        margin.setApiKeys(Config::getInstance().getBybitAPIKey(), Config::getInstance().getBybitSignature());
        margin.setTestnet(Config::getInstance().bybitIsDemoMode());
        OrderResult closeResult = margin.closePosition(blueprintManager->GetDataManager()->GetSymbol());

        OrderHistoryResult history = margin.getOrderHistory("linear", blueprintManager->GetDataManager()->GetSymbol(), closeResult.orderId);
        outputs[1]->setValue<bool>(closeResult.success);
        outputs[2]->setValue<long double>(!history.orders.empty() ? history.orders[0].price : candle.close);

        if (closeResult.success)
        {
            if (blueprintManager && blueprintManager->GetDataManager() && blueprintManager->GetDataManager()->GetRuntimeTradingStats()) {
                if (blueprintManager->GetDataManager()->getPublicData().size() > 0) {
                    blueprintManager->GetDataManager()->GetRuntimeTradingStats()->CloseLong(candle.timestamp, !history.orders.empty() ? history.orders[0].price : candle.close, percent);
                }
            }
        }

        if (!outputs[0]->connected_pins.empty()) {
            for (auto* connected_pin : outputs[0]->connected_pins) {
                if (connected_pin->owner && !connected_pin->owner->isPure()) {
                    static_cast<ExecNode*>(connected_pin->owner)->execute();
                }
            }
        }
    }
    else
    {
        if (blueprintManager && blueprintManager->GetDataManager() && blueprintManager->GetDataManager()->GetBacktestTradingStats()) {
            if (blueprintManager->GetDataManager()->getPublicData().size() > 0) {
                blueprintManager->GetDataManager()->GetBacktestTradingStats()->CloseLong(candle.timestamp, candle.close, percent);
            }
        }

        outputs[1]->setValue<bool>(true);
        outputs[2]->setValue<long double>(candle.close);

        if (!outputs[0]->connected_pins.empty()) {
            for (auto* connected_pin : outputs[0]->connected_pins) {
                if (connected_pin->owner && !connected_pin->owner->isPure()) {
                    static_cast<ExecNode*>(connected_pin->owner)->execute();
                }
            }
        }
    }
}

BybiyMarginCloseShortPositionNode::BybiyMarginCloseShortPositionNode(const std::string& guid, BlueprintManager* dManager) : ExecNode(guid, "Bybit Margin Close Short", dManager) {
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    inputs.push_back(std::make_unique<PinIn>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Percent"));

    inputs[1]->default_stored_double = 100.0;

    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::EXEC, ""));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::BOOL, "Success"));
    outputs.push_back(std::make_unique<PinOut>(GUIDGenerator::generate(), guid, PinType::DOUBLE, "Price"));

    for (auto& pin : inputs) pin->owner = this;
    for (auto& pin : outputs) pin->owner = this;
    size = ImVec2(200, 100);
}

void BybiyMarginCloseShortPositionNode::execute() {

    long double percent = inputs[1]->getValue<long double>();
    auto candle = blueprintManager->GetDataManager()->getPublicData().back();

    if (blueprintManager->GetDataManager()->RuntimeModeIsActive())
    {
        BybitMargin margin;
        margin.setApiKeys(Config::getInstance().getBybitAPIKey(), Config::getInstance().getBybitSignature());
        margin.setTestnet(Config::getInstance().bybitIsDemoMode());
        OrderResult closeResult = margin.closePosition(blueprintManager->GetDataManager()->GetSymbol());

        OrderHistoryResult history = margin.getOrderHistory("linear", blueprintManager->GetDataManager()->GetSymbol(), closeResult.orderId);
        outputs[1]->setValue<bool>(closeResult.success);
        outputs[2]->setValue<long double>(!history.orders.empty() ? history.orders[0].price : candle.close);

        if (closeResult.success)
        {
            if (blueprintManager && blueprintManager->GetDataManager() && blueprintManager->GetDataManager()->GetRuntimeTradingStats()) {
                if (blueprintManager->GetDataManager()->getPublicData().size() > 0) {
                    blueprintManager->GetDataManager()->GetRuntimeTradingStats()->CloseShort(candle.timestamp, !history.orders.empty() ? history.orders[0].price : candle.close, percent);
                }
            }
        }

        if (!outputs[0]->connected_pins.empty()) {
            for (auto* connected_pin : outputs[0]->connected_pins) {
                if (connected_pin->owner && !connected_pin->owner->isPure()) {
                    static_cast<ExecNode*>(connected_pin->owner)->execute();
                }
            }
        }
    }
    else
    {
        if (blueprintManager && blueprintManager->GetDataManager() && blueprintManager->GetDataManager()->GetBacktestTradingStats()) {
            if (blueprintManager->GetDataManager()->getPublicData().size() > 0) {
                blueprintManager->GetDataManager()->GetBacktestTradingStats()->CloseShort(candle.timestamp, candle.close, percent);
            }
        }

        outputs[1]->setValue<bool>(true);
        outputs[2]->setValue<long double>(candle.close);

        if (!outputs[0]->connected_pins.empty()) {
            for (auto* connected_pin : outputs[0]->connected_pins) {
                if (connected_pin->owner && !connected_pin->owner->isPure()) {
                    static_cast<ExecNode*>(connected_pin->owner)->execute();
                }
            }
        }
    }
}