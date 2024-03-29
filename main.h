#include "redismodule.h"
#include <time.h>
#include <stdlib.h>

// App level constants
static const char* CTQ_STORE_NAMESPACE = "ctq:store:";
static const char* CTQ_TEMP_NAMESPACE = "ctq:temp:";
static const char* CTQ_STORE_HASH_VALUE = "value";
static const char* CTQ_STORE_HASH_LIST = "list";
static const char* CTQ_STORE_HASH_TIME_EXPECTED = "time_expected";
static const char* CTQ_STORE_HASH_TIME_OCURRED = "time_ocurred";

// main.c Methods
char * appendString(RedisModuleCtx* ctx, const char* base, const char* stringToAppend);