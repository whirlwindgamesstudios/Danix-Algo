#include "../../../Public/Exchanges/Bybit/BybitSpot.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>

using json = nlohmann::json;

namespace Bybit {

BybitSpot::BybitSpot() {
}

BybitSpot::~BybitSpot() {
}

OrderResult BybitSpot::buyLimit(const std::string& symbol, double quantity, double price) {
    return placeOrder(symbol, OrderSide::BUY, OrderType::LIMIT, quantity, price);
}

OrderResult BybitSpot::sellLimit(const std::string& symbol, double quantity, double price) {
    return placeOrder(symbol, OrderSide::SELL, OrderType::LIMIT, quantity, price);
}

OrderResult BybitSpot::buyMarket(const std::string& symbol, double quantity) {
    return placeOrder(symbol, OrderSide::BUY, OrderType::MARKET, quantity);
}

OrderResult BybitSpot::sellMarket(const std::string& symbol, double quantity) {
    return placeOrder(symbol, OrderSide::SELL, OrderType::MARKET, quantity);
}

OrderResult BybitSpot::placeOrder(
    const std::string& symbol,
    OrderSide side,
    OrderType orderType,
    double quantity,
    double price,
    TimeInForce timeInForce
) {
    std::ostringstream body;
    body << "{";
    body << "\"category\":\"spot\",";
    body << "\"symbol\":\"" << symbol << "\",";
    body << "\"side\":\"" << (side == OrderSide::BUY ? "Buy" : "Sell") << "\",";
    body << "\"orderType\":\"" << (orderType == OrderType::LIMIT ? "Limit" : "Market") << "\",";
    body << "\"qty\":\"" << std::fixed << std::setprecision(8) << quantity << "\"";
    
    if (orderType == OrderType::LIMIT && price > 0.0) {
        body << ",\"price\":\"" << std::fixed << std::setprecision(2) << price << "\"";
    }
    
    // TimeInForce
    std::string tif;
    switch (timeInForce) {
        case TimeInForce::GTC: tif = "GTC"; break;
        case TimeInForce::IOC: tif = "IOC"; break;
        case TimeInForce::FOK: tif = "FOK"; break;
    }
    body << ",\"timeInForce\":\"" << tif << "\"";
    
    body << "}";
    
    std::string response = post("/v5/order/create", body.str(), true);
    return parseOrderResult(response);
}

bool BybitSpot::cancelOrder(const std::string& symbol, const std::string& orderId) {
    std::ostringstream body;
    body << "{";
    body << "\"category\":\"spot\",";
    body << "\"symbol\":\"" << symbol << "\",";
    body << "\"orderId\":\"" << orderId << "\"";
    body << "}";
    
    std::string response = post("/v5/order/cancel", body.str(), true);
    
    // Простая проверка успеха
    return response.find("\"retCode\":0") != std::string::npos;
}

std::string BybitSpot::getOrder(const std::string& symbol, const std::string& orderId) {
    std::map<std::string, std::string> params;
    params["category"] = "spot";
    params["symbol"] = symbol;
    params["orderId"] = orderId;
    
    return get("/v5/order/realtime", params, true);
}

std::string BybitSpot::getOpenOrders(const std::string& symbol) {
    std::map<std::string, std::string> params;
    params["category"] = "spot";
    
    if (!symbol.empty()) {
        params["symbol"] = symbol;
    }
    
    return get("/v5/order/realtime", params, true);
}

std::vector<Balance> BybitSpot::getBalance() {
    std::map<std::string, std::string> params;
    params["accountType"] = "SPOT";
    
    std::string response = get("/v5/account/wallet-balance", params, true);
    return parseBalances(response);
}

Balance BybitSpot::getCoinBalance(const std::string& coin) {
    std::vector<Balance> balances = getBalance();
    
    for (const auto& balance : balances) {
        if (balance.coin == coin) {
            return balance;
        }
    }
    
    // Если не найден - возвращаем пустой баланс
    Balance empty;
    empty.coin = coin;
    return empty;
}

// Парсинг JSON с nlohmann/json
OrderResult BybitSpot::parseOrderResult(const std::string& response) {
    OrderResult result;
    
    try {
        json j = json::parse(response);
        
        result.errorCode = j.value("retCode", -1);
        result.success = (result.errorCode == 0);
        
        if (!result.success) {
            result.errorMessage = j.value("retMsg", "Unknown error");
            return result;
        }
        
        if (j.contains("result")) {
            auto resultObj = j["result"];
            result.orderId = resultObj.value("orderId", "");
            result.orderLinkId = resultObj.value("orderLinkId", "");
        }
        
    } catch (const json::exception& e) {
        result.success = false;
        result.errorMessage = std::string("JSON parse error: ") + e.what();
    }
    
    return result;
}

std::vector<Balance> BybitSpot::parseBalances(const std::string& response) {
    std::vector<Balance> balances;
    
    try {
        json j = json::parse(response);
        
        if (j.contains("result") && j["result"].contains("list")) {
            for (const auto& accountItem : j["result"]["list"]) {
                if (accountItem.contains("coin")) {
                    for (const auto& coinItem : accountItem["coin"]) {
                        Balance balance;
                        balance.coin = coinItem.value("coin", "");
                        
                        std::string totalStr = coinItem.value("walletBalance", "0");
                        std::string availStr = coinItem.value("availableToWithdraw", "0");
                        
                        balance.total = std::stod(totalStr);
                        balance.available = std::stod(availStr);
                        balance.locked = balance.total - balance.available;
                        
                        balances.push_back(balance);
                    }
                }
            }
        }
        
    } catch (const json::exception& e) {
        // Логируем ошибку, возвращаем пустой вектор
    } catch (const std::exception& e) {
        // Ошибка преобразования строки в число
    }
    
    return balances;
}

} // namespace Bybit
