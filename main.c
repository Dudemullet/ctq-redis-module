#include "main.h"
#include "cancel.h"

void getTime(RedisModuleCtx *ctx, char* char_timestamp, int offset) {
    time_t now;
    struct tm *tm;

    now = time(0) + offset;
    tm = localtime(&now);
    asprintf(&char_timestamp, "%04d-%02d-%02d %02d:%02d:%02d\n", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
}

void chopN(char *str, size_t n) {
    size_t len = strlen(str);
    if (n > len)
        return;  // Or: n = len;
    memmove(str, str+n, len - n + 1);
}

char * appendString(RedisModuleCtx* ctx, const char* base, const char* stringToAppend) {
    char * newTempKey;
    size_t userKeyLength = strlen(stringToAppend);

    newTempKey = RedisModule_PoolAlloc(ctx, strlen(base) + userKeyLength);
    newTempKey = strcpy(newTempKey, base);
    newTempKey = strcat(newTempKey, stringToAppend);

    return newTempKey;
}

int onKeyExpired(RedisModuleCtx *ctx, int type, const char *event, RedisModuleString *redisStringKey) {
    RedisModule_AutoMemory(ctx);
    const char* keyTemp = RedisModule_StringPtrLen(redisStringKey, NULL);
    const char* char_value;
    const char* char_listName;
    size_t valueLen;
    size_t listNameLen;

    // notify of expired keey
    RedisModule_Log(ctx, "warning", "Event: %s", event);
    RedisModule_Log(ctx, "warning", "Event key: %s", RedisModule_StringPtrLen(redisStringKey, NULL));

    char* trimmedKey = RedisModule_Strdup(keyTemp);
    chopN(trimmedKey, strlen(CTQ_TEMP_NAMESPACE));

    // Get the key value from the store
    char* c_storeKey = appendString(ctx, CTQ_STORE_NAMESPACE, trimmedKey);
    RedisModuleCallReply* rmr_value = RedisModule_Call(ctx, "HGET", "cc", c_storeKey, CTQ_STORE_HASH_VALUE);
    RedisModuleCallReply* rmr_listName = RedisModule_Call(ctx, "HGET", "cc", c_storeKey, CTQ_STORE_HASH_LIST);

    // Open store Key, cancel op if it doesn't exist
    RedisModuleString* rms_storeKey = RedisModule_CreateString(ctx, c_storeKey, strlen(c_storeKey));
    RedisModule_Log(ctx, "warning", "About to delete key: %s", RedisModule_StringPtrLen(rms_storeKey, NULL));
    RedisModuleKey* rmk_storeKey = RedisModule_OpenKey(ctx, rms_storeKey, REDISMODULE_WRITE);
    if( RedisModule_KeyType(rmk_storeKey) == REDISMODULE_KEYTYPE_EMPTY) {
        // key was not found, maybe this key isn't part of CTQ.
        RedisModule_Log(ctx, "warning", "No ctq key found for %s, cancelling ctq delete op", c_storeKey);
        return REDISMODULE_OK;
    }

    char_value = RedisModule_CallReplyStringPtr(rmr_value, &valueLen);
    char_listName = RedisModule_CallReplyStringPtr(rmr_listName, &listNameLen);

    // Add trimmedKeys value to a list
    RedisModuleString* rms_value = RedisModule_CreateString(ctx, char_value, valueLen);
    RedisModuleString* rms_listName = RedisModule_CreateString(ctx, char_listName, listNameLen);
    RedisModule_Log(ctx, "warning", "Pushing to list: %s", char_listName);
    RedisModule_Call(ctx, "LPUSH", "!ss", rms_listName, rms_value);

    // Delete ctq:store:<expired> key
    RedisModule_Call(ctx, "DEL", "!c", c_storeKey);
    return REDISMODULE_OK;
}

/* ctq.add key value list EX */
int addKey(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    RedisModule_AutoMemory(ctx);

    // Arity and type checking
    if (argc != 5) {
        return RedisModule_WrongArity(ctx);
    }
    if (RedisModule_StringToLongLong(argv[4], NULL) != REDISMODULE_OK) {
        return RedisModule_ReplyWithError(ctx, "ERR: EX must be a valid number");
    }

    RedisModuleString* rms_userKey = argv[1];
    RedisModuleString* rms_userValue = argv[2];
    RedisModuleString* rms_userList = argv[3];
    RedisModuleString* rms_userExValue = argv[4];
    char* char_timestamp = NULL;

    const char* userKey = RedisModule_StringPtrLen(rms_userKey, NULL);
    const char* char_userEX = RedisModule_StringPtrLen(rms_userExValue, NULL);

    char* char_newTempKey = appendString(ctx, CTQ_TEMP_NAMESPACE, userKey);
    RedisModule_Call(ctx, "SET", "!cccc", char_newTempKey, "", "EX", char_userEX);

    char* char_newStoreKey = appendString(ctx, CTQ_STORE_NAMESPACE, userKey);

    time_t time_currentTime = time(NULL) + atoi(char_userEX);
    asprintf(&char_timestamp, "%d", (int)time_currentTime);

    RedisModule_Call(ctx, "HMSET", "!ccscscc", char_newStoreKey,
        CTQ_STORE_HASH_VALUE, rms_userValue,
        CTQ_STORE_HASH_LIST, rms_userList,
        CTQ_STORE_HASH_TIME_EXPECTED, char_timestamp);

    RedisModule_ReplyWithSimpleString(ctx, "OK");
    return REDISMODULE_OK;
}

int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {

    if (RedisModule_Init(ctx, "ctq", 2, 1) != REDISMODULE_OK) {
       printf("\n\r****** Init Fail!\n\r\n\r");
       return REDISMODULE_ERR;
    }

    RedisModule_SubscribeToKeyspaceEvents(ctx, REDISMODULE_NOTIFY_EXPIRED, &onKeyExpired);

    // register all commands
    int int_createAddCommand = RedisModule_CreateCommand(ctx, "ctq.add", addKey, "", 1, 1, 1);
    int int_createCancelCommand = RedisModule_CreateCommand(ctx, "ctq.cancel", cancelTimeout, "", 1, 1, 1);

    if (int_createAddCommand != REDISMODULE_OK || int_createCancelCommand != REDISMODULE_OK)
    {
        printf("\n Loading module failed");
        return REDISMODULE_ERR;
    }

    return REDISMODULE_OK;
}