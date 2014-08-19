/*	$NetBSD: common.h,v 1.1.2.3 2014/08/20 00:05:05 tls Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	VNDCOMPRESS_COMMON_H
#define	VNDCOMPRESS_COMMON_H

#include <sys/cdefs.h>
#include <sys/param.h>

#include <limits.h>
#include <stdint.h>

#ifndef __type_fit

/* XXX Cruft from HEAD.  */

#define __type_mask(t) (/*LINTED*/sizeof(t) < sizeof(intmax_t) ? \
    (~((1ULL << (sizeof(t) * NBBY)) - 1)) : 0ULL)

#define __zeroll() (0LL)
#define __negative_p(x) ((x) < 0)

#define __type_min_s(t) ((t)((1ULL << (sizeof(t) * NBBY - 1))))
#define __type_max_s(t) ((t)~((1ULL << (sizeof(t) * NBBY - 1))))
#define __type_min_u(t) ((t)0ULL)
#define __type_max_u(t) ((t)~0ULL)
#define __type_is_signed(t) (/*LINTED*/__type_min_s(t) + (t)1 < (t)1)
#define __type_min(t) (__type_is_signed(t) ? __type_min_s(t) : __type_min_u(t))
#define __type_max(t) (__type_is_signed(t) ? __type_max_s(t) : __type_max_u(t))


#define __type_fit_u(t, a) (/*LINTED*/sizeof(t) < sizeof(intmax_t) ? \
    (((a) & __type_mask(t)) == 0) : !__negative_p(a))

#define __type_fit_s(t, a) (/*LINTED*/__negative_p(a) ? \
    ((intmax_t)((a) + __zeroll()) >= (intmax_t)__type_min_s(t)) : \
    ((intmax_t)((a) + __zeroll()) <= (intmax_t)__type_max_s(t)))

/*
 * return true if value 'a' fits in type 't'
 */
#define __type_fit(t, a) (__type_is_signed(t) ? \
    __type_fit_s(t, a) : __type_fit_u(t, a))

#endif

/* XXX urk */
#ifndef OFF_MAX
#define	OFF_MAX			__type_max(off_t)
#endif

/* XXX Why is this kernel-only?  */
#define	SET(t, f)	((t) |= (f))
#define	CLR(t, f)	((t) &= ~(f))
#define	ISSET(t, f)	((t) & (f))

/*
 * We require:
 *
 *   0 < blocksize			(duh)
 *   blocksize % DEV_BSIZE == 0		(for kernel use)
 *
 *   blocksize <= UINT32_MAX		(per the format)
 *   blocksize*2 <= SIZE_MAX		(for a compression buffer)
 *   blocksize*2 <= ULONG_MAX		(for zlib API)
 */
#define	MIN_BLOCKSIZE		DEV_BSIZE
#define	DEF_BLOCKSIZE		0x10000
#define	MAX_BLOCKSIZE							\
	rounddown(MIN(UINT32_MAX, (MIN(SIZE_MAX, ULONG_MAX) / 2)),	\
	    MIN_BLOCKSIZE)

/*
 * We require:
 *
 *   n_offsets * sizeof(uint64_t) <= SIZE_MAX	(array of 64-bit offsets)
 *   n_blocks = (n_offsets + 1)		(extra offset to set last block end)
 *   n_blocks <= UINT32_MAX		(per the format)
 *   n_offsets <= UINT32_MAX		(for the sake of simplicity)
 */
#define	MAX_N_BLOCKS							\
	(MIN(UINT32_MAX, (SIZE_MAX / sizeof(uint64_t))) - 1)
#define	MAX_N_OFFSETS		(MAX_N_BLOCKS + 1)

/*
 * The window size is at most the number of offsets, so it has the same
 * maximum bound.  The default window size is chosen so that windows
 * fit in one 4096-byte page of memory.  We could use 64k bytes, or
 * st_blksize, to maximize I/O transfer size, but the transfers won't
 * be aligned without a lot of extra work.
 */
#define	MAX_WINDOW_SIZE		MAX_N_OFFSETS
#define	DEF_WINDOW_SIZE		512

struct cloop2_header {
	char		cl2h_magic[128];
	uint32_t	cl2h_blocksize;
	uint32_t	cl2h_n_blocks;
} __packed;

#define	CLOOP2_OFFSET_TABLE_OFFSET	136

struct options {
	int		flags;
#define	FLAG_c	0x0001	/* Compress */
#define	FLAG_d	0x0002	/* Decompress */
#define	FLAG_p	0x0004	/* Partial */
#define	FLAG_r	0x0008	/* Restart */
#define	FLAG_b	0x0010	/* Block size */
#define	FLAG_R	0x0020	/* abort on Restart failure */
#define	FLAG_k	0x0040	/* checKpoint blocks */
#define	FLAG_l	0x0080	/* Length of input */
#define	FLAG_w	0x0100	/* Window size */
	uint32_t	blocksize;
	uint32_t	end_block;	/* end for partial transfer */
	uint32_t	checkpoint_blocks;	/* blocks before checkpoint */
	uint64_t	length;			/* length of image in bytes */
	uint32_t	window_size;	/* size of window into offset table */
};

int	vndcompress(int, char **, const struct options *);
int	vnduncompress(int, char **, const struct options *);
void	usage(void) __dead;

static const char cloop2_magic[] = "\
#!/bin/sh\n\
#V2.0 Format\n\
insmod cloop.o file=$0 && mount -r -t iso9660 /dev/cloop $1\n\
exit $?\n\
";

#endif	/* VNDCOMPRESS_COMMON_H */
