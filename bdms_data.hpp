#include "bdms_common.hpp"

class BDMSDataManager : public BaseBDMSDataManager
{
public:
    using BaseBDMSDataManager::BaseBDMSDataManager; // Inherit constructors

    mxArray *getArray(const SessionID &sessionID, std::vector<BDMSDataID> &dataIDs);
    mxArray *getArraysBySessionId(const std::map<SessionID, std::vector<BDMSDataID>> &dataToDownload, const std::vector<SessionID> &sessionInsertionOrder);
};

mxArray *BDMSDataManager::getArray(const SessionID &sessionID, std::vector<BDMSDataID> &dataIDs)
{
    auto dataFutures = getDataArraysAsync(sessionID, dataIDs);
    std::vector<GenericVector> chunks(dataFutures.size());

    size_t totalByteSize = 0;
    for (size_t i = 0; i < dataFutures.size(); ++i)
    {
        chunks[i] = dataFutures[i].get();
        totalByteSize += chunks[i].byteSize();
    }

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

mxArray *BDMSDataManager::getArraysBySessionId(const std::map<SessionID, std::vector<BDMSDataID>> &dataToDownload, const std::vector<SessionID> &sessionInsertionOrder)
{
    mxArray *output = mxCreateCellMatrix(sessionInsertionOrder.size(), 1);

    // Store futures for all sessions
    std::map<SessionID, std::vector<std::future<GenericVector>>> allFutures;

    // Trigger getDataArraysAsync for all sessions
    for (const SessionID &sessionID : sessionInsertionOrder)
    {
        const std::vector<BDMSDataID> &dataIDs = dataToDownload[sessionID];
        allFutures.emplace(sessionID, getDataArraysAsync(sessionID, dataIDs));
    }

    // Process the futures
    for (int i = 0; i < sessionInsertionOrder.size(); ++i)
    {
        const SessionID &sessionID = sessionInsertionOrder[i];
        const std::vector<std::future<GenericVector>> &dataFutures = allFutures[sessionID];

        // set session ID in output structure
        mxArray *outputForSessionID = mxCreateCellMatrix(dataFutures.size() + 1, 1);
        mxSetCell(outputForSessionID, 0, mxCreateString(sessionID.c_str()));

        for (size_t j = 0; j < dataFutures.size(); ++j)
        {
            GenericVector chunk = dataFutures[j].get();
            size_t chunkByteSize = chunk.byteSize();

            mwSize dims[2] = {chunkByteSize, 1};
            mxArray *outputBytes = mxCreateNumericArray(2, dims, mxUINT8_CLASS, mxREAL);
            char *outputBuffer = static_cast<char *>(mxGetData(outputBytes));

            std::memcpy(outputBuffer, chunk.buffer(), chunkByteSize);

            mxSetCell(outputForSessionID, j + 1, outputBytes);
        }

        mxSetCell(output, i, outputForSessionID);
    }

    return output;
}
