#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <mutex>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

typedef websocketpp::client<websocketpp::config::asio_tls_client> WebSocketClient;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> ContextPtr;

class BybitWebSocketClient {
public:
    BybitWebSocketClient();
    ~BybitWebSocketClient();

    bool connect(const std::string& url);

    void disconnect();

    bool subscribeToKline(const std::string& symbol, const std::string& interval);

    bool unsubscribe(const std::string& topic);

    bool isConnected() const { return m_isConnected.load(); }

    void setOnMessageCallback(std::function<void(const std::string&)> callback);
    void setOnErrorCallback(std::function<void(const std::string&)> callback);
    void setOnConnectedCallback(std::function<void()> callback);
    void setOnDisconnectedCallback(std::function<void()> callback);

private:
    void onOpen(websocketpp::connection_hdl hdl);
    void onClose(websocketpp::connection_hdl hdl);
    void onFail(websocketpp::connection_hdl hdl);
    void onMessage(websocketpp::connection_hdl hdl, WebSocketClient::message_ptr msg);

    ContextPtr onTlsInit(websocketpp::connection_hdl hdl);

    bool sendMessage(const std::string& message);

    void runEventLoop();

private:
    WebSocketClient m_client;
    websocketpp::connection_hdl m_hdl;
    std::thread m_thread;

    std::atomic<bool> m_isConnected;
    std::atomic<bool> m_shouldStop;

    std::mutex m_mutex;

    std::function<void(const std::string&)> m_onMessage;
    std::function<void(const std::string&)> m_onError;
    std::function<void()> m_onConnected;
    std::function<void()> m_onDisconnected;

    std::string m_currentUrl;
};