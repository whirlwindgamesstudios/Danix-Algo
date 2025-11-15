
#include "../../Public/Chart/CandleChart.h"
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <chrono>

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

float clampffff(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}


CandlestickChart::CandlestickChart(CandlestickDataManager* manager)
    : dataManager(manager)
    , viewStartTime(0.0)
    , viewEndTime(100.0)      
    , viewMinPrice(0.0)
    , viewMaxPrice(100.0)
    , bullishColor(IM_COL32(26, 152, 129, 255))     
    , bearishColor(IM_COL32(239, 83, 80, 255))      
    , wickColor(IM_COL32(134, 142, 147, 255))      
    , gridColor(IM_COL32(42, 46, 57, 255))          
    , textColor(IM_COL32(209, 212, 220, 255))       
    , priceScaleColor(IM_COL32(17, 17, 17, 255))     
    , candleWidth(8.0f)
    , candleSpacing(2.0f)
    , isDragging(false)
    , isPriceScaleDragging(false)
    , lastMousePos(0, 0)
    , priceScaleWidth(60.0f) {

    if (dataManager && !dataManager->empty()) {
        resetView();
        dataManager->SetChart(this);
    }
    else if(dataManager) dataManager->SetChart(this);

}

double CandlestickChart::powf_custom(double base, double exp) {
    return std::pow(base, exp);
}

int CandlestickChart::snprintf_custom(char* buffer, size_t size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vsnprintf(buffer, size, format, args);
    va_end(args);
    return result;
}

void CandlestickChart::render() {
    if (!dataManager || dataManager->empty()) {
        ImGui::Text("No data to display");
        return;
    }

    ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
    ImVec2 canvas_sz = ImGui::GetContentRegionAvail(); 
    if (canvas_sz.x < 100.0f) canvas_sz.x = 100.0f;
    if (canvas_sz.y < 100.0f) canvas_sz.y = 100.0f;
    ImGui::InvisibleButton("chart_canvas", canvas_sz,
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
    handleInput(canvas_p0, canvas_sz);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 chart_area = ImVec2(canvas_sz.x - priceScaleWidth, canvas_sz.y);
    draw_list->AddRectFilled(canvas_p0,
        ImVec2(canvas_p0.x + chart_area.x, canvas_p0.y + chart_area.y),
        IM_COL32(17, 17, 17, 255));
    draw_list->PushClipRect(canvas_p0, ImVec2(canvas_p0.x + chart_area.x, canvas_p0.y + chart_area.y), true);
    drawGrid(draw_list, canvas_p0, chart_area);

    drawCandles(draw_list, canvas_p0, chart_area);
    drawTradingLines(draw_list, canvas_p0, chart_area);
    drawTradingMarks(draw_list, canvas_p0, chart_area);
    if (ImGui::IsItemHovered()) {
        size_t candleIndex;
        const MarketData* currentCandle = getCandleUnderCursor(canvas_p0, chart_area, &candleIndex);
        if (currentCandle) {
            const auto& data = dataManager->getData();
            const MarketData* prevCandle = (candleIndex > 0) ? &data[candleIndex - 1] : nullptr;
            drawCandleInfo(draw_list, canvas_p0, *currentCandle, prevCandle);
        }
        drawCrosshair(draw_list, canvas_p0, chart_area);
    }
    draw_list->PopClipRect();
    drawPriceScale(draw_list, canvas_p0, canvas_sz);
    ImVec2 full_area_end = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);
    draw_list->AddRect(canvas_p0, full_area_end, IM_COL32(60, 60, 60, 255), 0.0f, 0, 2.0f);
    draw_list->AddRect(
        ImVec2(canvas_p0.x + 1, canvas_p0.y + 1),
        ImVec2(full_area_end.x - 1, full_area_end.y - 1),
        IM_COL32(40, 40, 40, 150), 0.0f, 0, 1.0f);
}

void CandlestickChart::handleInput(const ImVec2& canvas_p0, const ImVec2& canvas_sz) {
    if (!ImGui::IsItemHovered()) return;

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 mouse_pos = io.MousePos;
    ImVec2 chart_area = ImVec2(canvas_sz.x - priceScaleWidth, canvas_sz.y);

    bool mouse_on_price_scale = (mouse_pos.x >= canvas_p0.x + chart_area.x &&
        mouse_pos.x <= canvas_p0.x + canvas_sz.x);

    if (io.MouseWheel != 0.0f) {
        if (mouse_on_price_scale) {
            double center_price = yToPrice(mouse_pos.y, canvas_p0, chart_area);
            double price_range = viewMaxPrice - viewMinPrice;
            double zoom_factor = powf_custom(0.9, io.MouseWheel);
            double new_range = price_range * zoom_factor;

            viewMinPrice = center_price - new_range * ((center_price - viewMinPrice) / price_range);
            viewMaxPrice = center_price + new_range * ((viewMaxPrice - center_price) / price_range);
        }
        else {
            double center_time = xToTime(mouse_pos.x, canvas_p0, chart_area);
            double time_range = viewEndTime - viewStartTime;
            double zoom_factor = powf_custom(0.9, io.MouseWheel);
            double new_range = time_range * zoom_factor;

            new_range = MAX(new_range, 5.0);

            viewStartTime = center_time - new_range * ((center_time - viewStartTime) / time_range);
            viewEndTime = center_time + new_range * ((viewEndTime - center_time) / time_range);
        }
    }

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        isDragging = true;
        isPriceScaleDragging = mouse_on_price_scale;
        lastMousePos = mouse_pos;
    }

    if (isDragging && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        ImVec2 delta = ImVec2(mouse_pos.x - lastMousePos.x, mouse_pos.y - lastMousePos.y);

        if (isPriceScaleDragging || !mouse_on_price_scale) {
            double price_per_pixel = (viewMaxPrice - viewMinPrice) / chart_area.y;
            double price_delta = delta.y * price_per_pixel;
            viewMinPrice += price_delta;
            viewMaxPrice += price_delta;

            double time_per_pixel = (viewEndTime - viewStartTime) / chart_area.x;
            double time_delta = -delta.x * time_per_pixel;     

            viewStartTime += time_delta;
            viewEndTime += time_delta;
        }

        lastMousePos = mouse_pos;
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        isDragging = false;
        isPriceScaleDragging = false;
    }

    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        if (mouse_on_price_scale) {
            fitPriceToView();
        }
        else {
            resetView();
        }
    }
}

float CandlestickChart::timeToX(double time, const ImVec2& canvas_p0, const ImVec2& canvas_sz) {
    return canvas_p0.x + static_cast<float>((time - viewStartTime) / (viewEndTime - viewStartTime)) * canvas_sz.x;
}

float CandlestickChart::priceToY(double price, const ImVec2& canvas_p0, const ImVec2& canvas_sz) {
    return canvas_p0.y + canvas_sz.y - static_cast<float>((price - viewMinPrice) / (viewMaxPrice - viewMinPrice)) * canvas_sz.y;
}

double CandlestickChart::xToTime(float x, const ImVec2& canvas_p0, const ImVec2& canvas_sz) {
    return viewStartTime + ((x - canvas_p0.x) / canvas_sz.x) * (viewEndTime - viewStartTime);
}

double CandlestickChart::yToPrice(float y, const ImVec2& canvas_p0, const ImVec2& canvas_sz) {
    return viewMinPrice + (1.0 - (y - canvas_p0.y) / canvas_sz.y) * (viewMaxPrice - viewMinPrice);
}

void CandlestickChart::drawGrid(ImDrawList* draw_list, const ImVec2& canvas_p0, const ImVec2& canvas_sz) {
    int price_lines = 8;
    for (int i = 1; i < price_lines; ++i) {
        double price = viewMinPrice + (viewMaxPrice - viewMinPrice) * i / price_lines;
        float y = priceToY(price, canvas_p0, canvas_sz);

        draw_list->AddLine(ImVec2(canvas_p0.x, y),
            ImVec2(canvas_p0.x + canvas_sz.x, y), gridColor, 1.0f);
    }

    double time_range = viewEndTime - viewStartTime;
    int time_lines = MAX(4, MIN(12, static_cast<int>(time_range / 10.0)));

    for (int i = 1; i < time_lines; ++i) {
        double time = viewStartTime + time_range * i / time_lines;
        float x = timeToX(time, canvas_p0, canvas_sz);

        if (x >= canvas_p0.x && x <= canvas_p0.x + canvas_sz.x) {
            draw_list->AddLine(ImVec2(x, canvas_p0.y),
                ImVec2(x, canvas_p0.y + canvas_sz.y), gridColor, 1.0f);
        }
    }
}

void CandlestickChart::drawCandles(ImDrawList* draw_list, const ImVec2& canvas_p0, const ImVec2& canvas_sz) {
    std::vector<MarketData> data = dataManager->getData();

    if (data.empty()) return;

    size_t data_size = data.size();
    size_t start_idx = static_cast<size_t>(MAX(0.0, floor(viewStartTime)));
    size_t end_idx = static_cast<size_t>(MIN(static_cast<double>(data_size), ceil(viewEndTime)));

    if (end_idx > data_size) end_idx = data_size;
    if (start_idx >= data_size) start_idx = 0;
    if (start_idx >= end_idx) return;

    double time_range = viewEndTime - viewStartTime;
    if (time_range <= 0) return;

    float candle_width_px = canvas_sz.x / static_cast<float>(time_range);
    candle_width_px = clampffff(candle_width_px * 0.8f, 1.0f, 20.0f);

    for (size_t i = start_idx; i < end_idx; ++i) {
        const auto& candle = data[i];

        float center_x = timeToX(static_cast<double>(i), canvas_p0, canvas_sz);
        float candle_left = center_x - candle_width_px * 0.5f;
        float candle_right = center_x + candle_width_px * 0.5f;

        if (candle_right < canvas_p0.x || candle_left > canvas_p0.x + canvas_sz.x)
            continue;

        float y_open = priceToY(candle.open, canvas_p0, canvas_sz);
        float y_close = priceToY(candle.close, canvas_p0, canvas_sz);
        float y_high = priceToY(candle.high, canvas_p0, canvas_sz);
        float y_low = priceToY(candle.low, canvas_p0, canvas_sz);

        bool is_bullish = candle.close > candle.open;
        ImU32 candle_color = is_bullish ? bullishColor : bearishColor;

        draw_list->AddLine(ImVec2(center_x, y_high),
            ImVec2(center_x, y_low),
            wickColor, 1.0f);

        float body_top = MIN(y_open, y_close);
        float body_bottom = MAX(y_open, y_close);
        float body_height = body_bottom - body_top;

        if (body_height < 1.0f) {
            draw_list->AddLine(ImVec2(candle_left, body_top),
                ImVec2(candle_right, body_top),
                candle_color, 1.5f);
        }
        else {
            draw_list->AddRectFilled(ImVec2(candle_left, body_top),
                ImVec2(candle_right, body_bottom),
                candle_color);
        }
    }
}

void CandlestickChart::drawPriceScale(ImDrawList* draw_list, const ImVec2& canvas_p0, const ImVec2& canvas_sz) {
    float extended_width = 20.0f;
    ImVec2 scale_p0 = ImVec2(canvas_p0.x + canvas_sz.x - (priceScaleWidth + extended_width), canvas_p0.y);
    ImVec2 scale_sz = ImVec2(priceScaleWidth + extended_width, canvas_sz.y);

    draw_list->AddRectFilled(scale_p0,
        ImVec2(scale_p0.x + scale_sz.x, scale_p0.y + scale_sz.y), priceScaleColor);

    draw_list->AddLine(ImVec2(scale_p0.x, scale_p0.y),
        ImVec2(scale_p0.x, scale_p0.y + scale_sz.y), gridColor, 1.0f);

    int price_labels = 8;
    for (int i = 0; i <= price_labels; ++i) {
        double price = viewMinPrice + (viewMaxPrice - viewMinPrice) * i / price_labels;
        float y = priceToY(price, canvas_p0, ImVec2(canvas_sz.x - priceScaleWidth, canvas_sz.y));

        char buf[64];
        snprintf(buf, sizeof(buf), "%.12f", price);
        std::string text(buf);

        while (!text.empty() && text.back() == '0') text.pop_back();
        if (!text.empty() && text.back() == '.') text.pop_back();

        std::string main_part = text;
        std::string zeros_count_str;
        std::string tail;

        size_t dot_pos = text.find('.');
        if (dot_pos != std::string::npos) {
            size_t zero_count = 0;
            size_t i2 = dot_pos + 1;
            while (i2 < text.size() && text[i2] == '0') {
                zero_count++;
                i2++;
            }
            if (zero_count >= 4) {   
                main_part = text.substr(0, dot_pos + 1);  
                zeros_count_str = std::to_string(zero_count);
                tail = text.substr(dot_pos + 1 + zero_count);
            }
        }

        ImVec2 text_size = ImGui::CalcTextSize("123456789");    
        float text_y = y - text_size.y * 0.5f;
        text_y = MAX(text_y, scale_p0.y);
        text_y = MIN(text_y, scale_p0.y + scale_sz.y - text_size.y);
        ImVec2 text_pos = ImVec2(scale_p0.x + 5, text_y);

        draw_list->AddRectFilled(ImVec2(text_pos.x - 3, text_pos.y),
            ImVec2(text_pos.x + extended_width - 5, text_pos.y + text_size.y), priceScaleColor);

        int used_chars = 0;
        ImVec2 cur = text_pos;

        if (!main_part.empty() && used_chars < 9) {
            int remaining = 9 - used_chars;
            std::string part_to_draw = main_part;
            if ((int)part_to_draw.length() > remaining) {
                part_to_draw = part_to_draw.substr(0, remaining);
            }
            draw_list->AddText(cur, textColor, part_to_draw.c_str());
            cur.x += ImGui::CalcTextSize(part_to_draw.c_str()).x;
            used_chars += (int)part_to_draw.length();
        }

        if (!zeros_count_str.empty() && used_chars < 9) {
            int remaining = 9 - used_chars;
            std::string part_to_draw = zeros_count_str;
            if ((int)part_to_draw.length() > remaining) {
                part_to_draw = part_to_draw.substr(0, remaining);
            }
            float small_font_size = ImGui::GetFontSize() * 0.9f;
            draw_list->AddText(ImGui::GetFont(), small_font_size,
                ImVec2(cur.x, cur.y + 1), IM_COL32(180, 180, 180, 255), part_to_draw.c_str());
            cur.x += ImGui::CalcTextSize(part_to_draw.c_str()).x * 0.9f;
            used_chars += (int)part_to_draw.length();
        }

        if (!tail.empty() && used_chars < 9) {
            int remaining = 9 - used_chars;
            std::string part_to_draw = tail;
            if ((int)part_to_draw.length() > remaining) {
                part_to_draw = part_to_draw.substr(0, remaining);
            }
            draw_list->AddText(cur, textColor, part_to_draw.c_str());
        }
    }
}


std::string CandlestickChart::formatPrice(double price, int max_chars) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%.12f", price);
    std::string text(buf);

    while (!text.empty() && text.back() == '0') {
        text.pop_back();
    }
    if (!text.empty() && text.back() == '.') {
        text.pop_back();
    }

    if (text.length() <= max_chars) {
        return text;
    }

    size_t dot_pos = text.find('.');
    if (dot_pos != std::string::npos) {
        int before_dot = dot_pos + 1;
        int available_after_dot = max_chars - before_dot;

        if (available_after_dot > 0) {
            text = text.substr(0, dot_pos + 1 + available_after_dot);

            while (!text.empty() && text.back() == '0') {
                text.pop_back();
            }
            if (!text.empty() && text.back() == '.') {
                text.pop_back();
            }
        }
        else {
            text = text.substr(0, max_chars);
        }
    }
    else {
        text = text.substr(0, max_chars);
    }

    if (text.length() > max_chars) {
        text = text.substr(0, max_chars);
    }

    return text;
}


const MarketData* CandlestickChart::getCandleUnderCursor(const ImVec2& canvas_p0, const ImVec2& canvas_sz, size_t* candleIndex) {
    ImGuiIO& io = ImGui::GetIO();
    double time = xToTime(io.MousePos.x, canvas_p0, canvas_sz);

    size_t idx = static_cast<size_t>(clampffff(round(time), 0.0, static_cast<double>(dataManager->size() - 1)));

    if (idx < dataManager->size()) {
        if (candleIndex) *candleIndex = idx;
        return &dataManager->getData()[idx];
    }

    return nullptr;
}

void CandlestickChart::drawCandleInfo(ImDrawList* draw_list, const ImVec2& canvas_p0,
    const MarketData& candle, const MarketData* prevCandle) {

    const float padding = 10.0f;
    ImVec2 text_pos = ImVec2(canvas_p0.x + padding, canvas_p0.y + padding);

    char buffer[512];

    char ohlc_part[128];
    snprintf_custom(ohlc_part, sizeof(ohlc_part), "ID %d OPEN %.8Lf  HIGH %.8Lf  LOW %.8Lf  CLOSE %.8Lf",
        candle.index,candle.open, candle.high, candle.low, candle.close);

    char volume_part[64];
    if (candle.volume > 1000000) {
        snprintf_custom(volume_part, sizeof(volume_part), "  VOL %.1fM", candle.volume / 1000000.0);
    }
    else if (candle.volume > 1000) {
        snprintf_custom(volume_part, sizeof(volume_part), "  VOL %.1fK", candle.volume / 1000.0);
    }
    else {
        snprintf_custom(volume_part, sizeof(volume_part), "  VOL %.0f", candle.volume);
    }

    char mcap_part[64] = "";
    if (candle.mcap > 0) {
        if (candle.mcap >= 1000000000.0) {
            snprintf_custom(mcap_part, sizeof(mcap_part), "  MCAP $%.1fB", candle.mcap / 1000000000.0);
        }
        else if (candle.mcap >= 1000000.0) {
            snprintf_custom(mcap_part, sizeof(mcap_part), "  MCAP $%.1fM", candle.mcap / 1000000.0);
        }
        else if (candle.mcap >= 1000.0) {
            snprintf_custom(mcap_part, sizeof(mcap_part), "  MCAP $%.1fK", candle.mcap / 1000.0);
        }
        else {
            snprintf_custom(mcap_part, sizeof(mcap_part), "  MCAP $%.0f", candle.mcap);
        }
    }


    snprintf_custom(buffer, sizeof(buffer), "%s%s%s", ohlc_part, volume_part, mcap_part);
    draw_list->AddText(text_pos, textColor, buffer);

    if (prevCandle) {
        double change = candle.close - prevCandle->close;
        double change_percent = (change / prevCandle->close) * 100.0;

        ImU32 change_color = (change >= 0) ? bullishColor : bearishColor;
        const char* sign = (change >= 0) ? "+" : "";

        char change_part[64];
        snprintf_custom(change_part, sizeof(change_part), "  %s%.2f%% (%s$%.10Lf)",
            sign, change_percent, sign, change);

        ImVec2 main_text_size = ImGui::CalcTextSize(buffer);
        ImVec2 change_pos = ImVec2(text_pos.x + main_text_size.x, text_pos.y);

        draw_list->AddText(change_pos, change_color, change_part);
    }
}

void CandlestickChart::drawCrosshair(ImDrawList* draw_list, const ImVec2& canvas_p0, const ImVec2& canvas_sz) {
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 mouse_pos = io.MousePos;

    if (mouse_pos.x < canvas_p0.x || mouse_pos.x > canvas_p0.x + canvas_sz.x ||
        mouse_pos.y < canvas_p0.y || mouse_pos.y > canvas_p0.y + canvas_sz.y) {
        return;
    }

    ImU32 crosshair_color = IM_COL32(120, 123, 134, 200);

    draw_list->AddLine(ImVec2(canvas_p0.x, mouse_pos.y),
        ImVec2(canvas_p0.x + canvas_sz.x, mouse_pos.y), crosshair_color, 1.0f);

    draw_list->AddLine(ImVec2(mouse_pos.x, canvas_p0.y),
        ImVec2(mouse_pos.x, canvas_p0.y + canvas_sz.y), crosshair_color, 1.0f);
}

void CandlestickChart::fitPriceToView() {
    if (!dataManager || dataManager->empty()) return;

    size_t start_idx = static_cast<size_t>(MAX(0.0, floor(viewStartTime)));
    size_t end_idx = static_cast<size_t>(MIN(static_cast<double>(dataManager->size()), ceil(viewEndTime)));

    if (start_idx >= end_idx) return;

    auto price_range = dataManager->getPriceRange(start_idx, end_idx);
    double range = price_range.second - price_range.first;

    viewMinPrice = price_range.first - range * 0.05;    
    viewMaxPrice = price_range.second + range * 0.05;
}

void CandlestickChart::resetView() {
    if (dataManager == nullptr && dataManager->empty()) return;

    size_t total_candles = dataManager->size();

    viewEndTime = static_cast<double>(total_candles);
    viewStartTime = MAX(0.0, static_cast<double>(total_candles) - 100.0);

    fitPriceToView();
}

void CandlestickChart::addTradingMark(size_t candleIndex, double price, ImU32 color, const std::string& label) {
    TradingMark newMark(candleIndex, price, color, label);

    for (const auto& existingMark : tradingMarks) {
        if (existingMark == newMark) {
            return;     
        }
    }

    tradingMarks.emplace_back(newMark);
}

void CandlestickChart::addTradingLine(size_t startCandleIndex, double startPrice, size_t endCandleIndex, double endPrice, ImU32 color, float thickness) {
    TradingLine newLine(startCandleIndex, startPrice, endCandleIndex, endPrice, color, thickness);

    for (const auto& existingLine : tradingLines) {
        if (existingLine == newLine) {
            return;     
        }
    }

    tradingLines.emplace_back(newLine);
}

void CandlestickChart::drawTradingMarks(ImDrawList* draw_list, const ImVec2& canvas_p0, const ImVec2& canvas_sz) {
    const float mark_radius = 8.0f;
    const float text_offset = 12.0f;

    for (const auto& mark : tradingMarks) {
        double time = static_cast<double>(mark.candleIndex);
        if (time < viewStartTime || time > viewEndTime) continue;
        if (mark.price < viewMinPrice || mark.price > viewMaxPrice) continue;

        float x = timeToX(time, canvas_p0, canvas_sz);
        float y = priceToY(mark.price, canvas_p0, canvas_sz);

        if (x < canvas_p0.x || x > canvas_p0.x + canvas_sz.x ||
            y < canvas_p0.y || y > canvas_p0.y + canvas_sz.y) continue;

        ImVec2 center(x, y);

        draw_list->AddCircleFilled(center, mark_radius, mark.color);

        draw_list->AddCircle(center, mark_radius, IM_COL32(255, 255, 255, 150), 0, 1.5f);

        if (!mark.label.empty()) {
            ImVec2 text_size = ImGui::CalcTextSize(mark.label.c_str());
            ImVec2 text_pos = ImVec2(center.x - text_size.x * 0.5f, center.y + text_offset);

            if (text_pos.y + text_size.y > canvas_p0.y + canvas_sz.y) {
                text_pos.y = center.y - text_offset - text_size.y;   
            }

            draw_list->AddRectFilled(
                ImVec2(text_pos.x - 2, text_pos.y - 1),
                ImVec2(text_pos.x + text_size.x + 2, text_pos.y + text_size.y + 1),
                IM_COL32(0, 0, 0, 180)
            );

            draw_list->AddText(text_pos, textColor, mark.label.c_str());
        }
    }
}

void CandlestickChart::drawTradingLines(ImDrawList* draw_list, const ImVec2& canvas_p0, const ImVec2& canvas_sz) {
    for (const auto& line : tradingLines) {
        double start_time = static_cast<double>(line.startCandleIndex);
        float start_x = timeToX(start_time, canvas_p0, canvas_sz);
        float start_y = priceToY(line.startPrice, canvas_p0, canvas_sz);

        double end_time = static_cast<double>(line.endCandleIndex);
        float end_x = timeToX(end_time, canvas_p0, canvas_sz);
        float end_y = priceToY(line.endPrice, canvas_p0, canvas_sz);

        ImVec2 start_point(start_x, start_y);
        ImVec2 end_point(end_x, end_y);

        bool start_visible = (start_x >= canvas_p0.x && start_x <= canvas_p0.x + canvas_sz.x &&
            start_y >= canvas_p0.y && start_y <= canvas_p0.y + canvas_sz.y);
        bool end_visible = (end_x >= canvas_p0.x && end_x <= canvas_p0.x + canvas_sz.x &&
            end_y >= canvas_p0.y && end_y <= canvas_p0.y + canvas_sz.y);

        if (start_visible || end_visible ||
            (start_x < canvas_p0.x && end_x > canvas_p0.x + canvas_sz.x) ||
            (start_y < canvas_p0.y && end_y > canvas_p0.y + canvas_sz.y)) {

            draw_list->AddLine(start_point, end_point, line.color, line.thickness);

            if (start_visible) {
                draw_list->AddCircleFilled(start_point, 3.0f, line.color);
            }
            if (end_visible) {
                draw_list->AddCircleFilled(end_point, 3.0f, line.color);
            }
        }
    }
}

void CandlestickChart::clearTradingMarks() {
    tradingMarks.clear();
}

void CandlestickChart::clearTradingLines() {
    tradingLines.clear();
}

void CandlestickChart::clearAllTradingElements() {
    tradingMarks.clear();
    tradingLines.clear();
}