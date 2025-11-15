#pragma once
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

class CandlestickDataManager;

class BottomPanel {
private:
    CandlestickDataManager* dataManager;
    std::vector<std::string> strategyFiles;
    std::string selectedStrategy;
    std::string loadedStrategy;    
    int selectedFileIndex;
    bool isVisible;
    int activeTab;       
    bool showStrategyPopup;

    void scanStrategyFiles();
    void renderPerformanceTab();
    void renderRuntimePerformanceTab();
    void renderSettingsTab();
    void renderStrategySelectionPopup();

    bool fileExists(const std::string& filepath);
    bool hasJsonExtension(const std::string& filename);

public:
    BottomPanel(CandlestickDataManager* dManager);
    ~BottomPanel();

    void render();
    void setVisible(bool visible) { isVisible = visible; }
    bool getVisible() const { return isVisible; }
    void refreshStrategyList();

    void setActiveTab(int tab) { activeTab = tab; }
    int getActiveTab() const { return activeTab; }

    void openStrategySelectionPopup() { showStrategyPopup = true; }
    bool hasLoadedStrategy() const { return !loadedStrategy.empty(); }
    const std::string& getLoadedStrategyName() const { return loadedStrategy; }
};