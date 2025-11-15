#include "../../../Public/Blueprints/Node/Node.h"
#include "../../../Public/Blueprints/BlueprintManager.h"

void BasicNode::drawNodeToDrawList(ImDrawList* draw_list) {
    ImU32 bg_color = IM_COL32(25, 25, 25, 255);
    draw_list->AddRectFilled(position,
        ImVec2(position.x + size.x, position.y + size.y),
        bg_color, 8.0f);

    ImU32 header_color = getHeaderColorValue(getHeaderColor());

    float header_height = 25.0f;
    draw_list->AddRectFilled(position,
        ImVec2(position.x + size.x, position.y + header_height),
        header_color, 8.0f, ImDrawFlags_RoundCornersTop);

    draw_list->AddRect(position,
        ImVec2(position.x + size.x, position.y + size.y),
        IM_COL32(0, 0, 0, 255), 8.0f, 0, 1.5f);

    ImVec2 title_pos = ImVec2(position.x + 8, position.y + 4);
    draw_list->AddText(title_pos, IM_COL32(255, 255, 255, 255), name.c_str());

    drawPinsToDrawList(draw_list);
}

void ConstValueNode::drawNodeToDrawList(ImDrawList* draw_list) {

    ImU32 bg_color = IM_COL32(25, 25, 25, 255);
    draw_list->AddRectFilled(position,
        ImVec2(position.x + size.x, position.y + size.y),
        bg_color, 8.0f);

    ImU32 header_color = getHeaderColorValue(getHeaderColor());

    float header_height = 25.0f;
    draw_list->AddRectFilled(position,
        ImVec2(position.x + size.x, position.y + header_height),
        header_color, 8.0f, ImDrawFlags_RoundCornersTop);

    draw_list->AddRect(position,
        ImVec2(position.x + size.x, position.y + size.y),
        IM_COL32(0, 0, 0, 255), 8.0f, 0, 1.5f);

    ImVec2 title_pos = ImVec2(position.x + 8, position.y + 4);
    draw_list->AddText(title_pos, IM_COL32(255, 255, 255, 255), name.c_str());

    std::string valueText = "";
    ImU32 valueColor = WhiteColor;    

    switch (type) {
    case PinType::INT:
        valueText = std::to_string(intValue);
        valueColor = WhiteColor;  
        break;

    case PinType::FLOAT:
        valueText = std::to_string(floatValue);
        valueText = valueText.substr(0, valueText.find_last_not_of('0') + 1);
        if (valueText.back() == '.') valueText.pop_back();
        valueColor = WhiteColor;  
        break;

    case PinType::DOUBLE: {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.6Lf", doubleValue);
        valueText = std::string(buffer);
        valueText = valueText.substr(0, valueText.find_last_not_of('0') + 1);
        if (valueText.back() == '.') valueText.pop_back();
        valueColor = WhiteColor;  
        break;
    }

    case PinType::BOOL:
        valueText = boolValue ? "true" : "false";
        valueColor = boolValue ? IM_COL32(100, 255, 100, 255) : IM_COL32(255, 100, 100, 255);  
        break;

    case PinType::STRING:
        valueText = "\"" + stringValue + "\"";
        valueColor = WhiteColor;
        if (valueText.length() > 15) {
            valueText = valueText.substr(0, 12) + "...\"";
        }
        break;

    default:
        valueText = "?";
        valueColor = WhiteColor;  
        break;
    }

    ImVec2 text_size = ImGui::CalcTextSize(valueText.c_str());
    ImVec2 value_pos = ImVec2(
        position.x + (size.x - text_size.x) * 0.5f,    
        position.y + header_height + (size.y - header_height - text_size.y) * 0.5f    
    );

    draw_list->AddText(value_pos, valueColor, valueText.c_str());

    drawPinsToDrawList(draw_list);
}

ImU32 BasicNode::getHeaderColorValue(NodeHeaderColor color) const {
    switch (color) {
    case NodeHeaderColor::DEFAULT:
        return IM_COL32(45, 45, 45, 255);           
    case NodeHeaderColor::EXECUTION:
        return IM_COL32(144, 144, 144, 255);         
    case NodeHeaderColor::MATH:
        return IM_COL32(58, 110, 45, 255);           
    case NodeHeaderColor::LOGIC:
        return IM_COL32(98, 35, 35, 255);            
    case NodeHeaderColor::CONVERSION:
        return IM_COL32(35, 65, 98, 255);           
    case NodeHeaderColor::CONSTANT:
        return IM_COL32(98, 98, 35, 255);            
    case NodeHeaderColor::VARIABLE:
        return IM_COL32(75, 35, 98, 255);           
    case NodeHeaderColor::CHART:
        return IM_COL32(145, 85, 25, 255);          
    case NodeHeaderColor::ARRAY:
        return IM_COL32(35, 98, 98, 255);           
    case NodeHeaderColor::FLOW:
        return IM_COL32(98, 35, 75, 255);           
    default:
        return IM_COL32(45, 45, 45, 255);          
    }
}

void BasicNode::drawPinsToDrawList(ImDrawList* draw_list) {
    for (int i = 0; i < inputs.size(); i++) {
        ImVec2 pin_pos = getInputPinPos(i);
        inputs[i]->position = pin_pos;

        ImU32 color = WhiteColor;
        switch (inputs[i]->type) {
        case PinType::EXEC: color = WhiteColor; break;
        case PinType::BOOL: color = RedColor; break;
        case PinType::FLOAT: color = GreenColor; break;
        case PinType::DOUBLE: color = CyanColor; break;
        case PinType::STRING: color = PinkColor; break;
        case PinType::INT: color = ForestGreenColor; break;
        case PinType::ARRAY_BOOL: color = RedColor; break;
        case PinType::ARRAY_FLOAT: color = GreenColor; break;
        case PinType::ARRAY_DOUBLE: color = CyanColor; break;
        case PinType::ARRAY_STRING: color = PinkColor; break;
        case PinType::ARRAY_INT: color = ForestGreenColor; break;
        default: color = WhiteColor; break;
        }

        draw_list->AddCircleFilled(pin_pos, 5, color);

        if (!inputs[i]->name.empty()) {
            ImVec2 text_pos = ImVec2(pin_pos.x + 8, pin_pos.y - 7);
            draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), inputs[i]->name.c_str());
        }
    }

    for (int i = 0; i < outputs.size(); i++) {
        ImVec2 pin_pos = getOutputPinPos(i);
        outputs[i]->position = pin_pos;

        ImU32 color = WhiteColor;
        switch (outputs[i]->type) {
        case PinType::EXEC: color = WhiteColor; break;
        case PinType::BOOL: color = RedColor; break;
        case PinType::FLOAT: color = GreenColor; break;
        case PinType::DOUBLE: color = CyanColor; break;
        case PinType::STRING: color = PinkColor; break;
        case PinType::INT: color = ForestGreenColor; break;
        case PinType::ARRAY_BOOL: color = RedColor; break;
        case PinType::ARRAY_FLOAT: color = GreenColor; break;
        case PinType::ARRAY_DOUBLE: color = CyanColor; break;
        case PinType::ARRAY_STRING: color = PinkColor; break;
        case PinType::ARRAY_INT: color = ForestGreenColor; break;
        default: color = WhiteColor; break;
        }

        draw_list->AddCircleFilled(pin_pos, 6, color);

        if (!outputs[i]->name.empty()) {
            ImVec2 text_size = ImGui::CalcTextSize(outputs[i]->name.c_str());
            ImVec2 text_pos = ImVec2(pin_pos.x - text_size.x - 8, pin_pos.y - 7);
            draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), outputs[i]->name.c_str());
        }
    }
}
void BasicNode::BeginNodeDraw(const std::string& title, ImU32 bg_color) {
    ImGui::SetNextWindowPos(position);
    ImGui::SetNextWindowSize(size);


    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImColor(bg_color).Value);

    std::string window_name = title + "##" + node_guid;
    ImGui::Begin(window_name.c_str(), nullptr,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
}

void BasicNode::EndNodeDraw(std::map<int, bool> not_show_inpin_names_map, std::map<int, bool> not_show_outpin_names_map) {
    std::vector<bool> not_show_inpin_names(inputs.size(), false);
    std::vector<bool> not_show_outpin_names(outputs.size(), false);

    for (const auto& pair : not_show_inpin_names_map) {
        if (pair.first < inputs.size()) {
            not_show_inpin_names[pair.first] = pair.second;
        }
    }

    for (const auto& pair : not_show_outpin_names_map) {
        if (pair.first < outputs.size()) {
            not_show_outpin_names[pair.first] = pair.second;
        }
    }

    drawPins(not_show_inpin_names, not_show_outpin_names);
    ImGui::End();
    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(1);
}

void BasicNode::drawPins(std::vector<bool> not_show_inpin_names, std::vector<bool> not_show_outpin_names) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    for (int i = 0; i < inputs.size(); i++) {
        ImVec2 pin_pos = getInputPinPos(i);
        inputs[i]->position = pin_pos;

        ImU32 color = WhiteColor;
        switch (inputs[i]->type) {
        case PinType::EXEC: color = WhiteColor; break;
        case PinType::BOOL: color = RedColor; break;
        case PinType::FLOAT: color = GreenColor; break;
        case PinType::DOUBLE: color = CyanColor; break;
        case PinType::STRING: color = PinkColor; break;
        case PinType::INT: color = ForestGreenColor; break;
        default: color = WhiteColor; break;
        }

        draw_list->AddCircleFilled(pin_pos, 5, color);

        if(!not_show_inpin_names[i])
        if (!inputs[i]->name.empty()) {
            ImVec2 text_pos = ImVec2(pin_pos.x + 8, pin_pos.y - 7);     
            draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), inputs[i]->name.c_str());
        }
    }

    for (int i = 0; i < outputs.size(); i++) {
        ImVec2 pin_pos = getOutputPinPos(i);
        outputs[i]->position = pin_pos;

        ImU32 color = WhiteColor;
        switch (outputs[i]->type) {
        case PinType::EXEC: color = WhiteColor; break;
        case PinType::BOOL: color = RedColor; break;
        case PinType::FLOAT: color = GreenColor; break;
        case PinType::DOUBLE: color = CyanColor; break;
        case PinType::STRING: color = PinkColor; break;
        case PinType::INT: color = ForestGreenColor; break;
        default: color = WhiteColor; break;
        }

        draw_list->AddCircleFilled(pin_pos, 6, color);

        if (!not_show_outpin_names[i])
        if (!outputs[i]->name.empty()) {
            ImVec2 text_size = ImGui::CalcTextSize(outputs[i]->name.c_str());
            ImVec2 text_pos = ImVec2(pin_pos.x - text_size.x - 8, pin_pos.y - 7);     
            draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), outputs[i]->name.c_str());
        }
    }
}

