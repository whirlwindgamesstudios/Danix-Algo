#pragma once
#include "../../Public/Chart/CandleChartManager.h"
#include <imgui.h>
#include <vector>
#include <string>

struct TradingMark {
    size_t candleIndex;       
    double price;              
    ImU32 color;             
    std::string label;          

    TradingMark(size_t idx, double p, ImU32 c, const std::string& l)
        : candleIndex(idx), price(p), color(c), label(l.length() > 5 ? l.substr(0, 5) : l) {
    }

    bool operator==(const TradingMark& other) const {
        return candleIndex == other.candleIndex &&
            std::abs(price - other.price) < 0.0000001;    
    }
};

struct TradingLine {
    size_t startCandleIndex;      
    double startPrice;            
    size_t endCandleIndex;        
    double endPrice;              
    ImU32 color;                 
    float thickness;             

    TradingLine(size_t startIdx, double startP, size_t endIdx, double endP, ImU32 c, float t = 2.0f)
        : startCandleIndex(startIdx), startPrice(startP), endCandleIndex(endIdx),
        endPrice(endP), color(c), thickness(t) {
    }

    bool operator==(const TradingLine& other) const {
        return startCandleIndex == other.startCandleIndex &&
            endCandleIndex == other.endCandleIndex &&
            std::abs(startPrice - other.startPrice) < 0.0000001 &&
            std::abs(endPrice - other.endPrice) < 0.0000001;
    }
};

class CandlestickChart {
private:
    CandlestickDataManager* dataManager;

    double viewStartTime;           
    double viewEndTime;           
    double viewMinPrice;         
    double viewMaxPrice;         

    ImU32 bullishColor;
    ImU32 bearishColor;
    ImU32 wickColor;
    ImU32 gridColor;
    ImU32 textColor;
    ImU32 priceScaleColor;
    float candleWidth;
    float candleSpacing;

    bool isDragging;
    bool isPriceScaleDragging;
    ImVec2 lastMousePos;
    float priceScaleWidth;

    std::vector<TradingMark> tradingMarks;
    std::vector<TradingLine> tradingLines;

    void handleInput(const ImVec2& canvas_p0, const ImVec2& canvas_sz);
    void drawGrid(ImDrawList* draw_list, const ImVec2& canvas_p0, const ImVec2& canvas_sz);
    void drawCandles(ImDrawList* draw_list, const ImVec2& canvas_p0, const ImVec2& canvas_sz);
    void drawPriceScale(ImDrawList* draw_list, const ImVec2& canvas_p0, const ImVec2& canvas_sz);
    void drawCandleInfo(ImDrawList* draw_list, const ImVec2& canvas_p0, const MarketData& candle, const MarketData* prevCandle = nullptr);
    void drawCrosshair(ImDrawList* draw_list, const ImVec2& canvas_p0, const ImVec2& canvas_sz);

    void drawTradingMarks(ImDrawList* draw_list, const ImVec2& canvas_p0, const ImVec2& canvas_sz);
    void drawTradingLines(ImDrawList* draw_list, const ImVec2& canvas_p0, const ImVec2& canvas_sz);

    float timeToX(double time, const ImVec2& canvas_p0, const ImVec2& canvas_sz);
    float priceToY(double price, const ImVec2& canvas_p0, const ImVec2& canvas_sz);
    double xToTime(float x, const ImVec2& canvas_p0, const ImVec2& canvas_sz);
    double yToPrice(float y, const ImVec2& canvas_p0, const ImVec2& canvas_sz);

    const MarketData* getCandleUnderCursor(const ImVec2& canvas_p0, const ImVec2& canvas_sz, size_t* candleIndex = nullptr);
    double powf_custom(double base, double exp);
    int snprintf_custom(char* buffer, size_t size, const char* format, ...);

    std::string formatPrice(double price, int max_chars);

public:
    explicit CandlestickChart(CandlestickDataManager* manager);

    void fitPriceToView();
    void resetView();
    void render();

    void addTradingMark(size_t candleIndex, double price, ImU32 color, const std::string& label);
    void addTradingLine(size_t startCandleIndex, double startPrice, size_t endCandleIndex, double endPrice, ImU32 color, float thickness = 2.0f);

    void clearTradingMarks();
    void clearTradingLines();
    void clearAllTradingElements();
};