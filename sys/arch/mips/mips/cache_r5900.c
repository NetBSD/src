/*	$NetBSD: cache_r5900.c,v 1.2.4.3 2002/03/16 15:58:37 jdolecek Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/param.h>

#include <mips/cpuregs.h>
#include <mips/cache.h>
#include <mips/cache_r5900.h>
#include <mips/locore.h>

/*
 * Cache operations for R5900-style caches:
 *	I-cache 16KB/64B 2-way assoc.	 
 *	D-cache 8KB/64B 2-way assoc.
 *	No L2-cache.
 *	and sync.p/sync.l are needed after/before cache instruction.
 *
 */

#define	round_line(x)		(((x) + 63) & ~63)
#define	trunc_line(x)		((x) & ~63)

void
r5900_icache_sync_all_64()
{
	vaddr_t va = MIPS_PHYS_TO_KSEG0(0);
	vaddr_t eva = va + (CACHE_R5900_SIZE_I >> 1); /* 2way */
	int s;

	mips_dcache_wbinv_all();

	s = _intr_suspend();
	while (va < eva) {
		cache_r5900_op_4lines_64_2way(va, CACHEOP_R5900_IINV_I);
		va += (4 * 64);
	}
	_intr_resume(s);
}

void
r5900_icache_sync_range_64(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);
	int s;

	va = trunc_line(va);

	mips_dcache_wb_range(va, (eva - va));

	s = _intr_suspend();
	while ((eva - va) >= (4 * 64)) {
		cache_r5900_op_4lines_64(va, CACHEOP_R5900_HINV_I);
		va += (4 * 64);
	}

	while (va < eva) {
		cache_op_r5900_line_64(va, CACHEOP_R5900_HINV_I);
		va += 64;
	}
	_intr_resume(s);
}

void
r5900_icache_sync_range_index_64(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);
	int s;

	va = trunc_line(va);

	mips_dcache_wbinv_range_index(va, (eva - va));

	s = _intr_suspend();
	while ((eva - va) >= (4 * 64)) {
		cache_r5900_op_4lines_64_2way(va, CACHEOP_R5900_IINV_I);
		va += (4 * 64);
	}

	while (va < eva) {
		/* way 0 */
		cache_op_r5900_line_64(va, CACHEOP_R5900_IINV_I);
		/* way 1 */
		cache_op_r5900_line_64(va + 1, CACHEOP_R5900_IINV_I);
		va += 64;
	}
	_intr_resume(s);
}

void
r5900_pdcache_wbinv_all_64()
{
	vaddr_t va = MIPS_PHYS_TO_KSEG0(0);
	vaddr_t eva = va + (CACHE_R5900_SIZE_D >> 1); /* 2way */
	int s;

	s = _intr_suspend();
	while (va < eva) {
		cache_r5900_op_4lines_64_2way(va, CACHEOP_R5900_IWBINV_D);
		va += (4 * 64);
	}
	_intr_resume(s);
}

void
r5900_pdcache_wbinv_range_64(vaddr_t va, vsize_t size)
{
	vaddr_t eva;
	int s;
	
	eva = round_line(va + size);
	va = trunc_line(va);

	s = _intr_suspend();
	while ((eva - va) >= (4 * 64)) {
		cache_r5900_op_4lines_64(va, CACHEOP_R5900_HWBINV_D);
		va += (4 * 64);
	}

	while (va < eva) {
		cache_op_r5900_line_64(va, CACHEOP_R5900_HWBINV_D);
		va += 64;
	}
	_intr_resume(s);
}

void
r5900_pdcache_wbinv_range_index_64(vaddr_t va, vsize_t size)
{
	vaddr_t eva;
	int s;
	
	eva = round_line(va + size);
	va = trunc_line(va);

	s = _intr_suspend();
	while ((eva - va) >= (4 * 64)) {
		cache_r5900_op_4lines_64_2way(va, CACHEOP_R5900_IWBINV_D);
		va += (4 * 64);
	}

	while (va < eva) {
		/* way 0 */
		cache_op_r5900_line_64(va, CACHEOP_R5900_IWBINV_D);
		/* way 1 */
		cache_op_r5900_line_64(va + 1, CACHEOP_R5900_IWBINV_D);
		va += 64;
	}
	_intr_resume(s);
}

void
r5900_pdcache_inv_range_64(vaddr_t va, vsize_t size)
{
	vaddr_t eva;
	int s;
	
	eva = round_line(va + size);
	va = trunc_line(va);

	s = _intr_suspend();
	while ((eva - va) >= (4 * 64)) {
		cache_r5900_op_4lines_64(va, CACHEOP_R5900_HINV_D);
		va += (4 * 64);
	}

	while (va < eva) {
		cache_op_r5900_line_64(va, CACHEOP_R5900_HINV_D);
		va += 64;
	}
	_intr_resume(s);
}

void
r5900_pdcache_wb_range_64(vaddr_t va, vsize_t size)
{
	vaddr_t eva;
	int s;
	
	eva = round_line(va + size);
	va = trunc_line(va);

	s = _intr_suspend();
	while ((eva - va) >= (4 * 64)) {
		cache_r5900_op_4lines_64(va, CACHEOP_R5900_HWB_D);
		va += (4 * 64);
	}

	while (va < eva) {
		cache_op_r5900_line_64(va, CACHEOP_R5900_HWB_D);
		va += 64;
	}
	_intr_resume(s);
}
