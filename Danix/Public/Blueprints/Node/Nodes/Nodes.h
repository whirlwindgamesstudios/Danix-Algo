#pragma once
#include "../../../../Public/Blueprints/Node/Node.h"
#include "../../../Systems/GuidGenerator.h"

class EntryNode : public ExecNode {
public:
    EntryNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::EXECUTION; }
};

class BranchNode : public ExecNode {
public:
    BranchNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::EXECUTION; }
};

class SequenceNode : public ExecNode {
public:
    SequenceNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::EXECUTION; }
};

class PrintString : public ExecNode {
private:
    std::string text;
public:
    PrintString(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    nlohmann::json serializeData() const override;
    void deserializeData(const nlohmann::json& data) override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::EXECUTION; }
};

class GetVariableNode : public PureNode {
private:
    std::string variableGuid;
public:
    GetVariableNode(const std::string& guid, BlueprintManager* dManager, const std::string& varGuid = "");
    void execute() override;
    std::string getNodeType() const override;
    void bindToVariable(const std::string& varGuid);
    const std::string& getVariableGuid() const;
    nlohmann::json serializeData() const override;
    void deserializeData(const nlohmann::json& data) override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::VARIABLE; }
};

class SetVariableNode : public ExecNode {
private:
    std::string variableGuid;
public:
    SetVariableNode(const std::string& guid, BlueprintManager* dManager, const std::string& varGuid = "");
    void execute() override;
    std::string getNodeType() const override;
    void bindToVariable(const std::string& varGuid);
    const std::string& getVariableGuid() const;
    nlohmann::json serializeData() const override;
    void deserializeData(const nlohmann::json& data) override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::VARIABLE; }
};

class IntNotEqualNode : public PureNode {
public:
    IntNotEqualNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::LOGIC; }
};

class IntEqualNode : public PureNode {
public:
    IntEqualNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::LOGIC; }
};

class IntLessThanNode : public PureNode {
public:
    IntLessThanNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::LOGIC; }
};

class IntMoreThanNode : public PureNode {
public:
    IntMoreThanNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::LOGIC; }
};

class DoubleNotEqualNode : public PureNode {
public:
    DoubleNotEqualNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::LOGIC; }
};

class DoubleEqualNode : public PureNode {
public:
    DoubleEqualNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::LOGIC; }
};

class DoubleLessThanNode : public PureNode {
public:
    DoubleLessThanNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::LOGIC; }
};

class DoubleMoreThanNode : public PureNode {
public:
    DoubleMoreThanNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::LOGIC; }
};

class FloatNotEqualNode : public PureNode {
public:
    FloatNotEqualNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::LOGIC; }
};

class FloatEqualNode : public PureNode {
public:
    FloatEqualNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::LOGIC; }
};

class FloatLessThanNode : public PureNode {
public:
    FloatLessThanNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::LOGIC; }
};

class FloatMoreThanNode : public PureNode {
public:
    FloatMoreThanNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::LOGIC; }
};

class BoolAndNode : public PureNode {
public:
    BoolAndNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::LOGIC; }
};

class BoolOrNode : public PureNode {
public:
    BoolOrNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::LOGIC; }
};

class BoolEqualNode : public PureNode {
public:
    BoolEqualNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::LOGIC; }
};

class BoolNotEqualNode : public PureNode {
public:
    BoolNotEqualNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::LOGIC; }
};

class IntToFloatNode : public PureNode {
public:
    IntToFloatNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CONVERSION; }
};

class IntToDoubleNode : public PureNode {
public:
    IntToDoubleNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CONVERSION; }
};

class IntToTextNode : public PureNode {
public:
    IntToTextNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CONVERSION; }
};

class DoubleToFloatNode : public PureNode {
public:
    DoubleToFloatNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CONVERSION; }
};

class DoubleToIntNode : public PureNode {
public:
    DoubleToIntNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CONVERSION; }
};

class DoubleToTextNode : public PureNode {
public:
    DoubleToTextNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CONVERSION; }
};

class FloatToDoubleNode : public PureNode {
public:
    FloatToDoubleNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CONVERSION; }
};

class FloatToIntNode : public PureNode {
public:
    FloatToIntNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CONVERSION; }
};

class FloatToTextNode : public PureNode {
public:
    FloatToTextNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CONVERSION; }
};

class BoolToTextNode : public PureNode {
public:
    BoolToTextNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CONVERSION; }
};

class ConstIntNode : public ConstValueNode {
public:
    ConstIntNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    float getValue() const { return intValue; }
    void setValue(float newValue) { intValue = newValue; }
    std::string getNodeType()const override;
    nlohmann::json serializeData() const;
    void deserializeData(const nlohmann::json& data);
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CONSTANT; }
};

class ConstFloatNode : public ConstValueNode {
public:
    ConstFloatNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    float getValue() const { return floatValue; }
    void setValue(float newValue) { floatValue = newValue; }
    std::string getNodeType()const override;
    nlohmann::json serializeData() const;
    void deserializeData(const nlohmann::json& data);
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CONSTANT; }
};

class ConstDoubleNode : public ConstValueNode {
public:
    ConstDoubleNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    float getValue() const { return doubleValue; }
    void setValue(float newValue) { doubleValue = newValue; }
    std::string getNodeType()const override;
    nlohmann::json serializeData() const;
    void deserializeData(const nlohmann::json& data);
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CONSTANT; }
};

class ConstBoolNode : public ConstValueNode {
public:
    ConstBoolNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    float getValue() const { return boolValue; }
    void setValue(float newValue) { boolValue = newValue; }
    std::string getNodeType()const override;
    nlohmann::json serializeData() const;
    void deserializeData(const nlohmann::json& data);
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CONSTANT; }
};

class ConstTextNode : public ConstValueNode {
public:
    ConstTextNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getValue() const { return stringValue; }
    void setValue(float newValue) { stringValue = newValue; }
    std::string getNodeType()const override;
    nlohmann::json serializeData() const;
    void deserializeData(const nlohmann::json& data);
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CONSTANT; }
};

class IntMinusIntNode : public PureNode {
public:
    IntMinusIntNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class IntMinusDoubleNode : public PureNode {
public:
    IntMinusDoubleNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class IntMinusFloatNode : public PureNode {
public:
    IntMinusFloatNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class IntPlusIntNode : public PureNode {
public:
    IntPlusIntNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class IntPlusDoubleNode : public PureNode {
public:
    IntPlusDoubleNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class IntPlusFloatNode : public PureNode {
public:
    IntPlusFloatNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class IntDivideIntNode : public PureNode {
public:
    IntDivideIntNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class IntDivideDoubleNode : public PureNode {
public:
    IntDivideDoubleNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class IntDivideFloatNode : public PureNode {
public:
    IntDivideFloatNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class IntMultiplyIntNode : public PureNode {
public:
    IntMultiplyIntNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class IntMultiplyDoubleNode : public PureNode {
public:
    IntMultiplyDoubleNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class IntMultiplyFloatNode : public PureNode {
public:
    IntMultiplyFloatNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class FloatMinusIntNode : public PureNode {
public:
    FloatMinusIntNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class FloatMinusDoubleNode : public PureNode {
public:
    FloatMinusDoubleNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class FloatMinusFloatNode : public PureNode {
public:
    FloatMinusFloatNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class FloatPlusIntNode : public PureNode {
public:
    FloatPlusIntNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class FloatPlusDoubleNode : public PureNode {
public:
    FloatPlusDoubleNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class FloatPlusFloatNode : public PureNode {
public:
    FloatPlusFloatNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class FloatDivideIntNode : public PureNode {
public:
    FloatDivideIntNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class FloatDivideDoubleNode : public PureNode {
public:
    FloatDivideDoubleNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class FloatDivideFloatNode : public PureNode {
public:
    FloatDivideFloatNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class FloatMultiplyIntNode : public PureNode {
public:
    FloatMultiplyIntNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class FloatMultiplyDoubleNode : public PureNode {
public:
    FloatMultiplyDoubleNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class FloatMultiplyFloatNode : public PureNode {
public:
    FloatMultiplyFloatNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class DoubleMinusIntNode : public PureNode {
public:
    DoubleMinusIntNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class DoubleMinusDoubleNode : public PureNode {
public:
    DoubleMinusDoubleNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class DoubleMinusFloatNode : public PureNode {
public:
    DoubleMinusFloatNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class DoublePlusIntNode : public PureNode {
public:
    DoublePlusIntNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class DoublePlusDoubleNode : public PureNode {
public:
    DoublePlusDoubleNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class DoublePlusFloatNode : public PureNode {
public:
    DoublePlusFloatNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class DoubleDivideIntNode : public PureNode {
public:
    DoubleDivideIntNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class DoubleDivideDoubleNode : public PureNode {
public:
    DoubleDivideDoubleNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class DoubleDivideFloatNode : public PureNode {
public:
    DoubleDivideFloatNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class DoubleMultiplyIntNode : public PureNode {
public:
    DoubleMultiplyIntNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class DoubleMultiplyDoubleNode : public PureNode {
public:
    DoubleMultiplyDoubleNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class DoubleMultiplyFloatNode : public PureNode {
public:
    DoubleMultiplyFloatNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::MATH; }
};

class TextAppendNode : public PureNode {
private:
    std::string text;
public:
    TextAppendNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    nlohmann::json serializeData() const override;
    void deserializeData(const nlohmann::json& data) override;
};

class GetIntArrayElementNode : public PureNode {
public:
    GetIntArrayElementNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::ARRAY; }
};

class AddIntArrayElementNode : public ExecNode {
public:
    AddIntArrayElementNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::ARRAY; }
};

class SizeIntArrayElementNode : public PureNode {
public:
    SizeIntArrayElementNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::ARRAY; }
};

class ClearIntArrayElementNode : public ExecNode {
public:
    ClearIntArrayElementNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::ARRAY; }
};

class RemoveIntArrayElementNode : public ExecNode {
public:
    RemoveIntArrayElementNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::ARRAY; }
};

class GetFloatArrayElementNode : public PureNode {
public:
    GetFloatArrayElementNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::ARRAY; }
};

class AddFloatArrayElementNode : public ExecNode {
public:
    AddFloatArrayElementNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::ARRAY; }
};

class SizeFloatArrayElementNode : public PureNode {
public:
    SizeFloatArrayElementNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::ARRAY; }
};

class ClearFloatArrayElementNode : public ExecNode {
public:
    ClearFloatArrayElementNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::ARRAY; }
};

class RemoveFloatArrayElementNode : public ExecNode {
public:
    RemoveFloatArrayElementNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::ARRAY; }
};

class GetDoubleArrayElementNode : public PureNode {
public:
    GetDoubleArrayElementNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::ARRAY; }
};

class AddDoubleArrayElementNode : public ExecNode {
public:
    AddDoubleArrayElementNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::ARRAY; }
};

class SizeDoubleArrayElementNode : public PureNode {
public:
    SizeDoubleArrayElementNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::ARRAY; }
};

class ClearDoubleArrayElementNode : public ExecNode {
public:
    ClearDoubleArrayElementNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::ARRAY; }
};

class RemoveDoubleArrayElementNode : public ExecNode {
public:
    RemoveDoubleArrayElementNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::ARRAY; }
};

class GetBoolArrayElementNode : public PureNode {
public:
    GetBoolArrayElementNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::ARRAY; }
};

class AddBoolArrayElementNode : public ExecNode {
public:
    AddBoolArrayElementNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::ARRAY; }
};

class SizeBoolArrayElementNode : public PureNode {
public:
    SizeBoolArrayElementNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::ARRAY; }
};

class ClearBoolArrayElementNode : public ExecNode {
public:
    ClearBoolArrayElementNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::ARRAY; }
};

class RemoveBoolArrayElementNode : public ExecNode {
public:
    RemoveBoolArrayElementNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::ARRAY; }
};

class GetStringArrayElementNode : public PureNode {
public:
    GetStringArrayElementNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::ARRAY; }
};

class AddStringArrayElementNode : public ExecNode {
public:
    AddStringArrayElementNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::ARRAY; }
};

class SizeStringArrayElementNode : public PureNode {
public:
    SizeStringArrayElementNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::ARRAY; }
};

class ClearStringArrayElementNode : public ExecNode {
public:
    ClearStringArrayElementNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::ARRAY; }
};

class RemoveStringArrayElementNode : public ExecNode {
public:
    RemoveStringArrayElementNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::ARRAY; }
};

class ForLoopNode : public ExecNode {
public:
    ForLoopNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::FLOW; }
    bool isLoop() const override { return true; }
};

class AddMarkNode : public ExecNode {
public:
    AddMarkNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CHART; }
};

class AddLineNode : public ExecNode {
public:
    AddLineNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CHART; }
};

class GetCurrentCandleDataNode : public PureNode {
public:
    GetCurrentCandleDataNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CHART; }
};

class GetCandleDataAtIndexNode : public PureNode {
public:
    GetCandleDataAtIndexNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CHART; }
};

class GetCurrentCandleIndexNode : public PureNode {
public:
    GetCurrentCandleIndexNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CHART; }
};

class SMANode : public PureNode {
public:
    SMANode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CHART; }
};

class EMANode : public PureNode {
public:
    EMANode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CHART; }
};

class RSINode : public PureNode {
public:
    RSINode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CHART; }
};

class MACDNode : public PureNode {
public:
    MACDNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CHART; }
};

class BollingerBandsNode : public PureNode {
public:
    BollingerBandsNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CHART; }
};

class StochasticNode : public PureNode {
public:
    StochasticNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CHART; }
};

class ATRNode : public PureNode {
public:
    ATRNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CHART; }
};

class CCINode : public PureNode {
public:
    CCINode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CHART; }
};

class WilliamsRNode : public PureNode {
public:
    WilliamsRNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CHART; }
};

class MFINode : public PureNode {
public:
    MFINode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CHART; }
};

class VWAPNode : public PureNode {
public:
    VWAPNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CHART; }
};

class HighestNode : public PureNode {
public:
    HighestNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CHART; }
};

class LowestNode : public PureNode {
public:
    LowestNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CHART; }
};

class DojiPatternNode : public PureNode {
public:
    DojiPatternNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CHART; }
};

class HammerPatternNode : public PureNode {
public:
    HammerPatternNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CHART; }
};

class EngulfingPatternNode : public PureNode {
public:
    EngulfingPatternNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CHART; }
};

class CenterOfGravityNode : public PureNode {
public:
    CenterOfGravityNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType() const override;
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CHART; }
};

class BybiyMarginLongNode : public ExecNode {
public:
    BybiyMarginLongNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override { return "BybiyMarginLongNode"; };
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CHART; }
};

class BybiyMarginShortNode : public ExecNode {
public:
    BybiyMarginShortNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override { return "BybiyMarginShortNode"; };
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CHART; }
};

class BybiyMarginCloseLongPositionNode : public ExecNode {
public:
    BybiyMarginCloseLongPositionNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override { return "BybiyMarginCloseLongPositionNode"; };
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CHART; }
};

class BybiyMarginCloseShortPositionNode : public ExecNode {
public:
    BybiyMarginCloseShortPositionNode(const std::string& guid, BlueprintManager* dManager = nullptr);
    void execute() override;
    std::string getNodeType()const override { return "BybiyMarginCloseShortPositionNode"; };
    NodeHeaderColor getHeaderColor() const override { return NodeHeaderColor::CHART; }
};
