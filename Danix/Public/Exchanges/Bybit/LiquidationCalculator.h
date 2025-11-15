#pragma once

#include "BybitAPI.h"
#include "BybitTypes.h"

namespace Bybit {

class LiquidationCalculator : public BybitAPI {
public:
    LiquidationCalculator();
    virtual ~LiquidationCalculator();
    
    RiskParams getRiskParams(const std::string& symbol, double positionValue = 0.0);
    
    bool getFeeRate(const std::string& symbol, double& takerFee, double& makerFee);
    
    LiquidationResult calculateLiquidationPrice(
        double entryPrice,
        double leverage,
        bool isLong,
        const RiskParams& params
    );
    
    LiquidationResult calculateLiquidationWithSize(
        double entryPrice,
        double quantity,
        double leverage,
        bool isLong,
        const std::string& symbol
    );
    
    double quickCalculate(double entryPrice, double leverage, bool isLong);
    
private:
    RiskParams parseRiskLimit(const std::string& response, double positionValue);
    
    bool parseFeeRate(const std::string& response, double& takerFee, double& makerFee);
    
    double calculateMMRForPosition(const RiskParams& params, double positionValue);
};

}   
