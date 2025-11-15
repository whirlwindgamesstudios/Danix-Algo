#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <vector>
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

#include "../../Public/TabManager/TabManager.h"
#include "../../Public/TabManager/EditorTab/EditorTab.h"
#include "../../Public/TabManager/MainChartTab/MainChartTab.h"


TradingApplication::TradingApplication() : active_tab_index(-1), next_tab_id(0) {
    AddTab<MainChartTab>("BTC Chart");
}

template<typename T>
void TradingApplication::AddTab(const std::string& name) {
    auto tab = std::make_unique<T>(name, next_tab_id++);
    tabs.push_back(std::move(tab));

    if (tabs.size() == 1) {
        SetActiveTab(0);
    }
}

void TradingApplication::SetActiveTab(int index) {
    if (index >= 0 && index < tabs.size()) {
        if (active_tab_index >= 0 && active_tab_index < tabs.size()) {
            tabs[active_tab_index]->is_active = false;
            tabs[active_tab_index]->OnDeactivate();
        }

        active_tab_index = index;
        tabs[active_tab_index]->is_active = true;
        tabs[active_tab_index]->OnActivate();
    }
}

void TradingApplication::CloseTab(int index) {
    if (index >= 0 && index < tabs.size() && tabs.size() > 1) {
        tabs.erase(tabs.begin() + index);

        if (active_tab_index >= tabs.size()) {
            active_tab_index = tabs.size() - 1;
        }

        if (active_tab_index >= 0) {
            SetActiveTab(active_tab_index);
        }
    }
}

void TradingApplication::Render() {

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 255)));
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImGui::ColorConvertU32ToFloat4(IM_COL32(40, 40, 40, 255)));
    ImGui::PushStyleColor(ImGuiCol_Tab, ImGui::ColorConvertU32ToFloat4(IM_COL32(60, 60, 60, 255)));
    ImGui::PushStyleColor(ImGuiCol_TabHovered, ImGui::ColorConvertU32ToFloat4(IM_COL32(100, 100, 100, 255)));
    ImGui::PushStyleColor(ImGuiCol_TabActive, ImGui::ColorConvertU32ToFloat4(IM_COL32(120, 120, 120, 255)));
    ImGui::PushStyleColor(ImGuiCol_TabUnfocused, ImGui::ColorConvertU32ToFloat4(IM_COL32(40, 40, 40, 255)));
    ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive, ImGui::ColorConvertU32ToFloat4(IM_COL32(80, 80, 80, 255)));
    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(IM_COL32(76, 76, 76, 255)));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::ColorConvertU32ToFloat4(IM_COL32(140, 140, 140, 255)));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::ColorConvertU32ToFloat4(IM_COL32(76, 76, 76, 255)));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 255)));
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(IM_COL32(255, 255, 255, 255)));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImGui::ColorConvertU32ToFloat4(IM_COL32(102, 102, 102, 255)));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImGui::ColorConvertU32ToFloat4(IM_COL32(160, 160, 160, 255)));
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImGui::ColorConvertU32ToFloat4(IM_COL32(76, 76, 76, 255)));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::ColorConvertU32ToFloat4(IM_COL32(40, 40, 40, 255)));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImGui::ColorConvertU32ToFloat4(IM_COL32(60, 60, 60, 255)));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImGui::ColorConvertU32ToFloat4(IM_COL32(80, 80, 80, 255)));

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 6.0f);

    for (auto& tab : tabs) {
        tab->LightUpdate();
    }

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    if (ImGui::Begin("Trading Platform", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoFocusOnAppearing)) {

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Chart")) {
                    AddTab<MainChartTab>("Chart " + std::to_string(next_tab_id));
                }
                if (ImGui::MenuItem("New Editor")) {
                    AddTab<EditorTab>("Editor " + std::to_string(next_tab_id));
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Close Tab") && active_tab_index >= 0) {
                    CloseTab(active_tab_index);
                }
                ImGui::EndMenu();
            }
            ImGui::Text("| Status: Connected | Tabs: %zu", tabs.size());
            ImGui::EndMenuBar();
        }

        if (!tabs.empty()) {
            if (ImGui::BeginTabBar("MainTabs",
                ImGuiTabBarFlags_Reorderable |
                ImGuiTabBarFlags_TabListPopupButton |
                ImGuiTabBarFlags_FittingPolicyScroll)) {

                for (int i = 0; i < tabs.size(); i++) {
                    bool is_open = true;
                    if (ImGui::BeginTabItem(tabs[i]->name.c_str(), &is_open)) {
                        if (active_tab_index != i) {
                            SetActiveTab(i);
                        }
                        tabs[i]->is_visible = true;
                        tabs[i]->Render();
                        ImGui::EndTabItem();
                    }
                    else {
                        tabs[i]->is_visible = false;
                    }
                    if (!is_open) {
                        CloseTab(i);
                        break;
                    }
                }

                if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing)) {
                    ImGui::OpenPopup("AddTabMenu");
                }
                if (ImGui::BeginPopup("AddTabMenu")) {
                    if (ImGui::MenuItem("Chart")) {
                        AddTab<MainChartTab>("Chart " + std::to_string(next_tab_id));
                    }
                    if (ImGui::MenuItem("Editor")) {
                        AddTab<EditorTab>("Editor " + std::to_string(next_tab_id));
                    }
                    ImGui::EndPopup();
                }
                ImGui::EndTabBar();
            }
        }
    }
    ImGui::End();

    ImGui::PopStyleColor(18);
    ImGui::PopStyleVar(3);
}