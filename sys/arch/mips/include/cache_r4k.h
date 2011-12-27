/*	$NetBSD: cache_r4k.h,v 1.11.96.2 2011/12/27 01:56:33 matt Exp $	*/

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

/*
 * cache_r4k_op_line:
 *
 *	Perform the specified cache operation on a single line.
 */
static inline void
cache_op_r4k_line(vaddr_t va, u_int op)
{
	__asm volatile(
		".set push"		"\n\t"
		".set noreorder"	"\n\t"
		"cache %[op], 0(%[va])"	"\n\t"
		".set pop"
	    :
	    : [va] "r" (va), [op] "i" (op)
	    : "memory");
}

/*
 * cache_r4k_op_8lines_NN:
 *
 *	Perform the specified cache operation on 8 n-byte cache lines.
 */
static inline void
cache_r4k_op_8lines_NN(size_t n, vaddr_t va, u_int op)
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

/*
 * cache_r4k_op_32lines_NN:
 *
 *	Perform the specified cache operation on 32 n-byte cache lines.
 */
static inline void
cache_r4k_op_32lines_NN(size_t n, vaddr_t va, u_int op)
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
		"cache %[op], (8*%[n])(%[va])"	"\n\t"
		"cache %[op], (9*%[n])(%[va])"	"\n\t"
		"cache %[op], (10*%[n])(%[va])"	"\n\t"
		"cache %[op], (11*%[n])(%[va])"	"\n\t"
		"cache %[op], (12*%[n])(%[va])"	"\n\t"
		"cache %[op], (13*%[n])(%[va])"	"\n\t"
		"cache %[op], (14*%[n])(%[va])"	"\n\t"
		"cache %[op], (15*%[n])(%[va])"	"\n\t"
		"cache %[op], (16*%[n])(%[va])"	"\n\t"
		"cache %[op], (17*%[n])(%[va])"	"\n\t"
		"cache %[op], (18*%[n])(%[va])"	"\n\t"
		"cache %[op], (19*%[n])(%[va])"	"\n\t"
		"cache %[op], (20*%[n])(%[va])"	"\n\t"
		"cache %[op], (21*%[n])(%[va])"	"\n\t"
		"cache %[op], (22*%[n])(%[va])"	"\n\t"
		"cache %[op], (23*%[n])(%[va])"	"\n\t"
		"cache %[op], (24*%[n])(%[va])"	"\n\t"
		"cache %[op], (25*%[n])(%[va])"	"\n\t"
		"cache %[op], (26*%[n])(%[va])"	"\n\t"
		"cache %[op], (27*%[n])(%[va])"	"\n\t"
		"cache %[op], (28*%[n])(%[va])"	"\n\t"
		"cache %[op], (29*%[n])(%[va])"	"\n\t"
		"cache %[op], (30*%[n])(%[va])"	"\n\t"
		"cache %[op], (31*%[n])(%[va])"	"\n\t"
		".set pop"
	    :
	    :	[va] "r" (va), [op] "i" (op), [n] "n" (n)
	    :	"memory");
}

/*
 * cache_r4k_op_32lines_16:
 *
 *	Perform the specified cache operation on 32 16-byte cache lines.
 */
#define	cache_r4k_op_32lines_16(va, op)	\
	    cache_r4k_op_32lines_NN(16, (va), (op))
#define	cache_r4k_op_32lines_32(va, op)	\
	    cache_r4k_op_32lines_NN(32, (va), (op))
#define	cache_r4k_op_32lines_128(va, op) \
	    cache_r4k_op_32lines_NN(128, (va), (op))

/*
 * cache_r4k_op_16lines_16_2way:
 *	Perform the specified cache operation on 16 n-byte cache lines, 2-ways.
 */
static inline void
cache_r4k_op_16lines_NN_2way(size_t n, vaddr_t va1, vaddr_t va2, u_int op)
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
		cache_r4k_op_16lines_NN_2way(16, (va1), (va2), (op)
#define	cache_r4k_op_16lines_32_2way(va1, va2, op)			\
		cache_r4k_op_16lines_NN_2way(32, (va1), (va2), (op)

/*
 * cache_r4k_op_8lines_NN_4way:
 *	Perform the specified cache operation on 8 n-byte cache lines, 4-ways.
 */
static inline void
cache_r4k_op_8lines_NN_4way(size_t n, vaddr_t va1, vaddr_t va2, vaddr_t va3,
	vaddr_t va4, u_int op)
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

void	r4k_icache_sync_all_16(void);
void	r4k_icache_sync_range_16(vaddr_t, vsize_t);
void	r4k_icache_sync_range_index_16(vaddr_t, vsize_t);

void	r4k_icache_sync_all_32(void);
void	r4k_icache_sync_range_32(vaddr_t, vsize_t);
void	r4k_icache_sync_range_index_32(vaddr_t, vsize_t);

void	r4k_pdcache_wbinv_all_16(void);
void	r4k_pdcache_wbinv_range_16(vaddr_t, vsize_t);
void	r4k_pdcache_wbinv_range_index_16(vaddr_t, vsize_t);

void	r4k_pdcache_inv_range_16(vaddr_t, vsize_t);
void	r4k_pdcache_wb_range_16(vaddr_t, vsize_t);

void	r4k_pdcache_wbinv_all_32(void);
void	r4k_pdcache_wbinv_range_32(vaddr_t, vsize_t);
void	r4k_pdcache_wbinv_range_index_32(vaddr_t, vsize_t);

void	r4k_pdcache_inv_range_32(vaddr_t, vsize_t);
void	r4k_pdcache_wb_range_32(vaddr_t, vsize_t);

void	r4k_sdcache_wbinv_all_32(void);
void	r4k_sdcache_wbinv_range_32(vaddr_t, vsize_t);
void	r4k_sdcache_wbinv_range_index_32(vaddr_t, vsize_t);

void	r4k_sdcache_inv_range_32(vaddr_t, vsize_t);
void	r4k_sdcache_wb_range_32(vaddr_t, vsize_t);

void	r4k_sdcache_wbinv_all_128(void);
void	r4k_sdcache_wbinv_range_128(vaddr_t, vsize_t);
void	r4k_sdcache_wbinv_range_index_128(vaddr_t, vsize_t);

void	r4k_sdcache_inv_range_128(vaddr_t, vsize_t);
void	r4k_sdcache_wb_range_128(vaddr_t, vsize_t);

void	r4k_sdcache_wbinv_all_generic(void);
void	r4k_sdcache_wbinv_range_generic(vaddr_t, vsize_t);
void	r4k_sdcache_wbinv_range_index_generic(vaddr_t, vsize_t);

void	r4k_sdcache_inv_range_generic(vaddr_t, vsize_t);
void	r4k_sdcache_wb_range_generic(vaddr_t, vsize_t);

/* cache_r4k_pcache16.S */

void	cache_r4k_icache_index_inv_16(vaddr_t, vsize_t);
void	cache_r4k_icache_hit_inv_16(vaddr_t, vsize_t);
void	cache_r4k_pdcache_index_wb_inv_16(vaddr_t, vsize_t);
void	cache_r4k_pdcache_hit_inv_16(vaddr_t, vsize_t);
void	cache_r4k_pdcache_hit_wb_inv_16(vaddr_t, vsize_t);
void	cache_r4k_pdcache_hit_wb_16(vaddr_t, vsize_t);

/* cache_r4k_pcache32.S */

void	cache_r4k_icache_index_inv_32(vaddr_t, vsize_t);
void	cache_r4k_icache_hit_inv_32(vaddr_t, vsize_t);
void	cache_r4k_pdcache_index_wb_inv_32(vaddr_t, vsize_t);
void	cache_r4k_pdcache_hit_inv_32(vaddr_t, vsize_t);
void	cache_r4k_pdcache_hit_wb_inv_32(vaddr_t, vsize_t);
void	cache_r4k_pdcache_hit_wb_32(vaddr_t, vsize_t);

#endif /* !_LOCORE */
