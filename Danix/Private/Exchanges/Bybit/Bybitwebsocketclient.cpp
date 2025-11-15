#include "../../../Public/Exchanges/Bybit/Bybitwebsocketclient.h"
#include <iostream>

BybitWebSocketClient::BybitWebSocketClient()
    : m_isConnected(false)
    , m_shouldStop(false) {

    // Инициализация клиента
    m_client.clear_access_channels(websocketpp::log::alevel::all);
    m_client.clear_error_channels(websocketpp::log::elevel::all);

    m_client.init_asio();
    m_client.start_perpetual();

    // Установка callback'ов
    m_client.set_open_handler([this](websocketpp::connection_hdl hdl) {
        onOpen(hdl);
        });

    m_client.set_close_handler([this](websocketpp::connection_hdl hdl) {
        onClose(hdl);
        });

    m_client.set_fail_handler([this](websocketpp::connection_hdl hdl) {
        onFail(hdl);
        });

    m_client.set_message_handler([this](websocketpp::connection_hdl hdl, WebSocketClient::message_ptr msg) {
        onMessage(hdl, msg);
        });

    m_client.set_tls_init_handler([this](websocketpp::connection_hdl hdl) {
        return onTlsInit(hdl);
        });
}

BybitWebSocketClient::~BybitWebSocketClient() {
    disconnect();
}

bool BybitWebSocketClient::connect(const std::string& url) {
    if (m_isConnected.load()) {
        std::cerr << "Already connected" << std::endl;
        return false;
    }

    m_currentUrl = url;
    m_shouldStop = false;

    try {
        websocketpp::lib::error_code ec;
        WebSocketClient::connection_ptr con = m_client.get_connection(url, ec);

        if (ec) {
            if (m_onError) {
                m_onError("Connection error: " + ec.message());
            }
            return false;
        }

        m_hdl = con->get_handle();
        m_client.connect(con);

        // Запустить поток обработки событий
        m_thread = std::thread(&BybitWebSocketClient::runEventLoop, this);

        return true;

    }
    catch (const std::exception& e) {
        if (m_onError) {
            m_onError(std::string("Connection exception: ") + e.what());
        }
        return false;
    }
}

void BybitWebSocketClient::disconnect() {
    m_shouldStop = true;

    if (m_isConnected.load()) {
        try {
            websocketpp::lib::error_code ec;
            m_client.close(m_hdl, websocketpp::close::status::normal, "Closing connection", ec);

            if (ec) {
                std::cerr << "Error closing connection: " << ec.message() << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Exception during disconnect: " << e.what() << std::endl;
        }
    }

    m_client.stop_perpetual();

    if (m_thread.joinable()) {
        m_thread.join();
    }

    m_isConnected = false;
}

bool BybitWebSocketClient::subscribeToKline(const std::string& symbol, const std::string& interval) {
    if (!m_isConnected.load()) {
        if (m_onError) {
            m_onError("Not connected to WebSocket");
        }
        return false;
    }

    try {
        json subscribeMsg = {
            {"op", "subscribe"},
            {"args", {
                "kline." + interval + "." + symbol
            }}
        };

        return sendMessage(subscribeMsg.dump());

    }
    catch (const std::exception& e) {
        if (m_onError) {
            m_onError(std::string("Subscribe error: ") + e.what());
        }
        return false;
    }
}

bool BybitWebSocketClient::unsubscribe(const std::string& topic) {
    if (!m_isConnected.load()) {
        return false;
    }

    try {
        json unsubscribeMsg = {
            {"op", "unsubscribe"},
            {"args", {topic}}
        };

        return sendMessage(unsubscribeMsg.dump());

    }
    catch (const std::exception& e) {
        if (m_onError) {
            m_onError(std::string("Unsubscribe error: ") + e.what());
        }
        return false;
    }
}

void BybitWebSocketClient::setOnMessageCallback(std::function<void(const std::string&)> callback) {
    m_onMessage = callback;
}

void BybitWebSocketClient::setOnErrorCallback(std::function<void(const std::string&)> callback) {
    m_onError = callback;
}

void BybitWebSocketClient::setOnConnectedCallback(std::function<void()> callback) {
    m_onConnected = callback;
}

void BybitWebSocketClient::setOnDisconnectedCallback(std::function<void()> callback) {
    m_onDisconnected = callback;
}

void BybitWebSocketClient::onOpen(websocketpp::connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isConnected = true;
    m_hdl = hdl;

    std::cout << "WebSocket connected!" << std::endl;

    if (m_onConnected) {
        m_onConnected();
    }
}

void BybitWebSocketClient::onClose(websocketpp::connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isConnected = false;

    std::cout << "WebSocket disconnected" << std::endl;

    if (m_onDisconnected) {
        m_onDisconnected();
    }
}

void BybitWebSocketClient::onFail(websocketpp::connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isConnected = false;

    std::cout << "WebSocket connection failed" << std::endl;

    if (m_onError) {
        m_onError("Connection failed");
    }

    if (m_onDisconnected) {
        m_onDisconnected();
    }
}

void BybitWebSocketClient::onMessage(websocketpp::connection_hdl hdl, WebSocketClient::message_ptr msg) {
    const std::string& payload = msg->get_payload();

    try {
        // Парсим JSON для проверки типа сообщения
        json data = json::parse(payload);

        // Проверяем, что это не служебное сообщение (ping/pong, subscribe confirmation)
        if (data.contains("op")) {
            std::string op = data["op"];

            if (op == "subscribe") {
                std::cout << "Subscription confirmed" << std::endl;
                return;
            }
            else if (op == "pong") {
                // Ответ на ping - игнорируем
                return;
            }
        }

        // Передаём сообщение в callback
        if (m_onMessage) {
            m_onMessage(payload);
        }

    }
    catch (const std::exception& e) {
        if (m_onError) {
            m_onError(std::string("Message parse error: ") + e.what());
        }
    }
}

ContextPtr BybitWebSocketClient::onTlsInit(websocketpp::connection_hdl hdl) {
    ContextPtr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(
        boost::asio::ssl::context::tlsv12_client
    );

    try {
        ctx->set_options(
            boost::asio::ssl::context::default_workarounds |
            boost::asio::ssl::context::no_sslv2 |
            boost::asio::ssl::context::no_sslv3 |
            boost::asio::ssl::context::single_dh_use
        );

    }
    catch (const std::exception& e) {
        std::cerr << "TLS init error: " << e.what() << std::endl;
    }

    return ctx;
}

bool BybitWebSocketClient::sendMessage(const std::string& message) {
    if (!m_isConnected.load()) {
        return false;
    }

    try {
        std::lock_guard<std::mutex> lock(m_mutex);
        websocketpp::lib::error_code ec;

        m_client.send(m_hdl, message, websocketpp::frame::opcode::text, ec);

        if (ec) {
            if (m_onError) {
                m_onError("Send error: " + ec.message());
            }
            return false;
        }

        return true;

    }
    catch (const std::exception& e) {
        if (m_onError) {
            m_onError(std::string("Send exception: ") + e.what());
        }
        return false;
    }
}

void BybitWebSocketClient::runEventLoop() {
    try {
        m_client.run();
    }
    catch (const std::exception& e) {
        std::cerr << "WebSocket event loop error: " << e.what() << std::endl;
        if (m_onError) {
            m_onError(std::string("Event loop error: ") + e.what());
        }
    }
}