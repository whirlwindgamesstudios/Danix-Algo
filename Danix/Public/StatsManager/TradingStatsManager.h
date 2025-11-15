#pragma once
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>
#include <map>

enum class OrderType {
    BUY,
    SELL,
    LONG,
    SHORT,
    CLOSE_LONG,
    CLOSE_SHORT
};

enum class PositionMode {
    SPOT,
    MARGIN
};

struct Order {
    int id;
    OrderType type;
    uint64_t time;
    double price;
    double quantity;
    double commission;
    bool isExecuted;
    double leverage;

    Order() : id(0), type(OrderType::BUY), time(0), price(0), quantity(0),
        commission(0), isExecuted(false), leverage(1.0) {
    }

    Order(OrderType t, double p, double q, uint64_t tm = 0, double comm = 0, double lev = 1.0)
        : type(t), price(p), quantity(q), time(tm), commission(comm),
        isExecuted(true), id(0), leverage(lev) {
    }
};

struct Position {
    int buyOrderId;
    int sellOrderId;
    double buyPrice;
    double sellPrice;
    double quantity;
    uint64_t buyTime;
    uint64_t sellTime;
    double buyCommission;
    double sellCommission;
    double pnl;
    double roi;
    bool isOpen;
    PositionMode mode;
    bool isLong;
    double leverage;
    double liquidationPrice;
    double initialMargin;

    Position() : buyOrderId(-1), sellOrderId(-1), buyPrice(0), sellPrice(0),
        quantity(0), buyTime(0), sellTime(0), buyCommission(0), sellCommission(0),
        pnl(0), roi(0), isOpen(true), mode(PositionMode::SPOT),
        isLong(true), leverage(1.0), liquidationPrice(0), initialMargin(0) {
    }

    void Close(double sell_price, uint64_t sell_time, int sell_order_id, double sell_comm) {
        sellPrice = sell_price;
        sellTime = sell_time;
        sellOrderId = sell_order_id;
        sellCommission = sell_comm;
        isOpen = false;
        CalculatePnLAndROI();
    }

    void CalculatePnLAndROI() {
        if (!isOpen) {
            if (mode == PositionMode::SPOT) {
                pnl = (sellPrice - buyPrice) * quantity - buyCommission - sellCommission;
                roi = buyPrice > 0 ? pnl / (buyPrice * quantity + buyCommission) * 100.0 : 0.0;
            }
            else {
                // Для маржинальной торговли
                // quantity уже включает leverage, поэтому НЕ умножаем еще раз!
                if (isLong) {
                    // Long: profit = (sellPrice - buyPrice) * quantity
                    pnl = (sellPrice - buyPrice) * quantity - buyCommission - sellCommission;
                }
                else {
                    // Short: profit = (buyPrice - sellPrice) * quantity  
                    pnl = (buyPrice - sellPrice) * quantity - buyCommission - sellCommission;
                }
                roi = initialMargin > 0 ? pnl / initialMargin * 100.0 : 0.0;
            }
        }
        else {
            pnl = 0;
            roi = 0;
        }
    }

    double GetUnrealizedPnL(double currentPrice) const {
        if (isOpen) {
            if (mode == PositionMode::SPOT) {
                return (currentPrice - buyPrice) * quantity - buyCommission;
            }
            else {
                // Для маржинальной торговли - БЕЗ повторного умножения на leverage
                if (isLong) {
                    return (currentPrice - buyPrice) * quantity - buyCommission;
                }
                else {
                    return (buyPrice - currentPrice) * quantity - buyCommission;
                }
            }
        }
        return 0;
    }

    double GetTotalCommission() const {
        return buyCommission + (isOpen ? 0 : sellCommission);
    }
};

struct Trade {
    int tradeNumber;
    int sellOrderId;
    std::vector<int> buyOrderIds;
    double totalPnL;
    double totalROI;
    double sellPrice;
    uint64_t sellTime;
    double totalInvestment;
    PositionMode mode;

    Trade() : tradeNumber(0), sellOrderId(-1), totalPnL(0.0), totalROI(0.0),
        sellPrice(0.0), sellTime(0.0), totalInvestment(0.0), mode(PositionMode::SPOT) {
    }
};

class TradingStatsManager {
private:
    std::vector<Order> allOrders;
    std::vector<Position> allPositions;
    std::vector<double> balanceHistory;
    std::vector<double> balanceTimepoints;
    std::vector<double> orderCandlesIDs;
    double startingBalance;
    double currentBalance;
    double totalShares;
    double fixedCommission;
    double fixedPercentCommission;
    double totalCommissionPaid;
    int nextOrderId;

    mutable bool statsCacheValid;
    mutable std::vector<Trade> cachedTrades;
    mutable double cachedWinRate;
    mutable double cachedAvgPnL;
    mutable double cachedBestTrade;
    mutable double cachedWorstTrade;

public:
    TradingStatsManager(double initial_balance = 10000.0, double commission = 0.0000);

    // Spot торговля
    int PlaceBuyOrder(uint64_t time, double price, double quantity);
    int PlaceSellOrder(uint64_t time, double price, double percentage);
    int PlaceSellOrderQuantity(uint64_t time, double price, double quantity);

    // Маржинальная торговля
    int EnterLong(uint64_t time, double price, double quantity, double leverage, double liquidationPrice = 0);
    int EnterShort(uint64_t time, double price, double quantity, double leverage, double liquidationPrice = 0);
    int CloseLong(uint64_t time, double price, double percentage);
    int CloseShort(uint64_t time, double price, double percentage);
    int CloseLongQuantity(uint64_t time, double price, double quantity);
    int CloseShortQuantity(uint64_t time, double price, double quantity);

    void ExecuteOrder(int orderId);

    // Управление комиссией
    void SetFixedCommission(double commission) { fixedCommission = commission; }
    double GetFixedCommission() const { return fixedCommission; }
    void SetPercentFixedCommission(double commission) { fixedPercentCommission = commission; }
    double GetPercentFixedCommission() const { return fixedPercentCommission; }
    double GetTotalCommissionPaid() const { return totalCommissionPaid; }

    // Управление позициями
    void UpdateOpenPositions();
    double GetUnrealizedPnL(double currentPrice) const;
    double GetRealizedPnL() const;
    bool CheckLiquidation(double currentPrice);

    // Управление балансом
    void SetStartingBalance(double balance);
    double GetStartingBalance() const { return startingBalance; }
    double GetCurrentBalance() const { return currentBalance; }
    double GetTotalShares() const { return totalShares; }
    void UpdateBalanceHistory();

    // Статистика
    double GetTotalPnL() const { return GetRealizedPnL(); }
    double GetTotalROI() const;
    double GetNetPnL() const { return GetRealizedPnL(); }
    size_t GetOpenPositionsCount() const;
    size_t GetClosedPositionsCount() const;
    size_t GetTotalOrdersCount() const { return allOrders.size(); }
    size_t GetTotalTradesCount() const;
    double GetWinRate() const;
    double GetAveragePnL() const;
    double GetBestTrade() const;
    double GetWorstTrade() const;

    // Доступ к данным
    const std::vector<Order>& GetOrders() const { return allOrders; }
    const std::vector<Position>& GetPositions() const { return allPositions; }
    std::vector<Position> GetOpenPositions() const;
    std::vector<Position> GetClosedPositions() const;
    std::vector<Position> GetOpenLongPositions() const;
    Position GetOpenLongPosition() const;
    std::vector<Position> GetOpenShortPositions() const;
    Position GetOpenShortPosition() const;
    const std::vector<double>& GetBalanceHistory() const { return balanceHistory; }
    std::vector<double>& GetOrdersCandleIDs() { return orderCandlesIDs; }
    const std::vector<Trade>& GetTrades() const;

    // Форматирование
    std::string FormatPrice(double price, int precision = 9) const;
    std::string FormatQuantity(double quantity, int precision = 6) const;
    std::string FormatPnL(double pnl, int precision = 6) const;

    // Утилиты
    void Reset();
    void Reset(double new_starting_balance);

private:
    void ProcessBuyOrder(const Order& order);
    void ProcessSellOrder(const Order& order);
    void ProcessSellOrderByPercentage(const Order& order);
    void ProcessSellOrderInternal(const Order& order, double totalQuantityToSell);

    // Маржинальные операции
    void ProcessLongOrder(const Order& order);
    void ProcessShortOrder(const Order& order);
    void ProcessCloseLongByPercentage(const Order& order);
    void ProcessCloseShortByPercentage(const Order& order);
    void ProcessCloseLong(const Order& order, double quantityToClose);
    void ProcessCloseShort(const Order& order, double quantityToClose);

    Position* FindOldestOpenPosition();
    std::vector<Position*> FindOpenMarginPositions(bool isLong);

    void InvalidateStatsCache();
    void RebuildStatsCache() const;
};