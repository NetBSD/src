/*	$NetBSD: psgpam_enc.c,v 1.3 2023/01/15 05:08:33 tsutsui Exp $	*/

/*
 * Copyright (c) 2018 Yosuke Sugahara. All rights reserved.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * PSGPAM, PSGPCM encoders
 * Function names are PSGPAM, but also used by PSGPCM.
 *
 * PSGPAM and PSGPCM internally use the unsigned type auint_t for
 * intermediate calculations to manage non-linear conversions.
 */

#include <sys/types.h>

#if defined(_KERNEL)
#include <sys/device.h>
#include <sys/audioio.h>
#include <dev/audio/audio_if.h>
#include <luna68k/dev/psgpam_enc.h>
#include <luna68k/dev/psgpam_table.h>
#else
#include <stdint.h>
#include <stdlib.h>
#include "audio/userland.h"
#include "psgpam_enc.h"
#include "psgpam_table.c"
#include "psgpam_table.h"
#endif

void
psgpam_init_context(struct psgpam_codecvar *ctx, u_int sample_rate)
{

	ctx->offset = 65535;
	ctx->sample_rate = sample_rate;
	ctx->expire_initial = sample_rate / 10;
	ctx->expire = ctx->expire_initial;
}

static inline auint_t
dynamic_offset(struct psgpam_codecvar *ctx, auint_t v)
{

	/*
	 * if (the passed value cannot be handled by current offset) {
	 *   update offset to handle the passed value
	 * } else {
	 *   increment offset
	 * }
	 */
	if (v <= ctx->offset) {
		ctx->offset = v;
	} else {
		if (--ctx->expire < 0) {
			ctx->offset += 1;
			ctx->expire = ctx->expire_initial;
		}
	}
	return v - ctx->offset;
}

#define BULK(table) *d++ = table[v]

#define W8(table) *d++ = table[v]

#define W16(table) do {							\
	uint16_t t = (uint16_t)table[v];				\
	*d++ = ((t & 0xf0) << 4) | (t & 0x0f);				\
} while (0)

#define W32(table) do {							\
	uint32_t t = (uint32_t)table[v];				\
	*d++ = ((t & 0xf000) << 12)					\
	     | ((t & 0x0f00) <<  8)					\
	     | ((t & 0x00f0) <<  4)					\
	     | ((t & 0x000f));						\
} while (0)

#define SPLIT3(table) do {						\
	uint16_t t = (uint16_t)table[v];				\
	*d++ = ((t & 0xf000) >> 12);					\
	*d++ = ((t & 0x0f00) >>  8);					\
	*d++ = ((t & 0x000f));						\
} while (0)

#define ENCODER_DEFINE(enc, TT, table, writer)				\
void									\
psgpam_aint_to_##enc(audio_filter_arg_t *arg)				\
{									\
	const aint_t *s = arg->src;					\
	TT *d = arg->dst;						\
									\
	for (int i = 0; i < arg->count; i++) {				\
		auint_t v = (*s++) ^ AINT_T_MIN;			\
		v >>= (AUDIO_INTERNAL_BITS - table##_BITS);		\
		writer(table);						\
	}								\
}

#define ENCODER_D_DEFINE(enc, TT, table, writer)			\
void									\
psgpam_aint_to_##enc##_d(audio_filter_arg_t *arg)			\
{									\
	const aint_t *s = arg->src;					\
	TT *d = arg->dst;						\
									\
	for (int i = 0; i < arg->count; i++) {				\
		auint_t v = (*s++) ^ AINT_T_MIN;			\
		v >>= (AUDIO_INTERNAL_BITS - table##_BITS);		\
		v = dynamic_offset(arg->context, v);			\
		writer(table);						\
	}								\
}

ENCODER_DEFINE(pam2a, uint16_t, PAM2A_TABLE, W16)
ENCODER_DEFINE(pam2b, uint16_t, PAM2B_TABLE, W16)
ENCODER_DEFINE(pam3a, uint32_t, PAM3A_TABLE, W32)
ENCODER_DEFINE(pam3b, uint32_t, PAM3B_TABLE, W32)
ENCODER_DEFINE(pcm1, uint8_t,  PCM1_TABLE, W8)
ENCODER_DEFINE(pcm2, uint16_t, PCM2_TABLE, W16)
ENCODER_DEFINE(pcm3, uint8_t,  PCM3_TABLE, SPLIT3)

ENCODER_D_DEFINE(pam2a, uint16_t, PAM2A_TABLE, W16)
ENCODER_D_DEFINE(pam2b, uint16_t, PAM2B_TABLE, W16)
ENCODER_D_DEFINE(pam3a, uint32_t, PAM3A_TABLE, W32)
ENCODER_D_DEFINE(pam3b, uint32_t, PAM3B_TABLE, W32)
ENCODER_D_DEFINE(pcm1, uint8_t,  PCM1_TABLE, W8)
ENCODER_D_DEFINE(pcm2, uint16_t, PCM2_TABLE, W16)
ENCODER_D_DEFINE(pcm3, uint8_t,  PCM3_TABLE, SPLIT3)
