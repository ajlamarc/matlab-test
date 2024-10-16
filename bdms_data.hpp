#include "bdms_common.hpp"

class BDMSDataManager : public BaseBDMSDataManager
{
public:
    using BaseBDMSDataManager::BaseBDMSDataManager; // Inherit constructors

    size_t getArray(const SessionID &sessionID, std::vector<std::string> &dataIDs, char *outputBuffer);
};

size_t BDMSDataManager::getArray(const SessionID &sessionID, std::vector<std::string> &dataIDs, char *outputBuffer)
{
    auto dataFutures = getDataArraysAsync(sessionID, dataIDs);

    size_t totalSize = 0;
    for (size_t i = 0; i < dataFutures.size(); ++i)
    {
        GenericVector chunk = dataFutures[i].get();
        size_t chunkSize = chunk.byteSize();

        std::memcpy(outputBuffer + totalSize, chunk.buffer(), chunkSize);
        totalSize += chunkSize;
    }

    return totalSize;
}
