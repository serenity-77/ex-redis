#include <stdio.h>
#include <string.h>
#include <openssl/crypto.h>
#include "redismodule.h"
#include "bnfp.h"


static RedisModuleString *hincrbyfloat_ex(RedisModuleCtx *ctx, RedisModuleString *a, RedisModuleString *b);


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
        if((resultString = hincrbyfloat_ex(ctx, curr, argv[3])) == NULL)
            return REDISMODULE_OK;

        if(!RedisModule_HashSet(key, REDISMODULE_HASH_NONE, argv[2], resultString, NULL))
            return RedisModule_ReplyWithError(ctx, "Failed to update hash value after bnfp operation");
    }

    RedisModule_ReplyWithString(ctx, resultString);

    RedisModule_ReplicateVerbatim(ctx);
    return REDISMODULE_OK;
}


static RedisModuleString *hincrbyfloat_ex(RedisModuleCtx *ctx, RedisModuleString *a, RedisModuleString *b)
{
    bnfp_t *value = NULL, *op = NULL, *res = NULL;
    char *va, *vb, *err;
    size_t len;
    int r;
    RedisModuleString *resultString;

    if((value = bnfp_create()) == NULL)
    {
        RedisModule_ReplyWithError(ctx, "Failed to create bnfp value");
        return NULL;
    }

    if((op = bnfp_create()) == NULL)
    {
        err = "Failed to create bnfp op";
        goto _err;
    }

    va = (char *) RedisModule_StringPtrLen(a, &len);
    if((r = bnfp_from_string(value, va, len)) < 0)
    {
        err = (char *) bnfp_get_error_string(r);
        goto _err;
    }

    vb = (char *) RedisModule_StringPtrLen(b, &len);
    if((r = bnfp_from_string(op, vb, len)) < 0)
    {
        err = (char *) bnfp_get_error_string(r);
        goto _err;
    }

    if((res = bnfp_create()) == NULL)
    {
        err = "Failed to create bnfp result";
        goto _err;
    }

    if((r = bnfp_add(res, value, op)) < 0)
    {
        err = (char *) bnfp_get_error_string(r);
        goto _err;
    }

    char *rs;
    if((rs = bnfp_to_string(res)) == NULL)
    {
        err = "Failed to create bnfp string from result";
        goto _err;
    }

    resultString = RedisModule_CreateString(ctx, rs, strlen(rs));

    bnfp_free_string(rs);
    bnfp_destroy(value);
    bnfp_destroy(op);
    bnfp_destroy(res);

    return resultString;

_err:
    if(value != NULL)
        bnfp_destroy(value);
    if(op != NULL)
        bnfp_destroy(op);
    if(res != NULL)
        bnfp_destroy(res);
    RedisModule_ReplyWithError(ctx, err);
    return NULL;
}


int RedisModule_OnLoad(RedisModuleCtx *ctx)
{
    if (RedisModule_Init(ctx, "ex_module", 1, REDISMODULE_APIVER_1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx, "ex.hincrbyfloat", hincrByFloatEx_Command, "write deny-oom", 1, 1, 1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    return REDISMODULE_OK;
}
