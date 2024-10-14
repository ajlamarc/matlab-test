#define CPPHTTPLIB_OPENSSL_SUPPORT
#define CPPHTTPLIB_ZLIB_SUPPORT
#define CPPHTTPLIB_READ_TIMEOUT_SECOND 20

// TODO: remove
#include "mex.h"

#include "httplib.h"
#include "nlohmann/json.hpp"
using json = nlohmann::json;

#include <set>
#include <vector>
#include <variant>
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

enum VectorType
{
    BDMS_BOOL,
    BDMS_UINT8,
    BDMS_INT8,
    BDMS_UINT16,
    BDMS_INT16,
    BDMS_UINT32,
    BDMS_INT32,
    BDMS_UINT64,
    BDMS_INT64,
    BDMS_FLOAT,
    BDMS_DOUBLE
};

struct GenericVector
{
    VectorType type;
    union
    {
        std::vector<uint8_t> *uint8_vec;
        std::vector<int8_t> *int8_vec;
        std::vector<uint16_t> *uint16_vec;
        std::vector<int16_t> *int16_vec;
        std::vector<uint32_t> *uint32_vec;
        std::vector<int32_t> *int32_vec;
        std::vector<uint64_t> *uint64_vec;
        std::vector<int64_t> *int64_vec;
        std::vector<float> *float_vec;
        std::vector<double> *double_vec;
    } data;

    GenericVector() : type(BDMS_BOOL), data{nullptr} {}
    ~GenericVector()
    {
        clear();
    }

    void clear()
    {
        delete data.uint8_vec;
        data.uint8_vec = nullptr;
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

enum HTTPMethod
{
    GET,
    HEAD,
    POST
};

// see https://stackoverflow.com/a/64054899
#if __cplusplus < 201402L
template <class T, class... Args>
std::unique_ptr<T> make_unique(Args &&...args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#else
using std::make_unique;
#endif

class DataStats
{
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
    const int getDimensionality() const;
    const int getTotalValueCount() const;
    template <typename T>
    const T getMinValue() const;
    template <typename T>
    const T getMaxValue() const;
    DataStats(BDMSDataID idVal, std::string dataTypeVal,
              std::string dataCountVal, std::string zipHashVal,
              std::string minValueHexVal, std::string maxValueHexVal)
        : identifier(idVal), data_type(dataTypeVal),
          data_count(std::stol(dataCountVal)), zip_hash(zipHashVal),
          min_value_hex(minValueHexVal), max_value_hex(maxValueHexVal) {}
};

const DataStats DataStats::fromIdentifier(const BDMSDataID &bdmsDataID)
{
    std::vector<std::string> identifierParts = getIdentifierParts(bdmsDataID);

    std::string data_type;
    std::string data_count;
    std::string zip_hash;
    std::string min_value_hex;
    std::string max_value_hex;

    if (identifierParts[0] == "v1")
    {
        data_type = identifierParts[1];
        data_count = identifierParts[2];
        zip_hash = identifierParts[3];
        min_value_hex = identifierParts[4];
        max_value_hex = identifierParts[5];
    }
    else if (identifierParts[0] == "special")
    {
        if (identifierParts[1] == "constant")
        {
            data_type = identifierParts[2];
            data_count = identifierParts[3];
            auto value_hex = identifierParts[4];
            zip_hash = "";
            min_value_hex = value_hex;
            max_value_hex = value_hex;
        }
        else if (identifierParts[1] == "steps" ||
                 identifierParts[1] == "range")
        {
            data_type = identifierParts[2];
            data_count = identifierParts[3];
            zip_hash = "";
            min_value_hex = identifierParts[5];
            max_value_hex = identifierParts[6];
        }
        else if (identifierParts[1] == "bdmsv1")
        {
            data_type = identifierParts[2];
            data_count = identifierParts[3];
            zip_hash = "";
            min_value_hex = identifierParts[4];
            max_value_hex = identifierParts[5];
        }
        else
        {
            throw std::runtime_error("Unexpected data identifier.");
        }
    }
    else if (identifierParts[0] == "function" &&
             identifierParts[1] == "region")
    {
        data_type = identifierParts[2];
        data_count = identifierParts[3];
        zip_hash = "";
        min_value_hex = identifierParts[identifierParts.size() - 2];
        max_value_hex = identifierParts[identifierParts.size() - 1];
    }
    else
    {
        throw std::runtime_error("Unexpected data identifier.");
    }
    return DataStats(bdmsDataID, data_type, data_count, zip_hash, min_value_hex,
                     max_value_hex);
}

const std::vector<std::string>
DataStats::getIdentifierParts(const BDMSDataID &bdmsDataID)
{
    std::istringstream dataIDStream(bdmsDataID);
    std::string part;
    std::vector<std::string> identifierParts;
    while (std::getline(dataIDStream, part, ':'))
    {
        identifierParts.push_back(part);
    }

    // if last part was empty, the above loop does not add it, so add it here
    if (bdmsDataID.back() == ':')
    {
        identifierParts.push_back("");
    }

    return identifierParts;
}

// convert hex string (BDMS data identifier) to any value type.  Will work for
// times and values.
template <typename T>
T DataStats::littleEndianHexToDecimal(std::string &littleEndianHex)
{
    T decimal_value = 0;
    size_t length = littleEndianHex.length();
    size_t num_bytes = sizeof(T);
    std::stringstream ss;
    for (size_t i = 0; i < num_bytes; ++i)
    {
        std::string byte_string = littleEndianHex.substr(i * 2, 2);
        unsigned int byte_value;
        ss.clear();
        ss << std::hex << byte_string;
        ss >> byte_value;
        decimal_value |= (static_cast<T>(byte_value) << (i * 8));
    }
    return decimal_value;
}

const std::string DataStats::getBDMSDataType() const
{
    std::istringstream ss(this->data_type);
    std::string bdmsDataType;
    // "uint64,1" -> "uint64"
    std::getline(ss, bdmsDataType, ',');
    return bdmsDataType;
}

const size_t DataStats::getDataCount() const { return this->data_count; }

const int DataStats::getTotalValueCount() const
{
    const int dimensionality = this->getDimensionality();
    if (dimensionality == -1)
        return -1;
    return this->data_count * dimensionality;
}

/* Returns -1 if data is 3-dimensional or greater, otherwise the single
dimensionality value.  1 represents 1-dimensional, any other is two-dimensional.
*/
const int DataStats::getDimensionality() const
{
    size_t pos = this->data_type.find(',');
    if (pos != std::string::npos)
    {
        std::string dimensionality = this->data_type.substr(pos + 1);

        // found a second comma, so it's 3-dimensional or greater
        size_t pos2 = dimensionality.find(',');
        if (pos2 != std::string::npos)
            return -1;
        return std::stoi(dimensionality);
    }
    // auto modalBody =
    //     "Failed to parse dimensionality of the following data identifier: " +
    //     this->identifier + ". Please contact the BDMS team.";
    // MessageBox(NULL, TEXT(modalBody.c_str()), TEXT("getDimensionality error"),
    //            MB_ICONERROR);
    throw std::runtime_error("getDimensionality error");
}

template <typename T>
const T DataStats::getMinValue() const
{
    if (this->min_value_hex.empty())
    {
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

template <typename T>
const T DataStats::getMaxValue() const
{
    if (this->max_value_hex.empty())
    {
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

// Common, static operations on BDMS data.
class DataFunctions
{
public:
    static void getRangeValues(const BDMSDataID &bdmsDataID, DataStats stats,
                               char *buffer, std::string type);
    static void getConstantValues(const BDMSDataID &bdmsDataID, char *buffer,
                                  size_t size, std::string type);
    template <typename T>
    static void fillBufferWithSequence(T *buffer, T min_value, T max_value,
                                       T step_value);
    template <typename T>
    static VectorType getVectorType();
    template <typename T>
    static void assignVector(GenericVector &vec, size_t size);
    template <typename T>
    static void assignBufferAndVector(GenericVector &vec, char *&buffer,
                                      size_t size);
};

template <>
VectorType DataFunctions::getVectorType<bool>() { return BDMS_BOOL; }
template <>
VectorType DataFunctions::getVectorType<uint8_t>() { return BDMS_UINT8; }
template <>
VectorType DataFunctions::getVectorType<int8_t>() { return BDMS_INT8; }
template <>
VectorType DataFunctions::getVectorType<uint16_t>() { return BDMS_UINT16; }
template <>
VectorType DataFunctions::getVectorType<int16_t>() { return BDMS_INT16; }
template <>
VectorType DataFunctions::getVectorType<uint32_t>() { return BDMS_UINT32; }
template <>
VectorType DataFunctions::getVectorType<int32_t>() { return BDMS_INT32; }
template <>
VectorType DataFunctions::getVectorType<uint64_t>() { return BDMS_UINT64; }
template <>
VectorType DataFunctions::getVectorType<int64_t>() { return BDMS_INT64; }
template <>
VectorType DataFunctions::getVectorType<float>() { return BDMS_FLOAT; }
template <>
VectorType DataFunctions::getVectorType<double>() { return BDMS_DOUBLE; }

// Initialize vec for any type of GenericVector
template <typename T>
void DataFunctions::assignVector(GenericVector &vec, size_t size)
{
    vec.type = getVectorType<T>();
    switch (vec.type)
    {
    case BDMS_BOOL:
    case BDMS_UINT8:
    case BDMS_INT8:
        vec.data.uint8_vec = new std::vector<uint8_t>(size);
        break;
    case BDMS_UINT16:
    case BDMS_INT16:
        vec.data.uint16_vec = new std::vector<uint16_t>(size);
        break;
    case BDMS_UINT32:
    case BDMS_INT32:
        vec.data.uint32_vec = new std::vector<uint32_t>(size);
        break;
    case BDMS_UINT64:
    case BDMS_INT64:
        vec.data.uint64_vec = new std::vector<uint64_t>(size);
        break;
    case BDMS_FLOAT:
        vec.data.float_vec = new std::vector<float>(size);
        break;
    case BDMS_DOUBLE:
        vec.data.double_vec = new std::vector<double>(size);
        break;
    default:
        throw std::runtime_error("Unexpected vector type in assignVector");
    }
}

// Initalize vec and buffer for any type of GenericVector
template <typename T>
void DataFunctions::assignBufferAndVector(GenericVector &vec, char *&buffer,
                                          size_t size)
{
    assignVector<T>(vec, size);
    buffer = reinterpret_cast<char *>(vec.data.uint8_vec->data());
}

template <typename T>
void DataFunctions::fillBufferWithSequence(T *buffer, T min_value,
                                           T max_value, T step_value)
{
    bool is_forward_stepping = step_value > 0;

    if (is_forward_stepping)
    {
        size_t index = 0;
        for (T value = min_value; value <= max_value; value += step_value)
        {
            buffer[index] = value;
            index++;
        }
    }
    else
    {
        size_t index = 0;
        for (T value = max_value; value >= min_value; value += step_value)
        {
            buffer[index] = value;
            index++;
        }
    }
}

void DataFunctions::getRangeValues(const BDMSDataID &identifier,
                                   DataStats stats, char *buffer,
                                   std::string type)
{
    std::vector<std::string> identifierParts =
        DataStats::getIdentifierParts(identifier);
    std::string step_value_dec = identifierParts[4];

    if (type == "char" || type == "byte" || type == "int8" || type == "uint8")
    {
        uint8_t *typed_buffer = reinterpret_cast<uint8_t *>(buffer);
        fillBufferWithSequence<uint8_t>(
            typed_buffer, stats.getMinValue<uint8_t>(),
            stats.getMaxValue<uint8_t>(),
            static_cast<uint8_t>(std::stoul(step_value_dec)));
    }
    else if (type == "uint16")
    {
        uint16_t *typed_buffer = reinterpret_cast<uint16_t *>(buffer);
        fillBufferWithSequence<uint16_t>(
            typed_buffer, stats.getMinValue<uint16_t>(),
            stats.getMaxValue<uint16_t>(),
            static_cast<uint16_t>(std::stoul(step_value_dec)));
    }
    else if (type == "int16")
    {
        int16_t *typed_buffer = reinterpret_cast<int16_t *>(buffer);
        fillBufferWithSequence<int16_t>(
            typed_buffer, stats.getMinValue<int16_t>(),
            stats.getMaxValue<int16_t>(),
            static_cast<int16_t>(std::stoi(step_value_dec)));
    }
    else if (type == "uint32")
    {
        uint32_t *typed_buffer = reinterpret_cast<uint32_t *>(buffer);
        fillBufferWithSequence<uint32_t>(
            typed_buffer, stats.getMinValue<uint32_t>(),
            stats.getMaxValue<uint32_t>(), std::stoul(step_value_dec));
    }
    else if (type == "int32")
    {
        int32_t *typed_buffer = reinterpret_cast<int32_t *>(buffer);
        fillBufferWithSequence<int32_t>(
            typed_buffer, stats.getMinValue<int32_t>(),
            stats.getMaxValue<int32_t>(),
            static_cast<uint32_t>(std::stoi(step_value_dec)));
    }
    else if (type == "uint64")
    {
        uint64_t *typed_buffer = reinterpret_cast<uint64_t *>(buffer);
        fillBufferWithSequence<uint64_t>(
            typed_buffer, stats.getMinValue<uint64_t>(),
            stats.getMaxValue<uint64_t>(), std::stoull(step_value_dec));
    }
    else if (type == "int64")
    {
        int64_t *typed_buffer = reinterpret_cast<int64_t *>(buffer);
        fillBufferWithSequence<int64_t>(
            typed_buffer, stats.getMinValue<int64_t>(),
            stats.getMaxValue<int64_t>(), std::stoll(step_value_dec));
    }
    else
    {
        // TODO: catch error and raise back to MATLAB
        // auto modalBody =
        //     "Unexpected BDMS data type in getStepsValues: " + type +
        //     "for identifier " + identifier;
        // MessageBox(NULL, TEXT(modalBody.c_str()), TEXT("getStepsValues error"),
        //            MB_ICONERROR);
        throw std::runtime_error("Unexpected data identifier.");
    }
}

void DataFunctions::getConstantValues(const BDMSDataID &identifier,
                                      char *buffer, size_t size,
                                      std::string type)
{
    std::vector<std::string> identifierParts =
        DataStats::getIdentifierParts(identifier);
    std::string constant_value_hex = identifierParts[4];

    if (type == "bool" || type == "char" || type == "byte" || type == "int8" ||
        type == "uint8")
    {
        uint8_t *typed_buffer = reinterpret_cast<uint8_t *>(buffer);
        std::fill_n(
            typed_buffer, size,
            DataStats::littleEndianHexToDecimal<uint8_t>(constant_value_hex));
    }
    else if (type == "uint16")
    {
        uint16_t *typed_buffer = reinterpret_cast<uint16_t *>(buffer);
        std::fill_n(
            typed_buffer, size,
            DataStats::littleEndianHexToDecimal<uint16_t>(constant_value_hex));
    }
    else if (type == "int16")
    {
        int16_t *typed_buffer = reinterpret_cast<int16_t *>(buffer);
        std::fill_n(
            typed_buffer, size,
            DataStats::littleEndianHexToDecimal<int16_t>(constant_value_hex));
    }
    else if (type == "uint32")
    {
        uint32_t *typed_buffer = reinterpret_cast<uint32_t *>(buffer);
        std::fill_n(
            typed_buffer, size,
            DataStats::littleEndianHexToDecimal<uint32_t>(constant_value_hex));
    }
    else if (type == "int32")
    {
        int32_t *typed_buffer = reinterpret_cast<int32_t *>(buffer);
        std::fill_n(
            typed_buffer, size,
            DataStats::littleEndianHexToDecimal<int32_t>(constant_value_hex));
    }
    else if (type == "uint64")
    {
        uint64_t *typed_buffer = reinterpret_cast<uint64_t *>(buffer);
        std::fill_n(
            typed_buffer, size,
            DataStats::littleEndianHexToDecimal<uint64_t>(constant_value_hex));
    }
    else if (type == "int64")
    {
        int64_t *typed_buffer = reinterpret_cast<int64_t *>(buffer);
        std::fill_n(
            typed_buffer, size,
            DataStats::littleEndianHexToDecimal<int64_t>(constant_value_hex));
    }
    // The bitwise opperations in littleEndianHexToDecimal don't work on
    // floating point numbers. The "union" trick gets around that problem.
    else if (type == "float")
    {
        float *typed_buffer = reinterpret_cast<float *>(buffer);
        union
        {
            float f;
            uint32_t i;
        } u;
        u.i = DataStats::littleEndianHexToDecimal<uint32_t>(constant_value_hex);
        std::fill_n(typed_buffer, size, u.f);
    }
    else if (type == "double")
    {
        double *typed_buffer = reinterpret_cast<double *>(buffer);
        union
        {
            double f;
            uint64_t i;
        } u;
        u.i = DataStats::littleEndianHexToDecimal<uint64_t>(constant_value_hex);
        std::fill_n(typed_buffer, size, u.f);
    }
    else
    {
        // TODO: catch error and raise back to MATLAB
        // auto modalBody =
        //     "Unexpected BDMS data type in getStepsValues: " + type +
        //     "for identifier " + identifier;
        // MessageBox(NULL, TEXT(modalBody.c_str()), TEXT("getStepsValues error"),
        //            MB_ICONERROR);
        throw std::runtime_error("Unexpected data identifier.");
    }
}

class BaseBlueDataManager
{
protected:
    std::string _apiKey;
    std::string _baseUrl;
    std::string _userAgent;
    httplib::Client *client();
    // ***************************************
    std::pair<bool, std::shared_ptr<httplib::Result>>
    request(const std::string &endpoint, const json &body, HTTPMethod method);
    std::pair<bool, std::shared_ptr<httplib::Result>>
    post(const std::string &endpoint, const json &body);
    std::pair<bool, std::shared_ptr<httplib::Result>>
    head(const std::string &endpoint);
    std::pair<bool, std::shared_ptr<httplib::Result>>
    get(const std::string &endpoint);
    std::vector<std::future<GenericVector>>
    getDataArraysAsync(const std::string &sessionID,
                       const std::vector<std::string> &ids);

public:
    const DataStats getStats(const SessionID &sessionID,
                             const BDMSDataID &bdmsDataID);

    BaseBlueDataManager(std::string apiKey, std::string baseUrl, std::string userAgent)
        : _apiKey(apiKey), _baseUrl(baseUrl), _userAgent(userAgent) {}
};

httplib::Client *BaseBlueDataManager::client()
{
    static thread_local std::unique_ptr<httplib::Client> _client =
        make_unique<httplib::Client>(_baseUrl);
    // default connection timeout is 300 seconds, which is sufficient
    _client->set_read_timeout(std::chrono::seconds(300));
    _client->set_write_timeout(std::chrono::seconds(300));
    _client->set_keep_alive(true);
    _client->set_follow_location(true);
    _client->set_bearer_token_auth(_apiKey);
    return _client.get();
}

std::pair<bool, std::shared_ptr<httplib::Result>>
BaseBlueDataManager::request(const std::string &endpoint, const json &body,
                             HTTPMethod method)
{
    httplib::Headers headers = {
        {"User-Agent", _userAgent}};
    if (method == POST)
    {
        headers.emplace("Accept", "application/json");
    }
    auto cl = client();
    std::set<int> retryStatusCodes = {429, 500, 502, 503, 504};
    for (int retry = 0; retry < 4; ++retry)
    {
        if (retry > 0)
        {
            mexWarnMsgTxt("Debug: Retrying request (attempt %d)\n", retry + 1);
            std::this_thread::sleep_for(std::chrono::seconds(retry - 1));
        }
        std::shared_ptr<httplib::Result> resPtr;
        if (method == POST)
        {
            resPtr = std::make_shared<httplib::Result>(
                cl->Post(endpoint, headers, body.dump(), "application/json"));
        }
        else if (method == HEAD)
        {
            resPtr =
                std::make_shared<httplib::Result>(cl->Head(endpoint, headers));
        }
        else if (method == GET)
        {
            resPtr =
                std::make_shared<httplib::Result>(cl->Get(endpoint, headers));
        }

        mexWarnMsgTxt("Debug: Request method: %s\n", (method == POST ? "POST" : (method == HEAD ? "HEAD" : "GET")));
        mexWarnMsgTxt("Debug: Endpoint: %s\n", endpoint.c_str());

        if (resPtr)
        {
            mexWarnMsgTxt("Debug: Response received\n");
            mexWarnMsgTxt("Debug: Error code: %d\n", static_cast<int>(resPtr->error()));

            if (resPtr->error() == httplib::Error::Success)
            {
                mexWarnMsgTxt("Debug: Status code: %d\n", (*resPtr)->status);
                mexWarnMsgTxt("Debug: Response body size: %zu bytes\n", (*resPtr)->body.size());

                if ((*resPtr)->status == 403 || (*resPtr)->status == 401)
                {
                    mexWarnMsgTxt("Debug: Authentication error (status %d)\n", (*resPtr)->status);
                    return std::make_pair(false, resPtr);
                }
                else if ((*resPtr)->status == 200)
                {
                    mexWarnMsgTxt("Debug: Request successful\n");
                    return std::make_pair(true, resPtr);
                }
                else if (retryStatusCodes.find((*resPtr)->status) == retryStatusCodes.end())
                {
                    mexWarnMsgTxt("Debug: Non-retryable error\n");
                    return std::make_pair(false, resPtr);
                }
                else
                {
                    mexWarnMsgTxt("Debug: Retryable error (status %d)\n", (*resPtr)->status);
                }
            }
            else
            {
                mexWarnMsgTxt("Debug: HTTP request failed at transport layer\n");
            }
        }
        else
        {
            mexWarnMsgTxt("Debug: No response received\n");
        }

        if (retry == 3)
        {
            mexWarnMsgTxt("Debug: Max retries reached\n");
            return std::make_pair(false, resPtr);
        }
    }

    mexWarnMsgTxt("Debug: Request failed after all retries\n");
    return std::make_pair(false, nullptr);
}

std::pair<bool, std::shared_ptr<httplib::Result>>
BaseBlueDataManager::head(const std::string &endpoint)
{
    return request(endpoint, json({}), HEAD);
}

std::pair<bool, std::shared_ptr<httplib::Result>>
BaseBlueDataManager::post(const std::string &endpoint, const json &body)
{
    return request(endpoint, body, POST);
}

std::pair<bool, std::shared_ptr<httplib::Result>>
BaseBlueDataManager::get(const std::string &endpoint)
{
    return request(endpoint, json({}), GET);
}

/* This function does not care about multidimensional data.
It is the responsibility of the caller to reshape resulting chunks. */
std::vector<std::future<GenericVector>>
BaseBlueDataManager::getDataArraysAsync(const std::string &sessionID,
                                        const std::vector<std::string> &ids)
{
    std::vector<std::future<GenericVector>> futures;

    for (auto &bdmsDataID : ids)
    {
        futures.push_back(std::async(
            std::launch::async, [&sessionID, &bdmsDataID, this]
            {
                GenericVector vec;
                char *buffer;
                DataStats stats = getStats(sessionID, bdmsDataID);
                std::string type = stats.getBDMSDataType();
                int size = stats.getTotalValueCount();

                if (size == -1) {
                    return vec; // abort further processing
                }

                // Set buffer and vector up for given type
                // This can't be type agnostic since we need to provide the type of
                // the returned data.
                if (type == "bool" || type == "char" || type == "byte" ||
                    type == "int8" || type == "uint8") {
                    DataFunctions::assignBufferAndVector<uint8_t>(vec, buffer, size);
                } else if (type == "uint16") {
                    DataFunctions::assignBufferAndVector<uint16_t>(vec, buffer, size);
                } else if (type == "int16") {
                    DataFunctions::assignBufferAndVector<int16_t>(vec, buffer, size);
                } else if (type == "uint32") {
                    DataFunctions::assignBufferAndVector<uint32_t>(vec, buffer, size);
                } else if (type == "int32") {
                    DataFunctions::assignBufferAndVector<int32_t>(vec, buffer, size);
                } else if (type == "uint64") {
                    DataFunctions::assignBufferAndVector<uint64_t>(vec, buffer, size);
                } else if (type == "int64") {
                    DataFunctions::assignBufferAndVector<int64_t>(vec, buffer, size);
                } else if (type == "float") {
                    DataFunctions::assignBufferAndVector<float>(vec, buffer, size);
                } else if (type == "double") {
                    DataFunctions::assignBufferAndVector<double>(vec, buffer, size);
                } else {
                    throw std::runtime_error("Unexpected BDMS data type in getData");
                    // TODO: surface error
                    // auto modalBody =
                    //     "Unexpected BDMS data type in getData: " + type +
                    //     "for Session ID " + sessionID + "and data ID " + bdmsDataID;
                    // MessageBox(NULL, TEXT(modalBody.c_str()), TEXT("getData error"),
                    //            MB_ICONERROR);
                    return vec; // abort further processing
                }

                std::vector<std::string> identifierParts =
                    DataStats::getIdentifierParts(bdmsDataID);
                std::string id_type_base = identifierParts[0];
                std::string id_type_special = identifierParts[1];

                if (id_type_base == "special" &&
                    (id_type_special == "steps" || id_type_special == "range")) {
                    DataFunctions::getRangeValues(bdmsDataID, stats, buffer, type);
                } else if (id_type_base == "special" &&
                        id_type_special == "constant") {
                    DataFunctions::getConstantValues(bdmsDataID, buffer, size, type);
                } else {
                    // if data can't be generated, get from BDMS
                    std::string endpoint =
                        "/v5/data/" + sessionID + "/" + bdmsDataID;
                    bool success;
                    std::shared_ptr<httplib::Result> res;
                    std::tie(success, res) = get(endpoint);

                    if (!success) {
                        // TODO: surface error to MATLAB
                        // MessageBox(NULL, TEXT("Request for getDataAsync failed."),
                        //            TEXT("getDataAsync error"), MB_ICONERROR);
                        throw std::runtime_error("getDataAsync error");
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
                        // TODO: surface error to MATLAB
                        // MessageBox(NULL, TEXT("Decompression failed."),
                        //            TEXT("getDataAsync error"), MB_ICONERROR);
                        throw std::runtime_error("Decompression failed.");
                    }
                }
                return vec; }));
    }

    return futures;
}

const DataStats BaseBlueDataManager::getStats(const SessionID &sessionID,
                                              const BDMSDataID &bdmsDataID)
{
    try
    {
        DataStats stats = DataStats::fromIdentifier(bdmsDataID);
        return stats;
    }
    catch (...)
    {
        // Make HEAD request for data
        std::string baseEndpoint = "/v5/data/";
        std::ostringstream urlStream;
        urlStream << baseEndpoint << sessionID << "/" << bdmsDataID;

        bool success;
        std::shared_ptr<httplib::Result> response;
        std::tie(success, response) = head(urlStream.str());
        if (!success)
        {
            // auto modalBody = "Request for getStatsHead for session ID " +
            //                  sessionID + " and data ID " + bdmsDataID +
            //                  " failed.";
            // MessageBox(NULL, TEXT(modalBody.c_str()),
            //            TEXT("getStatsHead error"), MB_ICONERROR);
            throw std::runtime_error("getStatsHead error");
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