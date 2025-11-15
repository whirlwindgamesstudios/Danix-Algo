#pragma once

#include <vector>
#include <utility>

#include <string>
#include <mutex>


struct MarketData {
    int index;
    uint64_t timestamp;
    long double open;
    long double high;
    long double low;
    long double close;
    float volume;
    float mcap;
};

class CandlestickChart;
class BlueprintManager;
class TradingStatsManager;
class CandlestickDataManager {
private:
    std::string symbol;
    std::vector<MarketData> data;
    std::vector<MarketData> public_data;
    bool RuntimeMode = false;
    std::string token_key;
    CandlestickChart* chart;
    BlueprintManager* dataManager;
    TradingStatsManager* backtestStatsManager;
    TradingStatsManager* runtimeStatsManager;
public:
    CandlestickDataManager();
    void addData(const std::vector<MarketData>& newData);

    void addCandle(const MarketData& candle);

    void UpdateLastCandle(const MarketData& candle);

    const std::vector<MarketData>& getData() const;

    std::vector<MarketData> getDataRange(size_t start, size_t end) const;

    const std::vector<MarketData>& getPublicData() const;
    std::vector<MarketData>* getPublicDataPtr();
    std::vector<MarketData> getPublicDataRange(size_t start, size_t end) const;

    void clear();

    size_t size() const;

    bool empty() const;

    std::pair<double, double> getPriceRange(size_t start = 0, size_t end = SIZE_MAX) const;

    void SaveCSVFile(const std::string& filename);
    void LoadSelectedCSVFile(const std::string& filename);
    bool RuntimeModeIsActive() { return RuntimeMode; }
    void SetRuntimeMode(bool newMode) { RuntimeMode = newMode; }

    void SetChart(CandlestickChart* newChart) { chart = newChart; }
    CandlestickChart* GetChart() { return chart; }

    void SetBlueprintManager(BlueprintManager* newBlueprint) { dataManager = newBlueprint; }
    BlueprintManager* GetBlueprintManager() { return dataManager; }

    void SetBacktestTradingStats(TradingStatsManager* newStatsManager) { backtestStatsManager = newStatsManager; }
    TradingStatsManager* GetBacktestTradingStats() { return backtestStatsManager; }

    void SetRuntimeTradingStats(TradingStatsManager* newStatsManager) { runtimeStatsManager = newStatsManager; }
    TradingStatsManager* GetRuntimeTradingStats() { return runtimeStatsManager; }

    void SetTokenKey(std::string newKey) { token_key = newKey; }
    std::string GetTokenKey() { return token_key; }

    void SetSymbol(std::string newSymbol) { symbol = newSymbol; }
    std::string GetSymbol() { return symbol; }
};