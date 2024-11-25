// see https://www.mathworks.com/matlabcentral/answers/2147499-why-does-matlab-r2024a-crash-when-running-a-mex-file-compiled-with-the-microsoft-visual-studio-2022
#define _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR

#include "mex.h"
#include "matrix.h"
#include "class_handle.hpp"
#include "bdms_data.hpp"
#include <string.h>

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    char cmd[64];

    if (nrhs < 1 || mxGetString(prhs[0], cmd, sizeof(cmd)))
        mexErrMsgTxt("First input should be a command string less than 64 characters long");

    if (!strcmp("new", cmd))
    {
        // Check parameters
        if (nlhs != 1)
            mexErrMsgTxt("New: One output expected.");

        if (nrhs != 6)
            mexErrMsgTxt("New: Requires 5 additional arguments.");

        // initializeLogging();

        char apiKey[256], host[256], protocol[256], certificatePath[256], userAgent[256];
        mxGetString(prhs[1], apiKey, sizeof(apiKey));
        mxGetString(prhs[2], host, sizeof(host));
        mxGetString(prhs[3], protocol, sizeof(protocol));
        mxGetString(prhs[4], certificatePath, sizeof(certificatePath));
        mxGetString(prhs[5], userAgent, sizeof(userAgent));

        BDMSProvidedConfig provided;
        provided.apiKey = std::string(apiKey);
        provided.host = std::string(host);
        provided.protocol = std::string(protocol);
        provided.certificatePath = std::string(certificatePath);
        provided.userAgent = std::string(userAgent);

        // Return a handle to a new C++ instance
        plhs[0] = convertPtr2Mat<BDMSDataManager>(new BDMSDataManager(provided));
        return;
    }

    // Check there is a second input, which should be the class instance handle
    if (nrhs < 2)
        mexErrMsgTxt("Second input should be a class instance handle.");

    // Delete
    if (!strcmp("delete", cmd))
    {
        // Destroy the C++ object
        destroyObject<BDMSDataManager>(prhs[1]);
        // Warn if other commands were ignored
        if (nlhs != 0 || nrhs != 2)
            mexWarnMsgTxt("Delete: Unexpected arguments ignored.");

        // closeLogging();
        return;
    }

    // Get the class instance pointer from the second input
    BDMSDataManager *bdms_instance = convertMat2Ptr<BDMSDataManager>(prhs[1]);

    // Call the various class methods
    if (!strcmp("getArray", cmd))
    {
        // Check parameters
        if (nlhs != 1 || nrhs != 4)
            mexErrMsgTxt("getArray: Unexpected arguments.");

        char sessionID[256];
        mxGetString(prhs[2], sessionID, sizeof(sessionID));

        const mxArray *cellArray = prhs[3];
        size_t numDataIDs = mxGetNumberOfElements(cellArray);
        char **dataIDs = (char **)mxCalloc(numDataIDs, sizeof(char *));

        for (size_t i = 0; i < numDataIDs; i++)
        {
            mxArray *cellElement = mxGetCell(cellArray, i);
            char *str = mxArrayToString(cellElement);
            dataIDs[i] = str;
        }
        std::vector<std::string> ids(dataIDs, dataIDs + numDataIDs);
        for (int i = 0; i < numDataIDs; i++)
        {
            mxFree(dataIDs[i]);
        }
        mxFree(dataIDs);

        plhs[0] = bdms_instance->getArray(sessionID, ids);
        return;
    }

    if (!strcmp("getArraysBySessionId", cmd))
    {
        if (nlhs != 1 || nrhs != 3)
            mexErrMsgTxt("getArraysBySessionId: Unexpected arguments.");

        std::vector<std::vector<std::string>> dataToDownload;

        const mxArray *sessionIDsAndDataIDs = prhs[2];
        size_t numSessions = mxGetNumberOfElements(sessionIDsAndDataIDs);
        dataToDownload.reserve(numSessions);

        for (size_t i = 0; i < numSessions; i++)
        {
            mxArray *sessionIDAndDataIDs = mxGetCell(sessionIDsAndDataIDs, i);
            size_t numDataIDsIncludingSessionID = mxGetNumberOfElements(sessionIDAndDataIDs);
            size_t numDataIDs = numDataIDsIncludingSessionID - 1;

            if (numDataIDs < 1)
                mexErrMsgTxt("getArraysBySessionId: Each session must have at least one data ID.");

            // Convert cell array to vector structure
            std::vector<std::string> sessionDataToDownload;
            sessionDataToDownload.reserve(numDataIDsIncludingSessionID);

            // get session ID
            mxArray *sessionID = mxGetCell(sessionIDAndDataIDs, 0);
            char *sessionIDChars = mxArrayToString(sessionID);
            std::string sessionIDStr = std::string(sessionIDChars);
            mxFree(sessionIDChars);
            sessionDataToDownload.emplace_back(sessionIDStr);

            // get data IDs
            for (size_t j = 0; j < numDataIDs; j++)
            {
                mxArray *dataID = mxGetCell(sessionIDAndDataIDs, j + 1);
                char *dataIDChars = mxArrayToString(dataID);
                std::string dataIDStr = std::string(dataIDChars);
                mxFree(dataIDChars);
                sessionDataToDownload.emplace_back(dataIDStr);
            }
            dataToDownload.emplace_back(sessionDataToDownload);
        }

        plhs[0] = bdms_instance->getArraysBySessionId(dataToDownload);
        return;
    }

    mexErrMsgTxt("Command not recognized.");
}