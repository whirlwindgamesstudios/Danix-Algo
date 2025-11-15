
#include "Public/TabManager/TabManager.h"

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
#include <winsock2.h>  // Включаем winsock2.h первым
#include <ws2tcpip.h>
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
#include "Public/Systems/Config/PlatformConfig.h"
#include "Public/Exchanges/Bybit/Bybitwebsocketmanager.h"
#include "Public/Exchanges/Bybit/BybitSpot.h"
#include "Public/Exchanges/Bybit/BybitMargin.h"
#include "Public/Exchanges/Bybit/LiquidationCalculator.h"
using namespace Bybit;


void printSeparator() {
    std::cout << "\n" << std::string(60, '=') << "\n\n";
}

int main() {
    std::cout << std::fixed << std::setprecision(2);

    Config::getInstance();

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;

    const char* glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    GLFWwindow* window = glfwCreateWindow(1920, 1080, "Danix - Algo Trading", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    TradingApplication app;

    ImVec4 clear_color = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);

    constexpr int TARGET_FPS = 60;
    constexpr auto FRAME_DURATION = std::chrono::microseconds(1000000 / TARGET_FPS); // 16666 микросекунд

    auto nextFrameTime = std::chrono::steady_clock::now();

    while (!glfwWindowShouldClose(window)) {
        auto currentTime = std::chrono::steady_clock::now();

        if (currentTime < nextFrameTime) {
            std::this_thread::sleep_until(nextFrameTime);
            currentTime = std::chrono::steady_clock::now();
        }

        nextFrameTime = currentTime + FRAME_DURATION;

        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        app.Render();


        ImGui::Render();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    BybitWebSocketManager::getInstance().shutdown();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}