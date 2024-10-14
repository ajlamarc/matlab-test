#include "bdms_wrapper.h"
#include "bdms_data.hpp"
#include "mex.h"
#include <cstring>
#include <vector>

void *createBlueDataManager(const char *apiKey, const char *baseUrl, const char *userAgent)
{
    mexWarnMsgTxt("Debug: Creating BlueDataManager\n");
    mexWarnMsgTxt("Debug: API Key: %s\n", apiKey);
    mexWarnMsgTxt("Debug: Base URL: %s\n", baseUrl);
    mexWarnMsgTxt("Debug: User Agent: %s\n", userAgent);

    return new BlueDataManager(apiKey, baseUrl, userAgent);
}

void destroyBlueDataManager(void *bdm)
{
    mexWarnMsgTxt("Debug: Destroying BlueDataManager\n");
    delete static_cast<BlueDataManager *>(bdm);
}

size_t getArrayWrapper(void *bdm, const char *sessionID, char **dataIDs, int numDataIDs, void *outputBuffer)
{
    mexWarnMsgTxt("Debug: Calling getArrayWrapper\n");
    mexWarnMsgTxt("Debug: Session ID: %s\n", sessionID);
    if (numDataIDs > 0)
    {
        mexWarnMsgTxt("Debug: First Data ID: %s\n", dataIDs[0]);
    }
    mexWarnMsgTxt("Debug: Number of Data IDs: %d\n", numDataIDs);

    BlueDataManager *manager = static_cast<BlueDataManager *>(bdm);
    std::vector<std::string> ids(dataIDs, dataIDs + numDataIDs);

    return manager->getArray(sessionID, ids, static_cast<char *>(outputBuffer));
}