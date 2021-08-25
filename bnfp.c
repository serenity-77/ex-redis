#include <ctype.h>
#include <string.h>
#include <assert.h>
#include "bnfp.h"
#include "utils.h"

/*A SIMPLE FLOATING POINT NUMBER USING OpenSSL BIGNUM*/

struct bnfp_s {
    BIGNUM *value;
    unsigned int scale;
};


bnfp_t *bnfp_create()
{
    bnfp_t *b;
    if((b = OPENSSL_malloc(sizeof(*b))) == NULL)
        return NULL;
    b->value = NULL;
    b->scale = 0;
    return b;
}


int bnfp_from_string(bnfp_t *fn, const char *s, size_t len)
{
    char buf[len + 1], *d, *f, c;
    size_t sep_found = 0, buf_len = 0;
    unsigned int scale = 0;
    BIGNUM *value = NULL;

    memset(buf, 0, len + 1);

    d = (char *) s;
    f = buf;

    while((c = *d))
    {
        if(isdigit(c) || (c == '-' && d - s == 0))
        {
            if(sep_found) scale++;

            if(c != '0' || buf_len) // skip awalan 0
            {
                if(c == '-') f++; // skip satu tempat untuk tanda -
                else
                {
                    *f++ = c;
                    buf_len++;
                }
            }
        }
        else if(
            c == '.' && !sep_found &&
            (d - s > 0 && isdigit(*(d - 1)))
        )
        {
            sep_found = 1;
        }
        else
            return -2;
        d++;
    }

    if(*s == '-' && buf_len)
        buf[0] = '-';

    if(!buf_len)
    {
        if(*s == '-') f--;
        *f++ = '0';
        scale = 0;
    }

    else if(*(f - 1) == '0' && sep_found) // hapus akhiran 0
    {
        while(*--f == '0')
            scale--;
        f++;
    }

    buf[f - buf] = '\0';

    if(!BN_dec2bn(&value, buf))
        return -1;

    fn->value = value;
    fn->scale = scale;

    return 0;
}


char *bnfp_to_string(const bnfp_t *fn)
{
    size_t len, tmp_len, alloc_size;
    char *s, *r;

    if((s = BN_bn2dec(fn->value)) == NULL)
        return NULL;

    if(!fn->scale)
        return s;

    len = strlen(s);

    alloc_size = len + 2; // +2 untuk . dan '\0';

    tmp_len = len;
    if(*s == '-')
        tmp_len--;

    if(fn->scale == tmp_len)
        alloc_size++;
    else if(fn->scale > tmp_len)
        alloc_size += fn->scale;

    if((r = OPENSSL_malloc(alloc_size)) == NULL)
        goto err;

    char *d = s, *f = r, c;

    if(fn->scale >= tmp_len)
    {
        if(*s == '-')
        {
            *f++ = '-';
            d++;
        }

        *f++ = '0';
        *f++ = '.';

        int zi = fn->scale - tmp_len;
        while(zi--)
            *f++ = '0';
        while((c = *d))
        {
            *f++ = c;
            d++;
        }
    }
    else
    {
        int sep_idx = len - fn->scale;
        while((c = *d))
        {
            if(d - s == sep_idx)
                *f++ = '.';
            *f++ = c;
            d++;
        }
    }

    *f = '\0';

    OPENSSL_free(s);

    return r;

err:
    bnfp_free_string(s);
    return NULL;
}


void bnfp_destroy(const bnfp_t *fn)
{
    if(fn->value != NULL)
        BN_free(fn->value);
    OPENSSL_free((void *) fn);
}


void bnfp_free_string(const void *s)
{
    OPENSSL_free((void *) s);
}


const char *bnfp_get_error_string(int err)
{
    switch(err)
    {
        case -1:
            return "Failed to initialize bignum";
        case -2:
            return "Invalid floating point number";
        case -3:
            return "Failed to perform addition operation";
        default:
            return "BNFP Unknown Error";
    }
}


#define MAX_(a, b) (a) >= (b) ? (a) : (b)
#define MIN_(a, b) (a) <= (b) ? (a) : (b)


int bnfp_add(bnfp_t *r, bnfp_t *a, bnfp_t *b)
{
    BIGNUM *res = NULL;

    if((res = BN_new()) == NULL)
        return -3;

    if(!a->scale && !b->scale)
    {
        if(!BN_add(res, a->value, b->value))
        {
            BN_free(res);
            return -3;
        }
        r->value = res;
        r->scale = 0;
        return 0;
    }

    BN_CTX *ctx = NULL;

    if((ctx = BN_CTX_new()) == NULL)
        return -3;

    if(a->scale != b->scale)
    {
        BIGNUM *tmp_mul = NULL, *scale_mul = NULL;
        unsigned int scale_mul_len, scale_diff;

        scale_diff = (MAX_(a->scale, b->scale)) - (MIN_(a->scale, b->scale));

        scale_mul_len = scale_diff + 2; // +2 untuk 1 dan '\0';

        char s_scale_mul[scale_mul_len];
        s_scale_mul[0] = '1';
        memset(s_scale_mul + 1, '0', scale_mul_len - 2);
        s_scale_mul[scale_mul_len - 1] = '\0';

        if((tmp_mul = BN_new()) == NULL)
            goto err;

        if(!BN_dec2bn(&scale_mul, s_scale_mul))
        {
            BN_free(tmp_mul);
            goto err;
        }

        BIGNUM *h, *op;

        if(a->scale < b->scale)
        {
            h = a->value;
            op = b->value;
        }

        else
        {
            h = b->value;
            op = a->value;
        }

        if(!BN_mul(tmp_mul, h, scale_mul, ctx))
        {
            BN_free(tmp_mul);
            BN_free(scale_mul);
            goto err;
        }

        if(!BN_add(res, tmp_mul, op))
        {
            BN_free(tmp_mul);
            BN_free(scale_mul);
            goto err;
        }

        BN_free(tmp_mul);
        BN_free(scale_mul);

        r->value = res;
        r->scale = MAX_(a->scale, b->scale);
    }
    else
    {
        BIGNUM *ten = NULL,
               *tmp_res = NULL,
               *div_rem = NULL;

        unsigned int scale = a->scale;
        char *s_ten = "10";

        if(!BN_add(res, a->value, b->value))
            goto err;

        if(!BN_dec2bn(&ten, s_ten))
            goto err;

        if((tmp_res = BN_new()) == NULL)
        {
            BN_free(ten);
            goto err;
        }

        if((div_rem = BN_new()) == NULL)
        {
            BN_free(ten);
            BN_free(tmp_res);
            goto err;
        }

        while(1)
        {
            if(!BN_div(tmp_res, div_rem, res, ten, ctx))
            {
                BN_free(ten);
                BN_free(tmp_res);
                BN_free(div_rem);
                goto err;
            }

            if(!BN_is_zero(div_rem) || !scale)
                break;

            scale--;

            if((res = BN_copy(res, tmp_res)) == NULL)
            {
                BN_free(ten);
                BN_free(tmp_res);
                BN_free(div_rem);
                goto err;
            }
        }

        BN_free(ten);
        BN_free(tmp_res);
        BN_free(div_rem);

        r->value = res;
        r->scale = scale;
    }

    BN_CTX_free(ctx);

    return 0;

err:
    if(ctx != NULL)
        BN_CTX_free(ctx);
    if(res != NULL)
        BN_free(res);
    return -3;
}


#ifdef TEST_MAIN

#include <stdio.h>
#include <string.h>

void test_bnfp_1();
void test_get_error_string();
void test_invalid_floating_point_number();
void test_bnfp_add();


int main(int argc, char *argv[])
{
    test_bnfp_1();
    test_get_error_string();
    test_invalid_floating_point_number();
    test_bnfp_add();
    return 0;
}

void test_bnfp_1()
{
    int i, r;
    bnfp_t *fn;
    char *to_string;

    struct arg {
        char *value;
        unsigned int scale;
        char *to_string;
    };

    const struct arg args[] = {
        {"124312", 0, "124312"},
        {"17", 0, "17"},
        {"7", 0, "7"},
        {"3000", 0, "3000"},
        {"10", 0, "10"},
        {"-124312", 0, "-124312"},
        {"-17", 0, "-17"},
        {"-7", 0, "-7"},
        {"-3000", 0, "-3000"},
        {"-10", 0, "-10"},
        {"12.173", 3, "12.173"},
        {"23424234242.25345353453", 11, "23424234242.25345353453"},
        {"234.50000", 1, "234.5"},
        {"3000.500", 1, "3000.5"},
        {"1.2", 1, "1.2"},
        {"12.0003245", 7, "12.0003245"},
        {"12.004003245", 9, "12.004003245"},
        {"-12.173", 3, "-12.173"},
        {"-23424234242.25345353453", 11, "-23424234242.25345353453"},
        {"-234.50000", 1, "-234.5"},
        {"-3000.500", 1, "-3000.5"},
        {"-1.2", 1, "-1.2"},
        {"-12.0003245", 7, "-12.0003245"},
        {"-12.004003245", 9, "-12.004003245"},
        {"0.78", 2, "0.78"},
        {"-0.78", 2, "-0.78"},
        {"000.3245", 4, "0.3245"},
        {"003000.3245", 4, "3000.3245"},
        {"-000.3245", 4, "-0.3245"},
        {"-003000.3245", 4, "-3000.3245"},
        {"10002.004003245", 9, "10002.004003245"},
        {"-10002.004003245", 9, "-10002.004003245"},
        {"-0001.1", 1, "-1.1"},
        {"-003000.3245", 4, "-3000.3245"},
        {"0.2", 1, "0.2"},
        {"0.02", 2, "0.02"},
        {"0.002", 3, "0.002"},
        {"0.0002", 4, "0.0002"},
        {"00.0002", 4, "0.0002"},
        {"00.000212", 6, "0.000212"},
        {"0.000212", 6, "0.000212"},
        {"-0.2", 1, "-0.2"},
        {"-0.02", 2, "-0.02"},
        {"-0.002", 3, "-0.002"},
        {"-0.0002", 4, "-0.0002"},
        {"-00.0002", 4, "-0.0002"},
        {"-00.000212", 6, "-0.000212"},
        {"-0.000212", 6, "-0.000212"},
        {"0.20", 1, "0.2"},
        {"0.200", 1, "0.2"},
        {"0.020", 2, "0.02"},
        {"0.0200", 2, "0.02"},
        {"0.00200", 3, "0.002"},
        {"0.0002000", 4, "0.0002"},
        {"00.00020000", 4, "0.0002"},
        {"00.000212000", 6, "0.000212"},
        {"0.0002120", 6, "0.000212"},
        {"0124312", 0, "124312"},
        {"00124312", 0, "124312"},
        {"000124312", 0, "124312"},
        {"0017", 0, "17"},
        {"00007", 0, "7"},
        {"000003000", 0, "3000"},
        {"00000010", 0, "10"},
        {"-0124312", 0, "-124312"},
        {"-00124312", 0, "-124312"},
        {"-000124312", 0, "-124312"},
        {"-0017", 0, "-17"},
        {"-00007", 0, "-7"},
        {"-000003000", 0, "-3000"},
        {"-00000010", 0, "-10"},
        {"0", 0, "0"},
        {"00", 0, "0"},
        {"000", 0, "0"},
        {"0000", 0, "0"},
        {"-0", 0, "0"},
        {"-00", 0, "0"},
        {"-000", 0, "0"},
        {"-0000", 0, "0"},
        {"0.0", 0, "0"},
        {"0.00", 0, "0"},
        {"00.0", 0, "0"},
        {"0.000", 0, "0"},
        {"000.0", 0, "0"},
        {"000.00", 0, "0"},
        {"-0.0", 0, "0"},
        {"-0.00", 0, "0"},
        {"-00.0", 0, "0"},
        {"-0.000", 0, "0"},
        {"-000.0", 0, "0"},
        {"-000.00", 0, "0"},
    };

    for(i = 0; i < sizeof(args) / sizeof(args[0]); i++)
    {
        fn = bnfp_create();
        r = bnfp_from_string(fn, args[i].value, strlen(args[i].value));

        ASSERT(r == 0);
        ASSERT(fn->scale == args[i].scale);

        to_string = bnfp_to_string(fn);

        // printf("value: %s\targs[i].to_string: %s\tto_string: %s\n", args[i].value, args[i].to_string, to_string);
        ASSERT(fn->scale == args[i].scale);

        ASSERT(strlen(args[i].to_string) == strlen(to_string));
        ASSERT(memcmp(to_string, args[i].to_string, strlen(args[i].to_string)) == 0);

        bnfp_free_string(to_string);

        bnfp_destroy(fn);
    }
}


void test_get_error_string()
{
    char *s;

    s = (char *) bnfp_get_error_string(-1);
    ASSERT(memcmp(s, "Failed to initialize bignum", strlen("Failed to initialize bignum")) == 0);

    s = (char *) bnfp_get_error_string(-3);
    ASSERT(memcmp(s, "Failed to perform addition operation", strlen("Failed to perform addition operation")) == 0);

    s = (char *) bnfp_get_error_string(-99);
    ASSERT(memcmp(s, "BNFP Unknown Error", strlen("BNFP Unknown Error")) == 0);
}


void test_invalid_floating_point_number()
{
    int r, i;
    bnfp_t fn;

    const char *args[] = {
        "23ds42.3434",
        "2342.34sdfs34"
        "23s42.34sdfs34"
        "23-42.3434",
        "2342.343-4",
        "23-42.343-4",
        ".2342",
        "..2342",
        "--2342",
        "--.2342",
        "-.-.2342",
        "-.2342",
        ".-2342",
        "2343-.4082",
        "a343.4082",
        "1343.40.82",
        "0000abc",
        "0000abc000",
        "00.00abc000",
        "0.000abc000",
        "000adas.000",
        "0adas00.000",
        "000adas00.000"
    };

    for(i = 0; i < sizeof(args) / sizeof(args[0]); i++)
    {
        r = bnfp_from_string(&fn, args[i], strlen(args[i]));
        ASSERT(r == -2);
        ASSERT(memcmp(bnfp_get_error_string(r), "Invalid floating point number", strlen("Invalid floating point number")) == 0);
    }
}


void test_bnfp_add()
{
    int i, r;
    bnfp_t *va, *vb, *vr;
    char *to_string;

    struct arg {
        struct va {
            char *value;
            unsigned int scale;
            char *to_string;
        } va;

        struct vb {
            char *value;
            unsigned int scale;
            char *to_string;
        } vb;

        struct vr {
            char *value;
            unsigned int scale;
            char *to_string;
        } vr;
    };

    const struct arg args[] = {
        {{"12173", 0, "12173"}, {"12173", 0, "12173"}, {"24346", 0, "24346"}},
        {{"1217.8", 1, "1217.8"}, {"124.353", 3, "124.353"}, {"1342.153", 3, "1342.153"}},
        {{"124.353", 3, "124.353"}, {"1217.8", 1, "1217.8"}, {"1342.153", 3, "1342.153"}},
        {{"12.173", 3, "12.173"}, {"2.827", 3, "2.827"}, {"15", 0, "15"}},
        {{"12.173", 3, "12.173"}, {"2.727", 3, "2.727"}, {"14.9", 1, "14.9"}},
        {{"12.173", 3, "12.173"}, {"2.817", 3, "2.817"}, {"14.99", 2, "14.99"}},
        {{"12.173", 3, "12.173"}, {"2.826", 3, "2.826"}, {"14.999", 3, "14.999"}},
        {{"12.173", 3, "12.173"}, {"-0.173", 3, "-0.173"}, {"12", 0, "12"}},
        {{"23424234242.25345353453", 11, "23424234242.25345353453"}, {"1.2", 1, "1.2"}, {"23424234243.45345353453", 11, "23424234243.45345353453"}},
        {{"23424234242.25345353453", 11, "23424234242.25345353453"}, {"-1.2", 1, "-1.2"}, {"23424234241.05345353453", 11, "23424234241.05345353453"}},
        {{"99.373", 3, "99.373"}, {"1.859", 3, "1.859"}, {"101.232", 3, "101.232"}},
        {{"99.373", 3, "99.373"}, {"-1.859", 3, "-1.859"}, {"97.514", 3, "97.514"}},
        {{"4998.8", 1, "4998.8"}, {"1.2", 1, "1.2"}, {"5000", 0, "5000"}},
        {{"0", 0, "0"}, {"-30", 0, "-30"}, {"-30", 0, "-30"}},
        {{"0", 0, "0"}, {"-30.002", 3, "-30.002"}, {"-30.002", 3, "-30.002"}},
        {{"5000", 0, "5000"}, {"5000", 0, "5000"}, {"10000", 0, "10000"}},
        {{"10000", 0, "10000"}, {"-0.0002", 4, "-0.0002"}, {"9999.9998", 4, "9999.9998"}},
        {{"10000", 0, "10000"}, {"0.0002", 4, "0.0002"}, {"10000.0002", 4, "10000.0002"}},
        {{"10000.0002", 4, "10000.0002"}, {"-0.01", 2, "-0.01"}, {"9999.9902", 4, "9999.9902"}},
        {{"10000.0002", 4, "10000.0002"}, {"0.01", 2, "0.01"}, {"10000.0102", 4, "10000.0102"}}
    };

    for(i = 0; i < sizeof(args) / sizeof(args[0]); i++)
    {
        // printf("va: %s, vb: %s, vr: %s, vr.to_string: %s\n", args[i].va.value, args[i].vb.value, args[i].vr.value, args[i].vr.to_string);
        //va
        va = bnfp_create();
        r = bnfp_from_string(va, args[i].va.value, strlen(args[i].va.value));

        ASSERT(r == 0);
        ASSERT(va->scale == args[i].va.scale);

        to_string = bnfp_to_string(va);

        ASSERT(va->scale == args[i].va.scale);

        ASSERT(strlen(args[i].va.to_string) == strlen(to_string));
        ASSERT(memcmp(to_string, args[i].va.to_string, strlen(args[i].va.to_string)) == 0);

        bnfp_free_string(to_string);

        // vb
        vb = bnfp_create();
        r = bnfp_from_string(vb, args[i].vb.value, strlen(args[i].vb.value));

        ASSERT(r == 0);
        ASSERT(vb->scale == args[i].vb.scale);

        to_string = bnfp_to_string(vb);

        ASSERT(vb->scale == args[i].vb.scale);

        ASSERT(strlen(args[i].vb.to_string) == strlen(to_string));
        ASSERT(memcmp(to_string, args[i].vb.to_string, strlen(args[i].vb.to_string)) == 0);

        bnfp_free_string(to_string);

        vr = bnfp_create();

        r = bnfp_add(vr, va, vb);

        // vr
        ASSERT(r == 0);
        // ASSERT(vr->scale == args[i].vr.scale);

        to_string = bnfp_to_string(vr);
        // printf("vr->scale: %d, vr to_string: %s\n", vr->scale, to_string);
        ASSERT(vr->scale == args[i].vr.scale);

        ASSERT(strlen(args[i].vr.to_string) == strlen(to_string));
        ASSERT(memcmp(to_string, args[i].vr.to_string, strlen(args[i].vr.to_string)) == 0);

        bnfp_free_string(to_string);

        bnfp_destroy(va);
        bnfp_destroy(vb);
        bnfp_destroy(vr);
    }
}


#endif
