/*	$NetBSD: footbridge_machdep.c,v 1.3.6.5 2002/04/01 07:39:09 nathanw Exp $	*/

/*
 * Copyright (c) 1997 Mark Brinicombe.
 * Copyright (c) 1997 Causality Limited
 * All rights reserved.
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <uvm/uvm_extern.h>
#include <machine/pmap.h>
#include <arm/footbridge/footbridge.h>
#include <arm/footbridge/dc21285mem.h>

/*
 * For optimal cache cleaning we need two 16K banks of
 * virtual address space that NOTHING else will access
 * and then we alternate the cache cleaning between the
 * two banks.
 * The cache cleaning code requires requires 2 banks aligned
 * on total size boundry so the banks can be alternated by
 * eorring the size bit (assumes the bank size is a power of 2)
 *
 * On the DC21285 we have a special cache clean area so we will
 * use it.
 */

extern unsigned int sa110_cache_clean_addr;
extern unsigned int sa110_cache_clean_size;

#if 0
void
footbridge_sa110_cc_setup(void)
{
	vm_offset_t vaddr;
	vm_offset_t addr;
	int cleanarea;
	int loop;
	pt_entry_t *pte;

	cleanarea = (NBPG * 4) * 2;
	vaddr = uvm_km_valloc(kernel_map, cleanarea + (NBPG * 4));
	addr = (vaddr + (cleanarea - 1)) & ~(cleanarea - 1);

/*	printf("vaddr=%x addr=%x\n", vaddr, addr);*/

	for (loop = 0; loop < cleanarea; loop += NBPG) {
		pte = vtopte(addr + loop);
		*pte = L2_PTE(DC21285_SA_CACHE_FLUSH_BASE + loop, AP_KR);
	}
	sa110_cache_clean_addr = addr;
	sa110_cache_clean_size = cleanarea / 2;
}
#else
void
footbridge_sa110_cc_setup(void)
{
	sa110_cache_clean_addr = DC21285_CACHE_FLUSH_VBASE;
	sa110_cache_clean_size = (NBPG * 4);
}
#endif
