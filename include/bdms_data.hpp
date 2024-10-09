#include "bdms_common.hpp"

class BlueDataManager : public BaseBlueDataManager
{
public:
    size_t getArray(const SessionID &sessionID, std::vector<std::string> &dataIDs, char *outputBuffer);

    BlueDataManager(std::string apiKey, std::string baseUrl, std::string userAgent)
        : BaseBlueDataManager(apiKey, baseUrl, userAgent) {}
};

size_t
BlueDataManager::getArray(const SessionID &sessionID, std::vector<std::string> &dataIDs, char *outputBuffer)
{
    std::vector<GenericVector> chunks(dataIDs.size());
    auto dataFutures = getDataArraysAsync(sessionID, dataIDs);

    size_t totalSize = 0;
    for (auto &future : dataFutures)
    {
        GenericVector chunk = future.get();
        switch (chunk.type)
        {
        case BDMS_BOOL:
        case BDMS_UINT8:
        case BDMS_INT8:
        {
            size_t byteSize = chunk.data.uint8_vec->size() * sizeof(uint8_t);
            std::memcpy(outputBuffer + totalSize, chunk.data.uint8_vec->data(), byteSize);
            totalSize += byteSize;
            break;
        }
        case BDMS_UINT16:
        case BDMS_INT16:
        {
            size_t byteSize = chunk.data.uint16_vec->size() * sizeof(uint16_t);
            std::memcpy(outputBuffer + totalSize, chunk.data.uint16_vec->data(), byteSize);
            totalSize += byteSize;
            break;
        }
        case BDMS_UINT32:
        case BDMS_INT32:
        {
            size_t byteSize = chunk.data.uint32_vec->size() * sizeof(uint32_t);
            std::memcpy(outputBuffer + totalSize, chunk.data.uint32_vec->data(), byteSize);
            totalSize += byteSize;
            break;
        }
        case BDMS_UINT64:
        case BDMS_INT64:
        {
            size_t byteSize = chunk.data.uint64_vec->size() * sizeof(uint64_t);
            std::memcpy(outputBuffer + totalSize, chunk.data.uint64_vec->data(), byteSize);
            totalSize += byteSize;
            break;
        }
        case BDMS_FLOAT:
        {
            size_t byteSize = chunk.data.float_vec->size() * sizeof(float);
            std::memcpy(outputBuffer + totalSize, chunk.data.float_vec->data(), byteSize);
            totalSize += byteSize;
            break;
        }
        case BDMS_DOUBLE:
        {
            size_t byteSize = chunk.data.double_vec->size() * sizeof(double);
            std::memcpy(outputBuffer + totalSize, chunk.data.double_vec->data(), byteSize);
            totalSize += byteSize;
            break;
        }
        default:
            throw std::runtime_error("Unexpected vector type in getArray");
        }

        // free chunk memory
        chunk.clear();
    }

    return totalSize;
}