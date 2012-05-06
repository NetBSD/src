/*-
 * Copyright (c) 2012 Alistair Crooks <agc@NetBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef CAST_H_
#define CAST_H_	20120411

#include <inttypes.h>

#ifndef __BEGIN_DECLS
#  if defined(__cplusplus)
#  define __BEGIN_DECLS           extern "C" {
#  define __END_DECLS             }
#  else
#  define __BEGIN_DECLS
#  define __END_DECLS
#  endif
#endif

__BEGIN_DECLS

typedef struct castkey_t {
	uint32_t	data[32];
	int		short_key;
} castkey_t;

#define CAST_KEY	castkey_t

#define CAST_LONG	uint32_t

/* misc defs */
#define CAST_BLOCK	8
#define CAST_KEY_LENGTH	16

#define CAST_DECRYPT	0
#define CAST_ENCRYPT	1

void CAST_set_key(CAST_KEY */*key*/, int /*len*/, const uint8_t */*data*/);
void CAST_cfb64_encrypt(const uint8_t */*in*/, uint8_t */*out*/, long /*length*/, const CAST_KEY */*schedule*/, uint8_t */*iv*/, int */*num*/, int /*enc*/);
void CAST_ecb_encrypt(const uint8_t */*in*/, uint8_t */*out*/, const CAST_KEY */*ks*/, int /*enc*/);
void CAST_encrypt(CAST_LONG */*data*/, const CAST_KEY */*key*/);
void CAST_decrypt(CAST_LONG */*data*/, const CAST_KEY */*key*/);
void CAST_cbc_encrypt(const uint8_t */*in*/, uint8_t */*out*/, long /*length*/, const CAST_KEY */*ks*/, uint8_t */*iv*/, int /*enc*/);
void CAST_ofb64_encrypt(const uint8_t */*in*/, uint8_t */*out*/, long /*length*/, const CAST_KEY */*key*/, uint8_t */*iv*/, int */*num*/);

__END_DECLS

#endif
