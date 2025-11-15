#pragma once
#include "../../../Public/TabManager/TabContent/TabContent.h"
#include <vector>
#include "../../Chart/CandleChart.h"
#include "../../Blueprints/BlueprintManager.h"
#include "BottomPanel/MainMenuBottomPanel.h"
#include "../../../Public/StatsManager/TradingStatsManager.h"
#include "../../Exchanges/Bybit/BybitPriceCollector.h"

enum CEX
{
    Bybit,
    Binance
};

class MainChartTab : public TabContent {
private:
    CEX currentExchange = CEX::Bybit;
    CandlestickDataManager dataManager;
    BlueprintManager blueprintEditor;
    TradingStatsManager backtestStatsManager;
    TradingStatsManager runtimeStatsManager;
    CandlestickChart chart = CandlestickChart(&dataManager);
    BottomPanel bottomPanel;    
    char text_buffer[4096];
    std::vector<std::string> log_messages;

    BybitPriceCollector bybit_priceCollector;
    char tokenInput[128] = "";
    int selectedBlockchain = 0;
    int selectedTimeframe = 0;
    std::string statusMessage;

    bool auto_scroll;
    void generateTestData(int count = 50);
    void addRandomCandle();
public:
    MainChartTab(const std::string& name, int id);
    virtual void Render();
    virtual void AddLogMessage(const std::string& message);
    virtual void LightUpdate();
};