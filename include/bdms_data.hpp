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
    logMessage("Debug: Entering getArray function");
    logMessage("Debug: Session ID: " + sessionID);
    logMessage("Debug: Number of data IDs: " + std::to_string(dataIDs.size()));

    std::vector<GenericVector> chunks(dataIDs.size());
    auto dataFutures = getDataArraysAsync(sessionID, dataIDs);

    logMessage("Debug: Async calls made, processing results");

    size_t totalSize = 0;
    for (size_t i = 0; i < dataFutures.size(); ++i)
    {
        logMessage("Debug: Processing future " + std::to_string(i));
        GenericVector chunk = dataFutures[i].get();

        std::string chunkTypeStr;
        size_t chunkSize = 0;

        switch (chunk.type)
        {
        case BDMS_BOOL:
        case BDMS_UINT8:
        case BDMS_INT8:
        {
            chunkTypeStr = "UINT8/INT8";
            chunkSize = chunk.data.uint8_vec->size() * sizeof(uint8_t);
            break;
        }
        case BDMS_UINT16:
        case BDMS_INT16:
        {
            chunkTypeStr = "UINT16/INT16";
            chunkSize = chunk.data.uint16_vec->size() * sizeof(uint16_t);
            break;
        }
        case BDMS_UINT32:
        case BDMS_INT32:
        {
            chunkTypeStr = "UINT32/INT32";
            chunkSize = chunk.data.uint32_vec->size() * sizeof(uint32_t);
            break;
        }
        case BDMS_UINT64:
        case BDMS_INT64:
        {
            chunkTypeStr = "UINT64/INT64";
            chunkSize = chunk.data.uint64_vec->size() * sizeof(uint64_t);
            break;
        }
        case BDMS_FLOAT:
        {
            chunkTypeStr = "FLOAT";
            chunkSize = chunk.data.float_vec->size() * sizeof(float);
            break;
        }
        case BDMS_DOUBLE:
        {
            chunkTypeStr = "DOUBLE";
            chunkSize = chunk.data.double_vec->size() * sizeof(double);
            break;
        }
        default:
            logMessage("Error: Unexpected vector type in getArray");
            throw std::runtime_error("Unexpected vector type in getArray");
        }

        logMessage("Debug: Chunk type: " + chunkTypeStr + ", Size: " + std::to_string(chunkSize) + " bytes");
        logMessage("Debug: Current total size: " + std::to_string(totalSize) + " bytes");
        logMessage("Debug: Copying chunk to output buffer");

        std::memcpy(outputBuffer + totalSize, chunk.data.uint8_vec->data(), chunkSize);
        totalSize += chunkSize;

        logMessage("Debug: Chunk copied, new total size: " + std::to_string(totalSize) + " bytes");

        // free chunk memory
        chunk.clear();
        logMessage("Debug: Chunk memory cleared");
    }

    logMessage("Debug: All chunks processed");
    logMessage("Debug: Final total size: " + std::to_string(totalSize) + " bytes");
    logMessage("Debug: Exiting getArray function");

    return totalSize;
}