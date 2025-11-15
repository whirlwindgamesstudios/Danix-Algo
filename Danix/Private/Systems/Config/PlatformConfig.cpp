
#include "../../../Public/Systems/Config/PlatformConfig.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

Config* Config::instance = nullptr;

Config& Config::getInstance() {
    static Config instance;
    return instance;
}

Config::Config() {
    loadConfig("Config.ini");       
}

void Config::loadConfig(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mtx);
    parseConfigFile(filename);

    std::string valuedebugmode = getValue("system", "debug_mode");
    debugmode = (valuedebugmode == "true");

    std::string valuebybit_demo = getValue("exchanges", "bybit_demo");
    bybit_demo = (valuebybit_demo == "true");

    birdeyeAPIKey = getValue("keys", "birdeye_key");
    bibytAPIKey = getValue("keys", "bybit_key");
    bibytSignature = getValue("keys", "bibyt_signature");
}

std::string Config::getValue(const std::string& section, const std::string& key) const {
    auto sectionIt = configData.find(section);
    if (sectionIt != configData.end()) {
        auto keyIt = sectionIt->second.find(key);
        if (keyIt != sectionIt->second.end()) {
            return keyIt->second;
        }
    }
    throw std::runtime_error("Key not found in configuration");
}

void Config::parseConfigFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening configuration file: " << strerror(errno) << std::endl;
        throw std::runtime_error("Unable to open configuration file");
    }

    std::string line, section;
    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));
        line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);

        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue;
        }
        if (line[0] == '[' && line.back() == ']') {
            section = line.substr(1, line.size() - 2);
        }
        else {
            std::istringstream is_line(line);
            std::string key, value;
            if (std::getline(is_line, key, '=') && std::getline(is_line, value)) {
                key.erase(0, key.find_first_not_of(" \t\n\r\f\v"));
                key.erase(key.find_last_not_of(" \t\n\r\f\v") + 1);
                value.erase(0, value.find_first_not_of(" \t\n\r\f\v"));
                value.erase(value.find_last_not_of(" \t\n\r\f\v") + 1);
                configData[section][key] = value;
            }
        }
    }
    file.close();
}