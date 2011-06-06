/*	$NetBSD: pmap.h,v 1.1.4.1 2011/06/06 09:06:28 jruoho Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
#ifndef _POWERPC_BOOKE_PMAP_H_
#define _POWERPC_BOOKE_PMAP_H_

#ifdef _LOCORE
#error use assym.h instead
#endif

#include <sys/cpu.h>
#include <sys/kcore.h>
#include <uvm/uvm_page.h>
#ifdef __PMAP_PRIVATE
#include <powerpc/booke/cpuvar.h>
#include <powerpc/cpuset.h>
#endif

#define	PMAP_MD_NOCACHE		0x01000000
#define	PMAP_NEED_PROCWR

#include <powerpc/booke/pte.h>

#define	NBSEG		(NBPG*NPTEPG)
#define	SEGSHIFT	(PGSHIFT + PGSHIFT - 2)
#define SEGOFSET	((1 << SEGSHIFT) - 1)
#define PMAP_SEGTABSIZE	(1 << (32 - SEGSHIFT))
#define	NPTEPG		(NBPG >> 2)

#define	KERNEL_PID	0

#define	PMAP_TLB_NUM_PIDS		256
#define	PMAP_INVALID_SEGTAB_ADDRESS	((struct pmap_segtab *)0xfeeddead)

#ifndef _LOCORE
#define	pmap_phys_address(x)		(x)

void	pmap_procwr(struct proc *, vaddr_t, size_t);
#define	PMAP_NEED_PROCWR

struct vm_page *
	pmap_md_alloc_poolpage(int flags);
vaddr_t	pmap_md_map_poolpage(paddr_t);
bool	pmap_md_direct_mapped_vaddr_p(vaddr_t);
bool	pmap_md_io_vaddr_p(vaddr_t);
paddr_t	pmap_md_direct_mapped_vaddr_to_paddr(vaddr_t);
vaddr_t	pmap_md_direct_map_paddr(paddr_t);
void	pmap_md_init(void);

void	pmap_md_page_syncicache(struct vm_page *, __cpuset_t);
void	pmap_bootstrap(vaddr_t, vaddr_t, const phys_ram_seg_t *, size_t);
bool	pmap_extract(struct pmap *, vaddr_t, paddr_t *);

static inline paddr_t vtophys(vaddr_t);

static inline paddr_t
vtophys(vaddr_t va)
{
	paddr_t pa;

	if (pmap_extract(pmap_kernel(), va, &pa))
		return pa;
	KASSERT(0);
	return (paddr_t) -1;
}

/*
 * Virtual Cache Alias helper routines.  Not a problem for Booke CPUs.
 */
static inline bool
pmap_md_vca_add(struct vm_page *pg, vaddr_t va, pt_entry_t *nptep)
{
	return false;
}

static inline void
pmap_md_vca_remove(struct vm_page *pg, vaddr_t va)
{

}

static inline void
pmap_md_vca_clean(struct vm_page *pg, vaddr_t va, int op)
{
}

#define	POOL_VTOPHYS(va)	((paddr_t)(vaddr_t)(va))
#define	POOL_PHYSTOV(pa)	((vaddr_t)(paddr_t)(pa))

#include <common/pmap/tlb/pmap.h>
#endif /* _LOCORE */

#endif /* !_POWERPC_BOOKE_PMAP_H_ */
