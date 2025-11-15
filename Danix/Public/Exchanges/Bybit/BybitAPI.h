#pragma once

#include <string>
#include <map>
#include <memory>
#include <curl/curl.h>

namespace Bybit {

class BybitAPI {
public:
    BybitAPI();
    virtual ~BybitAPI();
    
    void setApiKeys(const std::string& apiKey, const std::string& apiSecret);
    
    void setTestnet(bool isTestnet);
    
    void setTimeout(long timeout);
    
protected:
    std::string get(const std::string& endpoint, const std::map<std::string, std::string>& params, bool needAuth);
    std::string post(const std::string& endpoint, const std::string& body, bool needAuth);
    
    std::string buildUrl(const std::string& endpoint, const std::map<std::string, std::string>& params) const;
    
    std::string signRequest(const std::string& timestamp, const std::string& params) const;
    
    std::string getTimestamp() const;
    
    std::string urlEncode(const std::string& value) const;
    
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
    
private:
    std::string m_apiKey;
    std::string m_apiSecret;
    std::string m_baseUrl;
    long m_timeout;
    CURL* m_curl;
    bool isDebugAPI;
    
    void initCurl();
};

}   
