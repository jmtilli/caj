#ifndef _CAJ_HDR_H_
#define _CAJ_HDR_H_

#include <string.h>
#include <stdint.h>

static inline uint32_t caj_htonl(uint32_t h)
{
  uint32_t n;
  unsigned char buf[4];
  buf[0] = (unsigned char)(h>>24);
  buf[1] = (unsigned char)(h>>16);
  buf[2] = (unsigned char)(h>>8);
  buf[3] = (unsigned char)(h>>0);
  memcpy(&n, buf, sizeof(n));
  return n;
}
static inline uint16_t caj_htons(uint16_t h)
{
  uint16_t n;
  unsigned char buf[2];
  buf[0] = (unsigned char)(h>>8);
  buf[1] = (unsigned char)(h>>0);
  memcpy(&n, buf, sizeof(n));
  return n;
}
static inline uint32_t caj_ntohl(uint32_t n)
{
  unsigned char buf[4];
  memcpy(buf, &n, sizeof(n));
  return (uint32_t)((((uint32_t)buf[0])<<24) | (((uint32_t)buf[1])<<16) | (((uint32_t)buf[2])<<8) | (((uint32_t)buf[3])<<0));
}
static inline uint16_t caj_ntohs(uint16_t n)
{
  unsigned char buf[2];
  memcpy(buf, &n, sizeof(n));
  return (uint16_t)((((uint16_t)buf[0])<<8) | (((uint16_t)buf[1])<<0));
}

static inline uint64_t caj_hdr_get64h(const void *buf)
{
  uint64_t res;
  memcpy(&res, buf, sizeof(res));
  return res;
}

static inline uint32_t caj_hdr_get32h(const void *buf)
{
  uint32_t res;
  memcpy(&res, buf, sizeof(res));
  return res;
}

static inline uint16_t caj_hdr_get16h(const void *buf)
{
  uint16_t res;
  memcpy(&res, buf, sizeof(res));
  return res;
}

static inline uint8_t caj_hdr_get8h(const void *buf)
{
  uint8_t res;
  memcpy(&res, buf, sizeof(res));
  return res;
}

static inline void caj_hdr_set64h(void *buf, uint64_t val)
{
  memcpy(buf, &val, sizeof(val));
}

static inline void caj_hdr_set32h(void *buf, uint32_t val)
{
  memcpy(buf, &val, sizeof(val));
}

static inline void caj_hdr_set16h(void *buf, uint16_t val)
{
  memcpy(buf, &val, sizeof(val));
}

static inline void caj_hdr_set8h(void *buf, uint8_t val)
{
  memcpy(buf, &val, sizeof(val));
}

static inline uint32_t caj_hdr_get32n(const void *buf)
{
  return caj_ntohl(caj_hdr_get32h(buf));
}

static inline uint16_t caj_hdr_get16n(const void *buf)
{
  return caj_ntohs(caj_hdr_get16h(buf));
}

static inline void caj_hdr_set32n(void *buf, uint32_t val)
{
  caj_hdr_set32h(buf, caj_htonl(val));
}

static inline void caj_hdr_set16n(void *buf, uint16_t val)
{
  caj_hdr_set16h(buf, caj_htons(val));
}

#endif
