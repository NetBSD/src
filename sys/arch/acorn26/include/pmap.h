/* $NetBSD: pmap.h,v 1.9.24.1 2017/12/03 11:35:44 jdolecek Exp $ */
/*-
 * Copyright (c) 1997, 1998 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * machine/pmap.h - Machine-dependent pmap stuff from MEMC1A
 */

#ifndef _ARM26_PMAP_H
#define _ARM26_PMAP_H

#ifdef _KERNEL
#include <machine/memcreg.h>

extern void pmap_bootstrap(int, paddr_t);

extern bool pmap_fault(pmap_t, vaddr_t, vm_prot_t);

/* These have to be macros, whatever pmap(9) says. */
#define pmap_resident_count(pmap)	_pmap_resident_count(pmap)
#define pmap_wired_count(pmap)		_pmap_wired_count(pmap)

extern long _pmap_resident_count(pmap_t);
extern long _pmap_wired_count(pmap_t);

static __inline void
pmap_remove_all(struct pmap *pmap)
{
	/* Nothing. */
}

/* Save on hassle and kernel VM */
#define PMAP_MAP_POOLPAGE(pa)	((vaddr_t)MEMC_PHYS_BASE + (pa))
#define PMAP_UNMAP_POOLPAGE(va)	((va) - (vaddr_t)MEMC_PHYS_BASE)
#define PMAP_STEAL_MEMORY

#define UVM_PHYSSEG_LEGACY /* for uvm_physseg_set_avail_start */

#endif /* _KERNEL */

#endif
