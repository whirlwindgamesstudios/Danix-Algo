
#include "../../../../Public/TabManager/MainChartTab/BottomPanel/MainMenuBottomPanel.h"
#include <imgui.h>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include "../../../../Public/Blueprints/BlueprintManager.h"
#include "../../../../Public/Chart/CandleChart.h"
#include "../../../../Public/Chart/CandleChartManager.h"

#include <algorithm>
#include <cmath>
#include "../../../../Public/StatsManager/TradingStatsManager.h"


#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

BottomPanel::BottomPanel(CandlestickDataManager* dManager)
    : dataManager(dManager), selectedFileIndex(-1), isVisible(true), activeTab(0), showStrategyPopup(false) {
    scanStrategyFiles();
}

BottomPanel::~BottomPanel() {
}

bool BottomPanel::fileExists(const std::string& filepath) {
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

bool BottomPanel::hasJsonExtension(const std::string& filename) {
    if (filename.length() < 5) return false;

    std::string extension = filename.substr(filename.length() - 5);
    for (char& c : extension) {
        c = tolower(c);
    }

    return extension == ".json";
}

void BottomPanel::scanStrategyFiles() {
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

void BottomPanel::refreshStrategyList() {
    scanStrategyFiles();
    selectedFileIndex = -1;
    selectedStrategy.clear();
}

void BottomPanel::render() {
    if (!isVisible) return;

    ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetIO().DisplaySize.y - 335), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 335), ImGuiCond_Always);

    if (ImGui::Begin("Strategy Control Panel", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {

        if (ImGui::BeginTabBar("ControlTabs")) {

            if (ImGui::BeginTabItem("Backtesting")) {
                activeTab = 0;
                renderPerformanceTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Runtime")) {
                activeTab = 0;
                renderRuntimePerformanceTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Settings")) {
                activeTab = 1;
                renderSettingsTab();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();

    renderStrategySelectionPopup();
}

void BottomPanel::renderPerformanceTab() {
    if (!hasLoadedStrategy()) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "No strategy loaded");
        ImGui::Separator();
        ImGui::TextWrapped("You need to load a strategy first to view performance analytics.");
        ImGui::Spacing();
        if (ImGui::Button("Select Strategy", ImVec2(150, 40))) {
            refreshStrategyList();       
            openStrategySelectionPopup();
            dataManager->GetChart()->clearAllTradingElements();
        }
        return;
    }

    if (ImGui::Button("Run backtest")) {
        dataManager->GetChart()->clearAllTradingElements();
        std::cout << "back test started!!!!!" << std::endl;
        dataManager->GetBlueprintManager()->clearAllBeforeLoad();
        std::string fullPath = "Data/" + selectedStrategy;
        bool success = dataManager->GetBlueprintManager()->loadBlueprint(fullPath);
        std::vector<MarketData>* data = dataManager->getPublicDataPtr();
        data->clear();
        for (auto& Candle : dataManager->getData())
        {
            data->push_back(Candle);
            dataManager->GetBlueprintManager()->executeFromEntry();
        }
    }

    ImGui::Separator();

    ImGui::Text("Strategy Performance: %s", loadedStrategy.c_str());
    ImGui::SameLine();
    if (ImGui::Button("Change Strategy")) {
        refreshStrategyList();
        openStrategySelectionPopup();
    }
    ImGui::Separator();

    float fixedCommissionSOL = dataManager->GetBacktestTradingStats()->GetFixedCommission();
    float percentCommission = dataManager->GetBacktestTradingStats()->GetPercentFixedCommission();

    ImGui::Text("Commission:"); ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    if (ImGui::InputFloat("##FixedSOL", &fixedCommissionSOL, 0.0f, 0.0f, "%.4f")) {
        if (fixedCommissionSOL < 0.0f) fixedCommissionSOL = 0.0f;
        dataManager->GetBacktestTradingStats()->SetFixedCommission(fixedCommissionSOL);
    }
    ImGui::SameLine(); ImGui::Text("$ +"); ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    if (ImGui::InputFloat("##Percent", &percentCommission, 0.0f, 0.0f, "%.2f")) {
        if (percentCommission < 0.0f) percentCommission = 0.0f;
        if (percentCommission > 100.0f) percentCommission = 100.0f;
        dataManager->GetBacktestTradingStats()->SetPercentFixedCommission(percentCommission);
    }
    ImGui::SameLine(); ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%%");

    ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();
    double exampleAmount = 100.0;
    double exampleCommission = (fixedCommissionSOL)+(exampleAmount * percentCommission / 100.0);
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f),
        "(Example: $100 trade = $%.4f commission)", exampleCommission);

    ImGui::Separator();

    double winRate = dataManager->GetBacktestTradingStats()->GetWinRate();
    double roi = dataManager->GetBacktestTradingStats()->GetTotalROI();
    double totalPnL = dataManager->GetBacktestTradingStats()->GetTotalPnL();
    double netPnL = dataManager->GetBacktestTradingStats()->GetNetPnL();
    double totalCommission = dataManager->GetBacktestTradingStats()->GetTotalCommissionPaid();
    size_t totalTrades = dataManager->GetBacktestTradingStats()->GetTotalOrdersCount();
    size_t openPositions = dataManager->GetBacktestTradingStats()->GetOpenPositionsCount();
    size_t closedPositions = dataManager->GetBacktestTradingStats()->GetClosedPositionsCount();
    double avgPnL = dataManager->GetBacktestTradingStats()->GetAveragePnL();
    double bestTrade = dataManager->GetBacktestTradingStats()->GetBestTrade();
    double worstTrade = dataManager->GetBacktestTradingStats()->GetWorstTrade();
    double currentBalance = dataManager->GetBacktestTradingStats()->GetCurrentBalance();
    double startingBalance = dataManager->GetBacktestTradingStats()->GetStartingBalance();

    ImGui::Columns(3, "MainStatsColumns", true);

    ImGui::Text("Win Rate");
    ImGui::Spacing();
    ImGui::Text("%.1f%%", winRate);
    ImGui::ProgressBar(winRate / 100.0f, ImVec2(-1, 30), "");

    ImVec4 winRateColor;
    if (winRate >= 70) winRateColor = ImVec4(0.0f, 0.8f, 0.0f, 1.0f);       
    else if (winRate >= 50) winRateColor = ImVec4(1.0f, 0.8f, 0.0f, 1.0f);  
    else winRateColor = ImVec4(1.0f, 0.2f, 0.0f, 1.0f);                     

    ImGui::TextColored(winRateColor, "Status: %s",
        winRate >= 70 ? "Excellent" : winRate >= 50 ? "Good" : "Poor");

    ImGui::NextColumn();

    ImGui::Text("Total ROI");
    ImGui::Spacing();
    ImGui::Text("%.2f%%", roi);
    float roiProgress = (roi / 100.0) > 1.0 ? 1.0f : ((roi / 100.0) < 0.0 ? 0.0f : (float)(roi / 100.0));
    ImGui::ProgressBar(roiProgress, ImVec2(-1, 30), "");

    ImVec4 roiColor;
    if (roi >= 20) roiColor = ImVec4(0.0f, 0.8f, 0.0f, 1.0f);
    else if (roi >= 0) roiColor = ImVec4(1.0f, 0.8f, 0.0f, 1.0f);
    else roiColor = ImVec4(1.0f, 0.2f, 0.0f, 1.0f);

    ImGui::TextColored(roiColor, "Performance: %s",
        roi >= 20 ? "High" : roi >= 0 ? "Positive" : "Negative");

    ImGui::NextColumn();

    ImGui::Text("Total P&L");
    ImGui::Spacing();
    ImGui::Text("$%s", dataManager->GetBacktestTradingStats()->FormatPnL(totalPnL).c_str());

    ImVec4 pnlColor = totalPnL >= 0 ? ImVec4(0.0f, 0.8f, 0.0f, 1.0f) : ImVec4(1.0f, 0.2f, 0.0f, 1.0f);
    ImGui::TextColored(pnlColor, "%s", totalPnL >= 0 ? "Profit" : "Loss");

    ImGui::Columns(1);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Balance Information:");
    ImGui::Columns(2, "BalanceColumns", true);

    ImGui::BulletText("Starting Balance: $%.2f", startingBalance);
    ImGui::BulletText("Current Balance: $%.2f", currentBalance);
    ImGui::BulletText("Total Shares: %.4f", dataManager->GetBacktestTradingStats()->GetTotalShares());

    ImGui::NextColumn();

    ImGui::BulletText("Net P&L: $%s", dataManager->GetBacktestTradingStats()->FormatPnL(netPnL).c_str());
    ImGui::BulletText("Total Commission: $%.2f", totalCommission);
    ImGui::BulletText("Commission Rate: $%.4f", dataManager->GetBacktestTradingStats()->GetFixedCommission());

    ImGui::Columns(1);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Trading Statistics:");
    ImGui::Columns(2, "TradingStatsColumns", true);

    ImGui::BulletText("Total Orders: %zu", totalTrades);
    ImGui::BulletText("Open Positions: %zu", openPositions);
    ImGui::BulletText("Closed Positions: %zu", closedPositions);

    ImGui::NextColumn();

    ImGui::BulletText("Average P&L: $%s", dataManager->GetBacktestTradingStats()->FormatPnL(avgPnL).c_str());
    ImGui::BulletText("Best Trade: $%s", dataManager->GetBacktestTradingStats()->FormatPnL(bestTrade).c_str());
    ImGui::BulletText("Worst Trade: $%s", dataManager->GetBacktestTradingStats()->FormatPnL(worstTrade).c_str());

    ImGui::Columns(1);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::BeginChild("OrdersScrolling", ImVec2(0, 200), true)) {
        auto& orders = dataManager->GetBacktestTradingStats()->GetOrders();
        auto& positions = dataManager->GetBacktestTradingStats()->GetPositions();

        if (orders.empty()) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No orders yet");
        }
        else {
            std::map<int, std::vector<const Position*>> sellOrderToPositions;
            std::set<int> sellOrderIds;

            for (const auto& pos : positions) {
                if (!pos.isOpen && pos.sellOrderId > 0) {
                    sellOrderToPositions[pos.sellOrderId].push_back(&pos);
                    sellOrderIds.insert(pos.sellOrderId);
                }
            }

            struct Trade {
                int tradeNumber;
                int sellOrderId;
                std::vector<const Position*> positions;
                double totalPnL;
                double totalROI;
                double sellPrice;
                double sellTime;
            };

            std::vector<Trade> trades;
            int tradeNum = 1;

            std::vector<int> sortedSellOrders(sellOrderIds.begin(), sellOrderIds.end());
            std::sort(sortedSellOrders.begin(), sortedSellOrders.end(), [&orders](int a, int b) {
                const Order* orderA = nullptr;
                const Order* orderB = nullptr;
                for (const auto& ord : orders) {
                    if (ord.id == a) orderA = &ord;
                    if (ord.id == b) orderB = &ord;
                }
                if (orderA && orderB) return orderA->time < orderB->time;
                return a < b;
                });

            for (int sellId : sortedSellOrders) {
                Trade trade;
                trade.tradeNumber = tradeNum++;
                trade.sellOrderId = sellId;
                trade.positions = sellOrderToPositions[sellId];
                trade.totalPnL = 0.0;
                trade.totalROI = 0.0;

                for (const auto& ord : orders) {
                    if (ord.id == sellId) {
                        trade.sellPrice = ord.price;
                        trade.sellTime = ord.time;
                        break;
                    }
                }

                double totalInvestment = 0.0;
                for (const auto* pos : trade.positions) {
                    trade.totalPnL += pos->pnl;
                    if (pos->mode == PositionMode::SPOT) {
                        totalInvestment += (pos->buyPrice * pos->quantity + pos->buyCommission);
                    }
                    else {
                        totalInvestment += pos->initialMargin;
                    }
                }

                if (totalInvestment > 0) {
                    trade.totalROI = (trade.totalPnL / totalInvestment) * 100.0;
                }

                trades.push_back(trade);
            }

            std::map<int, int> orderIdToTradeNumber;
            for (const auto& trade : trades) {
                orderIdToTradeNumber[trade.sellOrderId] = trade.tradeNumber;
                for (const auto* pos : trade.positions) {
                    orderIdToTradeNumber[pos->buyOrderId] = trade.tradeNumber;
                }
            }

            ImGui::Columns(8, "OrdersTable", true);
            ImGui::Text("Trade #"); ImGui::NextColumn();
            ImGui::Text("Type"); ImGui::NextColumn();
            ImGui::Text("Price"); ImGui::NextColumn();
            ImGui::Text("Quantity"); ImGui::NextColumn();
            ImGui::Text("Leverage"); ImGui::NextColumn();
            ImGui::Text("Time"); ImGui::NextColumn();
            ImGui::Text("Status"); ImGui::NextColumn();
            ImGui::Text("P&L"); ImGui::NextColumn();
            ImGui::Separator();

            int lastTradeNum = -1;
            bool lastWasSell = false;

            for (int i = orders.size() - 1; i >= 0; i--) {
                const Order& order = orders[i];

                int currentTradeNum = -1;
                auto it = orderIdToTradeNumber.find(order.id);
                if (it != orderIdToTradeNumber.end()) {
                    currentTradeNum = it->second;
                }

                if (lastTradeNum != -1 && currentTradeNum != lastTradeNum) {
                    ImGui::Columns(1);
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                        "--- Trade #%d Complete ---", lastTradeNum);
                    ImGui::Separator();
                    ImGui::Columns(8, "OrdersTable", true);
                }

                bool isClosingOrder = (order.type == OrderType::SELL ||
                    order.type == OrderType::CLOSE_LONG ||
                    order.type == OrderType::CLOSE_SHORT);

                if (lastWasSell && currentTradeNum == -1) {
                    ImGui::Columns(1);
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                        "--- Open Position ---");
                    ImGui::Separator();
                    ImGui::Columns(8, "OrdersTable", true);
                }

                lastWasSell = isClosingOrder;
                lastTradeNum = currentTradeNum;

                if (currentTradeNum > 0) {
                    ImGui::Text("#%d", currentTradeNum);
                }
                else {
                    ImGui::Text("---");
                }
                ImGui::NextColumn();

                const char* orderTypeStr = "";
                ImVec4 typeColor;

                switch (order.type) {
                case OrderType::BUY:
                    orderTypeStr = "BUY";
                    typeColor = ImVec4(0.0f, 0.8f, 0.0f, 1.0f);  
                    break;
                case OrderType::SELL:
                    orderTypeStr = "SELL";
                    typeColor = ImVec4(1.0f, 0.2f, 0.0f, 1.0f);  
                    break;
                case OrderType::LONG:
                    orderTypeStr = "LONG";
                    typeColor = ImVec4(0.0f, 1.0f, 0.5f, 1.0f);   
                    break;
                case OrderType::SHORT:
                    orderTypeStr = "SHORT";
                    typeColor = ImVec4(1.0f, 0.4f, 0.0f, 1.0f);  
                    break;
                case OrderType::CLOSE_LONG:
                    orderTypeStr = "CLOSE LONG";
                    typeColor = ImVec4(0.5f, 1.0f, 0.5f, 1.0f);   
                    break;
                case OrderType::CLOSE_SHORT:
                    orderTypeStr = "CLOSE SHORT";
                    typeColor = ImVec4(1.0f, 0.6f, 0.3f, 1.0f);   
                    break;
                }

                ImGui::TextColored(typeColor, "%s", orderTypeStr);
                ImGui::NextColumn();

                ImGui::Text("$%s", dataManager->GetBacktestTradingStats()->FormatPrice(order.price).c_str());
                ImGui::NextColumn();

                if ((order.type == OrderType::SELL ||
                    order.type == OrderType::CLOSE_LONG ||
                    order.type == OrderType::CLOSE_SHORT) && order.quantity <= 1.0) {
                    ImGui::Text("%.1f%%", order.quantity * 100);
                }
                else {
                    ImGui::Text("%s", dataManager->GetBacktestTradingStats()->FormatQuantity(order.quantity).c_str());
                }
                ImGui::NextColumn();

                if (order.leverage > 1.0) {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "%.0fx", order.leverage);
                }
                else {
                    ImGui::Text("---");
                }
                ImGui::NextColumn();

                if(order.time > 0)
                {
                    time_t timestamp = static_cast<time_t>(order.time / 1000);
                    struct tm timeinfo;
                    localtime_s(&timeinfo, &timestamp);
                    char time_str[32];
                    strftime(time_str, sizeof(time_str), "%m/%d %H:%M:%S", &timeinfo);
                    ImGui::Text("%s", time_str);
                    ImGui::NextColumn();
                }

                if (order.isExecuted) {
                    ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "EXECUTED");
                }
                else {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "PENDING");
                }
                ImGui::NextColumn();

                if (isClosingOrder && currentTradeNum > 0) {
                    for (const auto& trade : trades) {
                        if (trade.tradeNumber == currentTradeNum) {
                            ImVec4 pnlColor = trade.totalPnL >= 0 ?
                                ImVec4(0.0f, 0.8f, 0.0f, 1.0f) : ImVec4(1.0f, 0.2f, 0.0f, 1.0f);
                            ImGui::TextColored(pnlColor, "$%s (%.1f%%)",
                                dataManager->GetBacktestTradingStats()->FormatPnL(trade.totalPnL).c_str(),
                                trade.totalROI);
                            break;
                        }
                    }
                }
                else {
                    ImGui::Text("---");
                }
                ImGui::NextColumn();
            }
            ImGui::Columns(1);
        }
    }
    ImGui::EndChild();

    ImGui::Spacing();

    if (ImGui::Button("Reset Statistics")) {
        dataManager->GetBacktestTradingStats()->Reset();
        dataManager->GetChart()->clearAllTradingElements();
    }
    ImGui::SameLine();
    if (ImGui::Button("Export Trades")) {
        ImGui::OpenPopup("Export Info");
    }

    if (ImGui::BeginPopupModal("Export Info", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Export functionality coming soon!");
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void BottomPanel::renderRuntimePerformanceTab() {
    if (!hasLoadedStrategy()) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "No strategy loaded");
        ImGui::Separator();
        ImGui::TextWrapped("You need to load a strategy first to start runtime trade.");
        ImGui::Spacing();
        if (ImGui::Button("Select Strategy", ImVec2(150, 40))) {
            refreshStrategyList();       
            openStrategySelectionPopup();
            dataManager->GetChart()->clearAllTradingElements();
        }
        return;
    }

    if (ImGui::Button("Run")) {
        if (!dataManager->getData().empty())
            dataManager->SetRuntimeMode(true);
    }
    ImGui::SameLine();
    if (ImGui::Button("Pause")) {
        if (!dataManager->getData().empty())
            dataManager->SetRuntimeMode(false);
    }
    ImGui::SameLine();

    bool isActive = dataManager->RuntimeModeIsActive();
    ImVec4 activeColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    ImVec4 inactiveColor = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);

    ImGui::TextColored(isActive ? activeColor : inactiveColor,
        isActive ? "Active" : "Inactive");

    ImGui::Text("Strategy Performance: %s", loadedStrategy.c_str());
    ImGui::SameLine();
    if (ImGui::Button("Change Strategy")) {
        refreshStrategyList();
        openStrategySelectionPopup();
    }
    ImGui::Separator();

    float fixedCommissionSOL = dataManager->GetBacktestTradingStats()->GetFixedCommission();
    float percentCommission = dataManager->GetBacktestTradingStats()->GetPercentFixedCommission();

    ImGui::Text("Commission:"); ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    if (ImGui::InputFloat("##FixedSOL", &fixedCommissionSOL, 0.0f, 0.0f, "%.4f")) {
        if (fixedCommissionSOL < 0.0f) fixedCommissionSOL = 0.0f;
        dataManager->GetBacktestTradingStats()->SetFixedCommission(fixedCommissionSOL);
    }
    ImGui::SameLine(); ImGui::Text("$ +"); ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    if (ImGui::InputFloat("##Percent", &percentCommission, 0.0f, 0.0f, "%.2f")) {
        if (percentCommission < 0.0f) percentCommission = 0.0f;
        if (percentCommission > 100.0f) percentCommission = 100.0f;
        dataManager->GetBacktestTradingStats()->SetPercentFixedCommission(percentCommission);
    }
    ImGui::SameLine(); ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%%");

    ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();
    double exampleAmount = 100.0;
    double exampleCommission = (fixedCommissionSOL)+(exampleAmount * percentCommission / 100.0);
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f),
        "(Example: $100 trade = $%.4f commission)", exampleCommission);


    ImGui::Separator();

    double winRate = dataManager->GetRuntimeTradingStats()->GetWinRate();
    double roi = dataManager->GetRuntimeTradingStats()->GetTotalROI();
    double totalPnL = dataManager->GetRuntimeTradingStats()->GetTotalPnL();
    double netPnL = dataManager->GetRuntimeTradingStats()->GetNetPnL();
    double totalCommission = dataManager->GetRuntimeTradingStats()->GetTotalCommissionPaid();
    size_t totalTrades = dataManager->GetRuntimeTradingStats()->GetTotalOrdersCount();
    size_t openPositions = dataManager->GetRuntimeTradingStats()->GetOpenPositionsCount();
    size_t closedPositions = dataManager->GetRuntimeTradingStats()->GetClosedPositionsCount();
    double avgPnL = dataManager->GetRuntimeTradingStats()->GetAveragePnL();
    double bestTrade = dataManager->GetRuntimeTradingStats()->GetBestTrade();
    double worstTrade = dataManager->GetRuntimeTradingStats()->GetWorstTrade();
    double currentBalance = dataManager->GetRuntimeTradingStats()->GetCurrentBalance();
    double startingBalance = dataManager->GetRuntimeTradingStats()->GetStartingBalance();

    ImGui::Columns(3, "MainStatsColumns", true);

    ImGui::Text("Win Rate");
    ImGui::Spacing();
    ImGui::Text("%.1f%%", winRate);
    ImGui::ProgressBar(winRate / 100.0f, ImVec2(-1, 30), "");

    ImVec4 winRateColor;
    if (winRate >= 70) winRateColor = ImVec4(0.0f, 0.8f, 0.0f, 1.0f);
    else if (winRate >= 50) winRateColor = ImVec4(1.0f, 0.8f, 0.0f, 1.0f);
    else winRateColor = ImVec4(1.0f, 0.2f, 0.0f, 1.0f);

    ImGui::TextColored(winRateColor, "Status: %s",
        winRate >= 70 ? "Excellent" : winRate >= 50 ? "Good" : "Poor");

    ImGui::NextColumn();

    ImGui::Text("Total ROI");
    ImGui::Spacing();
    ImGui::Text("%.2f%%", roi);
    float roiProgress = (roi / 100.0) > 1.0 ? 1.0f : ((roi / 100.0) < 0.0 ? 0.0f : (float)(roi / 100.0));
    ImGui::ProgressBar(roiProgress, ImVec2(-1, 30), "");

    ImVec4 roiColor;
    if (roi >= 20) roiColor = ImVec4(0.0f, 0.8f, 0.0f, 1.0f);
    else if (roi >= 0) roiColor = ImVec4(1.0f, 0.8f, 0.0f, 1.0f);
    else roiColor = ImVec4(1.0f, 0.2f, 0.0f, 1.0f);

    ImGui::TextColored(roiColor, "Performance: %s",
        roi >= 20 ? "High" : roi >= 0 ? "Positive" : "Negative");

    ImGui::NextColumn();

    ImGui::Text("Total P&L");
    ImGui::Spacing();
    ImGui::Text("$%s", dataManager->GetRuntimeTradingStats()->FormatPnL(totalPnL).c_str());

    ImVec4 pnlColor = totalPnL >= 0 ? ImVec4(0.0f, 0.8f, 0.0f, 1.0f) : ImVec4(1.0f, 0.2f, 0.0f, 1.0f);
    ImGui::TextColored(pnlColor, "%s", totalPnL >= 0 ? "Profit" : "Loss");

    ImGui::Columns(1);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Balance Information:");
    ImGui::Columns(2, "BalanceColumns", true);

    ImGui::BulletText("Starting Balance: $%.2f", startingBalance);
    ImGui::BulletText("Current Balance: $%.2f", currentBalance);
    ImGui::BulletText("Total Shares: %.4f", dataManager->GetRuntimeTradingStats()->GetTotalShares());

    ImGui::NextColumn();

    ImGui::BulletText("Net P&L: $%s", dataManager->GetRuntimeTradingStats()->FormatPnL(netPnL).c_str());
    ImGui::BulletText("Total Commission: $%.2f", totalCommission);
    ImGui::BulletText("Commission Rate: $%.4f", dataManager->GetRuntimeTradingStats()->GetFixedCommission());

    ImGui::Columns(1);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Trading Statistics:");
    ImGui::Columns(2, "TradingStatsColumns", true);

    ImGui::BulletText("Total Orders: %zu", totalTrades);
    ImGui::BulletText("Open Positions: %zu", openPositions);
    ImGui::BulletText("Closed Positions: %zu", closedPositions);

    ImGui::NextColumn();

    ImGui::BulletText("Average P&L: $%s", dataManager->GetRuntimeTradingStats()->FormatPnL(avgPnL).c_str());
    ImGui::BulletText("Best Trade: $%s", dataManager->GetRuntimeTradingStats()->FormatPnL(bestTrade).c_str());
    ImGui::BulletText("Worst Trade: $%s", dataManager->GetRuntimeTradingStats()->FormatPnL(worstTrade).c_str());

    ImGui::Columns(1);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::BeginChild("OrdersScrolling", ImVec2(0, 200), true)) {
        auto& orders = dataManager->GetRuntimeTradingStats()->GetOrders();
        auto& positions = dataManager->GetRuntimeTradingStats()->GetPositions();

        if (orders.empty()) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No orders yet");
        }
        else {
            std::map<int, std::vector<const Position*>> sellOrderToPositions;
            std::set<int> sellOrderIds;

            for (const auto& pos : positions) {
                if (!pos.isOpen && pos.sellOrderId > 0) {
                    sellOrderToPositions[pos.sellOrderId].push_back(&pos);
                    sellOrderIds.insert(pos.sellOrderId);
                }
            }

            struct Trade {
                int tradeNumber;
                int sellOrderId;
                std::vector<const Position*> positions;
                double totalPnL;
                double totalROI;
                double sellPrice;
                double sellTime;
            };

            std::vector<Trade> trades;
            int tradeNum = 1;

            std::vector<int> sortedSellOrders(sellOrderIds.begin(), sellOrderIds.end());
            std::sort(sortedSellOrders.begin(), sortedSellOrders.end(), [&orders](int a, int b) {
                const Order* orderA = nullptr;
                const Order* orderB = nullptr;
                for (const auto& ord : orders) {
                    if (ord.id == a) orderA = &ord;
                    if (ord.id == b) orderB = &ord;
                }
                if (orderA && orderB) return orderA->time < orderB->time;
                return a < b;
                });

            for (int sellId : sortedSellOrders) {
                Trade trade;
                trade.tradeNumber = tradeNum++;
                trade.sellOrderId = sellId;
                trade.positions = sellOrderToPositions[sellId];
                trade.totalPnL = 0.0;
                trade.totalROI = 0.0;

                for (const auto& ord : orders) {
                    if (ord.id == sellId) {
                        trade.sellPrice = ord.price;
                        trade.sellTime = ord.time;
                        break;
                    }
                }

                double totalInvestment = 0.0;
                for (const auto* pos : trade.positions) {
                    trade.totalPnL += pos->pnl;
                    totalInvestment += (pos->buyPrice * pos->quantity + pos->buyCommission);
                }

                if (totalInvestment > 0) {
                    trade.totalROI = (trade.totalPnL / totalInvestment) * 100.0;
                }

                trades.push_back(trade);
            }

            std::map<int, int> orderIdToTradeNumber;
            for (const auto& trade : trades) {
                orderIdToTradeNumber[trade.sellOrderId] = trade.tradeNumber;
                for (const auto* pos : trade.positions) {
                    orderIdToTradeNumber[pos->buyOrderId] = trade.tradeNumber;
                }
            }

            ImGui::Columns(7, "OrdersTable", true);
            ImGui::Text("Trade #"); ImGui::NextColumn();
            ImGui::Text("Type"); ImGui::NextColumn();
            ImGui::Text("Price"); ImGui::NextColumn();
            ImGui::Text("Quantity"); ImGui::NextColumn();
            ImGui::Text("Time"); ImGui::NextColumn();
            ImGui::Text("Status"); ImGui::NextColumn();
            ImGui::Text("P&L"); ImGui::NextColumn();
            ImGui::Separator();

            int totalOrders = (int)orders.size();
            int startIndex = totalOrders > 50 ? totalOrders - 50 : 0;

            int lastTradeNum = -1;
            bool lastWasSell = false;

            for (int i = orders.size() - 1; i >= startIndex; i--) {
                const Order& order = orders[i];

                int currentTradeNum = -1;
                auto it = orderIdToTradeNumber.find(order.id);
                if (it != orderIdToTradeNumber.end()) {
                    currentTradeNum = it->second;
                }

                if (lastTradeNum != -1 && currentTradeNum != lastTradeNum) {
                    ImGui::Columns(1);
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                        "--- Trade #%d Complete ---", lastTradeNum);
                    ImGui::Separator();
                    ImGui::Columns(7, "OrdersTable", true);
                }

                if (lastWasSell && currentTradeNum == -1) {
                    ImGui::Columns(1);
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                        "--- Open Position ---");
                    ImGui::Separator();
                    ImGui::Columns(7, "OrdersTable", true);
                }

                lastWasSell = (order.type == OrderType::SELL);
                lastTradeNum = currentTradeNum;

                if (currentTradeNum > 0) {
                    ImGui::Text("#%d", currentTradeNum);
                }
                else {
                    ImGui::Text("---");
                }
                ImGui::NextColumn();

                ImVec4 typeColor = (order.type == OrderType::BUY) ?
                    ImVec4(0.0f, 0.8f, 0.0f, 1.0f) : ImVec4(1.0f, 0.2f, 0.0f, 1.0f);
                ImGui::TextColored(typeColor, "%s",
                    (order.type == OrderType::BUY) ? "BUY" : "SELL");
                ImGui::NextColumn();

                ImGui::Text("$%s", dataManager->GetRuntimeTradingStats()->FormatPrice(order.price).c_str());
                ImGui::NextColumn();

                if (order.type == OrderType::SELL && order.quantity <= 1.0) {
                    ImGui::Text("%.1f%%", order.quantity * 100);
                }
                else {
                    ImGui::Text("%.1f%%", order.quantity);
                }
                ImGui::NextColumn();

                time_t timestamp = (time_t)order.time;
                struct tm timeinfo;
                localtime_s(&timeinfo, &timestamp);
                char time_str[32];
                strftime(time_str, sizeof(time_str), "%m/%d %H:%M:%S", &timeinfo);
                ImGui::Text("%s", time_str); ImGui::NextColumn();

                if (order.isExecuted) {
                    ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "EXECUTED");
                }
                else {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "PENDING");
                }
                ImGui::NextColumn();

                if (order.type == OrderType::SELL && currentTradeNum > 0) {
                    for (const auto& trade : trades) {
                        if (trade.tradeNumber == currentTradeNum) {
                            ImVec4 pnlColor = trade.totalPnL >= 0 ?
                                ImVec4(0.0f, 0.8f, 0.0f, 1.0f) : ImVec4(1.0f, 0.2f, 0.0f, 1.0f);
                            ImGui::TextColored(pnlColor, "$%s (%.1f%%)",
                                dataManager->GetRuntimeTradingStats()->FormatPnL(trade.totalPnL).c_str(),
                                trade.totalROI);
                            break;
                        }
                    }
                }
                else {
                    ImGui::Text("---");
                }
                ImGui::NextColumn();
            }
            ImGui::Columns(1);
        }
    }
    ImGui::EndChild();

    ImGui::Spacing();

    if (ImGui::Button("Reset Statistics")) {
        dataManager->GetChart()->clearAllTradingElements();
        dataManager->GetRuntimeTradingStats()->Reset();
    }
    ImGui::SameLine();
    if (ImGui::Button("Export Trades")) {
        ImGui::OpenPopup("Export Info");
    }

    if (ImGui::BeginPopupModal("Export Info", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Export functionality coming soon!");
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void BottomPanel::renderSettingsTab() {
    ImGui::Text("Settings & Configuration");
    ImGui::Separator();
    ImGui::TextWrapped("This tab will contain various settings for the trading platform, strategy parameters, and other configurations.");
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Feature coming soon...");
}

void BottomPanel::renderStrategySelectionPopup() {
    if (showStrategyPopup) {
        ImGui::OpenPopup("Select Strategy");
        showStrategyPopup = false;     
    }

    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_Always);

    if (ImGui::BeginPopupModal("Select Strategy", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {

        ImGui::Text("Choose a strategy to load:");
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Refresh List")) {
            refreshStrategyList();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::Spacing();

        if (ImGui::BeginListBox("##StrategyFilesList", ImVec2(-1, 200))) {
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
                    ImGui::SetTooltip("Strategy: %s", strategyFiles[i].c_str());
                }
            }
            ImGui::EndListBox();
        }

        ImGui::Spacing();

        if (strategyFiles.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "No .json files found in Data folder");
        }
        else {
            ImGui::Text("Found %d strategy files", (int)strategyFiles.size());

            if (selectedFileIndex >= 0 && selectedFileIndex < strategyFiles.size()) {
                ImGui::Spacing();
                ImGui::Text("Selected: %s", selectedStrategy.c_str());
                ImGui::Spacing();

                if (ImGui::Button("Load Strategy", ImVec2(120, 30))) {
                    if (dataManager->GetBlueprintManager()) {
                        std::string fullPath = "Data/" + selectedStrategy;
                        bool success = dataManager->GetBlueprintManager()->loadBlueprint(fullPath);
                        

                        if (success) {
                            loadedStrategy = selectedStrategy;
                            ImGui::CloseCurrentPopup();
                        }
                        else {
                            ImGui::OpenPopup("LoadError");
                        }
                    }
                }

                ImGui::SameLine();

                if (ImGui::Button("Delete", ImVec2(120, 30))) {
                    ImGui::OpenPopup("ConfirmDelete");
                }
            }
        }

        if (ImGui::BeginPopupModal("LoadError", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Failed to load strategy!");
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
                    refreshStrategyList();
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
}