#include "bdms_common.hpp"
#include <fstream>

void log_message(const std::string& msg) {
    std::ofstream log_file("bdms_debug.log", std::ios::app);
    log_file << "[MEX] " << msg << std::endl;
    log_file.close();
}

class BDMSDataManager : public BaseBDMSDataManager
{
public:
    using BaseBDMSDataManager::BaseBDMSDataManager; // Inherit constructors

    mxArray *getArray(const SessionID &sessionID, std::vector<BDMSDataID> &dataIDs);
    mxArray *getArraysBySessionId(const std::vector<std::vector<std::string>> &dataToDownload);
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

mxArray *BDMSDataManager::getArraysBySessionId(const std::vector<std::vector<std::string>> &dataToDownload)
{
    log_message("Starting getArraysBySessionId implementation");
    mxArray *output = mxCreateCellMatrix(dataToDownload.size(), 1);

    // Create a vector to store futures for all sessions
    log_message("Creating futures vector");
    std::vector<std::pair<SessionID, std::vector<std::future<GenericVector>>>> allFutures;
    allFutures.reserve(dataToDownload.size());

    // Trigger getDataArraysAsync for all sessions
    log_message("Triggering async calls");
    for (const auto &entry : dataToDownload)
    {
        const SessionID &sessionID = entry[0];  // First element is session ID
        log_message("Processing session ID: " + sessionID);
        const std::vector<BDMSDataID> dataIDs(entry.begin() + 1, entry.end());  // Rest are data IDs
        allFutures.emplace_back(sessionID, getDataArraysAsync(sessionID, dataIDs));
    }

    // Process the futures
    log_message("Processing futures");
    for (size_t i = 0; i < allFutures.size(); ++i)
    {
        log_message("Processing future " + std::to_string(i));
        const SessionID &sessionID = allFutures[i].first;
        std::vector<std::future<GenericVector>> &dataFutures = allFutures[i].second;

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

    log_message("Finished getArraysBySessionId implementation");
    return output;
}
