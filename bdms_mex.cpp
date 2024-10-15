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

        initializeLogging();

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

        closeLogging();
        return;
    }

    // Get the class instance pointer from the second input
    BDMSDataManager *bdms_instance = convertMat2Ptr<BDMSDataManager>(prhs[1]);

    // Call the various class methods
    if (!strcmp("getArray", cmd))
    {
        // Check parameters
        if (nlhs != 1 || nrhs != 5)
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

        // Get the expected output size
        size_t expectedSize = (size_t)mxGetScalar(prhs[4]);

        // Create MATLAB uint8 column vector with the expected size
        mwSize dims[2] = {expectedSize, 1};
        plhs[0] = mxCreateNumericArray(2, dims, mxUINT8_CLASS, mxREAL);

        std::vector<std::string> ids(dataIDs, dataIDs + numDataIDs);
        // Get the data and copy directly into the mxArray
        size_t dataSize = bdms_instance->getArray(sessionID, ids, static_cast<char *>(mxGetData(plhs[0])));

        // Verify that the actual data size matches the expected size
        if (dataSize != expectedSize)
            mexErrMsgTxt("Actual data size does not match expected size");

        for (int i = 0; i < numDataIDs; i++)
        {
            mxFree(dataIDs[i]);
        }
        mxFree(dataIDs);
        return;
    }

    mexErrMsgTxt("Command not recognized.");
}