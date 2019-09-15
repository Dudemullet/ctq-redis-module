#include "redismodule.h"

int onMsg(RedisModuleCtx *ctx, int type, const char *event, RedisModuleString *key) {
    printf("WTF in listener");
    RedisModule_Log(ctx, "warning", "Hello");
    return 1;
}

static int ctq_Register_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    RedisModule_AutoMemory(ctx);
    RedisModule_ReplicateVerbatim(ctx);

    RedisModule_SubscribeToKeyspaceEvents(ctx, REDISMODULE_NOTIFY_ALL, &onMsg);

    return REDISMODULE_OK;
}

int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {

    if (RedisModule_Init(ctx, "ctq", 1, 1) != REDISMODULE_OK) {
       printf("\n\r****** Init Fail!\n\r\n\r");
       return REDISMODULE_ERR;
    }

    if (RedisModule_CreateCommand(ctx, "ctq.CREATE", ctq_Register_RedisCommand, "", 1, 1, 1) != REDISMODULE_OK)
    {
        printf("\n Loading module failed");
        return REDISMODULE_ERR;
    }

    return REDISMODULE_OK;
}