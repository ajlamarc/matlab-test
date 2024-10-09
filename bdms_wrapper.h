#ifndef BDMS_WRAPPER_H
#define BDMS_WRAPPER_H

#include <cstddef>

void *createBlueDataManager(const char *apiKey, const char *baseUrl, const char *userAgent);
void destroyBlueDataManager(void *bdm);
size_t getArrayWrapper(void *bdm, const char *sessionID, char **dataIDs, int numDataIDs, void *outputBuffer);

#endif // BDMS_WRAPPER_H