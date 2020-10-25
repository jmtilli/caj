#ifndef _CAJ_MURMUR_H_
#define _CAJ_MURMUR_H_

#include <stdint.h>
#include <stddef.h>
#include "cajhdr.h"

#ifdef __cplusplus
extern "C" {
#endif

struct caj_murmurctx {
  uint32_t hash;
  uint32_t len;
};

#define CAJ_MURMURCTX_INITER(seed) \
  { \
    .hash = (seed), \
    .len = 0, \
  }

static inline void caj_murmurctx_feed32(struct caj_murmurctx *ctx, uint32_t val)
{
  uint32_t c1 = 0xcc9e2d51;
  uint32_t c2 = 0x1b873593;
  uint32_t m = 5;
  uint32_t n = 0xe6546b64;
  uint32_t k = val;
  uint32_t hash = ctx->hash;
  k = k*c1;
  k = (k << 15) | (k >> 17);
  k = k*c2;
  hash = hash ^ k;
  hash = (hash << 13) | (hash >> 19);
  hash = hash*m + n;
  ctx->len += 4;
  ctx->hash = hash;
}

static inline void caj_murmurctx_feed_buf(
  struct caj_murmurctx *ctx, const void *buf, size_t sz)
{
  const char *cbuf = (const char*)buf;
  size_t i = 0;
  struct caj_murmurctx ctx2 = *ctx;
  while (i < (sz/4)*4)
  {
    caj_murmurctx_feed32(&ctx2, caj_hdr_get32h(&cbuf[i]));
    i += 4;
  }
  while (i < sz)
  {
    caj_murmurctx_feed32(&ctx2, caj_hdr_get8h(&cbuf[i]));
    i += 1;
  }
  *ctx = ctx2;
}

static inline uint32_t caj_murmurctx_get(struct caj_murmurctx *ctx)
{
  uint32_t hash;
  hash = ctx->hash;
  hash = hash ^ ctx->len;
  hash = hash ^ (hash >> 16);
  hash = hash * 0x85ebca6b;
  hash = hash ^ (hash >> 13);
  hash = hash * 0xc2b2ae35;
  hash = hash ^ (hash >> 16);
  return hash;
}


static inline uint32_t caj_murmur32(uint32_t seed, uint32_t val)
{
  struct caj_murmurctx ctx = CAJ_MURMURCTX_INITER(seed);
  caj_murmurctx_feed32(&ctx, val);
  return caj_murmurctx_get(&ctx);
}

static inline uint32_t caj_murmur_buf(uint32_t seed, const void *buf, size_t sz)
{
  struct caj_murmurctx ctx = CAJ_MURMURCTX_INITER(seed);
  caj_murmurctx_feed_buf(&ctx, buf, sz);
  return caj_murmurctx_get(&ctx);
}

#ifdef __cplusplus
};
#endif

#endif
