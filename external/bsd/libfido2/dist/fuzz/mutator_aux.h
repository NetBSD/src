/*
 * Copyright (c) 2019 Yubico AB. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 */

#ifndef _MUTATOR_AUX_H
#define _MUTATOR_AUX_H

#include <stddef.h>
#include <stdint.h>

/*
 * As of LLVM 7.0.1, MSAN support in libFuzzer was still experimental.
 * We therefore have to be careful when using our custom mutator, or
 * MSAN will flag uninitialised reads on memory populated by libFuzzer.
 * Since there is no way to suppress MSAN without regenerating object
 * code (in which case you might as well rebuild libFuzzer with MSAN),
 * we adjust our mutator to make it less accurate while allowing
 * fuzzing to proceed.
 */

#if defined(__has_feature)
# if  __has_feature(memory_sanitizer)
#  define NO_MSAN	__attribute__((no_sanitize("memory")))
#  define WITH_MSAN	1
# endif
#endif

#if !defined(WITH_MSAN)
# define NO_MSAN
#endif

#define MAXSTR	1024
#define MAXBLOB	3072

#define GETLEN_MIN	0
#define GETLEN_MAX	1

struct blob {
	uint8_t	body[MAXBLOB];
	size_t	len;
};

size_t xstrlen(const char *);
void consume(const void *, size_t);
void consume_str(const char *);

int unpack_blob(uint8_t, uint8_t **, size_t *, struct blob *);
int unpack_byte(uint8_t, uint8_t **, size_t *, uint8_t *);
int unpack_int(uint8_t, uint8_t **, size_t *, int *);
int unpack_string(uint8_t, uint8_t **, size_t *, char *);

int pack_blob(uint8_t, uint8_t **, size_t *, const struct blob *);
int pack_byte(uint8_t, uint8_t **, size_t *, uint8_t);
int pack_int(uint8_t, uint8_t **, size_t *, int);
int pack_string(uint8_t, uint8_t **, size_t *, const char *);

size_t len_int(void);
size_t len_string(int);
size_t len_byte(void);
size_t len_blob(int);

void mutate_byte(uint8_t *);
void mutate_int(int *);
void mutate_blob(struct blob *);
void mutate_string(char *);

void * dev_open(const char *);
void dev_close(void *);
void set_wire_data(uint8_t *, size_t);
int dev_read(void *, unsigned char *, size_t, int);
int dev_write(void *, const unsigned char *, size_t);

void prng_init(unsigned long);
unsigned long prng_uint32(void);

uint32_t uniform_random(uint32_t);

#endif /* !_MUTATOR_AUX_H */
