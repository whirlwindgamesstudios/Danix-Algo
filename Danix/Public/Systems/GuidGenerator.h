#pragma once
#include <random>
#include <sstream>
#include <string>
#include <iomanip>

class GUIDGenerator {
public:
    static std::string generate() {
        return getInstance().generateImpl();
    }

    static std::string generateSimple() {
        return getInstance().generateSimpleImpl();
    }

private:
    static GUIDGenerator& getInstance() {
        static GUIDGenerator instance;
        return instance;
    }

    std::random_device rd;
    std::mt19937 gen;
    std::uniform_int_distribution<> dis;

    GUIDGenerator() : gen(rd()), dis(0, 15) {}

    std::string generateImpl() {
        std::stringstream ss;
        ss << std::hex;

        for (int i = 0; i < 32; i++) {
            ss << dis(gen);
            if (i == 7 || i == 11 || i == 15 || i == 19) {
                ss << "-";
            }
        }

        return ss.str();
    }

    std::string generateSimpleImpl() {
        std::stringstream ss;
        ss << std::hex;

        for (int i = 0; i < 16; i++) {
            ss << dis(gen);
        }

        return ss.str();
    }
};