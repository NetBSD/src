/*	$NetBSD: cache_tx39.c,v 1.6 2008/01/26 14:40:08 tsutsui Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cache_tx39.c,v 1.6 2008/01/26 14:40:08 tsutsui Exp $");

#include <sys/param.h>

#include <mips/cache.h>
#include <mips/cache_tx39.h>
#include <mips/locore.h>

/*
 * Cache operations for TX3900/TX3920-style caches:
 *
 *	- I-cache direct-mapped (TX3900) or 2-way set-associative (TX3920)
 *	- D-cache 2-way set-associative
 *	- Write-through (TX3900, TX3920) or write-back (TX3920)
 *	- Physically indexed, phyiscally tagged
 *
 * XXX THIS IS NOT YET COMPLETE.
 */

#define	round_line(x)		(((x) + 15) & ~15)
#define	trunc_line(x)		((x) & ~15)

void
tx3900_icache_sync_all_16(void)
{

	tx3900_icache_do_inv_index_16(MIPS_PHYS_TO_KSEG0(0),
	    MIPS_PHYS_TO_KSEG0(mips_picache_size));
}

void
tx3900_icache_sync_range_16(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	if ((eva - va) >= mips_picache_size) {
		/* Just hit the whole thing. */
		va = MIPS_PHYS_TO_KSEG0(0);
		eva = MIPS_PHYS_TO_KSEG0(mips_picache_size);
	}

	tx3900_icache_do_inv_index_16(va, eva);
}

#undef round_line
#undef trunc_line

#define	round_line(x)		(((x) + 3) & ~3)
#define	trunc_line(x)		((x) & ~3)

static int tx3900_dummy_buffer[R3900_C_SIZE_MAX / sizeof(int)];

void
tx3900_pdcache_wbinv_all_4(void)
{
	vaddr_t va = MIPS_PHYS_TO_KSEG0(0);
	vaddr_t eva = va + mips_pdcache_size;
	volatile int *p;

	/*
	 * No Index Invalidate for the TX3900 -- have to execute a
	 * series of load instructions from the dummy buffer, instead.
	 */

	p = tx3900_dummy_buffer;
	while (va < eva) {
		(void) *p++; (void) *p++; (void) *p++; (void) *p++;
		(void) *p++; (void) *p++; (void) *p++; (void) *p++;
		(void) *p++; (void) *p++; (void) *p++; (void) *p++;
		(void) *p++; (void) *p++; (void) *p++; (void) *p++;
		(void) *p++; (void) *p++; (void) *p++; (void) *p++;
		(void) *p++; (void) *p++; (void) *p++; (void) *p++;
		(void) *p++; (void) *p++; (void) *p++; (void) *p++;
		(void) *p++; (void) *p++; (void) *p++; (void) *p++;
		va += (32 * 4);
	}
}

void
tx3900_pdcache_inv_range_4(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	while ((eva - va) >= (32 * 4)) {
		cache_tx39_op_32lines_4(va,
		    CACHE_TX39_D|CACHEOP_TX3900_HIT_INV);
		va += (32 * 4);
	};

	while (va < eva) {
		cache_op_tx39_line(va, CACHE_TX39_D|CACHEOP_TX3900_HIT_INV);
		va += 4;
	}
}

void
tx3900_pdcache_wb_range_4(vaddr_t va, vsize_t size)
{

	/* Cache is write-through. */
}

#undef round_line
#undef trunc_line

#define	round_line(x)		(((x) + 15) & ~15)
#define	trunc_line(x)		((x) & ~15)

void
tx3920_icache_sync_all_16wb(void)
{

	mips_dcache_wbinv_all();

	__asm volatile(".set push; .set mips2; sync; .set pop");

	tx3920_icache_do_inv_16(MIPS_PHYS_TO_KSEG0(0),
	    MIPS_PHYS_TO_KSEG0(mips_picache_size));
}

void
tx3920_icache_sync_range_16wt(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	tx3920_icache_do_inv_16(va, eva);
}

void
tx3920_icache_sync_range_16wb(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	mips_dcache_wb_range(va, (eva - va));

	__asm volatile(".set push; .set mips2; sync; .set pop");

	tx3920_icache_do_inv_16(va, eva);
}

void
tx3920_pdcache_wbinv_all_16wt(void)
{
	vaddr_t va = MIPS_PHYS_TO_KSEG0(0);
	vaddr_t eva = va + mips_pdcache_size;

	/*
	 * Since we're hitting the whole thing, we don't have to
	 * worry about the 2 different "ways".
	 */

	while (va < eva) {
		cache_tx39_op_32lines_16(va,
		    CACHE_TX39_D|CACHEOP_TX3920_INDEX_INV);
		va += (32 * 16);
	}
}

void
tx3920_pdcache_wbinv_all_16wb(void)
{
	vaddr_t va = MIPS_PHYS_TO_KSEG0(0);
	vaddr_t eva = va + mips_pdcache_size;

	/*
	 * Since we're hitting the whole thing, we don't have to
	 * worry about the 2 different "ways".
	 */

	while (va < eva) {
		cache_tx39_op_32lines_16(va,
		    CACHE_TX39_D|CACHEOP_TX3920_INDEX_WB_INV);
		va += (32 * 16);
	}
}

void
tx3920_pdcache_wbinv_range_16wb(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	while ((eva - va) >= (32 * 16)) {
		cache_tx39_op_32lines_16(va,
		    CACHE_TX39_D|CACHEOP_TX3920_HIT_WB_INV);
		va += (32 * 16);
	}

	while (va < eva) {
		cache_op_tx39_line(va, CACHE_TX39_D|CACHEOP_TX3920_HIT_WB_INV);
		va += 16;
	}
}

void
tx3920_pdcache_inv_range_16(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	while ((eva - va) >= (32 * 16)) {
		cache_tx39_op_32lines_16(va,
		    CACHE_TX39_D|CACHEOP_TX3920_HIT_INV);
		va += (32 * 16);
	}

	while (va < eva) {
		cache_op_tx39_line(va, CACHE_TX39_D|CACHEOP_TX3920_HIT_INV);
		va += 16;
	}
}

void
tx3920_pdcache_wb_range_16wt(vaddr_t va, vsize_t size)
{

	/* Cache is write-through. */
}

void
tx3920_pdcache_wb_range_16wb(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	while ((eva - va) >= (32 * 16)) {
		cache_tx39_op_32lines_16(va,
		    CACHE_TX39_D|CACHEOP_TX3920_HIT_WB);
		va += (32 * 16);
	}

	while (va < eva) {
		cache_op_tx39_line(va, CACHE_TX39_D|CACHEOP_TX3920_HIT_WB);
		va += 16;
	}
}
