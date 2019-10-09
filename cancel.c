#include "main.h"
#include "cancel.h"

int cancelTimeout(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    RedisModule_AutoMemory(ctx);

    if (argc < 2) {
        return RedisModule_WrongArity(ctx);
    }

    // create ctq:temp:<key> and ctq:temp:<key> strings
    RedisModuleString* rms_keyToCancel = argv[1];
    const char* char_keyToCancel = RedisModule_StringPtrLen(rms_keyToCancel, NULL);
    char* char_storeKey = appendString(CTQ_STORE_NAMESPACE, char_keyToCancel);
    char* char_tempKey = appendString(CTQ_TEMP_NAMESPACE, char_keyToCancel);

    // Convert them to RedisModuleString for key opening
    RedisModuleString* rms_storeKey = RedisModule_CreateString(ctx, char_storeKey, strlen(char_storeKey));
    RedisModuleString* rms_tempKey = RedisModule_CreateString(ctx, char_tempKey, strlen(char_tempKey));
    RedisModuleKey* rmk_storeKey = RedisModule_OpenKey(ctx, rms_storeKey, REDISMODULE_WRITE);
    RedisModuleKey* rmk_tempKey = RedisModule_OpenKey(ctx, rms_tempKey, REDISMODULE_WRITE);

    //Delete the keys
    RedisModule_DeleteKey(rmk_storeKey);
    RedisModule_DeleteKey(rmk_tempKey);

    RedisModule_ReplyWithSimpleString(ctx, "OK");
    return REDISMODULE_OK;
}