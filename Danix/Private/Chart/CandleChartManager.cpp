#include "../../Public/Chart/CandleChartManager.h"
#include <algorithm>
#include "../../Public/Chart/CandleChart.h"
#include "../../Public/Blueprints/BlueprintManager.h"


#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))


float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

CandlestickDataManager::CandlestickDataManager()
{

}

void CandlestickDataManager::addData(const std::vector<MarketData>& newData) {
    data.insert(data.end(), newData.begin(), newData.end());
    std::sort(data.begin(), data.end(), [](const MarketData& a, const MarketData& b) {
        if (a.timestamp <= 0 && a.timestamp <= 0) std::cout << "Error in timestamp" << std::endl;
        return a.timestamp < b.timestamp;
        });

    public_data.insert(public_data.end(), newData.begin(), newData.end());
    std::sort(public_data.begin(), public_data.end(), [](const MarketData& a, const MarketData& b) {
        return a.timestamp < b.timestamp;
        });
}

void CandlestickDataManager::addCandle(const MarketData& candle) {
    if (candle.timestamp <= 0) std::cout << "Error in timestamp" << std::endl;
    auto it = std::lower_bound(data.begin(), data.end(), candle,
        [](const MarketData& a, const MarketData& b) {
            return a.timestamp < b.timestamp;
        });
    data.insert(it, candle);

    auto it1 = std::lower_bound(public_data.begin(), public_data.end(), candle,
        [](const MarketData& a, const MarketData& b) {
            return a.timestamp < b.timestamp;
        });
    public_data.insert(it1, candle);

    if (RuntimeMode)
    {
        GetBlueprintManager()->executeFromEntry();
    }
}

void CandlestickDataManager::UpdateLastCandle(const MarketData& candle) {
    if (data.empty())
        return;

    MarketData& lastCandle = data.back();
    lastCandle = candle;



    if (!public_data.empty()) {
        MarketData& lastPublicCandle = public_data.back();
        if (lastPublicCandle.timestamp == candle.timestamp) {
            lastPublicCandle = candle;
        }
    }

    if (RuntimeMode) {
        GetBlueprintManager()->executeFromEntry();
    }
}



const std::vector<MarketData>& CandlestickDataManager::getData() const {
    return data;
}

std::vector<MarketData> CandlestickDataManager::getDataRange(size_t start, size_t end) const {
    if (start >= data.size()) return {};
    end = MIN(end, data.size());
    return std::vector<MarketData>(data.begin() + start, data.begin() + end);
}

const std::vector<MarketData>& CandlestickDataManager::getPublicData() const {
    return public_data;
}

std::vector<MarketData>* CandlestickDataManager::getPublicDataPtr(){
    return &public_data;
}

std::vector<MarketData> CandlestickDataManager::getPublicDataRange(size_t start, size_t end) const {
    if (start >= public_data.size()) return {};
    end = MIN(end, public_data.size());
    return std::vector<MarketData>(public_data.begin() + start, public_data.begin() + end);
}


void CandlestickDataManager::clear() {
    data.clear();
    public_data.clear();
    chart->clearAllTradingElements();
}

size_t CandlestickDataManager::size() const {
    return data.size();
}

bool CandlestickDataManager::empty() const {
    return data.empty();
}

std::pair<double, double> CandlestickDataManager::getPriceRange(size_t start, size_t end) const {
    if (data.empty()) return std::make_pair(0.0, 0.0);

    end = MIN(end, data.size());
    if (start >= end) return std::make_pair(0.0, 0.0);

    double minPrice = data[start].low;
    double maxPrice = data[start].high;

    for (size_t i = start; i < end; ++i) {
        minPrice = MIN(minPrice, data[i].low);
        maxPrice = MAX(maxPrice, data[i].high);
    }

    return std::make_pair(minPrice, maxPrice);
}
