#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <cstddef>
#include <fstream>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>   
#endif

class CompileTimeObfuscator {
private:
    static constexpr uint8_t genKey(size_t i) {
        return static_cast<uint8_t>(((i * 0x45u) + 0x89u) ^ 0xABu);
    }

public:
    template <size_t N>
    class ObfString {
    private:
        std::array<uint8_t, N> data;

    public:
        constexpr ObfString(const char(&str)[N]) : data{} {
            for (size_t i = 0; i < N; ++i) {
                data[i] = static_cast<uint8_t>(str[i]) ^ genKey(i);
            }
        }

        std::string decrypt() const {
            std::string result;
            result.resize(N - 1);
            for (size_t i = 0; i < N - 1; ++i) {
                result[i] = static_cast<char>(data[i] ^ genKey(i));
            }
            return result;
        }
    };
};

#define OBFUSCATE(str) ([]() { \
    CompileTimeObfuscator::ObfString<sizeof(str)> obf(str); \
    return obf.decrypt(); \
}())

class SecureString {
private:
    std::vector<uint8_t> encrypted_data;
    uint8_t xor_key;

    static uint8_t generateKey() {
        return 0x5A;
    }

    void encrypt(const std::string& data) {
        encrypted_data.resize(data.size());
        for (size_t i = 0; i < data.size(); ++i) {
            encrypted_data[i] = static_cast<uint8_t>(data[i]) ^ xor_key ^ static_cast<uint8_t>(i);
        }
    }

public:
    explicit SecureString(const std::string& data)
        : xor_key(generateKey())
    {
        encrypt(data);
    }

    std::string decrypt() const {
        std::string result;
        result.resize(encrypted_data.size());
        for (size_t i = 0; i < encrypted_data.size(); ++i) {
            result[i] = static_cast<char>(encrypted_data[i] ^ xor_key ^ static_cast<uint8_t>(i));
        }
        return result;
    }

    ~SecureString() {
        if (!encrypted_data.empty()) {
            volatile uint8_t* p = encrypted_data.data();
            for (size_t i = 0; i < encrypted_data.size(); ++i) {
                p[i] = 0;
            }
        }
    }

    SecureString(const SecureString&) = delete;
    SecureString& operator=(const SecureString&) = delete;
};

class SecureCredentials {
private:
    SecureString access_token;
    SecureString rsa_key;
    int product_id;

    static constexpr int obfuscateInt(int value) { return value ^ 0xDEADBEEF; }
    static int deobfuscateInt(int value) { return value ^ 0xDEADBEEF; }

public:
    SecureCredentials()
        : access_token(OBFUSCATE("WyIxMTMzODM1NTMiLCJQajNnLzA1VlZJS1JtY05IMmZpY3VZY29lV00zSE9vTmNvbU5BdFNaIl0=")),
        rsa_key(OBFUSCATE("<RSAKeyValue><Modulus>31c7UIApnCVzSGs27RogWLr7hrzpQ9u26rsETRDCMr85t6uQtYreOqU+vbpj8P3OL1azLsRsHInzCyxc6I+nDWPG/S5d7BFkjFb6dh57lbKDzjZ9IM9LCwr2U9RQlE/BRGvjdYee94cwlWI4kOsqcX4tgDex/oHdMdJlLXR/5QxaYaA1SnDmbvw31tQJWpIWV6qBk8PuLqi0ZvKnDfOQhYZzeW4s4lFnvgrbBrhZCejDYc7t6zQ/4CM4zfNeNJDDYYqXFD5D3IcyOPyv3nAciOv4jpmB6IWlmpjrf9Jjg0J7yvACSHeNuyvgxeDEMhcsl0agjJT69/rlYF5FK0kTGcQ==</Modulus><Exponent>AQAB</Exponent></RSAKeyValue>")),
        product_id(obfuscateInt(31281))
    {
    }

    std::string getAccessToken() const { return access_token.decrypt(); }
    std::string getRsaKey() const { return rsa_key.decrypt(); }
    int getProductId() const { return deobfuscateInt(product_id); }
};

inline bool isDebuggerPresentPlatform() {
#ifdef _WIN32
    return IsDebuggerPresent() != 0;
#else
    std::ifstream f("/proc/self/status");
    if (!f.is_open()) return false;
    std::string line;
    while (std::getline(f, line)) {
        const std::string key = "TracerPid:";
        if (line.compare(0, key.size(), key) == 0) {
            std::size_t pos = key.size();
            while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos]))) ++pos;
            int tracer = std::stoi(line.substr(pos));
            return tracer != 0;
        }
    }
    return false;
#endif
}

template<typename Func>
auto secureCall(Func&& func) {
    if (isDebuggerPresentPlatform()) {
        throw std::runtime_error("Security violation detected");
    }
    return func();
}