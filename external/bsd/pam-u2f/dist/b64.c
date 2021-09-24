/*
 * Copyright (C) 2018 Yubico AB - See COPYING
 */

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

#include "b64.h"

int b64_encode(const void *ptr, size_t len, char **out) {
  BIO *bio_b64 = NULL;
  BIO *bio_mem = NULL;
  char *b64_ptr = NULL;
  long b64_len;
  int n;
  int ok = 0;

  if (ptr == NULL || out == NULL || len > INT_MAX)
    return (0);

  *out = NULL;

  bio_b64 = BIO_new(BIO_f_base64());
  if (bio_b64 == NULL)
    goto fail;

  bio_mem = BIO_new(BIO_s_mem());
  if (bio_mem == NULL)
    goto fail;

  BIO_set_flags(bio_b64, BIO_FLAGS_BASE64_NO_NL);
  BIO_push(bio_b64, bio_mem);

  n = BIO_write(bio_b64, ptr, (int) len);
  if (n < 0 || (size_t) n != len)
    goto fail;

  if (BIO_flush(bio_b64) < 0)
    goto fail;

  b64_len = BIO_get_mem_data(bio_b64, &b64_ptr);
  if (b64_len < 0 || (size_t) b64_len == SIZE_MAX || b64_ptr == NULL)
    goto fail;

  *out = calloc(1, (size_t) b64_len + 1);
  if (*out == NULL)
    goto fail;

  memcpy(*out, b64_ptr, (size_t) b64_len);
  ok = 1;

fail:
  BIO_free(bio_b64);
  BIO_free(bio_mem);

  return (ok);
}

int b64_decode(const char *in, void **ptr, size_t *len) {
  BIO *bio_mem = NULL;
  BIO *bio_b64 = NULL;
  size_t alloc_len;
  int n;
  int ok = 0;

  if (in == NULL || ptr == NULL || len == NULL || strlen(in) > INT_MAX)
    return (0);

  *ptr = NULL;
  *len = 0;

  bio_b64 = BIO_new(BIO_f_base64());
  if (bio_b64 == NULL)
    goto fail;

  bio_mem = BIO_new_mem_buf((const void *) in, -1);
  if (bio_mem == NULL)
    goto fail;

  BIO_set_flags(bio_b64, BIO_FLAGS_BASE64_NO_NL);
  BIO_push(bio_b64, bio_mem);

  alloc_len = strlen(in);
  *ptr = calloc(1, alloc_len);
  if (*ptr == NULL)
    goto fail;

  n = BIO_read(bio_b64, *ptr, (int) alloc_len);
  if (n < 0 || BIO_eof(bio_b64) == 0)
    goto fail;

  *len = (size_t) n;
  ok = 1;

fail:
  BIO_free(bio_b64);
  BIO_free(bio_mem);

  if (!ok) {
    free(*ptr);
    *ptr = NULL;
    *len = 0;
  }

  return (ok);
}
