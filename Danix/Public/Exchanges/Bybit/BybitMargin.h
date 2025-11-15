#pragma once

#include "BybitAPI.h"
#include "BybitTypes.h"
#include <vector>

namespace Bybit {

class BybitMargin : public BybitAPI {
public:
    std::string category = "linear";
    BybitMargin();
    virtual ~BybitMargin();
    
    bool setLeverage(const std::string& symbol, double leverage);
    
    bool setMarginMode(const std::string& symbol, TradeMode mode);
    
    OrderResult openLong(
        const std::string& symbol,
        double quantity,
        double price = 0.0,      
        bool reduceOnly = false
    );
    
    OrderResult openShort(
        const std::string& symbol,
        double quantity,
        double price = 0.0,
        bool reduceOnly = false
    );
    
    OrderResult closePosition(
        const std::string& symbol,
        double quantity = 0.0       
    );
    
    bool setStopLoss(const std::string& symbol, double stopLoss, double quantity = 0.0);
    
    bool setTakeProfit(const std::string& symbol, double takeProfit, double quantity = 0.0);
    
    PositionInfo getPosition(const std::string& symbol);


    
    std::vector<PositionInfo> getAllPositions();
    
    std::vector<Balance> getBalance();

    ExecutionResult getExecutions(
        const std::string& category,         
        const std::string& symbol = "",    
        const std::string& orderId = "",    
        int limit = 50                     
    );
    
    bool cancelOrder(const std::string& symbol, const std::string& orderId);
    
    OpenOrdersResult getOpenOrders(const std::string& symbol = "",
    const std::string& baseCoin = "",
    const std::string& settleCoin = "",
    const std::string& orderId = "",
    const std::string& orderLinkId = "",
    int openOnly = 0,
    const std::string& orderFilter = "",
    int limit = 20,
    const std::string& cursor = "");

    OrderHistoryResult getOrderHistory(const std::string& category = "linear",    
        const std::string& symbol = "BTCUSDT",    
        const std::string& orderId = "0",    
        int limit = 50                  
    );

private:
    OrderResult placeOrder(
        const std::string& symbol,
        OrderSide side,
        OrderType orderType,
        double quantity,
        double price = 0.0,
        bool reduceOnly = false
    );
    
    OrderResult parseOrderResult(const std::string& response);
    
    PositionInfo parsePosition(const std::string& response);
    
    std::vector<PositionInfo> parsePositions(const std::string& response);

    OpenOrdersResult parseOpenOrders(const std::string& jsonResponse);
    
    std::vector<Balance> parseBalances(const std::string& response);
    ExecutionResult parseExecutions(const std::string& jsonResponse);
    OrderHistoryResult parseOrderHistory(const std::string& jsonResponse);
};

}   
