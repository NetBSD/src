/*	$NetBSD: pmap_subr.c,v 1.1 2002/07/17 03:11:09 matt Exp $	*/
/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com> of Allegro Networks, Inc.
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
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>
#ifdef PPC_MPC6XX
#include <powerpc/mpc6xx/vmparam.h>
#endif
#include <powerpc/psl.h>

#define	ISYNC()		__asm __volatile("isync")
#define	MFMSR()		mfmsr()
#define	MTMSR(psl)	__asm __volatile("mtmsr %0" :: "r"(psl))

static __inline u_int32_t
mfmsr(void)
{
	u_int32_t psl;
	__asm __volatile("mfmsr %0" : "=r"(psl) : );
	return psl;
}

/*
 * This file uses a sick & twisted method to deal with the common pmap
 * operations of zero'ing, copying, and syncing the page with the
 * instruction cache.
 *
 * When a PowerPC CPU takes an exception (interrupt or trap), that 
 * exception is handled with the MMU off.  The handler has to explicitly
 * renable the MMU before continuing.  The state of the MMU will be restored
 * when the exception is returned from.
 *
 * Therefore if we disable the MMU we know that doesn't affect any exception.
 * So it's safe for us to disable the MMU so we can deal with physical
 * addresses without having to map any pages via a BAT or into a page table.
 *
 * It's also safe to do regardless of IPL.
 */

/*
 * Fill the given physical page with zeroes.
 */
void
pmap_zero_page(paddr_t pa)
{
	u_int32_t msr;

#ifdef PPC_MPC6XX
	if (pa < SEGMENT_LENGTH) {
		memset((caddr_t) pa, 0, NBPG);
		return;
	}
#endif

	/*
	 * Turn off data relocation (DMMU off).
	 */
	msr = MFMSR();
	MTMSR(msr & ~PSL_DR);
	ISYNC();

	/*
	 * Zero the page
	 */
	memset((caddr_t) pa, 0, NBPG);

	/*
	 * Restore data relocation (DMMU on).
	 */
	MTMSR(msr);
	ISYNC();
}

/*
 * Copy the given physical source page to its destination.
 */
void
pmap_copy_page(paddr_t src, paddr_t dst)
{
	u_int32_t msr;

#ifdef PPC_MPC6xx
	if (src < SEGMENT_LENGTH && dst < SEGMENT_LENGTH) {
		/*
		 * Copy the page
		 */
		memcpy((void *) dst, (void *) src, NBPG);
		return;
	}
#endif

	/*
	 * Turn off data relocation (DMMU off).
	 */
	msr = MFMSR();
	MTMSR(msr & ~PSL_DR);
	ISYNC();

	/*
	 * Copy the page
	 */
	memcpy((void *) dst, (void *) src, NBPG);

	/*
	 * Restore data relocation (DMMU on).
	 */
	MTMSR(msr);
	ISYNC();
}

void
pmap_syncicache(paddr_t pa, psize_t len)
{
	u_int32_t msr;

#ifdef PPC_MPC6XX
	if (pa + len <= SEGMENT_LENGTH) {
		__syncicache((void *)pa, len);
		return;
	}
#endif

	/*
	 * Turn off instruction and data relocation (MMU off).
	 */
	msr = MFMSR();
	MTMSR(msr & ~(PSL_IR|PSL_DR));
	ISYNC();

	/*
	 * Sync the instruction cache
	 */
	__syncicache((void *)pa, len);

	/*
	 * Restore relocation (MMU on).
	 */
	MTMSR(msr);
	ISYNC();
}

boolean_t
pmap_pageidlezero(paddr_t pa)
{
	u_int32_t msr;
	u_int32_t *dp = (u_int32_t *) pa;
	boolean_t rv = TRUE;
	int i;

#ifdef PPC_MPC6XX
	if (pa < SEGMENT_LENGTH) {
		for (i = 0; i < NBPG / sizeof(dp[0]); i++) {
			if (sched_whichqs != 0)
				return FALSE;
			*dp++ = 0;
		}
		return TRUE;
	}
#endif

	/*
	 * Turn off instruction and data relocation (MMU off).
	 */
	msr = MFMSR();
	MTMSR(msr & ~(PSL_IR|PSL_DR));
	ISYNC();

	/*
	 * Zero the page until a process becomes runnable.
	 */
	for (i = 0; i < NBPG / sizeof(dp[0]); i++) {
		if (sched_whichqs != 0) {
			rv = FALSE;
			break;
		}
		*dp++ = 0;
	}

	/*
	 * Restore relocation (MMU on).
	 */
	MTMSR(msr);
	ISYNC();
	return rv;
}
