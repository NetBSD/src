/*	$NetBSD: cache.c,v 1.2.2.3 2002/04/01 07:41:01 nathanw Exp $	*/

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
 *    Broadcom Corporation. Neither the "Broadcom Corporation" name nor any
 *    trademark or logo of Broadcom Corporation may be used to endorse or
 *    promote products derived from this software without the prior written
 *    permission of Broadcom Corporation.
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

#include "opt_cputype.h"

#include <sys/param.h>

#include <uvm/uvm_extern.h>

#include <mips/cache.h>
#include <mips/locore.h>

#ifdef MIPS1
#include <mips/cache_r3k.h>
#endif

#ifdef MIPS3_PLUS
#include <mips/cache_r4k.h>	/* includes r5k and greater */
#endif

#if defined(MIPS32) || defined(MIPS64)
#include <mips/mipsNN.h>		/* MIPS32/MIPS64 registers */
#include <mips/cache_mipsNN.h>
#endif

/* PRIMARY CACHE VARIABLES */
int mips_picache_size;
int mips_picache_line_size;
int mips_picache_ways;
int mips_picache_way_size;
int mips_picache_way_mask;
 
int mips_pdcache_size;           /* and unified */
int mips_pdcache_line_size;
int mips_pdcache_ways;
int mips_pdcache_way_size;
int mips_pdcache_way_mask;
int mips_pdcache_write_through;

int mips_pcache_unified;

/* SECONDARY CACHE VARIABLES */
int mips_sicache_size;
int mips_sicache_line_size; 
int mips_sicache_ways;
int mips_sicache_way_size;
int mips_sicache_way_mask;

int mips_sdcache_size;           /* and unified */
int mips_sdcache_line_size;
int mips_sdcache_ways;
int mips_sdcache_way_size;
int mips_sdcache_way_mask;
int mips_sdcache_write_through;

int mips_scache_unified;

/* TERTIARY CACHE VARIABLES */
int mips_tcache_size;            /* always unified */
int mips_tcache_line_size;
int mips_tcache_ways;
int mips_tcache_way_size;
int mips_tcache_way_mask;
int mips_tcache_write_through;

/*
 * These two variables inform the rest of the kernel about the
 * size of the largest D-cache line present in the system.  The
 * mask can be used to determine if a region of memory is cache
 * line size aligned.
 *
 * Whenever any code updates a data cache line size, it should
 * call mips_dcache_compute_align() to recompute these values.
 */
int mips_dcache_align;
int mips_dcache_align_mask;

int mips_cache_alias_mask;	/* for virtually-indexed caches */
int mips_cache_prefer_mask;

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
#ifdef MIPS3_5900
#include <mips/cache_r5900.h>
#endif /* MIPS3_5900 */
void	mips3_get_cache_config(int);
#endif /* MIPS3 || MIPS4 */

#if defined(MIPS1) || defined(MIPS3) || defined(MIPS4)
static void mips_config_cache_prehistoric(void);
#endif
#if defined(MIPS32) || defined(MIPS64)
static void mips_config_cache_modern(void);
#endif

/*
 * mips_dcache_compute_align:
 *
 *	Compute the D-cache alignment values.
 */
void
mips_dcache_compute_align(void)
{
	int align;

	align = mips_pdcache_line_size;

	if (mips_sdcache_line_size > align)
		align = mips_sdcache_line_size;

	if (mips_tcache_line_size > align)
		align = mips_tcache_line_size;

	mips_dcache_align = align;
	mips_dcache_align_mask = align - 1;
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
	
#if defined(MIPS1) || defined(MIPS3) || defined(MIPS4)
	if (MIPS_PRID_CID(cpu_id) == MIPS_PRID_CID_PREHISTORIC)
		mips_config_cache_prehistoric();
#endif
#if defined(MIPS32) || defined(MIPS64)
	if (MIPS_PRID_CID(cpu_id) != MIPS_PRID_CID_PREHISTORIC)
		mips_config_cache_modern();
#endif

#ifdef DIAGNOSTIC
	/* Check that all cache ops are set up. */
	if (mips_picache_size || 1) {	/* XXX- must have primary Icache */
		if (!mips_cache_ops.mco_icache_sync_all)
			panic("no icache_sync_all cache op");
		if (!mips_cache_ops.mco_icache_sync_range)
			panic("no icache_sync_range cache op");
		if (!mips_cache_ops.mco_icache_sync_range_index)
			panic("no icache_sync_range_index cache op");
	}
	if (mips_pdcache_size || 1) {	/* XXX- must have primary Icache */
		if (!mips_cache_ops.mco_pdcache_wbinv_all)
			panic("no pdcache_wbinv_all");
		if (!mips_cache_ops.mco_pdcache_wbinv_range)
			panic("no pdcache_wbinv_range");
		if (!mips_cache_ops.mco_pdcache_wbinv_range_index)
			panic("no pdcache_wbinv_range_index");
		if (!mips_cache_ops.mco_pdcache_inv_range)
			panic("no pdcache_inv_range");
		if (!mips_cache_ops.mco_pdcache_wb_range)
			panic("no pdcache_wb_range");
	}
	if (mips_sdcache_size) {
		if (!mips_cache_ops.mco_sdcache_wbinv_all)
			panic("no sdcache_wbinv_all");
		if (!mips_cache_ops.mco_sdcache_wbinv_range)
			panic("no sdcache_wbinv_range");
		if (!mips_cache_ops.mco_sdcache_wbinv_range_index)
			panic("no sdcache_wbinv_range_index");
		if (!mips_cache_ops.mco_sdcache_inv_range)
			panic("no sdcache_inv_range");
		if (!mips_cache_ops.mco_sdcache_wb_range)
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
		mips_picache_size = r3k_picache_size();
		mips_pdcache_size = r3k_pdcache_size();

		mips_picache_line_size = 4;
		mips_pdcache_line_size = 4;

		mips_picache_ways = 1;
		mips_pdcache_ways = 1;

		mips_pdcache_write_through = 1;

		mips_cache_ops.mco_icache_sync_all =
		    r3k_icache_sync_all;
		mips_cache_ops.mco_icache_sync_range =
		    r3k_icache_sync_range;
		mips_cache_ops.mco_icache_sync_range_index =
		    mips_cache_ops.mco_icache_sync_range;

		mips_cache_ops.mco_pdcache_wbinv_all =
		    r3k_pdcache_wbinv_all;
		mips_cache_ops.mco_pdcache_wbinv_range =
		    r3k_pdcache_inv_range;
		mips_cache_ops.mco_pdcache_wbinv_range_index =
		    mips_cache_ops.mco_pdcache_wbinv_range;
		mips_cache_ops.mco_pdcache_inv_range =
		    r3k_pdcache_inv_range;
		mips_cache_ops.mco_pdcache_wb_range =
		    r3k_pdcache_wb_range;

		uvmexp.ncolors = atop(mips_pdcache_size);
		break;

#ifdef ENABLE_MIPS_TX3900
	case MIPS_TX3900:
		switch (MIPS_PRID_REV_MAJ(cpu_id)) {
		case 1:		/* TX3912 */
			mips_picache_ways = 1;
			mips_picache_line_size = 16;
			mips_pdcache_line_size = 4;

			tx3900_get_cache_config();

			mips_pdcache_write_through = 1;

			mips_cache_ops.mco_icache_sync_all =
			    tx3900_icache_sync_all_16;
			mips_cache_ops.mco_icache_sync_range =
			    tx3900_icache_sync_range_16;
			mips_cache_ops.mco_icache_sync_range_index =
			    tx3900_icache_sync_range_16;

			mips_cache_ops.mco_pdcache_wbinv_all =
			    tx3900_pdcache_wbinv_all_4;
			mips_cache_ops.mco_pdcache_wbinv_range =
			    tx3900_pdcache_inv_range_4;
			mips_cache_ops.mco_pdcache_wbinv_range_index =
			    tx3900_pdcache_inv_range_4;
			mips_cache_ops.mco_pdcache_inv_range =
			    tx3900_pdcache_inv_range_4;
			mips_cache_ops.mco_pdcache_wb_range =
			    tx3900_pdcache_wb_range_4;
			break;

		case 3:		/* TX3922 */
			mips_picache_ways = 2;
			mips_picache_line_size = 16;
			mips_pdcache_line_size = 16;

			tx3920_get_cache_config();

			mips_cache_ops.mco_icache_sync_all =
			    mips_pdcache_write_through ?
			    tx3900_icache_sync_all_16 :
			    tx3920_icache_sync_all_16wb;
			mips_cache_ops.mco_icache_sync_range =
			    mips_pdcache_write_through ?
			    tx3920_icache_sync_range_16wt :
			    tx3920_icache_sync_range_16wb;
			mips_cache_ops.mco_icache_sync_range_index =
			    mips_cache_ops.mco_icache_sync_range;

			mips_cache_ops.mco_pdcache_wbinv_all =
			    mips_pdcache_write_through ?
			    tx3920_pdcache_wbinv_all_16wt :
			    tx3920_pdcache_wbinv_all_16wb;
			mips_cache_ops.mco_pdcache_wbinv_range =
			    mips_pdcache_write_through ?
			    tx3920_pdcache_inv_range_16 :
			    tx3920_pdcache_wbinv_range_16wb;
			mips_cache_ops.mco_pdcache_wbinv_range_index =
			    mips_cache_ops.mco_pdcache_wbinv_range;
			mips_cache_ops.mco_pdcache_inv_range =
			    tx3920_pdcache_inv_range_16;
			mips_cache_ops.mco_pdcache_wb_range =
			    mips_pdcache_write_through ?
			    tx3920_pdcache_wb_range_16wt :
			    tx3920_pdcache_wb_range_16wb;
			break;

		default:
			panic("mips_config_cache: unsupported TX3900");
		}

		mips_pdcache_ways = 2;
		tx3900_get_cache_config();
		/* change to write-through mode */
		tx39_cache_config_write_through();

		uvmexp.ncolors = atop(mips_pdcache_size) / mips_pdcache_ways;
		break;
#endif /* ENABLE_MIPS_TX3900 */
#endif /* MIPS1 */

#if defined(MIPS3) || defined(MIPS4)
	case MIPS_R4100:
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
		mips_picache_ways = 1;
		mips_pdcache_ways = 1;
		mips_sdcache_ways = 1;

		mips3_get_cache_config(csizebase);

		switch (mips_picache_line_size) {
		case 16:
			mips_cache_ops.mco_icache_sync_all =
			    r4k_icache_sync_all_16;
			mips_cache_ops.mco_icache_sync_range =
			    r4k_icache_sync_range_16;
			mips_cache_ops.mco_icache_sync_range_index =
			    r4k_icache_sync_range_index_16;
			break;

		case 32:
			mips_cache_ops.mco_icache_sync_all =
			    r4k_icache_sync_all_32;
			mips_cache_ops.mco_icache_sync_range =
			    r4k_icache_sync_range_32;
			mips_cache_ops.mco_icache_sync_range_index =
			    r4k_icache_sync_range_index_32;
			break;

		default:
			panic("r4k picache line size %d",
			    mips_picache_line_size);
		}

		switch (mips_pdcache_line_size) {
		case 16:
			mips_cache_ops.mco_pdcache_wbinv_all =
			    r4k_pdcache_wbinv_all_16;
			mips_cache_ops.mco_pdcache_wbinv_range =
			    r4k_pdcache_wbinv_range_16;
			mips_cache_ops.mco_pdcache_wbinv_range_index =
			    r4k_pdcache_wbinv_range_index_16;
			mips_cache_ops.mco_pdcache_inv_range =
			    r4k_pdcache_inv_range_16;
			mips_cache_ops.mco_pdcache_wb_range =
			    r4k_pdcache_wb_range_16;
			break;

		case 32:
			mips_cache_ops.mco_pdcache_wbinv_all =
			    r4k_pdcache_wbinv_all_32;
			mips_cache_ops.mco_pdcache_wbinv_range =
			    r4k_pdcache_wbinv_range_32;
			mips_cache_ops.mco_pdcache_wbinv_range_index =
			    r4k_pdcache_wbinv_range_index_32;
			mips_cache_ops.mco_pdcache_inv_range =
			    r4k_pdcache_inv_range_32;
			mips_cache_ops.mco_pdcache_wb_range =
			    r4k_pdcache_wb_range_32;
			break;

		default:
			panic("r4k pdcache line size %d",
			    mips_pdcache_line_size);
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
		mips_picache_ways = 2;
		mips_pdcache_ways = 2;

		mips3_get_cache_config(csizebase);

		switch (mips_picache_line_size) {
		case 32:
			mips_cache_ops.mco_icache_sync_all =
			    r5k_icache_sync_all_32;
			mips_cache_ops.mco_icache_sync_range =
			    r5k_icache_sync_range_32;
			mips_cache_ops.mco_icache_sync_range_index =
			    r5k_icache_sync_range_index_32;
			break;

		default:
			panic("r5k picache line size %d",
			    mips_picache_line_size);
		}

		switch (mips_pdcache_line_size) {
		case 16:
			mips_cache_ops.mco_pdcache_wbinv_all =
			    r5k_pdcache_wbinv_all_16;
			mips_cache_ops.mco_pdcache_wbinv_range =
			    r5k_pdcache_wbinv_range_16;
			mips_cache_ops.mco_pdcache_wbinv_range_index =
			    r5k_pdcache_wbinv_range_index_16;
			mips_cache_ops.mco_pdcache_inv_range =
			    r5k_pdcache_inv_range_16;
			mips_cache_ops.mco_pdcache_wb_range =
			    r5k_pdcache_wb_range_16;
			break;

		case 32:
			mips_cache_ops.mco_pdcache_wbinv_all =
			    r5k_pdcache_wbinv_all_32;
			mips_cache_ops.mco_pdcache_wbinv_range =
			    r5k_pdcache_wbinv_range_32;
			mips_cache_ops.mco_pdcache_wbinv_range_index =
			    r5k_pdcache_wbinv_range_index_32;
			mips_cache_ops.mco_pdcache_inv_range =
			    r5k_pdcache_inv_range_32;
			mips_cache_ops.mco_pdcache_wb_range =
			    r5k_pdcache_wb_range_32;
			break;

		default:
			panic("r5k pdcache line size %d",
			    mips_pdcache_line_size);
		}

		/*
		 * Deal with R4600 chip bugs.
		 */
		if (MIPS_PRID_IMPL(cpu_id) == MIPS_R4600 &&
		    MIPS_PRID_REV_MAJ(cpu_id) == 1) {
			KASSERT(mips_pdcache_line_size == 32);
			mips_cache_ops.mco_pdcache_wbinv_range =
			    r4600v1_pdcache_wbinv_range_32;
			mips_cache_ops.mco_pdcache_inv_range =
			    r4600v1_pdcache_inv_range_32;
			mips_cache_ops.mco_pdcache_wb_range =
			    r4600v1_pdcache_wb_range_32;
		} else if (MIPS_PRID_IMPL(cpu_id) == MIPS_R4600 &&
			   MIPS_PRID_REV_MAJ(cpu_id) == 2) {
			KASSERT(mips_pdcache_line_size == 32);
			mips_cache_ops.mco_pdcache_wbinv_range =
			    r4600v2_pdcache_wbinv_range_32;
			mips_cache_ops.mco_pdcache_inv_range =
			    r4600v2_pdcache_inv_range_32;
			mips_cache_ops.mco_pdcache_wb_range =
			    r4600v2_pdcache_wb_range_32;
		}

		/*
		 * Deal with VR4131 chip bugs.
		 */
		if (MIPS_PRID_IMPL(cpu_id) == MIPS_R4100 &&
		    MIPS_PRID_REV_MAJ(cpu_id) == 8) {
			KASSERT(mips_pdcache_line_size == 16);
			mips_cache_ops.mco_pdcache_wbinv_range =
			    vr4131v1_pdcache_wbinv_range_16;
		}

		/* Virtually-indexed cache; no use for colors. */
		break;
#ifdef MIPS3_5900
	case MIPS_R5900:
		/* cache spec */
		mips_picache_ways = 2;
		mips_pdcache_ways = 2;
		mips_picache_size = CACHE_R5900_SIZE_I;
		mips_picache_line_size = CACHE_R5900_LSIZE_I;
		mips_pdcache_size = CACHE_R5900_SIZE_D;
		mips_pdcache_line_size = CACHE_R5900_LSIZE_D;
		mips_cache_alias_mask =
		    ((mips_pdcache_size / mips_pdcache_ways) - 1) &
		    ~(PAGE_SIZE - 1);
		mips_cache_prefer_mask =
		    max(mips_pdcache_size, mips_picache_size) - 1;
		/* cache ops */
		mips_cache_ops.mco_icache_sync_all =
		    r5900_icache_sync_all_64;
		mips_cache_ops.mco_icache_sync_range =
		    r5900_icache_sync_range_64;
		mips_cache_ops.mco_icache_sync_range_index =
		    r5900_icache_sync_range_index_64;
		mips_cache_ops.mco_pdcache_wbinv_all =
		    r5900_pdcache_wbinv_all_64;
		mips_cache_ops.mco_pdcache_wbinv_range =
		    r5900_pdcache_wbinv_range_64;
		mips_cache_ops.mco_pdcache_wbinv_range_index =
		    r5900_pdcache_wbinv_range_index_64;
		mips_cache_ops.mco_pdcache_inv_range =
		    r5900_pdcache_inv_range_64;
		mips_cache_ops.mco_pdcache_wb_range =
		    r5900_pdcache_wb_range_64;
		break;
#endif /* MIPS3_5900 */
#endif /* MIPS3 || MIPS4 */

	default:
		panic("can't handle primary cache on impl 0x%x\n",
		    MIPS_PRID_IMPL(cpu_id));
	}

	/*
	 * Compute the "way mask" for each cache.
	 */
	if (mips_picache_size) {
		KASSERT(mips_picache_ways != 0);
		mips_picache_way_size = (mips_picache_size / mips_picache_ways);
		mips_picache_way_mask = mips_picache_way_size - 1;
	}
	if (mips_pdcache_size) {
		KASSERT(mips_pdcache_ways != 0);
		mips_pdcache_way_size = (mips_pdcache_size / mips_pdcache_ways);
		mips_pdcache_way_mask = mips_pdcache_way_size - 1;
	}

	mips_dcache_compute_align();

	if (mips_sdcache_line_size == 0)
		return;

	/*
	 * Configure the secondary cache.
	 */
	switch (MIPS_PRID_IMPL(cpu_id)) {
#if defined(MIPS3) || defined(MIPS4)
	case MIPS_R4000:
		/*
		 * R4000/R4400 always detects virtual alias as if
		 * primary cache size is 32KB. Actual primary cache size
		 * is ignored wrt VCED/VCEI.
		 */
		mips_cache_alias_mask =
			(MIPS3_MAX_PCACHE_SIZE - 1) & ~(PAGE_SIZE - 1);
		mips_cache_prefer_mask = MIPS3_MAX_PCACHE_SIZE - 1;
		/* FALLTHROUGH */
	case MIPS_R4100:
	case MIPS_R4300:
	case MIPS_R4600:
#ifdef ENABLE_MIPS_R4700
	case MIPS_R4700:
#endif
#ifndef ENABLE_MIPS_R3NKK
	case MIPS_R5000:
#endif
	case MIPS_RM5200:
		switch (mips_sdcache_ways) {
		case 1:
			switch (mips_sdcache_line_size) {
			case 32:
				mips_cache_ops.mco_sdcache_wbinv_all =
				    r4k_sdcache_wbinv_all_32;
				mips_cache_ops.mco_sdcache_wbinv_range =
				    r4k_sdcache_wbinv_range_32;
				mips_cache_ops.mco_sdcache_wbinv_range_index =
				    r4k_sdcache_wbinv_range_index_32;
				mips_cache_ops.mco_sdcache_inv_range =
				    r4k_sdcache_inv_range_32;
				mips_cache_ops.mco_sdcache_wb_range =
				    r4k_sdcache_wb_range_32;
				break;

			case 16:
			case 64:
				mips_cache_ops.mco_sdcache_wbinv_all =
				    r4k_sdcache_wbinv_all_generic;
				mips_cache_ops.mco_sdcache_wbinv_range =
				    r4k_sdcache_wbinv_range_generic;
				mips_cache_ops.mco_sdcache_wbinv_range_index =
				    r4k_sdcache_wbinv_range_index_generic;
				mips_cache_ops.mco_sdcache_inv_range =
				    r4k_sdcache_inv_range_generic;
				mips_cache_ops.mco_sdcache_wb_range =
				    r4k_sdcache_wb_range_generic;
				break;

			case 128:
				mips_cache_ops.mco_sdcache_wbinv_all =
				    r4k_sdcache_wbinv_all_128;
				mips_cache_ops.mco_sdcache_wbinv_range =
				    r4k_sdcache_wbinv_range_128;
				mips_cache_ops.mco_sdcache_wbinv_range_index =
				    r4k_sdcache_wbinv_range_index_128;
				mips_cache_ops.mco_sdcache_inv_range =
				    r4k_sdcache_inv_range_128;
				mips_cache_ops.mco_sdcache_wb_range =
				    r4k_sdcache_wb_range_128;
				break;

			default:
				panic("r4k sdcache %d way line size %d\n",
				    mips_sdcache_ways, mips_sdcache_line_size);
			}
			break;

		default:
			panic("r4k sdcache %d way line size %d\n",
			    mips_sdcache_ways, mips_sdcache_line_size);
		}
		break;
#endif /* MIPS3 || MIPS4 */

	default:
		panic("can't handle secondary cache on impl 0x%x\n",
		    MIPS_PRID_IMPL(cpu_id));
	}

	/*
	 * Compute the "way mask" for each secondary cache.
	 */
	if (mips_sdcache_size) {
		KASSERT(mips_sdcache_ways != 0);
		mips_sdcache_way_size = (mips_sdcache_size / mips_sdcache_ways);
		mips_sdcache_way_mask = mips_sdcache_way_size - 1;
	}

	mips_dcache_compute_align();
}

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
	uint32_t config;

	config = tx3900_cp0_config_read();

	mips_picache_size = R3900_C_SIZE_MIN <<
	    ((config & R3900_CONFIG_ICS_MASK) >> R3900_CONFIG_ICS_SHIFT);

	mips_pdcache_size = R3900_C_SIZE_MIN <<
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

	/* Size is the same as TX3900. */
	tx3900_get_cache_config();

	/* Now determine write-through/write-back mode. */
	if ((tx3900_cp0_config_read() & R3900_CONFIG_WBON) == 0)
		mips_pdcache_write_through = 1;
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

	__asm__ __volatile__("mfc0 %0, $3" : "=r"(r));
	r &= 0xffffdfff;
	__asm__ __volatile__("mtc0 %0, $3" : : "r"(r));
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
	uint32_t config = mips3_cp0_config_read();

	mips_picache_size = MIPS3_CONFIG_CACHE_SIZE(config,
	    MIPS3_CONFIG_IC_MASK, csizebase, MIPS3_CONFIG_IC_SHIFT);
	mips_picache_line_size = MIPS3_CONFIG_CACHE_L1_LSIZE(config,
	    MIPS3_CONFIG_IB);

	mips_pdcache_size = MIPS3_CONFIG_CACHE_SIZE(config,
	    MIPS3_CONFIG_DC_MASK, csizebase, MIPS3_CONFIG_DC_SHIFT);
	mips_pdcache_line_size = MIPS3_CONFIG_CACHE_L1_LSIZE(config,
	    MIPS3_CONFIG_DB);

	mips_cache_alias_mask =
	    ((mips_pdcache_size / mips_pdcache_ways) - 1) & ~(PAGE_SIZE - 1);
	mips_cache_prefer_mask =
	    max(mips_pdcache_size, mips_picache_size) - 1;

	if ((config & MIPS3_CONFIG_SC) == 0) {
		mips_sdcache_line_size = MIPS3_CONFIG_CACHE_L2_LSIZE(config);
		if ((config & MIPS3_CONFIG_SS) == 0)
			mips_scache_unified = 1;
	}
}
#endif /* MIPS3 || MIPS4 */
#endif /* MIPS1 || MIPS3 || MIPS4 */

#if defined(MIPS32) || defined(MIPS64)

#ifdef MIPS_DISABLE_L1_CACHE
static void cache_noop(void);
static void cache_noop(void) {}
#endif

static void
mips_config_cache_modern(void)
{
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
		mips_pdcache_line_size = mips_pdcache_way_size =
		    mips_pdcache_ways = 0;
		break;
	case MIPSNN_CFG1_DL_RSVD:
		panic("reserved MIPS32/64 Dcache line size");
		break;
	default:
		if (MIPSNN_GET(CFG1_DS, cfg1) == MIPSNN_CFG1_DS_RSVD)
			panic("reserved MIPS32/64 Dcache sets per way");
		mips_pdcache_line_size = MIPSNN_CFG1_DL(cfg1);
		mips_pdcache_way_size =
		    mips_pdcache_line_size * MIPSNN_CFG1_DS(cfg1);
		mips_pdcache_ways = MIPSNN_CFG1_DA(cfg1) + 1;

		/*
		 * Compute the total size and "way mask" for the
		 * primary Icache.
		 */
		mips_pdcache_size =
		    mips_pdcache_way_size * mips_pdcache_ways;
		mips_pdcache_way_mask = mips_pdcache_way_size - 1;
		break;
	}

	/* figure out Icache params. */
	switch (MIPSNN_GET(CFG1_IL, cfg1)) {
	case MIPSNN_CFG1_IL_NONE:
		mips_picache_line_size = mips_picache_way_size =
		    mips_picache_ways = 0;
		break;
	case MIPSNN_CFG1_IL_RSVD:
		panic("reserved MIPS32/64 Icache line size");
		break;
	default:
		if (MIPSNN_GET(CFG1_IS, cfg1) == MIPSNN_CFG1_IS_RSVD)
			panic("reserved MIPS32/64 Icache sets per way");
		mips_picache_line_size = MIPSNN_CFG1_IL(cfg1);
		mips_picache_way_size =
		    mips_picache_line_size * MIPSNN_CFG1_IS(cfg1);
		mips_picache_ways = MIPSNN_CFG1_IA(cfg1) + 1;

		/*
		 * Compute the total size and "way mask" for the
		 * primary Dcache.
		 */
		mips_picache_size =
		    mips_picache_way_size * mips_picache_ways;
		mips_picache_way_mask = mips_picache_way_size - 1;
		break;
	}

#define CACHE_DEBUG
#ifdef CACHE_DEBUG
	printf("MIPS32/64 params: cpu arch: %d\n", cpu_arch);
	printf("MIPS32/64 params: TLB entries: %d\n", mips_num_tlb_entries);
	if (mips_picache_line_size == 0)
		printf("MIPS32/64 params: no Icache\n");
	else {
		printf("MIPS32/64 params: Icache: line = %d, total = %d, "
		    "ways = %d\n", mips_picache_line_size,
		    mips_picache_way_size * mips_picache_ways,
		    mips_picache_ways);
		printf("\t\t sets = %d\n", (mips_picache_way_size *
		    mips_picache_ways / mips_picache_line_size) /
		    mips_picache_ways);
	}
	if (mips_pdcache_line_size == 0)
		printf("MIPS32/64 params: no Dcache\n");
	else {
		printf("MIPS32/64 params: Dcache: line = %d, total = %d, "
		    "ways = %d\n", mips_pdcache_line_size,
		    mips_pdcache_way_size * mips_pdcache_ways,
		    mips_pdcache_ways);
		printf("\t\t sets = %d\n", (mips_pdcache_way_size *
		    mips_pdcache_ways / mips_pdcache_line_size) /
		    mips_pdcache_ways);
	}
#endif /* CACHE_DEBUG */

	switch (mips_picache_line_size) {
	case 16:
		mips_cache_ops.mco_icache_sync_all = mipsNN_icache_sync_all_16;
		mips_cache_ops.mco_icache_sync_range =
		    mipsNN_icache_sync_range_16;
		switch (mips_picache_ways) {
		case 4:
			mips_cache_ops.mco_icache_sync_range_index =
			    mipsNN_icache_sync_range_index_16_4way;
			break;
		default:
			panic("no %d-way Icache ops", mips_picache_ways);
		}
		break;
	case 32:
		mips_cache_ops.mco_icache_sync_all = mipsNN_icache_sync_all_32;
		mips_cache_ops.mco_icache_sync_range =
		    mipsNN_icache_sync_range_32;
		switch (mips_picache_ways) {
		case 4:
			mips_cache_ops.mco_icache_sync_range_index =
			    mipsNN_icache_sync_range_index_32_4way;
			break;
		default:
			panic("no %d-way Icache ops", mips_picache_ways);
		}
		break;
#ifdef MIPS_DISABLE_L1_CACHE
	case 0:
		mips_cache_ops.mco_icache_sync_all = (void *)cache_noop;
		mips_cache_ops.mco_icache_sync_range = (void *)cache_noop;
		mips_cache_ops.mco_icache_sync_range_index = (void *)cache_noop;
		break;
#endif
	default:
		panic("no Icache ops for %d byte lines",
		    mips_picache_line_size);
	}

	switch (mips_pdcache_line_size) {
	case 16:
		mips_cache_ops.mco_pdcache_wbinv_all =
		    mipsNN_pdcache_wbinv_all_16;
		mips_cache_ops.mco_pdcache_wbinv_range =
		    mipsNN_pdcache_wbinv_range_16;
		switch (mips_pdcache_ways) {
		case 4:
			mips_cache_ops.mco_pdcache_wbinv_range_index =
			    mipsNN_pdcache_wbinv_range_index_16_4way;
			break;
		default:
			panic("no %d-way Dcache ops", mips_pdcache_ways);
		}
		mips_cache_ops.mco_pdcache_inv_range =
		    mipsNN_pdcache_inv_range_16;
		mips_cache_ops.mco_pdcache_wb_range =
		    mipsNN_pdcache_wb_range_16;
		break;
	case 32:
		mips_cache_ops.mco_pdcache_wbinv_all =
		    mipsNN_pdcache_wbinv_all_32;
		mips_cache_ops.mco_pdcache_wbinv_range =
		    mipsNN_pdcache_wbinv_range_32;
		switch (mips_pdcache_ways) {
		case 4:
			mips_cache_ops.mco_pdcache_wbinv_range_index =
			    mipsNN_pdcache_wbinv_range_index_32_4way;
			break;
		default:
			panic("no %d-way Dcache ops", mips_pdcache_ways);
		}
		mips_cache_ops.mco_pdcache_inv_range =
		    mipsNN_pdcache_inv_range_32;
		mips_cache_ops.mco_pdcache_wb_range =
		    mipsNN_pdcache_wb_range_32;
		break;
#ifdef MIPS_DISABLE_L1_CACHE
	case 0:
		mips_cache_ops.mco_pdcache_wbinv_all = (void *)cache_noop;
		mips_cache_ops.mco_pdcache_wbinv_range = (void *)cache_noop;
		mips_cache_ops.mco_pdcache_wbinv_range_index =
		    (void *)cache_noop;
		mips_cache_ops.mco_pdcache_inv_range = (void *)cache_noop;
		mips_cache_ops.mco_pdcache_wb_range = (void *)cache_noop;
		break;
#endif
	default:
		panic("no Dcache ops for %d byte lines",
		    mips_pdcache_line_size);
	}
}
#endif /* MIPS32 || MIPS64 */
