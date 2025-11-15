#include "../../../Public/Exchanges/Bybit/BybitAPI.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <openssl/hmac.h>
#include <openssl/sha.h>

namespace Bybit {

BybitAPI::BybitAPI() 
    : m_baseUrl("https://api-demo.bybit.com")
    , m_timeout(10)
    , m_curl(nullptr)
{
    initCurl();
}

BybitAPI::~BybitAPI() {
    if (m_curl) {
        curl_easy_cleanup(m_curl);
    }
}

void BybitAPI::setApiKeys(const std::string& apiKey, const std::string& apiSecret) {
    m_apiKey = apiKey;
    m_apiSecret = apiSecret;
}

void BybitAPI::setTestnet(bool isTestnet) {
    if (isTestnet) {
        m_baseUrl = "https://api-demo.bybit.com";
    } else {
        m_baseUrl = "https://api.bybit.com";
    }
}

void BybitAPI::setTimeout(long timeout) {
    m_timeout = timeout;
}

void BybitAPI::initCurl() {
    m_curl = curl_easy_init();
}

size_t BybitAPI::writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string BybitAPI::getTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return std::to_string(ms.count());
}

std::string BybitAPI::urlEncode(const std::string& value) const {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : value) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << std::uppercase;
            escaped << '%' << std::setw(2) << int((unsigned char)c);
            escaped << std::nouppercase;
        }
    }

    return escaped.str();
}

std::string BybitAPI::buildUrl(const std::string& endpoint, const std::map<std::string, std::string>& params) const {
    std::string url = m_baseUrl + endpoint;
    
    if (!params.empty()) {
        url += "?";
        bool first = true;
        for (const auto& pair : params) {
            if (!first) url += "&";
            url += urlEncode(pair.first) + "=" + urlEncode(pair.second);
            first = false;
        }
    }
    
    return url;
}

std::string BybitAPI::signRequest(const std::string& timestamp, const std::string& params) const {
    std::string toSign = timestamp + m_apiKey + "5000" + params;
    
    unsigned char* digest = HMAC(EVP_sha256(), 
                                  m_apiSecret.c_str(), 
                                  m_apiSecret.length(),
                                  (unsigned char*)toSign.c_str(), 
                                  toSign.length(), 
                                  nullptr, 
                                  nullptr);
    
    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
    }
    
    return oss.str();
}

int curlDebugCallback(CURL* handle, curl_infotype type, char* data, size_t size, void* userptr) {
    switch (type) {
    case CURLINFO_TEXT:
        std::cerr << "[CURL TEXT] " << std::string(data, size);
        break;
    case CURLINFO_HEADER_IN:
        std::cerr << "[HEADER IN] " << std::string(data, size);
        break;
    case CURLINFO_HEADER_OUT:
        std::cerr << "[HEADER OUT] " << std::string(data, size);
        break;
    case CURLINFO_DATA_IN:
        std::cerr << "[DATA IN] " << std::string(data, size);
        break;
    case CURLINFO_DATA_OUT:
        std::cerr << "[DATA OUT] " << std::string(data, size);
        break;
    default:
        break;
    }
    return 0;
}

std::string BybitAPI::get(const std::string& endpoint, const std::map<std::string, std::string>& params, bool needAuth) {
    if (!m_curl) return "{\"error\":\"CURL not initialized\"}";
    
    std::string response;
    std::string url = buildUrl(endpoint, params);
    
    std::string queryString;
    if (!params.empty()) {
        bool first = true;
        for (const auto& pair : params) {
            if (!first) queryString += "&";
            queryString += pair.first + "=" + pair.second;
            first = false;
        }
    }
    
    struct curl_slist* headers = nullptr;
    
    if (needAuth) {
        std::string timestamp = getTimestamp();
        std::string signature = signRequest(timestamp, queryString);
        
        headers = curl_slist_append(headers, ("X-BAPI-API-KEY: " + m_apiKey).c_str());
        headers = curl_slist_append(headers, ("X-BAPI-SIGN: " + signature).c_str());
        headers = curl_slist_append(headers, ("X-BAPI-TIMESTAMP: " + timestamp).c_str());
        headers = curl_slist_append(headers, "X-BAPI-RECV-WINDOW: 5000");

    }
    
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(m_curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(m_curl, CURLOPT_POST, 0L);

    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response);
    //curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1L);
    //curl_easy_setopt(m_curl, CURLOPT_DEBUGFUNCTION, curlDebugCallback);
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, m_timeout);
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 2L);
    
    CURLcode res = curl_easy_perform(m_curl);
    
    if (headers) {
        curl_slist_free_all(headers);
    }
    
    if (res != CURLE_OK) {
        return "{\"error\":\"" + std::string(curl_easy_strerror(res)) + "\"}";
    }
    
    return response;
}

std::string BybitAPI::post(const std::string& endpoint, const std::string& body, bool needAuth) {
    if (!m_curl) return "{\"error\":\"CURL not initialized\"}";
    
    std::string response;
    std::string url = m_baseUrl + endpoint;
    
    struct curl_slist* headers = nullptr;
    
    if (needAuth) {
        std::string timestamp = getTimestamp();
        std::string signature = signRequest(timestamp, body);
        
        headers = curl_slist_append(headers, ("X-BAPI-API-KEY: " + m_apiKey).c_str());
        headers = curl_slist_append(headers, ("X-BAPI-SIGN: " + signature).c_str());
        headers = curl_slist_append(headers, ("X-BAPI-TIMESTAMP: " + timestamp).c_str());
        headers = curl_slist_append(headers, "X-BAPI-RECV-WINDOW: 5000");
    }
    
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(m_curl, CURLOPT_HTTPGET, 0L);
    curl_easy_setopt(m_curl, CURLOPT_POST, 1L);

    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_POST, 1L);
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, m_timeout);
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 2L);
    
    CURLcode res = curl_easy_perform(m_curl);
    
    if (headers) {
        curl_slist_free_all(headers);
    }
    
    if (res != CURLE_OK) {
        return "{\"error\":\"" + std::string(curl_easy_strerror(res)) + "\"}";
    }
    
    return response;
}

} // namespace Bybit
