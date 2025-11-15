#pragma once

#include "BybitAPI.h"
#include "BybitTypes.h"
#include <vector>

namespace Bybit {

class BybitSpot : public BybitAPI {
public:
    BybitSpot();
    virtual ~BybitSpot();
    
    OrderResult buyLimit(const std::string& symbol, double quantity, double price);
    
    OrderResult sellLimit(const std::string& symbol, double quantity, double price);
    
    OrderResult buyMarket(const std::string& symbol, double quantity);
    
    OrderResult sellMarket(const std::string& symbol, double quantity);
    
    bool cancelOrder(const std::string& symbol, const std::string& orderId);
    
    std::string getOrder(const std::string& symbol, const std::string& orderId);
    
    std::string getOpenOrders(const std::string& symbol = "");
    
    std::vector<Balance> getBalance();
    
    Balance getCoinBalance(const std::string& coin);
    
private:
    OrderResult placeOrder(
        const std::string& symbol,
        OrderSide side,
        OrderType orderType,
        double quantity,
        double price = 0.0,
        TimeInForce timeInForce = TimeInForce::GTC
    );
    
    OrderResult parseOrderResult(const std::string& response);
    
    std::vector<Balance> parseBalances(const std::string& response);
};

}   
