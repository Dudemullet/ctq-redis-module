#include "redismodule.h"

const char* CTQ_STORE_NAMESPACE = "ctq:store:";
const char* CTQ_TEMP_NAMESPACE = "ctq:temp:";

int onMsg(RedisModuleCtx *ctx, int type, const char *event, RedisModuleString *key) {
    RedisModule_Log(ctx, "warning", "Event: %s", event);
    RedisModule_Log(ctx, "warning", "Key: %s", RedisModule_StringPtrLen(key, NULL));
    return REDISMODULE_OK;
}

static int addKey(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    RedisModule_AutoMemory(ctx);
    // if (argc != 1)
    //     return RedisModule_WrongArity(ctx);

    char* newStorKey;
    char* newTempKey;
    const char* userKey = RedisModule_StringPtrLen(argv[1], NULL);
    size_t nameSpaceLength = strlen(CTQ_STORE_NAMESPACE);
    size_t userKeyLength = strlen(userKey);

    newStorKey = RedisModule_Alloc(nameSpaceLength + userKeyLength);
    newStorKey = strcpy(newStorKey, CTQ_STORE_NAMESPACE);
    newStorKey = strcat(newStorKey, userKey);

    newTempKey = RedisModule_Alloc(strlen(CTQ_TEMP_NAMESPACE) + userKeyLength);
    newTempKey = strcpy(newTempKey, CTQ_TEMP_NAMESPACE);
    newTempKey = strcat(newTempKey, userKey);

    RedisModule_Call(ctx, "SET", "cs", newStorKey, argv[2]);
    RedisModule_Call(ctx, "SET", "cccc", newTempKey, "", "EX", "5");

    RedisModule_Free(newStorKey);
    RedisModule_Free(newTempKey);

    RedisModule_ReplyWithNull(ctx);
    return REDISMODULE_OK;
}

int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {

    if (RedisModule_Init(ctx, "ctq", 2, 1) != REDISMODULE_OK) {
       printf("\n\r****** Init Fail!\n\r\n\r");
       return REDISMODULE_ERR;
    }

    RedisModule_SubscribeToKeyspaceEvents(ctx, REDISMODULE_NOTIFY_EXPIRED, &onMsg);

    int createStatus = RedisModule_CreateCommand(ctx, "ctq.add", addKey, "", 1, 1, 1);

    if (createStatus != REDISMODULE_OK)
    {
        printf("\n Loading module failed");
        return REDISMODULE_ERR;
    }

    return REDISMODULE_OK;
}