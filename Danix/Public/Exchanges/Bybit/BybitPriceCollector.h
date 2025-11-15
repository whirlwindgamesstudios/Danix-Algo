#pragma once

#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <functional>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct CandleData;
class CandlestickDataManager;

struct BybitCandle {
    int index;
    uint64_t timestamp;          
    double open;
    double high;
    double low;
    double close;
    double volume;
    double turnover;          
    bool confirmed;                 

    BybitCandle() : index(0), timestamp(0), open(0), high(0), low(0),
        close(0), volume(0), turnover(0), confirmed(false) {
    }
};

enum class BybitCategory {
    SPOT,             
    LINEAR,            
    INVERSE,           
    OPTION           
};

enum class BybitInterval {
    MIN_1,        
    MIN_3,        
    MIN_5,        
    MIN_15,       
    MIN_30,       
    HOUR_1,       
    HOUR_2,       
    HOUR_4,       
    HOUR_6,       
    HOUR_12,      
    DAY_1,        
    WEEK_1,       
    MONTH_1       
};


class BybitPriceCollector {
public:
    BybitPriceCollector();
    ~BybitPriceCollector();

    void setSymbol(const std::string& symbol, BybitCategory category = BybitCategory::SPOT);

    void setInterval(BybitInterval interval);

    void setDataManager(CandlestickDataManager* dm);

    bool loadHistoricalData(size_t limit = 1000);

    bool startRealtimeStream();

    void stopRealtimeStream();

    bool checkInstrumentExists(const std::string& symbol, BybitCategory category);

    const std::vector<BybitCandle>& getCandles() const;

    BybitCandle getLastCandle() const;

    double getCurrentPrice() const;

    void setOnNewCandleCallback(std::function<void(const BybitCandle&)> callback);

    void setOnCandleUpdateCallback(std::function<void(const BybitCandle&)> callback);

    void setOnStatusCallback(std::function<void(const std::string&)> callback);

    bool isLoading() const { return m_isLoading.load(); }
    bool isStreaming() const { return m_isStreaming.load(); }

    const std::string& getCurrentSymbol() const { return m_symbol; }
    BybitCategory getCurrentCategory() const { return m_category; }
    BybitInterval getCurrentInterval() const { return m_interval; }

    void onCandleReceived(const CandleData& candle);

private:
    std::vector<BybitCandle> fetchKlines(const std::string& symbol,
        BybitCategory category,
        BybitInterval interval,
        uint64_t startTime = 0,
        uint64_t endTime = 0,
        size_t limit = 200);

    json getInstrumentInfo(const std::string& symbol, BybitCategory category);

    json makeHttpRequest(const std::string& endpoint, const std::string& params = "");

    std::vector<BybitCandle> parseKlinesFromJson(const json& data);

    std::string categoryToString(BybitCategory category) const;
    std::string intervalToString(BybitInterval interval) const;

    void updateStatus(const std::string& status);

    void addCandle(const BybitCandle& candle);

    void updateCandle(const BybitCandle& candle);

    void addCandlesToDataManager(const std::vector<BybitCandle>& candles);

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);

private:
    std::string m_symbol;
    BybitCategory m_category;
    BybitInterval m_interval;

    CandlestickDataManager* m_dataManager;

    std::vector<BybitCandle> m_candles;
    mutable std::mutex m_candlesMutex;

    std::atomic<bool> m_isLoading;
    std::atomic<bool> m_isStreaming;

    CURL* m_curl;

    std::function<void(const BybitCandle&)> m_onNewCandle;
    std::function<void(const BybitCandle&)> m_onCandleUpdate;
    std::function<void(const std::string&)> m_onStatus;

    static constexpr const char* REST_API_URL = "https://api.bybit.com";
};