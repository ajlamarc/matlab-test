#define CPPHTTPLIB_OPENSSL_SUPPORT
#define CPPHTTPLIB_ZLIB_SUPPORT
#define CPPHTTPLIB_READ_TIMEOUT_SECOND 20

#include "httplib.h"
#include "nlohmann/json.hpp"
using json = nlohmann::json;

#include <set>
#include <vector>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <cstdint>
#include <iterator>
#include <future>
#include <thread>
#include <memory>
#include <sstream>
#include <utility>
#include <fstream>
#include <sys/stat.h>
#include <ctime>
#include <mutex>
#include <type_traits>
#include <cstdlib>

const char *CERT_BYTES = R"(-----BEGIN CERTIFICATE-----
MIIFnjCCA4agAwIBAgIQIyrcRJGv6EQmZ1Iq+we5yDANBgkqhkiG9w0BAQwFADBp
MQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2FzaGluZ3RvbjENMAsGA1UEBxMES2Vu
dDEYMBYGA1UEChMPQmx1ZSBPcmlnaW4gTExDMRwwGgYDVQQDExNCbHVlIE9yaWdp
biBSb290IENBMB4XDTE5MTExNDAwMDAwMFoXDTM5MTExMzIzNTk1OVowaTELMAkG
A1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24xDTALBgNVBAcTBEtlbnQxGDAW
BgNVBAoTD0JsdWUgT3JpZ2luIExMQzEcMBoGA1UEAxMTQmx1ZSBPcmlnaW4gUm9v
dCBDQTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBANdHabT8nRwiA7sO
B+omAS5fq4n4VoADpMwH8d/f0wu7nKRCCdbf+nJMrq8AKcegKyoay0Sf6vRhnkrV
96POgYaY/5cm9xnIJBl9CZiVT0g7AwVj0EjJovxpe2A6zqJ9jdbPUZz04etd1agQ
b5FNdEY2WuDVWzD8YhPONFj5sgBpr83yDA2hYELXasA1xEn4b5P7YXnGfGY6/C4b
6x4gZFBkaRcDhVWyp+SD09m1vvzvKnOXQeA8KSYdM/X/36c+yH/bhA1B8WQtUajp
FK0eafQqlA5tYMLR1aSNNAcp4NsoKHDnr0oOIPzBgG5kxpTrLaEyezQeiOtYThWe
+Tak1HGaiRpHZr8k8qVutgslvutM8KQJdqAY3b3vi6emyF1LDLvmqMqIUeOh54nB
JEUtJs3ujBrElU5qThJzbogril0jNdarq/jRmQb/cgquVHoCHA/7m+9rbQ/F3Cbk
W1p9/MXGorUMbL6yJmkizEO8p89zH/od4G0zcj8jIf0hHffRhH286lvGl+Ih839S
1NOeCgkjtP1OQyZEJkX98ML2AfNhIgwfQGnD/gSUucalhtl94Wi3I2tuYM3XkrTI
FffTSaE9UxKOSELu3uLWPunbC18/PdP7MgaS1gp0/aJDX5FxuE4eNlI0O2NamK8v
/GAW7/YP6JCc5TZtbXMebS77G1+1AgMBAAGjQjBAMB0GA1UdDgQWBBQOPjAjL/0Q
6IoIo1RXnTsPt8GBQjAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAN
BgkqhkiG9w0BAQwFAAOCAgEATfmCpE/5rKgPN49jyS8U4BI94SIQh5ZJdca17tZ+
6X51JrLznAyRy5osgN7WNY8XZ5qZgiSVB5Lz/nUcuyOF1RhPB1i1YGxBj/Os7J25
s0wjvBYKoktBSkuqC5bhXalsBmR+h3pnbQSIKxlbtNcXTPEz6WA1hg06loVHFMQy
qk3WcaN52Jdvtg/WMNgkdu5hYLRFKPBZqRynTPXshHnUJzCuDybkpYBdUW1t23T0
9Yme0TjFi5ViPxuOvnkF8NsmxR6RJ9wPSFmIJBuDy/sFI9oiA6EQOjsfN4WsKtYJ
cVebUSs/Qppzq9rPANfI8zO5q1+zZZqePk8uVOSmth8YaosUuCCmKPCwF+/qFMif
k1LedMnLU1NeRs3MSAYgIjDhwctCcjsDoHBZHR0RbSrKzEWJr5LFEgg6ov1Ra8pp
jcQp8BNhDHmAzz3BJ/H3IFiO0lNmShnZDhJMGjEKyBZaTDRXr0aYNqX2fmnyPWba
3mnPxjs1ndtawKoVeZ2B+94EMwPPhfGamzNi4+ltw5cffPATb9oWcetD0LwM6xDD
fvjtd/q9GpbXqNBVNerphiEUaAgJ1jgFwAeZxEKgGTvcrSOKQbGwxyXFMVo9q4yG
BHlsKbFE93JR0MoSDy1MhjW0vWs/vK/ybwE02lkVvoADdCVkkA798Vy6/eW0Kcv1
3Z8=
-----END CERTIFICATE-----)";

const size_t CERT_BYTES_SIZE = strlen(CERT_BYTES);

#ifdef _WIN32
#include <direct.h>

std::string HOME_DIR = "USERPROFILE";
std::string PATH_SEPARATOR = "\\";
#define MKDIR(dir) _mkdir(dir)
#else
#include <unistd.h>

std::string HOME_DIR = "HOME";
std::string PATH_SEPARATOR = "/";
#define MKDIR(dir) mkdir(dir, 0755)
#endif

enum class BDMSDataType {
    UNKNOWN,
    // TODO: handle these types better
    // BOOL,
    // CHAR,
    // BYTE,
    // INT8,
    UINT8,
    UINT16,
    INT16,
    UINT32,
    INT32,
    UINT64,
    INT64,
    FLOAT,
    DOUBLE
};

// Type definitions
class GenericVectorBase {
  public:
    virtual ~GenericVectorBase() = default;
    virtual char *data() = 0;
    virtual size_t size() = 0;
    virtual size_t byteSize() = 0;
    virtual BDMSDataType getType() const = 0;
};

template <typename T> class GenericVectorImpl : public GenericVectorBase {
  public:
    std::vector<T> vec;

    GenericVectorImpl(size_t size) : vec(size) {}

    char *data() override { return reinterpret_cast<char *>(vec.data()); }

    size_t size() override { return vec.size(); }

    size_t byteSize() override { return vec.size() * sizeof(T); }

    BDMSDataType getType() const override {
        if (std::is_same<T, uint8_t>::value)
            return BDMSDataType::UINT8;
        if (std::is_same<T, uint16_t>::value)
            return BDMSDataType::UINT16;
        if (std::is_same<T, int16_t>::value)
            return BDMSDataType::INT16;
        if (std::is_same<T, uint32_t>::value)
            return BDMSDataType::UINT32;
        if (std::is_same<T, int32_t>::value)
            return BDMSDataType::INT32;
        if (std::is_same<T, uint64_t>::value)
            return BDMSDataType::UINT64;
        if (std::is_same<T, int64_t>::value)
            return BDMSDataType::INT64;
        if (std::is_same<T, float>::value)
            return BDMSDataType::FLOAT;
        if (std::is_same<T, double>::value)
            return BDMSDataType::DOUBLE;
        // Catch unsupported types at compile time
        static_assert(std::is_same<T, uint8_t>::value ||
                          std::is_same<T, uint16_t>::value ||
                          std::is_same<T, int16_t>::value ||
                          std::is_same<T, uint32_t>::value ||
                          std::is_same<T, int32_t>::value ||
                          std::is_same<T, uint64_t>::value ||
                          std::is_same<T, int64_t>::value ||
                          std::is_same<T, float>::value ||
                          std::is_same<T, double>::value,
                      "Unsupported vector type");
    }
};

struct GenericVector {
    std::unique_ptr<GenericVectorBase> data;

    template <typename T> void assign(size_t size) {
        data = httplib::detail::make_unique<GenericVectorImpl<T>>(size);
    }

    char *buffer() { return data ? data->data() : nullptr; }

    size_t size() { return data ? data->size() : 0; }

    size_t byteSize() { return data ? data->byteSize() : 0; }

    BDMSDataType getType() const {
        return data ? data->getType() : BDMSDataType::UNKNOWN;
    }
};

typedef std::string CampaignID;
typedef std::string CampaignName;
typedef std::string SessionID;
typedef std::string SessionName;
typedef std::string ElementID;
typedef std::string ElementName;
typedef std::string EventID;
typedef std::string EventName;
typedef std::string BDMSDataID;

enum HTTPMethod { GET, HEAD, POST };

struct BDMSProvidedConfig {
    std::string profile, host, apiKey, protocol, certificatePath, userAgent;
};
struct BDMSProfileConfig {
    std::string host, apiKey, protocol, certificatePath;
};
struct BDMSResolvedConfig {
    std::string baseUrl, apiKey, certificatePath, userAgent;
};

// see
// https://raymii.org/s/tutorials/Cpp_std_async_with_a_concurrency_limit.html
class Semafoor {
  public:
    explicit Semafoor(size_t count) : count(count) {}
    size_t getCount() const { return count; };
    void lock() { // call before critical section
        std::unique_lock<std::mutex> lock(mutex);
        condition_variable.wait(lock, [this] {
            if (count != 0) // written out for clarity, could just be return
                            // (count != 0);
                return true;
            else
                return false;
        });
        --count;
    }
    void unlock() { // call after critical section
        std::unique_lock<std::mutex> lock(mutex);
        ++count;
        condition_variable.notify_one();
    }

  private:
    std::mutex mutex;
    std::condition_variable condition_variable;
    size_t count;
};

// RAII wrapper, make on of these in your 'work-doing' class to
// lock the critical section. once it goes out of scope the
// critical section is unlocked
class CriticalSection {
  public:
    explicit CriticalSection(Semafoor &s) : semafoor{s} { semafoor.lock(); }
    ~CriticalSection() { semafoor.unlock(); }

  private:
    Semafoor &semafoor;
};

// Class definitions
/* TODO: DataStats and BDMSConfig classes do not utilize our custom exception
handler, only BaseBDMSDataManager. We want these classes to remain public
separate from BaseBDMSDataManager, so functions which throw exceptions should
accept an additional parameter of type BaseBDMSExceptionHandler. Example:

getRangeValues(const BDMSDataID &bdmsDataID,
                             DataStats stats,
                             char *buffer,
                             std::string type,
                             BDMSExceptionsBase* error_handler = nullptr) {
    if (error_occurred) {
        if (error_handler) {
            error_handler->raiseError("getRangeValues error", "Error message");
        } else {
            // default error handling
        }
    }
 */

class BaseBDMSExceptionHandler {
  public:
    virtual ~BaseBDMSExceptionHandler() = default;
    virtual void raiseError(const std::string &title,
                            const std::string &message) = 0;
    virtual void raiseWarning(const std::string &title,
                              const std::string &message) = 0;
};

class DefaultBDMSExceptionHandler : public BaseBDMSExceptionHandler {
  public:
    void raiseError(const std::string &title,
                    const std::string &message) override {
        throw std::runtime_error(title + ": " + message);
    }
    void raiseWarning(const std::string &title,
                      const std::string &message) override {
        std::cerr << "[bdms2-cpp-library] WARNING - " << title << ": "
                  << message << std::endl;
    }
};

class DataStats {
  public:
    BDMSDataID identifier;
    std::string data_type;
    size_t data_count;
    // below variables are optional (can be "")
    std::string zip_hash;
    std::string min_value_hex;
    std::string max_value_hex;
    static const DataStats fromIdentifier(const BDMSDataID &bdmsDataID);
    static const std::vector<std::string>
    getIdentifierParts(const BDMSDataID &bdmsDataID);
    template <typename T>
    static T littleEndianHexToDecimal(std::string &littleEndianHex);
    const std::string getBDMSDataType() const;
    const size_t getDataCount() const;
    std::vector<size_t> getDimensionality() const;
    const size_t getTotalValueCount() const;
    template <typename T> const T getMinValue() const;
    template <typename T> const T getMaxValue() const;
    DataStats(BDMSDataID idVal, std::string dataTypeVal,
              std::string dataCountVal, std::string zipHashVal,
              std::string minValueHexVal, std::string maxValueHexVal)
        : identifier(idVal), data_type(dataTypeVal),
          data_count(std::stol(dataCountVal)), zip_hash(zipHashVal),
          min_value_hex(minValueHexVal), max_value_hex(maxValueHexVal) {}
};

const DataStats DataStats::fromIdentifier(const BDMSDataID &bdmsDataID) {
    std::vector<std::string> identifierParts = getIdentifierParts(bdmsDataID);

    std::string data_type;
    std::string data_count;
    std::string zip_hash;
    std::string min_value_hex;
    std::string max_value_hex;

    if (identifierParts[0] == "v1") {
        data_type = identifierParts[1];
        data_count = identifierParts[2];
        zip_hash = identifierParts[3];
        min_value_hex = identifierParts[4];
        max_value_hex = identifierParts[5];
    } else if (identifierParts[0] == "v2") {
        data_type = identifierParts[2];
        data_count = identifierParts[3];
        zip_hash = identifierParts[4];
        min_value_hex = identifierParts[5];
        max_value_hex = identifierParts[6];
    } else if (identifierParts[0] == "special") {
        if (identifierParts[1] == "constant") {
            data_type = identifierParts[2];
            data_count = identifierParts[3];
            auto value_hex = identifierParts[4];
            zip_hash = "";
            min_value_hex = value_hex;
            max_value_hex = value_hex;
        } else if (identifierParts[1] == "steps" ||
                   identifierParts[1] == "range") {
            data_type = identifierParts[2];
            data_count = identifierParts[3];
            zip_hash = "";
            min_value_hex = identifierParts[5];
            max_value_hex = identifierParts[6];
        } else if (identifierParts[1] == "bdmsv1") {
            data_type = identifierParts[2];
            data_count = identifierParts[3];
            zip_hash = "";
            min_value_hex = identifierParts[4];
            max_value_hex = identifierParts[5];
        } else {
            throw std::runtime_error("Unexpected data identifier.");
        }
    } else if (identifierParts[0] == "function" &&
               identifierParts[1] == "region") {
        data_type = identifierParts[2];
        data_count = identifierParts[3];
        zip_hash = "";
        min_value_hex = identifierParts[identifierParts.size() - 2];
        max_value_hex = identifierParts[identifierParts.size() - 1];
    } else {
        throw std::runtime_error("Unexpected data identifier.");
    }
    return DataStats(bdmsDataID, data_type, data_count, zip_hash, min_value_hex,
                     max_value_hex);
}

const std::vector<std::string>
DataStats::getIdentifierParts(const BDMSDataID &bdmsDataID) {
    std::istringstream dataIDStream(bdmsDataID);
    std::string part;
    std::vector<std::string> identifierParts;
    while (std::getline(dataIDStream, part, ':')) {
        identifierParts.push_back(part);
    }

    // if last part was empty, the above loop does not add it, so add it here
    if (bdmsDataID.back() == ':') {
        identifierParts.push_back("");
    }

    return identifierParts;
}

// convert hex string (BDMS data identifier) to any value type.  Will work for
// times and values.
template <typename T>
T DataStats::littleEndianHexToDecimal(std::string &littleEndianHex) {
    T decimal_value = 0;
    size_t length = littleEndianHex.length();
    size_t num_bytes = sizeof(T);
    std::stringstream ss;
    for (size_t i = 0; i < num_bytes; ++i) {
        std::string byte_string = littleEndianHex.substr(i * 2, 2);
        unsigned int byte_value;
        ss.clear();
        ss << std::hex << byte_string;
        ss >> byte_value;
        decimal_value |= (static_cast<T>(byte_value) << (i * 8));
    }
    return decimal_value;
}

const std::string DataStats::getBDMSDataType() const {
    std::istringstream ss(this->data_type);
    std::string bdmsDataType;
    // "uint64,1" -> "uint64"
    std::getline(ss, bdmsDataType, ',');
    return bdmsDataType;
}

const size_t DataStats::getDataCount() const { return this->data_count; }

const size_t DataStats::getTotalValueCount() const {
    std::vector<size_t> dimensionality = this->getDimensionality();
    size_t total = this->data_count;

    for (size_t i = 0; i < dimensionality.size(); ++i) {
        total *= dimensionality[i];
    }

    return total;
}

std::vector<size_t> DataStats::getDimensionality() const {
    std::vector<size_t> dimensions;
    std::string remaining = this->data_type;

    // skip over "data type", e.g. the "int32" part of "int32,4,3"
    size_t typePos = remaining.find(',');
    if (typePos == std::string::npos) {
        throw std::runtime_error(
            "Failed to parse dimensionality of identifier (comma not found): " +
            this->identifier);
    }

    // Extract the dimensionality part
    remaining = remaining.substr(typePos + 1);
    if (remaining.empty()) {
        throw std::runtime_error("Failed to parse dimensionality of identifier "
                                 "(remaining is empty): " +
                                 this->identifier);
    }

    size_t pos = 0;
    while ((pos = remaining.find(',')) != std::string::npos) {
        std::string dimension = remaining.substr(0, pos);
        try {
            dimensions.push_back(std::stoul(dimension));
        } catch (const std::exception &e) {
            throw std::runtime_error(
                "Failed to parse dimensionality of identifier: " +
                this->identifier);
        }
        remaining = remaining.substr(pos + 1);
    }

    // Add the last dimension
    if (!remaining.empty()) {
        try {
            dimensions.push_back(std::stoul(remaining));
        } catch (const std::exception &e) {
            throw std::runtime_error(
                "Failed to parse dimensionality of identifier: " +
                this->identifier);
        }
    }

    if (dimensions.empty()) {
        throw std::runtime_error(
            "getDimensionality error: No dimensions found");
    }

    return dimensions;
}

template <typename T> const T DataStats::getMinValue() const {
    if (this->min_value_hex.empty()) {
        // auto modalBody =
        //     "Unexpected empty DataStats minimum value hex for data ID " +
        //     this->identifier + ". Please contact the BDMS team.";
        // MessageBox(NULL, TEXT(modalBody.c_str()), TEXT("getMinValue error"),
        //            MB_ICONERROR);
        throw std::runtime_error("getMinValue error");
    }

    std::string min_value_hex = this->min_value_hex;
    return littleEndianHexToDecimal<T>(min_value_hex);
}

template <typename T> const T DataStats::getMaxValue() const {
    if (this->max_value_hex.empty()) {
        // auto modalBody =
        //     "Unexpected empty DataStats maximum value hex for data ID " +
        //     this->identifier + ". Please contact the BDMS team.";
        // MessageBox(NULL, TEXT(modalBody.c_str()), TEXT("getMaxValue error"),
        //            MB_ICONERROR);
        throw std::runtime_error("getMaxValue error");
    }

    std::string max_value_hex = this->max_value_hex;
    return littleEndianHexToDecimal<T>(max_value_hex);
}

// Global log file stream
// std::ofstream logFile;
// std::mutex logFileMutex;

// void initializeLogging()
// {
//     if (!logFile.is_open())
//     {
//         std::lock_guard<std::mutex> lock(logFileMutex);
//         logFile.open("bdms_log.txt", std::ios::app);
//     }
//     if (!logFile.is_open())
//     {
//         throw std::runtime_error("Failed to open log file");
//     }
// }

// void closeLogging()
// {
//     if (logFile.is_open())
//     {
//         logFile.close();
//     }
// }

// void logMessage(const char *message)
// {
//     if (logFile.is_open())
//     {
//         time_t now = time(0);
//         char *dt = ctime(&now);
//         logFile << dt << ": " << message << std::endl;
//         logFile.flush(); // Ensure the message is written immediately
//     }
// }

/* Common, static operations for loading BDMS2 configuration values. */
class BDMSConfig {
  private:
    static std::string _getBDMSConfigValueByPriority(
        const std::string &provided, const std::string &environValue,
        const std::string &profile, const std::string &defaultValue);
    static std::string _readBDMSKeyFromFile(const std::string &filePath,
                                            const std::string &profile,
                                            const std::string &key);
    static std::string _getBDMSEnv(const std::string &key,
                                   const std::string &defaultValue = "");
    static std::string _getBDMSConfigDir();
    static void _ensureCertificateExists(const std::string &certPath);
    static BDMSProfileConfig _getProfileHostTokenProtocolCertificateValues(
        const std::string &providedProfile, const std::string &configDir);

  public:
    static BDMSResolvedConfig
    getHostTokenProtocolCertificateAgentValues(BDMSProvidedConfig provided);
};

std::string BDMSConfig::_getBDMSConfigValueByPriority(
    const std::string &provided, const std::string &environValue,
    const std::string &profile, const std::string &defaultValue) {
    if (!provided.empty()) {
        return provided;
    } else if (!environValue.empty()) {
        return environValue;
    } else if (!profile.empty()) {
        return profile;
    }
    return defaultValue;
}

// read value for key from "config" or "credentials" file based on profile
std::string BDMSConfig::_readBDMSKeyFromFile(const std::string &filePath,
                                             const std::string &profile,
                                             const std::string &key) {
    std::ifstream file(filePath);
    if (file.is_open()) {
        std::string line;
        bool inCorrectProfile = false;
        while (std::getline(file, line)) {
            // Trim whitespace from the beginning and end of the line
            line.erase(line.begin(), std::find_if(line.begin(), line.end(),
                                                  [](unsigned char ch) {
                                                      return !std::isspace(ch);
                                                  }));
            line.erase(
                std::find_if(line.rbegin(), line.rend(),
                             [](unsigned char ch) { return !std::isspace(ch); })
                    .base(),
                line.end());

            if (line == "[" + profile + "]") {
                inCorrectProfile = true;
            } else if (inCorrectProfile && line.find(key) == 0) {
                size_t equalsPos = line.find('=');
                if (equalsPos != std::string::npos) {
                    std::string value = line.substr(equalsPos + 1);
                    // Trim leading and trailing whitespace from the value
                    value.erase(value.begin(),
                                std::find_if(value.begin(), value.end(),
                                             [](unsigned char ch) {
                                                 return !std::isspace(ch);
                                             }));
                    value.erase(std::find_if(value.rbegin(), value.rend(),
                                             [](unsigned char ch) {
                                                 return !std::isspace(ch);
                                             })
                                    .base(),
                                value.end());
                    return value;
                }
            } else if (line.find('[') == 0) {
                // We've reached a new section, stop searching
                break;
            }
        }
    }
    return "";
}

std::string BDMSConfig::_getBDMSEnv(const std::string &key,
                                    const std::string &defaultValue) {
    std::string bdms2_key = "BDMS2_" + key;
    const char *bdms2_value = std::getenv(bdms2_key.c_str());

    if (bdms2_value != nullptr) {
        return std::string(bdms2_value);
    }

    std::string bdms_key = "BDMS_" + key;
    const char *bdms_value = std::getenv(bdms_key.c_str());

    if (bdms_value != nullptr) {
        return std::string(bdms_value);
    }

    return defaultValue;
}

std::string BDMSConfig::_getBDMSConfigDir() {
    std::string defaultConfigDir =
        std::string(std::getenv(HOME_DIR.c_str())) + PATH_SEPARATOR + ".bdms2";
    return _getBDMSEnv("CONFIG_DIRECTORY", defaultConfigDir);
}

/* If the cert cannot be found in the expected location,
   it will be copied there from the BlueOriginRootCA.py certificate
   data. */
void BDMSConfig::_ensureCertificateExists(const std::string &certPath) {

    // Check if file exists
    struct stat buffer;
    bool exists = (stat(certPath.c_str(), &buffer) == 0);

    if (!exists) {
        // File doesn't exist, create it and write the certificate
        std::ofstream certFile(certPath, std::ios::out | std::ios::binary);
        if (certFile.is_open()) {
            certFile.write(CERT_BYTES, CERT_BYTES_SIZE);
            certFile.close();
        } else {
            throw std::runtime_error(
                "Failed to create BlueOriginRootCA.crt file");
        }
    }
}

/* Initialize a BaseBDMSDataManager object with the provided configuration.
Empty strings are interpreted as "no value provided".

Determine the BDMS host, user, API key, and API protocol based on (in precedence
order):
    * The values provided when calling the code
    * Environment variables
    * Values from the BDMS profile configuration
    * Sensible defaults

// TODO: how to handle user agent??
This client also accepts an optional user agent string, which is used to
identify the client making requests to BDMS and can be helpful for collaborative
debugging. We recommend including the client name and version, ex.
"igs-cpp-bdms2/1.0.0". */
BDMSProfileConfig BDMSConfig::_getProfileHostTokenProtocolCertificateValues(
    const std::string &providedProfile, const std::string &configDir) {
    BDMSProfileConfig profile;
    std::string profileName;

    if (!providedProfile.empty()) {
        profileName = providedProfile;
    } else {
        profileName = _getBDMSEnv("API_PROFILE", "default");
    }

    std::string configPath = configDir + PATH_SEPARATOR + "config";
    std::string credentialsPath = configDir + PATH_SEPARATOR + "credentials";

    profile.host =
        _readBDMSKeyFromFile(configPath, profileName, "bdms_api_host");
    profile.protocol =
        _readBDMSKeyFromFile(configPath, profileName, "bdms_api_protocol");
    profile.certificatePath =
        _readBDMSKeyFromFile(configPath, profileName, "bdms_certificate_path");
    profile.apiKey = _readBDMSKeyFromFile(credentialsPath, profileName,
                                          "bdms_secret_api_key");

    return profile;
}

BDMSResolvedConfig BDMSConfig::getHostTokenProtocolCertificateAgentValues(
    BDMSProvidedConfig provided) {
    BDMSResolvedConfig resolved;
    std::string configDir = _getBDMSConfigDir();

    BDMSProfileConfig profile =
        BDMSConfig::_getProfileHostTokenProtocolCertificateValues(
            provided.profile, configDir);

    std::string environHost = BDMSConfig::_getBDMSEnv("API_HOST");
    std::string environProtocol = BDMSConfig::_getBDMSEnv("API_PROTOCOL");
    std::string environApiKey = BDMSConfig::_getBDMSEnv("SECRET_API_KEY");
    std::string environCertificatePath =
        BDMSConfig::_getBDMSEnv("CERTIFICATE_PATH");

    std::string defaultHost = "bdms2.blueorigin.com";
    std::string defaultProtocol = "https";
    std::string defaultApiKey = "";
    std::string defaultCertificatePath = configDir + PATH_SEPARATOR + "BlueOriginRootCA.crt";
    std::string defaultUserAgent = "cpp-bdms2/unknown";

    // evaluate configuration values in precedence order described in docstring
    // TODO: enable when we have profile
    std::string host = _getBDMSConfigValueByPriority(provided.host, environHost,
                                                     profile.host, defaultHost);
    std::string protocol = _getBDMSConfigValueByPriority(
        provided.protocol, environProtocol, profile.protocol, defaultProtocol);

    // Set class variables based on config
    resolved.apiKey = _getBDMSConfigValueByPriority(
        provided.apiKey, environApiKey, profile.apiKey, defaultApiKey);
    resolved.certificatePath = _getBDMSConfigValueByPriority(
        provided.certificatePath, environCertificatePath,
        profile.certificatePath, defaultCertificatePath);
    resolved.userAgent =
        !provided.userAgent.empty() ? provided.userAgent : defaultUserAgent;
    resolved.baseUrl = protocol + "://" + host;

    _ensureCertificateExists(resolved.certificatePath);

    return resolved;
}

class BaseBDMSDataManager {
  private:
    Semafoor _semafoor;
    httplib::Client *client();
    std::pair<bool, std::shared_ptr<httplib::Result>>
    request(const std::string &endpoint, const json &body, HTTPMethod method);

    void getRangeValues(const BDMSDataID &bdmsDataID, DataStats stats,
                        char *buffer, std::string type);
    void getConstantValues(const BDMSDataID &bdmsDataID, char *buffer,
                           size_t size, std::string type);
    template <typename T>
    static void fillBufferWithSequence(T *buffer, T min_value, T max_value,
                                       T step_value);
    template <typename T>
    static void assignBufferAndVector(GenericVector &vec, char *&buffer,
                                      size_t size);

  protected:
    std::string _apiKey;
    std::string _baseUrl;
    std::string _userAgent;
    std::string _certificatePath;
    std::unique_ptr<BaseBDMSExceptionHandler> errorHandler;
    std::pair<bool, std::shared_ptr<httplib::Result>>
    post(const std::string &endpoint, const json &body);
    std::pair<bool, std::shared_ptr<httplib::Result>>
    head(const std::string &endpoint);
    std::pair<bool, std::shared_ptr<httplib::Result>>
    get(const std::string &endpoint);
    std::vector<std::future<GenericVector>>
    getDataArraysAsync(const std::string &sessionID,
                       const std::vector<std::string> &ids);
    const DataStats getStats(const SessionID &sessionID,
                             const BDMSDataID &bdmsDataID);

  public:
    // Primary constructor
    BaseBDMSDataManager(BDMSProvidedConfig provided,
                        std::unique_ptr<BaseBDMSExceptionHandler> error_handler)
        : errorHandler(std::move(error_handler)),
          _semafoor(std::thread::hardware_concurrency() * 2) {
        BDMSResolvedConfig resolved =
            BDMSConfig::getHostTokenProtocolCertificateAgentValues(provided);
        _apiKey = resolved.apiKey;
        _baseUrl = resolved.baseUrl;
        _userAgent = resolved.userAgent;
        _certificatePath = resolved.certificatePath;
    }

    // Delegating constructors
    BaseBDMSDataManager(BDMSProvidedConfig provided)
        : BaseBDMSDataManager(
              provided,
              httplib::detail::make_unique<DefaultBDMSExceptionHandler>()) {}
    BaseBDMSDataManager()
        : BaseBDMSDataManager(
              BDMSProvidedConfig(),
              httplib::detail::make_unique<DefaultBDMSExceptionHandler>()) {}
};

httplib::Client *BaseBDMSDataManager::client() {
    static thread_local std::unique_ptr<httplib::Client> _client =
        httplib::detail::make_unique<httplib::Client>(_baseUrl);
    // default connection timeout is 300 seconds, which is sufficient
    _client->set_read_timeout(std::chrono::seconds(300));
    _client->set_write_timeout(std::chrono::seconds(300));
    _client->set_keep_alive(true);
    _client->set_follow_location(true);
    _client->set_bearer_token_auth(_apiKey);
    _client->set_ca_cert_path(_certificatePath, "");
    return _client.get();
}

/* NOTE: this also retries "unsafe" request types automatically (e.g. POST).
since the client doesn't support create / update / delete requests currently.

If it does in the future, add a "retry_unsafe_methods" argument. */
std::pair<bool, std::shared_ptr<httplib::Result>>
BaseBDMSDataManager::request(const std::string &endpoint, const json &body,
                             HTTPMethod method) {
    httplib::Headers headers = {{"User-Agent", _userAgent}};
    if (method == POST) {
        headers.emplace("Accept", "application/json");
    }

    auto cl = client();
    std::set<int> retryStatusCodes = {429, 500, 502, 503, 504};
    const double backoffFactor = 3.0;
    const double backoffJitter = 6.0;

    for (int retry = 0; retry < 4; ++retry) {
        // Make the request
        std::shared_ptr<httplib::Result> resPtr;
        {
            // Enforce concurrency limit on HTTP requests
            CriticalSection cs(_semafoor);
            if (method == POST) {
                resPtr = std::make_shared<httplib::Result>(cl->Post(
                    endpoint, headers, body.dump(), "application/json"));
            } else if (method == HEAD) {
                resPtr = std::make_shared<httplib::Result>(
                    cl->Head(endpoint, headers));
            } else if (method == GET) {
                resPtr = std::make_shared<httplib::Result>(
                    cl->Get(endpoint, headers));
            }
        }

        // Handle transport layer errors
        if (!resPtr || resPtr->error() != httplib::Error::Success) {
            if (retry == 3) {
                std::ostringstream err;
                err << "Transport layer error"
                    << "\nError code: "
                    << httplib::to_string(resPtr ? resPtr->error()
                                                 : httplib::Error::Unknown)
                    << "\nEndpoint: " << endpoint
                    << "\nRequest body: " << body.dump(2);
                errorHandler->raiseError("HTTP Request Failed", err.str());
                return std::make_pair(false, resPtr);
            }

            double waitTime = std::pow(backoffFactor, retry) + 
                            (static_cast<double>(rand()) / RAND_MAX) * backoffJitter;
            std::this_thread::sleep_for(std::chrono::duration<double>(waitTime));
            continue;
        }

        // TODO: this doesn't extend to other endpoints that would return,
        // e.g. a 201
        if ((*resPtr)->status == 200) {
            return std::make_pair(true, resPtr);
        }

        // Handle authentication errors
        if ((*resPtr)->status == 401 || (*resPtr)->status == 403) {
            std::ostringstream err;
            err << "Authentication failed (HTTP " << (*resPtr)->status << ")"
                << "\nEndpoint: " << endpoint
                << "\nPlease verify your API key and access permissions."
                << "\nTo set a new API key: Data -> BlueAcc -> Reset API key";
            errorHandler->raiseError("Authentication Error", err.str());
            return std::make_pair(false, resPtr);
        }

        const json jsonResponse = json::parse((*resPtr)->body);

        // Handle retryable status codes
        if (retryStatusCodes.find((*resPtr)->status) != retryStatusCodes.end()) {
            if (retry < 3) {
                // Check for Retry-After header
                double waitTime = 0;
                std::string retryAfter = std::string((*resPtr)->get_header_value("retry-after"));
                
                if (!retryAfter.empty()) {
                    waitTime = std::stod(retryAfter);
                } else {
                    // Use exponential backoff with jitter
                    waitTime = std::pow(backoffFactor, retry + 1) + 
                             (static_cast<double>(rand()) / RAND_MAX) * backoffJitter;
                }
                
                std::this_thread::sleep_for(std::chrono::duration<double>(waitTime));
            } else if ((*resPtr)->status != 429) { // Don't show error for rate limiting
                std::ostringstream err;
                err << "Max retries reached (HTTP " << (*resPtr)->status << ")"
                    << "\nEndpoint: " << endpoint
                    << "\nRequest body: " << body.dump(2)
                    << "\nResponse body: " << jsonResponse.dump(2);
                errorHandler->raiseError("Request Failed After Retries",
                                       err.str());
            }
            continue;
        }

        // Handle other HTTP errors
        std::ostringstream err;
        err << "Request failed (HTTP " << (*resPtr)->status << ")"
            << "\nEndpoint: " << endpoint << "\nRequest body: " << body.dump(2)
            << "\nResponse body: " << jsonResponse.dump(2);
        errorHandler->raiseError("Request Failed", err.str());
        return std::make_pair(false, resPtr);
    }

    // this will not be hit, added to suppress compiler complaint
    return std::make_pair(false, nullptr);
}

std::pair<bool, std::shared_ptr<httplib::Result>>
BaseBDMSDataManager::head(const std::string &endpoint) {
    return request(endpoint, json({}), HEAD);
}

std::pair<bool, std::shared_ptr<httplib::Result>>
BaseBDMSDataManager::post(const std::string &endpoint, const json &body) {
    return request(endpoint, body, POST);
}

std::pair<bool, std::shared_ptr<httplib::Result>>
BaseBDMSDataManager::get(const std::string &endpoint) {
    return request(endpoint, json({}), GET);
}

template <typename T>
void BaseBDMSDataManager::assignBufferAndVector(GenericVector &vec,
                                                char *&buffer, size_t size) {
    vec.assign<T>(size);
    buffer = vec.buffer();
}

template <typename T>
void BaseBDMSDataManager::fillBufferWithSequence(T *buffer, T min_value,
                                                 T max_value, T step_value) {
    bool is_forward_stepping = step_value > 0;

    if (is_forward_stepping) {
        size_t index = 0;
        for (T value = min_value; value <= max_value; value += step_value) {
            buffer[index] = value;
            index++;
        }
    } else {
        size_t index = 0;
        for (T value = max_value; value >= min_value; value += step_value) {
            buffer[index] = value;
            index++;
        }
    }
}

void BaseBDMSDataManager::getRangeValues(const BDMSDataID &identifier,
                                         DataStats stats, char *buffer,
                                         std::string type) {
    std::vector<std::string> identifierParts =
        DataStats::getIdentifierParts(identifier);
    std::string step_value_dec = identifierParts[4];

    if (type == "char" || type == "byte" || type == "int8" || type == "uint8") {
        uint8_t *typed_buffer = reinterpret_cast<uint8_t *>(buffer);
        fillBufferWithSequence<uint8_t>(
            typed_buffer, stats.getMinValue<uint8_t>(),
            stats.getMaxValue<uint8_t>(),
            static_cast<uint8_t>(std::stoul(step_value_dec)));
    } else if (type == "uint16") {
        uint16_t *typed_buffer = reinterpret_cast<uint16_t *>(buffer);
        fillBufferWithSequence<uint16_t>(
            typed_buffer, stats.getMinValue<uint16_t>(),
            stats.getMaxValue<uint16_t>(),
            static_cast<uint16_t>(std::stoul(step_value_dec)));
    } else if (type == "int16") {
        int16_t *typed_buffer = reinterpret_cast<int16_t *>(buffer);
        fillBufferWithSequence<int16_t>(
            typed_buffer, stats.getMinValue<int16_t>(),
            stats.getMaxValue<int16_t>(),
            static_cast<int16_t>(std::stoi(step_value_dec)));
    } else if (type == "uint32") {
        uint32_t *typed_buffer = reinterpret_cast<uint32_t *>(buffer);
        fillBufferWithSequence<uint32_t>(
            typed_buffer, stats.getMinValue<uint32_t>(),
            stats.getMaxValue<uint32_t>(), std::stoul(step_value_dec));
    } else if (type == "int32") {
        int32_t *typed_buffer = reinterpret_cast<int32_t *>(buffer);
        fillBufferWithSequence<int32_t>(
            typed_buffer, stats.getMinValue<int32_t>(),
            stats.getMaxValue<int32_t>(),
            static_cast<uint32_t>(std::stoi(step_value_dec)));
    } else if (type == "uint64") {
        uint64_t *typed_buffer = reinterpret_cast<uint64_t *>(buffer);
        fillBufferWithSequence<uint64_t>(
            typed_buffer, stats.getMinValue<uint64_t>(),
            stats.getMaxValue<uint64_t>(), std::stoull(step_value_dec));
    } else if (type == "int64") {
        int64_t *typed_buffer = reinterpret_cast<int64_t *>(buffer);
        fillBufferWithSequence<int64_t>(
            typed_buffer, stats.getMinValue<int64_t>(),
            stats.getMaxValue<int64_t>(), std::stoll(step_value_dec));
    } else {
        errorHandler->raiseError("Unexpected BDMS data type in getRangeValues",
                                 type + " for identifier " + identifier +
                                     ". Please contact the BDMS team.");
    }
}

void BaseBDMSDataManager::getConstantValues(const BDMSDataID &identifier,
                                            char *buffer, size_t size,
                                            std::string type) {
    std::vector<std::string> identifierParts =
        DataStats::getIdentifierParts(identifier);
    std::string constant_value_hex = identifierParts[4];

    if (type == "bool" || type == "char" || type == "byte" || type == "int8" ||
        type == "uint8") {
        uint8_t *typed_buffer = reinterpret_cast<uint8_t *>(buffer);
        std::fill_n(
            typed_buffer, size,
            DataStats::littleEndianHexToDecimal<uint8_t>(constant_value_hex));
    } else if (type == "uint16") {
        uint16_t *typed_buffer = reinterpret_cast<uint16_t *>(buffer);
        std::fill_n(
            typed_buffer, size,
            DataStats::littleEndianHexToDecimal<uint16_t>(constant_value_hex));
    } else if (type == "int16") {
        int16_t *typed_buffer = reinterpret_cast<int16_t *>(buffer);
        std::fill_n(
            typed_buffer, size,
            DataStats::littleEndianHexToDecimal<int16_t>(constant_value_hex));
    } else if (type == "uint32") {
        uint32_t *typed_buffer = reinterpret_cast<uint32_t *>(buffer);
        std::fill_n(
            typed_buffer, size,
            DataStats::littleEndianHexToDecimal<uint32_t>(constant_value_hex));
    } else if (type == "int32") {
        int32_t *typed_buffer = reinterpret_cast<int32_t *>(buffer);
        std::fill_n(
            typed_buffer, size,
            DataStats::littleEndianHexToDecimal<int32_t>(constant_value_hex));
    } else if (type == "uint64") {
        uint64_t *typed_buffer = reinterpret_cast<uint64_t *>(buffer);
        std::fill_n(
            typed_buffer, size,
            DataStats::littleEndianHexToDecimal<uint64_t>(constant_value_hex));
    } else if (type == "int64") {
        int64_t *typed_buffer = reinterpret_cast<int64_t *>(buffer);
        std::fill_n(
            typed_buffer, size,
            DataStats::littleEndianHexToDecimal<int64_t>(constant_value_hex));
    }
    // The bitwise opperations in littleEndianHexToDecimal don't work on
    // floating point numbers. The "union" trick gets around that problem.
    else if (type == "float") {
        float *typed_buffer = reinterpret_cast<float *>(buffer);
        union {
            float f;
            uint32_t i;
        } u;
        u.i = DataStats::littleEndianHexToDecimal<uint32_t>(constant_value_hex);
        std::fill_n(typed_buffer, size, u.f);
    } else if (type == "double") {
        double *typed_buffer = reinterpret_cast<double *>(buffer);
        union {
            double f;
            uint64_t i;
        } u;
        u.i = DataStats::littleEndianHexToDecimal<uint64_t>(constant_value_hex);
        std::fill_n(typed_buffer, size, u.f);
    } else {
        errorHandler->raiseError(
            "Unexpected BDMS data type in getConstantValues",
            type + " for identifier " + identifier +
                ". Please contact the BDMS team.");
    }
}

/* This function does not care about multidimensional data.
It is the responsibility of the caller to reshape resulting chunks. */
std::vector<std::future<GenericVector>>
BaseBDMSDataManager::getDataArraysAsync(const std::string &sessionID,
                                        const std::vector<std::string> &ids) {
    std::vector<std::future<GenericVector>> futures;

    for (auto &bdmsDataID : ids) {
        futures.push_back(std::async(std::launch::async, [&sessionID,
                                                          &bdmsDataID, this] {
            GenericVector vec;
            char *buffer;
            DataStats stats = getStats(sessionID, bdmsDataID);
            std::string type = stats.getBDMSDataType();
            size_t size = stats.getTotalValueCount();

            // Set buffer and vector up for given type
            // This can't be type agnostic since we need to provide the type of
            // the returned data.
            if (type == "bool" || type == "char" || type == "byte" ||
                type == "int8" || type == "uint8") {
                assignBufferAndVector<uint8_t>(vec, buffer, size);
            } else if (type == "uint16") {
                assignBufferAndVector<uint16_t>(vec, buffer, size);
            } else if (type == "int16") {
                assignBufferAndVector<int16_t>(vec, buffer, size);
            } else if (type == "uint32") {
                assignBufferAndVector<uint32_t>(vec, buffer, size);
            } else if (type == "int32") {
                assignBufferAndVector<int32_t>(vec, buffer, size);
            } else if (type == "uint64") {
                assignBufferAndVector<uint64_t>(vec, buffer, size);
            } else if (type == "int64") {
                assignBufferAndVector<int64_t>(vec, buffer, size);
            } else if (type == "float") {
                assignBufferAndVector<float>(vec, buffer, size);
            } else if (type == "double") {
                assignBufferAndVector<double>(vec, buffer, size);
            } else {
                errorHandler->raiseError(
                    "Unexpected BDMS data type in getData",
                    type + "for Session ID " + sessionID + " and data ID " +
                        bdmsDataID + " is not one of the supported types.");
                return vec; // abort further processing
            }

            std::vector<std::string> identifierParts =
                DataStats::getIdentifierParts(bdmsDataID);
            std::string id_type_base = identifierParts[0];
            std::string id_type_special = identifierParts[1];

            if (id_type_base == "special" &&
                (id_type_special == "steps" || id_type_special == "range")) {
                getRangeValues(bdmsDataID, stats, buffer, type);
            } else if (id_type_base == "special" &&
                       id_type_special == "constant") {
                getConstantValues(bdmsDataID, buffer, size, type);
            } else {
                // if data can't be generated, get from BDMS
                std::string endpoint =
                    "/v5/data/" + sessionID + "/" + bdmsDataID;
                bool success;
                std::shared_ptr<httplib::Result> res;
                std::tie(success, res) = get(endpoint);

                if (!success) {
                    if (res && res->error() == httplib::Error::Success) {
                        errorHandler->raiseError(
                            "Request for getDataAsync failed",
                            "with status code " +
                                std::to_string((*res)->status) +
                                " for session ID " + sessionID +
                                " and data ID " + bdmsDataID);
                    } else {
                        errorHandler->raiseError(
                            "Request for getDataAsync failed",
                            "Reason unknown. Please contact the BDMS team.");
                    }
                }

                httplib::detail::gzip_decompressor comp;
                size_t offset = 0;
                if (!comp.decompress(
                        (*res)->body.c_str(), (*res)->body.length(),
                        [&offset, buffer, size](const char *decompData,
                                                size_t decompLength) {
                            memcpy(&buffer[offset], decompData, decompLength);
                            offset += decompLength;
                            return true;
                        })) {

                    errorHandler->raiseError(
                        "Decompression failed",
                        "Decompression failed for session ID " + sessionID +
                            " and data ID " + bdmsDataID +
                            ". Please contact the BDMS team.");
                }
            }
            return vec;
        }));
    }

    return futures;
}

const DataStats BaseBDMSDataManager::getStats(const SessionID &sessionID,
                                              const BDMSDataID &bdmsDataID) {
    try {
        DataStats stats = DataStats::fromIdentifier(bdmsDataID);
        return stats;
    } catch (...) {
        // Make HEAD request for data
        std::string baseEndpoint = "/v5/data/";
        std::ostringstream urlStream;
        urlStream << baseEndpoint << sessionID << "/" << bdmsDataID;

        bool success;
        std::shared_ptr<httplib::Result> response;
        std::tie(success, response) = head(urlStream.str());
        if (!success) {
            const std::string errorMessage =
                "Request for getStatsHead for session ID " + sessionID +
                " and data ID " + bdmsDataID + " failed.";
            errorHandler->raiseError("getStats error", errorMessage);
        }
        const json headers = (*response)->headers;
        // default to ""
        std::string data_type = headers.value("x-data-type", "");
        std::string data_count = headers.value("x-data-count", "");
        std::string zip_hash = headers.value("x-zip-hash", "");
        std::string min_value_hex = headers.value("x-min-value", "");
        std::string max_value_hex = headers.value("x-max-value", "");

        DataStats stats(bdmsDataID, data_type, data_count, zip_hash,
                        min_value_hex, max_value_hex);
        return stats;
    }
}