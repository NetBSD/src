/*	$NetBSD: mmu.c,v 1.1 1997/03/13 17:52:52 gwr Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jeremy Cooper.
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

/*
 * Standalone functions for manipulating the Sun3x MMU.
 */

#include <sys/param.h>
#include <machine/pte.h>
#include <machine/mon.h>

/* XXX - I'll let Jeremy deal with this. -gwr */
#define	MON_LOMEM_BASE	0
#define	MON_LOMEM_SIZE	0x400000
#define MON_KDB_BASE	MON_KDB_START

void mmu_atc_flush __P((u_int32_t));

void
set_pte(va, pa)
	u_int32_t va;	/* virt. address */
	u_int32_t pa;	/* phys. address */
{
	u_int	pn;
	mmu_short_pte_t *tbl;

	if (va >= MON_LOMEM_BASE && va < (MON_LOMEM_BASE + MON_LOMEM_SIZE)) {
		/*
		 * Main memory range.
		 */
		tbl = (mmu_short_pte_t *) *romVectorPtr->lomemptaddr;
	} else if (va >= MON_KDB_BASE && va < (MON_KDB_BASE + MON_KDB_SIZE)) {
		/*
		 * Kernel Debugger range.
		 */
		va -= MON_KDB_BASE;
		tbl = (mmu_short_pte_t *) *romVectorPtr->monptaddr;
	} else if (va >= MON_DVMA_BASE) {
		/*
		 * DVMA range.
		 */
		va -= MON_DVMA_BASE;
		tbl = (mmu_short_pte_t *) *romVectorPtr->shadowpteaddr;
	} else {
		/* invalid range */
		return;
	}

	/* Calculate the page number within the selected table. */
	pn = (va >> MMU_PAGE_SHIFT);
	/* Enter the PTE into the table. */
	tbl[pn].attr.raw = pa;
	/* Flush the ATC of any cached entries for the va. */
	mmu_atc_flush(va);
}

u_int32_t
get_pte(va)
	u_int32_t va;	/* virt. address */
{
	u_int	pn;
	mmu_short_pte_t *tbl;

	if (va >= MON_LOMEM_BASE && va < (MON_LOMEM_BASE + MON_LOMEM_SIZE)) {
		tbl = (mmu_short_pte_t *) *romVectorPtr->lomemptaddr;
	} else if (va >= MON_KDB_BASE && va < (MON_KDB_BASE + MON_KDB_SIZE)) {
		va -= MON_KDB_BASE;
		tbl = (mmu_short_pte_t *) *romVectorPtr->monptaddr;
	} else if (va >= MON_DVMA_BASE) {
		va -= MON_DVMA_BASE;
		tbl = (mmu_short_pte_t *) *romVectorPtr->shadowpteaddr;
	} else {
		return 0;
	}

	/* Calculate the page number within the selected table. */
	pn = (va >> MMU_PAGE_SHIFT);
	/* Extract the PTE from the table. */
	return tbl[pn].attr.raw;
}
