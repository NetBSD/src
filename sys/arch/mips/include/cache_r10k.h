/*	$NetBSD: cache_r10k.h,v 1.1 2003/10/05 11:10:25 tsutsui Exp $	*/

/*
 * Copyright (c) 2003 KIYOHARA Takashi <kiyohara@kk.iij4u.or.jp>
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

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Cache definitions/operations for R10000-style caches.
 */

#define	CACHEOP_R10K_CBARRIER		(5 << 2)	/* I */
#define	CACHEOP_R10K_IDX_LOAD_DATA	(6 << 2)	/* I, D, SD */
#define	CACHEOP_R10K_IDX_STORE_DATA	(7 << 2)	/* SI, SD */

#if defined(_KERNEL) && !defined(_LOCORE)

/*
 * cache_r10k_op_8lines_64:
 *
 *	Perform the specified cache operation on 8 64-byte cache lines.
 */
#define	cache_r10k_op_8lines_64(va, op)					\
do {									\
	__asm __volatile(						\
		".set noreorder					\n\t"	\
		"cache %1, 0x000(%0); cache %1, 0x040(%0)	\n\t"	\
		"cache %1, 0x080(%0); cache %1, 0x0c0(%0)	\n\t"	\
		"cache %1, 0x100(%0); cache %1, 0x140(%0)	\n\t"	\
		"cache %1, 0x180(%0); cache %1, 0x1c0(%0)	\n\t"	\
		".set reorder"						\
	    :								\
	    : "r" (va), "i" (op)					\
	    : "memory");						\
} while (/*CONSTCOND*/0)

/*
 * cache_r10k_op_32lines_64:
 *
 *	Perform the specified cache operation on 32 64-byte
 *	cache lines.
 */
#define	cache_r10k_op_32lines_64(va, op)				\
do {									\
	__asm __volatile(						\
		".set noreorder					\n\t"	\
		"cache %1, 0x000(%0); cache %1, 0x040(%0);	\n\t"	\
		"cache %1, 0x080(%0); cache %1, 0x0c0(%0);	\n\t"	\
		"cache %1, 0x100(%0); cache %1, 0x140(%0);	\n\t"	\
		"cache %1, 0x180(%0); cache %1, 0x1c0(%0);	\n\t"	\
		"cache %1, 0x200(%0); cache %1, 0x240(%0);	\n\t"	\
		"cache %1, 0x280(%0); cache %1, 0x2c0(%0);	\n\t"	\
		"cache %1, 0x300(%0); cache %1, 0x340(%0);	\n\t"	\
		"cache %1, 0x380(%0); cache %1, 0x3c0(%0);	\n\t"	\
		"cache %1, 0x400(%0); cache %1, 0x440(%0);	\n\t"	\
		"cache %1, 0x480(%0); cache %1, 0x4c0(%0);	\n\t"	\
		"cache %1, 0x500(%0); cache %1, 0x540(%0);	\n\t"	\
		"cache %1, 0x580(%0); cache %1, 0x5c0(%0);	\n\t"	\
		"cache %1, 0x600(%0); cache %1, 0x640(%0);	\n\t"	\
		"cache %1, 0x680(%0); cache %1, 0x6c0(%0);	\n\t"	\
		"cache %1, 0x700(%0); cache %1, 0x740(%0);	\n\t"	\
		"cache %1, 0x780(%0); cache %1, 0x7c0(%0);	\n\t"	\
		".set reorder"						\
	    :								\
	    : "r" (va), "i" (op)					\
	    : "memory");						\
} while (/*CONSTCOND*/0)

/*
 * cache_r10k_op_16lines_32_2way:
 *
 *	Perform the specified cache operation on 16 64-byte
 * 	cache lines, 2-ways.
 */
#define	cache_r10k_op_16lines_64_2way(va1, va2, op)			\
do {									\
	__asm __volatile(						\
		".set noreorder					\n\t"	\
		"cache %2, 0x000(%0); cache %2, 0x000(%1);	\n\t"	\
		"cache %2, 0x040(%0); cache %2, 0x040(%1);	\n\t"	\
		"cache %2, 0x080(%0); cache %2, 0x080(%1);	\n\t"	\
		"cache %2, 0x0c0(%0); cache %2, 0x0c0(%1);	\n\t"	\
		"cache %2, 0x100(%0); cache %2, 0x100(%1);	\n\t"	\
		"cache %2, 0x140(%0); cache %2, 0x140(%1);	\n\t"	\
		"cache %2, 0x180(%0); cache %2, 0x180(%1);	\n\t"	\
		"cache %2, 0x1c0(%0); cache %2, 0x1c0(%1);	\n\t"	\
		"cache %2, 0x200(%0); cache %2, 0x200(%1);	\n\t"	\
		"cache %2, 0x240(%0); cache %2, 0x240(%1);	\n\t"	\
		"cache %2, 0x280(%0); cache %2, 0x280(%1);	\n\t"	\
		"cache %2, 0x2c0(%0); cache %2, 0x2c0(%1);	\n\t"	\
		"cache %2, 0x300(%0); cache %2, 0x300(%1);	\n\t"	\
		"cache %2, 0x340(%0); cache %2, 0x340(%1);	\n\t"	\
		"cache %2, 0x380(%0); cache %2, 0x380(%1);	\n\t"	\
		"cache %2, 0x3c0(%0); cache %2, 0x3c0(%1);	\n\t"	\
		".set reorder"						\
	    :								\
	    : "r" (va1), "r" (va2), "i" (op)				\
	    : "memory");						\
} while (/*CONSTCOND*/0)

void	r10k_icache_sync_all_64(void);
void	r10k_icache_sync_range_64(vaddr_t, vsize_t);
void	r10k_icache_sync_range_index_64(vaddr_t, vsize_t);

void	r10k_pdcache_wb_range(vaddr_t, vsize_t);

#endif /* _KERNEL && !_LOCORE */

