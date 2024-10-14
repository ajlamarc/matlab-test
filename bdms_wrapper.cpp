#include "bdms_wrapper.h"
#include "bdms_data.hpp"
#include "mex.h"
#include <cstring>
#include <vector>
#include <sstream>

void *createBlueDataManager(const char *apiKey, const char *baseUrl, const char *userAgent)
{
    initializeLogging();
    std::ostringstream debugMsg;
    debugMsg << "Debug: Creating BlueDataManager\n"
             << "Debug: API Key: " << apiKey << "\n"
             << "Debug: Base URL: " << baseUrl << "\n"
             << "Debug: User Agent: " << userAgent;
    logMessage(debugMsg.str().c_str());

    return new BlueDataManager(apiKey, baseUrl, userAgent);
}

void destroyBlueDataManager(void *bdm)
{
    logMessage("Debug: Destroying BlueDataManager");
    delete static_cast<BlueDataManager *>(bdm);
    closeLogging();
}

size_t getArrayWrapper(void *bdm, const char *sessionID, char **dataIDs, int numDataIDs, void *outputBuffer)
{
    std::ostringstream debugMsg;
    debugMsg << "Debug: Calling getArrayWrapper\n"
             << "Debug: Session ID: " << sessionID << "\n"
             << "Debug: Number of Data IDs: " << numDataIDs;

    if (numDataIDs > 0)
    {
        debugMsg << "\nDebug: First Data ID: " << dataIDs[0];
    }

    logMessage(debugMsg.str().c_str());

    BlueDataManager *manager = static_cast<BlueDataManager *>(bdm);
    std::vector<std::string> ids(dataIDs, dataIDs + numDataIDs);

    return manager->getArray(sessionID, ids, static_cast<char *>(outputBuffer));
}