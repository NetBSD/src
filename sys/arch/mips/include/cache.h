/*	$NetBSD: cache.h,v 1.11.32.2 2016/10/05 20:55:31 skrll Exp $	*/

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

#ifndef _MIPS_CACHE_H_
#define _MIPS_CACHE_H_

/*
 * Cache operations.
 *
 * We define the following primitives:
 *
 * --- Instruction cache synchronization (mandatory):
 *
 *	icache_sync_all		Synchronize I-cache
 *
 *	icache_sync_range	Synchronize I-cache range
 *
 *	icache_sync_range_index	(index ops)
 *
 * --- Primary data cache (mandatory):
 *
 *	pdcache_wbinv_all	Write-back Invalidate primary D-cache
 *
 *	pdcache_wbinv_range	Write-back Invalidate primary D-cache range
 *
 *	pdcache_wbinv_range_index (index ops)
 *
 *	pdcache_inv_range	Invalidate primary D-cache range
 *
 *	pdcache_wb_range	Write-back primary D-cache range
 *
 * --- Secondary data cache (optional):
 *
 *	sdcache_wbinv_all	Write-back Invalidate secondary D-cache
 *
 *	sdcache_wbinv_range	Write-back Invalidate secondary D-cache range
 *
 *	sdcache_wbinv_range_index (index ops)
 *
 *	sdcache_inv_range	Invalidate secondary D-cache range
 *
 *	sdcache_wb_range	Write-back secondary D-cache range
 *
 * There are some rules that must be followed:
 *
 *	I-cache Synch (all or range):
 *		The goal is to synchronize the instruction stream,
 *		so you may need to write-back dirty data cache
 *		blocks first.  If a range is requested, and you
 *		can't synchronize just a range, you have to hit
 *		the whole thing.
 *
 *	D-cache Write-back Invalidate range:
 *		If you can't WB-Inv a range, you must WB-Inv the
 *		entire D-cache.
 *
 *	D-cache Invalidate:
 *		If you can't Inv the D-cache without doing a
 *		Write-back, YOU MUST PANIC.  This is to catch
 *		errors in calling code.  Callers must be aware
 *		of this scenario, and must handle it appropriately
 *		(consider the bus_dma(9) operations).
 *
 *	D-cache Write-back:
 *		If you can't Write-back without doing an invalidate,
 *		that's fine.  Then treat this as a WB-Inv.  Skipping
 *		the invalidate is merely an optimization.
 *
 *	All operations:
 *		Valid virtual addresses must be passed to the
 *		cache operation.
 *
 * Finally, these primitives are grouped together in reasonable
 * ways.  For all operations described here, first the primary
 * cache is frobbed, then the secondary cache frobbed, if the
 * operation for the secondary cache exists.
 *
 *	mips_icache_sync_all	Synchronize I-cache
 *
 *	mips_icache_sync_range	Synchronize I-cache range
 *
 *	mips_icache_sync_range_index (index ops)
 *
 *	mips_dcache_wbinv_all	Write-back Invalidate D-cache
 *
 *	mips_dcache_wbinv_range	Write-back Invalidate D-cache range
 *
 *	mips_dcache_wbinv_range_index (index ops)
 *
 *	mips_dcache_inv_range	Invalidate D-cache range
 *
 *	mips_dcache_wb_range	Write-back D-cache range
 */

struct mips_cache_ops {
	void	(*mco_icache_sync_all)(void);
	void	(*mco_icache_sync_range)(register_t, vsize_t);
	void	(*mco_icache_sync_range_index)(vaddr_t, vsize_t);

	void	(*mco_pdcache_wbinv_all)(void);
	void	(*mco_pdcache_wbinv_range)(register_t, vsize_t);
	void	(*mco_pdcache_wbinv_range_index)(vaddr_t, vsize_t);
	void	(*mco_pdcache_inv_range)(register_t, vsize_t);
	void	(*mco_pdcache_wb_range)(register_t, vsize_t);

	/* These are called only by the (mipsNN) icache functions. */
	void	(*mco_intern_icache_sync_range_index)(vaddr_t, vsize_t);
	void	(*mco_intern_icache_sync_range)(register_t, vsize_t);
	void	(*mco_intern_pdcache_sync_all)(void);
	void	(*mco_intern_pdcache_sync_range_index)(vaddr_t, vsize_t);
	void	(*mco_intern_pdcache_sync_range)(register_t, vsize_t);
	/* This is used internally by the (mipsNN) pdcache functions. */
	void	(*mco_intern_pdcache_wbinv_range_index)(vaddr_t, vsize_t);

	void	(*mco_sdcache_wbinv_all)(void);
	void	(*mco_sdcache_wbinv_range)(register_t, vsize_t);
	void	(*mco_sdcache_wbinv_range_index)(vaddr_t, vsize_t);
	void	(*mco_sdcache_inv_range)(register_t, vsize_t);
	void	(*mco_sdcache_wb_range)(register_t, vsize_t);

	/* These are called only by the (mipsNN) icache functions. */
	void	(*mco_intern_sdcache_sync_all)(void);
	void	(*mco_intern_sdcache_sync_range_index)(vaddr_t, vsize_t);
	void	(*mco_intern_sdcache_sync_range)(register_t, vsize_t);

	/* This is used internally by the (mipsNN) sdcache functions. */
	void	(*mco_intern_sdcache_wbinv_range_index)(vaddr_t, vsize_t);
};

extern struct mips_cache_ops mips_cache_ops;

/* PRIMARY CACHE VARIABLES */
struct mips_cache_info {
	u_int mci_picache_size;
	u_int mci_picache_line_size;
	u_int mci_picache_ways;
	u_int mci_picache_way_size;
	u_int mci_picache_way_mask;
	bool mci_picache_vivt;		/* virtually indexed and tagged */

	u_int mci_pdcache_size;		/* and unified */
	u_int mci_pdcache_line_size;
	u_int mci_pdcache_ways;
	u_int mci_pdcache_way_size;
	u_int mci_pdcache_way_mask;
	bool mci_pdcache_write_through;

	bool mci_pcache_unified;

	/* SECONDARY CACHE VARIABLES */
	u_int mci_sicache_size;
	u_int mci_sicache_line_size;
	u_int mci_sicache_ways;
	u_int mci_sicache_way_size;
	u_int mci_sicache_way_mask;

	u_int mci_sdcache_size;		/* and unified */
	u_int mci_sdcache_line_size;
	u_int mci_sdcache_ways;
	u_int mci_sdcache_way_size;
	u_int mci_sdcache_way_mask;
	bool mci_sdcache_write_through;

	bool mci_scache_unified;

	/* TERTIARY CACHE VARIABLES */
	u_int mci_tcache_size;		/* always unified */
	u_int mci_tcache_line_size;
	u_int mci_tcache_ways;
	u_int mci_tcache_way_size;
	u_int mci_tcache_way_mask;
	bool mci_tcache_write_through;

	/*
	 * These two variables inform the rest of the kernel about the
	 * size of the largest D-cache line present in the system.  The
	 * mask can be used to determine if a region of memory is cache
	 * line size aligned.
	 *
	 * Whenever any code updates a data cache line size, it should
	 * call mips_dcache_compute_align() to recompute these values.
	 */
	u_int mci_dcache_align;
	u_int mci_dcache_align_mask;

	u_int mci_cache_prefer_mask;
	u_int mci_cache_alias_mask;
	u_int mci_icache_alias_mask;

	bool mci_cache_virtual_alias;
	bool mci_icache_virtual_alias;
};


#if (MIPS1 + MIPS64_RMIXL + MIPS64R2_RMIXL + MIPS64_OCTEON) > 0 && \
    (MIPS3 + MIPS4) == 0 \
     && !defined(MODULE)
#define	MIPS_CACHE_ALIAS_MASK		0
#define	MIPS_CACHE_VIRTUAL_ALIAS	false
#else
#define	MIPS_CACHE_ALIAS_MASK		mips_cache_info.mci_cache_alias_mask
#define	MIPS_CACHE_VIRTUAL_ALIAS	mips_cache_info.mci_cache_virtual_alias
#endif
#if (MIPS1 + MIPS64_RMIXL + MIPS64_OCTEON) > 0 && \
    (MIPS3 + MIPS4) == 0 \
    && !defined(_MODULE)
#define	MIPS_ICACHE_ALIAS_MASK		0
#define	MIPS_ICACHE_VIRTUAL_ALIAS	false
#else
#define	MIPS_ICACHE_ALIAS_MASK		mips_cache_info.mci_icache_alias_mask
#define	MIPS_ICACHE_VIRTUAL_ALIAS	mips_cache_info.mci_icache_virtual_alias
#endif

extern struct mips_cache_info mips_cache_info;


/*
 * XXX XXX XXX THIS SHOULD NOT EXIST XXX XXX XXX
 */
#define	mips_cache_indexof(x)	(((vaddr_t)(x)) & MIPS_CACHE_ALIAS_MASK)
#define	mips_cache_badalias(x,y) (((vaddr_t)(x)^(vaddr_t)(y)) & MIPS_CACHE_ALIAS_MASK)

#define	__mco_noargs(prefix, x)						\
do {									\
	(*mips_cache_ops.mco_ ## prefix ## p ## x )();			\
	if (*mips_cache_ops.mco_ ## prefix ## s ## x )			\
		(*mips_cache_ops.mco_ ## prefix ## s ## x )();		\
} while (/*CONSTCOND*/0)

#define	__mco_2args(prefix, x, a, b)					\
do {									\
	(*mips_cache_ops.mco_ ## prefix ## p ## x )((a), (b));		\
	if (*mips_cache_ops.mco_ ## prefix ## s ## x )			\
		(*mips_cache_ops.mco_ ## prefix ## s ## x )((a), (b));	\
} while (/*CONSTCOND*/0)

#define	mips_icache_sync_all()						\
	(*mips_cache_ops.mco_icache_sync_all)()

#define	mips_icache_sync_range(v, s)					\
	(*mips_cache_ops.mco_icache_sync_range)((v), (s))

#define	mips_icache_sync_range_index(v, s)				\
	(*mips_cache_ops.mco_icache_sync_range_index)((v), (s))

#define	mips_dcache_wbinv_all()						\
	__mco_noargs(, dcache_wbinv_all)

#define	mips_dcache_wbinv_range(v, s)					\
	__mco_2args(, dcache_wbinv_range, (v), (s))

#define	mips_dcache_wbinv_range_index(v, s)				\
	__mco_2args(, dcache_wbinv_range_index, (v), (s))

#define	mips_dcache_inv_range(v, s)					\
	__mco_2args(, dcache_inv_range, (v), (s))

#define	mips_dcache_wb_range(v, s)					\
	__mco_2args(, dcache_wb_range, (v), (s))


/*
 * Private D-cache functions only called from (currently only the
 * mipsNN) I-cache functions.
 */
#define	mips_intern_dcache_sync_all()					\
	__mco_noargs(intern_, dcache_sync_all)

#define	mips_intern_dcache_sync_range_index(v, s)			\
	__mco_2args(intern_, dcache_sync_range_index, (v), (s))

#define	mips_intern_dcache_sync_range(v, s)				\
	__mco_2args(intern_, dcache_sync_range, (v), (s))

#define	mips_intern_pdcache_wbinv_range_index(v, s)			\
	(*mips_cache_ops.mco_intern_pdcache_wbinv_range_index)((v), (s))

#define	mips_intern_sdcache_wbinv_range_index(v, s)			\
	(*mips_cache_ops.mco_intern_sdcache_wbinv_range_index)((v), (s))

#define	mips_intern_icache_sync_range(v, s)				\
	(*mips_cache_ops.mco_intern_icache_sync_range)((v), (s))

#define	mips_intern_icache_sync_range_index(v, s)			\
	(*mips_cache_ops.mco_intern_icache_sync_range_index)((v), (s))

void	mips_config_cache(void);
void	mips_dcache_compute_align(void);

#include <mips/cache_mipsNN.h>

#endif /* _MIPS_CACHE_H_ */
