#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <string>
#include <iostream>
#include <memory>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#include <gl/GL.h>
#pragma comment(lib, "opengl32.lib")
#endif

#ifdef __linux__
#include <GL/gl.h>
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#endif

#include "../../../Public/TabManager/EditorTab/EditorTab.h"

EditorTab::EditorTab(const std::string& name, int id) : TabContent(name, id) {
    memset(text_buffer, 0, sizeof(text_buffer));
    strcpy_s(text_buffer, "// Trading strategy code\nfloat price = GetPrice();\nif (price > 50000) {\n    Buy(0.1);\n}");

    log_messages.push_back("Editor initialized");
    log_messages.push_back("Ready for input");
}

void EditorTab::Render() {
    editor.draw();

}

void EditorTab::AddLogMessage(const std::string& message) {
    log_messages.push_back("[" + std::to_string((int)ImGui::GetTime()) + "] " + message);
    if (log_messages.size() > 50) {
        log_messages.erase(log_messages.begin());
    }
}

void EditorTab::LightUpdate() {

}