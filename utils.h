#ifndef _EX_REDIS_UTILS_H
#define _EX_REDIS_UTILS_H

#include "redismodule.h"

#define DOUBLE_CHAR_LENGTH  1024

int redis_string_to_long_double(RedisModuleString *s, long double *dptr);
RedisModuleString *long_double_to_redis_string(RedisModuleCtx *ctx, long double d);

#endif
