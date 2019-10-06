#include "redismodule.h"

const char* CTQ_STORE_NAMESPACE = "ctq:store:";
const char* CTQ_TEMP_NAMESPACE = "ctq:temp:";

void chopN(char *str, size_t n) {
    size_t len = strlen(str);
    if (n > len)
        return;  // Or: n = len;
    memmove(str, str+n, len - n + 1);
}

char * appendString(const char* base, const char* stringToAppend) {
    char * newTempKey;
    size_t userKeyLength = strlen(stringToAppend);

    newTempKey = RedisModule_Alloc(strlen(base) + userKeyLength);
    newTempKey = strcpy(newTempKey, base);
    newTempKey = strcat(newTempKey, stringToAppend);

    return newTempKey;
}

int onKeyExpired(RedisModuleCtx *ctx, int type, const char *event, RedisModuleString *redisStringKey) {
    RedisModule_AutoMemory(ctx);
    const char* keyTemp = RedisModule_StringPtrLen(redisStringKey, NULL);
    const char* keyValue;
    size_t keyValueLen;

    // notify of expired keey
    RedisModule_Log(ctx, "warning", "Event: %s", event);
    RedisModule_Log(ctx, "warning", "Event key: %s", RedisModule_StringPtrLen(redisStringKey, NULL));

    // Trim the temp string from the key
    char *trimmedKey = strdup(keyTemp);
    chopN(trimmedKey, strlen(CTQ_TEMP_NAMESPACE));
    RedisModule_Log(ctx, "warning", "Trimmed key: %s", trimmedKey);

    // Get the key value from the store
    char* storeKey = appendString(CTQ_STORE_NAMESPACE, trimmedKey);
    RedisModuleCallReply* reply = RedisModule_Call(ctx, "GET", "c", storeKey);
    int replyType = RedisModule_CallReplyType(reply);
    switch (replyType)
    {
        case REDISMODULE_REPLY_ERROR:
            RedisModule_Log(ctx, "warning", "Trying to get key produced error %s", storeKey);
            return REDISMODULE_ERR;
            break;
        case REDISMODULE_REPLY_STRING: 
            keyValue = RedisModule_CallReplyStringPtr(reply, &keyValueLen);
            RedisModule_Log(ctx, "warning", "Found key value: %s", keyValue);
            break;
        default:
            return REDISMODULE_ERR;
            break;
    }

    // Add trimmedKeys value to a list
    RedisModuleString* tempVal = RedisModule_CreateString(ctx, keyValue, keyValueLen);
    RedisModuleString* listName = RedisModule_CreateString(ctx, "TEMP", strlen("TEMP"));
    RedisModuleKey* listKey = RedisModule_OpenKey(ctx, listName, REDISMODULE_WRITE);
    RedisModule_ListPush(listKey, REDISMODULE_LIST_TAIL, tempVal);

    return REDISMODULE_OK;
}

int addKey(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    RedisModule_AutoMemory(ctx);

    char* newStoreKey;
    char* newTempKey;
    const char* userKey = RedisModule_StringPtrLen(argv[1], NULL);
    newStoreKey = appendString(CTQ_STORE_NAMESPACE, userKey);
    newTempKey = appendString(CTQ_TEMP_NAMESPACE, userKey);

    RedisModule_Call(ctx, "SET", "cs", newStoreKey, argv[2]);
    RedisModule_Call(ctx, "SET", "cccs", newTempKey, "", "EX", argv[3]);

    RedisModule_Free(newStoreKey);
    RedisModule_Free(newTempKey);

    RedisModule_ReplyWithNull(ctx);
    return REDISMODULE_OK;
}

int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {

    if (RedisModule_Init(ctx, "ctq", 2, 1) != REDISMODULE_OK) {
       printf("\n\r****** Init Fail!\n\r\n\r");
       return REDISMODULE_ERR;
    }

    RedisModule_SubscribeToKeyspaceEvents(ctx, REDISMODULE_NOTIFY_EXPIRED, &onKeyExpired);

    int createStatus = RedisModule_CreateCommand(ctx, "ctq.add", addKey, "", 1, 1, 1);

    if (createStatus != REDISMODULE_OK)
    {
        printf("\n Loading module failed");
        return REDISMODULE_ERR;
    }

    return REDISMODULE_OK;
}