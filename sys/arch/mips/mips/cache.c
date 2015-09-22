/*	$NetBSD: cache.c,v 1.48.26.2 2015/09/22 12:05:47 skrll Exp $	*/

/*
 * Copyright 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe and Simon Burge for Wasabi Systems, Inc.
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
 * Copyright 2000, 2001
 * Broadcom Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and copied only
 * in accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * modified or unmodified copies of this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and
 *    retain this copyright notice and list of conditions as they appear in
 *    the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Broadcom Corporation.  The "Broadcom Corporation" name may not be
 *    used to endorse or promote products derived from this software
 *    without the prior written permission of Broadcom Corporation.
 *
 * 3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR IMPLIED
 *    WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
 *    NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL BROADCOM BE LIABLE
 *    FOR ANY DAMAGES WHATSOEVER, AND IN PARTICULAR, BROADCOM SHALL NOT BE
 *    LIABLE FOR DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *    OR OTHERWISE), EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cache.c,v 1.48.26.2 2015/09/22 12:05:47 skrll Exp $");

#include "opt_cputype.h"
#include "opt_mips_cache.h"

#include <sys/param.h>

#include <uvm/uvm_extern.h>

#include <mips/cache.h>
#include <mips/locore.h>

#ifdef MIPS1
#include <mips/cache_r3k.h>
#endif

#ifdef MIPS3_PLUS
#include <mips/cache_r4k.h>
#include <mips/cache_r5k.h>
#ifdef ENABLE_MIPS4_CACHE_R10K
#include <mips/cache_r10k.h>
#endif
#ifdef MIPS3_LOONGSON2
#include <mips/cache_ls2.h>
#endif
#endif

#if (MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0
#include <mips/mipsNN.h>		/* MIPS32/MIPS64 registers */
#include <mips/cache_mipsNN.h>
#ifdef MIPS64_OCTEON
#include <mips/cache_octeon.h>
#endif
#endif

struct mips_cache_info mips_cache_info;
struct mips_cache_ops mips_cache_ops;

#ifdef MIPS1
#ifdef ENABLE_MIPS_TX3900
#include <mips/cache_tx39.h>
void	tx3900_get_cache_config(void);
void	tx3920_get_cache_config(void);
void	tx39_cache_config_write_through(void);
#endif /* ENABLE_MIPS_TX3900 */
#endif /* MIPS1 */

#if defined(MIPS3) || defined(MIPS4)
void	mips3_get_cache_config(int);
#ifdef ENABLE_MIPS4_CACHE_R10K
void	mips4_get_cache_config(int);
#endif /* ENABLE_MIPS4_CACHE_R10K */
#endif /* MIPS3 || MIPS4 */

#if defined(MIPS1) || defined(MIPS3) || defined(MIPS4)
static void mips_config_cache_prehistoric(void);
static void mips_config_cache_emips(void);
#endif
#if (MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0
static void mips_config_cache_modern(uint32_t);
#endif

#if defined(MIPS1) || defined(MIPS3) || defined(MIPS4)
/* no-cache definition */
static void no_cache_op(void);
static void no_cache_op_range(vaddr_t va, vsize_t size);

/* no-cache implementation */
static void no_cache_op(void) {}
static void no_cache_op_range(vaddr_t va, vsize_t size) {}
#endif

/*
 * mips_dcache_compute_align:
 *
 *	Compute the D-cache alignment values.
 */
void
mips_dcache_compute_align(void)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	u_int align;

	align = mci->mci_pdcache_line_size;

	if (mci->mci_sdcache_line_size > align)
		align = mci->mci_sdcache_line_size;

	if (mci->mci_tcache_line_size > align)
		align = mci->mci_tcache_line_size;

	mci->mci_dcache_align = align;
	mci->mci_dcache_align_mask = align - 1;
}

/*
 * mips_config_cache:
 *
 *	Configure the cache for the system.
 *
 *	XXX DOES NOT HANDLE SPLIT SECONDARY CACHES.
 */
void
mips_config_cache(void)
{
#ifdef DIAGNOSTIC
	struct mips_cache_info * const mci = &mips_cache_info;
	struct mips_cache_ops * const mco = &mips_cache_ops;
#endif
	const mips_prid_t cpu_id = mips_options.mips_cpu_id;

#if defined(MIPS1) || defined(MIPS3) || defined(MIPS4)
	if (MIPS_PRID_CID(cpu_id) == MIPS_PRID_CID_PREHISTORIC)
		mips_config_cache_prehistoric();
	else if (MIPS_PRID_CID(cpu_id) == MIPS_PRID_CID_MICROSOFT)
		mips_config_cache_emips();
#endif
#if (MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0
	if (MIPS_PRID_CID(cpu_id) != MIPS_PRID_CID_PREHISTORIC)
		mips_config_cache_modern(cpu_id);
#endif

#ifdef DIAGNOSTIC
	/* Check that all cache ops are set up. */
	if (mci->mci_picache_size || 1) {	/* XXX- must have primary Icache */
		if (!mco->mco_icache_sync_all)
			panic("no icache_sync_all cache op");
		if (!mco->mco_icache_sync_range)
			panic("no icache_sync_range cache op");
		if (!mco->mco_icache_sync_range_index)
			panic("no icache_sync_range_index cache op");
	}
	if (mci->mci_pdcache_size || 1) {	/* XXX- must have primary Dcache */
		if (!mco->mco_pdcache_wbinv_all)
			panic("no pdcache_wbinv_all");
		if (!mco->mco_pdcache_wbinv_range)
			panic("no pdcache_wbinv_range");
		if (!mco->mco_pdcache_wbinv_range_index)
			panic("no pdcache_wbinv_range_index");
		if (!mco->mco_pdcache_inv_range)
			panic("no pdcache_inv_range");
		if (!mco->mco_pdcache_wb_range)
			panic("no pdcache_wb_range");
	}
	if (mci->mci_sdcache_size) {
		if (!mco->mco_sdcache_wbinv_all)
			panic("no sdcache_wbinv_all");
		if (!mco->mco_sdcache_wbinv_range)
			panic("no sdcache_wbinv_range");
		if (!mco->mco_sdcache_wbinv_range_index)
			panic("no sdcache_wbinv_range_index");
		if (!mco->mco_sdcache_inv_range)
			panic("no sdcache_inv_range");
		if (!mco->mco_sdcache_wb_range)
			panic("no sdcache_wb_range");
	}
#endif /* DIAGNOSTIC */
}

#if defined(MIPS1) || defined(MIPS3) || defined(MIPS4)
/*
 *	XXX DOES NOT HANDLE SPLIT SECONDARY CACHES.
 */
void
mips_config_cache_prehistoric(void)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	struct mips_cache_ops * const mco = &mips_cache_ops;
	const mips_prid_t cpu_id = mips_options.mips_cpu_id;
#if defined(MIPS3) || defined(MIPS4)
	int csizebase = MIPS3_CONFIG_C_DEFBASE;
#endif

	KASSERT(PAGE_SIZE != 0);

	/*
	 * Configure primary caches.
	 */
	switch (MIPS_PRID_IMPL(cpu_id)) {
#ifdef MIPS1
	case MIPS_R2000:
	case MIPS_R3000:
		mci->mci_picache_size = r3k_picache_size();
		mci->mci_pdcache_size = r3k_pdcache_size();

		mci->mci_picache_line_size = 4;
		mci->mci_pdcache_line_size = 4;

		mci->mci_picache_ways = 1;
		mci->mci_pdcache_ways = 1;

		mci->mci_pdcache_write_through = true;

		mco->mco_icache_sync_all =
		    r3k_icache_sync_all;
		mco->mco_icache_sync_range =
		    r3k_icache_sync_range;
		mco->mco_icache_sync_range_index =
		    mco->mco_icache_sync_range;

		mco->mco_pdcache_wbinv_all =
		    r3k_pdcache_wbinv_all;
		mco->mco_pdcache_wbinv_range =
		    r3k_pdcache_inv_range;
		mco->mco_pdcache_wbinv_range_index =
		    mco->mco_pdcache_wbinv_range;
		mco->mco_pdcache_inv_range =
		    r3k_pdcache_inv_range;
		mco->mco_pdcache_wb_range =
		    r3k_pdcache_wb_range;

		uvmexp.ncolors = atop(mci->mci_pdcache_size);
		break;

#ifdef ENABLE_MIPS_TX3900
	case MIPS_TX3900:
		switch (MIPS_PRID_REV_MAJ(cpu_id)) {
		case 1:		/* TX3912 */
			mci->mci_picache_ways = 1;
			mci->mci_picache_line_size = 16;
			mci->mci_pdcache_line_size = 4;

			tx3900_get_cache_config();

			mci->mci_pdcache_write_through = true;

			mco->mco_icache_sync_all =
			    tx3900_icache_sync_all_16;
			mco->mco_icache_sync_range =
			    tx3900_icache_sync_range_16;
			mco->mco_icache_sync_range_index =
			    tx3900_icache_sync_range_16;

			mco->mco_pdcache_wbinv_all =
			    tx3900_pdcache_wbinv_all_4;
			mco->mco_pdcache_wbinv_range =
			    tx3900_pdcache_inv_range_4;
			mco->mco_pdcache_wbinv_range_index =
			    tx3900_pdcache_inv_range_4;
			mco->mco_pdcache_inv_range =
			    tx3900_pdcache_inv_range_4;
			mco->mco_pdcache_wb_range =
			    tx3900_pdcache_wb_range_4;
			break;

		case 3:		/* TX3922 */
			mci->mci_picache_ways = 2;
			mci->mci_picache_line_size = 16;
			mci->mci_pdcache_line_size = 16;

			tx3920_get_cache_config();

			mco->mco_icache_sync_all =
			    mci->mci_pdcache_write_through ?
			    tx3900_icache_sync_all_16 :
			    tx3920_icache_sync_all_16wb;
			mco->mco_icache_sync_range =
			    mci->mci_pdcache_write_through ?
			    tx3920_icache_sync_range_16wt :
			    tx3920_icache_sync_range_16wb;
			mco->mco_icache_sync_range_index =
			    mco->mco_icache_sync_range;

			mco->mco_pdcache_wbinv_all =
			    mci->mci_pdcache_write_through ?
			    tx3920_pdcache_wbinv_all_16wt :
			    tx3920_pdcache_wbinv_all_16wb;
			mco->mco_pdcache_wbinv_range =
			    mci->mci_pdcache_write_through ?
			    tx3920_pdcache_inv_range_16 :
			    tx3920_pdcache_wbinv_range_16wb;
			mco->mco_pdcache_wbinv_range_index =
			    mco->mco_pdcache_wbinv_range;
			mco->mco_pdcache_inv_range =
			    tx3920_pdcache_inv_range_16;
			mco->mco_pdcache_wb_range =
			    mci->mci_pdcache_write_through ?
			    tx3920_pdcache_wb_range_16wt :
			    tx3920_pdcache_wb_range_16wb;
			break;

		default:
			panic("mips_config_cache: unsupported TX3900");
		}

		mci->mci_pdcache_ways = 2;
		tx3900_get_cache_config();
		/* change to write-through mode */
		tx39_cache_config_write_through();

		uvmexp.ncolors = atop(mci->mci_pdcache_size) / mci->mci_pdcache_ways;
		break;
#endif /* ENABLE_MIPS_TX3900 */
#endif /* MIPS1 */

#if defined(MIPS3) || defined(MIPS4)
	case MIPS_R4100:
		if ((mips3_cp0_config_read() & MIPS3_CONFIG_CS) != 0)
			csizebase = MIPS3_CONFIG_C_4100BASE;

		/*
		 * R4100 (NEC VR series) revision number means:
		 *
		 *		MIPS_PRID_REV_MAJ	MIPS_PRID_REV_MIN
		 * VR4102	4			?
		 * VR4111	5			?
		 * VR4181	5			?
		 * VR4121	6			?
		 * VR4122	7			0 or 1
		 * VR4181A	7			3 <
		 * VR4131	8			?
		 */
		/* Vr4131 has R4600 style 2-way set-associative cache */
		if (MIPS_PRID_REV_MAJ(cpu_id) == 8)
			goto primary_cache_is_2way;
		/* FALLTHROUGH */

	case MIPS_R4000:
	case MIPS_R4300:
		mci->mci_picache_ways = 1;
		mci->mci_pdcache_ways = 1;
		mci->mci_sdcache_ways = 1;

		mips3_get_cache_config(csizebase);

		if (mci->mci_picache_size > PAGE_SIZE ||
		    mci->mci_pdcache_size > PAGE_SIZE)
			/* no VCE support if there is no L2 cache */
			mci->mci_cache_virtual_alias = true;

		switch (mci->mci_picache_line_size) {
		case 16:
			mco->mco_icache_sync_all =
			    r4k_icache_sync_all_16;
			mco->mco_icache_sync_range =
			    r4k_icache_sync_range_16;
			mco->mco_icache_sync_range_index =
			    r4k_icache_sync_range_index_16;
			break;

		case 32:
			mco->mco_icache_sync_all =
			    r4k_icache_sync_all_32;
			mco->mco_icache_sync_range =
			    r4k_icache_sync_range_32;
			mco->mco_icache_sync_range_index =
			    r4k_icache_sync_range_index_32;
			break;

		default:
			panic("r4k picache line size %d",
			    mci->mci_picache_line_size);
		}

		switch (mci->mci_pdcache_line_size) {
		case 16:
			mco->mco_pdcache_wbinv_all =
			    r4k_pdcache_wbinv_all_16;
			mco->mco_pdcache_wbinv_range =
			    r4k_pdcache_wbinv_range_16;
			mco->mco_pdcache_wbinv_range_index =
			    r4k_pdcache_wbinv_range_index_16;
			mco->mco_pdcache_inv_range =
			    r4k_pdcache_inv_range_16;
			mco->mco_pdcache_wb_range =
			    r4k_pdcache_wb_range_16;
			break;

		case 32:
			mco->mco_pdcache_wbinv_all =
			    r4k_pdcache_wbinv_all_32;
			mco->mco_pdcache_wbinv_range =
			    r4k_pdcache_wbinv_range_32;
			mco->mco_pdcache_wbinv_range_index =
			    r4k_pdcache_wbinv_range_index_32;
			mco->mco_pdcache_inv_range =
			    r4k_pdcache_inv_range_32;
			mco->mco_pdcache_wb_range =
			    r4k_pdcache_wb_range_32;
			break;

		default:
			panic("r4k pdcache line size %d",
			    mci->mci_pdcache_line_size);
		}

		/* Virtually-indexed cache; no use for colors. */
		break;

	case MIPS_R4600:
#ifdef ENABLE_MIPS_R4700
	case MIPS_R4700:
#endif
#ifndef ENABLE_MIPS_R3NKK
	case MIPS_R5000:
#endif
	case MIPS_RM5200:
primary_cache_is_2way:
		mci->mci_picache_ways = 2;
		mci->mci_pdcache_ways = 2;

		mips3_get_cache_config(csizebase);

		if ((mci->mci_picache_size / mci->mci_picache_ways) > PAGE_SIZE ||
		    (mci->mci_pdcache_size / mci->mci_pdcache_ways) > PAGE_SIZE)
			mci->mci_cache_virtual_alias = true;

		switch (mci->mci_picache_line_size) {
		case 32:
			mco->mco_icache_sync_all =
			    r5k_icache_sync_all_32;
			mco->mco_icache_sync_range =
			    r5k_icache_sync_range_32;
			mco->mco_icache_sync_range_index =
			    r5k_icache_sync_range_index_32;
			break;

		default:
			panic("r5k picache line size %d",
			    mci->mci_picache_line_size);
		}

		switch (mci->mci_pdcache_line_size) {
		case 16:
			mco->mco_pdcache_wbinv_all =
			    r5k_pdcache_wbinv_all_16;
			mco->mco_pdcache_wbinv_range =
			    r5k_pdcache_wbinv_range_16;
			mco->mco_pdcache_wbinv_range_index =
			    r5k_pdcache_wbinv_range_index_16;
			mco->mco_pdcache_inv_range =
			    r5k_pdcache_inv_range_16;
			mco->mco_pdcache_wb_range =
			    r5k_pdcache_wb_range_16;
			break;

		case 32:
			mco->mco_pdcache_wbinv_all =
			    r5k_pdcache_wbinv_all_32;
			mco->mco_pdcache_wbinv_range =
			    r5k_pdcache_wbinv_range_32;
			mco->mco_pdcache_wbinv_range_index =
			    r5k_pdcache_wbinv_range_index_32;
			mco->mco_pdcache_inv_range =
			    r5k_pdcache_inv_range_32;
			mco->mco_pdcache_wb_range =
			    r5k_pdcache_wb_range_32;
			break;

		default:
			panic("r5k pdcache line size %d",
			    mci->mci_pdcache_line_size);
		}

		/*
		 * Deal with R4600 chip bugs.
		 */
		if (MIPS_PRID_IMPL(cpu_id) == MIPS_R4600 &&
		    MIPS_PRID_REV_MAJ(cpu_id) == 1) {
			KASSERT(mci->mci_pdcache_line_size == 32);
			mco->mco_pdcache_wbinv_range =
			    r4600v1_pdcache_wbinv_range_32;
			mco->mco_pdcache_inv_range =
			    r4600v1_pdcache_inv_range_32;
			mco->mco_pdcache_wb_range =
			    r4600v1_pdcache_wb_range_32;
		} else if (MIPS_PRID_IMPL(cpu_id) == MIPS_R4600 &&
			   MIPS_PRID_REV_MAJ(cpu_id) == 2) {
			KASSERT(mci->mci_pdcache_line_size == 32);
			mco->mco_pdcache_wbinv_range =
			    r4600v2_pdcache_wbinv_range_32;
			mco->mco_pdcache_inv_range =
			    r4600v2_pdcache_inv_range_32;
			mco->mco_pdcache_wb_range =
			    r4600v2_pdcache_wb_range_32;
		}

		/*
		 * Deal with VR4131 chip bugs.
		 */
		if (MIPS_PRID_IMPL(cpu_id) == MIPS_R4100 &&
		    MIPS_PRID_REV_MAJ(cpu_id) == 8) {
			KASSERT(mci->mci_pdcache_line_size == 16);
			mco->mco_pdcache_wbinv_range =
			    vr4131v1_pdcache_wbinv_range_16;
		}

		/* Virtually-indexed cache; no use for colors. */
		break;
#ifdef ENABLE_MIPS4_CACHE_R10K
	case MIPS_R10000:
	case MIPS_R12000:
	case MIPS_R14000:
		mci->mci_picache_ways = 2;
		mci->mci_pdcache_ways = 2;
		mci->mci_sdcache_ways = 2;

		mips4_get_cache_config(csizebase);

		/* VCE is handled by hardware */

		mco->mco_icache_sync_all =
		    r10k_icache_sync_all;
		mco->mco_icache_sync_range =
		    r10k_icache_sync_range;
		mco->mco_icache_sync_range_index =
		    r10k_icache_sync_range_index;
		mco->mco_pdcache_wbinv_all =
		    r10k_pdcache_wbinv_all;
		mco->mco_pdcache_wbinv_range =
		    r10k_pdcache_wbinv_range;
		mco->mco_pdcache_wbinv_range_index =
		    r10k_pdcache_wbinv_range_index;
		mco->mco_pdcache_inv_range =
		    r10k_pdcache_inv_range;
		mco->mco_pdcache_wb_range =
		    r10k_pdcache_wb_range;
		break;
#endif /* ENABLE_MIPS4_CACHE_R10K */
#ifdef MIPS3_LOONGSON2
	case MIPS_LOONGSON2:
		mci->mci_picache_ways = 4;
		mci->mci_pdcache_ways = 4;

		mips3_get_cache_config(csizebase);

		mci->mci_sdcache_line_size = 32;	/* don't trust config reg */

		if (mci->mci_picache_size / mci->mci_picache_ways > PAGE_SIZE ||
		    mci->mci_pdcache_size / mci->mci_pdcache_ways > PAGE_SIZE)
			mci->mci_cache_virtual_alias = true;

		mco->mco_icache_sync_all =
		    ls2_icache_sync_all;
		mco->mco_icache_sync_range =
		    ls2_icache_sync_range;
		mco->mco_icache_sync_range_index =
		    ls2_icache_sync_range_index;

		mco->mco_pdcache_wbinv_all =
		    ls2_pdcache_wbinv_all;
		mco->mco_pdcache_wbinv_range =
		    ls2_pdcache_wbinv_range;
		mco->mco_pdcache_wbinv_range_index =
		    ls2_pdcache_wbinv_range_index;
		mco->mco_pdcache_inv_range =
		    ls2_pdcache_inv_range;
		mco->mco_pdcache_wb_range =
		    ls2_pdcache_wb_range;

		/*
		 * For current version chips, [the] operating system is
		 * obliged to eliminate the potential for virtual aliasing.
		 */
		uvmexp.ncolors = mci->mci_pdcache_ways;
		break;
#endif
#endif /* MIPS3 || MIPS4 */
	default:
		panic("can't handle primary cache on impl 0x%x",
		    MIPS_PRID_IMPL(cpu_id));
	}

	/*
	 * Compute the "way mask" for each cache.
	 */
	if (mci->mci_picache_size) {
		KASSERT(mci->mci_picache_ways != 0);
		mci->mci_picache_way_size = (mci->mci_picache_size / mci->mci_picache_ways);
		mci->mci_picache_way_mask = mci->mci_picache_way_size - 1;
	}
	if (mci->mci_pdcache_size) {
		KASSERT(mci->mci_pdcache_ways != 0);
		mci->mci_pdcache_way_size = (mci->mci_pdcache_size / mci->mci_pdcache_ways);
		mci->mci_pdcache_way_mask = mci->mci_pdcache_way_size - 1;
	}

	mips_dcache_compute_align();

	if (mci->mci_sdcache_line_size == 0)
		return;

	/*
	 * Configure the secondary cache.
	 */
	switch (MIPS_PRID_IMPL(cpu_id)) {
#if defined(MIPS3) || defined(MIPS4)
	case MIPS_R4000:
		/*
		 * R4000/R4400 detects virtual alias by VCE as if
		 * its primary cache size were 32KB, because it always
		 * compares 3 bits of vaddr[14:12] which causes
		 * primary cache miss and PIdx[2:0] in the secondary
		 * cache tag regardless of its primary cache size.
		 * i.e. VCE could happen even if there is no actual
		 * virtual alias on its 8KB or 16KB primary cache
		 * which has only 1 or 2 bit valid PIdx in 4KB page.
		 * Actual primary cache size is ignored wrt VCE
		 * and virtual aliases are resolved by the VCE hander,
		 * but it's still worth to avoid unnecessary VCE by
		 * setting alias mask and prefer mask to 32K, though
		 * some other possible aliases (maybe caused by KSEG0
		 * accesses which can't be managed by PMAP_PREFER(9))
		 * will still be resolved by the VCED/VCEI handler.
		 */
		mci->mci_cache_alias_mask =
		    (MIPS3_MAX_PCACHE_SIZE - 1) & ~PAGE_MASK;	/* va[14:12] */
		mci->mci_cache_prefer_mask = MIPS3_MAX_PCACHE_SIZE - 1;

		mci->mci_cache_virtual_alias = false;
		/* FALLTHROUGH */
	case MIPS_R4600:
#ifdef ENABLE_MIPS_R4700
	case MIPS_R4700:
#endif
		switch (mci->mci_sdcache_ways) {
		case 1:
			switch (mci->mci_sdcache_line_size) {
			case 32:
				mco->mco_sdcache_wbinv_all =
				    r4k_sdcache_wbinv_all_32;
				mco->mco_sdcache_wbinv_range =
				    r4k_sdcache_wbinv_range_32;
				mco->mco_sdcache_wbinv_range_index =
				    r4k_sdcache_wbinv_range_index_32;
				mco->mco_sdcache_inv_range =
				    r4k_sdcache_inv_range_32;
				mco->mco_sdcache_wb_range =
				    r4k_sdcache_wb_range_32;
				break;

			case 16:
			case 64:
				mco->mco_sdcache_wbinv_all =
				    r4k_sdcache_wbinv_all_generic;
				mco->mco_sdcache_wbinv_range =
				    r4k_sdcache_wbinv_range_generic;
				mco->mco_sdcache_wbinv_range_index =
				    r4k_sdcache_wbinv_range_index_generic;
				mco->mco_sdcache_inv_range =
				    r4k_sdcache_inv_range_generic;
				mco->mco_sdcache_wb_range =
				    r4k_sdcache_wb_range_generic;
				break;

			case 128:
				mco->mco_sdcache_wbinv_all =
				    r4k_sdcache_wbinv_all_128;
				mco->mco_sdcache_wbinv_range =
				    r4k_sdcache_wbinv_range_128;
				mco->mco_sdcache_wbinv_range_index =
				    r4k_sdcache_wbinv_range_index_128;
				mco->mco_sdcache_inv_range =
				    r4k_sdcache_inv_range_128;
				mco->mco_sdcache_wb_range =
				    r4k_sdcache_wb_range_128;
				break;

			default:
				panic("r4k sdcache %d way line size %d",
				    mci->mci_sdcache_ways, mci->mci_sdcache_line_size);
			}
			break;

		default:
			panic("r4k sdcache %d way line size %d",
			    mci->mci_sdcache_ways, mci->mci_sdcache_line_size);
		}
		break;
#ifndef ENABLE_MIPS_R3NKK
	case MIPS_R5000:
#endif
	case MIPS_RM5200:
		mci->mci_sdcache_write_through = true;
		mco->mco_sdcache_wbinv_all =
		    r5k_sdcache_wbinv_all;
		mco->mco_sdcache_wbinv_range =
		    r5k_sdcache_wbinv_range;
		mco->mco_sdcache_wbinv_range_index =
		    r5k_sdcache_wbinv_range_index;
		mco->mco_sdcache_inv_range =
		    r5k_sdcache_wbinv_range;
		mco->mco_sdcache_wb_range =
		    r5k_sdcache_wb_range;
		break;
#ifdef ENABLE_MIPS4_CACHE_R10K
	case MIPS_R10000:
	case MIPS_R12000:
	case MIPS_R14000:
		mco->mco_sdcache_wbinv_all =
		    r10k_sdcache_wbinv_all;
		mco->mco_sdcache_wbinv_range =
		    r10k_sdcache_wbinv_range;
		mco->mco_sdcache_wbinv_range_index =
		    r10k_sdcache_wbinv_range_index;
		mco->mco_sdcache_inv_range =
		    r10k_sdcache_inv_range;
		mco->mco_sdcache_wb_range =
		    r10k_sdcache_wb_range;
		break;
#endif /* ENABLE_MIPS4_CACHE_R10K */
#ifdef MIPS3_LOONGSON2
	case MIPS_LOONGSON2:
		mci->mci_sdcache_ways = 4;
		mci->mci_sdcache_size = 512*1024;
		mci->mci_scache_unified = 1;

		mco->mco_sdcache_wbinv_all =
		    ls2_sdcache_wbinv_all;
		mco->mco_sdcache_wbinv_range =
		    ls2_sdcache_wbinv_range;
		mco->mco_sdcache_wbinv_range_index =
		    ls2_sdcache_wbinv_range_index;
		mco->mco_sdcache_inv_range =
		    ls2_sdcache_inv_range;
		mco->mco_sdcache_wb_range =
		    ls2_sdcache_wb_range;

		/*
		 * The secondary cache is physically indexed and tagged
		 */
		break;
#endif
#endif /* MIPS3 || MIPS4 */

	default:
		panic("can't handle secondary cache on impl 0x%x",
		    MIPS_PRID_IMPL(cpu_id));
	}

	/*
	 * Compute the "way mask" for each secondary cache.
	 */
	if (mci->mci_sdcache_size) {
		KASSERT(mci->mci_sdcache_ways != 0);
		mci->mci_sdcache_way_size = (mci->mci_sdcache_size / mci->mci_sdcache_ways);
		mci->mci_sdcache_way_mask = mci->mci_sdcache_way_size - 1;
	}

	mips_dcache_compute_align();
}

#if defined(MIPS1) || defined(MIPS3) || defined(MIPS4)
void
mips_config_cache_emips(void)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	struct mips_cache_ops * const mco = &mips_cache_ops;
	const mips_prid_t cpu_id = mips_options.mips_cpu_id;
	KASSERT(PAGE_SIZE != 0);

	/*
	 * Configure primary caches.
	 */
	switch (MIPS_PRID_IMPL(cpu_id)) {
	case MIPS_eMIPS:
		mci->mci_picache_size = 0;
		mci->mci_pdcache_size = 0;

		mci->mci_picache_line_size = 4;
		mci->mci_pdcache_line_size = 4;

		mci->mci_picache_ways = 1;
		mci->mci_pdcache_ways = 1;

		mci->mci_pdcache_write_through = 1;

		mco->mco_icache_sync_all = no_cache_op;
		mco->mco_icache_sync_range = no_cache_op_range;
		mco->mco_icache_sync_range_index = mco->mco_icache_sync_range;

		mco->mco_pdcache_wbinv_all = no_cache_op;
		mco->mco_pdcache_wbinv_range = no_cache_op_range;
		mco->mco_pdcache_wbinv_range_index =
		    mco->mco_pdcache_wbinv_range;
		mco->mco_pdcache_inv_range = no_cache_op_range;
		mco->mco_pdcache_wb_range = no_cache_op_range;

		uvmexp.ncolors = 1;
		break;

	default:
		panic("%s: unsupported eMIPS", __func__);
	}
}
#endif

#ifdef MIPS1
#ifdef ENABLE_MIPS_TX3900
/*
 * tx3900_get_cache_config:
 *
 *	Fetch cache size information for the TX3900.
 */
void
tx3900_get_cache_config(void)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	uint32_t config;

	config = tx3900_cp0_config_read();

	mci->mci_picache_size = R3900_C_SIZE_MIN <<
	    ((config & R3900_CONFIG_ICS_MASK) >> R3900_CONFIG_ICS_SHIFT);

	mci->mci_pdcache_size = R3900_C_SIZE_MIN <<
	    ((config & R3900_CONFIG_DCS_MASK) >> R3900_CONFIG_DCS_SHIFT);
}

/*
 * tx3920_get_cache_config:
 *
 *	Fetch cache size information for the TX3920.
 */
void
tx3920_get_cache_config(void)
{
	struct mips_cache_info * const mci = &mips_cache_info;

	/* Size is the same as TX3900. */
	tx3900_get_cache_config();

	/* Now determine write-through/write-back mode. */
	if ((tx3900_cp0_config_read() & R3900_CONFIG_WBON) == 0)
		mci->mci_pdcache_write_through = true;
}

/*
 * tx39_cache_config_write_through:
 *
 *	TX3922 write-through D-cache mode.
 *	for TX3912, no meaning. (no write-back mode)
 */
void
tx39_cache_config_write_through(void)
{
	u_int32_t r;

	mips_dcache_wbinv_all();

	__asm volatile("mfc0 %0, $3" : "=r"(r));
	r &= 0xffffdfff;
	__asm volatile("mtc0 %0, $3" : : "r"(r));
}

#endif /* ENABLE_MIPS_TX3900 */
#endif /* MIPS1 */

#if defined(MIPS3) || defined(MIPS4)
/*
 * mips3_get_cache_config:
 *
 *	Fetch the cache config information for a MIPS-3 or MIPS-4
 *	processor (virtually-indexed cache).
 *
 *	NOTE: Fetching the size of the secondary cache is something
 *	that platform specific code has to do.  We'd appreciate it
 *	if they initialized the size before now.
 *
 *	ALSO NOTE: The number of ways in the cache must already be
 *	initialized.
 */
void
mips3_get_cache_config(int csizebase)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	const mips_prid_t cpu_id = mips_options.mips_cpu_id;
	bool has_sdcache_enable = false;
	uint32_t config = mips3_cp0_config_read();

	mci->mci_picache_size = MIPS3_CONFIG_CACHE_SIZE(config,
	    MIPS3_CONFIG_IC_MASK, csizebase, MIPS3_CONFIG_IC_SHIFT);
	mci->mci_picache_line_size = MIPS3_CONFIG_CACHE_L1_LSIZE(config,
	    MIPS3_CONFIG_IB);

	mci->mci_pdcache_size = MIPS3_CONFIG_CACHE_SIZE(config,
	    MIPS3_CONFIG_DC_MASK, csizebase, MIPS3_CONFIG_DC_SHIFT);
	mci->mci_pdcache_line_size = MIPS3_CONFIG_CACHE_L1_LSIZE(config,
	    MIPS3_CONFIG_DB);

	mci->mci_cache_alias_mask =
	    ((mci->mci_pdcache_size / mci->mci_pdcache_ways) - 1) & ~PAGE_MASK;
	mci->mci_cache_prefer_mask =
	    max(mci->mci_pdcache_size, mci->mci_picache_size) - 1;
	uvmexp.ncolors = (mci->mci_cache_alias_mask >> PAGE_SHIFT) + 1;

	switch(MIPS_PRID_IMPL(cpu_id)) {
#ifndef ENABLE_MIPS_R3NKK
	case MIPS_R5000:
#endif
	case MIPS_RM5200:
		has_sdcache_enable = true;
		break;
	}

	/*
 	 * If CPU has a software-enabled L2 cache, check both if it's
	 * present and if it's enabled before making assumptions the
	 * L2 is usable.  If the L2 is disabled, we treat it the same
	 * as if there were no L2 cache.
	 */
	if ((config & MIPS3_CONFIG_SC) == 0) {
		if (has_sdcache_enable == 0 ||
		    (has_sdcache_enable && (config & MIPS3_CONFIG_SE))) {
			mci->mci_sdcache_line_size =
				MIPS3_CONFIG_CACHE_L2_LSIZE(config);
			if ((config & MIPS3_CONFIG_SS) == 0)
				mci->mci_scache_unified = true;
		} else {
#ifdef CACHE_DEBUG
			printf("External cache detected, but is disabled -- WILL NOT ENABLE!\n");
#endif	/* CACHE_DEBUG */
		}
	}
}

#ifdef ENABLE_MIPS4_CACHE_R10K
void
mips4_get_cache_config(int csizebase)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	uint32_t config = mips3_cp0_config_read();

	mci->mci_picache_size = MIPS4_CONFIG_CACHE_SIZE(config,
	    MIPS4_CONFIG_IC_MASK, csizebase, MIPS4_CONFIG_IC_SHIFT);
	mci->mci_picache_line_size = 64;	/* 64 Byte */

	mci->mci_pdcache_size = MIPS4_CONFIG_CACHE_SIZE(config,
	    MIPS4_CONFIG_DC_MASK, csizebase, MIPS4_CONFIG_DC_SHIFT);
	mci->mci_pdcache_line_size = 32;	/* 32 Byte */

	mci->mci_cache_alias_mask =
	    ((mci->mci_pdcache_size / mci->mci_pdcache_ways) - 1) & ~PAGE_MASK;
	mci->mci_cache_prefer_mask =
	    max(mci->mci_pdcache_size, mci->mci_picache_size) - 1;
}
#endif /* ENABLE_MIPS4_CACHE_R10K */
#endif /* MIPS3 || MIPS4 */
#endif /* MIPS1 || MIPS3 || MIPS4 */

#if (MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0

static void cache_noop(void) __unused;
static void cache_noop(void) {}

static void
mips_config_cache_modern(uint32_t cpu_id)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	struct mips_cache_ops * const mco = &mips_cache_ops;
	/* MIPS32/MIPS64, use coprocessor 0 config registers */
	uint32_t cfg, cfg1;

	cfg = mips3_cp0_config_read();
	cfg1 = mipsNN_cp0_config1_read();

#ifdef MIPS_DISABLE_L1_CACHE
	cfg1 &= ~MIPSNN_CFG1_IL_MASK;
	cfg1 &= ~MIPSNN_CFG1_DL_MASK;
	mipsNN_cp0_config1_write(cfg1);
#endif

	/* figure out Dcache params. */
	switch (MIPSNN_GET(CFG1_DL, cfg1)) {
	case MIPSNN_CFG1_DL_NONE:
#ifdef MIPS64_OCTEON
		mci->mci_pdcache_line_size = 128;
		mci->mci_pdcache_way_size = 256;
		mci->mci_pdcache_ways = 64;

		mci->mci_pdcache_size =
		    mci->mci_pdcache_way_size * mci->mci_pdcache_ways;
		mci->mci_pdcache_way_mask = mci->mci_pdcache_way_size - 1;
		uvmexp.ncolors = atop(mci->mci_pdcache_size) / mci->mci_pdcache_ways;
#else
		mci->mci_pdcache_line_size = mci->mci_pdcache_way_size =
		    mci->mci_pdcache_ways = 0;
#endif
		break;
	case MIPSNN_CFG1_DL_RSVD:
		panic("reserved MIPS32/64 Dcache line size");
		break;
	default:
		if (MIPSNN_GET(CFG1_DS, cfg1) == MIPSNN_CFG1_DS_RSVD)
			panic("reserved MIPS32/64 Dcache sets per way");
		mci->mci_pdcache_line_size = MIPSNN_CFG1_DL(cfg1);
		mci->mci_pdcache_way_size =
		    mci->mci_pdcache_line_size * MIPSNN_CFG1_DS(cfg1);
		mci->mci_pdcache_ways = MIPSNN_CFG1_DA(cfg1) + 1;

		/*
		 * Compute the total size and "way mask" for the
		 * primary Dcache.
		 */
		mci->mci_pdcache_size =
		    mci->mci_pdcache_way_size * mci->mci_pdcache_ways;
		mci->mci_pdcache_way_mask = mci->mci_pdcache_way_size - 1;
		uvmexp.ncolors = atop(mci->mci_pdcache_size) / mci->mci_pdcache_ways;
		break;
	}

	/* figure out Icache params. */
	switch (MIPSNN_GET(CFG1_IL, cfg1)) {
	case MIPSNN_CFG1_IL_NONE:
		mci->mci_picache_line_size = mci->mci_picache_way_size =
		    mci->mci_picache_ways = 0;
		break;
	case MIPSNN_CFG1_IL_RSVD:
		panic("reserved MIPS32/64 Icache line size");
		break;
	default:
		if (MIPSNN_GET(CFG1_IS, cfg1) == MIPSNN_CFG1_IS_RSVD)
			panic("reserved MIPS32/64 Icache sets per way");
		mci->mci_picache_line_size = MIPSNN_CFG1_IL(cfg1);
		mci->mci_picache_way_size =
		    mci->mci_picache_line_size * MIPSNN_CFG1_IS(cfg1);
		mci->mci_picache_ways = MIPSNN_CFG1_IA(cfg1) + 1;

		/*
		 * Compute the total size and "way mask" for the
		 * primary Dcache.
		 */
		mci->mci_picache_size =
		    mci->mci_picache_way_size * mci->mci_picache_ways;
		mci->mci_picache_way_mask = mci->mci_picache_way_size - 1;
		break;
	}

#define CACHE_DEBUG
#ifdef CACHE_DEBUG
	printf("MIPS32/64 params: cpu arch: %d\n", mips_options.mips_cpu_arch);
	printf("MIPS32/64 params: TLB entries: %d\n", mips_options.mips_num_tlb_entries);
	if (mci->mci_picache_line_size == 0)
		printf("MIPS32/64 params: no Icache\n");
	else {
		printf("MIPS32/64 params: Icache: line = %d, total = %d, "
		    "ways = %d\n", mci->mci_picache_line_size,
		    mci->mci_picache_way_size * mci->mci_picache_ways,
		    mci->mci_picache_ways);
		printf("\t\t sets = %d\n", (mci->mci_picache_way_size *
		    mci->mci_picache_ways / mci->mci_picache_line_size) /
		    mci->mci_picache_ways);
	}
	if (mci->mci_pdcache_line_size == 0)
		printf("MIPS32/64 params: no Dcache\n");
	else {
		printf("MIPS32/64 params: Dcache: line = %d, total = %d, "
		    "ways = %d\n", mci->mci_pdcache_line_size,
		    mci->mci_pdcache_way_size * mci->mci_pdcache_ways,
		    mci->mci_pdcache_ways);
		printf("\t\t sets = %d\n", (mci->mci_pdcache_way_size *
		    mci->mci_pdcache_ways / mci->mci_pdcache_line_size) /
		    mci->mci_pdcache_ways);
	}
#endif /* CACHE_DEBUG */

	switch (mci->mci_picache_line_size) {
	case 16:
		mco->mco_icache_sync_all = mipsNN_icache_sync_all_16;
		mco->mco_icache_sync_range =
		    mipsNN_icache_sync_range_16;
		mco->mco_icache_sync_range_index =
		    mipsNN_icache_sync_range_index_16;
		break;
	case 32:
		mco->mco_icache_sync_all = mipsNN_icache_sync_all_32;
		mco->mco_icache_sync_range =
		    mipsNN_icache_sync_range_32;
		mco->mco_icache_sync_range_index =
		    mipsNN_icache_sync_range_index_32;
		break;
#ifdef MIPS_DISABLE_L1_CACHE
	case 0:
		mco->mco_icache_sync_all = cache_noop;
		mco->mco_icache_sync_range =
		    (void (*)(vaddr_t, vsize_t))cache_noop;
		mco->mco_icache_sync_range_index =
		    (void (*)(vaddr_t, vsize_t))cache_noop;
		break;
#endif
#ifdef MIPS64_OCTEON
	case 128:
		mco->mco_icache_sync_all = octeon_icache_sync_all;
		mco->mco_icache_sync_range = octeon_icache_sync_range;
		mco->mco_icache_sync_range_index = octeon_icache_sync_range_index;
		break;
#endif
	default:
		panic("no Icache ops for %d byte lines",
		    mci->mci_picache_line_size);
	}

	switch (mci->mci_pdcache_line_size) {
	case 16:
		mco->mco_pdcache_wbinv_all =
		    mco->mco_intern_pdcache_wbinv_all =
		    mipsNN_pdcache_wbinv_all_16;
		mco->mco_pdcache_wbinv_range =
		    mipsNN_pdcache_wbinv_range_16;
		mco->mco_pdcache_wbinv_range_index =
		    mco->mco_intern_pdcache_wbinv_range_index =
		    mipsNN_pdcache_wbinv_range_index_16;
		mco->mco_pdcache_inv_range =
		    mipsNN_pdcache_inv_range_16;
		mco->mco_pdcache_wb_range =
		    mco->mco_intern_pdcache_wb_range =
		    mipsNN_pdcache_wb_range_16;
		break;
	case 32:
		mco->mco_pdcache_wbinv_all =
		    mco->mco_intern_pdcache_wbinv_all =
		    mipsNN_pdcache_wbinv_all_32;
		mco->mco_pdcache_wbinv_range =
		    mipsNN_pdcache_wbinv_range_32;
		mco->mco_pdcache_wbinv_range_index =
		    mco->mco_intern_pdcache_wbinv_range_index =
		    mipsNN_pdcache_wbinv_range_index_32;
		mco->mco_pdcache_inv_range =
		    mipsNN_pdcache_inv_range_32;
		mco->mco_pdcache_wb_range =
		    mco->mco_intern_pdcache_wb_range =
		    mipsNN_pdcache_wb_range_32;
		break;
#ifdef MIPS64_OCTEON
	case 128:
		mco->mco_pdcache_wbinv_all =
		    mco->mco_intern_pdcache_wbinv_all =
		    octeon_pdcache_inv_all;
		mco->mco_pdcache_wbinv_range =
		    octeon_pdcache_inv_range;
		mco->mco_pdcache_wbinv_range_index =
		    mco->mco_intern_pdcache_wbinv_range_index =
		    octeon_pdcache_inv_range_index;
		mco->mco_pdcache_inv_range =
		    (void (*)(vaddr_t, vsize_t))cache_noop;
		mco->mco_pdcache_wb_range =
		    mco->mco_intern_pdcache_wb_range =
		    octeon_pdcache_inv_range;
		break;
#endif
#ifdef MIPS_DISABLE_L1_CACHE
	case 0:
		mco->mco_pdcache_wbinv_all = cache_noop;
		mco->mco_intern_pdcache_wbinv_all = cache_noop;
		mco->mco_pdcache_wbinv_range =
		    (void (*)(vaddr_t, vsize_t))cache_noop;
		mco->mco_pdcache_wbinv_range_index =
		    (void (*)(vaddr_t, vsize_t))cache_noop;
		mco->mco_intern_pdcache_wbinv_range_index =
		    (void (*)(vaddr_t, vsize_t))cache_noop;
		mco->mco_pdcache_inv_range =
		    (void (*)(vaddr_t, vsize_t))cache_noop;
		mco->mco_pdcache_wb_range =
		    (void (*)(vaddr_t, vsize_t))cache_noop;
		mco->mco_intern_pdcache_wb_range =
		    (void (*)(vaddr_t, vsize_t))cache_noop;
		break;
#endif
	default:
		panic("no Dcache ops for %d byte lines",
		    mci->mci_pdcache_line_size);
	}

	/*
	 * RMI (NetLogic/Broadcom) don't support WB (op 6) so we have to make
	 * do with WBINV (op 5).  This is merely for correctness since because
	 * the caches are coherent, these routines will become noops in a bit.
	 */
	if (MIPS_PRID_CID(cpu_id) == MIPS_PRID_CID_RMI) {
		mco->mco_pdcache_wb_range = mco->mco_pdcache_wbinv_range;
		mco->mco_intern_pdcache_wb_range = mco->mco_pdcache_wbinv_range;
	}

	mipsNN_cache_init(cfg, cfg1);

	if (mips_options.mips_cpu_flags &
	    (CPU_MIPS_D_CACHE_COHERENT | CPU_MIPS_I_D_CACHE_COHERENT)) {
#ifdef CACHE_DEBUG
		printf("  Dcache is coherent\n");
#endif
		mco->mco_pdcache_wbinv_all = cache_noop;
		mco->mco_pdcache_wbinv_range =
		    (void (*)(vaddr_t, vsize_t))cache_noop;
		mco->mco_pdcache_wbinv_range_index =
		    (void (*)(vaddr_t, vsize_t))cache_noop;
		mco->mco_pdcache_inv_range =
		    (void (*)(vaddr_t, vsize_t))cache_noop;
		mco->mco_pdcache_wb_range =
		    (void (*)(vaddr_t, vsize_t))cache_noop;
	}
	if (mips_options.mips_cpu_flags & CPU_MIPS_I_D_CACHE_COHERENT) {
#ifdef CACHE_DEBUG
		printf("  Icache is coherent against Dcache\n");
#endif
		mco->mco_intern_pdcache_wbinv_all =
		    cache_noop;
		mco->mco_intern_pdcache_wbinv_range_index =
		    (void (*)(vaddr_t, vsize_t))cache_noop;
		mco->mco_intern_pdcache_wb_range =
		    (void (*)(vaddr_t, vsize_t))cache_noop;
	}
}
#endif /* MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2 > 0 */
