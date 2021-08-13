#include <stdio.h>
#include <stdlib.h>
#include "utils.h"


int redis_string_to_long_double(RedisModuleString *s, long double *dptr)
{
    long double d;
    char *buf, *sptr;

    buf = RedisModule_StringPtrLen(s, NULL);

    d = strtold(buf, &sptr);

    if(*sptr != '\0')
        return -1;

    *dptr = d;

    return 0;
}


RedisModuleString *long_double_to_redis_string(RedisModuleCtx *ctx, long double d)
{
    int r;
    char dstring[DOUBLE_CHAR_LENGTH];

    r = snprintf(dstring, DOUBLE_CHAR_LENGTH, "%.10Lg", d);

    return RedisModule_CreateString(ctx, dstring, r);
}
