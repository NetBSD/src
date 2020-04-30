/*	$NetBSD: entpool.h,v 1.1 2020/04/30 03:28:19 riastradh Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
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

#ifndef	ENTPOOL_H
#define	ENTPOOL_H

#include <sys/types.h>
#include <sys/endian.h>

#if defined(_KERNEL) || defined(_STANDALONE)
#include <sys/stdbool.h>
#else
#include <stdbool.h>
#endif

#if ENTPOOL_SMALL
#define	ENTPOOL_HEADER		<gimli.h>
#define	ENTPOOL_PERMUTE		gimli
#define	ENTPOOL_SIZE		48
#define	ENTPOOL_WORD		uint32_t
#define	ENTPOOL_WTOH		le32toh
#define	ENTPOOL_HTOW		htole32
#define	ENTPOOL_SECURITY	16
#else
#define	ENTPOOL_HEADER		<keccak.h>
#define	ENTPOOL_PERMUTE		keccakf1600
#define	ENTPOOL_SIZE		200
#define	ENTPOOL_WORD		uint64_t
#define	ENTPOOL_WTOH		le64toh
#define	ENTPOOL_HTOW		htole64
#define	ENTPOOL_SECURITY	16
#endif

#define	ENTPOOL_CAPACITY	(2*ENTPOOL_SECURITY)
#define	ENTPOOL_RATE		(ENTPOOL_SIZE - ENTPOOL_CAPACITY)

struct entpool {
	union {
		uint8_t		u8[ENTPOOL_SIZE];
		ENTPOOL_WORD	w[ENTPOOL_SIZE/sizeof(ENTPOOL_WORD)];
	}		s;
	unsigned	i;
};

int	entpool_selftest(void);
void	entpool_enter(struct entpool *, const void *, size_t);
bool	entpool_enter_nostir(struct entpool *, const void *, size_t);
void	entpool_stir(struct entpool *);
void	entpool_extract(struct entpool *, void *, size_t);

#endif	/* ENTPOOL_H */
