#include "bdms_wrapper.h"
#include "bdms_data.hpp"
#include "mex.h"
#include <cstring>
#include <vector>

void *createBlueDataManager(const char *apiKey, const char *baseUrl, const char *userAgent)
{
    mexPrintf("Debug: Creating BlueDataManager\n");
    mexPrintf("Debug: API Key: %s\n", apiKey);
    mexPrintf("Debug: Base URL: %s\n", baseUrl);
    mexPrintf("Debug: User Agent: %s\n", userAgent);

    return new BlueDataManager(apiKey, baseUrl, userAgent);
}

void destroyBlueDataManager(void *bdm)
{
    mexPrintf("Debug: Destroying BlueDataManager\n");
    delete static_cast<BlueDataManager *>(bdm);
}

size_t getArrayWrapper(void *bdm, const char *sessionID, char **dataIDs, int numDataIDs, void *outputBuffer)
{
    mexPrintf("Debug: Calling getArrayWrapper\n");
    mexPrintf("Debug: Session ID: %s\n", sessionID);
    if (numDataIDs > 0)
    {
        mexPrintf("Debug: First Data ID: %s\n", dataIDs[0]);
    }
    mexPrintf("Debug: Number of Data IDs: %d\n", numDataIDs);

    BlueDataManager *manager = static_cast<BlueDataManager *>(bdm);
    std::vector<std::string> ids(dataIDs, dataIDs + numDataIDs);

    return manager->getArray(sessionID, ids, static_cast<char *>(outputBuffer));
}