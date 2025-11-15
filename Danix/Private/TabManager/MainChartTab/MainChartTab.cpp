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

#include "../../../Public/TabManager/MainChartTab/MainChartTab.h"
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
float clampfff(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}



MainChartTab::MainChartTab(const std::string& name, int id) 
    : TabContent(name, id), bottomPanel(&dataManager), bybit_priceCollector(){    
    dataManager.SetBlueprintManager(&blueprintEditor);
    dataManager.SetBacktestTradingStats(&backtestStatsManager);
    dataManager.SetRuntimeTradingStats(&runtimeStatsManager);
    dataManager.GetBlueprintManager()->SetDataManager(&dataManager);
    bybit_priceCollector.setDataManager(&dataManager);
    memset(text_buffer, 0, sizeof(text_buffer));
    strcpy_s(text_buffer, "// Trading strategy code\nfloat price = GetPrice();\nif (price > 50000) {\n    Buy(0.1);\n}");
    log_messages.push_back("Editor initialized");
    log_messages.push_back("Ready for input");
    log_messages.push_back("Strategy control panel loaded");

    bybit_priceCollector.setOnNewCandleCallback([&](const BybitCandle& candle) {
        MarketData data;
        data.index = candle.index;
        data.timestamp = candle.timestamp;
        data.open = candle.open;
        data.high = candle.high;
        data.low = candle.low;
        data.close = candle.close;
        data.volume = candle.volume;

        dataManager.addCandle(data);
        });

    bybit_priceCollector.setOnCandleUpdateCallback([&](const BybitCandle& candle) {
        MarketData data;
        data.index = candle.index;
        data.timestamp = candle.timestamp;
        data.open = candle.open;
        data.high = candle.high;
        data.low = candle.low;
        data.close = candle.close;
        data.volume = candle.volume;

        dataManager.UpdateLastCandle(data);
        });

}

void MainChartTab::generateTestData(int count)
{
    std::vector<MarketData> testData;
    double currentPrice = 100.0;
    double currentTime = 1000000000.0;
    for (int i = 0; i < count; ++i) {
        MarketData candle;
        candle.timestamp = currentTime + i * 3600; 
        candle.open = currentPrice;
        double change = ((rand() % 200 - 100) / 100.0) * 2.0;
        candle.close = currentPrice + change;
        candle.high = MAX(candle.open, candle.close) + (rand() % 100) / 100.0;
        candle.low = MIN(candle.open, candle.close) - (rand() % 100) / 100.0;
        candle.volume = 1000 + rand() % 5000;
        candle.mcap = candle.close * 1000000;
        testData.push_back(candle);
        currentPrice = candle.close;
    }
    dataManager.addData(testData);
}

void MainChartTab::addRandomCandle()
{
    if (dataManager.empty()) {
        generateTestData(1);
        return;
    }
    const auto& lastCandle = dataManager.getData().back();
    MarketData newCandle;
    newCandle.timestamp = lastCandle.timestamp + 3600;
    newCandle.open = lastCandle.close;
    double change = ((rand() % 200 - 100) / 100.0) * 2.0;
    newCandle.close = newCandle.open + change;
    newCandle.high = MAX(newCandle.open, newCandle.close) + (rand() % 100) / 100.0;
    newCandle.low = MIN(newCandle.open, newCandle.close) - (rand() % 100) / 100.0;
    newCandle.volume = 1000 + rand() % 5000;
    newCandle.mcap = newCandle.close * 1000000;
    dataManager.addCandle(newCandle);
}

void MainChartTab::Render() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.Colors[ImGuiCol_Text] = ImGui::ColorConvertU32ToFloat4(IM_COL32(255, 255, 255, 255));
    style.Colors[ImGuiCol_TextDisabled] = ImGui::ColorConvertU32ToFloat4(IM_COL32(128, 128, 128, 255));
    style.Colors[ImGuiCol_WindowBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 255));
    style.Colors[ImGuiCol_ChildBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(20, 20, 20, 255));
    style.Colors[ImGuiCol_PopupBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 255));
    style.Colors[ImGuiCol_Border] = ImGui::ColorConvertU32ToFloat4(IM_COL32(102, 102, 102, 255));
    style.Colors[ImGuiCol_BorderShadow] = ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 255));
    style.Colors[ImGuiCol_FrameBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(40, 40, 40, 255));
    style.Colors[ImGuiCol_FrameBgHovered] = ImGui::ColorConvertU32ToFloat4(IM_COL32(60, 60, 60, 255));
    style.Colors[ImGuiCol_FrameBgActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(80, 80, 80, 255));

    style.Colors[ImGuiCol_TitleBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(102, 102, 102, 255));
    style.Colors[ImGuiCol_TitleBgActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(160, 160, 160, 255));
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImGui::ColorConvertU32ToFloat4(IM_COL32(76, 76, 76, 255));

    style.Colors[ImGuiCol_MenuBarBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(40, 40, 40, 255));

    style.Colors[ImGuiCol_ScrollbarBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(20, 20, 20, 255));
    style.Colors[ImGuiCol_ScrollbarGrab] = ImGui::ColorConvertU32ToFloat4(IM_COL32(76, 76, 76, 255));
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImGui::ColorConvertU32ToFloat4(IM_COL32(102, 102, 102, 255));
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(140, 140, 140, 255));

    style.Colors[ImGuiCol_CheckMark] = ImGui::ColorConvertU32ToFloat4(IM_COL32(255, 255, 255, 255));

    style.Colors[ImGuiCol_SliderGrab] = ImGui::ColorConvertU32ToFloat4(IM_COL32(102, 102, 102, 255));
    style.Colors[ImGuiCol_SliderGrabActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(140, 140, 140, 255));

    style.Colors[ImGuiCol_Button] = ImGui::ColorConvertU32ToFloat4(IM_COL32(76, 76, 76, 255));
    style.Colors[ImGuiCol_ButtonHovered] = ImGui::ColorConvertU32ToFloat4(IM_COL32(140, 140, 140, 255));
    style.Colors[ImGuiCol_ButtonActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(76, 76, 76, 255));

    style.Colors[ImGuiCol_Header] = ImGui::ColorConvertU32ToFloat4(IM_COL32(102, 102, 102, 255));
    style.Colors[ImGuiCol_HeaderHovered] = ImGui::ColorConvertU32ToFloat4(IM_COL32(140, 140, 140, 255));
    style.Colors[ImGuiCol_HeaderActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(102, 102, 102, 255));

    style.Colors[ImGuiCol_Separator] = ImGui::ColorConvertU32ToFloat4(IM_COL32(102, 102, 102, 255));
    style.Colors[ImGuiCol_SeparatorHovered] = ImGui::ColorConvertU32ToFloat4(IM_COL32(140, 140, 140, 255));
    style.Colors[ImGuiCol_SeparatorActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(160, 160, 160, 255));

    style.Colors[ImGuiCol_ResizeGrip] = ImGui::ColorConvertU32ToFloat4(IM_COL32(76, 76, 76, 255));
    style.Colors[ImGuiCol_ResizeGripHovered] = ImGui::ColorConvertU32ToFloat4(IM_COL32(102, 102, 102, 255));
    style.Colors[ImGuiCol_ResizeGripActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(140, 140, 140, 255));

    style.Colors[ImGuiCol_Tab] = ImGui::ColorConvertU32ToFloat4(IM_COL32(60, 60, 60, 255));
    style.Colors[ImGuiCol_TabHovered] = ImGui::ColorConvertU32ToFloat4(IM_COL32(100, 100, 100, 255));
    style.Colors[ImGuiCol_TabActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(120, 120, 120, 255));
    style.Colors[ImGuiCol_TabUnfocused] = ImGui::ColorConvertU32ToFloat4(IM_COL32(40, 40, 40, 255));
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(80, 80, 80, 255));

    style.Colors[ImGuiCol_PlotLines] = ImGui::ColorConvertU32ToFloat4(IM_COL32(255, 255, 255, 255));
    style.Colors[ImGuiCol_PlotLinesHovered] = ImGui::ColorConvertU32ToFloat4(IM_COL32(255, 255, 255, 255));
    style.Colors[ImGuiCol_PlotHistogram] = ImGui::ColorConvertU32ToFloat4(IM_COL32(140, 140, 140, 255));
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImGui::ColorConvertU32ToFloat4(IM_COL32(160, 160, 160, 255));

    style.Colors[ImGuiCol_TableHeaderBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(76, 76, 76, 255));
    style.Colors[ImGuiCol_TableBorderStrong] = ImGui::ColorConvertU32ToFloat4(IM_COL32(102, 102, 102, 255));
    style.Colors[ImGuiCol_TableBorderLight] = ImGui::ColorConvertU32ToFloat4(IM_COL32(60, 60, 60, 255));
    style.Colors[ImGuiCol_TableRowBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 0));
    style.Colors[ImGuiCol_TableRowBgAlt] = ImGui::ColorConvertU32ToFloat4(IM_COL32(20, 20, 20, 255));

    style.Colors[ImGuiCol_TextSelectedBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(102, 102, 102, 255));

    style.Colors[ImGuiCol_DragDropTarget] = ImGui::ColorConvertU32ToFloat4(IM_COL32(255, 255, 255, 255));

    style.Colors[ImGuiCol_NavHighlight] = ImGui::ColorConvertU32ToFloat4(IM_COL32(140, 140, 140, 255));
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImGui::ColorConvertU32ToFloat4(IM_COL32(255, 255, 255, 255));
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 128));

    style.Colors[ImGuiCol_ModalWindowDimBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 128));

    style.FrameRounding = 8.0f;
    style.WindowRounding = 12.0f;
    style.TabRounding = 6.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 8.0f;
    style.PopupRounding = 8.0f;

    float mainWindowHeight = ImGui::GetIO().DisplaySize.y - 250 - 20;

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, mainWindowHeight), ImGuiCond_Always);

    ImGui::Begin("Trading Platform", nullptr,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    const char* blockchains[] = { "solana", "bsc"};
    const char* timeframes[] = { "1m", "3m", "5m", "15m", "30m", "1H", "2H", "4H", "6H", "8H", "12H", "1D", "3D", "1W", "1M" };

    ImGui::Text("Token:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(250);
    ImGui::InputText("##token", tokenInput, IM_ARRAYSIZE(tokenInput));

    ImGui::SameLine();
    ImGui::Text("Timeframe:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);

    if (ImGui::Combo("##timeframe", &selectedTimeframe, timeframes, IM_ARRAYSIZE(timeframes))) {
        BybitInterval interval;

        switch (selectedTimeframe) {
        case 0:  interval = BybitInterval::MIN_1; break;
        case 1:  interval = BybitInterval::MIN_3; break;
        case 2:  interval = BybitInterval::MIN_5; break;
        case 3:  interval = BybitInterval::MIN_15; break;
        case 4:  interval = BybitInterval::MIN_30; break;
        case 5:  interval = BybitInterval::HOUR_1; break;
        case 6:  interval = BybitInterval::HOUR_2; break;
        case 7:  interval = BybitInterval::HOUR_4; break;
        case 8:  interval = BybitInterval::HOUR_6; break;
        case 9:  interval = BybitInterval::HOUR_12; break;
        case 10: interval = BybitInterval::DAY_1; break;
        case 11: interval = BybitInterval::WEEK_1; break;
        case 12: interval = BybitInterval::MONTH_1; break;
        default: interval = BybitInterval::MIN_1; break;
        }

        bybit_priceCollector.setInterval(interval);
    }

    ImGui::SameLine();
    if (ImGui::Button("Load Data")) {
        if (strlen(tokenInput) > 0) {

            if (dataManager.GetTokenKey() != tokenInput) dataManager.SetTokenKey(tokenInput);
            if (bybit_priceCollector.getCurrentSymbol() != tokenInput) {
                bybit_priceCollector.setSymbol(tokenInput, BybitCategory::SPOT);
                name = tokenInput;
            }

            dataManager.clear();

            bybit_priceCollector.loadHistoricalData(10000);
            dataManager.GetChart()->resetView();
            bybit_priceCollector.startRealtimeStream();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear Data")) {
        dataManager.clear();
        bybit_priceCollector.stopRealtimeStream();
    }

    if (bybit_priceCollector.isLoading()) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Loading...");
    }
    else if (bybit_priceCollector.isStreaming()) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Live");
    }

    chart.render();

    ImGui::End();

    bottomPanel.render();
}

void MainChartTab::AddLogMessage(const std::string& message) {
    log_messages.push_back("[" + std::to_string((int)ImGui::GetTime()) + "] " + message);
    if (log_messages.size() > 50) {
        log_messages.erase(log_messages.begin());
    }
}

void MainChartTab::LightUpdate() {
}