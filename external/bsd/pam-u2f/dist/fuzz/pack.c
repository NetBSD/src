/* Copyright (C) 2021 Yubico AB - See COPYING */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <arpa/inet.h>

#include "fuzz/fuzz.h"

static int do_unpack(const uint8_t **buf, size_t *rem, uint8_t *dst,
                     size_t size) {
  if (*rem < size)
    return 0;
  memcpy(dst, *buf, size);
  *buf += size;
  *rem -= size;
  return 1;
}

static int do_pack(uint8_t **buf, size_t *rem, const uint8_t *src,
                   size_t size) {
  if (*rem < size)
    return 0;
  memcpy(*buf, src, size);
  *buf += size;
  *rem -= size;
  return 1;
}

int pack_u32(uint8_t **buf, size_t *rem, uint32_t val) {
  val = htonl(val);
  return do_pack(buf, rem, (uint8_t *) &val, sizeof(val));
}

int unpack_u32(const uint8_t **buf, size_t *rem, uint32_t *val) {
  if (!do_unpack(buf, rem, (uint8_t *) val, sizeof(*val)))
    return 0;
  *val = ntohl(*val);
  return 1;
}

int pack_blob(uint8_t **buf, size_t *rem, const struct blob *b) {
  if (b->len > UINT32_MAX || b->len >= MAXBLOB)
    return 0;

  return pack_u32(buf, rem, (uint32_t) b->len) &&
         do_pack(buf, rem, b->body, b->len);
}

int unpack_blob(const uint8_t **buf, size_t *rem, struct blob *b) {
  uint32_t bloblen;

  if (!unpack_u32(buf, rem, &bloblen) || bloblen > MAXBLOB ||
      !do_unpack(buf, rem, b->body, bloblen))
    return 0;

  b->len = bloblen;
  return 1;
}

int pack_string(uint8_t **buf, size_t *rem, const char *s) {
  size_t len = strlen(s);

  if (len > UINT32_MAX || len >= MAXSTR)
    return 0;

  return pack_u32(buf, rem, (uint32_t) len) &&
         do_pack(buf, rem, (const uint8_t *) s, len);
}

int unpack_string(const uint8_t **buf, size_t *rem, char *s) {
  uint32_t len;

  if (!unpack_u32(buf, rem, &len) || len >= MAXSTR ||
      !do_unpack(buf, rem, (uint8_t *) s, len))
    return 0;

  s[len] = '\0';
  return 1;
}
