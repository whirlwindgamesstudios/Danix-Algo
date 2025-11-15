#pragma once
#include "../../../Public/TabManager/TabContent/TabContent.h"
#include <vector>
#include "../../Blueprints/BlueprintManager.h"

class EditorTab : public TabContent {
private:
    BlueprintManager editor;

    char text_buffer[4096];
    std::vector<std::string> log_messages;
    bool auto_scroll;


public:
    EditorTab(const std::string& name, int id);

    virtual void Render();

    virtual void AddLogMessage(const std::string& message);

    virtual void LightUpdate();
};