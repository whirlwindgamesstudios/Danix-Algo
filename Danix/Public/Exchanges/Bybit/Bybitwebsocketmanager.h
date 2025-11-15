#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <mutex>
#include <functional>
#include "BybitWebSocketClient.h"

class BybitPriceCollector;

struct Subscription {
    std::string symbol;            
    std::string interval;                
    std::string category;             

    std::string getKey() const {
        return category + ":" + symbol + "." + interval;
    }

    bool operator<(const Subscription& other) const {
        return getKey() < other.getKey();
    }

    bool operator==(const Subscription& other) const {
        return getKey() == other.getKey();
    }
};

struct CandleData {
    std::string symbol;
    std::string interval;
    std::string category;
    uint64_t timestamp;
    double open;
    double high;
    double low;
    double close;
    double volume;
    double turnover;
    bool confirmed;
};

class WebSocketConnection {
public:
    WebSocketConnection(const std::string& url, const std::string& category);
    ~WebSocketConnection();

    bool addSubscription(const Subscription& sub);

    bool removeSubscription(const Subscription& sub);

    size_t getSubscriptionCount() const;

    bool isConnected() const;

    std::string getCategory() const { return m_category; }

    void setOnCandleCallback(std::function<void(const CandleData&)> callback);

private:
    void onMessage(const std::string& message);
    void onError(const std::string& error);
    void onConnected();
    void onDisconnected();

    CandleData parseCandleFromJson(const json& data, const std::string& topic);

private:
    std::unique_ptr<BybitWebSocketClient> m_client;
    std::set<Subscription> m_subscriptions;
    std::string m_url;
    std::string m_category;
    mutable std::mutex m_mutex;
    std::atomic<bool> m_isFullyConnected;

    std::function<void(const CandleData&)> m_onCandle;
    std::function<void()> m_onConnected;
};

class BybitWebSocketManager {
public:
    static BybitWebSocketManager& getInstance();

    void shutdown();

    BybitWebSocketManager(const BybitWebSocketManager&) = delete;
    BybitWebSocketManager& operator=(const BybitWebSocketManager&) = delete;

    bool subscribe(const std::string& symbol,
        const std::string& interval,
        const std::string& category,
        BybitPriceCollector* collector);

    bool unsubscribe(const std::string& symbol,
        const std::string& interval,
        const std::string& category,
        BybitPriceCollector* collector);

    void unsubscribeAll(BybitPriceCollector* collector);

    size_t getConnectionCount() const;
    size_t getTotalSubscriptions() const;

    void setMaxSubscriptionsPerConnection(size_t max);
    size_t getMaxSubscriptionsPerConnection() const { return m_maxSubscriptionsPerConnection; }

private:
    BybitWebSocketManager();
    ~BybitWebSocketManager();

    WebSocketConnection* findOrCreateConnection(const std::string& category);

    void cleanupEmptyConnections();

    std::string getWebSocketUrl(const std::string& category) const;

    void onCandleReceived(const CandleData& candle);

private:
    std::map<std::string, std::vector<std::unique_ptr<WebSocketConnection>>> m_connections;

    std::map<std::string, std::set<BybitPriceCollector*>> m_subscribers;

    std::map<BybitPriceCollector*, std::set<std::string>> m_collectorSubscriptions;

    mutable std::mutex m_mutex;

    size_t m_maxSubscriptionsPerConnection;

    static constexpr const char* WS_SPOT_URL = "wss://stream.bybit.com/v5/public/spot";
    static constexpr const char* WS_LINEAR_URL = "wss://stream.bybit.com/v5/public/linear";
    static constexpr const char* WS_INVERSE_URL = "wss://stream.bybit.com/v5/public/inverse";
    static constexpr const char* WS_OPTION_URL = "wss://stream.bybit.com/v5/public/option";
};