#pragma once
#include "../../Public/TabManager/TabContent/TabContent.h"
#include <vector>
#include <iostream>
// Главный класс приложения
class TradingApplication {
private:

    std::vector<std::unique_ptr<TabContent>> tabs;
    int active_tab_index;
    int next_tab_id;

public:
    TradingApplication();

    template<typename T>
    void AddTab(const std::string& name);

    void SetActiveTab(int index);

    void CloseTab(int index);

    void Render();
};

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}