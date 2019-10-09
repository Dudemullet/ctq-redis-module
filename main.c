#include "redismodule.h"
#include "main.h"
#include "cancel.h"

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
    const char* char_value;
    const char* char_listName;
    size_t valueLen;
    size_t listNameLen;

    // notify of expired keey
    RedisModule_Log(ctx, "warning", "Event: %s", event);
    RedisModule_Log(ctx, "warning", "Event key: %s", RedisModule_StringPtrLen(redisStringKey, NULL));

    // Trim the temp string from the key
    char *trimmedKey = strdup(keyTemp);
    chopN(trimmedKey, strlen(CTQ_TEMP_NAMESPACE));
    RedisModule_Log(ctx, "warning", "Trimmed key: %s", trimmedKey);

    // Get the key value from the store
    char* c_storeKey = appendString(CTQ_STORE_NAMESPACE, trimmedKey);
    RedisModuleCallReply* rmr_value = RedisModule_Call(ctx, "HGET", "cc", c_storeKey, CTQ_STORE_HASH_VALUE);
    RedisModuleCallReply* rmr_listName = RedisModule_Call(ctx, "HGET", "cc", c_storeKey, CTQ_STORE_HASH_LIST);

    char_value = RedisModule_CallReplyStringPtr(rmr_value, &valueLen);
    char_listName = RedisModule_CallReplyStringPtr(rmr_listName, &listNameLen);

    // Add trimmedKeys value to a list
    RedisModuleString* tempVal = RedisModule_CreateString(ctx, char_value, valueLen);
    RedisModuleString* listName = RedisModule_CreateString(ctx, char_listName, listNameLen);
    RedisModuleKey* listKey = RedisModule_OpenKey(ctx, listName, REDISMODULE_WRITE);
    RedisModule_ListPush(listKey, REDISMODULE_LIST_TAIL, tempVal);

    // Delete ctq:store:<expired> key
    RedisModuleString* rms_storeKey = RedisModule_CreateString(ctx, c_storeKey, strlen(c_storeKey));
    RedisModuleKey* rmk_storeKey = RedisModule_OpenKey(ctx, rms_storeKey, REDISMODULE_WRITE);
    RedisModule_DeleteKey(rmk_storeKey);

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

    char* newStoreKey;
    char* newTempKey;
    RedisModuleString* rms_userKey = argv[1];
    RedisModuleString* rms_userValue = argv[2];
    RedisModuleString* rms_userList = argv[3];
    RedisModuleString* rms_userExValue = argv[4];

    const char* userKey = RedisModule_StringPtrLen(rms_userKey, NULL);
    newStoreKey = appendString(CTQ_STORE_NAMESPACE, userKey);
    newTempKey = appendString(CTQ_TEMP_NAMESPACE, userKey);

    RedisModule_Call(ctx, "HMSET", "ccscs", newStoreKey, CTQ_STORE_HASH_VALUE, rms_userValue, CTQ_STORE_HASH_LIST, rms_userList);
    RedisModule_Call(ctx, "SET", "cccs", newTempKey, "", "EX", rms_userExValue);

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