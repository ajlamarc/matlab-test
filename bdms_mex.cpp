#include "mex.h"
#include "matrix.h"
#include "bdms_wrapper.hpp"
#include <string.h>

static void *bdm = NULL;

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    char cmd[64];

    if (nrhs < 1 || mxGetString(prhs[0], cmd, sizeof(cmd)))
        mexErrMsgTxt("First input should be a command string");

    if (strcmp(cmd, "init") == 0)
    {
        if (nrhs != 6)
            mexErrMsgTxt("init requires 5 additional arguments");

        // host, apiKey, protocol, certificatePath, userAgent

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

        bdm = createBDMSDataManager(provided);
    }
    else if (strcmp(cmd, "cleanup") == 0)
    {
        if (bdm != NULL)
        {
            destroyBDMSDataManager(bdm);
            bdm = NULL;
        }
    }
    else if (strcmp(cmd, "getArray") == 0)
    {
        if (nrhs != 4)
            mexErrMsgTxt("getArray requires 3 additional arguments");

        if (bdm == NULL)
            mexErrMsgTxt("BDMSDataManager not initialized");

        char sessionID[256];
        mxGetString(prhs[1], sessionID, sizeof(sessionID));

        const mxArray *cellArray = prhs[2];
        size_t numDataIDs = mxGetNumberOfElements(cellArray);
        char **dataIDs = (char **)mxCalloc(numDataIDs, sizeof(char *));

        for (size_t i = 0; i < numDataIDs; i++)
        {
            mxArray *cellElement = mxGetCell(cellArray, i);
            char *str = mxArrayToString(cellElement);
            dataIDs[i] = str;
        }

        // Get the expected output size
        size_t expectedSize = (size_t)mxGetScalar(prhs[3]);

        // Create MATLAB uint8 column vector with the expected size
        mwSize dims[2] = {expectedSize, 1};
        plhs[0] = mxCreateNumericArray(2, dims, mxUINT8_CLASS, mxREAL);

        // Get the data and copy directly into the mxArray
        size_t dataSize = getArrayWrapper(bdm, sessionID, dataIDs, numDataIDs, mxGetData(plhs[0]));

        // Verify that the actual data size matches the expected size
        if (dataSize != expectedSize)
        {
            mexErrMsgTxt("Actual data size does not match expected size");
        }

        for (int i = 0; i < numDataIDs; i++)
        {
            mxFree(dataIDs[i]);
        }
        mxFree(dataIDs);
    }
    else
    {
        mexErrMsgTxt("Unknown command");
    }
}