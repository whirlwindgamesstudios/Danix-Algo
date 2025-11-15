#ifndef CONFIG_H
#define CONFIG_H
#define _CRT_SECURE_NO_WARNINGS
#include <string>
#include <unordered_map>
#include <mutex>

class Config {
public:
    static Config& getInstance();
    std::string getValue(const std::string& section, const std::string& key) const;

    bool isDebugMode() const { return debugmode; }

    bool bybitIsDemoMode() const { return bybit_demo; }

    std::string getBirdeyeKey() const { return birdeyeAPIKey; }
    std::string getBybitAPIKey() const { return bibytAPIKey; }
    std::string getBybitSignature() const { return bibytSignature; }


private:
    Config();
    void loadConfig(const std::string& filename);
    void parseConfigFile(const std::string& filename);

    static Config* instance;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> configData;
    mutable std::mutex mtx;

    bool debugmode;

    bool bybit_demo;

    std::string birdeyeAPIKey;
    std::string bibytAPIKey;
    std::string bibytSignature;

};

#endif  