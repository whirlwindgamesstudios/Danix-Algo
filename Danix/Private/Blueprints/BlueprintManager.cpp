#include "../../Public/Blueprints/Node/Nodes/Nodes.h"
#include "../../Public/Blueprints/BlueprintManager.h"
#include "../../Public/Systems/GuidGenerator.h"
#include "../../Public/GUI/Color.h"
#include "../../Public/Chart/CandleChartManager.h"
#include <cstdio>
#include <cstring>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
float clampff(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

float VecLength(ImVec2 v) {
    return sqrt(v.x * v.x + v.y * v.y);
}

float VecLengthSqr(ImVec2 v) {
    return v.x * v.x + v.y * v.y;
}

bool setClipboardText(const std::string& text) {
    if (!OpenClipboard(nullptr)) {
        return false;
    }

    EmptyClipboard();

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
    if (!hMem) {
        CloseClipboard();
        return false;
    }

    char* pMem = static_cast<char*>(GlobalLock(hMem));
    if (pMem) {
        memcpy(pMem, text.c_str(), text.size() + 1);
        GlobalUnlock(hMem);
    }

    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();

    return true;
}

std::string getClipboardText() {
    if (!OpenClipboard(nullptr)) {
        return "";
    }

    HANDLE hData = GetClipboardData(CF_TEXT);
    if (!hData) {
        CloseClipboard();
        return "";
    }

    char* pszText = static_cast<char*>(GlobalLock(hData));
    std::string text;
    if (pszText) {
        text = std::string(pszText);
    }

    GlobalUnlock(hData);
    CloseClipboard();

    return text;
}

BlueprintManager::BlueprintManager() : creating_connection(false), connection_start_pin(nullptr), dragging_node(false), selectedFileIndex(-1), showLoadMenuPopup(false) {
    viewport_pos = ImVec2(250, 80);
    viewport_size = ImVec2(800, 600);
    selected_node_guid = "";
    camera_offset = ImVec2(0, 0);
    camera_zoom = 1.0f;
    is_panning = false;
    pan_start_pos = ImVec2(0, 0);
    pan_start_offset = ImVec2(0, 0);
    show_context_menu = false;
    context_menu_pos = ImVec2(0, 0);
}
BasicNode* BlueprintManager::getNodeByGuid(const std::string& guid) {
    auto it = nodes.find(guid);
    return (it != nodes.end()) ? it->second.get() : nullptr;
}

Pin* BlueprintManager::getPinByGuid(const std::string& guid) {
    for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        BasicNode* node = it->second.get();
        for (auto& pin : node->inputs) {
            if (pin->pin_guid == guid) return pin.get();
        }
        for (auto& pin : node->outputs) {
            if (pin->pin_guid == guid) return pin.get();
        }
    }
    return nullptr;
}

void BlueprintManager::addNode(std::unique_ptr<BasicNode> node) {
    std::string guid = node->node_guid;
    nodes[guid] = std::move(node);
}

void BlueprintManager::createConnection(PinOut* from_pin, PinIn* to_pin) {
    if (!from_pin || !to_pin) {
        return;
    }

    if (from_pin->type == to_pin->type) {
        if (from_pin->type == PinType::EXEC)
        {
            removeConnectionsFromOutput(from_pin);
        }
        else
        {
            removeConnectionsToInput(to_pin);
        }

        Connection conn;
        conn.from_pin_guid = from_pin->pin_guid;
        conn.to_pin_guid = to_pin->pin_guid;
        conn.from_pin = from_pin;
        conn.to_pin = to_pin;

        connections.push_back(conn);

        from_pin->connected_pins.push_back(to_pin);
        from_pin->connected_pin_guids.push_back(to_pin->pin_guid);
        to_pin->linked_to = from_pin;
        to_pin->linked_to_guid = from_pin->pin_guid;
        return;
    }

    ImVec2 mid_pos = ImVec2((from_pin->position.x + to_pin->position.x) / 2 - 60,
        (from_pin->position.y + to_pin->position.y) / 2 - 30);

    BasicNode* converter = createAutoConverter(from_pin->type, to_pin->type, mid_pos);

    if (converter && converter->inputs.size() > 0 && converter->outputs.size() > 0) {
        PinIn* converter_input = dynamic_cast<PinIn*>(converter->inputs[0].get());
        if (converter_input) {
            removeConnectionsToInput(converter_input);

            Connection conn1;
            conn1.from_pin_guid = from_pin->pin_guid;
            conn1.to_pin_guid = converter_input->pin_guid;
            conn1.from_pin = from_pin;
            conn1.to_pin = converter_input;
            connections.push_back(conn1);

            from_pin->connected_pins.push_back(converter_input);
            from_pin->connected_pin_guids.push_back(converter_input->pin_guid);
            converter_input->linked_to = from_pin;
            converter_input->linked_to_guid = from_pin->pin_guid;
        }

        PinOut* converter_output = dynamic_cast<PinOut*>(converter->outputs[0].get());
        if (converter_output) {
            removeConnectionsToInput(to_pin);

            Connection conn2;
            conn2.from_pin_guid = converter_output->pin_guid;
            conn2.to_pin_guid = to_pin->pin_guid;
            conn2.from_pin = converter_output;
            conn2.to_pin = to_pin;
            connections.push_back(conn2);

            converter_output->connected_pins.push_back(to_pin);
            converter_output->connected_pin_guids.push_back(to_pin->pin_guid);
            to_pin->linked_to = converter_output;
            to_pin->linked_to_guid = converter_output->pin_guid;
        }
    }
}

void BlueprintManager::removeConnectionsToInput(PinIn* to_pin) {
    for (auto it = connections.begin(); it != connections.end();) {
        if (it->to_pin == to_pin) {
            PinOut* from_pin = it->from_pin;
            auto pin_it = std::find(from_pin->connected_pins.begin(), from_pin->connected_pins.end(), to_pin);
            if (pin_it != from_pin->connected_pins.end()) {
                from_pin->connected_pins.erase(pin_it);
            }

            to_pin->linked_to = nullptr;
            to_pin->linked_to_guid = "";

            it = connections.erase(it);
        }
        else {
            ++it;
        }
    }
}

void BlueprintManager::removeConnectionsFromOutput(PinOut* from_pin) {
    for (auto it = connections.begin(); it != connections.end();) {
        if (it->from_pin == from_pin) {
            PinIn* to_pin = it->to_pin;
            to_pin->linked_to = nullptr;
            to_pin->linked_to_guid = "";

            auto pin_it = std::find(from_pin->connected_pins.begin(), from_pin->connected_pins.end(), to_pin);
            if (pin_it != from_pin->connected_pins.end()) {
                from_pin->connected_pins.erase(pin_it);
            }

            it = connections.erase(it);
        }
        else {
            ++it;
        }
    }
}


void BlueprintManager::executeFromEntry() {
    for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        BasicNode* node = it->second.get();
        if (node->name == "Entry") {
            static_cast<ExecNode*>(node)->execute();
            break;
        }
    }

    for (auto it = variables.begin(); it != variables.end(); ++it) {
        Variable* variable = it->second.get();
        if (!variable->global) {
            variable->setDefaultValue();
        }
    }
}

void BlueprintManager::draw() {
    ImGuiIO& io = ImGui::GetIO();

    const float top_toolbar_height = 60.0f;
    const float left_sidebar_width = 250.0f;

    viewport_pos = ImVec2(left_sidebar_width, 50 + top_toolbar_height);
    viewport_size = ImVec2(io.DisplaySize.x - left_sidebar_width,
        io.DisplaySize.y - 50 - top_toolbar_height);

    drawViewport();

    drawTopToolbar();
    drawLeftSidebar();

    renderLoadMenu();
    renderSaveMenu();
}

void BlueprintManager::drawTopToolbar() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 50));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, 60));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 255)));           
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImGui::ColorConvertU32ToFloat4(IM_COL32(102, 102, 102, 255)));      
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImGui::ColorConvertU32ToFloat4(IM_COL32(102, 102, 102, 255)));    
    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(IM_COL32(76, 76, 76, 255)));            
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::ColorConvertU32ToFloat4(IM_COL32(140, 140, 140, 255)));    
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::ColorConvertU32ToFloat4(IM_COL32(76, 76, 76, 255)));      
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(IM_COL32(255, 255, 255, 255)));          

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);

    ImGui::Begin("Toolbar", nullptr,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);

    ImGui::Text("Blueprint Toolbar");
    ImGui::SameLine();
    if (ImGui::Button("Clear All")) {
        clearAll();
    }
    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        SaveBlueprintMenu();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        LoadBlueprintMenu();
    }
    ImGui::Separator();
    ImGui::Text("Nodes: %zu, Connections: %zu", nodes.size(), connections.size());
    if (creating_connection) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0, 0.8f, 1, 1), "Creating connection from: %s",
            connection_start_pin ? connection_start_pin->name.c_str() : "unknown");
    }
    ImGui::End();

    ImGui::PopStyleColor(7);
    ImGui::PopStyleVar(2);
}

void BlueprintManager::drawLeftSidebar() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 110));
    ImGui::SetNextWindowSize(ImVec2(250, io.DisplaySize.y - 110));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 255)));           
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImGui::ColorConvertU32ToFloat4(IM_COL32(102, 102, 102, 255)));      
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImGui::ColorConvertU32ToFloat4(IM_COL32(102, 102, 102, 255)));    
    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(IM_COL32(76, 76, 76, 255)));            
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::ColorConvertU32ToFloat4(IM_COL32(140, 140, 140, 255)));    
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::ColorConvertU32ToFloat4(IM_COL32(76, 76, 76, 255)));      
    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::ColorConvertU32ToFloat4(IM_COL32(102, 102, 102, 255)));        
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::ColorConvertU32ToFloat4(IM_COL32(140, 140, 140, 255)));   
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGui::ColorConvertU32ToFloat4(IM_COL32(102, 102, 102, 255)));   
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::ColorConvertU32ToFloat4(IM_COL32(40, 40, 40, 255)));          
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImGui::ColorConvertU32ToFloat4(IM_COL32(60, 60, 60, 255)));    
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImGui::ColorConvertU32ToFloat4(IM_COL32(80, 80, 80, 255)));    
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(IM_COL32(20, 20, 20, 255)));          
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 255)));            
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(IM_COL32(255, 255, 255, 255)));          

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);

    ImGui::Begin("Node Inspector", nullptr,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);

    if (!selected_node_guid.empty()) {
        BasicNode* selected_node = getNodeByGuid(selected_node_guid);
        if (selected_node) {
            ImGui::Text("Selected Node:");
            ImGui::Separator();

            ImGui::Text("Name: %s", selected_node->name.c_str());
            ImGui::Separator();

            if (!selected_node->description.empty()) {
                ImGui::Text("Description:");
                ImGui::TextWrapped("%s", selected_node->description.c_str());
                ImGui::Separator();
            }

            ConstValueNode* constNode = dynamic_cast<ConstValueNode*>(selected_node);
            if (constNode) {
                ImGui::Text("Constant Value:");
                ImGui::Separator();

                static std::map<std::string, char[256]> constStringBuffers;

                ImU32 typeColor;
                std::string shortTypeName;
                switch (constNode->type) {
                case PinType::INT:
                    shortTypeName = "I";
                    typeColor = ForestGreenColor;
                    break;
                case PinType::FLOAT:
                    shortTypeName = "F";
                    typeColor = GreenColor;
                    break;
                case PinType::DOUBLE:
                    shortTypeName = "D";
                    typeColor = CyanColor;
                    break;
                case PinType::BOOL:
                    shortTypeName = "B";
                    typeColor = RedColor;
                    break;
                case PinType::STRING:
                    shortTypeName = "S";
                    typeColor = PinkColor;
                    break;
                default:
                    shortTypeName = "?";
                    typeColor = WhiteColor;
                    break;
                }

                ImGui::Text("Type:");
                ImGui::SameLine();
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(typeColor), "[%s]", shortTypeName.c_str());

                ImGui::Text("Value:");
                ImGui::SameLine();
                ImGui::PushItemWidth(150);

                switch (constNode->type) {
                case PinType::INT:
                    ImGui::InputInt("##constval", &constNode->intValue);
                    break;

                case PinType::FLOAT:
                    ImGui::InputFloat("##constval", &constNode->floatValue, 0.0f, 0.0f, "%.3f");
                    break;

                case PinType::DOUBLE: {
                    double tempDouble = (double)constNode->doubleValue;
                    if (ImGui::InputDouble("##constval", &tempDouble)) {
                        constNode->doubleValue = (long double)tempDouble;
                    }
                    break;
                }

                case PinType::BOOL:
                    ImGui::Checkbox("##constval", &constNode->boolValue);
                    break;

                case PinType::STRING: {
                    if (constStringBuffers.find(constNode->node_guid) == constStringBuffers.end()) {
                        strcpy_s(constStringBuffers[constNode->node_guid], sizeof(constStringBuffers[constNode->node_guid]), constNode->stringValue.c_str());
                    }

                    if (ImGui::InputText("##constval", constStringBuffers[constNode->node_guid], sizeof(constStringBuffers[constNode->node_guid]))) {
                        constNode->stringValue = std::string(constStringBuffers[constNode->node_guid]);
                    }
                    break;
                }

                default:
                    ImGui::Text("Unknown type");
                    break;
                }

                ImGui::PopItemWidth();
                ImGui::Separator();
            }
            else if (!selected_node->inputs.empty()) {
                ImGui::Text("Pin Default Values:");
                ImGui::Separator();

                static std::map<std::string, char[256]> pinStringBuffers;

                for (int i = 0; i < selected_node->inputs.size(); i++) {
                    PinIn* pin = selected_node->inputs[i].get();
                    if (!pin) continue;

                    ImGui::PushID(pin->pin_guid.c_str());

                    ImU32 typeColor;
                    std::string shortTypeName;
                    switch (pin->type) {
                    case PinType::INT:
                        shortTypeName = "I";
                        typeColor = ForestGreenColor;
                        break;
                    case PinType::FLOAT:
                        shortTypeName = "F";
                        typeColor = GreenColor;
                        break;
                    case PinType::DOUBLE:
                        shortTypeName = "D";
                        typeColor = CyanColor;
                        break;
                    case PinType::BOOL:
                        shortTypeName = "B";
                        typeColor = RedColor;
                        break;
                    case PinType::STRING:
                        shortTypeName = "S";
                        typeColor = PinkColor;
                        break;
                    case PinType::ARRAY_INT:
                        shortTypeName = "AI";
                        typeColor = ForestGreenColor;
                        break;
                    case PinType::ARRAY_FLOAT:
                        shortTypeName = "AF";
                        typeColor = GreenColor;
                        break;
                    case PinType::ARRAY_DOUBLE:
                        shortTypeName = "AD";
                        typeColor = CyanColor;
                        break;
                    case PinType::ARRAY_BOOL:
                        shortTypeName = "AB";
                        typeColor = RedColor;
                        break;
                    case PinType::ARRAY_STRING:
                        shortTypeName = "AS";
                        typeColor = PinkColor;
                        break;
                    case PinType::EXEC:
                        ImGui::PopID();
                        continue;
                    default:
                        shortTypeName = "?";
                        typeColor = WhiteColor;
                        break;
                    }

                    ImGui::Text("%s", pin->name.c_str());

                    if (!pin->description.empty() && ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("%s", pin->description.c_str());
                    }

                    ImGui::SameLine();
                    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(typeColor), "[%s]", shortTypeName.c_str());

                    ImGui::SameLine();
                    ImGui::PushItemWidth(80);

                    switch (pin->type) {
                    case PinType::INT: {
                        int val = pin->getDefaultValue<int>();
                        if (ImGui::InputInt("##pinval", &val)) {
                            pin->default_stored_int = val;
                        }
                        break;
                    }
                    case PinType::FLOAT: {
                        float val = pin->getDefaultValue<float>();
                        if (ImGui::InputFloat("##pinval", &val, 0.0f, 0.0f, "%.3f")) {
                            pin->default_stored_float = val;
                        }
                        break;
                    }
                    case PinType::DOUBLE: {
                        double val = (double)pin->getDefaultValue<long double>();
                        if (ImGui::InputDouble("##pinval", &val)) {
                            pin->default_stored_double = (long double)val;
                        }
                        break;
                    }
                    case PinType::BOOL: {
                        bool val = pin->getDefaultValue<bool>();
                        if (ImGui::Checkbox("##pinval", &val)) {
                            pin->default_stored_bool = val;
                        }
                        break;
                    }
                    case PinType::STRING: {
                        if (pinStringBuffers.find(pin->pin_guid) == pinStringBuffers.end()) {
                            strcpy_s(pinStringBuffers[pin->pin_guid], sizeof(pinStringBuffers[pin->pin_guid]),
                                pin->default_stored_string.c_str());
                        }

                        if (ImGui::InputText("##pinval", pinStringBuffers[pin->pin_guid], sizeof(pinStringBuffers[pin->pin_guid]))) {
                            pin->default_stored_string = std::string(pinStringBuffers[pin->pin_guid]);
                        }
                        break;
                    }

                    case PinType::ARRAY_INT:
                        ImGui::Text("Array");
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Array editing not implemented yet");
                        }
                        break;

                    case PinType::ARRAY_FLOAT:
                        ImGui::Text("Array");
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Array editing not implemented yet");
                        }
                        break;

                    case PinType::ARRAY_DOUBLE:
                        ImGui::Text("Array");
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Array editing not implemented yet");
                        }
                        break;

                    case PinType::ARRAY_BOOL:
                        ImGui::Text("Array");
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Array editing not implemented yet");
                        }
                        break;

                    case PinType::ARRAY_STRING:
                        ImGui::Text("Array");
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Array editing not implemented yet");
                        }
                        break;
                    }

                    ImGui::PopItemWidth();
                    ImGui::PopID();
                }
                ImGui::Separator();
            }

            ImGui::Text("Outputs: %d", (int)selected_node->outputs.size());
            for (int i = 0; i < selected_node->outputs.size(); i++) {
                std::string type_name;
                switch (selected_node->outputs[i]->type) {
                case PinType::EXEC: type_name = "Exec"; break;
                case PinType::BOOL: type_name = "Bool"; break;
                case PinType::FLOAT: type_name = "Float"; break;
                case PinType::DOUBLE: type_name = "Double"; break;
                case PinType::STRING: type_name = "String"; break;
                case PinType::INT: type_name = "Int"; break;
                default: type_name = "Unknown"; break;
                }
                ImGui::BulletText("%s (%s)",
                    selected_node->outputs[i]->name.c_str(),
                    type_name.c_str());
            }
        }
    }
    else {
        ImGui::Text("No node selected");
        ImGui::Text("Click on a node to see its info");
    }

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Variables");
    ImGui::Separator();

    if (ImGui::Button("Create Variable", ImVec2(-1, 25))) {
        ImGui::OpenPopup("CreateVariable");
    }

    if (ImGui::BeginPopup("CreateVariable")) {
        static char varName[256] = "";
        static int varType = 0;
        static bool isGlobal = false;

        static int defaultInt = 0;
        static float defaultFloat = 0.0f;
        static double defaultDouble = 0.0;
        static bool defaultBool = false;
        static char defaultString[256] = "";

        const char* typeNames[] = { "INT", "FLOAT", "DOUBLE", "BOOL", "STRING" ,"Array_INT", "AFLOAT", "Array_DOUBLE", "Array_BOOL", "Array_STRING" };
        PinType pinTypes[] = { PinType::INT, PinType::FLOAT, PinType::DOUBLE, PinType::BOOL, PinType::STRING, PinType::ARRAY_INT, PinType::ARRAY_FLOAT, PinType::ARRAY_DOUBLE, PinType::ARRAY_BOOL, PinType::ARRAY_STRING };

        ImGui::Text("Create New Variable");
        ImGui::Separator();

        ImGui::Text("Name:");
        ImGui::InputText("##VarName", varName, sizeof(varName));

        ImGui::Text("Type:");
        ImGui::Combo("##VarType", &varType, typeNames, 10);

        ImGui::Spacing();

        ImGui::Checkbox("Global", &isGlobal);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Global variables keep their values between candles,\nnon-global variables reset each candle");
        }

        ImGui::Spacing();

        ImGui::Text("Default Value:");
        ImGui::PushItemWidth(150);

        switch (pinTypes[varType]) {
        case PinType::INT:
            ImGui::InputInt("##DefaultValue", &defaultInt);
            break;
        case PinType::FLOAT:
            ImGui::InputFloat("##DefaultValue", &defaultFloat, 0.0f, 0.0f, "%.3f");
            break;
        case PinType::DOUBLE:
            ImGui::InputDouble("##DefaultValue", &defaultDouble);
            break;
        case PinType::BOOL:
            ImGui::Checkbox("##DefaultValue", &defaultBool);
            break;
        case PinType::STRING:
            ImGui::InputText("##DefaultValue", defaultString, sizeof(defaultString));
            break;
        }
        ImGui::PopItemWidth();

        ImGui::Spacing();

        if (ImGui::Button("Create") && strlen(varName) > 0) {
            PinType type = pinTypes[varType];

            std::string guid = GUIDGenerator::generate();
            auto variable = std::make_unique<Variable>(std::string(varName), guid, type, isGlobal);
            variable->setDefaultValue();

            switch (type) {
            case PinType::INT:
                variable->def_intValue = defaultInt;
                break;
            case PinType::FLOAT:
                variable->def_floatValue = defaultFloat;
                break;
            case PinType::DOUBLE:
                variable->def_doubleValue = (long double)defaultDouble;
                break;
            case PinType::BOOL:
                variable->def_boolValue = defaultBool;
                break;
            case PinType::STRING:
                variable->def_stringValue = std::string(defaultString);
                break;
            }

            variables[guid] = std::move(variable);

            strcpy_s(varName, sizeof(varName), "");
            varType = 0;
            isGlobal = false;
            defaultInt = 0;
            defaultFloat = 0.0f;
            defaultDouble = 0.0;
            defaultBool = false;
            strcpy_s(defaultString, sizeof(defaultString), "");

            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::Spacing();

    ImGui::Text("Variables (%d):", (int)variables.size());

    if (variables.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No variables created");
    }
    else {
        ImGui::BeginChild("VariablesList", ImVec2(0, 200), true);

        std::string variableToDelete = "";      
        static std::map<std::string, char[256]> stringBuffers;     

        for (auto it = variables.begin(); it != variables.end(); ++it) {
            Variable* var = it->second.get();
            ImGui::PushID(var->guid.c_str());

            std::string shortTypeName;
            ImU32 typeColor;

            switch (var->type) {
            case PinType::INT:
                shortTypeName = "I";
                typeColor = ForestGreenColor;
                break;
            case PinType::FLOAT:
                shortTypeName = "F";
                typeColor = GreenColor;
                break;
            case PinType::DOUBLE:
                shortTypeName = "D";
                typeColor = CyanColor;
                break;
            case PinType::BOOL:
                shortTypeName = "B";
                typeColor = RedColor;
                break;
            case PinType::STRING:
                shortTypeName = "S";
                typeColor = PinkColor;
                break;
            case PinType::ARRAY_INT:
                shortTypeName = "AI";
                typeColor = ForestGreenColor;
                break;
            case PinType::ARRAY_FLOAT:
                shortTypeName = "AF";
                typeColor = GreenColor;
                break;
            case PinType::ARRAY_DOUBLE:
                shortTypeName = "AD";
                typeColor = CyanColor;
                break;
            case PinType::ARRAY_BOOL:
                shortTypeName = "AB";
                typeColor = RedColor;
                break;
            case PinType::ARRAY_STRING:
                shortTypeName = "AS";
                typeColor = PinkColor;
                break;
            default:
                shortTypeName = "?";
                typeColor = WhiteColor;
                break;
            }

            float buttonSize = 16.0f;
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            ImVec2 buttonPos = ImVec2(cursorPos.x + ImGui::GetContentRegionAvail().x - buttonSize - 2, cursorPos.y);

            ImGui::SetCursorScreenPos(buttonPos);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.1f, 0.1f, 1.0f));

            if (ImGui::Button("x", ImVec2(buttonSize, buttonSize))) {
                std::string popupId = "DeleteVariable##" + var->guid;
                ImGui::OpenPopup(popupId.c_str());
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Delete variable");
            }

            ImGui::PopStyleColor(3);

            ImGui::SetCursorScreenPos(cursorPos);

            ImGui::Text("%s", var->name.c_str());

            ImGui::SameLine();
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(typeColor), "[%s]", shortTypeName.c_str());
            if (ImGui::IsItemHovered()) {
                std::string fullTypeName;
                switch (var->type) {
                case PinType::INT: fullTypeName = "Integer"; break;
                case PinType::FLOAT: fullTypeName = "Float"; break;
                case PinType::DOUBLE: fullTypeName = "Double"; break;
                case PinType::BOOL: fullTypeName = "Boolean"; break;
                case PinType::STRING: fullTypeName = "String"; break;
                case PinType::ARRAY_INT: fullTypeName = "Array of Integers"; break;
                case PinType::ARRAY_FLOAT: fullTypeName = "Array of Floats"; break;
                case PinType::ARRAY_DOUBLE: fullTypeName = "Array of Doubles"; break;
                case PinType::ARRAY_BOOL: fullTypeName = "Array of Booleans"; break;
                case PinType::ARRAY_STRING: fullTypeName = "Array of Strings"; break;
                default: fullTypeName = "Unknown Type"; break;
                }
                ImGui::SetTooltip("%s", fullTypeName.c_str());
            }

            ImGui::SameLine();
            if (var->global) {
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "[G]");
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Global variable - keeps value between candles");
                }
            }
            else {
                ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "[L]");
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Local variable - resets each candle");
                }
            }

            ImGui::Text("Value:");
            ImGui::SameLine();
            ImGui::PushItemWidth(100);

            switch (var->type) {
            case PinType::INT:
                if (ImGui::InputInt("##editval", &var->def_intValue)) {
                }
                break;

            case PinType::FLOAT:
                if (ImGui::InputFloat("##editval", &var->def_floatValue, 0.0f, 0.0f, "%.3f")) {
                }
                break;

            case PinType::DOUBLE: {
                double tempDouble = (double)var->def_doubleValue;
                if (ImGui::InputDouble("##editval", &tempDouble)) {
                    var->def_doubleValue = (long double)tempDouble;
                }
                break;
            }

            case PinType::BOOL:
                if (ImGui::Checkbox("##editval", &var->def_boolValue)) {
                }
                break;

            case PinType::STRING: {
                if (stringBuffers.find(var->guid) == stringBuffers.end()) {
                    strcpy_s(stringBuffers[var->guid], sizeof(stringBuffers[var->guid]), var->def_stringValue.c_str());
                }

                if (ImGui::InputText("##editval", stringBuffers[var->guid], sizeof(stringBuffers[var->guid]))) {
                    var->def_stringValue = std::string(stringBuffers[var->guid]);
                }
                break;
            }
            }
            ImGui::PopItemWidth();

            ImGui::Spacing();

            if (ImGui::Button("Get", ImVec2(35, 20))) {
                ImVec2 center = ImVec2(400, 300);
                ImVec2 worldPos = screenToWorld(center);

                auto getNode = std::make_unique<GetVariableNode>(GUIDGenerator::generate(), this, var->guid);
                getNode->position = worldPos;
                getNode->bindToVariable(var->guid);
                addNode(std::move(getNode));
            }

            ImGui::SameLine();
            if (ImGui::Button("Set", ImVec2(35, 20))) {
                ImVec2 center = ImVec2(400, 300);
                ImVec2 worldPos = screenToWorld(center);

                auto setNode = std::make_unique<SetVariableNode>(GUIDGenerator::generate(), this, var->guid);
                setNode->position = worldPos;
                setNode->bindToVariable(var->guid);
                addNode(std::move(setNode));
            }

            std::string popupId = "DeleteVariable##" + var->guid;
            if (ImGui::BeginPopup(popupId.c_str())) {
                ImGui::Text("Delete variable '%s'?", var->name.c_str());
                ImGui::Text("This will also remove all Get/Set nodes!");
                ImGui::Separator();

                if (ImGui::Button("Yes, Delete")) {
                    variableToDelete = var->guid;     
                    stringBuffers.erase(var->guid);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) {
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            ImGui::PopID();      
            ImGui::Separator();
        }

        ImGui::EndChild();

        if (!variableToDelete.empty()) {
            removeVariable(variableToDelete);
        }
    }

    ImGui::End();

    ImGui::PopStyleColor(15);
    ImGui::PopStyleVar(2);
}

ImU32 getConnectionColor(PinType type) {
    switch (type) {
    case PinType::EXEC: return WhiteColor;
    case PinType::BOOL: return RedColor;
    case PinType::FLOAT: return GreenColor;
    case PinType::DOUBLE: return CyanColor;
    case PinType::STRING: return PinkColor;
    case PinType::INT: return ForestGreenColor;
    case PinType::ARRAY_BOOL: return RedColor;
    case PinType::ARRAY_FLOAT: return GreenColor;
    case PinType::ARRAY_DOUBLE: return CyanColor;
    case PinType::ARRAY_STRING: return PinkColor;
    case PinType::ARRAY_INT: return ForestGreenColor;
    default: return IM_COL32(255, 255, 255, 255);
    }
}

void BlueprintManager::drawViewport() {
    ImVec2 mouse_pos = ImGui::GetMousePos();
    if (mouse_pos.x >= viewport_pos.x && mouse_pos.x <= viewport_pos.x + viewport_size.x &&
        mouse_pos.y >= viewport_pos.y && mouse_pos.y <= viewport_pos.y + viewport_size.y) {
        handleInteractions();
        handleCameraControls();
        handleSelectionInput();
        handleKeyboardInput();
    }

    ImGui::SetNextWindowPos(viewport_pos);
    ImGui::SetNextWindowSize(viewport_size);
    ImGui::Begin("##Viewport", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoBackground);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    draw_list->PushClipRect(viewport_pos,
        ImVec2(viewport_pos.x + viewport_size.x, viewport_pos.y + viewport_size.y),
        true);

    drawGrid(draw_list);

    for (const auto& conn : connections) {
        if (conn.from_pin && conn.to_pin) {
            bool array = false;
            ImU32 color = getConnectionColor(conn.from_pin->type);

            if (conn.from_pin->type == PinType::ARRAY_BOOL ||
                conn.from_pin->type == PinType::ARRAY_FLOAT ||
                conn.from_pin->type == PinType::ARRAY_DOUBLE ||
                conn.from_pin->type == PinType::ARRAY_STRING ||
                conn.from_pin->type == PinType::ARRAY_INT) {
                array = true;
            }

            if (array) {
                drawStripedBezierConnection(draw_list, conn.from_pin->position, conn.to_pin->position, color, WhiteColor);
            }
            else {
                drawBezierConnection(draw_list, conn.from_pin->position, conn.to_pin->position, color);
            }
        }
    }

    if (creating_connection && connection_start_pin) {
        ImVec2 mouse_pos = ImGui::GetMousePos();
        drawBezierConnection(draw_list, connection_start_pin->position, mouse_pos, IM_COL32(0, 200, 255, 255));
    }

    for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        BasicNode* node = it->second.get();
        ImVec2 world_pos = node->position;
        ImVec2 screen_pos = worldToScreen(world_pos);

        node->position = screen_pos;

        if (isNodeSelected(it->first)) {
            draw_list->AddRect(
                ImVec2(screen_pos.x - 3, screen_pos.y - 3),
                ImVec2(screen_pos.x + node->size.x + 3, screen_pos.y + node->size.y + 3),
                IM_COL32(100, 150, 255, 150), 5.0f, 0, 3.0f
            );
        }

        node->drawNodeToDrawList(draw_list);

        node->position = world_pos;
    }

    drawSelectionBox();

    draw_list->PopClipRect();

    ImGui::End();

    if (creating_connection && connection_start_pin) {
        drawConnectionContextMenu();
    }
    else {
        drawContextMenu();
    }
}

void BlueprintManager::handleCameraControls() {
    ImVec2 mouse_pos = ImGui::GetMousePos();
    ImGuiIO& io = ImGui::GetIO();

    bool mouse_in_viewport = (mouse_pos.x >= viewport_pos.x && mouse_pos.x <= viewport_pos.x + viewport_size.x &&
        mouse_pos.y >= viewport_pos.y && mouse_pos.y <= viewport_pos.y + viewport_size.y);

    if (!mouse_in_viewport) return;

    bool should_pan = ImGui::IsMouseDown(2) || (ImGui::IsMouseDown(0) && io.KeyShift);

    if (should_pan && !is_panning && !creating_connection && !dragging_node) {
        is_panning = true;
        pan_start_pos = mouse_pos;
        pan_start_offset = camera_offset;
        show_context_menu = false;
    }

    if (is_panning) {
        if (should_pan) {
            ImVec2 delta = ImVec2(mouse_pos.x - pan_start_pos.x, mouse_pos.y - pan_start_pos.y);
            camera_offset = ImVec2(pan_start_offset.x + delta.x, pan_start_offset.y + delta.y);
        }
        else {
            is_panning = false;
        }
    }
}

void BlueprintManager::createNodeFromMenu(const std::string& node_type, ImVec2 world_pos) {
    std::unique_ptr<BasicNode> node = nullptr;

    if (node_type == "Entry") {
        node = std::make_unique<EntryNode>("Entry");
    }

    if (node_type == "BybiyMarginLong") {
        node = std::make_unique<BybiyMarginLongNode>(GUIDGenerator::generate());
    }
    else if (node_type == "BybiyMarginShort") {
        node = std::make_unique<BybiyMarginShortNode>(GUIDGenerator::generate());
    }
    else if (node_type == "BybiyMarginCloseLongPosition") {
        node = std::make_unique<BybiyMarginCloseLongPositionNode>(GUIDGenerator::generate());
    }
    else if (node_type == "BybiyMarginCloseShortPosition") {
        node = std::make_unique<BybiyMarginCloseShortPositionNode>(GUIDGenerator::generate());
    }

    if (node_type == "AddMark") {
        node = std::make_unique<EntryNode>(GUIDGenerator::generate());
    }
    else if (node_type == "AddLine") {
        node = std::make_unique<EntryNode>(GUIDGenerator::generate());
    }
    else if (node_type == "GetCurrentCandleDataIndex") {
        node = std::make_unique<GetCurrentCandleDataNode>(GUIDGenerator::generate());
    }
    else if (node_type == "GetCandleDataAtIndex") {
        node = std::make_unique<GetCandleDataAtIndexNode>(GUIDGenerator::generate());
    }
    else if (node_type == "GetCurrentCandleIndex") {
        node = std::make_unique<GetCurrentCandleIndexNode>(GUIDGenerator::generate());
    }
    if (node_type == "EngulfingPattern") {
        node = std::make_unique<EngulfingPatternNode>(GUIDGenerator::generate());
    }
    else if (node_type == "CenterOfGravity") {
        node = std::make_unique<CenterOfGravityNode>(GUIDGenerator::generate());
    }
    else if (node_type == "HammerPattern") {
        node = std::make_unique<HammerPatternNode>(GUIDGenerator::generate());
    }
    else if (node_type == "DojiPattern") {
        node = std::make_unique<DojiPatternNode>(GUIDGenerator::generate());
    }
    else if (node_type == "Lowest") {
        node = std::make_unique<LowestNode>(GUIDGenerator::generate());
    }
    else if (node_type == "Highest") {
        node = std::make_unique<HighestNode>(GUIDGenerator::generate());
    }
    else if (node_type == "VWAP") {
        node = std::make_unique<VWAPNode>(GUIDGenerator::generate());
    }
    else if (node_type == "MFI") {
        node = std::make_unique<MFINode>(GUIDGenerator::generate());
    }
    else if (node_type == "WilliamsR") {
        node = std::make_unique<WilliamsRNode>(GUIDGenerator::generate());
    }
    else if (node_type == "CCI") {
        node = std::make_unique<CCINode>(GUIDGenerator::generate());
    }
    else if (node_type == "ATR") {
        node = std::make_unique<ATRNode>(GUIDGenerator::generate());
    }
    else if (node_type == "Stochastic") {
        node = std::make_unique<StochasticNode>(GUIDGenerator::generate());
    }
    else if (node_type == "BollingerBands") {
        node = std::make_unique<BollingerBandsNode>(GUIDGenerator::generate());
    }
    else if (node_type == "MACD") {
        node = std::make_unique<MACDNode>(GUIDGenerator::generate());
    }
    else if (node_type == "RSI") {
        node = std::make_unique<RSINode>(GUIDGenerator::generate());
    }
    else if (node_type == "EMA") {
        node = std::make_unique<EMANode>(GUIDGenerator::generate());
    }
    else if (node_type == "SMA") {
        node = std::make_unique<SMANode>(GUIDGenerator::generate());
    }




    if (node_type == "IntNotEqual") {
        node = std::make_unique<IntNotEqualNode>(GUIDGenerator::generate());
    }
    else if (node_type == "IntEqual") {
        node = std::make_unique<IntEqualNode>(GUIDGenerator::generate());
    }
    else if (node_type == "IntLessThan") {
        node = std::make_unique<IntLessThanNode>(GUIDGenerator::generate());
    }
    else if (node_type == "IntMoreThan" || node_type == "IntMore") {
        node = std::make_unique<IntMoreThanNode>(GUIDGenerator::generate());
    }

    if (node_type == "DoubleNotEqual") {
        node = std::make_unique<DoubleNotEqualNode>(GUIDGenerator::generate());
    }
    else if (node_type == "DoubleEqual") {
        node = std::make_unique<DoubleEqualNode>(GUIDGenerator::generate());
    }
    else if (node_type == "DoubleLessThan") {
        node = std::make_unique<DoubleLessThanNode>(GUIDGenerator::generate());
    }
    else if (node_type == "DoubleMoreThan" || node_type == "DoubleMore") {
        node = std::make_unique<DoubleMoreThanNode>(GUIDGenerator::generate());
    }

    if (node_type == "FloatNotEqual") {
        node = std::make_unique<FloatNotEqualNode>(GUIDGenerator::generate());
    }
    else if (node_type == "FloatEqual") {
        node = std::make_unique<FloatEqualNode>(GUIDGenerator::generate());
    }
    else if (node_type == "FloatLessThan") {
        node = std::make_unique<FloatLessThanNode>(GUIDGenerator::generate());
    }
    else if (node_type == "FloatMoreThan" || node_type == "FloatMore") {
        node = std::make_unique<FloatMoreThanNode>(GUIDGenerator::generate());
    }

    if (node_type == "BoolAnd") {
        node = std::make_unique<BoolAndNode>(GUIDGenerator::generate());
    }
    else if (node_type == "BoolOr") {
        node = std::make_unique<BoolOrNode>(GUIDGenerator::generate());
    }
    else if (node_type == "BoolEqual") {
        node = std::make_unique<BoolEqualNode>(GUIDGenerator::generate());
    }
    else if (node_type == "BoolNotEqual") {
        node = std::make_unique<BoolNotEqualNode>(GUIDGenerator::generate());
    }

    if (node_type == "IntToFloat") {
        node = std::make_unique<IntToFloatNode>(GUIDGenerator::generate());
    }
    else if (node_type == "IntToDouble") {
        node = std::make_unique<IntToDoubleNode>(GUIDGenerator::generate());
    }
    else if (node_type == "IntToText") {
        node = std::make_unique<IntToTextNode>(GUIDGenerator::generate());
    }

    if (node_type == "DoubleToFloat") {
        node = std::make_unique<DoubleToFloatNode>(GUIDGenerator::generate());
    }
    else if (node_type == "DoubleToInt") {
        node = std::make_unique<DoubleToIntNode>(GUIDGenerator::generate());
    }
    else if (node_type == "DoubleToText") {
        node = std::make_unique<DoubleToTextNode>(GUIDGenerator::generate());
    }

    if (node_type == "FloatToDouble") {
        node = std::make_unique<FloatToDoubleNode>(GUIDGenerator::generate());
    }
    else if (node_type == "FloatToInt") {
        node = std::make_unique<FloatToIntNode>(GUIDGenerator::generate());
    }
    else if (node_type == "FloatToText") {
        node = std::make_unique<FloatToTextNode>(GUIDGenerator::generate());
    }

    else if (node_type == "BoolToText") {
        node = std::make_unique<BoolToTextNode>(GUIDGenerator::generate());
    }

    if (node_type == "GetIntArrayElement") {
        node = std::make_unique<GetIntArrayElementNode>(GUIDGenerator::generate());
    }
    else if (node_type == "AddIntArrayElement") {
        node = std::make_unique<AddIntArrayElementNode>(GUIDGenerator::generate());
    }
    else if (node_type == "SizeIntArrayElement") {
        node = std::make_unique<SizeIntArrayElementNode>(GUIDGenerator::generate());
    }
    else if (node_type == "ClearIntArrayElement") {
        node = std::make_unique<ClearIntArrayElementNode>(GUIDGenerator::generate());
    }
    else if (node_type == "RemoveIntArrayElement") {
        node = std::make_unique<RemoveIntArrayElementNode>(GUIDGenerator::generate());
    }
    else if (node_type == "GetFloatArrayElement") {
        node = std::make_unique<GetFloatArrayElementNode>(GUIDGenerator::generate());
    }
    else if (node_type == "AddFloatArrayElement") {
        node = std::make_unique<AddFloatArrayElementNode>(GUIDGenerator::generate());
    }
    else if (node_type == "SizeFloatArrayElement") {
        node = std::make_unique<SizeFloatArrayElementNode>(GUIDGenerator::generate());
    }
    else if (node_type == "ClearFloatArrayElement") {
        node = std::make_unique<ClearFloatArrayElementNode>(GUIDGenerator::generate());
    }
    else if (node_type == "RemoveFloatArrayElement") {
        node = std::make_unique<RemoveFloatArrayElementNode>(GUIDGenerator::generate());
    }
    else if (node_type == "GetDoubleArrayElement") {
        node = std::make_unique<GetDoubleArrayElementNode>(GUIDGenerator::generate());
    }
    else if (node_type == "AddDoubleArrayElement") {
        node = std::make_unique<AddDoubleArrayElementNode>(GUIDGenerator::generate());
    }
    else if (node_type == "SizeDoubleArrayElement") {
        node = std::make_unique<SizeDoubleArrayElementNode>(GUIDGenerator::generate());
    }
    else if (node_type == "ClearDoubleArrayElement") {
        node = std::make_unique<ClearDoubleArrayElementNode>(GUIDGenerator::generate());
    }
    else if (node_type == "RemoveDoubleArrayElement") {
        node = std::make_unique<RemoveDoubleArrayElementNode>(GUIDGenerator::generate());
    }
    else if (node_type == "GetBoolArrayElement") {
        node = std::make_unique<GetBoolArrayElementNode>(GUIDGenerator::generate());
    }
    else if (node_type == "AddBoolArrayElement") {
        node = std::make_unique<AddBoolArrayElementNode>(GUIDGenerator::generate());
    }
    else if (node_type == "SizeBoolArrayElement") {
        node = std::make_unique<SizeBoolArrayElementNode>(GUIDGenerator::generate());
    }
    else if (node_type == "ClearBoolArrayElement") {
        node = std::make_unique<ClearBoolArrayElementNode>(GUIDGenerator::generate());
    }
    else if (node_type == "RemoveBoolArrayElement") {
        node = std::make_unique<RemoveBoolArrayElementNode>(GUIDGenerator::generate());
    }
    else if (node_type == "GetStringArrayElement") {
        node = std::make_unique<GetStringArrayElementNode>(GUIDGenerator::generate());
    }
    else if (node_type == "AddStringArrayElement") {
        node = std::make_unique<AddStringArrayElementNode>(GUIDGenerator::generate());
    }
    else if (node_type == "SizeStringArrayElement") {
        node = std::make_unique<SizeStringArrayElementNode>(GUIDGenerator::generate());
    }
    else if (node_type == "ClearStringArrayElement") {
        node = std::make_unique<ClearStringArrayElementNode>(GUIDGenerator::generate());
    }
    else if (node_type == "RemoveStringArrayElement") {
        node = std::make_unique<RemoveStringArrayElementNode>(GUIDGenerator::generate());
    }

    else if (node_type == "ForLoop") {
        node = std::make_unique<ForLoopNode>(GUIDGenerator::generate());
    }

    if (node_type == "ConstInt") {
        node = std::make_unique<ConstIntNode>(GUIDGenerator::generate());
    }
    else if (node_type == "ConstFloat") {
        node = std::make_unique<ConstFloatNode>(GUIDGenerator::generate());
    }
    else if (node_type == "ConstDouble") {
        node = std::make_unique<ConstDoubleNode>(GUIDGenerator::generate());
    }
    else if (node_type == "ConstBool") {
        node = std::make_unique<ConstBoolNode>(GUIDGenerator::generate());
    }
    else if (node_type == "ConstText") {
        node = std::make_unique<ConstTextNode>(GUIDGenerator::generate());
    }

    if (node_type == "IntMinusInt") {
        node = std::make_unique<IntMinusIntNode>(GUIDGenerator::generate());
    }
    else if (node_type == "IntMinusFloat") {
        node = std::make_unique<IntMinusFloatNode>(GUIDGenerator::generate());
    }
    else if (node_type == "IntMinusDouble") {
        node = std::make_unique<IntMinusDoubleNode>(GUIDGenerator::generate());
    }
    else if (node_type == "IntPlusInt") {
        node = std::make_unique<IntPlusIntNode>(GUIDGenerator::generate());
    }
    else if (node_type == "IntPlusFloat") {
        node = std::make_unique<IntPlusFloatNode>(GUIDGenerator::generate());
    }
    else if (node_type == "IntPlusDouble") {
        node = std::make_unique<IntPlusDoubleNode>(GUIDGenerator::generate());
    }
    else if (node_type == "IntMultiplyInt") {
        node = std::make_unique<IntMultiplyIntNode>(GUIDGenerator::generate());
    }
    else if (node_type == "IntMultiplyFloat") {
        node = std::make_unique<IntMultiplyFloatNode>(GUIDGenerator::generate());
    }
    else if (node_type == "IntMultiplyDouble") {
        node = std::make_unique<IntMultiplyDoubleNode>(GUIDGenerator::generate());
    }
    else if (node_type == "IntDivideInt") {
        node = std::make_unique<IntDivideIntNode>(GUIDGenerator::generate());
    }
    else if (node_type == "IntDivideFloat") {
        node = std::make_unique<IntDivideFloatNode>(GUIDGenerator::generate());
    }
    else if (node_type == "IntDivideDouble") {
        node = std::make_unique<IntDivideDoubleNode>(GUIDGenerator::generate());
    }
    
    if (node_type == "FloatMinusInt") {
        node = std::make_unique<FloatMinusIntNode>(GUIDGenerator::generate());
    }
    else if (node_type == "FloatMinusFloat") {
        node = std::make_unique<FloatMinusFloatNode>(GUIDGenerator::generate());
    }
    else if (node_type == "FloatMinusDouble") {
        node = std::make_unique<FloatMinusDoubleNode>(GUIDGenerator::generate());
    }
    else if (node_type == "FloatPlusInt") {
        node = std::make_unique<FloatPlusIntNode>(GUIDGenerator::generate());
    }
    else if (node_type == "FloatPlusFloat") {
        node = std::make_unique<FloatPlusFloatNode>(GUIDGenerator::generate());
    }
    else if (node_type == "FloatPlusDouble") {
        node = std::make_unique<FloatPlusDoubleNode>(GUIDGenerator::generate());
    }
    else if (node_type == "FloatMultiplyInt") {
        node = std::make_unique<FloatMultiplyIntNode>(GUIDGenerator::generate());
    }
    else if (node_type == "FloatMultiplyFloat") {
        node = std::make_unique<FloatMultiplyFloatNode>(GUIDGenerator::generate());
    }
    else if (node_type == "FloatMultiplyDouble") {
        node = std::make_unique<FloatMultiplyDoubleNode>(GUIDGenerator::generate());
    }
    else if (node_type == "FloatDivideInt") {
        node = std::make_unique<FloatDivideIntNode>(GUIDGenerator::generate());
    }
    else if (node_type == "FloatDivideFloat") {
        node = std::make_unique<FloatDivideFloatNode>(GUIDGenerator::generate());
    }
    else if (node_type == "FloatDivideDouble") {
        node = std::make_unique<FloatDivideDoubleNode>(GUIDGenerator::generate());
    }
    
    if (node_type == "DoubleMinusInt") {
        node = std::make_unique<DoubleMinusIntNode>(GUIDGenerator::generate());
    }
    else if (node_type == "DoubleMinusFloat") {
        node = std::make_unique<DoubleMinusFloatNode>(GUIDGenerator::generate());
    }
    else if (node_type == "DoubleMinusDouble") {
        node = std::make_unique<DoubleMinusDoubleNode>(GUIDGenerator::generate());
    }
    else if (node_type == "DoublePlusInt") {
        node = std::make_unique<DoublePlusIntNode>(GUIDGenerator::generate());
    }
    else if (node_type == "DoublePlusFloat") {
        node = std::make_unique<DoublePlusFloatNode>(GUIDGenerator::generate());
    }
    else if (node_type == "DoublePlusDouble") {
        node = std::make_unique<DoublePlusDoubleNode>(GUIDGenerator::generate());
    }
    else if (node_type == "DoubleMultiplyInt") {
        node = std::make_unique<DoubleMultiplyIntNode>(GUIDGenerator::generate());
    }
    else if (node_type == "DoubleMultiplyFloat") {
        node = std::make_unique<DoubleMultiplyFloatNode>(GUIDGenerator::generate());
    }
    else if (node_type == "DoubleMultiplyDouble") {
        node = std::make_unique<DoubleMultiplyDoubleNode>(GUIDGenerator::generate());
    }
    else if (node_type == "DoubleDivideInt") {
        node = std::make_unique<DoubleDivideIntNode>(GUIDGenerator::generate());
    }
    else if (node_type == "DoubleDivideFloat") {
        node = std::make_unique<DoubleDivideFloatNode>(GUIDGenerator::generate());
    }
    else if (node_type == "DoubleDivideDouble") {
        node = std::make_unique<DoubleDivideDoubleNode>(GUIDGenerator::generate());
    }

    if (node_type == "Branch") {
        node = std::make_unique<BranchNode>(GUIDGenerator::generate());
    }
    else if (node_type == "Sequence") {
        node = std::make_unique<SequenceNode>(GUIDGenerator::generate());
        }
    else if (node_type == "PrintString") {
        node = std::make_unique<PrintString>(GUIDGenerator::generate());
    }

    if (node) {
        node->position = world_pos;
        std::string guid = node->node_guid;
        addNode(std::move(node));

        BasicNode* created_node = getNodeByGuid(guid);
        if (created_node && created_node->inputs.size() > 0 && connection_start_pin) {
            for (int i = 0; i <= created_node->inputs.size() - 1; i++)
            {
                PinIn* input = dynamic_cast<PinIn*>(created_node->inputs[i].get());
                if (connection_start_pin->type == input->type)
                {
                    if (input && connection_start_pin->type == input->type) {
                        createConnection(connection_start_pin, input);
                        if (created_node->outputs.size() > 0) {
                            creating_connection = true;
                            connection_start_pin = dynamic_cast<PinOut*>(created_node->outputs[0].get());
                        }
                    }
                    break;
                }
            }

        }

        ImGui::CloseCurrentPopup();
    }
}

void BlueprintManager::drawContextMenu() {
    if(!FirstEntry)
    {
        auto node = std::make_unique<EntryNode>("Entry");
        node->position = ImVec2(200, 400);
        addNode(std::move(node));
        FirstEntry = true;
    }

    if (show_context_menu) {
        ImGui::OpenPopup("ContextMenu");
        show_context_menu = false;
    }

    if (ImGui::BeginPopup("ContextMenu")) {
        ImVec2 world_pos = screenToWorld(context_menu_pos);

        if (ImGui::BeginMenu("Execution Nodes")) {
            if (ImGui::MenuItem("Branch")) {
                auto node = std::make_unique<BranchNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Sequence")) {
                auto node = std::make_unique<SequenceNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Print String")) {
                auto node = std::make_unique<PrintString>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Exchanges")) {
            if (ImGui::BeginMenu("Bybit")) {
                if (ImGui::MenuItem("Long")) {
                    auto node = std::make_unique<BybiyMarginLongNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Short")) {
                    auto node = std::make_unique<BybiyMarginShortNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Close Long Position")) {
                    auto node = std::make_unique<BybiyMarginCloseLongPositionNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Close Short Position")) {
                    auto node = std::make_unique<BybiyMarginCloseShortPositionNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Chart Methods")) {
            if (ImGui::MenuItem("Add Mark")) {
                auto node = std::make_unique<AddMarkNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Add Line")) {
                auto node = std::make_unique<AddLineNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Get Current Candle")) {
                auto node = std::make_unique<GetCurrentCandleDataNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Get Candle At Index")) {
                auto node = std::make_unique<GetCandleDataAtIndexNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Get Current Candle Index")) {
                auto node = std::make_unique<GetCurrentCandleIndexNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Indicators")) {
            if (ImGui::MenuItem("Engulfing Pattern")) {
                auto node = std::make_unique<EngulfingPatternNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Center Of Gravity")) {
                auto node = std::make_unique<CenterOfGravityNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Hammer Pattern")) {
                auto node = std::make_unique<HammerPatternNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Doji Pattern")) {
                auto node = std::make_unique<DojiPatternNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Lowest")) {
                auto node = std::make_unique<LowestNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Highest")) {
                auto node = std::make_unique<HighestNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("VWAP")) {
                auto node = std::make_unique<VWAPNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("MFI")) {
                auto node = std::make_unique<MFINode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Williams %R")) {
                auto node = std::make_unique<WilliamsRNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("CCI")) {
                auto node = std::make_unique<CCINode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("ATR")) {
                auto node = std::make_unique<ATRNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Stochastic")) {
                auto node = std::make_unique<StochasticNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Bollinger Bands")) {
                auto node = std::make_unique<BollingerBandsNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("MACD")) {
                auto node = std::make_unique<MACDNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("RSI")) {
                auto node = std::make_unique<RSINode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("EMA")) {
                auto node = std::make_unique<EMANode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("SMA")) {
                auto node = std::make_unique<SMANode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndMenu();
        }


        if (ImGui::BeginMenu("Array")) {
            if (ImGui::BeginMenu("Int")) {
                if (ImGui::MenuItem("Get")) {
                    auto node = std::make_unique<GetIntArrayElementNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Add")) {
                    auto node = std::make_unique<AddIntArrayElementNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Size")) {
                    auto node = std::make_unique<SizeIntArrayElementNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Clear")) {
                    auto node = std::make_unique<ClearIntArrayElementNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Remove")) {
                    auto node = std::make_unique<RemoveIntArrayElementNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Float")) {
                if (ImGui::MenuItem("Get")) {
                    auto node = std::make_unique<GetFloatArrayElementNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Add")) {
                    auto node = std::make_unique<AddFloatArrayElementNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Size")) {
                    auto node = std::make_unique<SizeFloatArrayElementNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Clear")) {
                    auto node = std::make_unique<ClearFloatArrayElementNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Remove")) {
                    auto node = std::make_unique<RemoveFloatArrayElementNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Double")) {
                if (ImGui::MenuItem("Get")) {
                    auto node = std::make_unique<GetDoubleArrayElementNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Add")) {
                    auto node = std::make_unique<AddDoubleArrayElementNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Size")) {
                    auto node = std::make_unique<SizeDoubleArrayElementNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Clear")) {
                    auto node = std::make_unique<ClearDoubleArrayElementNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Remove")) {
                    auto node = std::make_unique<RemoveDoubleArrayElementNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Bool")) {
                if (ImGui::MenuItem("Get")) {
                    auto node = std::make_unique<GetBoolArrayElementNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Add")) {
                    auto node = std::make_unique<AddBoolArrayElementNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Size")) {
                    auto node = std::make_unique<SizeBoolArrayElementNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Clear")) {
                    auto node = std::make_unique<ClearBoolArrayElementNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Remove")) {
                    auto node = std::make_unique<RemoveBoolArrayElementNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("String")) {
                if (ImGui::MenuItem("Get")) {
                    auto node = std::make_unique<GetStringArrayElementNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Add")) {
                    auto node = std::make_unique<AddStringArrayElementNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Size")) {
                    auto node = std::make_unique<SizeStringArrayElementNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Clear")) {
                    auto node = std::make_unique<ClearStringArrayElementNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Remove")) {
                    auto node = std::make_unique<RemoveStringArrayElementNode>(GUIDGenerator::generate());
                    node->position = world_pos;
                    addNode(std::move(node));
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Loops")) {
            if (ImGui::MenuItem("For loop")) {
                auto node = std::make_unique<ForLoopNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Logic")) {
            if (ImGui::MenuItem("AND")) {
                auto node = std::make_unique<BoolAndNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("OR")) {
                auto node = std::make_unique<BoolOrNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Bool Equal")) {
                auto node = std::make_unique<BoolEqualNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Bool Not Equal")) {
                auto node = std::make_unique<BoolNotEqualNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Math")) {
            if (ImGui::BeginMenu("INT Operations")) {
                if (ImGui::BeginMenu("INT * INT")) {
                    if (ImGui::MenuItem("+")) {
                        auto node = std::make_unique<IntPlusIntNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("-")) {
                        auto node = std::make_unique<IntMinusIntNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("*")) {
                        auto node = std::make_unique<IntMultiplyIntNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("/")) {
                        auto node = std::make_unique<IntDivideIntNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("INT * FLOAT")) {
                    if (ImGui::MenuItem("+")) {
                        auto node = std::make_unique<IntPlusFloatNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("-")) {
                        auto node = std::make_unique<IntMinusFloatNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("*")) {
                        auto node = std::make_unique<IntMultiplyFloatNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("/")) {
                        auto node = std::make_unique<IntDivideFloatNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("INT * DOUBLE")) {
                    if (ImGui::MenuItem("+")) {
                        auto node = std::make_unique<IntPlusDoubleNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("-")) {
                        auto node = std::make_unique<IntMinusDoubleNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("*")) {
                        auto node = std::make_unique<IntMultiplyDoubleNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("/")) {
                        auto node = std::make_unique<IntDivideDoubleNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("FLOAT Operations")) {

                if (ImGui::BeginMenu("FLOAT * FLOAT")) {
                    if (ImGui::MenuItem("+")) {
                        auto node = std::make_unique<FloatPlusFloatNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("-")) {
                        auto node = std::make_unique<FloatMinusFloatNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("*")) {
                        auto node = std::make_unique<FloatMultiplyFloatNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("/")) {
                        auto node = std::make_unique<FloatDivideFloatNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("FLOAT * INT")) {
                    if (ImGui::MenuItem("+")) {
                        auto node = std::make_unique<FloatPlusIntNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("-")) {
                        auto node = std::make_unique<FloatMinusIntNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("*")) {
                        auto node = std::make_unique<FloatMultiplyIntNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("/")) {
                        auto node = std::make_unique<FloatDivideIntNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("FLOAT * DOUBLE")) {
                    if (ImGui::MenuItem("+")) {
                        auto node = std::make_unique<FloatPlusDoubleNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("-")) {
                        auto node = std::make_unique<FloatMinusDoubleNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("*")) {
                        auto node = std::make_unique<FloatMultiplyDoubleNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("/")) {
                        auto node = std::make_unique<FloatDivideDoubleNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("DOUBLE Operations")) {

                if (ImGui::BeginMenu("DOUBLE * DOUBLE")) {
                    if (ImGui::MenuItem("+")) {
                        auto node = std::make_unique<DoublePlusDoubleNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("-")) {
                        auto node = std::make_unique<DoubleMinusDoubleNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("*")) {
                        auto node = std::make_unique<DoubleMultiplyDoubleNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("/")) {
                        auto node = std::make_unique<DoubleMultiplyDoubleNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("DOUBLE * INT")) {
                    if (ImGui::MenuItem("+")) {
                        auto node = std::make_unique<DoublePlusIntNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("-")) {
                        auto node = std::make_unique<DoubleMinusIntNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("*")) {
                        auto node = std::make_unique<DoubleMultiplyIntNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("/")) {
                        auto node = std::make_unique<DoubleDivideIntNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("DOUBLE * FLOAT")) {
                    if (ImGui::MenuItem("+")) {
                        auto node = std::make_unique<DoublePlusFloatNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("-")) {
                        auto node = std::make_unique<DoubleMinusFloatNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("*")) {
                        auto node = std::make_unique<DoubleMultiplyFloatNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("/")) {
                        auto node = std::make_unique<DoubleDivideFloatNode>(GUIDGenerator::generate());
                        node->position = world_pos;
                        addNode(std::move(node));
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndMenu();
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Converters")) {
            if (ImGui::MenuItem("Int to Float")) {
                auto node = std::make_unique<IntToFloatNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Int to Double")) {
                auto node = std::make_unique<IntToDoubleNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Int to Text")) {
                auto node = std::make_unique<IntToTextNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Float to Int")) {
                auto node = std::make_unique<FloatToIntNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Float to Double")) {
                auto node = std::make_unique<FloatToDoubleNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Float to Text")) {
                auto node = std::make_unique<FloatToTextNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Double to Int")) {
                auto node = std::make_unique<DoubleToIntNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Double to Float")) {
                auto node = std::make_unique<DoubleToFloatNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Double to Text")) {
                auto node = std::make_unique<DoubleToTextNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Bool to Text")) {
                auto node = std::make_unique<BoolToTextNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Constants")) {
            if (ImGui::MenuItem("Int")) {
                auto node = std::make_unique<ConstIntNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Float")) {
                auto node = std::make_unique<ConstFloatNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Double")) {
                auto node = std::make_unique<ConstDoubleNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Bool")) {
                auto node = std::make_unique<ConstBoolNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Text")) {
                auto node = std::make_unique<ConstTextNode>(GUIDGenerator::generate());
                node->position = world_pos;
                addNode(std::move(node));
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();
        if (ImGui::MenuItem("Cancel")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

BasicNode* BlueprintManager::createAutoConverter(PinType from_type, PinType to_type, ImVec2 position) {
    std::unique_ptr<BasicNode> converter = nullptr;

    if (from_type == PinType::INT && to_type == PinType::FLOAT) {
        converter = std::make_unique<IntToFloatNode>(GUIDGenerator::generate());
    }
    else if (from_type == PinType::INT && to_type == PinType::DOUBLE) {
        converter = std::make_unique<IntToDoubleNode>(GUIDGenerator::generate());
    }
    else if (from_type == PinType::INT && to_type == PinType::STRING) {
        converter = std::make_unique<IntToTextNode>(GUIDGenerator::generate());
    }

    else if (from_type == PinType::FLOAT && to_type == PinType::INT) {
        converter = std::make_unique<FloatToIntNode>(GUIDGenerator::generate());
    }
    else if (from_type == PinType::FLOAT && to_type == PinType::DOUBLE) {
        converter = std::make_unique<FloatToDoubleNode>(GUIDGenerator::generate());
    }
    else if (from_type == PinType::FLOAT && to_type == PinType::STRING) {
        converter = std::make_unique<FloatToTextNode>(GUIDGenerator::generate());
    }

    else if (from_type == PinType::DOUBLE && to_type == PinType::INT) {
        converter = std::make_unique<DoubleToIntNode>(GUIDGenerator::generate());
    }
    else if (from_type == PinType::DOUBLE && to_type == PinType::FLOAT) {
        converter = std::make_unique<DoubleToFloatNode>(GUIDGenerator::generate());
    }
    else if (from_type == PinType::DOUBLE && to_type == PinType::STRING) {
        converter = std::make_unique<DoubleToTextNode>(GUIDGenerator::generate());
    }

    else if (from_type == PinType::BOOL && to_type == PinType::STRING) {
        converter = std::make_unique<BoolToTextNode>(GUIDGenerator::generate());
    }

    if (converter) {
        converter->position = position;
        BasicNode* converter_ptr = converter.get();
        addNode(std::move(converter));
        return converter_ptr;
    }

    return nullptr;
}

void BlueprintManager::drawGrid(ImDrawList* draw_list) {
    draw_list->AddRectFilled(viewport_pos,
        ImVec2(viewport_pos.x + viewport_size.x, viewport_pos.y + viewport_size.y),
        IM_COL32(40, 40, 40, 255));
    float grid_size = 50.0f;
    float grid_offset_x = fmod(camera_offset.x, grid_size);
    float grid_offset_y = fmod(camera_offset.y, grid_size);

    for (float x = viewport_pos.x + grid_offset_x; x < viewport_pos.x + viewport_size.x; x += grid_size) {
        draw_list->AddLine(ImVec2(x, viewport_pos.y),
            ImVec2(x, viewport_pos.y + viewport_size.y),
            IM_COL32(80, 80, 80, 150));
    }

    for (float y = viewport_pos.y + grid_offset_y; y < viewport_pos.y + viewport_size.y; y += grid_size) {
        draw_list->AddLine(ImVec2(viewport_pos.x, y),
            ImVec2(viewport_pos.x + viewport_size.x, y),
            IM_COL32(80, 80, 80, 150));
    }
}

ImVec2 BlueprintManager::screenToWorld(ImVec2 screen_pos) {
    return ImVec2(screen_pos.x - viewport_pos.x - camera_offset.x,
        screen_pos.y - viewport_pos.y - camera_offset.y);
}

ImVec2 BlueprintManager::worldToScreen(ImVec2 world_pos) {
    return ImVec2(world_pos.x + camera_offset.x + viewport_pos.x,
        world_pos.y + camera_offset.y + viewport_pos.y);
}

void BlueprintManager::drawConnections() {
    ImGui::SetNextWindowPos(viewport_pos);
    ImGui::SetNextWindowSize(viewport_size);
    if (ImGui::Begin("##Connections", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs)) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        for (const auto& conn : connections) {
            if (conn.from_pin && conn.to_pin) {
                BasicNode* from_node = nullptr;
                BasicNode* to_node = nullptr;
                for (auto& node_pair : nodes) {
                    BasicNode* node = node_pair.second.get();
                    for (auto& pin : node->outputs) {
                        if (pin.get() == conn.from_pin) {
                            from_node = node;
                            break;
                        }
                    }
                    if (from_node) break;
                }
                for (auto& node_pair : nodes) {
                    BasicNode* node = node_pair.second.get();
                    for (auto& pin : node->inputs) {
                        if (pin.get() == conn.to_pin) {
                            to_node = node;
                            break;
                        }
                    }
                    if (to_node) break;
                }
                if (from_node && to_node) {
                    ImVec2 from_screen = worldToScreen(from_node->position);
                    ImVec2 to_screen = worldToScreen(to_node->position);
                    bool from_visible = (from_screen.x >= viewport_pos.x &&
                        from_screen.x + from_node->size.x <= viewport_pos.x + viewport_size.x &&
                        from_screen.y >= viewport_pos.y &&
                        from_screen.y + from_node->size.y <= viewport_pos.y + viewport_size.y);
                    bool to_visible = (to_screen.x >= viewport_pos.x &&
                        to_screen.x + to_node->size.x <= viewport_pos.x + viewport_size.x &&
                        to_screen.y >= viewport_pos.y &&
                        to_screen.y + to_node->size.y <= viewport_pos.y + viewport_size.y);

                    bool array = false;
                    if (from_visible || to_visible) {
                        ImU32 color = IM_COL32(255, 255, 255, 255);
                        if (conn.from_pin->type == PinType::EXEC) {
                            color = WhiteColor;
                        }
                        else if (conn.from_pin->type == PinType::BOOL) {
                            color = RedColor;
                        }
                        else if (conn.from_pin->type == PinType::FLOAT) {
                            color = GreenColor;
                        }
                        else if (conn.from_pin->type == PinType::DOUBLE) {
                            color = CyanColor;
                        }
                        else if (conn.from_pin->type == PinType::STRING) {
                            color = PinkColor;
                        }
                        else if (conn.from_pin->type == PinType::INT) {
                            color = ForestGreenColor;
                        }
                        else if (conn.from_pin->type == PinType::ARRAY_BOOL) {
                            color = RedColor;
                            array = true;
                        }
                        else if (conn.from_pin->type == PinType::ARRAY_FLOAT) {
                            color = GreenColor;
                            array = true;
                        }
                        else if (conn.from_pin->type == PinType::ARRAY_DOUBLE) {
                            color = CyanColor;
                            array = true;
                        }
                        else if (conn.from_pin->type == PinType::ARRAY_STRING) {
                            color = PinkColor;
                            array = true;
                        }
                        else if (conn.from_pin->type == PinType::ARRAY_INT) {
                            color = ForestGreenColor;
                            array = true;
                        }
                        if (array)
                        drawStripedBezierConnection(draw_list, conn.from_pin->position, conn.to_pin->position, color, WhiteColor);
                        else
                        drawBezierConnection(draw_list, conn.from_pin->position, conn.to_pin->position, color);
                    }
                }
            }
        }

        if (creating_connection && connection_start_pin) {
            ImVec2 mouse_pos = ImGui::GetMousePos();
            drawBezierConnection(draw_list, connection_start_pin->position, mouse_pos, IM_COL32(0, 200, 255, 255));
        }

        if (show_selection_box) {
            ImVec2 screen_start = worldToScreen(selection_start);
            ImVec2 screen_end = worldToScreen(selection_end);

            ImVec2 min_pos = ImVec2(MIN(screen_start.x, screen_end.x),
                MIN(screen_start.y, screen_end.y));
            ImVec2 max_pos = ImVec2(MAX(screen_start.x, screen_end.x),
                MAX(screen_start.y, screen_end.y));

            draw_list->AddRect(min_pos, max_pos, IM_COL32(100, 150, 255, 150), 0.0f, 0, 2.0f);
            draw_list->AddRectFilled(min_pos, max_pos, IM_COL32(100, 150, 255, 30));
        }
    }
    ImGui::End();
}

void BlueprintManager::drawBezierConnection(ImDrawList* draw_list, ImVec2 from, ImVec2 to, ImU32 color) {
    float distance = abs(to.x - from.x) * 0.5f;
    distance = MAX(50.0f, MIN(200.0f, distance));
    ImVec2 cp1 = ImVec2(from.x + distance, from.y);
    ImVec2 cp2 = ImVec2(to.x - distance, to.y);
    draw_list->AddBezierCubic(from, cp1, cp2, to, color, 3.0f);
}

ImVec2 calculateBezierPoint(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, float t) {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    ImVec2 point;
    point.x = uuu * p0.x + 3 * uu * t * p1.x + 3 * u * tt * p2.x + ttt * p3.x;
    point.y = uuu * p0.y + 3 * uu * t * p1.y + 3 * u * tt * p2.y + ttt * p3.y;

    return point;
}

void BlueprintManager::drawStripedBezierConnection(ImDrawList* draw_list, ImVec2 from, ImVec2 to, ImU32 primary_color, ImU32 secondary_color) {
    float distance = abs(to.x - from.x) * 0.5f;
    distance = MAX(50.0f, MIN(200.0f, distance));
    ImVec2 cp1 = ImVec2(from.x + distance, from.y);
    ImVec2 cp2 = ImVec2(to.x - distance, to.y);

    float connection_length = sqrt((to.x - from.x) * (to.x - from.x) + (to.y - from.y) * (to.y - from.y));

    float stripe_length_pixels = 15.0f;

    int total_stripes = (int)(connection_length / stripe_length_pixels);
    int segments = total_stripes * 2;

    segments = MAX(10, segments);

    for (int i = 0; i < segments; i++) {
        float t1 = (float)i / segments;
        float t2 = (float)(i + 1) / segments;

        ImVec2 p1 = calculateBezierPoint(from, cp1, cp2, to, t1);
        ImVec2 p2 = calculateBezierPoint(from, cp1, cp2, to, t2);

        ImU32 color = ((i / 2) % 2 == 0) ? primary_color : secondary_color;

        draw_list->AddLine(p1, p2, color, 3.0f);
    }
}

void BlueprintManager::handleInteractions() {
    ImVec2 mouse_pos = ImGui::GetMousePos();
    ImVec2 world_mouse_pos = screenToWorld(mouse_pos);

    if (ImGui::IsMouseClicked(0) && !ImGui::IsPopupOpen("ContextMenu")) {
        handleMouseClick(mouse_pos);
    }

    if (ImGui::IsMouseReleased(0) && !ImGui::IsPopupOpen("ContextMenu")) {
        dragging_node = false;
    }

    if (ImGui::IsMouseClicked(1)) {
        bool clicked_on_pin = false;

        for (auto it = nodes.begin(); it != nodes.end(); ++it) {
            BasicNode* node = it->second.get();

            for (auto& output_pin : node->outputs) {
                if (VecLengthSqr(ImVec2(mouse_pos.x - output_pin->position.x,
                    mouse_pos.y - output_pin->position.y)) < 100) {

                    removeConnectionsFromOutput(dynamic_cast<PinOut*>(output_pin.get()));
                    clicked_on_pin = true;
                    break;
                }
            }

            if (clicked_on_pin) break;

            for (auto& input_pin : node->inputs) {
                if (VecLengthSqr(ImVec2(mouse_pos.x - input_pin->position.x,
                    mouse_pos.y - input_pin->position.y)) < 64) {

                    removeConnectionsToInput(dynamic_cast<PinIn*>(input_pin.get()));
                    clicked_on_pin = true;
                    break;
                }
            }

            if (clicked_on_pin) break;
        }

        if (!clicked_on_pin) {
            if (creating_connection && connection_start_pin) {
                show_context_menu = true;
                context_menu_pos = mouse_pos;
            }
            else {
                if (!ImGui::IsPopupOpen("ConnectionContextMenu") && !ImGui::IsPopupOpen("ContextMenu")) {
                    bool clicked_on_node = false;

                    for (auto it = nodes.begin(); it != nodes.end(); ++it) {
                        BasicNode* node = it->second.get();
                        if (world_mouse_pos.x >= node->position.x &&
                            world_mouse_pos.x <= node->position.x + node->size.x &&
                            world_mouse_pos.y >= node->position.y &&
                            world_mouse_pos.y <= node->position.y + node->size.y) {
                            clicked_on_node = true;
                            break;
                        }
                    }

                    if (!clicked_on_node) {
                        show_context_menu = true;
                        context_menu_pos = mouse_pos;
                    }
                }

                creating_connection = false;
                connection_start_pin = nullptr;
            }
        }
    }

    if (ImGui::IsMouseClicked(1)) {
        if (creating_connection && connection_start_pin) {
            show_context_menu = true;
            context_menu_pos = mouse_pos;
        }
        else {
            if (!ImGui::IsPopupOpen("ConnectionContextMenu") && !ImGui::IsPopupOpen("ContextMenu")) {
                bool clicked_on_node = false;

                for (auto it = nodes.begin(); it != nodes.end(); ++it) {
                    BasicNode* node = it->second.get();
                    if (world_mouse_pos.x >= node->position.x &&
                        world_mouse_pos.x <= node->position.x + node->size.x &&
                        world_mouse_pos.y >= node->position.y &&
                        world_mouse_pos.y <= node->position.y + node->size.y) {
                        clicked_on_node = true;
                        break;
                    }
                }

                if (!clicked_on_node) {
                    show_context_menu = true;
                    context_menu_pos = mouse_pos;
                }

                creating_connection = false;
                connection_start_pin = nullptr;
            }
        }
    }

    if (dragging_node && ImGui::IsMouseDragging(0) && !is_panning) {
        BasicNode* dragged_node = getNodeByGuid(dragged_node_guid);
        if (dragged_node) {
            ImVec2 start_pos = drag_start_positions[dragged_node_guid];
            ImVec2 new_pos = ImVec2(world_mouse_pos.x - drag_offset.x,
                world_mouse_pos.y - drag_offset.y);

            ImVec2 delta = ImVec2(new_pos.x - start_pos.x, new_pos.y - start_pos.y);

            if (isNodeSelected(dragged_node_guid)) {
                for (const std::string& guid : selected_nodes) {
                    BasicNode* node = getNodeByGuid(guid);
                    if (node && drag_start_positions.find(guid) != drag_start_positions.end()) {
                        ImVec2 node_start_pos = drag_start_positions[guid];
                        node->position = ImVec2(node_start_pos.x + delta.x, node_start_pos.y + delta.y);
                    }
                }
            }
            else {
                dragged_node->position = new_pos;
            }
        }
    }
}

void BlueprintManager::drawConnectionContextMenu() {
    if (show_context_menu && creating_connection && connection_start_pin) {
        ImGui::OpenPopup("ConnectionContextMenu");
        show_context_menu = false;
    }

    if (ImGui::BeginPopup("ConnectionContextMenu")) {
        ImVec2 world_pos = screenToWorld(context_menu_pos);
        PinType start_type = connection_start_pin->type;

        if(start_type == PinType::INT || start_type == PinType::FLOAT || start_type == PinType::DOUBLE || start_type == PinType::BOOL)
        if (ImGui::BeginMenu("Converters")) {
            if (start_type == PinType::INT) {
                if (ImGui::MenuItem("Int to Float")) {
                    createNodeFromMenu("IntToFloat", world_pos);
                }
                if (ImGui::MenuItem("Int to Double")) {
                    createNodeFromMenu("IntToDouble", world_pos);
                }
                if (ImGui::MenuItem("Int to Text")) {
                    createNodeFromMenu("IntToText", world_pos);
                }
            }
            else if (start_type == PinType::FLOAT) {
                if (ImGui::MenuItem("Float to Int")) {
                    createNodeFromMenu("FloatToInt", world_pos);
                }
                if (ImGui::MenuItem("Float to Double")) {
                    createNodeFromMenu("FloatToDouble", world_pos);
                }
                if (ImGui::MenuItem("Float to Text")) {
                    createNodeFromMenu("FloatToText", world_pos);
                }
            }
            else if (start_type == PinType::DOUBLE) {
                if (ImGui::MenuItem("Double to Int")) {
                    createNodeFromMenu("DoubleToInt", world_pos);
                }
                if (ImGui::MenuItem("Double to Float")) {
                    createNodeFromMenu("DoubleToFloat", world_pos);
                }
                if (ImGui::MenuItem("Double to Text")) {
                    createNodeFromMenu("DoubleToText", world_pos);
                }
            }
            else if (start_type == PinType::BOOL) {
                if (ImGui::MenuItem("Bool to Text")) {
                    createNodeFromMenu("BoolToText", world_pos);
                }
            }
            ImGui::EndMenu();
        }

        if ((start_type == PinType::ARRAY_INT || start_type == PinType::ARRAY_FLOAT || start_type == PinType::ARRAY_DOUBLE || start_type == PinType::ARRAY_BOOL || start_type == PinType::ARRAY_STRING) && ImGui::BeginMenu("Array")) {
            if (start_type == PinType::ARRAY_INT) {
                if (ImGui::MenuItem("Get")) {
                    createNodeFromMenu("GetIntArrayElement", world_pos);
                }
                if (ImGui::MenuItem("Add")) {
                    createNodeFromMenu("AddIntArrayElement", world_pos);
                }
                if (ImGui::MenuItem("Size")) {
                    createNodeFromMenu("SizeIntArrayElement", world_pos);
                }
                if (ImGui::MenuItem("Clear")) {
                    createNodeFromMenu("ClearIntArrayElement", world_pos);
                }
                if (ImGui::MenuItem("Remove")) {
                    createNodeFromMenu("RemoveIntArrayElement", world_pos);
                }
            }
            else if (start_type == PinType::ARRAY_FLOAT) {
                if (ImGui::MenuItem("Get")) {
                    createNodeFromMenu("GetFloatArrayElement", world_pos);
                }
                if (ImGui::MenuItem("Add")) {
                    createNodeFromMenu("AddFloatArrayElement", world_pos);
                }
                if (ImGui::MenuItem("Size")) {
                    createNodeFromMenu("SizeFloatArrayElement", world_pos);
                }
                if (ImGui::MenuItem("Clear")) {
                    createNodeFromMenu("ClearFloatArrayElement", world_pos);
                }
                if (ImGui::MenuItem("Remove")) {
                    createNodeFromMenu("RemoveFloatArrayElement", world_pos);
                }
            }
            else if (start_type == PinType::ARRAY_DOUBLE) {
                if (ImGui::MenuItem("Get")) {
                    createNodeFromMenu("GetDoubleArrayElement", world_pos);
                }
                if (ImGui::MenuItem("Add")) {
                    createNodeFromMenu("AddDoubleArrayElement", world_pos);
                }
                if (ImGui::MenuItem("Size")) {
                    createNodeFromMenu("SizeDoubleArrayElement", world_pos);
                }
                if (ImGui::MenuItem("Clear")) {
                    createNodeFromMenu("ClearDoubleArrayElement", world_pos);
                }
                if (ImGui::MenuItem("Remove")) {
                    createNodeFromMenu("RemoveDoubleArrayElement", world_pos);
                }
            }
            else if (start_type == PinType::ARRAY_BOOL) {
                if (ImGui::MenuItem("Get")) {
                    createNodeFromMenu("GetBoolArrayElement", world_pos);
                }
                if (ImGui::MenuItem("Add")) {
                    createNodeFromMenu("AddBoolArrayElement", world_pos);
                }
                if (ImGui::MenuItem("Size")) {
                    createNodeFromMenu("SizeBoolArrayElement", world_pos);
                }
                if (ImGui::MenuItem("Clear")) {
                    createNodeFromMenu("ClearBoolArrayElement", world_pos);
                }
                if (ImGui::MenuItem("Remove")) {
                    createNodeFromMenu("RemoveBoolArrayElement", world_pos);
                }
            }
            else if (start_type == PinType::ARRAY_STRING) {
                if (ImGui::MenuItem("Get")) {
                    createNodeFromMenu("GetStringArrayElement", world_pos);
                }
                if (ImGui::MenuItem("Add")) {
                    createNodeFromMenu("AddStringArrayElement", world_pos);
                }
                if (ImGui::MenuItem("Size")) {
                    createNodeFromMenu("SizeStringArrayElement", world_pos);
                }
                if (ImGui::MenuItem("Clear")) {
                    createNodeFromMenu("ClearStringArrayElement", world_pos);
                }
                if (ImGui::MenuItem("Remove")) {
                    createNodeFromMenu("RemoveStringArrayElement", world_pos);
                }
            }
            ImGui::EndMenu();
        }

        if (start_type == PinType::BOOL && ImGui::BeginMenu("Logic")) {
            if (ImGui::MenuItem("AND")) {
                createNodeFromMenu("BoolAnd", world_pos);
            }
            if (ImGui::MenuItem("OR")) {
                createNodeFromMenu("BoolOr", world_pos);
            }
            ImGui::EndMenu();
        }

        if ((start_type == PinType::INT || start_type == PinType::FLOAT || start_type == PinType::DOUBLE)
            && ImGui::BeginMenu("Comparison")) {

            if (start_type == PinType::INT) {
                if (ImGui::MenuItem("!=")) {
                    createNodeFromMenu("IntNotEqual", world_pos);
                }
                if (ImGui::MenuItem("==")) {
                    createNodeFromMenu("IntEqual", world_pos);
                }
                if (ImGui::MenuItem("<")) {
                    createNodeFromMenu("IntLessThan", world_pos);
                }
                if (ImGui::MenuItem(">")) {
                    createNodeFromMenu("IntMoreThan", world_pos);
                }
            }
            else if (start_type == PinType::FLOAT) {
                if (ImGui::MenuItem("!=")) {
                    createNodeFromMenu("FloatNotEqual", world_pos);
                }
                if (ImGui::MenuItem("==")) {
                    createNodeFromMenu("FloatEqual", world_pos);
                }
                if (ImGui::MenuItem("<")) {
                    createNodeFromMenu("FloatLessThan", world_pos);
                }
                if (ImGui::MenuItem(">")) {
                    createNodeFromMenu("FloatMoreThan", world_pos);
                }
            }
            else if (start_type == PinType::DOUBLE) {
                if (ImGui::MenuItem("!=")) {
                    createNodeFromMenu("DoubleNotEqual", world_pos);
                }
                if (ImGui::MenuItem("==")) {
                    createNodeFromMenu("DoubleEqual", world_pos);
                }
                if (ImGui::MenuItem("<")) {
                    createNodeFromMenu("DoubleLessThan", world_pos);
                }
                if (ImGui::MenuItem(">")) {
                    createNodeFromMenu("DoubleMoreThan", world_pos);
                }
            }
            else if (start_type == PinType::BOOL) {
                if (ImGui::MenuItem("!=")) {
                    createNodeFromMenu("BoolNotEqual", world_pos);
                }
                if (ImGui::MenuItem("==")) {
                    createNodeFromMenu("BoolEqual", world_pos);
                }
            }
            ImGui::EndMenu();
        }

        if ((start_type == PinType::INT || start_type == PinType::FLOAT || start_type == PinType::DOUBLE)
            && ImGui::BeginMenu("Math")) {

            if (start_type == PinType::INT) {
                if (ImGui::BeginMenu("INT + INT")) {
                    if (ImGui::MenuItem("+")) {
                        createNodeFromMenu("IntPlusInt", world_pos);
                    }
                    if (ImGui::MenuItem("-")) {
                        createNodeFromMenu("IntMinusInt", world_pos);
                    }
                    if (ImGui::MenuItem("*")) {
                        createNodeFromMenu("IntMultiplyInt", world_pos);
                    }
                    if (ImGui::MenuItem("/")) {
                        createNodeFromMenu("IntDivideInt", world_pos);
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("INT + FLOAT")) {
                    if (ImGui::MenuItem("+")) {
                        createNodeFromMenu("IntPlusFloat", world_pos);
                    }
                    if (ImGui::MenuItem("-")) {
                        createNodeFromMenu("IntMinusFloat", world_pos);
                    }
                    if (ImGui::MenuItem("*")) {
                        createNodeFromMenu("IntMultiplyFloat", world_pos);
                    }
                    if (ImGui::MenuItem("/")) {
                        createNodeFromMenu("IntDivideFloat", world_pos);
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("INT + DOUBLE")) {
                    if (ImGui::MenuItem("+")) {
                        createNodeFromMenu("IntPlusDouble", world_pos);
                    }
                    if (ImGui::MenuItem("-")) {
                        createNodeFromMenu("IntMinusDouble", world_pos);
                    }
                    if (ImGui::MenuItem("*")) {
                        createNodeFromMenu("IntMultiplyDouble", world_pos);
                    }
                    if (ImGui::MenuItem("/")) {
                        createNodeFromMenu("IntDivideDouble", world_pos);
                    }
                    ImGui::EndMenu();
                }
            }
            else if (start_type == PinType::FLOAT) {
                if (ImGui::BeginMenu("FLOAT + FLOAT")) {
                    if (ImGui::MenuItem("+")) {
                        createNodeFromMenu("FloatPlusFloat", world_pos);
                    }
                    if (ImGui::MenuItem("-")) {
                        createNodeFromMenu("FloatMinusFloat", world_pos);
                    }
                    if (ImGui::MenuItem("*")) {
                        createNodeFromMenu("FloatMultiplyFloat", world_pos);
                    }
                    if (ImGui::MenuItem("/")) {
                        createNodeFromMenu("FloatDivideFloat", world_pos);
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("FLOAT + INT")) {
                    if (ImGui::MenuItem("+")) {
                        createNodeFromMenu("FloatPlusInt", world_pos);
                    }
                    if (ImGui::MenuItem("-")) {
                        createNodeFromMenu("FloatMinusInt", world_pos);
                    }
                    if (ImGui::MenuItem("*")) {
                        createNodeFromMenu("FloatMultiplyInt", world_pos);
                    }
                    if (ImGui::MenuItem("/")) {
                        createNodeFromMenu("FloatDivideInt", world_pos);
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("FLOAT + DOUBLE")) {
                    if (ImGui::MenuItem("+")) {
                        createNodeFromMenu("FloatPlusDouble", world_pos);
                    }
                    if (ImGui::MenuItem("-")) {
                        createNodeFromMenu("FloatMinusDouble", world_pos);
                    }
                    if (ImGui::MenuItem("*")) {
                        createNodeFromMenu("FloatMultiplyDouble", world_pos);
                    }
                    if (ImGui::MenuItem("/")) {
                        createNodeFromMenu("FloatDivideDouble", world_pos);
                    }
                    ImGui::EndMenu();
                }
            }
            else if (start_type == PinType::DOUBLE) {
                if (ImGui::BeginMenu("DOUBLE + DOUBLE")) {
                    if (ImGui::MenuItem("+")) {
                        createNodeFromMenu("DoublePlusDouble", world_pos);
                    }
                    if (ImGui::MenuItem("-")) {
                        createNodeFromMenu("DoubleMinusDouble", world_pos);
                    }
                    if (ImGui::MenuItem("*")) {
                        createNodeFromMenu("DoubleMultiplyDouble", world_pos);
                    }
                    if (ImGui::MenuItem("/")) {
                        createNodeFromMenu("DoubleDivideDouble", world_pos);
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("DOUBLE + INT")) {
                    if (ImGui::MenuItem("+")) {
                        createNodeFromMenu("DoublePlusInt", world_pos);
                    }
                    if (ImGui::MenuItem("-")) {
                        createNodeFromMenu("DoubleMinusInt", world_pos);
                    }
                    if (ImGui::MenuItem("*")) {
                        createNodeFromMenu("DoubleMultiplyInt", world_pos);
                    }
                    if (ImGui::MenuItem("/")) {
                        createNodeFromMenu("DoubleDivideInt", world_pos);
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("DOUBLE + FLOAT")) {
                    if (ImGui::MenuItem("+")) {
                        createNodeFromMenu("DoublePlusFloat", world_pos);
                    }
                    if (ImGui::MenuItem("-")) {
                        createNodeFromMenu("DoubleMinusFloat", world_pos);
                    }
                    if (ImGui::MenuItem("*")) {
                        createNodeFromMenu("DoubleMultiplyFloat", world_pos);
                    }
                    if (ImGui::MenuItem("/")) {
                        createNodeFromMenu("DoubleDivideFloat", world_pos);
                    }
                    ImGui::EndMenu();
                }

            }

            ImGui::EndMenu();
        }

        ImGui::Separator();
        if (ImGui::MenuItem("Cancel")) {
            creating_connection = false;
            connection_start_pin = nullptr;
        }

        ImGui::EndPopup();
    }
}

void BlueprintManager::handleMouseClick(ImVec2 mouse_pos) {
    if (is_panning) return;

    if (ImGui::IsPopupOpen("ConnectionContextMenu")) {
        return;
    }

    if (ImGui::IsPopupOpen("ContextMenu")) {
        return;
    }

    ImVec2 world_mouse_pos = screenToWorld(mouse_pos);
    bool clicked_pin = false;
    selected_node_guid = "";

    for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        BasicNode* node = it->second.get();

        for (auto& output_pin : node->outputs) {
            if (VecLengthSqr(ImVec2(mouse_pos.x - output_pin->position.x,
                mouse_pos.y - output_pin->position.y)) < 100) {
                if (!creating_connection) {
                    creating_connection = true;
                    connection_start_pin = output_pin.get();
                }
                clicked_pin = true;
                break;
            }
        }

        if (clicked_pin) break;

        if (creating_connection && connection_start_pin) {
            for (auto& input_pin : node->inputs) {
                if (VecLengthSqr(ImVec2(mouse_pos.x - input_pin->position.x,
                    mouse_pos.y - input_pin->position.y)) < 64) {
                    createConnection(connection_start_pin, input_pin.get());
                    creating_connection = false;
                    connection_start_pin = nullptr;
                    clicked_pin = true;
                    break;
                }
            }
        }

        if (clicked_pin) break;
    }

    if (!clicked_pin && !creating_connection) {
        for (auto it = nodes.begin(); it != nodes.end(); ++it) {
            BasicNode* node = it->second.get();

            if (world_mouse_pos.x >= node->position.x &&
                world_mouse_pos.x <= node->position.x + node->size.x &&
                world_mouse_pos.y >= node->position.y &&
                world_mouse_pos.y <= node->position.y + node->size.y) {

                selected_node_guid = it->first;
                dragging_node = true;
                dragged_node_guid = it->first;
                drag_offset = ImVec2(world_mouse_pos.x - node->position.x,
                    world_mouse_pos.y - node->position.y);

                drag_start_positions.clear();
                if (isNodeSelected(dragged_node_guid)) {
                    for (const std::string& guid : selected_nodes) {
                        BasicNode* selected_node = getNodeByGuid(guid);
                        if (selected_node) {
                            drag_start_positions[guid] = selected_node->position;
                        }
                    }
                }
                else {
                    drag_start_positions[dragged_node_guid] = node->position;
                }

                break;
            }
        }
    }

    if (!clicked_pin && creating_connection && !ImGui::IsPopupOpen("ConnectionContextMenu")) {
        creating_connection = false;
        connection_start_pin = nullptr;
    }
}


void BlueprintManager::saveBlueprint(const std::string& filename) {
    nlohmann::json json = serializeToJson();
    std::ofstream file(filename);
    if (file.is_open()) {
        file << json.dump(4);      
        file.close();
        std::cout << "Blueprint saved to: " << filename << std::endl;
    }
    else {
        std::cout << "Failed to save blueprint to: " << filename << std::endl;
    }
}

bool BlueprintManager::loadBlueprint(const std::string& filename) {
    variables.clear();
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Failed to open file: " << filename << std::endl;
        return false;
    }

    nlohmann::json json;
    try {
        file >> json;
        file.close();

        clearAllBeforeLoad();

        bool success = deserializeFromJson(json);
        if (success) {
            std::cout << "Blueprint loaded from: " << filename << std::endl;
        }
        else {
            std::cout << "Failed to deserialize blueprint from: " << filename << std::endl;
        }
        return success;

    }
    catch (const std::exception& e) {
        std::cout << "JSON parsing error: " << e.what() << std::endl;
        file.close();
        return false;
    }
}

nlohmann::json BlueprintManager::serializeToJson() {
    nlohmann::json json;

    json["nodes"] = nlohmann::json::array();
    for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        BasicNode* node = it->second.get();
        nlohmann::json node_json;

        node_json["guid"] = node->node_guid;
        node_json["type"] = node->getNodeType();
        node_json["name"] = node->name;
        node_json["position"]["x"] = node->position.x;
        node_json["position"]["y"] = node->position.y;
        node_json["size"]["x"] = node->size.x;
        node_json["size"]["y"] = node->size.y;

        node_json["data"] = node->serializeData();

        node_json["inputs"] = nlohmann::json::array();
        for (auto& pin : node->inputs) {
            nlohmann::json pin_json;
            pin_json["guid"] = pin->pin_guid;
            pin_json["type"] = static_cast<int>(pin->type);
            pin_json["name"] = pin->name;
            pin_json["stored_int"] = pin->default_stored_int;
            pin_json["stored_float"] = pin->default_stored_float;
            pin_json["stored_double"] = pin->default_stored_double;
            pin_json["stored_bool"] = pin->default_stored_bool;
            pin_json["stored_string"] = pin->default_stored_string;
            node_json["inputs"].push_back(pin_json);
        }

        node_json["outputs"] = nlohmann::json::array();
        for (auto& pin : node->outputs) {
            nlohmann::json pin_json;
            pin_json["guid"] = pin->pin_guid;
            pin_json["type"] = static_cast<int>(pin->type);
            pin_json["name"] = pin->name;
            node_json["outputs"].push_back(pin_json);
        }

        json["nodes"].push_back(node_json);
    }

    json["connections"] = nlohmann::json::array();
    for (const auto& conn : connections) {
        nlohmann::json conn_json;
        conn_json["from_pin"] = conn.from_pin_guid;
        conn_json["to_pin"] = conn.to_pin_guid;
        json["connections"].push_back(conn_json);
    }

    json["variables"] = nlohmann::json::array();
    for (auto it = variables.begin(); it != variables.end(); ++it) {
        nlohmann::json var_json;
        var_json["guid"] = it->second->guid;
        var_json["name"] = it->second->name;
        var_json["type"] = static_cast<int>(it->second->type);
        var_json["global"] = it->second->global;

        switch (it->second->type) {
        case PinType::INT:
            var_json["value"] = it->second->def_intValue;
            break;
        case PinType::FLOAT:
            var_json["value"] = it->second->def_floatValue;
            break;
        case PinType::DOUBLE:
            var_json["value"] = it->second->def_doubleValue;
            break;
        case PinType::BOOL:
            var_json["value"] = it->second->def_boolValue;
            break;
        case PinType::STRING:
            var_json["value"] = it->second->def_stringValue;
            break;
        }

        json["variables"].push_back(var_json);
    }

    return json;
}

bool BlueprintManager::deserializeFromJson(const nlohmann::json& json) {
    try {
        if (json.contains("variables")) {
            for (auto it = json["variables"].begin(); it != json["variables"].end(); ++it) {
                const auto& var_json = *it;
                std::string guid = var_json["guid"];
                std::string name = var_json["name"];
                PinType type = static_cast<PinType>(var_json["type"]);
                bool global = var_json["global"];
                auto variable = std::make_unique<Variable>(name, guid, type,global);

                switch (type) {
                case PinType::INT:
                    variable->def_intValue = var_json["value"].get<int>();
                    break;
                case PinType::FLOAT:
                    variable->def_floatValue = var_json["value"].get<float>();
                    break;
                case PinType::DOUBLE:
                    variable->def_doubleValue = var_json["value"].get<long double>();
                    break;
                case PinType::BOOL:
                    variable->def_boolValue = var_json["value"].get<bool>();
                    break;
                case PinType::STRING:
                    variable->def_stringValue = var_json["value"].get<std::string>();
                    break;
                }
                variable->setDefaultValue();
                variables[guid] = std::move(variable);
            }
        }

        if (json.contains("nodes")) {
            for (const auto& node_json : json["nodes"]) {
                std::string type = node_json["type"];
                std::string guid = node_json["guid"];

                std::unique_ptr<BasicNode> node = nullptr;

                if (type == "EntryNode") {
                    node = std::make_unique<EntryNode>(guid, this);
                }

                else if (type == "BybiyMarginLongNode") {
                    node = std::make_unique<BybiyMarginLongNode>(guid, this);
                }
                else if (type == "BybiyMarginShortNode") {
                    node = std::make_unique<BybiyMarginShortNode>(guid, this);
                }
                else if (type == "BybiyMarginCloseLongPositionNode") {
                    node = std::make_unique<BybiyMarginCloseLongPositionNode>(guid, this);
                }
                else if (type == "BybiyMarginCloseShortPositionNode") {
                    node = std::make_unique<BybiyMarginCloseShortPositionNode>(guid, this);
                }
                else if (type == "AddMarkNode") {
                    node = std::make_unique<AddMarkNode>(guid, this);
                }
                else if (type == "AddLineNode") {
                    node = std::make_unique<AddLineNode>(guid, this);
                }
                else if (type == "GetCurrentCandleDataNode") {
                    node = std::make_unique<GetCurrentCandleDataNode>(guid, this);
                }
                else if (type == "GetCandleDataAtIndexNode") {
                    node = std::make_unique<GetCandleDataAtIndexNode>(guid, this);
                }
                else if (type == "GetCurrentCandleIndexNode") {
                    node = std::make_unique<GetCurrentCandleIndexNode>(guid, this);
                }
                else if (type == "EngulfingPatternNode") {
                    node = std::make_unique<EngulfingPatternNode>(guid, this);
                } 
                else if (type == "CenterOfGravityNode") {
                    node = std::make_unique<CenterOfGravityNode>(guid, this);
                }
                else if (type == "HammerPatternNode") {
                    node = std::make_unique<HammerPatternNode>(guid, this);
                }
                else if (type == "DojiPatternNode") {
                    node = std::make_unique<DojiPatternNode>(guid, this);
                }
                else if (type == "LowestNode") {
                    node = std::make_unique<LowestNode>(guid, this);
                }
                else if (type == "HighestNode") {
                    node = std::make_unique<HighestNode>(guid, this);
                }
                else if (type == "VWAPNode") {
                    node = std::make_unique<VWAPNode>(guid, this);
                }
                else if (type == "MFINode") {
                    node = std::make_unique<MFINode>(guid, this);
                }
                else if (type == "WilliamsRNode") {
                    node = std::make_unique<WilliamsRNode>(guid, this);
                }
                else if (type == "CCINode") {
                    node = std::make_unique<CCINode>(guid, this);
                }
                else if (type == "ATRNode") {
                    node = std::make_unique<ATRNode>(guid, this);
                }
                else if (type == "StochasticNode") {
                    node = std::make_unique<StochasticNode>(guid, this);
                }
                else if (type == "BollingerBandsNode") {
                    node = std::make_unique<BollingerBandsNode>(guid, this);
                }
                else if (type == "MACDNode") {
                    node = std::make_unique<MACDNode>(guid, this);
                }
                else if (type == "RSINode") {
                    node = std::make_unique<RSINode>(guid, this);
                }
                else if (type == "EMANode") {
                    node = std::make_unique<EMANode>(guid, this);
                }
                else if (type == "SMANode") {
                    node = std::make_unique<SMANode>(guid, this);
                }
                else if (type == "IntNotEqualNode") {
                    node = std::make_unique<IntNotEqualNode>(guid, this);
                }
                else if (type == "IntEqualNode") {
                    node = std::make_unique<IntEqualNode>(guid, this);
                }
                else if (type == "IntLessThanNode") {
                    node = std::make_unique<IntLessThanNode>(guid, this);
                }
                else if (type == "IntMoreThanNode") {
                    node = std::make_unique<IntMoreThanNode>(guid, this);
                }
                else if (type == "DoubleNotEqualNode") {
                    node = std::make_unique<DoubleNotEqualNode>(guid, this);
                }
                else if (type == "DoubleEqualNode") {
                    node = std::make_unique<DoubleEqualNode>(guid, this);
                }
                else if (type == "DoubleLessThanNode") {
                    node = std::make_unique<DoubleLessThanNode>(guid, this);
                }
                else if (type == "DoubleMoreThanNode") {
                    node = std::make_unique<DoubleMoreThanNode>(guid, this);
                }
                else if (type == "FloatNotEqualNode") {
                    node = std::make_unique<FloatNotEqualNode>(guid, this);
                }
                else if (type == "FloatEqualNode") {
                    node = std::make_unique<FloatEqualNode>(guid, this);
                }
                else if (type == "FloatLessThanNode") {
                    node = std::make_unique<FloatLessThanNode>(guid, this);
                }
                else if (type == "FloatMoreThanNode") {
                    node = std::make_unique<FloatMoreThanNode>(guid, this);
                }
                else if (type == "BoolAndNode") {
                    node = std::make_unique<BoolAndNode>(guid, this);
                }
                else if (type == "BoolOrNode") {
                    node = std::make_unique<BoolOrNode>(guid, this);
                }
                else if (type == "BoolEqualNode") {
                    node = std::make_unique<BoolEqualNode>(guid, this);
                }
                else if (type == "BoolNotEqualNode") {
                    node = std::make_unique<BoolNotEqualNode>(guid, this);
                }
                else if (type == "IntToFloatNode") {
                    node = std::make_unique<IntToFloatNode>(guid, this);
                }
                else if (type == "IntToDoubleNode") {
                    node = std::make_unique<IntToDoubleNode>(guid, this);
                }
                else if (type == "IntToTextNode") {
                    node = std::make_unique<IntToTextNode>(guid, this);
                }
                else if (type == "DoubleToFloatNode") {
                    node = std::make_unique<DoubleToFloatNode>(guid, this);
                }
                else if (type == "DoubleToIntNode") {
                    node = std::make_unique<DoubleToIntNode>(guid, this);
                }
                else if (type == "DoubleToTextNode") {
                    node = std::make_unique<DoubleToTextNode>(guid, this);
                }
                else if (type == "FloatToDoubleNode") {
                    node = std::make_unique<FloatToDoubleNode>(guid, this);
                }
                else if (type == "FloatToIntNode") {
                    node = std::make_unique<FloatToIntNode>(guid, this);
                }
                else if (type == "FloatToTextNode") {
                    node = std::make_unique<FloatToTextNode>(guid, this);
                }
                else if (type == "BoolToTextNode") {
                    node = std::make_unique<BoolToTextNode>(guid, this);
                }

                else if (type == "GetIntArrayElementNode") {
                    node = std::make_unique<GetIntArrayElementNode>(guid, this);
                }
                else if (type == "AddIntArrayElementNode") {
                    node = std::make_unique<AddIntArrayElementNode>(guid, this);
                }
                else if (type == "GetFloatArrayElementNode") {
                    node = std::make_unique<GetFloatArrayElementNode>(guid, this);
                }
                else if (type == "AddFloatArrayElementNode") {
                    node = std::make_unique<AddFloatArrayElementNode>(guid, this);
                }
                else if (type == "GetDoubleArrayElementNode") {
                    node = std::make_unique<GetDoubleArrayElementNode>(guid, this);
                }
                else if (type == "AddDoubleArrayElementNode") {
                    node = std::make_unique<AddDoubleArrayElementNode>(guid, this);
                }
                else if (type == "GetBoolArrayElementNode") {
                    node = std::make_unique<GetBoolArrayElementNode>(guid, this);
                }
                else if (type == "AddBoolArrayElementNode") {
                    node = std::make_unique<AddBoolArrayElementNode>(guid, this);
                }
                else if (type == "GetStringArrayElementNode") {
                    node = std::make_unique<GetStringArrayElementNode>(guid, this);
                }
                else if (type == "AddStringArrayElementNode") {
                    node = std::make_unique<AddStringArrayElementNode>(guid, this);
                }

                else if (type == "ForLoopNode") {
                    node = std::make_unique<ForLoopNode>(guid, this);
                }


                else if (type == "ConstIntNode") {
                    node = std::make_unique<ConstIntNode>(guid, this);
                }
                else if (type == "ConstFloatNode") {
                    node = std::make_unique<ConstFloatNode>(guid, this);
                }
                else if (type == "ConstDoubleNode") {
                    node = std::make_unique<ConstDoubleNode>(guid, this);
                }
                else if (type == "ConstBoolNode") {
                    node = std::make_unique<ConstBoolNode>(guid, this);
                }
                else if (type == "ConstTextNode") {
                    node = std::make_unique<ConstTextNode>(guid, this);
                }
                else if (type == "IntMinusIntNode") {
                    node = std::make_unique<IntMinusIntNode>(guid, this);
                }
                else if (type == "IntMinusFloatNode") {
                    node = std::make_unique<IntMinusFloatNode>(guid, this);
                }
                else if (type == "IntMinusDoubleNode") {
                    node = std::make_unique<IntMinusDoubleNode>(guid, this);
                }
                else if (type == "IntPlusIntNode") {
                    node = std::make_unique<IntPlusIntNode>(guid, this);
                }
                else if (type == "IntPlusFloatNode") {
                    node = std::make_unique<IntPlusFloatNode>(guid, this);
                }
                else if (type == "IntPlusDoubleNode") {
                    node = std::make_unique<IntPlusDoubleNode>(guid, this);
                }
                else if (type == "IntMultiplyIntNode") {
                    node = std::make_unique<IntMultiplyIntNode>(guid, this);
                }
                else if (type == "IntMultiplyFloatNode") {
                    node = std::make_unique<IntMultiplyFloatNode>(guid, this);
                }
                else if (type == "IntMultiplyDoubleNode") {
                    node = std::make_unique<IntMultiplyDoubleNode>(guid, this);
                }
                else if (type == "IntDivideIntNode") {
                    node = std::make_unique<IntDivideIntNode>(guid, this);
                }
                else if (type == "IntDivideFloatNode") {
                    node = std::make_unique<IntDivideFloatNode>(guid, this);
                }
                else if (type == "IntDivideDoubleNode") {
                    node = std::make_unique<IntDivideDoubleNode>(guid, this);
                }

                else if (type == "FloatMinusIntNode") {
                    node = std::make_unique<FloatMinusIntNode>(guid, this);
                }
                else if (type == "FloatMinusFloatNode") {
                    node = std::make_unique<FloatMinusFloatNode>(guid, this);
                }
                else if (type == "FloatMinusDoubleNode") {
                    node = std::make_unique<FloatMinusDoubleNode>(guid, this);
                }
                else if (type == "FloatPlusIntNode") {
                    node = std::make_unique<FloatPlusIntNode>(guid, this);
                }
                else if (type == "FloatPlusFloatNode") {
                    node = std::make_unique<FloatPlusFloatNode>(guid, this);
                }
                else if (type == "FloatPlusDoubleNode") {
                    node = std::make_unique<FloatPlusDoubleNode>(guid, this);
                }
                else if (type == "FloatMultiplyIntNode") {
                    node = std::make_unique<FloatMultiplyIntNode>(guid, this);
                }
                else if (type == "FloatMultiplyFloatNode") {
                    node = std::make_unique<FloatMultiplyFloatNode>(guid, this);
                }
                else if (type == "FloatMultiplyDoubleNode") {
                    node = std::make_unique<FloatMultiplyDoubleNode>(guid, this);
                }
                else if (type == "FloatDivideIntNode") {
                    node = std::make_unique<FloatDivideIntNode>(guid, this);
                }
                else if (type == "FloatDivideFloatNode") {
                    node = std::make_unique<FloatDivideFloatNode>(guid, this);
                }
                else if (type == "FloatDivideDoubleNode") {
                    node = std::make_unique<FloatDivideDoubleNode>(guid, this);
                }

                else if (type == "DoubleMinusIntNode") {
                    node = std::make_unique<DoubleMinusIntNode>(guid, this);
                }
                else if (type == "DoubleMinusFloatNode") {
                    node = std::make_unique<DoubleMinusFloatNode>(guid, this);
                }
                else if (type == "DoubleMinusDoubleNode") {
                    node = std::make_unique<DoubleMinusDoubleNode>(guid, this);
                }
                else if (type == "DoublePlusIntNode") {
                    node = std::make_unique<DoublePlusIntNode>(guid, this);
                }
                else if (type == "DoublePlusFloatNode") {
                    node = std::make_unique<DoublePlusFloatNode>(guid, this);
                }
                else if (type == "DoublePlusDoubleNode") {
                    node = std::make_unique<DoublePlusDoubleNode>(guid, this);
                }
                else if (type == "DoubleMultiplyIntNode") {
                    node = std::make_unique<DoubleMultiplyIntNode>(guid, this);
                }
                else if (type == "DoubleMultiplyFloatNode") {
                    node = std::make_unique<DoubleMultiplyFloatNode>(guid, this);
                }
                else if (type == "DoubleMultiplyDoubleNode") {
                    node = std::make_unique<DoubleMultiplyDoubleNode>(guid, this);
                }
                else if (type == "DoubleDivideIntNode") {
                    node = std::make_unique<DoubleDivideIntNode>(guid, this);
                }
                else if (type == "DoubleDivideFloatNode") {
                    node = std::make_unique<DoubleDivideFloatNode>(guid, this);
                }
                else if (type == "DoubleDivideDoubleNode") {
                    node = std::make_unique<DoubleDivideDoubleNode>(guid, this);
                }

                else if (type == "BranchNode") {
                    node = std::make_unique<BranchNode>(guid, this);
                }
                else if (type == "SequenceNode") {
                    node = std::make_unique<SequenceNode>(guid, this);
                    }
                else if (type == "PrintString") {
                    node = std::make_unique<PrintString>(guid, this);
                }   
                else if (type == "GetVariableNode") {
                    node = std::make_unique<GetVariableNode>(guid, this);
                }
                else if (type == "SetVariableNode") {
                        node = std::make_unique<SetVariableNode>(guid, this);
                }

                if (!node) {
                    std::cout << "Unknown node type: " << type << std::endl;
                    continue;
                }

                node->position.x = node_json["position"]["x"];
                node->position.y = node_json["position"]["y"];
                if (node_json.contains("inputs")) {
                    for (size_t i = 0; i < node_json["inputs"].size() && i < node->inputs.size(); i++) {
                        const auto& pin_json = node_json["inputs"][i];
                        auto& pin = node->inputs[i];

                        pin->pin_guid = pin_json["guid"];
                        pin->default_stored_int = pin_json["stored_int"];
                        pin->default_stored_float = pin_json["stored_float"];
                        pin->default_stored_double = pin_json["stored_double"];
                        pin->default_stored_bool = pin_json["stored_bool"];
                        pin->default_stored_string = pin_json["stored_string"];
                    }
                }

                if (node_json.contains("outputs")) {
                    for (size_t i = 0; i < node_json["outputs"].size() && i < node->outputs.size(); i++) {
                        const auto& pin_json = node_json["outputs"][i];
                        auto& pin = node->outputs[i];

                        pin->pin_guid = pin_json["guid"];
                    }
                }

                if (node_json.contains("data")) {
                    node->deserializeData(node_json["data"]);
                }

                addNode(std::move(node));
            }
        }

        if (json.contains("connections")) {
            restoreConnections(json["connections"]);
        }

        return true;

    }
    catch (const std::exception& e) {
        std::cout << "Deserialization error: " << e.what() << std::endl;
        return false;
    }
}

bool BlueprintManager::LoadBlueprintMenu()
{
    scanStrategyFiles();
    showLoadMenuPopup = true;
    return true;
}

bool BlueprintManager::SaveBlueprintMenu()
{
    showSaveMenuPopup = true;
    return true;
}

void BlueprintManager::createVariable(const std::string& name, PinType type, bool isGlobal) {
    std::string guid = GUIDGenerator::generate();
    auto variable = std::make_unique<Variable>(name, guid, type, isGlobal);
    variable->setDefaultValue();
    variables[guid] = std::move(variable);
}

void BlueprintManager::removeVariable(const std::string& guid) {
    std::vector<std::string> nodesToRemove;
    for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        if (auto* getNode = dynamic_cast<GetVariableNode*>(it->second.get())) {
            if (getNode->getVariableGuid() == guid) {
                nodesToRemove.push_back(it->first);
            }
        }
        else if (auto* setNode = dynamic_cast<SetVariableNode*>(it->second.get())) {
            if (setNode->getVariableGuid() == guid) {
                nodesToRemove.push_back(it->first);
            }
        }
    }

    for (const auto& nodeGuid : nodesToRemove) {
    }

    variables.erase(guid);
}

Variable* BlueprintManager::getVariable(const std::string& guid) {
    auto it = variables.find(guid);
    return (it != variables.end()) ? it->second.get() : nullptr;
}

void BlueprintManager::renderSaveMenu() {
    if (showSaveMenuPopup) {
        ImGui::OpenPopup("Save Blueprint");
        showSaveMenuPopup = false;
    }

    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);

    if (ImGui::BeginPopupModal("Save Blueprint", nullptr, ImGuiWindowFlags_NoResize)) {

        ImGui::Text("Save Blueprint Strategy");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Strategy Name:");
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##SaveFilename", saveFilename, sizeof(saveFilename))) {
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        bool canSave = strlen(saveFilename) > 0;

        if (!canSave) {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
        }

        if (ImGui::Button("Save Strategy", ImVec2(120, 30)) && canSave) {
            std::string filename = std::string(saveFilename);

            if (!hasJsonExtension(filename)) {
                filename += ".json";
            }

            std::string fullPath = "Data/" + filename;

            if (fileExists(fullPath)) {
                ImGui::OpenPopup("FileExists");
            }
            else {
                saveBlueprint(fullPath);
                ImGui::CloseCurrentPopup();
            }
        }

        if (!canSave) {
            ImGui::PopStyleVar();
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Please enter a strategy name");
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 30))) {
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::BeginPopupModal("FileExists", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("A file with this name already exists:");
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", saveFilename);
            ImGui::Text("Do you want to overwrite it?");
            ImGui::Separator();

            if (ImGui::Button("Yes, Overwrite", ImVec2(140, 0))) {
                std::string filename = std::string(saveFilename);

                if (!hasJsonExtension(filename)) {
                    filename += ".json";
                }

                std::string fullPath = "Data/" + filename;
                saveBlueprint(fullPath);

                ImGui::CloseCurrentPopup();
                ImGui::CloseCurrentPopup();     
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::EndPopup();
    }
}

void BlueprintManager::scanStrategyFiles() {
    strategyFiles.clear();

    const char* dataPath = "Data";

#ifdef _WIN32
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind;

    std::string searchPattern = std::string(dataPath) + "\\*.json";

    hFind = FindFirstFileA(searchPattern.c_str(), &findFileData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                strategyFiles.push_back(std::string(findFileData.cFileName));
            }
        } while (FindNextFileA(hFind, &findFileData) != 0);
        FindClose(hFind);
    }
    else {
        CreateDirectoryA(dataPath, NULL);
    }

#else
    DIR* dir = opendir(dataPath);
    if (dir != NULL) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_REG) {   
                std::string filename = entry->d_name;
                if (hasJsonExtension(filename)) {
                    strategyFiles.push_back(filename);
                }
            }
        }
        closedir(dir);
    }
    else {
        mkdir(dataPath, 0755);
    }
#endif
}

bool BlueprintManager::fileExists(const std::string& filepath) {
#ifdef _WIN32
    FILE* file = nullptr;
    errno_t err = fopen_s(&file, filepath.c_str(), "r");
    if (err == 0 && file) {
        fclose(file);
        return true;
    }
    return false;
#else
    FILE* file = fopen(filepath.c_str(), "r");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
#endif
}

bool BlueprintManager::hasJsonExtension(const std::string& filename) {
    const std::string jsonExt = ".json";

    if (filename.length() < jsonExt.length()) return false;

    std::string fileEnd = filename.substr(filename.length() - jsonExt.length());
    std::transform(fileEnd.begin(), fileEnd.end(), fileEnd.begin(), ::tolower);

    return fileEnd == jsonExt;
}

void BlueprintManager::renderLoadMenu() {
    if (showLoadMenuPopup) {
        ImGui::OpenPopup("Load Blueprint");
        showLoadMenuPopup = false;
    }

    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_Always);

    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 255)));           
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 255)));           
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImGui::ColorConvertU32ToFloat4(IM_COL32(255, 255, 255, 255)));      
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImGui::ColorConvertU32ToFloat4(IM_COL32(160, 160, 160, 255)));    
    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(IM_COL32(76, 76, 76, 255)));            
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::ColorConvertU32ToFloat4(IM_COL32(140, 140, 140, 255)));    
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::ColorConvertU32ToFloat4(IM_COL32(76, 76, 76, 255)));      
    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::ColorConvertU32ToFloat4(IM_COL32(102, 102, 102, 255)));        
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::ColorConvertU32ToFloat4(IM_COL32(140, 140, 140, 255)));   
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGui::ColorConvertU32ToFloat4(IM_COL32(102, 102, 102, 255)));   
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::ColorConvertU32ToFloat4(IM_COL32(40, 40, 40, 255)));          
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(IM_COL32(255, 255, 255, 255)));          

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);

    if (ImGui::BeginPopupModal("Load Blueprint", nullptr, ImGuiWindowFlags_NoResize)) {

        ImGui::Text("Choose a blueprint to load:");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::SameLine(ImGui::GetWindowWidth() - 80);     
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::Spacing();

        if (ImGui::BeginListBox("##BlueprintFiles", ImVec2(-1, 200))) {
            for (int i = 0; i < strategyFiles.size(); i++) {
                bool isSelected = (selectedFileIndex == i);
                std::string displayName = strategyFiles[i];
                if (displayName.length() > 5 && displayName.substr(displayName.length() - 5) == ".json") {
                    displayName = displayName.substr(0, displayName.length() - 5);
                }

                if (ImGui::Selectable(displayName.c_str(), isSelected)) {
                    selectedFileIndex = i;
                    selectedStrategy = strategyFiles[i];
                }

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Blueprint: %s", strategyFiles[i].c_str());
                }
            }
            ImGui::EndListBox();
        }

        ImGui::Spacing();

        if (strategyFiles.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "No .json files found in Data folder");
        }
        else {
            ImGui::Text("Found %d blueprint files", (int)strategyFiles.size());

            if (selectedFileIndex >= 0 && selectedFileIndex < strategyFiles.size()) {
                float windowHeight = ImGui::GetWindowHeight();
                float buttonHeight = 30.0f;
                float padding = 40.0f;    

                ImGui::SetCursorPosY(windowHeight - buttonHeight - padding);

                if (ImGui::Button("Load Blueprint", ImVec2(150, 30))) {
                    std::string fullPath = "Data/" + selectedStrategy;
                    bool success = loadBlueprint(fullPath);

                    if (success) {
                        ImGui::CloseCurrentPopup();
                    }
                    else {
                        ImGui::OpenPopup("LoadError");
                    }
                }

                ImGui::SameLine();

                if (ImGui::Button("Delete", ImVec2(120, 30))) {
                    ImGui::OpenPopup("ConfirmDelete");
                }
            }
        }

        if (ImGui::BeginPopupModal("LoadError", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Failed to load blueprint!");
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Please check the file format and try again.");
            ImGui::Separator();
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("ConfirmDelete", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Are you sure you want to delete:");
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", selectedStrategy.c_str());
            ImGui::Text("This action cannot be undone!");
            ImGui::Separator();

            if (ImGui::Button("Yes, Delete", ImVec2(120, 0))) {
                std::string fullPath = "Data/" + selectedStrategy;
                if (remove(fullPath.c_str()) == 0) {
                    scanStrategyFiles();
                    selectedFileIndex = -1;
                    selectedStrategy.clear();
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::PopStyleColor(12);
    ImGui::PopStyleVar(2);
}

void BlueprintManager::handleSelectionInput() {
    ImVec2 mouse_pos = ImGui::GetMousePos();
    ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsPopupOpen("ContextMenu")) return;
    if (ImGui::IsMouseClicked(0) && !dragging_node && !creating_connection) {
        ImVec2 world_mouse_pos = screenToWorld(mouse_pos);
        bool clicked_on_node = false;

        for (auto it = nodes.begin(); it != nodes.end(); ++it) {
            BasicNode* node = it->second.get();
            if (world_mouse_pos.x >= node->position.x &&
                world_mouse_pos.x <= node->position.x + node->size.x &&
                world_mouse_pos.y >= node->position.y &&
                world_mouse_pos.y <= node->position.y + node->size.y) {

                clicked_on_node = true;

                if (!io.KeyCtrl && !isNodeSelected(it->first)) {
                    clearSelection();
                }

                if (io.KeyCtrl) {
                    if (isNodeSelected(it->first)) {
                        deselectNode(it->first);
                    }
                    else {
                        selectNode(it->first, true);
                    }
                }
                else {
                    if (!isNodeSelected(it->first)) {
                        selectNode(it->first);
                    }
                }
                break;
            }
        }

        if (!clicked_on_node && !io.KeyCtrl) {
            clearSelection();
            is_selecting = true;
            selection_start = world_mouse_pos;
            selection_end = world_mouse_pos;
            show_selection_box = true;
        }
    }

    if (is_selecting && ImGui::IsMouseDown(0)) {
        selection_end = screenToWorld(mouse_pos);
    }

    if (is_selecting && ImGui::IsMouseReleased(0)) {
        is_selecting = false;
        show_selection_box = false;

        ImVec2 min_pos = ImVec2(MIN(selection_start.x, selection_end.x),
            MIN(selection_start.y, selection_end.y));
        ImVec2 max_pos = ImVec2(MAX(selection_start.x, selection_end.x),
            MAX(selection_start.y, selection_end.y));

        selectNodesInBox(min_pos, max_pos);
    }
}

void BlueprintManager::drawSelectionBox() {
    if (!show_selection_box) return;

    ImGui::SetNextWindowPos(viewport_pos);
    ImGui::SetNextWindowSize(viewport_size);
    if (ImGui::Begin("##SelectionBox", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs)) {

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        ImVec2 screen_start = worldToScreen(selection_start);
        ImVec2 screen_end = worldToScreen(selection_end);

        ImVec2 min_pos = ImVec2(MIN(screen_start.x, screen_end.x),
            MIN(screen_start.y, screen_end.y));
        ImVec2 max_pos = ImVec2(MAX(screen_start.x, screen_end.x),
            MAX(screen_start.y, screen_end.y));

        draw_list->AddRect(min_pos, max_pos, IM_COL32(100, 150, 255, 50), 0.0f, 0, 2.0f);
        draw_list->AddRectFilled(min_pos, max_pos, IM_COL32(100, 150, 255, 50));
    }
    ImGui::End();
}

void BlueprintManager::selectNodesInBox(ImVec2 min_pos, ImVec2 max_pos) {
    for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        BasicNode* node = it->second.get();

        bool intersects = !(node->position.x > max_pos.x ||
            node->position.x + node->size.x < min_pos.x ||
            node->position.y > max_pos.y ||
            node->position.y + node->size.y < min_pos.y);

        if (intersects) {
            selectNode(it->first, true);
        }
    }
}

void BlueprintManager::clearSelection() {
    selected_nodes.clear();
}

void BlueprintManager::selectNode(const std::string& guid, bool add_to_selection) {
    if (!add_to_selection) {
        clearSelection();
    }
    selected_nodes.insert(guid);
}

void BlueprintManager::deselectNode(const std::string& guid) {
    selected_nodes.erase(guid);
}

bool BlueprintManager::isNodeSelected(const std::string& guid) {
    return selected_nodes.find(guid) != selected_nodes.end();
}

void BlueprintManager::handleKeyboardInput() {
    ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsPopupOpen("ContextMenu")) return;
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_C))) {
        if (!selected_nodes.empty()) {
            copySelectedNodes();
        }
    }

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_V))) {
        ImVec2 mouse_pos = ImGui::GetMousePos();
        ImVec2 world_pos = screenToWorld(mouse_pos);
        pasteNodes(world_pos);
    }

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A))) {
        clearSelection();
        for (auto it = nodes.begin(); it != nodes.end(); ++it) {
            selectNode(it->first, true);
        }
    }

    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Backspace))) {
        if (!selected_nodes.empty()) {
            deleteSelectedNodes();
        }
    }
}

void BlueprintManager::copySelectedNodes() {
    if (selected_nodes.empty()) return;

    nlohmann::json clipboard_data;
    clipboard_data["nodes"] = nlohmann::json::array();
    clipboard_data["connections"] = nlohmann::json::array();

    for (const std::string& guid : selected_nodes) {
        BasicNode* node = getNodeByGuid(guid);
        if (!node) continue;

        nlohmann::json node_json;
        node_json["guid"] = node->node_guid;
        node_json["type"] = node->getNodeType();
        node_json["name"] = node->name;
        node_json["position"]["x"] = node->position.x;
        node_json["position"]["y"] = node->position.y;
        node_json["size"]["x"] = node->size.x;
        node_json["size"]["y"] = node->size.y;
        node_json["data"] = node->serializeData();

        node_json["inputs"] = nlohmann::json::array();
        for (auto& pin : node->inputs) {
            nlohmann::json pin_json;
            pin_json["guid"] = pin->pin_guid;
            pin_json["type"] = static_cast<int>(pin->type);
            pin_json["name"] = pin->name;
            node_json["inputs"].push_back(pin_json);
        }

        node_json["outputs"] = nlohmann::json::array();
        for (auto& pin : node->outputs) {
            nlohmann::json pin_json;
            pin_json["guid"] = pin->pin_guid;
            pin_json["type"] = static_cast<int>(pin->type);
            pin_json["name"] = pin->name;
            node_json["outputs"].push_back(pin_json);
        }

        clipboard_data["nodes"].push_back(node_json);
    }

    for (const auto& conn : connections) {
        bool from_selected = false;
        bool to_selected = false;

        for (const std::string& guid : selected_nodes) {
            BasicNode* node = getNodeByGuid(guid);
            if (!node) continue;

            for (auto& pin : node->outputs) {
                if (pin.get() == conn.from_pin) {
                    from_selected = true;
                    break;
                }
            }

            for (auto& pin : node->inputs) {
                if (pin.get() == conn.to_pin) {
                    to_selected = true;
                    break;
                }
            }
        }

        if (from_selected && to_selected) {
            nlohmann::json conn_json;
            conn_json["from_pin"] = conn.from_pin_guid;
            conn_json["to_pin"] = conn.to_pin_guid;
            clipboard_data["connections"].push_back(conn_json);
        }
    }

    std::string json_string = clipboard_data.dump(4);     

    if (setClipboardText(json_string)) {
        std::cout << "Copied " << selected_nodes.size() << " nodes to system clipboard" << std::endl;

    }
    else {
        std::cout << "Failed to copy to system clipboard" << std::endl;
    }
}

void BlueprintManager::pasteNodes(ImVec2 paste_position) {
    std::string json_string = getClipboardText();

    if (json_string.empty()) {
        std::cout << "Clipboard is empty" << std::endl;
        return;
    }

    nlohmann::json clipboard_data;
    try {
        clipboard_data = nlohmann::json::parse(json_string);
    }
    catch (const std::exception& e) {
        std::cout << "Failed to parse clipboard data: " << e.what() << std::endl;
        return;
    }

    if (!clipboard_data.contains("nodes") || !clipboard_data.contains("connections")) {
        std::cout << "Invalid clipboard data format" << std::endl;
        return;
    }

    std::map<std::string, std::string> guid_mapping;
    std::vector<std::string> new_node_guids;

    ImVec2 offset = paste_position;
    if (!clipboard_data["nodes"].empty()) {
        float min_x = clipboard_data["nodes"][0]["position"]["x"];
        float min_y = clipboard_data["nodes"][0]["position"]["y"];

        for (const auto& node_json : clipboard_data["nodes"]) {
            float pos_x = static_cast<float>(node_json["position"]["x"]);
            float pos_y = static_cast<float>(node_json["position"]["y"]);
            min_x = MIN(min_x, pos_x);
            min_y = MIN(min_y, pos_y);
        }

        offset = ImVec2(paste_position.x - min_x, paste_position.y - min_y);
    }

    for (const auto& node_json : clipboard_data["nodes"]) {
        std::string old_guid = node_json["guid"];
        std::string new_guid = GUIDGenerator::generate();
        std::string type = node_json["type"];

        std::unique_ptr<BasicNode> node = nullptr;

        
        if (type == "BybiyMarginLongNode") {
            node = std::make_unique<BybiyMarginLongNode>(new_guid, this);
        }
        else if (type == "BybiyMarginShortNode") {
            node = std::make_unique<BybiyMarginShortNode>(new_guid, this);
        }
        else if (type == "BybiyMarginCloseLongPositionNode") {
            node = std::make_unique<BybiyMarginCloseLongPositionNode>(new_guid, this);
        }
        else if (type == "BybiyMarginCloseShortPositionNode") {
            node = std::make_unique<BybiyMarginCloseShortPositionNode>(new_guid, this);
        }

        else if (type == "AddMarkNode") {
            node = std::make_unique<AddMarkNode>(new_guid, this);
        }
        else if (type == "AddLineNode") {
            node = std::make_unique<AddLineNode>(new_guid, this);
        }
        else if (type == "GetCurrentCandleDataNode") {
        node = std::make_unique<GetCurrentCandleDataNode>(new_guid, this);
        }
        else if (type == "GetCandleDataAtIndexNode") {
        node = std::make_unique<GetCandleDataAtIndexNode>(new_guid, this);
        }
        else if (type == "GetCurrentCandleIndexNode") {
        node = std::make_unique<GetCurrentCandleIndexNode>(new_guid, this);
        }

        else if (type == "EngulfingPatternNode") {
            node = std::make_unique<EngulfingPatternNode>(new_guid, this);
        }
        else if (type == "CenterOfGravityNode") {
            node = std::make_unique<CenterOfGravityNode>(new_guid, this);
        }
        else if (type == "HammerPatternNode") {
            node = std::make_unique<HammerPatternNode>(new_guid, this);
        }
        else if (type == "DojiPatternNode") {
            node = std::make_unique<DojiPatternNode>(new_guid, this);
        }
        else if (type == "LowestNode") {
            node = std::make_unique<LowestNode>(new_guid, this);
        }
        else if (type == "HighestNode") {
            node = std::make_unique<HighestNode>(new_guid, this);
        }
        else if (type == "VWAPNode") {
            node = std::make_unique<VWAPNode>(new_guid, this);
        }
        else if (type == "MFINode") {
            node = std::make_unique<MFINode>(new_guid, this);
        }
        else if (type == "WilliamsRNode") {
            node = std::make_unique<WilliamsRNode>(new_guid, this);
        }
        else if (type == "CCINode") {
            node = std::make_unique<CCINode>(new_guid, this);
        }
        else if (type == "ATRNode") {
            node = std::make_unique<ATRNode>(new_guid, this);
        }
        else if (type == "StochasticNode") {
            node = std::make_unique<StochasticNode>(new_guid, this);
        }
        else if (type == "BollingerBandsNode") {
            node = std::make_unique<BollingerBandsNode>(new_guid, this);
        }
        else if (type == "MACDNode") {
            node = std::make_unique<MACDNode>(new_guid, this);
        }
        else if (type == "RSINode") {
            node = std::make_unique<RSINode>(new_guid, this);
        }
        else if (type == "EMANode") {
            node = std::make_unique<EMANode>(new_guid, this);
        }
        else if (type == "SMANode") {
            node = std::make_unique<SMANode>(new_guid, this);
        }


        else if (type == "IntNotEqualNode") {
            node = std::make_unique<IntNotEqualNode>(new_guid, this);
        }
        else if (type == "IntEqualNode") {
            node = std::make_unique<IntEqualNode>(new_guid, this);
        }
        else if (type == "IntLessThanNode") {
            node = std::make_unique<IntLessThanNode>(new_guid, this);
        }
        else if (type == "IntMoreThanNode") {
            node = std::make_unique<IntMoreThanNode>(new_guid, this);
        }
        else if (type == "DoubleNotEqualNode") {
            node = std::make_unique<DoubleNotEqualNode>(new_guid, this);
        }
        else if (type == "DoubleEqualNode") {
            node = std::make_unique<DoubleEqualNode>(new_guid, this);
        }
        else if (type == "DoubleLessThanNode") {
            node = std::make_unique<DoubleLessThanNode>(new_guid, this);
        }
        else if (type == "DoubleMoreThanNode") {
            node = std::make_unique<DoubleMoreThanNode>(new_guid, this);
        }
        else if (type == "FloatNotEqualNode") {
            node = std::make_unique<FloatNotEqualNode>(new_guid, this);
        }
        else if (type == "FloatEqualNode") {
            node = std::make_unique<FloatEqualNode>(new_guid, this);
        }
        else if (type == "FloatLessThanNode") {
            node = std::make_unique<FloatLessThanNode>(new_guid, this);
        }
        else if (type == "FloatMoreThanNode") {
            node = std::make_unique<FloatMoreThanNode>(new_guid, this);
        }
        else if (type == "BoolAndNode") {
            node = std::make_unique<BoolAndNode>(new_guid, this);
        }
        else if (type == "BoolOrNode") {
            node = std::make_unique<BoolOrNode>(new_guid, this);
        }
        else if (type == "BoolEqualNode") {
            node = std::make_unique<BoolEqualNode>(new_guid, this);
        }
        else if (type == "BoolNotEqualNode") {
            node = std::make_unique<BoolNotEqualNode>(new_guid, this);
        }

        else if (type == "IntToFloatNode") {
            node = std::make_unique<IntToFloatNode>(new_guid, this);
        }
        else if (type == "IntToDoubleNode") {
            node = std::make_unique<IntToDoubleNode>(new_guid, this);
        }
        else if (type == "IntToTextNode") {
            node = std::make_unique<IntToTextNode>(new_guid, this);
        }
        else if (type == "DoubleToFloatNode") {
            node = std::make_unique<DoubleToFloatNode>(new_guid, this);
        }
        else if (type == "DoubleToIntNode") {
            node = std::make_unique<DoubleToIntNode>(new_guid, this);
        }
        else if (type == "DoubleToTextNode") {
            node = std::make_unique<DoubleToTextNode>(new_guid, this);
        }
        else if (type == "FloatToDoubleNode") {
            node = std::make_unique<FloatToDoubleNode>(new_guid, this);
        }
        else if (type == "FloatToIntNode") {
            node = std::make_unique<FloatToIntNode>(new_guid, this);
        }
        else if (type == "FloatToTextNode") {
            node = std::make_unique<FloatToTextNode>(new_guid, this);
        }
        else if (type == "BoolToTextNode") {
            node = std::make_unique<BoolToTextNode>(new_guid, this);
        }

        else if (type == "ForLoopNode") {
            node = std::make_unique<ForLoopNode>(new_guid, this);
        }

        else if (type == "ConstIntNode") {
            node = std::make_unique<ConstIntNode>(new_guid, this);
        }
        else if (type == "ConstFloatNode") {
            node = std::make_unique<ConstFloatNode>(new_guid, this);
        }
        else if (type == "ConstDoubleNode") {
            node = std::make_unique<ConstDoubleNode>(new_guid, this);
        }
        else if (type == "ConstBoolNode") {
            node = std::make_unique<ConstBoolNode>(new_guid, this);
        }
        else if (type == "ConstTextNode") {
            node = std::make_unique<ConstTextNode>(new_guid, this);
        }

        else if (type == "IntMinusIntNode") {
            node = std::make_unique<IntMinusIntNode>(new_guid, this);
        }
        else if (type == "IntMinusFloatNode") {
            node = std::make_unique<IntMinusFloatNode>(new_guid, this);
        }
        else if (type == "IntMinusDoubleNode") {
            node = std::make_unique<IntMinusDoubleNode>(new_guid, this);
        }
        else if (type == "IntPlusIntNode") {
            node = std::make_unique<IntPlusIntNode>(new_guid, this);
        }
        else if (type == "IntPlusFloatNode") {
            node = std::make_unique<IntPlusFloatNode>(new_guid, this);
        }
        else if (type == "IntPlusDoubleNode") {
            node = std::make_unique<IntPlusDoubleNode>(new_guid, this);
        }
        else if (type == "IntMultiplyIntNode") {
            node = std::make_unique<IntMultiplyIntNode>(new_guid, this);
        }
        else if (type == "IntMultiplyFloatNode") {
            node = std::make_unique<IntMultiplyFloatNode>(new_guid, this);
        }
        else if (type == "IntMultiplyDoubleNode") {
            node = std::make_unique<IntMultiplyDoubleNode>(new_guid, this);
        }
        else if (type == "IntDivideIntNode") {
            node = std::make_unique<IntDivideIntNode>(new_guid, this);
        }
        else if (type == "IntDivideFloatNode") {
            node = std::make_unique<IntDivideFloatNode>(new_guid, this);
        }
        else if (type == "IntDivideDoubleNode") {
            node = std::make_unique<IntDivideDoubleNode>(new_guid, this);
        }

        else if (type == "FloatMinusIntNode") {
            node = std::make_unique<FloatMinusIntNode>(new_guid, this);
        }
        else if (type == "FloatMinusFloatNode") {
            node = std::make_unique<FloatMinusFloatNode>(new_guid, this);
        }
        else if (type == "FloatMinusDoubleNode") {
            node = std::make_unique<FloatMinusDoubleNode>(new_guid, this);
        }
        else if (type == "FloatPlusIntNode") {
            node = std::make_unique<FloatPlusIntNode>(new_guid, this);
        }
        else if (type == "FloatPlusFloatNode") {
            node = std::make_unique<FloatPlusFloatNode>(new_guid, this);
        }
        else if (type == "FloatPlusDoubleNode") {
            node = std::make_unique<FloatPlusDoubleNode>(new_guid, this);
        }
        else if (type == "FloatMultiplyIntNode") {
            node = std::make_unique<FloatMultiplyIntNode>(new_guid, this);
        }
        else if (type == "FloatMultiplyFloatNode") {
            node = std::make_unique<FloatMultiplyFloatNode>(new_guid, this);
        }
        else if (type == "FloatMultiplyDoubleNode") {
            node = std::make_unique<FloatMultiplyDoubleNode>(new_guid, this);
        }
        else if (type == "FloatDivideIntNode") {
            node = std::make_unique<FloatDivideIntNode>(new_guid, this);
        }
        else if (type == "FloatDivideFloatNode") {
            node = std::make_unique<FloatDivideFloatNode>(new_guid, this);
        }
        else if (type == "FloatDivideDoubleNode") {
            node = std::make_unique<FloatDivideDoubleNode>(new_guid, this);
        }

        else if (type == "DoubleMinusIntNode") {
            node = std::make_unique<DoubleMinusIntNode>(new_guid, this);
        }
        else if (type == "DoubleMinusFloatNode") {
            node = std::make_unique<DoubleMinusFloatNode>(new_guid, this);
        }
        else if (type == "DoubleMinusDoubleNode") {
            node = std::make_unique<DoubleMinusDoubleNode>(new_guid, this);
        }
        else if (type == "DoublePlusIntNode") {
            node = std::make_unique<DoublePlusIntNode>(new_guid, this);
        }
        else if (type == "DoublePlusFloatNode") {
            node = std::make_unique<DoublePlusFloatNode>(new_guid, this);
        }
        else if (type == "DoublePlusDoubleNode") {
            node = std::make_unique<DoublePlusDoubleNode>(new_guid, this);
        }
        else if (type == "DoubleMultiplyIntNode") {
            node = std::make_unique<DoubleMultiplyIntNode>(new_guid, this);
        }
        else if (type == "DoubleMultiplyFloatNode") {
            node = std::make_unique<DoubleMultiplyFloatNode>(new_guid, this);
        }
        else if (type == "DoubleMultiplyDoubleNode") {
            node = std::make_unique<DoubleMultiplyDoubleNode>(new_guid, this);
        }
        else if (type == "DoubleDivideIntNode") {
            node = std::make_unique<DoubleDivideIntNode>(new_guid, this);
        }
        else if (type == "DoubleDivideFloatNode") {
            node = std::make_unique<DoubleDivideFloatNode>(new_guid, this);
        }
        else if (type == "DoubleDivideDoubleNode") {
            node = std::make_unique<DoubleDivideDoubleNode>(new_guid, this);
        }

        else if (type == "BranchNode") {
            node = std::make_unique<BranchNode>(new_guid, this);
        }
        else if (type == "SequenceNode") {
            node = std::make_unique<SequenceNode>(new_guid, this);
        }
        else if (type == "PrintString") {
            node = std::make_unique<PrintString>(new_guid, this);
        }

        else if (type == "GetVariableNode") {
            node = std::make_unique<GetVariableNode>(new_guid, this);
        }
        else if (type == "SetVariableNode") {
            node = std::make_unique<SetVariableNode>(new_guid, this);
        }

        if (!node) continue;

        node->position.x = node_json["position"]["x"].get<float>() + offset.x;
        node->position.y = node_json["position"]["y"].get<float>() + offset.y;
        node->size.x = node_json["size"]["x"];
        node->size.y = node_json["size"]["y"];

        if (node_json.contains("inputs")) {
            for (size_t i = 0; i < node_json["inputs"].size() && i < node->inputs.size(); i++) {
                std::string old_pin_guid = node_json["inputs"][i]["guid"];
                std::string new_pin_guid = GUIDGenerator::generate();
                node->inputs[i]->pin_guid = new_pin_guid;
                guid_mapping[old_pin_guid] = new_pin_guid;
            }
        }

        if (node_json.contains("outputs")) {
            for (size_t i = 0; i < node_json["outputs"].size() && i < node->outputs.size(); i++) {
                std::string old_pin_guid = node_json["outputs"][i]["guid"];
                std::string new_pin_guid = GUIDGenerator::generate();
                node->outputs[i]->pin_guid = new_pin_guid;
                guid_mapping[old_pin_guid] = new_pin_guid;
            }
        }

        if (node_json.contains("data")) {
            node->deserializeData(node_json["data"]);
        }

        guid_mapping[old_guid] = new_guid;
        new_node_guids.push_back(new_guid);
        addNode(std::move(node));
    }

    for (const auto& conn_json : clipboard_data["connections"]) {
        std::string old_from_guid = conn_json["from_pin"];
        std::string old_to_guid = conn_json["to_pin"];

        auto from_it = guid_mapping.find(old_from_guid);
        auto to_it = guid_mapping.find(old_to_guid);

        if (from_it != guid_mapping.end() && to_it != guid_mapping.end()) {
            Pin* from_pin = getPinByGuid(from_it->second);
            Pin* to_pin = getPinByGuid(to_it->second);

            PinOut* from_pin_out = dynamic_cast<PinOut*>(from_pin);
            PinIn* to_pin_in = dynamic_cast<PinIn*>(to_pin);

            if (from_pin_out && to_pin_in) {
                createConnection(from_pin_out, to_pin_in);
            }
        }
    }

    clearSelection();
    for (const std::string& guid : new_node_guids) {
        selectNode(guid, true);
    }

    std::cout << "Pasted " << new_node_guids.size() << " nodes" << std::endl;
}

void BlueprintManager::deleteSelectedNodes() {
    if (selected_nodes.empty()) return;

    for (auto it = connections.begin(); it != connections.end();) {
        bool should_remove = false;

        for (const std::string& guid : selected_nodes) {
            BasicNode* node = getNodeByGuid(guid);
            if (!node) continue;

            for (auto& pin : node->inputs) {
                if (pin.get() == it->to_pin) {
                    should_remove = true;
                    break;
                }
            }

            for (auto& pin : node->outputs) {
                if (pin.get() == it->from_pin) {
                    should_remove = true;
                    break;
                }
            }

            if (should_remove) break;
        }

        if (should_remove) {
            if (it->from_pin && it->to_pin) {
                auto pin_it = std::find(it->from_pin->connected_pins.begin(),
                    it->from_pin->connected_pins.end(), it->to_pin);
                if (pin_it != it->from_pin->connected_pins.end()) {
                    it->from_pin->connected_pins.erase(pin_it);
                }

                it->to_pin->linked_to = nullptr;
                it->to_pin->linked_to_guid = "";
            }

            it = connections.erase(it);
        }
        else {
            ++it;
        }
    }

    for (const std::string& guid : selected_nodes) {
        nodes.erase(guid);
    }

    std::cout << "Deleted " << selected_nodes.size() << " nodes" << std::endl;
    clearSelection();
}

void BlueprintManager::restoreConnections(const nlohmann::json& connections_json) {
    for (const auto& conn_json : connections_json) {
        std::string from_pin_guid = conn_json["from_pin"];
        std::string to_pin_guid = conn_json["to_pin"];

        Pin* from_pin_base = getPinByGuid(from_pin_guid);
        Pin* to_pin_base = getPinByGuid(to_pin_guid);

        if (!from_pin_base || !to_pin_base) {
            continue;
        }

        PinOut* from_pin = dynamic_cast<PinOut*>(from_pin_base);
        PinIn* to_pin = dynamic_cast<PinIn*>(to_pin_base);

        if (from_pin && to_pin) {
            createConnection(from_pin, to_pin);
        }
        else {
        }
    }
}

void BlueprintManager::clearAllBeforeLoad() {
    nodes.clear();
    connections.clear();
    creating_connection = false;
    connection_start_pin = nullptr;
    dragging_node = false;
    selected_node_guid = "";
    clearSelection();
    has_clipboard_data = false;
    clipboard_data.clear();
}

void BlueprintManager::clearAll() {
    for (auto it = nodes.begin(); it != nodes.end(); )
    {
        if (dynamic_cast<EntryNode*>(it->second.get()) == nullptr)
        {
            it = nodes.erase(it);     
        }
        else
        {
            ++it;
        }
    }
    connections.clear();
    creating_connection = false;
    connection_start_pin = nullptr;
    dragging_node = false;
    selected_node_guid = "";
    clearSelection();
    has_clipboard_data = false;
    clipboard_data.clear();
}