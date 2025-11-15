#pragma once

#include <string>
#include <vector>

namespace Bybit {

enum class OrderType {
    LIMIT,
    MARKET
};

enum class OrderSide {
    BUY,
    SELL
};

enum class Category {
    SPOT,       
    LINEAR,      
    INVERSE      
};

enum class TradeMode {
    CROSS = 0,      
    ISOLATED = 1     
};

enum class TimeInForce {
    GTC,     
    IOC,     
    FOK      
};

enum class OrderStatus {
    NEW,
    PARTIALLY_FILLED,
    FILLED,
    CANCELLED,
    REJECTED
};

struct RiskParams {
    double mmr;                 
    double takerFee;           
    double makerFee;           
    double insuranceFund;       
    double riskLimitValue;       
    double maxLeverage;        
    
    RiskParams() 
        : mmr(0.005)
        , takerFee(0.00055)
        , makerFee(-0.00025)
        , insuranceFund(0.001)
        , riskLimitValue(2000000.0)
        , maxLeverage(100.0)
    {}
};

struct PositionInfo {
    std::string symbol;            
    std::string side;                
    double size;                   
    double positionValue;          
    double avgPrice;               
    double markPrice;              
    double liqPrice;               
    double liqPriceByMp;           
    double bustPrice;                
    double leverage;               
    int autoAddMargin;             
    double positionIM;             
    double positionIMByMp;         
    double positionMM;             
    double positionMMByMp;         
    double unrealisedPnl;          
    double cumRealisedPnl;         
    double curRealisedPnl;         
    double positionBalance;        
    std::string tpslMode;          
    double stopLoss;               
    double takeProfit;             
    double trailingStop;           
    std::string positionStatus;    
    int tradeMode;                  
    int riskId;                    
    double riskLimitValue;         
    int adlRankIndicator;          
    bool isReduceOnly;             
    std::string tpTriggerBy;       
    std::string slTriggerBy;       
    int positionIdx;               
    uint64_t createdTime;       
    uint64_t updatedTime;       
    long long seq;                 
    std::string leverageSysUpdatedTime;   
    std::string mmrSysUpdatedTime;        
    double sessionAvgPrice;               

    PositionInfo()
        : size(0.0), positionValue(0.0), avgPrice(0.0), markPrice(0.0)
        , liqPrice(0.0), liqPriceByMp(0.0), bustPrice(0.0), leverage(1.0)
        , autoAddMargin(0), positionIM(0.0), positionIMByMp(0.0)
        , positionMM(0.0), positionMMByMp(0.0), unrealisedPnl(0.0)
        , cumRealisedPnl(0.0), curRealisedPnl(0.0), positionBalance(0.0)
        , stopLoss(0.0), takeProfit(0.0), trailingStop(0.0)
        , tradeMode(0), riskId(1), riskLimitValue(0.0)
        , adlRankIndicator(0), isReduceOnly(false), positionIdx(0)
        , seq(0), sessionAvgPrice(0.0)
    {
    }
};


struct OrderResult {
    bool success;
    std::string orderId;
    std::string orderLinkId;
    std::string errorMessage;
    int errorCode;
    
    OrderResult() 
        : success(false)
        , errorCode(0)
    {}
};



struct OpenOrder {
    std::string orderId;
    std::string orderLinkId;
    std::string symbol;
    std::string side;                 
    std::string orderType;           
    double price;
    double qty;
    double cumExecQty;
    double cumExecValue;
    double cumExecFee;
    std::string timeInForce;
    std::string orderStatus;
    double avgPrice;
    double leavesQty;
    double leavesValue;
    std::string stopOrderType;
    std::string orderIv;
    double triggerPrice;
    double takeProfit;
    double stopLoss;
    std::string tpTriggerBy;
    std::string slTriggerBy;
    int triggerDirection;
    std::string triggerBy;
    bool reduceOnly;
    bool closeOnTrigger;
    std::string placeType;
    std::string createdTime;
    std::string updatedTime;
    
    OpenOrder() 
        : price(0.0), qty(0.0), cumExecQty(0.0), cumExecValue(0.0), cumExecFee(0.0)
        , avgPrice(0.0), leavesQty(0.0), leavesValue(0.0), triggerPrice(0.0)
        , takeProfit(0.0), stopLoss(0.0), triggerDirection(0)
        , reduceOnly(false), closeOnTrigger(false)
    {}
};

struct OpenOrdersResult {
    bool success;
    int retCode;
    std::string retMsg;
    std::vector<OpenOrder> orders;
    std::string nextPageCursor;      
    std::string category;

    OpenOrdersResult()
        : success(false)
        , retCode(-1)
    {
    }
};

struct Balance {
    std::string coin;
    double total;             
    double available;         
    double locked;            
    
    Balance() 
        : total(0.0)
        , available(0.0)
        , locked(0.0)
    {}
};

struct LiquidationResult {
    double liquidationPrice;       
    double bankruptcyPrice;        
    double distancePercent;           
    double maintenanceMargin;       
    double initialMargin;          
    double availableDrawdown;      
    
    LiquidationResult() 
        : liquidationPrice(0.0)
        , bankruptcyPrice(0.0)
        , distancePercent(0.0)
        , maintenanceMargin(0.0)
        , initialMargin(0.0)
        , availableDrawdown(0.0)
    {}
};

struct Execution {
    std::string symbol;
    std::string orderId;
    std::string orderLinkId;
    std::string side;                 
    std::string orderType;            
    double execPrice;                
    double execQty;                 
    double execValue;               
    double execFee;                 
    std::string feeRate;
    std::string execTime;            
    bool isMaker;                        
    std::string execType;             
    std::string execId;
    long long seq;

    Execution()
        : execPrice(0.0)
        , execQty(0.0)
        , execValue(0.0)
        , execFee(0.0)
        , isMaker(false)
        , seq(0)
    {
    }
};

struct ExecutionResult {
    bool success;
    int retCode;
    std::string retMsg;
    std::vector<Execution> executions;
    std::string nextPageCursor;
    std::string category;

    ExecutionResult()
        : success(false)
        , retCode(-1)
    {
    }
};

struct OrderHistory {
    std::string orderId;
    std::string orderLinkId;
    std::string blockTradeId;
    std::string symbol;
    double price;
    double qty;
    std::string side;                 
    std::string isLeverage;
    int positionIdx;
    std::string orderStatus;          
    std::string cancelType;
    std::string rejectReason;
    double avgPrice;                  
    double leavesQty;
    double leavesValue;
    double cumExecQty;               
    double cumExecValue;             
    double cumExecFee;              
    std::string timeInForce;
    std::string orderType;            
    std::string stopOrderType;
    std::string orderIv;
    double triggerPrice;
    double takeProfit;
    double stopLoss;
    std::string tpTriggerBy;
    std::string slTriggerBy;
    int triggerDirection;
    std::string triggerBy;
    double lastPriceOnCreated;
    bool reduceOnly;
    bool closeOnTrigger;
    std::string smpType;
    int smpGroup;
    std::string smpOrderId;
    std::string tpslMode;
    double tpLimitPrice;
    double slLimitPrice;
    std::string placeType;
    std::string createdTime;
    std::string updatedTime;

    OrderHistory()
        : price(0.0), qty(0.0), positionIdx(0), avgPrice(0.0)
        , leavesQty(0.0), leavesValue(0.0), cumExecQty(0.0)
        , cumExecValue(0.0), cumExecFee(0.0), triggerPrice(0.0)
        , takeProfit(0.0), stopLoss(0.0), triggerDirection(0)
        , lastPriceOnCreated(0.0), reduceOnly(false), closeOnTrigger(false)
        , smpGroup(0), tpLimitPrice(0.0), slLimitPrice(0.0)
    {
    }
};

struct OrderHistoryResult {
    bool success;
    int retCode;
    std::string retMsg;
    std::vector<OrderHistory> orders;
    std::string nextPageCursor;
    std::string category;

    OrderHistoryResult()
        : success(false)
        , retCode(-1)
    {
    }
};

}   
