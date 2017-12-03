/*	$NetBSD: cache_r5900.h,v 1.8.4.3 2017/12/03 11:36:27 jdolecek Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#define CACHE_R5900_SIZE_I		16384
#define	CACHE_R5900_SIZE_D		8192

#define CACHE_R5900_LSIZE_I		64
#define CACHE_R5900_LSIZE_D		64

#define CACHEOP_R5900_IINV_I		0x07	/* INDEX INVALIDATE */
#define CACHEOP_R5900_HINV_I		0x0b	/* HIT INVALIDATE */
#define CACHEOP_R5900_IWBINV_D		0x14
					/* INDEX WRITE BACK INVALIDATE */
#define CACHEOP_R5900_ILTG_D		0x10	/* INDEX LOAD TAG */
#define CACHEOP_R5900_ISTG_D		0x12	/* INDEX STORE TAG */
#define CACHEOP_R5900_IINV_D		0x16	/* INDEX INVALIDATE */
#define CACHEOP_R5900_HINV_D		0x1a	/* HIT INVALIDATE */
#define CACHEOP_R5900_HWBINV_D		0x18	/* HIT WRITEBACK INVALIDATE */
#define CACHEOP_R5900_ILDT_D		0x11	/* INDEX LOAD DATA */
#define CACHEOP_R5900_ISDT_D		0x13	/* INDEX STORE DATA */
#define CACHEOP_R5900_HWB_D		0x1c
					/* HIT WRITEBACK W/O INVALIDATE */

#if !defined(_LOCORE)

#define	cache_op_r5900_line_64(va, op)					\
do {									\
	__asm volatile(						\
		".set noreorder					\n\t"	\
		"sync.l						\n\t"	\
		"sync.p						\n\t"	\
		"cache %1, 0(%0)				\n\t"	\
		"sync.l						\n\t"	\
		"sync.p						\n\t"	\
		".set reorder"						\
	    :								\
	    : "r" (va), "i" (op)					\
	    : "memory");						\
} while (/*CONSTCOND*/0)

#define	cache_r5900_op_4lines_64(va, op)				\
do {									\
	__asm volatile(						\
		".set noreorder					\n\t"	\
		"sync.l						\n\t"	\
		"sync.p						\n\t"	\
		"cache %1,   0(%0)				\n\t"	\
		"sync.l						\n\t"	\
		"sync.p						\n\t"	\
		"cache %1,  64(%0)				\n\t"	\
		"sync.l						\n\t"	\
		"sync.p						\n\t"	\
		"cache %1, 128(%0)				\n\t"	\
		"sync.l						\n\t"	\
		"sync.p						\n\t"	\
		"cache %1, 192(%0)				\n\t"	\
		"sync.l						\n\t"	\
		"sync.p						\n\t"	\
		".set reorder"						\
	    :								\
	    : "r" (va), "i" (op)					\
	    : "memory");						\
} while (/*CONSTCOND*/0)

#define	cache_r5900_op_4lines_64_2way(va, op)				\
do {									\
	__asm volatile(						\
		".set noreorder					\n\t"	\
		"sync.l						\n\t"	\
		"sync.p						\n\t"	\
		"cache %1,   0(%0)				\n\t"	\
		"sync.l						\n\t"	\
		"sync.p						\n\t"	\
		"cache %1,   1(%0)				\n\t"	\
		"sync.l						\n\t"	\
		"sync.p						\n\t"	\
		"cache %1,  64(%0)				\n\t"	\
		"sync.l						\n\t"	\
		"sync.p						\n\t"	\
		"cache %1,  65(%0)				\n\t"	\
		"sync.l						\n\t"	\
		"sync.p						\n\t"	\
		"cache %1, 128(%0)				\n\t"	\
		"sync.l						\n\t"	\
		"sync.p						\n\t"	\
		"cache %1, 129(%0)				\n\t"	\
		"sync.l						\n\t"	\
		"sync.p						\n\t"	\
		"cache %1, 192(%0)				\n\t"	\
		"sync.l						\n\t"	\
		"sync.p						\n\t"	\
		"cache %1, 193(%0)				\n\t"	\
		"sync.l						\n\t"	\
		"sync.p						\n\t"	\
		".set reorder"						\
	    :								\
	    : "r" (va), "i" (op)					\
	    : "memory");						\
} while (/*CONSTCOND*/0)

void	r5900_icache_sync_all_64(void);
void	r5900_icache_sync_range_64(register_t, vsize_t);
void	r5900_icache_sync_range_index_64(vaddr_t, vsize_t);

void	r5900_pdcache_wbinv_all_64(void);
void	r5900_pdcache_wbinv_range_64(register_t, vsize_t);
void	r5900_pdcache_wbinv_range_index_64(vaddr_t, vsize_t);

void	r5900_pdcache_inv_range_64(register_t, vsize_t);
void	r5900_pdcache_wb_range_64(register_t, vsize_t);

#endif /* !_LOCORE */
