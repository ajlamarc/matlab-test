#include "bdms_common.hpp"

class BDMSDataManager : public BaseBDMSDataManager
{
public:
    using BaseBDMSDataManager::BaseBDMSDataManager; // Inherit constructors

    mxArray *getArray(const SessionID &sessionID, std::vector<std::string> &dataIDs);
    mxArray *getArraysBySessionId(std::map<SessionID, std::vector<BDMSDataID>> &dataToDownload);
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

mxArray *BDMSDataManager::getArraysBySessionId(std::map<SessionID, std::vector<BDMSDataID>> &dataToDownload)
{
    mxArray *output = mxCreateCellMatrix(dataToDownload.size(), 1);

    // Create a vector to store futures for all sessions
    std::vector<std::pair<SessionID, std::vector<std::future<GenericVector>>>> allFutures;
    allFutures.reserve(dataToDownload.size());

    // Trigger getDataArraysAsync for all sessions
    for (auto &entry : dataToDownload)
    {
        SessionID &sessionID = entry.first;
        std::vector<BDMSDataID> &dataIDs = entry.second;
        allFutures.emplace_back(sessionID, getDataArraysAsync(sessionID, dataIDs));
    }

    // Process the futures
    for (size_t i = 0; i < allFutures.size(); ++i)
    {
        SessionID &sessionID = allFutures[i].first;
        std::vector<std::future<GenericVector>> &dataFutures = allFutures[i].second;

        // set session ID in output structure
        mxArray *outputForSessionID = mxCreateCellMatrix(dataFutures.size() + 1, 1);
        mxSetCell(outputForSessionID, 0, mxCreateString(sessionID.c_str()));

        std::vector<GenericVector> chunks(dataFutures.size());

        for (size_t j = 0; j < dataFutures.size(); ++j)
        {
            chunks[j] = dataFutures[j].get();
            size_t chunkByteSize = chunks[j].byteSize();

            mwSize dims[2] = {chunkByteSize, 1};
            mxArray *outputBytes = mxCreateNumericArray(2, dims, mxUINT8_CLASS, mxREAL);
            char *outputBuffer = static_cast<char *>(mxGetData(outputBytes));

            std::memcpy(outputBuffer, chunks[j].buffer(), chunkByteSize);

            mxSetCell(outputForSessionID, j + 1, outputBytes);
        }

        mxSetCell(output, i, outputForSessionID);
    }

    return output;
}
