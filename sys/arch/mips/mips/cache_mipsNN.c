/*	$NetBSD: cache_mipsNN.c,v 1.1.8.3 2002/04/17 00:03:47 nathanw Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
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

#include <sys/param.h>

#include <mips/cache.h>
#include <mips/cache_r4k.h>
#include <mips/cache_mipsNN.h>

#define	round_line16(x)		(((x) + 15) & ~15)
#define	trunc_line16(x)		((x) & ~15)

#define	round_line32(x)		(((x) + 31) & ~31)
#define	trunc_line32(x)		((x) & ~31)


#ifdef SB1250_PASS1
#define	SYNC	__asm __volatile("sync; sync")
#else
#define	SYNC	__asm __volatile("sync")
#endif


#ifdef SB1250_PASS1
void mipsNN_pdcache_wbinv_range_32_sb1(vaddr_t, vsize_t);
void mipsNN_pdcache_wbinv_range_index_32_4way_sb1(vaddr_t, vsize_t);
#endif

__asm(".set mips32");

void
mipsNN_icache_sync_all_16(void)
{
	vaddr_t va, eva;

	va = MIPS_PHYS_TO_KSEG0(0);
	eva = va + mips_picache_size;

	/*
	 * Since we're hitting the whole thing, we don't have to
	 * worry about the N different "ways".
	 */

	mips_dcache_wbinv_all();

	while (va < eva) {
		cache_r4k_op_32lines_16(va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va += (32 * 16);
	}

	SYNC;
}

void
mipsNN_icache_sync_all_32(void)
{
	vaddr_t va, eva;

	va = MIPS_PHYS_TO_KSEG0(0);
	eva = va + mips_picache_size;

	/*
	 * Since we're hitting the whole thing, we don't have to
	 * worry about the N different "ways".
	 */

	mips_dcache_wbinv_all();

	while (va < eva) {
		cache_r4k_op_32lines_32(va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va += (32 * 32);
	}

	SYNC;
}

void
mipsNN_icache_sync_range_16(vaddr_t va, vsize_t size)
{
	vaddr_t eva;

	va = trunc_line16(va);
	eva = round_line16(va + size);

	mips_dcache_wb_range(va, (eva - va));

	while ((eva - va) >= (32 * 16)) {
		cache_r4k_op_32lines_16(va, CACHE_R4K_I|CACHEOP_R4K_HIT_INV);
		va += (32 * 16);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_I|CACHEOP_R4K_HIT_INV);
		va += 16;
	}

	SYNC;
}

void
mipsNN_icache_sync_range_32(vaddr_t va, vsize_t size)
{
	vaddr_t eva;

	va = trunc_line32(va);
	eva = round_line32(va + size);

	mips_dcache_wb_range(va, (eva - va));

	while ((eva - va) >= (32 * 32)) {
		cache_r4k_op_32lines_32(va, CACHE_R4K_I|CACHEOP_R4K_HIT_INV);
		va += (32 * 32);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_I|CACHEOP_R4K_HIT_INV);
		va += 32;
	}

	SYNC;
}

void
mipsNN_icache_sync_range_index_16_2way(vaddr_t va, vsize_t size)
{
	vaddr_t w2va, eva;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & mips_picache_way_mask);

	va = trunc_line16(va);
	w2va = va + mips_picache_way_size;
	eva = round_line16(va + size);

	mips_dcache_wbinv_range_index(va, (eva - va));

	while ((eva - va) >= (16 * 16)) {
		cache_r4k_op_16lines_16_2way(va, w2va,
		    CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va   += (16 * 16);
		w2va += (16 * 16);
	}

	while (va < eva) {
		cache_op_r4k_line(va,   CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		cache_op_r4k_line(w2va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va   += 16;
		w2va += 16;
	}

	SYNC;
}

void
mipsNN_icache_sync_range_index_16_4way(vaddr_t va, vsize_t size)
{
	vaddr_t w2va, w3va, w4va, eva;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & mips_picache_way_mask);

	va = trunc_line16(va);
	w2va = va   + mips_picache_way_size;
	w3va = w2va + mips_picache_way_size;
	w4va = w3va + mips_picache_way_size;
	eva = round_line16(va + size);

	mips_dcache_wbinv_range_index(va, (eva - va));

	while ((eva - va) >= (8 * 16)) {
		cache_r4k_op_8lines_16_4way(va, w2va, w3va, w4va,
		    CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va   += (8 * 16);
		w2va += (8 * 16);
		w3va += (8 * 16);
		w4va += (8 * 16);
	}

	while (va < eva) {
		cache_op_r4k_line(va,   CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		cache_op_r4k_line(w2va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		cache_op_r4k_line(w3va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		cache_op_r4k_line(w4va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va   += 16;
		w2va += 16;
		w3va += 16;
		w4va += 16;
	}

	SYNC;
}

void
mipsNN_icache_sync_range_index_32_2way(vaddr_t va, vsize_t size)
{
	vaddr_t w2va, eva;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & mips_picache_way_mask);

	va = trunc_line32(va);
	w2va = va + mips_picache_way_size;
	eva = round_line32(va + size);

	mips_dcache_wbinv_range_index(va, (eva - va));

	while ((eva - va) >= (16 * 32)) {
		cache_r4k_op_16lines_32_2way(va, w2va,
		    CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va   += (16 * 32);
		w2va += (16 * 32);
	}

	while (va < eva) {
		cache_op_r4k_line(va,   CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		cache_op_r4k_line(w2va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va   += 32;
		w2va += 32;
	}

	SYNC;
}

void
mipsNN_icache_sync_range_index_32_4way(vaddr_t va, vsize_t size)
{
	vaddr_t w2va, w3va, w4va, eva;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & mips_picache_way_mask);

	va = trunc_line32(va);
	w2va = va   + mips_picache_way_size;
	w3va = w2va + mips_picache_way_size;
	w4va = w3va + mips_picache_way_size;
	eva = round_line32(va + size);

	mips_dcache_wbinv_range_index(va, (eva - va));

	while ((eva - va) >= (8 * 32)) {
		cache_r4k_op_8lines_32_4way(va, w2va, w3va, w4va,
		    CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va   += (8 * 32);
		w2va += (8 * 32);
		w3va += (8 * 32);
		w4va += (8 * 32);
	}

	while (va < eva) {
		cache_op_r4k_line(va,   CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		cache_op_r4k_line(w2va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		cache_op_r4k_line(w3va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		cache_op_r4k_line(w4va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va   += 32;
		w2va += 32;
		w3va += 32;
		w4va += 32;
	}

	SYNC;
}

void
mipsNN_pdcache_wbinv_all_16(void)
{
	vaddr_t va, eva;

	va = MIPS_PHYS_TO_KSEG0(0);
	eva = va + mips_pdcache_size;

	/*
	 * Since we're hitting the whole thing, we don't have to
	 * worry about the N different "ways".
	 */

	while (va < eva) {
		cache_r4k_op_32lines_16(va,
		    CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
		va += (32 * 16);
	}

	SYNC;
}

void
mipsNN_pdcache_wbinv_all_32(void)
{
	vaddr_t va, eva;

	va = MIPS_PHYS_TO_KSEG0(0);
	eva = va + mips_pdcache_size;

	/*
	 * Since we're hitting the whole thing, we don't have to
	 * worry about the N different "ways".
	 */

	while (va < eva) {
		cache_r4k_op_32lines_32(va,
		    CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
		va += (32 * 32);
	}

	SYNC;
}

void
mipsNN_pdcache_wbinv_range_16(vaddr_t va, vsize_t size)
{
	vaddr_t eva;

	va = trunc_line16(va);
	eva = round_line16(va + size);

	while ((eva - va) >= (32 * 16)) {
		cache_r4k_op_32lines_16(va,
		    CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va += (32 * 16);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va += 16;
	}

	SYNC;
}

void
mipsNN_pdcache_wbinv_range_32(vaddr_t va, vsize_t size)
{
	vaddr_t eva;

#ifdef SB1250_PASS1
	mipsNN_pdcache_wbinv_range_32_sb1(va, size);
	return;
#endif
	va = trunc_line32(va);
	eva = round_line32(va + size);

	while ((eva - va) >= (32 * 32)) {
		cache_r4k_op_32lines_32(va,
		    CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va += (32 * 32);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va += 32;
	}

	SYNC;
}

void
mipsNN_pdcache_wbinv_range_index_16_2way(vaddr_t va, vsize_t size)
{
	vaddr_t w2va, eva;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & mips_pdcache_way_mask);

	va = trunc_line16(va);
	w2va = va + mips_pdcache_way_size;
	eva = round_line16(va + size);

	while ((eva - va) >= (16 * 16)) {
		cache_r4k_op_16lines_16_2way(va, w2va,
		    CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va   += (16 * 16);
		w2va += (16 * 16);
	}

	while (va < eva) {
		cache_op_r4k_line(va,   CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		cache_op_r4k_line(w2va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va   += 16;
		w2va += 16;
	}

	SYNC;
}

void
mipsNN_pdcache_wbinv_range_index_16_4way(vaddr_t va, vsize_t size)
{
	vaddr_t w2va, w3va, w4va, eva;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & mips_pdcache_way_mask);

	va = trunc_line16(va);
	w2va = va   + mips_pdcache_way_size;
	w3va = w2va + mips_pdcache_way_size;
	w4va = w3va + mips_pdcache_way_size;
	eva = round_line16(va + size);

	while ((eva - va) >= (8 * 16)) {
		cache_r4k_op_8lines_16_4way(va, w2va, w3va, w4va,
		    CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va   += (8 * 16);
		w2va += (8 * 16);
		w3va += (8 * 16);
		w4va += (8 * 16);
	}

	while (va < eva) {
		cache_op_r4k_line(va,   CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		cache_op_r4k_line(w2va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		cache_op_r4k_line(w3va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		cache_op_r4k_line(w4va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va   += 16;
		w2va += 16;
		w3va += 16;
		w4va += 16;
	}

	SYNC;
}

void
mipsNN_pdcache_wbinv_range_index_32_2way(vaddr_t va, vsize_t size)
{
	vaddr_t w2va, eva;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & mips_pdcache_way_mask);

	va = trunc_line32(va);
	w2va = va + mips_pdcache_way_size;
	eva = round_line32(va + size);

	while ((eva - va) >= (16 * 32)) {
		cache_r4k_op_16lines_32_2way(va, w2va,
		    CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va   += (16 * 32);
		w2va += (16 * 32);
	}

	while (va < eva) {
		cache_op_r4k_line(va,   CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		cache_op_r4k_line(w2va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va   += 32;
		w2va += 32;
	}

	SYNC;
}
 
void
mipsNN_pdcache_wbinv_range_index_32_4way(vaddr_t va, vsize_t size)
{
	vaddr_t w2va, w3va, w4va, eva;

#ifdef SB1250_PASS1
	mipsNN_pdcache_wbinv_range_index_32_4way_sb1(va, size);
	return;
#endif
	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & mips_pdcache_way_mask);

	va = trunc_line32(va);
	w2va = va   + mips_pdcache_way_size;
	w3va = w2va + mips_pdcache_way_size;
	w4va = w3va + mips_pdcache_way_size;
	eva = round_line32(va + size);

	while ((eva - va) >= (8 * 32)) {
		cache_r4k_op_8lines_32_4way(va, w2va, w3va, w4va,
		    CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va   += (8 * 32);
		w2va += (8 * 32);
		w3va += (8 * 32);
		w4va += (8 * 32);
	}

	while (va < eva) {
		cache_op_r4k_line(va,   CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		cache_op_r4k_line(w2va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		cache_op_r4k_line(w3va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		cache_op_r4k_line(w4va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va   += 32;
		w2va += 32;
		w3va += 32;
		w4va += 32;
	}

	SYNC;
}
 
void
mipsNN_pdcache_inv_range_16(vaddr_t va, vsize_t size)
{
	vaddr_t eva;

	va = trunc_line16(va);
	eva = round_line16(va + size);

	while ((eva - va) >= (32 * 16)) {
		cache_r4k_op_32lines_16(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);
		va += (32 * 16);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);
		va += 16;
	}

	SYNC;
}

void
mipsNN_pdcache_inv_range_32(vaddr_t va, vsize_t size)
{
	vaddr_t eva;

	va = trunc_line32(va);
	eva = round_line32(va + size);

	while ((eva - va) >= (32 * 32)) {
		cache_r4k_op_32lines_32(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);
		va += (32 * 32);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);
		va += 32;
	}

	SYNC;
}

void
mipsNN_pdcache_wb_range_16(vaddr_t va, vsize_t size)
{
	vaddr_t eva;

	va = trunc_line16(va);
	eva = round_line16(va + size);

	while ((eva - va) >= (32 * 16)) {
		cache_r4k_op_32lines_16(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
		va += (32 * 16);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
		va += 16;
	}

	SYNC;
}

void
mipsNN_pdcache_wb_range_32(vaddr_t va, vsize_t size)
{
	vaddr_t eva;

	va = trunc_line32(va);
	eva = round_line32(va + size);

	while ((eva - va) >= (32 * 32)) {
		cache_r4k_op_32lines_32(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
		va += (32 * 32);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
		va += 32;
	}

	SYNC;
}

#ifdef SB1250_PASS1
#define	cache_r4k_op_line_load_off(va, off, op)				\
	__asm __volatile("lw $0, %1(%0); sync; cache %2, %1(%0)"	\
	:								\
	: "r" (va), "i" (off), "i" (op)					\
	: "memory")

#define	cache_r4k_op_32_4way_load_off(va1, va2, va3, va4, off, op)	\
do {									\
	cache_r4k_op_line_load_off((va ), (off), (op));			\
	cache_r4k_op_line_load_off((va2), (off), (op));			\
	cache_r4k_op_line_load_off((va1), (off), (op));			\
	cache_r4k_op_line_load_off((va3), (off), (op));			\
} while (/*CONSTCOND*/0)

#define	cache_r4k_op_8lines_32_4way_load(va1, va2, va3, va4, op)	\
do {									\
	asm __volatile(".set noreorder");				\
	cache_r4k_op_32_4way_load_off((va1), (va2), (va3), (va4), 0x00, (op)); \
	cache_r4k_op_32_4way_load_off((va1), (va2), (va3), (va4), 0x20, (op)); \
	cache_r4k_op_32_4way_load_off((va1), (va2), (va3), (va4), 0x40, (op)); \
	cache_r4k_op_32_4way_load_off((va1), (va2), (va3), (va4), 0x60, (op)); \
	cache_r4k_op_32_4way_load_off((va1), (va2), (va3), (va4), 0x80, (op)); \
	cache_r4k_op_32_4way_load_off((va1), (va2), (va3), (va4), 0xa0, (op)); \
	cache_r4k_op_32_4way_load_off((va1), (va2), (va3), (va4), 0xc0, (op)); \
	cache_r4k_op_32_4way_load_off((va1), (va2), (va3), (va4), 0xe0, (op)); \
	asm __volatile(".set reorder");					\
} while (/*CONSTCOND*/0)

#define	cache_r4k_op_32lines_32_load(va, op)				\
do {									\
	__asm __volatile(".set noreorder");				\
	cache_r4k_op_line_load_off((va), 0x000, (op));			\
	cache_r4k_op_line_load_off((va), 0x020, (op));			\
	cache_r4k_op_line_load_off((va), 0x040, (op));			\
	cache_r4k_op_line_load_off((va), 0x060, (op));			\
	cache_r4k_op_line_load_off((va), 0x080, (op));			\
	cache_r4k_op_line_load_off((va), 0x0a0, (op));			\
	cache_r4k_op_line_load_off((va), 0x0c0, (op));			\
	cache_r4k_op_line_load_off((va), 0x0e0, (op));			\
	cache_r4k_op_line_load_off((va), 0x100, (op));			\
	cache_r4k_op_line_load_off((va), 0x120, (op));			\
	cache_r4k_op_line_load_off((va), 0x140, (op));			\
	cache_r4k_op_line_load_off((va), 0x160, (op));			\
	cache_r4k_op_line_load_off((va), 0x180, (op));			\
	cache_r4k_op_line_load_off((va), 0x1a0, (op));			\
	cache_r4k_op_line_load_off((va), 0x1c0, (op));			\
	cache_r4k_op_line_load_off((va), 0x1e0, (op));			\
	cache_r4k_op_line_load_off((va), 0x200, (op));			\
	cache_r4k_op_line_load_off((va), 0x220, (op));			\
	cache_r4k_op_line_load_off((va), 0x240, (op));			\
	cache_r4k_op_line_load_off((va), 0x260, (op));			\
	cache_r4k_op_line_load_off((va), 0x280, (op));			\
	cache_r4k_op_line_load_off((va), 0x2a0, (op));			\
	cache_r4k_op_line_load_off((va), 0x2c0, (op));			\
	cache_r4k_op_line_load_off((va), 0x2e0, (op));			\
	cache_r4k_op_line_load_off((va), 0x300, (op));			\
	cache_r4k_op_line_load_off((va), 0x320, (op));			\
	cache_r4k_op_line_load_off((va), 0x340, (op));			\
	cache_r4k_op_line_load_off((va), 0x360, (op));			\
	cache_r4k_op_line_load_off((va), 0x380, (op));			\
	cache_r4k_op_line_load_off((va), 0x3a0, (op));			\
	cache_r4k_op_line_load_off((va), 0x3c0, (op));			\
	cache_r4k_op_line_load_off((va), 0x3e0, (op));			\
	__asm __volatile(".set reorder");				\
} while (/*CONSTCOND*/0)

#define	cache_r4k_op_line_load(va, op)	cache_r4k_op_line_load_off(va, 0, op)


void
mipsNN_pdcache_wbinv_range_32_sb1(vaddr_t va, vsize_t size)
{
	vaddr_t eva;
	int s;

	va = trunc_line32(va);
	eva = round_line32(va + size);

	s = splhigh();
	while ((eva - va) >= (32 * 32)) {
		cache_r4k_op_32lines_32_load(va,
		    CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va += (32 * 32);
	}

	while (va < eva) {
		cache_r4k_op_line_load(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va += 32;
	}

	SYNC;
	splx(s);
}

void
mipsNN_pdcache_wbinv_range_index_32_4way_sb1(vaddr_t va, vsize_t size)
{
	vaddr_t w2va, w3va, w4va, eva;
	int s;

#if 1
	mipsNN_pdcache_wbinv_all_32(); return;	/* XXX */
#endif
	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & mips_pdcache_way_mask);

	va = trunc_line32(va);
	w2va = va + mips_pdcache_way_size;
	w3va = w2va + mips_pdcache_way_size;
	w4va = w3va + mips_pdcache_way_size;
	eva = round_line32(va + size);

	s = splhigh();
	while ((eva - va) >= (8 * 32)) {
		cache_r4k_op_8lines_32_4way_load(va, w2va, w3va, w4va,
		    CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va   += (8 * 32);
		w2va += (8 * 32);
		w3va += (8 * 32);
		w4va += (8 * 32);
	}

	while (va < eva) {
		cache_r4k_op_line_load(va,   CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		cache_r4k_op_line_load(w2va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		cache_r4k_op_line_load(w3va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		cache_r4k_op_line_load(w4va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va   += 32;
		w2va += 32;
		w3va += 32;
		w4va += 32;
	}

	SYNC;
	splx(s);
}
#endif /* SB1250_PASS1 */
