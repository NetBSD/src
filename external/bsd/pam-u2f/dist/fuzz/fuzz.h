/* Copyright (C) 2021 Yubico AB - See COPYING */
#ifndef FUZZ_H
#define FUZZ_H

#include <stdint.h>
#include <stddef.h>
#include <security/pam_modules.h>

#define FUZZ_DEV_HANDLE 0x68696421
#define FUZZ_PAM_HANDLE 0x68696423

#define MAXSTR 256
#define MAXBLOB 3072

struct blob {
  uint8_t body[MAXBLOB];
  size_t len;
};

void set_wiredata(uint8_t *, size_t);
void set_user(const char *);
void set_conv(struct pam_conv *);
void set_authfile(int);

int pack_u32(uint8_t **, size_t *, uint32_t);
int unpack_u32(const uint8_t **, size_t *, uint32_t *);
int pack_blob(uint8_t **, size_t *, const struct blob *);
int unpack_blob(const uint8_t **, size_t *, struct blob *);
int pack_string(uint8_t **, size_t *, const char *);
int unpack_string(const uint8_t **, size_t *, char *);

/* part of libfido2's fuzzing instrumentation, requires build with -DFUZZ=1 */
void prng_init(unsigned long);
uint32_t uniform_random(uint32_t);

#endif
