#include "bdms_wrapper.h"
#include "bdms_data.hpp"
#include <cstring>
#include <vector>

void *createBlueDataManager(const char *apiKey, const char *baseUrl, const char *userAgent)
{
    return new BlueDataManager(apiKey, baseUrl, userAgent);
}

void destroyBlueDataManager(void *bdm)
{
    delete static_cast<BlueDataManager *>(bdm);
}

size_t getArrayWrapper(void *bdm, const char *sessionID, char **dataIDs, int numDataIDs, void *outputBuffer)
{
    BlueDataManager *manager = static_cast<BlueDataManager *>(bdm);
    std::vector<std::string> ids(dataIDs, dataIDs + numDataIDs);

    return manager->getArray(sessionID, ids, static_cast<char *>(outputBuffer));
}
