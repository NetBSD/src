/*	$NetBSD: cache_r4k.h,v 1.11.142.1 2016/10/05 20:55:31 skrll Exp $	*/

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
 * Cache definitions/operations for R4000-style caches.
 */

#define	CACHE_R4K_I			0
#define	CACHE_R4K_D			1
#define	CACHE_R4K_SI			2
#define	CACHE_R4K_SD			3

#define	CACHEOP_R4K_INDEX_INV		(0 << 2)	/* I, SI */
#define	CACHEOP_R4K_INDEX_WB_INV	(0 << 2)	/* D, SD */
#define	CACHEOP_R4K_INDEX_LOAD_TAG	(1 << 2)	/* all */
#define	CACHEOP_R4K_INDEX_STORE_TAG	(2 << 2)	/* all */
#define	CACHEOP_R4K_CREATE_DIRTY_EXCL	(3 << 2)	/* D, SD */
#define	CACHEOP_R4K_HIT_INV		(4 << 2)	/* all */
#define	CACHEOP_R4K_HIT_WB_INV		(5 << 2)	/* D, SD */
#define	CACHEOP_R4K_FILL		(5 << 2)	/* I */
#define	CACHEOP_R4K_HIT_WB		(6 << 2)	/* I, D, SD */
#define	CACHEOP_R4K_HIT_SET_VIRTUAL	(7 << 2)	/* SI, SD */

#if !defined(_LOCORE)

#if 1
/*
 * cache_r4k_op_line:
 *
 *	Perform the specified cache operation on a single line.
 */
#define cache_op_r4k_line(va, op)				\
{								\
	__asm volatile(						\
		".set push"		"\n\t"			\
		".set noreorder"	"\n\t"			\
		"cache %0, 0(%[va])"	"\n\t"			\
		".set pop"					\
	    :							\
	    : "i" (op), [va] "r" (va)				\
	    : "memory");					\
}

/*
 * cache_r4k_op_8lines_NN:
 *
 *	Perform the specified cache operation on 8 n-byte cache lines.
 */
static inline void
cache_r4k_op_8lines_NN(size_t n, register_t va, u_int op)
{
	__asm volatile(
		".set push"			"\n\t"
		".set noreorder"		"\n\t"
		"cache %[op], (0*%[n])(%[va])"	"\n\t"
		"cache %[op], (1*%[n])(%[va])"	"\n\t"
		"cache %[op], (2*%[n])(%[va])"	"\n\t"
		"cache %[op], (3*%[n])(%[va])"	"\n\t"
		"cache %[op], (4*%[n])(%[va])"	"\n\t"
		"cache %[op], (5*%[n])(%[va])"	"\n\t"
		"cache %[op], (6*%[n])(%[va])"	"\n\t"
		"cache %[op], (7*%[n])(%[va])"	"\n\t"
		".set pop"
	    :
	    :	[va] "r" (va), [op] "i" (op), [n] "n" (n)
	    :	"memory");
}

/*
 * cache_r4k_op_8lines_16:
 *	Perform the specified cache operation on 8 16-byte cache lines.
 * cache_r4k_op_8lines_32:
 *	Perform the specified cache operation on 8 32-byte cache lines.
 */
#define	cache_r4k_op_8lines_16(va, op)	\
	    cache_r4k_op_8lines_NN(16, (va), (op))
#define	cache_r4k_op_8lines_32(va, op)	\
	    cache_r4k_op_8lines_NN(32, (va), (op))
#define	cache_r4k_op_8lines_64(va, op)	\
	    cache_r4k_op_8lines_NN(64, (va), (op))
#define	cache_r4k_op_8lines_128(va, op)	\
	    cache_r4k_op_8lines_NN(128, (va), (op))

/*
 * cache_r4k_op_32lines_NN:
 *
 *	Perform the specified cache operation on 32 n-byte cache lines.
 */
#define cache_r4k_op_32lines_NN(n, va, op)				\
{									\
	__asm volatile(							\
		".set push"			"\n\t"			\
		".set noreorder"		"\n\t"			\
		"cache %2, (0*%0)(%[va])"	"\n\t"			\
		"cache %2, (1*%0)(%[va])"	"\n\t"			\
		"cache %2, (2*%0)(%[va])"	"\n\t"			\
		"cache %2, (3*%0)(%[va])"	"\n\t"			\
		"cache %2, (4*%0)(%[va])"	"\n\t"			\
		"cache %2, (5*%0)(%[va])"	"\n\t"			\
		"cache %2, (6*%0)(%[va])"	"\n\t"			\
		"cache %2, (7*%0)(%[va])"	"\n\t"			\
		"cache %2, (8*%0)(%[va])"	"\n\t"			\
		"cache %2, (9*%0)(%[va])"	"\n\t"			\
		"cache %2, (10*%0)(%[va])"	"\n\t"			\
		"cache %2, (11*%0)(%[va])"	"\n\t"			\
		"cache %2, (12*%0)(%[va])"	"\n\t"			\
		"cache %2, (13*%0)(%[va])"	"\n\t"			\
		"cache %2, (14*%0)(%[va])"	"\n\t"			\
		"cache %2, (15*%0)(%[va])"	"\n\t"			\
		"cache %2, (16*%0)(%[va])"	"\n\t"			\
		"cache %2, (17*%0)(%[va])"	"\n\t"			\
		"cache %2, (18*%0)(%[va])"	"\n\t"			\
		"cache %2, (19*%0)(%[va])"	"\n\t"			\
		"cache %2, (20*%0)(%[va])"	"\n\t"			\
		"cache %2, (21*%0)(%[va])"	"\n\t"			\
		"cache %2, (22*%0)(%[va])"	"\n\t"			\
		"cache %2, (23*%0)(%[va])"	"\n\t"			\
		"cache %2, (24*%0)(%[va])"	"\n\t"			\
		"cache %2, (25*%0)(%[va])"	"\n\t"			\
		"cache %2, (26*%0)(%[va])"	"\n\t"			\
		"cache %2, (27*%0)(%[va])"	"\n\t"			\
		"cache %2, (28*%0)(%[va])"	"\n\t"			\
		"cache %2, (29*%0)(%[va])"	"\n\t"			\
		"cache %2, (30*%0)(%[va])"	"\n\t"			\
		"cache %2, (31*%0)(%[va])"	"\n\t"			\
		".set pop"						\
	    :								\
	    :	"i" (n), [va] "r" (va), "i" (op)			\
	    :	"memory");						\
}

/*
 * cache_r4k_op_32lines_16:
 *
 *	Perform the specified cache operation on 32 16-byte cache lines.
 */
#define	cache_r4k_op_32lines_16(va, op)	\
	    cache_r4k_op_32lines_NN(16, va, op)
#define	cache_r4k_op_32lines_32(va, op)	\
	    cache_r4k_op_32lines_NN(32, va, op)
#define	cache_r4k_op_32lines_64(va, op) \
	    cache_r4k_op_32lines_NN(64, va, op)
#define	cache_r4k_op_32lines_128(va, op) \
	    cache_r4k_op_32lines_NN(128, va, op)

/*
 * cache_r4k_op_16lines_16_2way:
 *	Perform the specified cache operation on 16 n-byte cache lines, 2-ways.
 */
static inline void
cache_r4k_op_16lines_NN_2way(size_t n, register_t va1, register_t va2, u_int op)
{
	__asm volatile(
		".set push"			"\n\t"
		".set noreorder"		"\n\t"
		"cache %[op], (0*%[n])(%[va1])"	"\n\t"
		"cache %[op], (0*%[n])(%[va2])"	"\n\t"
		"cache %[op], (1*%[n])(%[va1])"	"\n\t"
		"cache %[op], (1*%[n])(%[va2])"	"\n\t"
		"cache %[op], (2*%[n])(%[va1])"	"\n\t"
		"cache %[op], (2*%[n])(%[va2])"	"\n\t"
		"cache %[op], (3*%[n])(%[va1])"	"\n\t"
		"cache %[op], (3*%[n])(%[va2])"	"\n\t"
		"cache %[op], (4*%[n])(%[va1])"	"\n\t"
		"cache %[op], (4*%[n])(%[va2])"	"\n\t"
		"cache %[op], (5*%[n])(%[va1])"	"\n\t"
		"cache %[op], (5*%[n])(%[va2])"	"\n\t"
		"cache %[op], (6*%[n])(%[va1])"	"\n\t"
		"cache %[op], (6*%[n])(%[va2])"	"\n\t"
		"cache %[op], (7*%[n])(%[va1])"	"\n\t"
		"cache %[op], (7*%[n])(%[va2])"	"\n\t"
		"cache %[op], (8*%[n])(%[va1])"	"\n\t"
		"cache %[op], (8*%[n])(%[va2])"	"\n\t"
		"cache %[op], (9*%[n])(%[va1])"	"\n\t"
		"cache %[op], (9*%[n])(%[va2])"	"\n\t"
		"cache %[op], (10*%[n])(%[va1])"	"\n\t"
		"cache %[op], (10*%[n])(%[va2])"	"\n\t"
		"cache %[op], (11*%[n])(%[va1])"	"\n\t"
		"cache %[op], (11*%[n])(%[va2])"	"\n\t"
		"cache %[op], (12*%[n])(%[va1])"	"\n\t"
		"cache %[op], (12*%[n])(%[va2])"	"\n\t"
		"cache %[op], (13*%[n])(%[va1])"	"\n\t"
		"cache %[op], (13*%[n])(%[va2])"	"\n\t"
		"cache %[op], (14*%[n])(%[va1])"	"\n\t"
		"cache %[op], (14*%[n])(%[va2])"	"\n\t"
		"cache %[op], (15*%[n])(%[va1])"	"\n\t"
		"cache %[op], (15*%[n])(%[va2])"	"\n\t"
		".set pop"
	    :
	    :	[va1] "r" (va1), [va2] "r" (va2), [op] "i" (op), [n] "n" (n)
	    :	"memory");
}

/*
 * cache_r4k_op_16lines_16_2way:
 *	Perform the specified cache operation on 16 16-byte cache lines, 2-ways.
 * cache_r4k_op_16lines_32_2way:
 *	Perform the specified cache operation on 16 32-byte cache lines, 2-ways.
 */
#define	cache_r4k_op_16lines_16_2way(va1, va2, op)			\
		cache_r4k_op_16lines_NN_2way(16, (va1), (va2), (op))
#define	cache_r4k_op_16lines_32_2way(va1, va2, op)			\
		cache_r4k_op_16lines_NN_2way(32, (va1), (va2), (op))
#define	cache_r4k_op_16lines_64_2way(va1, va2, op)			\
		cache_r4k_op_16lines_NN_2way(64, (va1), (va2), (op))

/*
 * cache_r4k_op_8lines_NN_4way:
 *	Perform the specified cache operation on 8 n-byte cache lines, 4-ways.
 */
static inline void
cache_r4k_op_8lines_NN_4way(size_t n, register_t va1, register_t va2,
	register_t va3, register_t va4, u_int op)
{
	__asm volatile(
		".set push"			"\n\t"
		".set noreorder"		"\n\t"
		"cache %[op], (0*%[n])(%[va1])"	"\n\t"
		"cache %[op], (0*%[n])(%[va2])"	"\n\t"
		"cache %[op], (0*%[n])(%[va3])"	"\n\t"
		"cache %[op], (0*%[n])(%[va4])"	"\n\t"
		"cache %[op], (1*%[n])(%[va1])"	"\n\t"
		"cache %[op], (1*%[n])(%[va2])"	"\n\t"
		"cache %[op], (1*%[n])(%[va3])"	"\n\t"
		"cache %[op], (1*%[n])(%[va4])"	"\n\t"
		"cache %[op], (2*%[n])(%[va1])"	"\n\t"
		"cache %[op], (2*%[n])(%[va2])"	"\n\t"
		"cache %[op], (2*%[n])(%[va3])"	"\n\t"
		"cache %[op], (2*%[n])(%[va4])"	"\n\t"
		"cache %[op], (3*%[n])(%[va1])"	"\n\t"
		"cache %[op], (3*%[n])(%[va2])"	"\n\t"
		"cache %[op], (3*%[n])(%[va3])"	"\n\t"
		"cache %[op], (3*%[n])(%[va4])"	"\n\t"
		"cache %[op], (4*%[n])(%[va1])"	"\n\t"
		"cache %[op], (4*%[n])(%[va2])"	"\n\t"
		"cache %[op], (4*%[n])(%[va3])"	"\n\t"
		"cache %[op], (4*%[n])(%[va4])"	"\n\t"
		"cache %[op], (5*%[n])(%[va1])"	"\n\t"
		"cache %[op], (5*%[n])(%[va2])"	"\n\t"
		"cache %[op], (5*%[n])(%[va3])"	"\n\t"
		"cache %[op], (5*%[n])(%[va4])"	"\n\t"
		"cache %[op], (6*%[n])(%[va1])"	"\n\t"
		"cache %[op], (6*%[n])(%[va2])"	"\n\t"
		"cache %[op], (6*%[n])(%[va3])"	"\n\t"
		"cache %[op], (6*%[n])(%[va4])"	"\n\t"
		"cache %[op], (7*%[n])(%[va1])"	"\n\t"
		"cache %[op], (7*%[n])(%[va2])"	"\n\t"
		"cache %[op], (7*%[n])(%[va3])"	"\n\t"
		"cache %[op], (7*%[n])(%[va4])"	"\n\t"
		".set pop"
	    :
	    :	[va1] "r" (va1), [va2] "r" (va2),
	    	[va3] "r" (va3), [va4] "r" (va4),
		[op] "i" (op), [n] "n" (n)
	    :	"memory");
}
/*
 * cache_r4k_op_8lines_16_4way:
 *	Perform the specified cache operation on 8 16-byte cache lines, 4-ways.
 * cache_r4k_op_8lines_32_4way:
 *	Perform the specified cache operation on 8 32-byte cache lines, 4-ways.
 */
#define	cache_r4k_op_8lines_16_4way(va1, va2, va3, va4, op) \
	    cache_r4k_op_8lines_NN_4way(16, (va1), (va2), (va3), (va4), (op))
#define	cache_r4k_op_8lines_32_4way(va1, va2, va3, va4, op) \
	    cache_r4k_op_8lines_NN_4way(32, (va1), (va2), (va3), (va4), (op))
#define	cache_r4k_op_8lines_64_4way(va1, va2, va3, va4, op) \
	    cache_r4k_op_8lines_NN_4way(64, (va1), (va2), (va3), (va4), (op))
#define	cache_r4k_op_8lines_128_4way(va1, va2, va3, va4, op) \
	    cache_r4k_op_8lines_NN_4way(128, (va1), (va2), (va3), (va4), (op))
#endif

/* cache_r4k.c */

void	r4k_icache_sync_all_generic(void);
void	r4k_icache_sync_range_generic(register_t, vsize_t);
void	r4k_icache_sync_range_index_generic(vaddr_t, vsize_t);
void	r4k_pdcache_wbinv_all_generic(void);
void	r4k_sdcache_wbinv_all_generic(void);

/* cache_r4k_pcache16.S */

void	cache_r4k_icache_index_inv_16(vaddr_t, vsize_t);
void	cache_r4k_icache_hit_inv_16(register_t, vsize_t);
void	cache_r4k_pdcache_index_wb_inv_16(vaddr_t, vsize_t);
void	cache_r4k_pdcache_hit_inv_16(register_t, vsize_t);
void	cache_r4k_pdcache_hit_wb_inv_16(register_t, vsize_t);
void	cache_r4k_pdcache_hit_wb_16(register_t, vsize_t);

/* cache_r4k_scache16.S */

void	cache_r4k_sdcache_index_wb_inv_16(vaddr_t, vsize_t);
void	cache_r4k_sdcache_hit_inv_16(register_t, vsize_t);
void	cache_r4k_sdcache_hit_wb_inv_16(register_t, vsize_t);
void	cache_r4k_sdcache_hit_wb_16(register_t, vsize_t);
 
/* cache_r4k_pcache32.S */

void	cache_r4k_icache_index_inv_32(vaddr_t, vsize_t);
void	cache_r4k_icache_hit_inv_32(register_t, vsize_t);
void	cache_r4k_pdcache_index_wb_inv_32(vaddr_t, vsize_t);
void	cache_r4k_pdcache_hit_inv_32(register_t, vsize_t);
void	cache_r4k_pdcache_hit_wb_inv_32(register_t, vsize_t);
void	cache_r4k_pdcache_hit_wb_32(register_t, vsize_t);

/* cache_r4k_scache32.S */

void	cache_r4k_sdcache_index_wb_inv_32(vaddr_t, vsize_t);
void	cache_r4k_sdcache_hit_inv_32(register_t, vsize_t);
void	cache_r4k_sdcache_hit_wb_inv_32(register_t, vsize_t);
void	cache_r4k_sdcache_hit_wb_32(register_t, vsize_t);
 
/* cache_r4k_pcache64.S */

void	cache_r4k_icache_index_inv_64(vaddr_t, vsize_t);
void	cache_r4k_icache_hit_inv_64(register_t, vsize_t);
void	cache_r4k_pdcache_index_wb_inv_64(vaddr_t, vsize_t);
void	cache_r4k_pdcache_hit_inv_64(register_t, vsize_t);
void	cache_r4k_pdcache_hit_wb_inv_64(register_t, vsize_t);
void	cache_r4k_pdcache_hit_wb_64(register_t, vsize_t);

/* cache_r4k_scache64.S */

void	cache_r4k_sdcache_index_wb_inv_64(vaddr_t, vsize_t);
void	cache_r4k_sdcache_hit_inv_64(register_t, vsize_t);
void	cache_r4k_sdcache_hit_wb_inv_64(register_t, vsize_t);
void	cache_r4k_sdcache_hit_wb_64(register_t, vsize_t);
 
/* cache_r4k_pcache128.S */

void	cache_r4k_icache_index_inv_128(vaddr_t, vsize_t);
void	cache_r4k_icache_hit_inv_128(register_t, vsize_t);
void	cache_r4k_pdcache_index_wb_inv_128(vaddr_t, vsize_t);
void	cache_r4k_pdcache_hit_inv_128(register_t, vsize_t);
void	cache_r4k_pdcache_hit_wb_inv_128(register_t, vsize_t);
void	cache_r4k_pdcache_hit_wb_128(register_t, vsize_t);

/* cache_r4k_scache128.S */

void	cache_r4k_sdcache_index_wb_inv_128(vaddr_t, vsize_t);
void	cache_r4k_sdcache_hit_inv_128(register_t, vsize_t);
void	cache_r4k_sdcache_hit_wb_inv_128(register_t, vsize_t);
void	cache_r4k_sdcache_hit_wb_128(register_t, vsize_t);
 
#endif /* !_LOCORE */
