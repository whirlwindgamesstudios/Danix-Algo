#include "../../../Public/Exchanges/Bybit/BybitPriceCollector.h"
#include "../../../Public/Exchanges/Bybit/BybitWebSocketManager.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include "../../../Public/Chart/CandleChartManager.h"


BybitPriceCollector::BybitPriceCollector()
    : m_symbol("")
    , m_category(BybitCategory::SPOT)
    , m_interval(BybitInterval::MIN_1)
    , m_dataManager(nullptr)
    , m_isLoading(false)
    , m_isStreaming(false)
    , m_curl(nullptr)
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
    m_curl = curl_easy_init();

    if (!m_curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }
}

BybitPriceCollector::~BybitPriceCollector() {
    stopRealtimeStream();

    if (m_curl) {
        curl_easy_cleanup(m_curl);
    }

    curl_global_cleanup();
}

void BybitPriceCollector::setSymbol(const std::string& symbol, BybitCategory category) {
    bool wasStreaming = m_isStreaming.load();

    if (wasStreaming) {
        stopRealtimeStream();
    }

    m_symbol = symbol;
    m_category = category;

    updateStatus("Symbol set: " + symbol + " (" + categoryToString(category) + ")");

    if (wasStreaming) {
        startRealtimeStream();
    }
}

void BybitPriceCollector::setInterval(BybitInterval interval) {
    bool wasStreaming = m_isStreaming.load();

    if (wasStreaming) {
        stopRealtimeStream();
    }

    m_interval = interval;

    updateStatus("Interval set: " + intervalToString(interval));

    if (wasStreaming) {
        startRealtimeStream();
    }
}

void BybitPriceCollector::setDataManager(CandlestickDataManager* dm) {
    m_dataManager = dm;
}

bool BybitPriceCollector::checkInstrumentExists(const std::string& symbol, BybitCategory category) {
    try {
        json info = getInstrumentInfo(symbol, category);

        if (info.contains("retCode") && info["retCode"] == 0) {
            if (info.contains("result") && info["result"].contains("list")) {
                auto list = info["result"]["list"];
                return !list.empty();
            }
        }

        return false;

    }
    catch (const std::exception& e) {
        updateStatus("Error checking instrument: " + std::string(e.what()));
        return false;
    }
}

bool BybitPriceCollector::loadHistoricalData(size_t limit) {
    if (m_isLoading.load()) {
        updateStatus("Already loading data...");
        return false;
    }

    if (m_symbol.empty()) {
        updateStatus("Error: Symbol not set");
        return false;
    }

    m_isLoading = true;
    updateStatus("Loading historical data...");

    try {
        if (!checkInstrumentExists(m_symbol, m_category)) {
            updateStatus("Error: Instrument not found");
            m_isLoading = false;
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(m_candlesMutex);
            m_candles.clear();
        }

        std::vector<BybitCandle> allCandles;

        uint64_t endTime = 0;
        size_t candlesLoaded = 0;
        size_t maxIterations = (limit / 1000) + 1;

        updateStatus("Loading " + std::to_string(limit) + " candles in chunks...");

        for (size_t iteration = 0; iteration < maxIterations && candlesLoaded < limit; ++iteration) {
            size_t chunkSize = std::min<size_t>(1000, limit - candlesLoaded);

            std::cout << "[PriceCollector] Loading chunk " << (iteration + 1)
                << "/" << maxIterations
                << " (requesting " << chunkSize << " candles)" << std::endl;

            std::vector<BybitCandle> chunk = fetchKlines(
                m_symbol,
                m_category,
                m_interval,
                0,
                endTime,
                chunkSize
            );

            if (chunk.empty()) {
                std::cout << "[PriceCollector] No more data available" << std::endl;
                break;
            }

            std::cout << "[PriceCollector] Loaded " << chunk.size() << " candles in this chunk" << std::endl;

            allCandles.insert(allCandles.end(), chunk.begin(), chunk.end());
            candlesLoaded += chunk.size();

            if (candlesLoaded % 1000 == 0) {
                updateStatus("Loaded " + std::to_string(candlesLoaded) + " / " + std::to_string(limit) + " candles...");
            }

            if (chunk.size() < chunkSize) {
                std::cout << "[PriceCollector] Reached end of available history" << std::endl;
                break;
            }

            uint64_t oldestTimestamp = chunk[0].timestamp;
            for (const auto& candle : chunk) {
                if (candle.timestamp < oldestTimestamp) {
                    oldestTimestamp = candle.timestamp;
                }
            }

            endTime = oldestTimestamp - 1;

            if (iteration < maxIterations - 1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        }

        if (allCandles.empty()) {
            updateStatus("Error: No data received");
            m_isLoading = false;
            return false;
        }

        std::cout << "[PriceCollector] Total loaded: " << allCandles.size() << " candles" << std::endl;

        std::sort(allCandles.begin(), allCandles.end(), [](const BybitCandle& a, const BybitCandle& b) {
            return a.timestamp < b.timestamp;
            });

        auto last = std::unique(allCandles.begin(), allCandles.end(),
            [](const BybitCandle& a, const BybitCandle& b) {
                return a.timestamp == b.timestamp;
            });
        allCandles.erase(last, allCandles.end());

        std::cout << "[PriceCollector] After deduplication: " << allCandles.size() << " candles" << std::endl;

        for (size_t i = 0; i < allCandles.size(); ++i) {
            allCandles[i].index = static_cast<int>(i);
        }

        {
            std::lock_guard<std::mutex> lock(m_candlesMutex);
            m_candles = allCandles;
        }

        if (m_dataManager) {
            addCandlesToDataManager(allCandles);
        }

        updateStatus("Loaded " + std::to_string(m_candles.size()) + " candles");
        m_isLoading = false;

        return true;

    }
    catch (const std::exception& e) {
        updateStatus("Error loading data: " + std::string(e.what()));
        m_isLoading = false;
        return false;
    }
}

bool BybitPriceCollector::startRealtimeStream() {
    if (m_isStreaming.load()) {
        updateStatus("Already streaming");
        return false;
    }

    if (m_symbol.empty()) {
        updateStatus("Error: Symbol not set");
        return false;
    }

    // Если нет исторических данных - загружаем
    if (m_candles.empty()) {
        updateStatus("No historical data, loading...");
        if (!loadHistoricalData()) {
            return false;
        }
    }

    updateStatus("Starting real-time stream...");

    // Подписываемся через глобальный WebSocketManager
    bool success = BybitWebSocketManager::getInstance().subscribe(
        m_symbol,
        intervalToString(m_interval),
        categoryToString(m_category),
        this
    );

    if (!success) {
        updateStatus("Error: Failed to subscribe to WebSocket");
        return false;
    }

    m_isStreaming = true;
    updateStatus("Streaming started for " + m_symbol);

    return true;
}

void BybitPriceCollector::stopRealtimeStream() {
    if (!m_isStreaming.load()) {
        return;
    }

    updateStatus("Stopping stream...");

    // Отписываемся через глобальный WebSocketManager
    // Это автоматически удалит все подписки этого коллектора
    BybitWebSocketManager::getInstance().unsubscribeAll(this);

    m_isStreaming = false;
    updateStatus("Streaming stopped");
}

const std::vector<BybitCandle>& BybitPriceCollector::getCandles() const {
    return m_candles;
}

BybitCandle BybitPriceCollector::getLastCandle() const {
    std::lock_guard<std::mutex> lock(m_candlesMutex);

    if (m_candles.empty()) {
        return BybitCandle();
    }

    return m_candles.back();
}

double BybitPriceCollector::getCurrentPrice() const {
    std::lock_guard<std::mutex> lock(m_candlesMutex);

    if (m_candles.empty()) {
        return 0.0;
    }

    return m_candles.back().close;
}

void BybitPriceCollector::setOnNewCandleCallback(std::function<void(const BybitCandle&)> callback) {
    m_onNewCandle = callback;
}

void BybitPriceCollector::setOnCandleUpdateCallback(std::function<void(const BybitCandle&)> callback) {
    m_onCandleUpdate = callback;
}

void BybitPriceCollector::setOnStatusCallback(std::function<void(const std::string&)> callback) {
    m_onStatus = callback;
}

// === ВЫЗЫВАЕТСЯ ИЗ WebSocketManager ===

void BybitPriceCollector::onCandleReceived(const CandleData& candleData) {
    // Конвертируем CandleData в BybitCandle
    BybitCandle candle;
    candle.timestamp = candleData.timestamp;
    candle.open = candleData.open;
    candle.high = candleData.high;
    candle.low = candleData.low;
    candle.close = candleData.close;
    candle.volume = candleData.volume;
    candle.turnover = candleData.turnover;
    candle.confirmed = candleData.confirmed;

    if (candle.confirmed) {
        // Свеча закрылась - добавляем новую
        addCandle(candle);

        // Передаём в DataManager
        //if (m_dataManager) {
            //std::vector<BybitCandle> singleCandle = { candle };
            //addCandlesToDataManager(singleCandle);
        //}

        if (m_onNewCandle) {
            if (!m_candles.empty())
            m_onNewCandle(m_candles.back());
        }
    }
    else {
        // Свеча ещё формируется - обновляем последнюю
        updateCandle(candle);

        if (m_onCandleUpdate) {
            if(!m_candles.empty())
            m_onCandleUpdate(m_candles.back());
        }
    }
}

// === REST API МЕТОДЫ ===

std::vector<BybitCandle> BybitPriceCollector::fetchKlines(
    const std::string& symbol,
    BybitCategory category,
    BybitInterval interval,
    uint64_t startTime,
    uint64_t endTime,
    size_t limit)
{
    try {
        std::stringstream params;
        params << "?category=" << categoryToString(category);
        params << "&symbol=" << symbol;
        params << "&interval=" << intervalToString(interval);

        if (limit > 0) {
            params << "&limit=" << std::min<size_t>(limit, (size_t)1000);
        }

        if (startTime > 0) {
            params << "&start=" << startTime;
        }

        if (endTime > 0) {
            params << "&end=" << endTime;
        }

        json response = makeHttpRequest("/v5/market/kline", params.str());

        return parseKlinesFromJson(response);

    }
    catch (const std::exception& e) {
        std::cerr << "fetchKlines error: " << e.what() << std::endl;
        return std::vector<BybitCandle>();
    }
}

json BybitPriceCollector::getInstrumentInfo(const std::string& symbol, BybitCategory category) {
    try {
        std::stringstream params;
        params << "?category=" << categoryToString(category);
        params << "&symbol=" << symbol;

        return makeHttpRequest("/v5/market/instruments-info", params.str());

    }
    catch (const std::exception& e) {
        std::cerr << "getInstrumentInfo error: " << e.what() << std::endl;
        return json::object();
    }
}

json BybitPriceCollector::makeHttpRequest(const std::string& endpoint, const std::string& params) {
    if (!m_curl) {
        throw std::runtime_error("CURL not initialized");
    }

    std::string url = std::string(REST_API_URL) + endpoint + params;
    std::string responseBuffer;

    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &responseBuffer);
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = curl_easy_perform(m_curl);

    if (res != CURLE_OK) {
        throw std::runtime_error("CURL request failed: " + std::string(curl_easy_strerror(res)));
    }

    if (responseBuffer.empty()) {
        throw std::runtime_error("Empty response");
    }

    return json::parse(responseBuffer);
}

std::vector<BybitCandle> BybitPriceCollector::parseKlinesFromJson(const json& data) {
    std::vector<BybitCandle> candles;

    try {
        if (!data.contains("retCode") || data["retCode"] != 0) {
            return candles;
        }

        if (!data.contains("result") || !data["result"].contains("list")) {
            return candles;
        }

        const auto& list = data["result"]["list"];

        if (!list.is_array()) {
            return candles;
        }

        for (const auto& item : list) {
            if (!item.is_array() || item.size() < 7) {
                continue;
            }

            BybitCandle candle;
            candle.timestamp = std::stoull(item[0].get<std::string>());
            candle.open = std::stod(item[1].get<std::string>());
            candle.high = std::stod(item[2].get<std::string>());
            candle.low = std::stod(item[3].get<std::string>());
            candle.close = std::stod(item[4].get<std::string>());
            candle.volume = std::stod(item[5].get<std::string>());
            candle.turnover = std::stod(item[6].get<std::string>());
            candle.confirmed = true;

            candles.push_back(candle);
        }

    }
    catch (const std::exception& e) {
        std::cerr << "Parse klines error: " << e.what() << std::endl;
    }

    return candles;
}

std::string BybitPriceCollector::categoryToString(BybitCategory category) const {
    switch (category) {
    case BybitCategory::SPOT: return "spot";
    case BybitCategory::LINEAR: return "linear";
    case BybitCategory::INVERSE: return "inverse";
    case BybitCategory::OPTION: return "option";
    default: return "spot";
    }
}

std::string BybitPriceCollector::intervalToString(BybitInterval interval) const {
    switch (interval) {
    case BybitInterval::MIN_1: return "1";
    case BybitInterval::MIN_3: return "3";
    case BybitInterval::MIN_5: return "5";
    case BybitInterval::MIN_15: return "15";
    case BybitInterval::MIN_30: return "30";
    case BybitInterval::HOUR_1: return "60";
    case BybitInterval::HOUR_2: return "120";
    case BybitInterval::HOUR_4: return "240";
    case BybitInterval::HOUR_6: return "360";
    case BybitInterval::HOUR_12: return "720";
    case BybitInterval::DAY_1: return "D";
    case BybitInterval::WEEK_1: return "W";
    case BybitInterval::MONTH_1: return "M";
    default: return "1";
    }
}

void BybitPriceCollector::updateStatus(const std::string& status) {
    if (m_onStatus) {
        m_onStatus(status);
    }
}

void BybitPriceCollector::addCandle(const BybitCandle& candle) {
    std::lock_guard<std::mutex> lock(m_candlesMutex);

    BybitCandle newCandle = candle;
    newCandle.index = static_cast<int>(m_candles.size() - 1);

    m_candles.push_back(newCandle);
}

void BybitPriceCollector::updateCandle(const BybitCandle& candle) {
    std::lock_guard<std::mutex> lock(m_candlesMutex);

    if (m_candles.empty()) {
        return;
    }

    BybitCandle& lastCandle = m_candles.back();
    lastCandle.index = static_cast<int>(m_candles.size() - 1);
    lastCandle.timestamp = candle.timestamp;
    lastCandle.open = candle.open;
    lastCandle.high = candle.high;
    lastCandle.low = candle.low;
    lastCandle.close = candle.close;
    lastCandle.volume = candle.volume;

    /*MarketData newCandle;

    newCandle.timestamp = candle.timestamp;
    newCandle.open = candle.open;
    newCandle.high = candle.high;
    newCandle.low = candle.low;
    newCandle.close = candle.close;
    newCandle.volume = candle.volume;
    m_dataManager->UpdateLastCandle(newCandle);*/
    

}

void BybitPriceCollector::addCandlesToDataManager(const std::vector<BybitCandle>& candles) {
    if (!m_dataManager) return;

    std::vector<MarketData> marketData;
    marketData.reserve(candles.size());

    for (const auto& candle : candles) {
        MarketData md;
        md.index = candle.index;
        md.timestamp = candle.timestamp;
        md.open = candle.open;
        md.high = candle.high;
        md.low = candle.low;
        md.close = candle.close;
        md.volume = candle.volume;
        md.mcap = 0.0;  // Bybit не даёт mcap
        marketData.push_back(md);
    }

    if (marketData.size() > 10) {
        m_dataManager->addData(marketData);
    }
    else {
        for (const auto& md : marketData) {
            m_dataManager->addCandle(md);
        }
    }
}

size_t BybitPriceCollector::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t totalSize = size * nmemb;
    userp->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}