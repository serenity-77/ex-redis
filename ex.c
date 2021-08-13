#include <stdio.h>
#include "redismodule.h"

int hincrByFloatEx_Command(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    RedisModuleKey *key;
    int keyType;
    RedisModuleString *curr, *resultString;
    long double value, op, result;

    RedisModule_AutoMemory(ctx);

    if(argc != 4)
        return RedisModule_WrongArity(ctx);

    key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);

    keyType = RedisModule_KeyType(key);

    if(keyType != REDISMODULE_KEYTYPE_HASH && keyType != REDISMODULE_KEYTYPE_EMPTY)
        return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);

    // TODO
    // handle non-existence key

    if(RedisModule_HashGet(key, REDISMODULE_HASH_NONE, argv[2], &curr, NULL) == REDISMODULE_ERR)
        return RedisModule_ReplyWithError(ctx, "Failed to get hash field");

    if(!curr)
    {
        RedisModule_HashSet(key, REDISMODULE_HASH_NONE, argv[2], argv[3], NULL);
        resultString = argv[3];
    }
    else
    {
        if(RedisModule_StringToLongDouble(curr, &value) == REDISMODULE_ERR)
            return RedisModule_ReplyWithError(ctx, "Invalid floating value");

        if(RedisModule_StringToLongDouble(argv[3], &op) == REDISMODULE_ERR)
            return RedisModule_ReplyWithError(ctx, "Invalid floating value");

        result = value + (op);

        char doubleString[1024];
        int r;

        r = snprintf(doubleString, 1024, "%.10Lg", result);

        resultString = RedisModule_CreateString(ctx, doubleString, r);

        RedisModule_HashSet(key, REDISMODULE_HASH_NONE, argv[2], resultString, NULL);
    }

    RedisModule_ReplyWithString(ctx, resultString);

    RedisModule_ReplicateVerbatim(ctx);
    return REDISMODULE_OK;
}

int RedisModule_OnLoad(RedisModuleCtx *ctx)
{
    if (RedisModule_Init(ctx, "ex_module", 1, REDISMODULE_APIVER_1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx, "ex.hincrbyfloat", hincrByFloatEx_Command, "write deny-oom", 1, 1, 1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    return REDISMODULE_OK;
}
