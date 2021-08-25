#ifndef _EX_BNFP_H
#define _EX_BNFP_H

#include <openssl/bn.h>

// opaque handle
typedef struct bnfp_s bnfp_t;

bnfp_t *bnfp_create();
int bnfp_from_string(bnfp_t *fn, const char *s, size_t len);
char *bnfp_to_string(const bnfp_t *fn);
void bnfp_free_string(const void *s);
void bnfp_destroy(const bnfp_t *fn);
const char *bnfp_get_error_string(int err);
int bnfp_add(bnfp_t *r, bnfp_t *a, bnfp_t *b);


#endif
