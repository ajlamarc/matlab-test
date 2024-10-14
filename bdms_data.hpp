#include "bdms_common.hpp"

class BDMSDataManager : public BaseBDMSDataManager
{
public:
    using BaseBDMSDataManager::BaseBDMSDataManager; // Inherit constructors

    size_t getArray(const SessionID &sessionID, std::vector<std::string> &dataIDs, char *outputBuffer);
};

size_t BDMSDataManager::getArray(const SessionID &sessionID, std::vector<std::string> &dataIDs, char *outputBuffer)
{
    std::ostringstream ss;
    ss << "Debug: Entering getArray function";
    logMessage(ss.str().c_str());

    ss.str("");
    ss << "Debug: Session ID: " << sessionID;
    logMessage(ss.str().c_str());

    ss.str("");
    ss << "Debug: Number of data IDs: " << dataIDs.size();
    logMessage(ss.str().c_str());

    auto dataFutures = getDataArraysAsync(sessionID, dataIDs);

    logMessage("Debug: Async calls made, processing results");

    size_t totalSize = 0;
    for (size_t i = 0; i < dataFutures.size(); ++i)
    {
        ss.str("");
        ss << "Debug: Processing future " << i;
        logMessage(ss.str().c_str());

        GenericVector chunk = dataFutures[i].get();
        size_t chunkSize = chunk.byteSize();

        ss.str("");
        ss << "Debug: Size: " << chunkSize << " bytes";
        logMessage(ss.str().c_str());

        ss.str("");
        ss << "Debug: Current total size: " << totalSize << " bytes";
        logMessage(ss.str().c_str());

        logMessage("Debug: Copying chunk to output buffer");

        std::memcpy(outputBuffer + totalSize, chunk.buffer(), chunkSize);
        totalSize += chunkSize;

        ss.str("");
        ss << "Debug: Chunk copied, new total size: " << totalSize << " bytes";
        logMessage(ss.str().c_str());

        // The chunk will be automatically freed when it goes out of scope
        logMessage("Debug: Chunk memory will be automatically cleared");
    }

    logMessage("Debug: All chunks processed");

    ss.str("");
    ss << "Debug: Final total size: " << totalSize << " bytes";
    logMessage(ss.str().c_str());

    logMessage("Debug: Exiting getArray function");

    return totalSize;
}
