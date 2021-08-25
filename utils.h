#ifndef _EX_REDIS_UTILS_H
#define _EX_REDIS_UTILS_H

#include "redismodule.h"
#include <stdlib.h>

#define ASSERT(expr)                                        \
    do {                                                    \
        if(!(expr))                                         \
        {                                                   \
            fprintf(                                        \
                stderr,                                     \
                "Assertion failed in %s on line %d: %s\n",  \
                __FILE__,                                   \
                __LINE__,                                   \
                # expr                                      \
            );                                              \
            abort();                                        \
        }                                                   \
    } while(0)


#define DOUBLE_CHAR_LENGTH  1024

int redis_string_to_long_double(RedisModuleString *s, long double *dptr);
RedisModuleString *long_double_to_redis_string(RedisModuleCtx *ctx, long double d);

#endif
