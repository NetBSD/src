/*	$NetBSD: audiofil.h,v 1.2 2019/05/08 13:40:17 isaki Exp $	*/

/*
 * Copyright (C) 2017 Tetsuya Isaki. All rights reserved.
 * Copyright (C) 2017 Y.Sugahara (moveccr). All rights reserved.
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

#ifndef _SYS_DEV_AUDIO_AUDIOFIL_H_
#define _SYS_DEV_AUDIO_AUDIOFIL_H_

/*
 * Number of bits for internal format.
 * XXX 32bit mode is not completed.
 * XXX Is this necessary?
 */
#define AUDIO_INTERNAL_BITS		16
/*#define AUDIO_INTERNAL_BITS		32 */

#if AUDIO_INTERNAL_BITS == 16

typedef int16_t  aint_t;	/* audio          integer */
typedef uint16_t auint_t;	/* audio unsigned integer */
typedef int32_t  aint2_t;	/* audio          wide integer */
typedef uint32_t auint2_t;	/* audio unsigned wide integer */
#define AINT_T_MAX	((aint_t)0x7fff)
#define AINT_T_MIN	((aint_t)0x8000)

#elif AUDIO_INTERNAL_BITS == 32

typedef int32_t  aint_t;
typedef uint32_t auint_t;
typedef int64_t  aint2_t;
typedef uint64_t auint2_t;
#define AINT_T_MAX	((aint_t)0x7fffffff)
#define AINT_T_MIN	((aint_t)0x80000000)

#else
#error Invalid AUDIO_INTERNAL_BITS
#endif

/*
 * audio format.
 *
 * precision <= stride always holds.
 */
typedef struct {
	u_int	sample_rate;	/* sample rate in Hz */
	u_int	encoding;	/* AUDIO_ENCODING_* */
	u_int	stride;		/* container bits of a sample */
	u_int	precision;	/* valid bits of a sample */
	u_int	channels;	/* 1..AUDIO_MAX_CHANNELS */
} audio_format2_t;

/* Parameters for conversion filters */
typedef struct {
	/* Pointer to source samples. */
	const void *src;
	/* Input format */
	const audio_format2_t *srcfmt;

	/* Pointer to destination buffer. */
	void *dst;
	/* Output format */
	const audio_format2_t *dstfmt;

	/*
	 * Number of frames to be converted.
	 * The conversion filter must output 'count' frames.  src and dst
	 * are guaranteed that at least 'count' frames are contiguous.
	 * The caller does not reference this count after calling, so
	 * that the conversion filter can use passed this variable.
	 * For example, decrementing it directly.
	 */
	u_int count;

	/* The conversion filters can use this pointer. */
	void *context;
} audio_filter_arg_t;

typedef void(*audio_filter_t)(audio_filter_arg_t *arg);

/* Filter registration structure */
typedef struct {
	audio_filter_t codec;	/* conversion function */
	void *context;		/* optional codec's argument */
} audio_filter_reg_t;

#endif /* !_SYS_DEV_AUDIO_AUDIOFIL_H_ */
