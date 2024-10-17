#include "bdms_common.hpp"

class BDMSDataManager : public BaseBDMSDataManager
{
public:
    using BaseBDMSDataManager::BaseBDMSDataManager; // Inherit constructors

    mxArray *getArray(const SessionID &sessionID, std::vector<std::string> &dataIDs);
};

mxArray *BDMSDataManager::getArray(const SessionID &sessionID, std::vector<std::string> &dataIDs)
{
    auto dataFutures = getDataArraysAsync(sessionID, dataIDs);
    std::vector<GenericVector> chunks(dataFutures.size());

    size_t totalByteSize = 0;
    for (size_t i = 0; i < dataFutures.size(); ++i)
    {
        chunks[i] = dataFutures[i].get();
        totalByteSize += chunks[i].byteSize();
    }

    std::ostringstream debugMsg;
    debugMsg << "Debug: Total byte size: " << totalByteSize;
    logMessage(debugMsg.str().c_str());

    // Create MATLAB uint8 column vector with the expected size
    mwSize dims[2] = {totalByteSize, 1};
    mxArray *outputBytes = mxCreateNumericArray(2, dims, mxUINT8_CLASS, mxREAL);
    char *outputBuffer = static_cast<char *>(mxGetData(outputBytes));

    size_t byteOffset = 0;
    for (size_t i = 0; i < chunks.size(); ++i)
    {
        size_t chunkByteSize = chunks[i].byteSize();
        std::memcpy(outputBuffer + byteOffset, chunks[i].buffer(), chunkByteSize);
        byteOffset += chunkByteSize;
    }

    return outputBytes;
}
