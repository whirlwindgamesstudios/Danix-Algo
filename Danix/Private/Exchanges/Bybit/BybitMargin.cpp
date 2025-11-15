#include "../../../Public/Exchanges/Bybit/BybitMargin.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>


using json = nlohmann::json;

namespace Bybit {

BybitMargin::BybitMargin() {
}

BybitMargin::~BybitMargin() {
}

bool BybitMargin::setLeverage(const std::string& symbol, double leverage) {
    json j;
    j["category"] = "linear";
    j["symbol"] = symbol;
    
    std::ostringstream leverageStream;
    leverageStream << std::fixed << std::setprecision(2) << leverage;
    std::string leverageStr = leverageStream.str();
    
    j["buyLeverage"] = leverageStr;
    j["sellLeverage"] = leverageStr;
    
    std::string response = post("/v5/position/set-leverage", j.dump(), true);
    return response.find("\"retCode\":0") != std::string::npos;
}

bool BybitMargin::setMarginMode(const std::string& symbol, TradeMode mode) {
    json j;
    j["category"] = "linear";
    j["symbol"] = symbol;
    j["tradeMode"] = static_cast<int>(mode);
    
    std::string response = post("/v5/position/switch-isolated", j.dump(), true);
    return response.find("\"retCode\":0") != std::string::npos;
}

OrderResult BybitMargin::openLong(const std::string& symbol, double quantity, double price, bool reduceOnly) {
    OrderType type = (price > 0.0) ? OrderType::LIMIT : OrderType::MARKET;
    return placeOrder(symbol, OrderSide::BUY, type, quantity, price, reduceOnly);
}

OrderResult BybitMargin::openShort(const std::string& symbol, double quantity, double price, bool reduceOnly) {
    OrderType type = (price > 0.0) ? OrderType::LIMIT : OrderType::MARKET;
    return placeOrder(symbol, OrderSide::SELL, type, quantity, price, reduceOnly);
}

OrderResult BybitMargin::closePosition(const std::string& symbol, double quantity) {
    PositionInfo pos = getPosition(symbol);
    
    if (pos.size == 0.0) {
        OrderResult result;
        result.success = false;
        result.errorMessage = "No open position";
        return result;
    }
    
    OrderSide closeSide = (pos.side == "Buy") ? OrderSide::SELL : OrderSide::BUY;
    double closeQty = (quantity > 0.0) ? quantity : pos.size;
    
    return placeOrder(symbol, closeSide, OrderType::MARKET, closeQty, 0.0, true);
}

bool BybitMargin::setStopLoss(const std::string& symbol, double stopLoss, double quantity) {
    json j;
    j["category"] = "linear";
    j["symbol"] = symbol;
    
    std::ostringstream slStream;
    slStream << std::fixed << std::setprecision(2) << stopLoss;
    j["stopLoss"] = slStream.str();
    
    if (quantity > 0.0) {
        std::ostringstream qtyStream;
        qtyStream << std::fixed << std::setprecision(8) << quantity;
        j["slSize"] = qtyStream.str();
    }
    
    std::string response = post("/v5/position/trading-stop", j.dump(), true);
    return response.find("\"retCode\":0") != std::string::npos;
}

bool BybitMargin::setTakeProfit(const std::string& symbol, double takeProfit, double quantity) {
    json j;
    j["category"] = "linear";
    j["symbol"] = symbol;
    
    std::ostringstream tpStream;
    tpStream << std::fixed << std::setprecision(2) << takeProfit;
    j["takeProfit"] = tpStream.str();
    
    if (quantity > 0.0) {
        std::ostringstream qtyStream;
        qtyStream << std::fixed << std::setprecision(8) << quantity;
        j["tpSize"] = qtyStream.str();
    }
    
    std::string response = post("/v5/position/trading-stop", j.dump(), true);
    return response.find("\"retCode\":0") != std::string::npos;
}

PositionInfo BybitMargin::getPosition(const std::string& symbol) {
    std::map<std::string, std::string> params;
    params["symbol"] = symbol;
    params["category"] = "linear";
    
    std::string response = get("/v5/position/list", params, true);
    return parsePosition(response);
}

std::vector<PositionInfo> BybitMargin::getAllPositions() {
    std::map<std::string, std::string> params;
    params["category"] = "linear";
    
    std::string response = get("/v5/position/list", params, true);
    return parsePositions(response);
}

std::vector<Balance> BybitMargin::getBalance() {
    std::map<std::string, std::string> params;
    params["accountType"] = "CONTRACT";
    
    std::string response = get("/v5/account/wallet-balance", params, true);
    return parseBalances(response);
}

bool BybitMargin::cancelOrder(const std::string& symbol, const std::string& orderId) {
    json j;
    j["category"] = "linear";
    j["symbol"] = symbol;
    j["orderId"] = orderId;
    
    std::string response = post("/v5/order/cancel", j.dump(), true);
    return response.find("\"retCode\":0") != std::string::npos;
}

OpenOrdersResult BybitMargin::getOpenOrders(
 const std::string& symbol,
    const std::string& baseCoin,
    const std::string& settleCoin,
    const std::string& orderId,
    const std::string& orderLinkId,
    int openOnly,
    const std::string& orderFilter,
    int limit,
    const std::string& cursor) {
    std::map<std::string, std::string> params;

    // Обязательный параметр
    params["category"] = "linear";

    // Опциональные параметры
    if (!symbol.empty()) {
        params["symbol"] = symbol;
    }

    if (!baseCoin.empty()) {
        params["baseCoin"] = baseCoin;
    }

    if (!settleCoin.empty()) {
        params["settleCoin"] = settleCoin;
    }

    if (!orderId.empty()) {
        params["orderId"] = orderId;
    }

    if (!orderLinkId.empty()) {
        params["orderLinkId"] = orderLinkId;
    }

    if (openOnly > 0) {
        params["openOnly"] = std::to_string(openOnly);
    }

    if (!orderFilter.empty()) {
        params["orderFilter"] = orderFilter;
    }

    if (limit > 0 && limit <= 50) {
        params["limit"] = std::to_string(limit);
    }

    if (!cursor.empty()) {
        params["cursor"] = cursor;
    }

    // Делаем GET запрос (требует аутентификации)
    std::string response = get("/v5/order/realtime", params, true);

    // Парсим ответ
    return parseOpenOrders(response);
}

OrderResult BybitMargin::placeOrder(
    const std::string& symbol,
    OrderSide side,
    OrderType orderType,
    double quantity,
    double price,
    bool reduceOnly
) {
    json j;
    j["category"] = "linear";
    j["symbol"] = symbol;
    j["side"] = (side == OrderSide::BUY) ? "Buy" : "Sell";
    j["orderType"] = (orderType == OrderType::LIMIT) ? "Limit" : "Market";
    
    std::ostringstream qtyStream;
    qtyStream << std::fixed << std::setprecision(8) << quantity;
    j["qty"] = qtyStream.str();
    
    if (orderType == OrderType::LIMIT && price > 0.0) {
        std::ostringstream priceStream;
        priceStream << std::fixed << std::setprecision(2) << price;
        j["price"] = priceStream.str();
    }
    
    j["timeInForce"] = "GTC";
    j["reduceOnly"] = reduceOnly;
    
    std::string response = post("/v5/order/create", j.dump(), true);
    return parseOrderResult(response);
}

ExecutionResult BybitMargin::getExecutions(
    const std::string& category,      // "linear" / "spot"
    const std::string& symbol,   // "BTCUSDT"
    const std::string& orderId,  // ID ордера
    int limit                    // 1-100
) {
    std::map<std::string, std::string> params;
    params["category"] = category;

    if (!symbol.empty()) {
        params["symbol"] = symbol;
    }

    if (!orderId.empty()) {
        params["orderId"] = orderId;
    }

    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }

    std::string response = get("/v5/execution/list", params, true);
    return parseExecutions(response);
}

OrderHistoryResult BybitMargin::getOrderHistory(const std::string& category,      // "linear" / "spot"
    const std::string& symbol,   // "BTCUSDT"
    const std::string& orderId,  // ID ордера
    int limit                  // 1-50
) {
    std::map<std::string, std::string> params;
    params["category"] = category;

    if (!symbol.empty()) {
        params["symbol"] = symbol;
    }

    if (!orderId.empty()) {
        params["orderId"] = orderId;
    }

    if (limit > 0) {
        params["limit"] = std::to_string(limit);
    }

    std::string response = get("/v5/order/history", params, true);
    return parseOrderHistory(response);
}

OrderResult BybitMargin::parseOrderResult(const std::string& response) {
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

PositionInfo BybitMargin::parsePosition(const std::string& response) {
    PositionInfo position;
    auto safeStod = [](const std::string& s) -> double {
        return (s.empty() || s == "") ? 0.0 : std::stod(s);
        };
    auto safe_uint64 = [](const json& v) -> uint64_t {
        if (v.is_number_unsigned()) return v.get<uint64_t>();
        if (v.is_number_integer())  return static_cast<uint64_t>(v.get<int64_t>());
        if (v.is_string() && !v.get<std::string>().empty()) return std::stoull(v.get<std::string>());
        return 0;
        };

    try {
        json j = json::parse(response);
        if (j.contains("result") && j["result"].contains("list") && !j["result"]["list"].empty()) {
            auto pos = j["result"]["list"][0];

            position.symbol = pos.value("symbol", "");
            position.side = pos.value("side", "");
            position.size = safeStod(pos.value("size", "0"));
            position.positionValue = safeStod(pos.value("positionValue", "0"));
            position.avgPrice = safeStod(pos.value("avgPrice", "0"));
            position.markPrice = safeStod(pos.value("markPrice", "0"));
            position.liqPrice = safeStod(pos.value("liqPrice", "0"));
            position.liqPriceByMp = safeStod(pos.value("liqPriceByMp", "0"));
            position.bustPrice = safeStod(pos.value("bustPrice", "0"));
            position.leverage = safeStod(pos.value("leverage", "1"));
            position.autoAddMargin = pos.value("autoAddMargin", 0);
            position.positionIM = safeStod(pos.value("positionIM", "0"));
            position.positionIMByMp = safeStod(pos.value("positionIMByMp", "0"));
            position.positionMM = safeStod(pos.value("positionMM", "0"));
            position.positionMMByMp = safeStod(pos.value("positionMMByMp", "0"));
            position.unrealisedPnl = safeStod(pos.value("unrealisedPnl", "0"));
            position.cumRealisedPnl = safeStod(pos.value("cumRealisedPnl", "0"));
            position.curRealisedPnl = safeStod(pos.value("curRealisedPnl", "0"));
            position.positionBalance = safeStod(pos.value("positionBalance", "0"));
            position.tpslMode = pos.value("tpslMode", "");
            position.stopLoss = safeStod(pos.value("stopLoss", "0"));
            position.takeProfit = safeStod(pos.value("takeProfit", "0"));
            position.trailingStop = safeStod(pos.value("trailingStop", "0"));
            position.positionStatus = pos.value("positionStatus", "");
            position.tradeMode = pos.value("tradeMode", 0);
            position.riskId = pos.value("riskId", 1);
            position.riskLimitValue = safeStod(pos.value("riskLimitValue", "0"));
            position.adlRankIndicator = pos.value("adlRankIndicator", 0);
            position.isReduceOnly = pos.value("isReduceOnly", false);
            position.tpTriggerBy = pos.value("tpTriggerBy", "");
            position.slTriggerBy = pos.value("slTriggerBy", "");
            position.positionIdx = pos.value("positionIdx", 0);
            position.createdTime = safe_uint64(pos["createdTime"]);
            position.updatedTime = safe_uint64(pos["updatedTime"]);
            position.seq = pos.value("seq", 0LL);
            position.leverageSysUpdatedTime = pos.value("leverageSysUpdatedTime", "");
            position.mmrSysUpdatedTime = pos.value("mmrSysUpdatedTime", "");
            position.sessionAvgPrice = safeStod(pos.value("sessionAvgPrice", "0"));
        }
    }
    catch (const json::exception& e) {
    }
    catch (const std::exception& e) {
    }
    return position;
}

std::vector<PositionInfo> BybitMargin::parsePositions(const std::string& response) {
    std::vector<PositionInfo> positions;
    auto safeStod = [](const std::string& s) -> double {
        return (s.empty() || s == "") ? 0.0 : std::stod(s);
        };
    auto safe_uint64 = [](const json& v) -> uint64_t {
        if (v.is_number_unsigned()) return v.get<uint64_t>();
        if (v.is_number_integer())  return static_cast<uint64_t>(v.get<int64_t>());
        if (v.is_string() && !v.get<std::string>().empty()) return std::stoull(v.get<std::string>());
        return 0;
        };
    try {
        json j = json::parse(response);
        if (j.contains("result") && j["result"].contains("list")) {
            for (const auto& pos : j["result"]["list"]) {
                PositionInfo position;

                position.symbol = pos.value("symbol", "");
                position.side = pos.value("side", "");
                position.size = safeStod(pos.value("size", "0"));

                if (position.size == 0.0) continue;

                position.positionValue = safeStod(pos.value("positionValue", "0"));
                position.avgPrice = safeStod(pos.value("avgPrice", "0"));
                position.markPrice = safeStod(pos.value("markPrice", "0"));
                position.liqPrice = safeStod(pos.value("liqPrice", "0"));
                position.liqPriceByMp = safeStod(pos.value("liqPriceByMp", "0"));
                position.bustPrice = safeStod(pos.value("bustPrice", "0"));
                position.leverage = safeStod(pos.value("leverage", "1"));
                position.autoAddMargin = pos.value("autoAddMargin", 0);
                position.positionIM = safeStod(pos.value("positionIM", "0"));
                position.positionIMByMp = safeStod(pos.value("positionIMByMp", "0"));
                position.positionMM = safeStod(pos.value("positionMM", "0"));
                position.positionMMByMp = safeStod(pos.value("positionMMByMp", "0"));
                position.unrealisedPnl = safeStod(pos.value("unrealisedPnl", "0"));
                position.cumRealisedPnl = safeStod(pos.value("cumRealisedPnl", "0"));
                position.curRealisedPnl = safeStod(pos.value("curRealisedPnl", "0"));
                position.positionBalance = safeStod(pos.value("positionBalance", "0"));
                position.tpslMode = pos.value("tpslMode", "");
                position.stopLoss = safeStod(pos.value("stopLoss", "0"));
                position.takeProfit = safeStod(pos.value("takeProfit", "0"));
                position.trailingStop = safeStod(pos.value("trailingStop", "0"));
                position.positionStatus = pos.value("positionStatus", "");
                position.tradeMode = pos.value("tradeMode", 0);
                position.riskId = pos.value("riskId", 1);
                position.riskLimitValue = safeStod(pos.value("riskLimitValue", "0"));
                position.adlRankIndicator = pos.value("adlRankIndicator", 0);
                position.isReduceOnly = pos.value("isReduceOnly", false);
                position.tpTriggerBy = pos.value("tpTriggerBy", "");
                position.slTriggerBy = pos.value("slTriggerBy", "");
                position.positionIdx = pos.value("positionIdx", 0);
                position.createdTime = safe_uint64(pos["createdTime"]);
                position.updatedTime = safe_uint64(pos["updatedTime"]);
                position.seq = pos.value("seq", 0LL);
                position.leverageSysUpdatedTime = pos.value("leverageSysUpdatedTime", "");
                position.mmrSysUpdatedTime = pos.value("mmrSysUpdatedTime", "");
                position.sessionAvgPrice = safeStod(pos.value("sessionAvgPrice", "0"));

                positions.push_back(position);
            }
        }
    }
    catch (const json::exception& e) {
    }
    catch (const std::exception& e) {
    }
    return positions;
}

OpenOrdersResult BybitMargin::parseOpenOrders(const std::string& jsonResponse)
{
    OpenOrdersResult result;

    auto safeStod = [](const std::string& s) -> double {
        return (s.empty() || s == "") ? 0.0 : std::stod(s);
        };

    try {
        json j = json::parse(jsonResponse);
        // Код ответа
        result.retCode = j.value("retCode", -1);
        result.retMsg = j.value("retMsg", "");
        result.success = (result.retCode == 0);
        if (!result.success) {
            return result;
        }
        // Парсим result объект
        if (j.contains("result")) {
            auto resultObj = j["result"];
            result.category = resultObj.value("category", "");
            result.nextPageCursor = resultObj.value("nextPageCursor", "");
            // Парсим список ордеров
            if (resultObj.contains("list") && resultObj["list"].is_array()) {
                for (const auto& orderJson : resultObj["list"]) {
                    OpenOrder order;
                    order.orderId = orderJson.value("orderId", "");
                    order.orderLinkId = orderJson.value("orderLinkId", "");
                    order.symbol = orderJson.value("symbol", "");
                    order.side = orderJson.value("side", "");
                    order.orderType = orderJson.value("orderType", "");
                    order.price = safeStod(orderJson.value("price", "0"));
                    order.qty = safeStod(orderJson.value("qty", "0"));
                    order.cumExecQty = safeStod(orderJson.value("cumExecQty", "0"));
                    order.cumExecValue = safeStod(orderJson.value("cumExecValue", "0"));
                    order.cumExecFee = safeStod(orderJson.value("cumExecFee", "0"));
                    order.timeInForce = orderJson.value("timeInForce", "");
                    order.orderStatus = orderJson.value("orderStatus", "");
                    order.avgPrice = safeStod(orderJson.value("avgPrice", "0"));
                    order.leavesQty = safeStod(orderJson.value("leavesQty", "0"));
                    order.leavesValue = safeStod(orderJson.value("leavesValue", "0"));
                    order.stopOrderType = orderJson.value("stopOrderType", "");
                    order.orderIv = orderJson.value("orderIv", "");
                    order.triggerPrice = safeStod(orderJson.value("triggerPrice", "0"));
                    order.takeProfit = safeStod(orderJson.value("takeProfit", "0"));
                    order.stopLoss = safeStod(orderJson.value("stopLoss", "0"));
                    order.tpTriggerBy = orderJson.value("tpTriggerBy", "");
                    order.slTriggerBy = orderJson.value("slTriggerBy", "");
                    order.triggerDirection = orderJson.value("triggerDirection", 0);
                    order.triggerBy = orderJson.value("triggerBy", "");
                    order.reduceOnly = orderJson.value("reduceOnly", false);
                    order.closeOnTrigger = orderJson.value("closeOnTrigger", false);
                    order.placeType = orderJson.value("placeType", "");
                    order.createdTime = orderJson.value("createdTime", "");
                    order.updatedTime = orderJson.value("updatedTime", "");
                    result.orders.push_back(order);
                }
            }
        }
    }
    catch (const json::exception& e) {
        result.success = false;
        result.retMsg = std::string("JSON parse error: ") + e.what();
    }
    catch (const std::exception& e) {
        result.success = false;
        result.retMsg = std::string("Error: ") + e.what();
    }
    return result;
}


std::vector<Balance> BybitMargin::parseBalances(const std::string& response) {
    std::vector<Balance> balances;
    
    try {
        json j = json::parse(response);
        
        if (j.contains("result") && j["result"].contains("list")) {
            for (const auto& accountItem : j["result"]["list"]) {
                if (accountItem.contains("coin")) {
                    for (const auto& coinItem : accountItem["coin"]) {
                        Balance balance;
                        balance.coin = coinItem.value("coin", "");
                        balance.total = std::stod(coinItem.value("walletBalance", "0"));
                        balance.available = std::stod(coinItem.value("availableToWithdraw", "0"));
                        balance.locked = balance.total - balance.available;
                        
                        balances.push_back(balance);
                    }
                }
            }
        }
        
    } catch (const json::exception& e) {
    } catch (const std::exception& e) {
    }
    
    return balances;
}

ExecutionResult BybitMargin::parseExecutions(const std::string& jsonResponse) {
    ExecutionResult result;

    auto safeStod = [](const std::string& s) -> double {
        return (s.empty() || s == "") ? 0.0 : std::stod(s);
        };

    try {
        json j = json::parse(jsonResponse);

        result.retCode = j.value("retCode", -1);
        result.retMsg = j.value("retMsg", "");
        result.success = (result.retCode == 0);

        if (!result.success) {
            return result;
        }

        if (j.contains("result")) {
            auto resultObj = j["result"];
            result.category = resultObj.value("category", "");
            result.nextPageCursor = resultObj.value("nextPageCursor", "");

            if (resultObj.contains("list") && resultObj["list"].is_array()) {
                for (const auto& execJson : resultObj["list"]) {
                    Execution exec;

                    exec.symbol = execJson.value("symbol", "");
                    exec.orderId = execJson.value("orderId", "");
                    exec.orderLinkId = execJson.value("orderLinkId", "");
                    exec.side = execJson.value("side", "");
                    exec.orderType = execJson.value("orderType", "");
                    exec.execPrice = safeStod(execJson.value("execPrice", "0"));
                    exec.execQty = safeStod(execJson.value("execQty", "0"));
                    exec.execValue = safeStod(execJson.value("execValue", "0"));
                    exec.execFee = safeStod(execJson.value("execFee", "0"));
                    exec.feeRate = execJson.value("feeRate", "");
                    exec.execTime = execJson.value("execTime", "");
                    exec.isMaker = execJson.value("isMaker", false);
                    exec.execType = execJson.value("execType", "");
                    exec.execId = execJson.value("execId", "");
                    exec.seq = execJson.value("seq", 0LL);

                    result.executions.push_back(exec);
                }
            }
        }
    }
    catch (const json::exception& e) {
        result.success = false;
        result.retMsg = std::string("JSON parse error: ") + e.what();
    }
    catch (const std::exception& e) {
        result.success = false;
        result.retMsg = std::string("Error: ") + e.what();
    }

    return result;
}

OrderHistoryResult BybitMargin::parseOrderHistory(const std::string& jsonResponse) {
    OrderHistoryResult result;

    auto safeStod = [](const std::string& s) -> double {
        return (s.empty() || s == "") ? 0.0 : std::stod(s);
        };

    try {
        json j = json::parse(jsonResponse);

        result.retCode = j.value("retCode", -1);
        result.retMsg = j.value("retMsg", "");
        result.success = (result.retCode == 0);

        if (!result.success) {
            return result;
        }

        if (j.contains("result")) {
            auto resultObj = j["result"];
            result.category = resultObj.value("category", "");
            result.nextPageCursor = resultObj.value("nextPageCursor", "");

            if (resultObj.contains("list") && resultObj["list"].is_array()) {
                for (const auto& orderJson : resultObj["list"]) {
                    OrderHistory order;

                    order.orderId = orderJson.value("orderId", "");
                    order.orderLinkId = orderJson.value("orderLinkId", "");
                    order.blockTradeId = orderJson.value("blockTradeId", "");
                    order.symbol = orderJson.value("symbol", "");
                    order.price = safeStod(orderJson.value("price", "0"));
                    order.qty = safeStod(orderJson.value("qty", "0"));
                    order.side = orderJson.value("side", "");
                    order.isLeverage = orderJson.value("isLeverage", "");
                    order.positionIdx = orderJson.value("positionIdx", 0);
                    order.orderStatus = orderJson.value("orderStatus", "");
                    order.cancelType = orderJson.value("cancelType", "");
                    order.rejectReason = orderJson.value("rejectReason", "");
                    order.avgPrice = safeStod(orderJson.value("avgPrice", "0"));
                    order.leavesQty = safeStod(orderJson.value("leavesQty", "0"));
                    order.leavesValue = safeStod(orderJson.value("leavesValue", "0"));
                    order.cumExecQty = safeStod(orderJson.value("cumExecQty", "0"));
                    order.cumExecValue = safeStod(orderJson.value("cumExecValue", "0"));
                    order.cumExecFee = safeStod(orderJson.value("cumExecFee", "0"));
                    order.timeInForce = orderJson.value("timeInForce", "");
                    order.orderType = orderJson.value("orderType", "");
                    order.stopOrderType = orderJson.value("stopOrderType", "");
                    order.orderIv = orderJson.value("orderIv", "");
                    order.triggerPrice = safeStod(orderJson.value("triggerPrice", "0"));
                    order.takeProfit = safeStod(orderJson.value("takeProfit", "0"));
                    order.stopLoss = safeStod(orderJson.value("stopLoss", "0"));
                    order.tpTriggerBy = orderJson.value("tpTriggerBy", "");
                    order.slTriggerBy = orderJson.value("slTriggerBy", "");
                    order.triggerDirection = orderJson.value("triggerDirection", 0);
                    order.triggerBy = orderJson.value("triggerBy", "");
                    order.lastPriceOnCreated = safeStod(orderJson.value("lastPriceOnCreated", "0"));
                    order.reduceOnly = orderJson.value("reduceOnly", false);
                    order.closeOnTrigger = orderJson.value("closeOnTrigger", false);
                    order.smpType = orderJson.value("smpType", "");
                    order.smpGroup = orderJson.value("smpGroup", 0);
                    order.smpOrderId = orderJson.value("smpOrderId", "");
                    order.tpslMode = orderJson.value("tpslMode", "");
                    order.tpLimitPrice = safeStod(orderJson.value("tpLimitPrice", "0"));
                    order.slLimitPrice = safeStod(orderJson.value("slLimitPrice", "0"));
                    order.placeType = orderJson.value("placeType", "");
                    order.createdTime = orderJson.value("createdTime", "");
                    order.updatedTime = orderJson.value("updatedTime", "");

                    result.orders.push_back(order);
                }
            }
        }
    }
    catch (const json::exception& e) {
        result.success = false;
        result.retMsg = std::string("JSON parse error: ") + e.what();
    }
    catch (const std::exception& e) {
        result.success = false;
        result.retMsg = std::string("Error: ") + e.what();
    }

    return result;
}

} // namespace Bybit
