#include <cstddef>
#include "bdms_data.hpp"
#include "mex.h"
#include <cstring>
#include <vector>
#include <sstream>

void *createBDMSDataManager(BDMSProvidedConfig provided)
{
    initializeLogging();
    return new BDMSDataManager(provided);
}

void destroyBDMSDataManager(void *bdm)
{
    logMessage("Debug: Destroying BDMSDataManager");
    delete static_cast<BDMSDataManager *>(bdm);
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

    BDMSDataManager *manager = static_cast<BDMSDataManager *>(bdm);
    std::vector<std::string> ids(dataIDs, dataIDs + numDataIDs);

    return manager->getArray(sessionID, ids, static_cast<char *>(outputBuffer));
}
