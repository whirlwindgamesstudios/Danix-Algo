#include "../../../Public/Exchanges/Bybit/LiquidationCalculator.h"
#include <nlohmann/json.hpp>
#include <cmath>

using json = nlohmann::json;

namespace Bybit {

LiquidationCalculator::LiquidationCalculator() {
}

LiquidationCalculator::~LiquidationCalculator() {
}

RiskParams LiquidationCalculator::getRiskParams(const std::string& symbol, double positionValue) {
    std::map<std::string, std::string> params;
    params["category"] = "linear";
    params["symbol"] = symbol;
    
    std::string response = get("/v5/market/risk-limit", params, false);
    return parseRiskLimit(response, positionValue);
}

bool LiquidationCalculator::getFeeRate(const std::string& symbol, double& takerFee, double& makerFee) {
    std::map<std::string, std::string> params;
    params["category"] = "linear";
    params["symbol"] = symbol;
    
    std::string response = get("/v5/account/fee-rate", params, true);
    return parseFeeRate(response, takerFee, makerFee);
}

LiquidationResult LiquidationCalculator::calculateLiquidationPrice(
    double entryPrice,
    double leverage,
    bool isLong,
    const RiskParams& params
) {
    LiquidationResult result;
    
    // Начальная маржа = 1 / плечо
    result.initialMargin = 1.0 / leverage;
    
    // Поддерживающая маржа
    result.maintenanceMargin = params.mmr;
    
    // Доступная просадка = Initial Margin - Maintenance Margin - Fees - Insurance
    result.availableDrawdown = result.initialMargin 
                              - params.mmr 
                              - params.takerFee 
                              - params.insuranceFund;
    
    // Цена ликвидации
    if (isLong) {
        // Лонг: цена падает
        result.liquidationPrice = entryPrice * (1.0 - result.availableDrawdown);
        // Цена банкротства (без страхового фонда)
        result.bankruptcyPrice = entryPrice * (1.0 - (result.initialMargin - params.mmr - params.takerFee));
    } else {
        // Шорт: цена растет
        result.liquidationPrice = entryPrice * (1.0 + result.availableDrawdown);
        // Цена банкротства
        result.bankruptcyPrice = entryPrice * (1.0 + (result.initialMargin - params.mmr - params.takerFee));
    }
    
    // Расстояние до ликвидации в процентах
    result.distancePercent = std::abs((entryPrice - result.liquidationPrice) / entryPrice * 100.0);
    
    return result;
}

LiquidationResult LiquidationCalculator::calculateLiquidationWithSize(
    double entryPrice,
    double quantity,
    double leverage,
    bool isLong,
    const std::string& symbol
) {
    // Рассчитываем стоимость позиции
    double positionValue = entryPrice * quantity;
    
    // Получаем параметры риска с учетом размера позиции
    RiskParams params = getRiskParams(symbol, positionValue);
    
    // Получаем актуальные комиссии
    double takerFee, makerFee;
    if (getFeeRate(symbol, takerFee, makerFee)) {
        params.takerFee = takerFee;
        params.makerFee = makerFee;
    }
    
    // Рассчитываем ликвидацию
    return calculateLiquidationPrice(entryPrice, leverage, isLong, params);
}

double LiquidationCalculator::quickCalculate(double entryPrice, double leverage, bool isLong) {
    // Упрощенный расчет с дефолтными параметрами
    RiskParams defaultParams;  // используем дефолтные значения из конструктора
    
    LiquidationResult result = calculateLiquidationPrice(entryPrice, leverage, isLong, defaultParams);
    return result.liquidationPrice;
}

RiskParams LiquidationCalculator::parseRiskLimit(const std::string& response, double positionValue) {
    RiskParams params;  // дефолтные значения
    
    try {
        json j = json::parse(response);
        
        if (j.contains("result") && j["result"].contains("list")) {
            const auto& list = j["result"]["list"];
            
            if (positionValue > 0.0) {
                // Ищем подходящий уровень риска для данного размера позиции
                for (const auto& level : list) {
                    double riskLimitValue = std::stod(level.value("riskLimitValue", "0"));
                    
                    if (positionValue <= riskLimitValue) {
                        params.mmr = std::stod(level.value("maintenanceMargin", "0.005"));
                        params.riskLimitValue = riskLimitValue;
                        params.maxLeverage = std::stod(level.value("maxLeverage", "100"));
                        break;
                    }
                }
                
                // Если позиция больше всех лимитов - берем последний уровень
                if (params.riskLimitValue == 2000000.0 && !list.empty()) {
                    const auto& lastLevel = list[list.size() - 1];
                    params.mmr = std::stod(lastLevel.value("maintenanceMargin", "0.01"));
                    params.riskLimitValue = std::stod(lastLevel.value("riskLimitValue", "2000000"));
                    params.maxLeverage = std::stod(lastLevel.value("maxLeverage", "50"));
                }
            } else {
                // Если размер не указан - берем первый (минимальный) уровень
                if (!list.empty()) {
                    const auto& firstLevel = list[0];
                    params.mmr = std::stod(firstLevel.value("maintenanceMargin", "0.005"));
                    params.riskLimitValue = std::stod(firstLevel.value("riskLimitValue", "2000000"));
                    params.maxLeverage = std::stod(firstLevel.value("maxLeverage", "100"));
                }
            }
        }
        
    } catch (const json::exception& e) {
        // Используем дефолтные значения
    } catch (const std::exception& e) {
        // Используем дефолтные значения
    }
    
    return params;
}

bool LiquidationCalculator::parseFeeRate(const std::string& response, double& takerFee, double& makerFee) {
    try {
        json j = json::parse(response);
        
        if (j.contains("result") && j["result"].contains("list") && !j["result"]["list"].empty()) {
            const auto& feeInfo = j["result"]["list"][0];
            
            takerFee = std::stod(feeInfo.value("takerFeeRate", "0.00055"));
            makerFee = std::stod(feeInfo.value("makerFeeRate", "-0.00025"));
            
            return true;
        }
        
    } catch (const json::exception& e) {
    } catch (const std::exception& e) {
    }
    
    return false;
}

double LiquidationCalculator::calculateMMRForPosition(const RiskParams& params, double positionValue) {
    // Эта функция используется если нужно динамически рассчитать MMR
    // В текущей реализации MMR уже определен в parseRiskLimit
    return params.mmr;
}

} // namespace Bybit
