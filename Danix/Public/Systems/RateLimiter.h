#pragma once
#include <mutex>
#include <queue>
#include <chrono>

class RateLimiter {
private:
    std::mutex mtx;
    std::condition_variable cv;
    std::queue<std::chrono::steady_clock::time_point> requests;
    const size_t maxRequests;
    const std::chrono::milliseconds timeWindow;

public:
    RateLimiter(size_t max = 15, double windowSeconds = 1.0)
        : maxRequests(max), timeWindow(std::chrono::milliseconds(static_cast<int>(windowSeconds * 1000))) {
    }

    bool tryAcquire() {
        std::lock_guard<std::mutex> lock(mtx);
        auto now = std::chrono::steady_clock::now();

        while (!requests.empty() && now - requests.front() > timeWindow) {
            requests.pop();
        }

        if (requests.size() < maxRequests) {
            requests.push(now);
            return true;
        }
        return false;
    }

    void acquire() {
        std::unique_lock<std::mutex> lock(mtx);
        auto now = std::chrono::steady_clock::now();

        while (!requests.empty() && now - requests.front() > timeWindow) {
            requests.pop();
        }

        if (requests.size() < maxRequests) {
            requests.push(now);
            return;
        }

        auto oldestRequestExpiry = requests.front() + timeWindow;

        cv.wait_until(lock, oldestRequestExpiry, [this, now]() {
            return requests.size() < maxRequests;
            });

        now = std::chrono::steady_clock::now();
        requests.push(now);
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mtx);
        while (!requests.empty()) {
            requests.pop();
        }
    }
};