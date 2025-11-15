#include "../../../Public/Exchanges/Bybit/Bybitwebsocketmanager.h"
#include "../../../Public/Exchanges/Bybit/BybitPriceCollector.h"
#include <iostream>
#include <algorithm>

// ============================================================================
// WebSocketConnection Implementation
// ============================================================================

WebSocketConnection::WebSocketConnection(const std::string& url, const std::string& category)
    : m_url(url)
    , m_category(category)
    , m_isFullyConnected(false)
{
    m_client = std::make_unique<BybitWebSocketClient>();

    // Установить callbacks
    m_client->setOnMessageCallback([this](const std::string& msg) {
        onMessage(msg);
        });

    m_client->setOnErrorCallback([this](const std::string& error) {
        onError(error);
        });

    m_client->setOnConnectedCallback([this]() {
        onConnected();
        });

    m_client->setOnDisconnectedCallback([this]() {
        onDisconnected();
        });

    // Подключаемся
    if (!m_client->connect(url)) {
        std::cerr << "Failed to connect WebSocket: " << url << std::endl;
        return;
    }

    // КРИТИЧНО: Ждём РЕАЛЬНОГО подключения (до 5 секунд)
    for (int i = 0; i < 50; ++i) {
        if (m_isFullyConnected.load()) {
            std::cout << "[WebSocketConnection] Successfully connected after "
                << (i * 100) << "ms" << std::endl;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!m_isFullyConnected.load()) {
        std::cerr << "[WebSocketConnection] Warning: Connection timeout, may not be ready" << std::endl;
    }
}

WebSocketConnection::~WebSocketConnection() {
    std::cout << "[WebSocketConnection] Destroying connection to " << m_url << std::endl;

    // Отписываемся от всех каналов
    if (!m_subscriptions.empty()) {
        std::cout << "[WebSocketConnection] Unsubscribing from "
            << m_subscriptions.size() << " channels..." << std::endl;

        for (const auto& sub : m_subscriptions) {
            std::string topic = "kline." + sub.interval + "." + sub.symbol;
            if (m_client) {
                m_client->unsubscribe(topic);
            }
        }

        // Даём время на отправку unsubscribe
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Закрываем соединение
    if (m_client) {
        m_client->disconnect();
    }

    std::cout << "[WebSocketConnection] Connection destroyed" << std::endl;
}

bool WebSocketConnection::addSubscription(const Subscription& sub) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Проверяем, подключены ли мы
    if (!m_isFullyConnected.load()) {
        std::cerr << "[WebSocketConnection] Cannot subscribe - not connected yet" << std::endl;
        return false;
    }

    // Проверяем, не подписаны ли уже
    if (m_subscriptions.find(sub) != m_subscriptions.end()) {
        return true; // Уже подписаны
    }

    // Подписываемся через WebSocket
    if (!m_client->subscribeToKline(sub.symbol, sub.interval)) {
        std::cerr << "Failed to subscribe to " << sub.getKey() << std::endl;
        return false;
    }

    m_subscriptions.insert(sub);

    std::cout << "[WebSocketConnection] Subscribed to " << sub.getKey()
        << " (total: " << m_subscriptions.size() << ")" << std::endl;

    return true;
}

bool WebSocketConnection::removeSubscription(const Subscription& sub) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_subscriptions.find(sub);
    if (it == m_subscriptions.end()) {
        return false; // Не были подписаны
    }

    // Отписываемся через WebSocket
    std::string topic = "kline." + sub.interval + "." + sub.symbol;
    m_client->unsubscribe(topic);

    m_subscriptions.erase(it);

    std::cout << "[WebSocketConnection] Unsubscribed from " << sub.getKey()
        << " (remaining: " << m_subscriptions.size() << ")" << std::endl;

    return true;
}

size_t WebSocketConnection::getSubscriptionCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_subscriptions.size();
}

bool WebSocketConnection::isConnected() const {
    return m_client && m_client->isConnected();
}

void WebSocketConnection::setOnCandleCallback(std::function<void(const CandleData&)> callback) {
    m_onCandle = callback;
}

void WebSocketConnection::onMessage(const std::string& message) {
    try {
        json data = json::parse(message);

        if (!data.contains("topic") || !data.contains("data")) {
            return;
        }

        std::string topic = data["topic"];

        // Проверяем, что это kline
        if (topic.find("kline") == std::string::npos) {
            return;
        }

        auto candleArray = data["data"];
        if (candleArray.is_array() && !candleArray.empty()) {
            CandleData candle = parseCandleFromJson(candleArray[0], topic);

            if (m_onCandle) {
                m_onCandle(candle);
            }
        }

    }
    catch (const std::exception& e) {
        std::cerr << "[WebSocketConnection] Parse error: " << e.what() << std::endl;
    }
}

void WebSocketConnection::onError(const std::string& error) {
    std::cerr << "[WebSocketConnection] Error: " << error << std::endl;
}

void WebSocketConnection::onConnected() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isFullyConnected = true;

    std::cout << "[WebSocketConnection] Connected to " << m_url << std::endl;

    if (m_onConnected) {
        m_onConnected();
    }
}

void WebSocketConnection::onDisconnected() {
    std::cout << "[WebSocketConnection] Disconnected from " << m_url << std::endl;
}

CandleData WebSocketConnection::parseCandleFromJson(const json& data, const std::string& topic) {
    CandleData candle;

    try {
        // Парсим topic: "kline.1.BTCUSDT"
        size_t firstDot = topic.find('.');
        size_t secondDot = topic.find('.', firstDot + 1);

        if (firstDot != std::string::npos && secondDot != std::string::npos) {
            candle.interval = topic.substr(firstDot + 1, secondDot - firstDot - 1);
            candle.symbol = topic.substr(secondDot + 1);
        }

        candle.category = m_category;
        candle.timestamp = data["start"].get<uint64_t>();
        candle.open = std::stod(data["open"].get<std::string>());
        candle.high = std::stod(data["high"].get<std::string>());
        candle.low = std::stod(data["low"].get<std::string>());
        candle.close = std::stod(data["close"].get<std::string>());
        candle.volume = std::stod(data["volume"].get<std::string>());
        candle.turnover = std::stod(data["turnover"].get<std::string>());
        candle.confirmed = data["confirm"].get<bool>();

    }
    catch (const std::exception& e) {
        std::cerr << "[WebSocketConnection] Parse candle error: " << e.what() << std::endl;
    }

    return candle;
}

// ============================================================================
// BybitWebSocketManager Implementation (СИНГЛТОН)
// ============================================================================

BybitWebSocketManager& BybitWebSocketManager::getInstance() {
    static BybitWebSocketManager instance;
    return instance;
}

BybitWebSocketManager::BybitWebSocketManager()
    : m_maxSubscriptionsPerConnection(10)  // По умолчанию 10 подписок на соединение
{
    std::cout << "[WebSocketManager] Initialized" << std::endl;
}

BybitWebSocketManager::~BybitWebSocketManager() {
    shutdown();
}

void BybitWebSocketManager::shutdown() {
    static std::atomic<bool> shutdownCalled(false);

    // Защита от повторного вызова
    if (shutdownCalled.exchange(true)) {
        return;
    }

    std::cout << "[WebSocketManager] Shutting down..." << std::endl;

    std::lock_guard<std::mutex> lock(m_mutex);

    // Отписываем всех коллекторов
    std::cout << "[WebSocketManager] Unsubscribing all collectors..." << std::endl;
    size_t collectorCount = m_collectorSubscriptions.size();
    size_t subscriptionCount = m_subscribers.size();

    m_subscribers.clear();
    m_collectorSubscriptions.clear();

    std::cout << "[WebSocketManager] Unsubscribed " << collectorCount
        << " collectors from " << subscriptionCount << " subscriptions" << std::endl;

    // Закрываем все WebSocket соединения
    std::cout << "[WebSocketManager] Closing all WebSocket connections..." << std::endl;
    size_t totalConnections = 0;

    for (auto& item : m_connections) {
        auto& category = item.first;
        auto& connections = item.second;

        std::cout << "[WebSocketManager] Closing " << connections.size()
            << " connections in category: " << category << std::endl;

        totalConnections += connections.size();
        connections.clear();  // Деструкторы WebSocketConnection закроют соединения
    }


    m_connections.clear();

    std::cout << "[WebSocketManager] Closed " << totalConnections
        << " total connections" << std::endl;
    std::cout << "[WebSocketManager] Shutdown complete" << std::endl;
}

bool BybitWebSocketManager::subscribe(
    const std::string& symbol,
    const std::string& interval,
    const std::string& category,
    BybitPriceCollector* collector)
{
    if (!collector) {
        std::cerr << "[WebSocketManager] Collector is null" << std::endl;
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    Subscription sub;
    sub.symbol = symbol;
    sub.interval = interval;
    sub.category = category;

    std::string subKey = sub.getKey();

    // Проверяем, не подписан ли уже этот коллектор
    auto& subs = m_subscribers[subKey];
    if (subs.find(collector) != subs.end()) {
        std::cout << "[WebSocketManager] Collector already subscribed to " << subKey << std::endl;
        return true;
    }

    // Если это первая подписка на этот ключ - добавляем в соединение
    if (subs.empty()) {
        WebSocketConnection* conn = findOrCreateConnection(category);

        if (!conn) {
            std::cerr << "[WebSocketManager] Failed to get connection" << std::endl;
            return false;
        }

        if (!conn->addSubscription(sub)) {
            std::cerr << "[WebSocketManager] Failed to add subscription to connection" << std::endl;
            return false;
        }
    }

    // Добавляем коллектор в список подписчиков
    subs.insert(collector);

    // Регистрируем подписку для коллектора (для быстрого удаления всех подписок)
    m_collectorSubscriptions[collector].insert(subKey);

    std::cout << "[WebSocketManager] Subscribed " << collector
        << " to " << subKey
        << " (subscribers: " << subs.size() << ")" << std::endl;

    return true;
}

bool BybitWebSocketManager::unsubscribe(
    const std::string& symbol,
    const std::string& interval,
    const std::string& category,
    BybitPriceCollector* collector)
{
    if (!collector) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    Subscription sub;
    sub.symbol = symbol;
    sub.interval = interval;
    sub.category = category;

    std::string subKey = sub.getKey();

    // Удаляем коллектор из подписчиков
    auto subIt = m_subscribers.find(subKey);
    if (subIt == m_subscribers.end()) {
        return false;
    }

    subIt->second.erase(collector);

    // Удаляем из реестра коллектора
    auto collIt = m_collectorSubscriptions.find(collector);
    if (collIt != m_collectorSubscriptions.end()) {
        collIt->second.erase(subKey);

        // Если у коллектора больше нет подписок - удаляем его из реестра
        if (collIt->second.empty()) {
            m_collectorSubscriptions.erase(collIt);
        }
    }

    std::cout << "[WebSocketManager] Unsubscribed " << collector << " from " << subKey << std::endl;

    // Если больше нет подписчиков на этот ключ - удаляем подписку из соединения
    if (subIt->second.empty()) {
        m_subscribers.erase(subIt);

        // Находим соединение и удаляем подписку
        auto connIt = m_connections.find(category);
        if (connIt != m_connections.end()) {
            for (auto& conn : connIt->second) {
                conn->removeSubscription(sub);
            }
        }

        // Очищаем пустые соединения
        cleanupEmptyConnections();
    }

    return true;
}

void BybitWebSocketManager::unsubscribeAll(BybitPriceCollector* collector) {
    if (!collector) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    auto collIt = m_collectorSubscriptions.find(collector);
    if (collIt == m_collectorSubscriptions.end()) {
        return; // Нет подписок
    }

    std::cout << "[WebSocketManager] Unsubscribing collector from "
        << collIt->second.size() << " subscriptions" << std::endl;

    // Копируем список подписок (т.к. будем модифицировать оригинал)
    auto subscriptions = collIt->second;

    // Удаляем все подписки
    for (const auto& subKey : subscriptions) {
        // Удаляем коллектор из подписчиков
        auto subIt = m_subscribers.find(subKey);
        if (subIt != m_subscribers.end()) {
            subIt->second.erase(collector);

            // Если больше нет подписчиков - удаляем подписку
            if (subIt->second.empty()) {
                m_subscribers.erase(subIt);

                // Парсим subKey обратно в Subscription
                // Формат: "category:symbol.interval"
                size_t colonPos = subKey.find(':');
                size_t dotPos = subKey.find('.');

                if (colonPos != std::string::npos && dotPos != std::string::npos) {
                    Subscription sub;
                    sub.category = subKey.substr(0, colonPos);
                    sub.symbol = subKey.substr(colonPos + 1, dotPos - colonPos - 1);
                    sub.interval = subKey.substr(dotPos + 1);

                    // Удаляем из соединения
                    auto connIt = m_connections.find(sub.category);
                    if (connIt != m_connections.end()) {
                        for (auto& conn : connIt->second) {
                            conn->removeSubscription(sub);
                        }
                    }
                }
            }
        }
    }

    // Удаляем коллектор из реестра
    m_collectorSubscriptions.erase(collIt);

    // Очищаем пустые соединения
    cleanupEmptyConnections();
}

size_t BybitWebSocketManager::getConnectionCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    size_t count = 0;
    for (const auto& item : m_connections) {
        const auto& category = item.first;
        const auto& connections = item.second;
        count += connections.size();
    }

    return count;
}

size_t BybitWebSocketManager::getTotalSubscriptions() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_subscribers.size();
}

void BybitWebSocketManager::setMaxSubscriptionsPerConnection(size_t max) {
    if (max > 0) {
        m_maxSubscriptionsPerConnection = max;
        std::cout << "[WebSocketManager] Max subscriptions per connection set to " << max << std::endl;
    }
}

WebSocketConnection* BybitWebSocketManager::findOrCreateConnection(const std::string& category) {
    auto& connections = m_connections[category];

    // Ищем соединение с местом
    for (auto& conn : connections) {
        size_t currentSubs = conn->getSubscriptionCount();

        std::cout << "[WebSocketManager] Checking connection: "
            << currentSubs << "/" << m_maxSubscriptionsPerConnection
            << " subscriptions" << std::endl;

        if (currentSubs < m_maxSubscriptionsPerConnection) {
            std::cout << "[WebSocketManager] Using existing connection" << std::endl;
            return conn.get();
        }
    }

    // Все соединения полные - создаём новое
    std::cout << "[WebSocketManager] All connections full, creating new one..." << std::endl;

    std::string url = getWebSocketUrl(category);
    auto newConn = std::make_unique<WebSocketConnection>(url, category);

    // Проверяем что соединение успешно подключилось
    if (!newConn->isConnected()) {
        std::cerr << "[WebSocketManager] Failed to create new connection" << std::endl;
        return nullptr;
    }

    // Устанавливаем callback для получения данных
    newConn->setOnCandleCallback([this](const CandleData& candle) {
        onCandleReceived(candle);
        });

    WebSocketConnection* ptr = newConn.get();
    connections.push_back(std::move(newConn));

    std::cout << "[WebSocketManager] Created new connection for " << category
        << " (total connections: " << connections.size() << ")" << std::endl;

    return ptr;
}

void BybitWebSocketManager::cleanupEmptyConnections() {
    std::vector<std::string> categoriesToRemove;

    for (auto& item : m_connections) {
        auto& category = item.first;
        auto& connections = item.second;

        // Удаляем пустые соединения в этой категории
        auto initialSize = connections.size();

        connections.erase(
            std::remove_if(connections.begin(), connections.end(),
                [](const std::unique_ptr<WebSocketConnection>& conn) {
                    bool empty = conn->getSubscriptionCount() == 0;
                    if (empty) {
                        std::cout << "[WebSocketManager] Removing empty connection" << std::endl;
                    }
                    return empty;
                }),
            connections.end()
        );

        if (connections.size() < initialSize) {
            std::cout << "[WebSocketManager] Cleaned up "
                << (initialSize - connections.size())
                << " empty connections in " << category << std::endl;
        }

        // Если в категории больше нет соединений — помечаем для удаления
        if (connections.empty()) {
            categoriesToRemove.push_back(category);
        }
    }


    // Удаляем пустые категории
    for (const auto& category : categoriesToRemove) {
        m_connections.erase(category);
        std::cout << "[WebSocketManager] Removed empty category: " << category << std::endl;
    }
}

std::string BybitWebSocketManager::getWebSocketUrl(const std::string& category) const {
    if (category == "spot") return WS_SPOT_URL;
    if (category == "linear") return WS_LINEAR_URL;
    if (category == "inverse") return WS_INVERSE_URL;
    if (category == "option") return WS_OPTION_URL;
    return WS_SPOT_URL;
}

void BybitWebSocketManager::onCandleReceived(const CandleData& candle) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Формируем ключ подписки
    std::string subKey = candle.category + ":" + candle.symbol + "." + candle.interval;

    // Находим всех подписчиков
    auto it = m_subscribers.find(subKey);
    if (it == m_subscribers.end()) {
        return; // Нет подписчиков
    }

    // Отправляем данные всем подписчикам
    for (BybitPriceCollector* collector : it->second) {
        if (collector) {
            collector->onCandleReceived(candle);
        }
    }
}