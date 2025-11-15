#include "../../Public/StatsManager/TradingStatsManager.h"
#include <iostream>
#include <cmath>

TradingStatsManager::TradingStatsManager(double initial_balance, double commission)
    : startingBalance(initial_balance), currentBalance(initial_balance),
    totalShares(0), fixedCommission(commission), fixedPercentCommission(0.0),
    totalCommissionPaid(0), nextOrderId(1),
    statsCacheValid(false), cachedWinRate(0.0), cachedAvgPnL(0.0),
    cachedBestTrade(0.0), cachedWorstTrade(0.0) {
    balanceHistory.push_back(startingBalance);
    balanceTimepoints.push_back(0.0);
}

int TradingStatsManager::PlaceBuyOrder(uint64_t time, double price, double quantity) {
    if (price <= 0 || quantity <= 0) {
        return -1;
    }

    double orderAmountUSD = price * quantity;
    double totalCommission = (fixedCommission)+(orderAmountUSD * fixedPercentCommission / 100.0);

    Order order(OrderType::BUY, price, quantity, time, totalCommission, 1.0);
    order.id = nextOrderId++;
    order.isExecuted = true;

    allOrders.push_back(order);
    ProcessBuyOrder(order);
    UpdateBalanceHistory();
    InvalidateStatsCache();

    return order.id;
}

int TradingStatsManager::PlaceSellOrder(uint64_t time, double price, double percentage) {
    if (percentage <= 0.0 || percentage > 100.0) {
        return -1;
    }

    if (totalShares <= 0) {
        return -1;
    }

    double tokensToSell = totalShares * (percentage / 100.0);
    double orderAmountUSD = price * tokensToSell;
    double totalCommission = (fixedCommission)+(orderAmountUSD * fixedPercentCommission / 100.0);

    Order order(OrderType::SELL, price, percentage, time, totalCommission, 1.0);
    order.id = nextOrderId++;
    order.isExecuted = true;

    allOrders.push_back(order);
    ProcessSellOrderByPercentage(order);
    UpdateBalanceHistory();
    InvalidateStatsCache();

    return order.id;
}

int TradingStatsManager::PlaceSellOrderQuantity(uint64_t time, double price, double quantity) {
    if (quantity <= 0) {
        return -1;
    }

    if (quantity > totalShares) {
        return -1;
    }

    double orderAmountUSD = price * quantity;
    double totalCommission = (fixedCommission)+(orderAmountUSD * fixedPercentCommission / 100.0);

    Order order(OrderType::SELL, price, quantity, time, totalCommission, 1.0);
    order.id = nextOrderId++;
    order.isExecuted = true;

    allOrders.push_back(order);
    ProcessSellOrder(order);
    UpdateBalanceHistory();
    InvalidateStatsCache();

    return order.id;
}

int TradingStatsManager::EnterLong(uint64_t time, double price, double quantity, double leverage, double liquidationPrice) {
    if (price <= 0 || quantity <= 0 || leverage <= 0) {
        return -1;
    }

    double initialMargin = (quantity * price) / leverage;
    double positionSize = price * quantity;       
    double calculatedCommission = quantity * price * (1 - 1.0 / 10) * 0.00055;
    double totalCommission = (fixedCommission) + (positionSize * fixedPercentCommission / 100.0) + calculatedCommission;

    if (initialMargin + totalCommission > currentBalance) {
        return -1;   
    }

    Order order(OrderType::LONG, price, quantity, time, totalCommission, leverage);
    order.id = nextOrderId++;
    order.isExecuted = true;

    allOrders.push_back(order);
    ProcessLongOrder(order);
    UpdateBalanceHistory();
    InvalidateStatsCache();

    if (liquidationPrice > 0) {
        for (auto& pos : allPositions) {
            if (pos.buyOrderId == order.id) {
                pos.liquidationPrice = liquidationPrice;
                break;
            }
        }
    }

    return order.id;
}

int TradingStatsManager::EnterShort(uint64_t time, double price, double quantity, double leverage, double liquidationPrice) {
    if (price <= 0 || quantity <= 0 || leverage <= 0) {
        return -1;
    }

    double initialMargin = (quantity * price) / leverage;
    double positionSize = price * quantity;       
    double calculatedCommission = quantity * price * (1 + 1.0 / 10) * 0.00055;
    double totalCommission = (fixedCommission)+(positionSize * fixedPercentCommission / 100.0) + calculatedCommission;

    if (initialMargin + totalCommission > currentBalance) {
        return -1;   
    }

    Order order(OrderType::SHORT, price, quantity, time, totalCommission, leverage);
    order.id = nextOrderId++;
    order.isExecuted = true;

    allOrders.push_back(order);
    ProcessShortOrder(order);
    UpdateBalanceHistory();
    InvalidateStatsCache();

    if (liquidationPrice > 0) {
        for (auto& pos : allPositions) {
            if (pos.buyOrderId == order.id) {
                pos.liquidationPrice = liquidationPrice;
                break;
            }
        }
    }

    return order.id;
}

int TradingStatsManager::CloseLong(uint64_t time, double price, double percentage) {
    if (percentage <= 0.0 || percentage > 100.0) {
        return -1;
    }

    auto longPositions = GetOpenLongPositions();
    if (longPositions.empty()) {
        return -1;
    }

    double totalLongQuantity = 0;
    for (const auto& pos : longPositions) {
        totalLongQuantity += pos.quantity;
    }

    double quantityToClose = totalLongQuantity * (percentage / 100.0);
    double positionSize = price * quantityToClose;     
    double totalCommission = (fixedCommission)+(positionSize * fixedPercentCommission / 100.0);

    Order order(OrderType::CLOSE_LONG, price, percentage, time, totalCommission, 1.0);
    order.id = nextOrderId++;
    order.isExecuted = true;

    allOrders.push_back(order);
    ProcessCloseLongByPercentage(order);
    UpdateBalanceHistory();
    InvalidateStatsCache();

    return order.id;
}

int TradingStatsManager::CloseShort(uint64_t time, double price, double percentage) {
    if (percentage <= 0.0 || percentage > 100.0) {
        return -1;
    }

    auto shortPositions = GetOpenShortPositions();
    if (shortPositions.empty()) {
        return -1;
    }

    double totalShortQuantity = 0;
    for (const auto& pos : shortPositions) {
        totalShortQuantity += pos.quantity;
    }

    double quantityToClose = totalShortQuantity * (percentage / 100.0);
    double positionSize = price * quantityToClose;     
    double totalCommission = (fixedCommission)+(positionSize * fixedPercentCommission / 100.0);

    Order order(OrderType::CLOSE_SHORT, price, percentage, time, totalCommission, 1.0);
    order.id = nextOrderId++;
    order.isExecuted = true;

    allOrders.push_back(order);
    ProcessCloseShortByPercentage(order);
    UpdateBalanceHistory();
    InvalidateStatsCache();

    return order.id;
}

int TradingStatsManager::CloseLongQuantity(uint64_t time, double price, double quantity) {
    if (quantity <= 0) {
        return -1;
    }

    auto longPositions = GetOpenLongPositions();
    double totalLongQuantity = 0;
    for (const auto& pos : longPositions) {
        totalLongQuantity += pos.quantity;
    }

    if (quantity > totalLongQuantity) {
        return -1;
    }

    double positionSize = price * quantity;     
    double totalCommission = (fixedCommission)+(positionSize * fixedPercentCommission / 100.0);

    Order order(OrderType::CLOSE_LONG, price, quantity, time, totalCommission, 1.0);
    order.id = nextOrderId++;
    order.isExecuted = true;

    allOrders.push_back(order);
    ProcessCloseLong(order, quantity);
    UpdateBalanceHistory();
    InvalidateStatsCache();

    return order.id;
}

int TradingStatsManager::CloseShortQuantity(uint64_t time, double price, double quantity) {
    if (quantity <= 0) {
        return -1;
    }

    auto shortPositions = GetOpenShortPositions();
    double totalShortQuantity = 0;
    for (const auto& pos : shortPositions) {
        totalShortQuantity += pos.quantity;
    }

    if (quantity > totalShortQuantity) {
        return -1;
    }

    double positionSize = price * quantity;     
    double totalCommission = (fixedCommission)+(positionSize * fixedPercentCommission / 100.0);

    Order order(OrderType::CLOSE_SHORT, price, quantity, time, totalCommission, 1.0);
    order.id = nextOrderId++;
    order.isExecuted = true;

    allOrders.push_back(order);
    ProcessCloseShort(order, quantity);
    UpdateBalanceHistory();
    InvalidateStatsCache();

    return order.id;
}

void TradingStatsManager::ProcessBuyOrder(const Order& order) {
    double totalCost = order.price * order.quantity + order.commission;
    if (totalCost > currentBalance) {
        return;
    }

    Position position;
    position.buyOrderId = order.id;
    position.buyPrice = order.price;
    position.quantity = order.quantity;
    position.buyTime = order.time;
    position.buyCommission = order.commission;
    position.isOpen = true;
    position.mode = PositionMode::SPOT;
    position.leverage = 1.0;

    allPositions.push_back(position);

    currentBalance -= totalCost;
    totalShares += order.quantity;
    totalCommissionPaid += order.commission;
}

void TradingStatsManager::ProcessSellOrderByPercentage(const Order& order) {
    double percentage = order.quantity;
    double totalTokensToSell = totalShares * (percentage / 100.0);

    if (totalTokensToSell <= 0 || totalShares <= 0) {
        return;
    }

    ProcessSellOrderInternal(order, totalTokensToSell);
}

void TradingStatsManager::ProcessSellOrder(const Order& order) {
    double quantityToSell = order.quantity;

    if (quantityToSell <= 0 || quantityToSell > totalShares) {
        return;
    }

    ProcessSellOrderInternal(order, quantityToSell);
}

void TradingStatsManager::ProcessSellOrderInternal(const Order& order, double totalQuantityToSell) {
    double remainingQuantity = totalQuantityToSell;
    double totalSellValue = 0;

    std::vector<Position*> openPositions;
    for (auto& position : allPositions) {
        if (position.isOpen && position.mode == PositionMode::SPOT) {
            openPositions.push_back(&position);
        }
    }

    std::sort(openPositions.begin(), openPositions.end(),
        [](const Position* a, const Position* b) {
            return a->buyTime < b->buyTime;
        });

    for (auto* position : openPositions) {
        if (remainingQuantity <= 1e-9) break;

        double quantityToSell = std::min(position->quantity, remainingQuantity);
        double sellValue = order.price * quantityToSell;

        if (std::abs(quantityToSell - position->quantity) < 1e-9) {
            position->Close(order.price, order.time, order.id, order.commission);
            totalSellValue += sellValue;
            totalShares -= quantityToSell;
        }
        else {
            Position closedPart = *position;
            closedPart.quantity = quantityToSell;

            double commissionRatio = quantityToSell / position->quantity;
            closedPart.buyCommission = position->buyCommission * commissionRatio;
            closedPart.Close(order.price, order.time, order.id, order.commission);
            allPositions.push_back(closedPart);

            position->quantity -= quantityToSell;
            position->buyCommission -= closedPart.buyCommission;

            totalSellValue += sellValue;
            totalShares -= quantityToSell;
        }

        remainingQuantity -= quantityToSell;
    }

    currentBalance += (totalSellValue - order.commission);
    totalCommissionPaid += order.commission;
}

void TradingStatsManager::ProcessLongOrder(const Order& order) {
    double initialMargin = (order.quantity * order.price) / order.leverage;
    double totalCost = initialMargin + order.commission;

    if (totalCost > currentBalance) {
        return;
    }

    Position position;
    position.buyOrderId = order.id;
    position.buyPrice = order.price;
    position.quantity = order.quantity;
    position.buyTime = order.time;
    position.buyCommission = order.commission;
    position.isOpen = true;
    position.mode = PositionMode::MARGIN;
    position.isLong = true;
    position.leverage = order.leverage;
    position.initialMargin = initialMargin;

    allPositions.push_back(position);

    currentBalance -= totalCost;
    totalCommissionPaid += order.commission;
}

void TradingStatsManager::ProcessShortOrder(const Order& order) {
    double initialMargin = (order.quantity * order.price) / order.leverage;
    double totalCost = initialMargin + order.commission;

    if (totalCost > currentBalance) {
        return;
    }

    Position position;
    position.buyOrderId = order.id;
    position.buyPrice = order.price;
    position.quantity = order.quantity;
    position.buyTime = order.time;
    position.buyCommission = order.commission;
    position.isOpen = true;
    position.mode = PositionMode::MARGIN;
    position.isLong = false;
    position.leverage = order.leverage;
    position.initialMargin = initialMargin;

    allPositions.push_back(position);

    currentBalance -= totalCost;
    totalCommissionPaid += order.commission;
}

void TradingStatsManager::ProcessCloseLongByPercentage(const Order& order) {
    double percentage = order.quantity;

    auto longPositions = FindOpenMarginPositions(true);
    if (longPositions.empty()) return;

    double totalLongQuantity = 0;
    for (auto* pos : longPositions) {
        totalLongQuantity += pos->quantity;
    }

    double quantityToClose = totalLongQuantity * (percentage / 100.0);
    ProcessCloseLong(order, quantityToClose);
}

void TradingStatsManager::ProcessCloseShortByPercentage(const Order& order) {
    double percentage = order.quantity;

    auto shortPositions = FindOpenMarginPositions(false);
    if (shortPositions.empty()) return;

    double totalShortQuantity = 0;
    for (auto* pos : shortPositions) {
        totalShortQuantity += pos->quantity;
    }

    double quantityToClose = totalShortQuantity * (percentage / 100.0);
    ProcessCloseShort(order, quantityToClose);
}

void TradingStatsManager::ProcessCloseLong(const Order& order, double quantityToClose) {
    auto longPositions = FindOpenMarginPositions(true);

    std::sort(longPositions.begin(), longPositions.end(),
        [](const Position* a, const Position* b) {
            return a->buyTime < b->buyTime;
        });

    double remainingQuantity = quantityToClose;

    for (auto* position : longPositions) {
        if (remainingQuantity <= 1e-9) break;

        double closeQty = std::min(position->quantity, remainingQuantity);

        if (std::abs(closeQty - position->quantity) < 1e-9) {
            position->Close(order.price, order.time, order.id, order.commission);

            currentBalance += position->initialMargin + position->pnl;
        }
        else {
            Position closedPart = *position;
            closedPart.quantity = closeQty;

            double ratio = closeQty / position->quantity;
            closedPart.initialMargin = position->initialMargin * ratio;
            closedPart.buyCommission = position->buyCommission * ratio;
            closedPart.Close(order.price, order.time, order.id, order.commission);

            allPositions.push_back(closedPart);

            currentBalance += closedPart.initialMargin + closedPart.pnl;

            position->quantity -= closeQty;
            position->initialMargin -= closedPart.initialMargin;
            position->buyCommission -= closedPart.buyCommission;
        }

        remainingQuantity -= closeQty;
    }

    totalCommissionPaid += order.commission;
}

void TradingStatsManager::ProcessCloseShort(const Order& order, double quantityToClose) {
    auto shortPositions = FindOpenMarginPositions(false);

    std::sort(shortPositions.begin(), shortPositions.end(),
        [](const Position* a, const Position* b) {
            return a->buyTime < b->buyTime;
        });

    double remainingQuantity = quantityToClose;

    for (auto* position : shortPositions) {
        if (remainingQuantity <= 1e-9) break;

        double closeQty = std::min(position->quantity, remainingQuantity);

        if (std::abs(closeQty - position->quantity) < 1e-9) {
            position->Close(order.price, order.time, order.id, order.commission);

            currentBalance += position->initialMargin + position->pnl;
        }
        else {
            Position closedPart = *position;
            closedPart.quantity = closeQty;

            double ratio = closeQty / position->quantity;
            closedPart.initialMargin = position->initialMargin * ratio;
            closedPart.buyCommission = position->buyCommission * ratio;
            closedPart.Close(order.price, order.time, order.id, order.commission);

            allPositions.push_back(closedPart);

            currentBalance += closedPart.initialMargin + closedPart.pnl;

            position->quantity -= closeQty;
            position->initialMargin -= closedPart.initialMargin;
            position->buyCommission -= closedPart.buyCommission;
        }

        remainingQuantity -= closeQty;
    }

    totalCommissionPaid += order.commission;
}

std::vector<Position*> TradingStatsManager::FindOpenMarginPositions(bool isLong) {
    std::vector<Position*> positions;
    for (auto& position : allPositions) {
        if (position.isOpen && position.mode == PositionMode::MARGIN && position.isLong == isLong) {
            positions.push_back(&position);
        }
    }
    return positions;
}

Position* TradingStatsManager::FindOldestOpenPosition() {
    Position* oldest = nullptr;
    for (auto& position : allPositions) {
        if (position.isOpen) {
            if (!oldest || position.buyTime < oldest->buyTime) {
                oldest = &position;
            }
        }
    }
    return oldest;
}

std::vector<Position> TradingStatsManager::GetOpenPositions() const {
    std::vector<Position> openPositions;
    for (const auto& position : allPositions) {
        if (position.isOpen) {
            openPositions.push_back(position);
        }
    }
    return openPositions;
}

std::vector<Position> TradingStatsManager::GetClosedPositions() const {
    std::vector<Position> closedPositions;
    for (const auto& position : allPositions) {
        if (!position.isOpen) {
            closedPositions.push_back(position);
        }
    }
    return closedPositions;
}

std::vector<Position> TradingStatsManager::GetOpenLongPositions() const {
    std::vector<Position> longPositions;
    for (const auto& position : allPositions) {
        if (position.isOpen && position.mode == PositionMode::MARGIN && position.isLong) {
            longPositions.push_back(position);
        }
    }
    return longPositions;
}

Position TradingStatsManager::GetOpenLongPosition() const {
    Position aggregated;
    aggregated.isLong = true;
    aggregated.mode = PositionMode::MARGIN;
    aggregated.isOpen = true;

    double totalQuantity = 0.0;
    double weightedPriceSum = 0.0;
    double totalBuyCommission = 0.0;
    double totalInitialMargin = 0.0;

    double sumLiqDelta = 0.0;      
    double totalQtyForLiq = 0.0;

    bool hasAny = false;

    for (const auto& p : allPositions) {
        if (p.isOpen && p.mode == PositionMode::MARGIN && p.isLong) {
            hasAny = true;
            totalQuantity += p.quantity;
            weightedPriceSum += p.buyPrice * p.quantity;
            totalBuyCommission += p.buyCommission;
            totalInitialMargin += p.initialMargin;

            if (aggregated.buyTime == 0 || p.buyTime < aggregated.buyTime)
                aggregated.buyTime = p.buyTime;

            if (p.buyPrice > 0.0 && p.liquidationPrice > 0.0 && p.quantity > 0.0) {
                sumLiqDelta += (p.buyPrice - p.liquidationPrice) * p.quantity;
                totalQtyForLiq += p.quantity;
            }
        }
    }

    if (!hasAny || totalQuantity <= 0.0)
        return Position();      

    aggregated.quantity = totalQuantity;
    aggregated.buyPrice = weightedPriceSum / totalQuantity;
    aggregated.buyCommission = totalBuyCommission;
    aggregated.initialMargin = totalInitialMargin;

    double totalWeightedLev = 0.0;
    for (const auto& p : allPositions) {
        if (p.isOpen && p.mode == PositionMode::MARGIN && p.isLong && p.quantity > 0.0)
            totalWeightedLev += p.leverage * p.quantity;
    }
    aggregated.leverage = (totalQuantity > 0.0) ? (totalWeightedLev / totalQuantity) : 1.0;

    if (totalQtyForLiq > 0.0) {
        double delta = sumLiqDelta / totalQtyForLiq;
        aggregated.liquidationPrice = aggregated.buyPrice - delta;
    }
    else {
        if (aggregated.leverage > 0.0)
            aggregated.liquidationPrice = aggregated.buyPrice * (1.0 - 1.0 / aggregated.leverage);
        else
            aggregated.liquidationPrice = 0.0;
    }

    return aggregated;
}



std::vector<Position> TradingStatsManager::GetOpenShortPositions() const {
    std::vector<Position> shortPositions;
    for (const auto& position : allPositions) {
        if (position.isOpen && position.mode == PositionMode::MARGIN && !position.isLong) {
            shortPositions.push_back(position);
        }
    }
    return shortPositions;
}

Position TradingStatsManager::GetOpenShortPosition() const {
    Position aggregated;
    aggregated.isLong = false;
    aggregated.mode = PositionMode::MARGIN;
    aggregated.isOpen = true;

    double totalQuantity = 0.0;
    double weightedPriceSum = 0.0;
    double totalBuyCommission = 0.0;
    double totalInitialMargin = 0.0;

    double sumLiqDelta = 0.0;        
    double totalQtyForLiq = 0.0;

    bool hasAny = false;

    for (const auto& p : allPositions) {
        if (p.isOpen && p.mode == PositionMode::MARGIN && !p.isLong) {
            hasAny = true;
            totalQuantity += p.quantity;
            weightedPriceSum += p.buyPrice * p.quantity;
            totalBuyCommission += p.buyCommission;
            totalInitialMargin += p.initialMargin;

            if (aggregated.buyTime == 0 || p.buyTime < aggregated.buyTime)
                aggregated.buyTime = p.buyTime;

            if (p.buyPrice > 0.0 && p.liquidationPrice > 0.0 && p.quantity > 0.0) {
                sumLiqDelta += (p.liquidationPrice - p.buyPrice) * p.quantity;
                totalQtyForLiq += p.quantity;
            }
        }
    }

    if (!hasAny || totalQuantity <= 0.0)
        return Position();    

    aggregated.quantity = totalQuantity;
    aggregated.buyPrice = weightedPriceSum / totalQuantity;
    aggregated.buyCommission = totalBuyCommission;
    aggregated.initialMargin = totalInitialMargin;

    double totalWeightedLev = 0.0;
    for (const auto& p : allPositions) {
        if (p.isOpen && p.mode == PositionMode::MARGIN && !p.isLong && p.quantity > 0.0)
            totalWeightedLev += p.leverage * p.quantity;
    }
    aggregated.leverage = (totalQuantity > 0.0) ? (totalWeightedLev / totalQuantity) : 1.0;

    if (totalQtyForLiq > 0.0) {
        double delta = sumLiqDelta / totalQtyForLiq;
        aggregated.liquidationPrice = aggregated.buyPrice + delta;
    }
    else {
        if (aggregated.leverage > 0.0)
            aggregated.liquidationPrice = aggregated.buyPrice * (1.0 + 1.0 / aggregated.leverage);
        else
            aggregated.liquidationPrice = 0.0;
    }

    return aggregated;
}




bool TradingStatsManager::CheckLiquidation(double currentPrice) {
    bool hasLiquidation = false;

    for (auto& position : allPositions) {
        if (!position.isOpen || position.mode != PositionMode::MARGIN) continue;
        if (position.liquidationPrice <= 0) continue;

        bool shouldLiquidate = false;
        if (position.isLong && currentPrice <= position.liquidationPrice) {
            shouldLiquidate = true;
        }
        else if (!position.isLong && currentPrice >= position.liquidationPrice) {
            shouldLiquidate = true;
        }

        if (shouldLiquidate) {
            position.Close(position.liquidationPrice, 0, -1, 0);
            position.pnl = -position.initialMargin - position.buyCommission;
            position.roi = -100.0;
            hasLiquidation = true;
        }
    }

    if (hasLiquidation) {
        UpdateBalanceHistory();
        InvalidateStatsCache();
    }

    return hasLiquidation;
}

double TradingStatsManager::GetUnrealizedPnL(double currentPrice) const {
    double unrealized = 0.0;
    for (const auto& position : allPositions) {
        if (position.isOpen) {
            unrealized += position.GetUnrealizedPnL(currentPrice);
        }
    }
    return unrealized;
}

double TradingStatsManager::GetRealizedPnL() const {
    double realized = 0.0;
    for (const auto& position : allPositions) {
        if (!position.isOpen) {
            realized += position.pnl;
        }
    }
    return realized;
}

size_t TradingStatsManager::GetOpenPositionsCount() const {
    size_t count = 0;
    for (const auto& position : allPositions) {
        if (position.isOpen) count++;
    }
    return count;
}

size_t TradingStatsManager::GetClosedPositionsCount() const {
    size_t count = 0;
    for (const auto& position : allPositions) {
        if (!position.isOpen) count++;
    }
    return count;
}

double TradingStatsManager::GetTotalROI() const {
    if (startingBalance <= 0) return 0.0;
    return (currentBalance - startingBalance) / startingBalance * 100.0;
}

void TradingStatsManager::InvalidateStatsCache() {
    statsCacheValid = false;
}

void TradingStatsManager::RebuildStatsCache() const {
    if (statsCacheValid) return;

    cachedTrades.clear();

    std::map<int, std::vector<const Position*>> sellOrderToPositions;

    for (const auto& pos : allPositions) {
        if (!pos.isOpen && pos.sellOrderId > 0) {
            sellOrderToPositions[pos.sellOrderId].push_back(&pos);
        }
    }

    int tradeNum = 1;
    std::vector<std::pair<double, int>> sellTimesAndIds;

    for (const auto& pair : sellOrderToPositions) {
        double sellTime = 0.0;
        for (const auto& ord : allOrders) {
            if (ord.id == pair.first) {
                sellTime = ord.time;
                break;
            }
        }
        sellTimesAndIds.push_back({ sellTime, pair.first });
    }

    std::sort(sellTimesAndIds.begin(), sellTimesAndIds.end());

    for (const auto& timePair : sellTimesAndIds) {
        int sellId = timePair.second;
        const auto& positions = sellOrderToPositions[sellId];

        Trade trade;
        trade.tradeNumber = tradeNum++;
        trade.sellOrderId = sellId;
        trade.totalPnL = 0.0;
        trade.totalInvestment = 0.0;

        for (const auto& ord : allOrders) {
            if (ord.id == sellId) {
                trade.sellPrice = ord.price;
                trade.sellTime = ord.time;
                break;
            }
        }

        for (const auto* pos : positions) {
            trade.totalPnL += pos->pnl;

            if (pos->mode == PositionMode::SPOT) {
                trade.totalInvestment += (pos->buyPrice * pos->quantity + pos->buyCommission);
                trade.mode = PositionMode::SPOT;
            }
            else {
                trade.totalInvestment += pos->initialMargin;
                trade.mode = PositionMode::MARGIN;
            }

            trade.buyOrderIds.push_back(pos->buyOrderId);
        }

        if (trade.totalInvestment > 0) {
            trade.totalROI = (trade.totalPnL / trade.totalInvestment) * 100.0;
        }

        cachedTrades.push_back(trade);
    }

    if (cachedTrades.empty()) {
        cachedWinRate = 0.0;
        cachedAvgPnL = 0.0;
        cachedBestTrade = 0.0;
        cachedWorstTrade = 0.0;
    }
    else {
        int winningTrades = 0;
        for (const auto& trade : cachedTrades) {
            if (trade.totalPnL > 0) winningTrades++;
        }
        cachedWinRate = (static_cast<double>(winningTrades) / cachedTrades.size()) * 100.0;

        double totalPnL = 0.0;
        for (const auto& trade : cachedTrades) {
            totalPnL += trade.totalPnL;
        }
        cachedAvgPnL = totalPnL / cachedTrades.size();

        cachedBestTrade = cachedTrades[0].totalPnL;
        cachedWorstTrade = cachedTrades[0].totalPnL;

        for (const auto& trade : cachedTrades) {
            if (trade.totalPnL > cachedBestTrade) {
                cachedBestTrade = trade.totalPnL;
            }
            if (trade.totalPnL < cachedWorstTrade) {
                cachedWorstTrade = trade.totalPnL;
            }
        }
    }

    statsCacheValid = true;
}

size_t TradingStatsManager::GetTotalTradesCount() const {
    RebuildStatsCache();
    return cachedTrades.size();
}

double TradingStatsManager::GetWinRate() const {
    RebuildStatsCache();
    return cachedWinRate;
}

double TradingStatsManager::GetAveragePnL() const {
    RebuildStatsCache();
    return cachedAvgPnL;
}

double TradingStatsManager::GetBestTrade() const {
    RebuildStatsCache();
    return cachedBestTrade;
}

double TradingStatsManager::GetWorstTrade() const {
    RebuildStatsCache();
    return cachedWorstTrade;
}

const std::vector<Trade>& TradingStatsManager::GetTrades() const {
    RebuildStatsCache();
    return cachedTrades;
}

void TradingStatsManager::UpdateBalanceHistory() {
    balanceHistory.clear();
    balanceTimepoints.clear();

    balanceHistory.push_back(startingBalance);
    balanceTimepoints.push_back(0.0);

    auto closedPositions = GetClosedPositions();
    std::sort(closedPositions.begin(), closedPositions.end(),
        [](const Position& a, const Position& b) {
            return a.sellTime < b.sellTime;
        });

    double runningBalance = startingBalance;
    for (const auto& position : closedPositions) {
        runningBalance += position.pnl;
        balanceHistory.push_back(runningBalance);
        balanceTimepoints.push_back(position.sellTime);
    }
}

void TradingStatsManager::SetStartingBalance(double balance) {
    if (balance <= 0) return;
    startingBalance = balance;
    currentBalance = balance;
    totalShares = 0;
    Reset();
}

void TradingStatsManager::Reset() {
    allOrders.clear();
    allPositions.clear();
    balanceHistory.clear();
    balanceTimepoints.clear();
    orderCandlesIDs.clear();

    currentBalance = startingBalance;
    totalShares = 0.0;
    totalCommissionPaid = 0.0;
    nextOrderId = 1;

    balanceHistory.push_back(startingBalance);
    balanceTimepoints.push_back(0.0);

    InvalidateStatsCache();
}

void TradingStatsManager::Reset(double new_starting_balance) {
    if (new_starting_balance <= 0) {
        return;
    }

    startingBalance = new_starting_balance;
    Reset();
}

void TradingStatsManager::ExecuteOrder(int orderId) {
    for (auto& order : allOrders) {
        if (order.id == orderId && !order.isExecuted) {
            order.isExecuted = true;
            if (order.type == OrderType::BUY) {
                ProcessBuyOrder(order);
            }
            else {
                ProcessSellOrderByPercentage(order);
            }
            UpdateBalanceHistory();
            InvalidateStatsCache();
            break;
        }
    }
}

void TradingStatsManager::UpdateOpenPositions() {
}

std::string TradingStatsManager::FormatPrice(double price, int precision) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << price;
    return oss.str();
}

std::string TradingStatsManager::FormatQuantity(double quantity, int precision) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << quantity;
    return oss.str();
}

std::string TradingStatsManager::FormatPnL(double pnl, int precision) const {
    std::ostringstream oss;
    if (pnl >= 0) {
        oss << "+" << std::fixed << std::setprecision(precision) << pnl;
    }
    else {
        oss << std::fixed << std::setprecision(precision) << pnl;
    }
    return oss.str();
}