/*	$NetBSD: pmap.c,v 1.33 2003/05/10 21:10:38 thorpej Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
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

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_ddb.h"
#include "opt_kernel_ipt.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/pool.h>
#include <sys/user.h>
#include <sys/queue.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm.h>

#include <machine/cpu.h>
#include <machine/pcb.h>
#include <machine/memregion.h>
#include <machine/cacheops.h>


#undef PMAP_DIAG
#ifdef PMAP_DIAG
#ifndef DDB
#define pmap_debugger()	panic("")
#else
#include <machine/db_machdep.h>
#define	pmap_debugger() asm volatile("brk");
int validate_kipt(int);
#endif
#endif

#ifdef DEBUG
static int pmap_debug = 0;
#define	PMPRINTF(x)	do { if (pmap_debug) printf x; } while (0)
static const char * PMSTR(pmap_t);
static const char *
PMSTR(pmap_t pm)
{
	static char pm_str[32];
	if (pm == pmap_kernel())
		return("KERNEL");
	sprintf(pm_str, "%p", pm);
	return (pm_str);
}
#else
#define	PMPRINTF(x)
#endif


/*
 * The basic SH5 MMU is just about as simple as these things can be,
 * having two 64-entry TLBs; one for Data and one for Instructions.
 *
 * That's it. End of story. Move along now, nothing more to see.
 *
 * According to the SH5 architecture manual, future silicon may well
 * expand the number of TLB slots, and the number of bits allocated to
 * various fields within the PTEs. Conversely, future silicon may also
 * scrap the separate instruction and data TLBs. Regardless, we defer
 * TLB manipulation to a cpu-specific backend so these details shouldn't
 * affect this pmap.
 * 
 * Given the limited options, the following VM layout is used:
 *
 * 32-bit kernel:
 * ~~~~~~~~~~~~~~
 * 00000000 - BFFFFFFF
 * 	User virtual address space.
 * 
 * 	[DI]TLB misses are looked up in a single, user Inverted Page
 * 	Table Group (pmap_pteg_table) using a hash of the VPN and VSID.
 * 
 * 	The IPT will be big enough to contain one PTEG for every 2 physical
 * 	pages of RAM in the system.
 * 
 * 	PTEG entries will contain 8 PTEs, consisting of PTEL and PTEH
 *	configuration register values (I.e, the bits which contain the
 *	EPN and PPN, size and protection/cache bits), and a VSID which is
 *	used in conjunction with the EPN to uniquely identify any user
 *	mapping in the system.
 * 
 * 	Hash collisions which overflow the 8 PTE group entries per bucket
 *	are dealt with using Overflow PTs.
 *
 *	For a typical NEFF=32 machine with 128MB RAM, the pmap_pteg_table will
 *	require 1.5MB of wired RAM.
 * 
 * C0000000 - DFFFFFFF  (KSEG0)
 *	The kernel lives here.
 *
 * 	Maps first 512MB of physical RAM, with an unused hole at the
 * 	top where sizeof(physical RAM) < 512MB.
 *
 * 	Uses one locked ITLB and DTLB slot for kernel text/data/bss.
 * 	The PTEH.SH bit will be set in these slots so that the kernel
 * 	VM space is mapped regardless of ASID, albeit with supervisor
 * 	access only.
 * 
 *	The size of this mapping is fixed at 512MB since that is the
 *	only suitable size available in the SH5 PTEs.
 *
 *	Note that this is effectively an unmanaged, wired mapping of
 *	the first hunk (or more likely, all) of physical RAM. As such,
 *	we can treat it like a direct-mapped memory segment from which
 *	to allocate backing pages for the pool(9) memory pool manager.
 *	This should *greatly* reduce TLB pressure for many kernel
 *	data structures.
 *
 *	Where physical memory is larger than 512MB, we can still try
 *	to allocate from this segment, and failing that, fall back to
 *	the traditional way.
 * 
 * E0000000 - FFFFFFFF  (KSEG1)
 * 	KSEG1 is basically the `managed' kernel virtual address space
 *	as reported to uvm(9) by pmap_virtual_space().
 * 
 * 	It uses regular TLB mappings, but backed by a dedicated IPT
 * 	with 1-1 V2Phys PTELs. The IPT will be up to 512KB (for NEFF=32)
 *	in size and allocated from KSEG0. IPT entries will be handled in
 *	the same way as the user IPT except for the PTEH, which will be
 * 	synthesised in the TLB handler. No VSID is required.
 *
 * 	Yes, this means device register access can cause a DTLB miss.
 *	In fact, accessing the kernel stack, even while dealing with
 *	an interrupt/exception, can cause a TLB miss!
 *
 * 	However, due to the dedicated 1-1 IPT, these should be handled
 * 	*very* quickly as we won't need to hash the VPN/VSID and deal
 * 	with hash collisions.
 * 
 * 	To reduce TLB misses on hardware registers, it will be possible to
 * 	request mappings which use a larger page size than the default
 *	4KB.
 *
 *	Also note that by reducing the size of managed KVA in KSEG1 from
 *	the default of 512MB to, say 128MB, the IPT size drops to 128KB.
 *
 * 64-bit kernel:
 * ~~~~~~~~~~~~~~
 * TBD.
 */

/*
 * The Primary IPT consists of an array of Hash Buckets, called PTE Groups,
 * where each group is 8 PTEs in size. The number of groups is calculated
 * at boot time such that there is one group for every two PAGE_SIZE-sized
 * pages of physical RAM.
 */
pteg_t *pmap_pteg_table;	/* Primary IPT group */
int pmap_pteg_cnt;		/* Number of PTE groups. A power of two */
u_int pmap_pteg_mask;		/* Basically, just (pmap_pteg_cnt - 1) */
u_int pmap_pteg_bits;		/* Number of bits set in pmap_pteg_mask */

/*
 * The kernel IPT is an array of kpte_t structures, indexed by virtual page
 * number.
 */
kpte_t pmap_kernel_ipt[KERNEL_IPT_SIZE];

/*
 * This structure serves a double purpose as over-flow table entry
 * and for tracking phys->virt mappings.
 */
struct pvo_entry {
	LIST_ENTRY(pvo_entry) pvo_vlink;	/* Link to common virt page */
	LIST_ENTRY(pvo_entry) pvo_olink;	/* Link to overflow entry */
	pmap_t pvo_pmap;			/* ptr to owning pmap */
	vaddr_t pvo_vaddr;			/* VA of entry */
#define	PVO_PTEGIDX_MASK	0x0007		/* which PTEG slot */
#define	PVO_PTEGIDX_VALID	0x0008		/* slot is valid */
#define	PVO_WIRED		0x0010		/* PVO entry is wired */
#define	PVO_MANAGED		0x0020		/* PVO e. for managed page */
#define	PVO_WRITABLE		0x0040		/* PVO e. for writable page */
#define	PVO_CACHEABLE		0x0100		/* PVO e. for cacheable page */
/* Note: These three fields must match the values for SH5_PTEL_[RM] */
#define	PVO_REFERENCED		0x0400		/* Cached referenced bit */
#define	PVO_MODIFIED		0x0800		/* Cached modified bit */
#define	PVO_REFMOD_MASK		0x0c00

	ptel_t pvo_ptel;			/* PTEL for this mapping */
};

#define	PVO_VADDR(pvo)		(sh5_trunc_page((pvo)->pvo_vaddr))
#define	PVO_ISEXECUTABLE(pvo)	(((pvo)->pvo_ptel & SH5_PTEL_PR_X)!=0)
#define	PVO_ISWIRED(pvo)	(((pvo)->pvo_vaddr & PVO_WIRED)!=0)
#define	PVO_ISMANAGED(pvo)	(((pvo)->pvo_vaddr & PVO_MANAGED)!=0)
#define	PVO_ISWRITABLE(pvo)	(((pvo)->pvo_vaddr & PVO_WRITABLE)!=0)
#define	PVO_ISCACHEABLE(pvo)	(((pvo)->pvo_vaddr & PVO_CACHEABLE)!=0)
#define	PVO_PTEGIDX_ISSET(pvo)	(((pvo)->pvo_vaddr & PVO_PTEGIDX_VALID)!=0)
#define	PVO_PTEGIDX_GET(pvo)	((pvo)->pvo_vaddr & PVO_PTEGIDX_MASK)
#define	PVO_PTEGIDX_CLR(pvo)	\
	((void)((pvo)->pvo_vaddr &= ~(PVO_PTEGIDX_VALID|PVO_PTEGIDX_MASK)))
#define	PVO_PTEGIDX_SET(pvo,i)	\
	((void)((pvo)->pvo_vaddr |= (i)|PVO_PTEGIDX_VALID))


/*
 * This array is allocated at boot time and contains one entry per
 * PTE group.
 */
struct pvo_head *pmap_upvo_table;	/* pvo entries by ptegroup index */

static struct evcnt pmap_pteg_idx_events[SH5_PTEG_SIZE] = {
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap",
	    "ptes added at pteg group [0]"),
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap",
	    "ptes added at pteg group [1]"),
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap",
	    "ptes added at pteg group [2]"),
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap",
	    "ptes added at pteg group [3]"),
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap",
	    "ptes added at pteg group [4]"),
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap",
	    "ptes added at pteg group [5]"),
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap",
	    "ptes added at pteg group [6]"),
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap",
	    "ptes added at pteg group [7]")
};

static struct evcnt pmap_pte_spill_events =
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap", "spills");
static struct evcnt pmap_pte_spill_evict_events =
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap", "spill evictions");

static struct evcnt pmap_asid_regen_events =
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap", "asid regens");

static struct evcnt pmap_shared_cache_downgrade_events =
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap", "cache downgrades");
static struct evcnt pmap_shared_cache_upgrade_events =
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap", "cache upgrades");

static struct evcnt pmap_zero_page_events =
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap", "zero page");
static struct evcnt pmap_copy_page_events =
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap", "copy page");
static struct evcnt pmap_zero_page_dpurge_events =
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap", "zero page purge");
static struct evcnt pmap_copy_page_dpurge_src_events =
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap", "copy page src purge");
static struct evcnt pmap_copy_page_dpurge_dst_events =
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap", "copy page dst purge");

/*
 * This array contains one entry per kernel IPT entry.
 */
struct pvo_head pmap_kpvo_table[KERNEL_IPT_SIZE];

static struct pvo_head pmap_pvo_unmanaged =
	    LIST_HEAD_INITIALIZER(pmap_pvo_unmanaged);

static struct pool pmap_pool;	   /* pool of pmap structures */
static struct pool pmap_upvo_pool; /* pool of pvo entries for unmanaged pages */
static struct pool pmap_mpvo_pool; /* pool of pvo entries for managed pages */

/*
 * We keep a cache of unmanaged pages to be used for pvo entries for
 * unmanaged pages.
 */
struct pvo_page {
	SIMPLEQ_ENTRY(pvo_page) pvop_link;
};
SIMPLEQ_HEAD(pvop_head, pvo_page);
struct pvop_head pmap_upvop_head = SIMPLEQ_HEAD_INITIALIZER(pmap_upvop_head);
u_long pmap_upvop_free;

static void *pmap_pool_ualloc(struct pool *, int);
static void pmap_pool_ufree(struct pool *, void *);

static struct pool_allocator pmap_pool_uallocator = {
	pmap_pool_ualloc, pmap_pool_ufree, 0,
};

volatile pte_t *pmap_pte_spill(u_int, vsid_t, vaddr_t);

static void pmap_copyzero_page_dpurge(paddr_t, struct evcnt *);
static volatile pte_t * pmap_pvo_to_pte(const struct pvo_entry *, int);
static struct pvo_entry * pmap_pvo_find_va(pmap_t, vaddr_t, int *);
static void pmap_pinit(pmap_t);
static void pmap_release(pmap_t);
static void pmap_pa_map_kva(vaddr_t, paddr_t, ptel_t);
static ptel_t pmap_pa_unmap_kva(vaddr_t, kpte_t *);
static void pmap_change_cache_attr(struct pvo_entry *, ptel_t);
static int pmap_pvo_enter(pmap_t, struct pool *, struct pvo_head *,
	vaddr_t, paddr_t, ptel_t, int);
static void pmap_pvo_remove(struct pvo_entry *, int);
static void pmap_sharing_policy(struct pvo_head *);

static u_int	pmap_asid_next;
static u_int	pmap_asid_max;
static u_int	pmap_asid_generation;
static void	pmap_asid_alloc(pmap_t);

extern void	pmap_asm_zero_page(vaddr_t);
extern void	pmap_asm_copy_page(vaddr_t, vaddr_t);

#define	NPMAPS		16384
#define	VSID_NBPW	(sizeof(uint32_t) * 8)
static uint32_t pmap_vsid_bitmap[NPMAPS / VSID_NBPW];


/*
 * Various pmap related variables
 */
int physmem;
paddr_t avail_start, avail_end;

static struct mem_region *mem;
struct pmap kernel_pmap_store;

static vaddr_t pmap_zero_page_kva;
static vaddr_t pmap_copy_page_src_kva;
static vaddr_t pmap_copy_page_dst_kva;
static vaddr_t pmap_kva_avail_start;
static vaddr_t pmap_device_kva_start;
#define	PMAP_BOOTSTRAP_DEVICE_KVA	(PAGE_SIZE * 512)
vaddr_t vmmap;
paddr_t pmap_kseg0_pa;

int pmap_initialized;

#ifdef PMAP_DIAG
int pmap_pvo_enter_depth;
int pmap_pvo_remove_depth;
#endif


/*
 * Returns non-zero if the given pmap is `current'.
 */
static __inline int
pmap_is_curpmap(pmap_t pm)
{
	if ((curlwp->l_proc && curlwp->l_proc->p_vmspace->vm_map.pmap == pm) ||
	    pm == pmap_kernel())
		return (1);

	return (0);
}

/*
 * Given a Kernel Virtual Address in KSEG1, return the
 * corresponding index into pmap_kernel_ipt[].
 */
static __inline int
kva_to_iptidx(vaddr_t kva)
{
	int idx;

	if (kva < SH5_KSEG1_BASE)
		return (-1);

	idx = (int) sh5_btop(kva - SH5_KSEG1_BASE);
	if (idx >= KERNEL_IPT_SIZE)
		return (-1);

	return (idx);
}

/*
 * Given a pointer to an element of the pmap_kernel_ipt array,
 * return the ptel.
 *
 * XXX: This hack generates *much* better code than the compiler.
 */
static __inline ptel_t
pmap_kernel_ipt_get_ptel(kpte_t *kpte)
{
	ptel_t ptel;
	register_t r1, r2;

	/*
	 * XXX: Need to revisit for big-endian
	 */
#if SH5_NEFF_BITS == 32
	__asm __volatile("ldlo.l %3, 0, %1; ldhi.l %3, 3, %2; or %1, %2, %0":
	    "=r"(ptel), "=r"(r1), "=r"(r2): "r"(kpte));
#else
	__asm __volatile("ldlo.q %3, 0, %1; ldhi.q %3, 7, %2; or %1, %2, %0":
	    "=r"(ptel), "=r"(r1), "=r"(r2): "r"(kpte));
#endif

	return (ptel);
}

static __inline void
pmap_kernel_ipt_set_ptel(kpte_t *kpte, ptel_t ptel)
{

#if SH5_NEFF_BITS == 32
	__asm __volatile("stlo.l %0, 0, %1; sthi.l %0, 3, %r1"::
	    "r"(kpte), "r"(ptel));
#else
	__asm __volatile("stlo.q %0, 0, %1; sthi.q %0, 7, %r1"::
	    "r"(kpte), "r"(ptel));
#endif
}

/*
 * Given a pointer to an element of the pmap_kernel_ipt array,
 * return the tlbcookie.
 */
static __inline tlbcookie_t
pmap_kernel_ipt_get_tlbcookie(kpte_t *kpte)
{
	u_int16_t *kpp = (u_int16_t *)&kpte->tlbcookie;

	return ((tlbcookie_t)*kpp);
}

static __inline void
pmap_kernel_ipt_set_tlbcookie(kpte_t *kpte, tlbcookie_t tlbcookie)
{
	u_int16_t *kpp = (u_int16_t *)&kpte->tlbcookie;

	*kpp = (u_int16_t)tlbcookie;
}

/*
 * Given a user virtual address, return the corresponding
 * index of the PTE group which is likely to contain the mapping.
 */
static __inline u_int
va_to_pteg(vsid_t vsid, vaddr_t va)
{

	return (pmap_ipt_hash(vsid, va));
}

/*
 * Given a physical address, return a pointer to the head of the
 * list of virtual address which map to that physical address.
 */
static __inline struct pvo_head *
pa_to_pvoh(paddr_t pa, struct vm_page **pg_p)
{
	struct vm_page *pg;

	pg = PHYS_TO_VM_PAGE(pa);

	if (pg_p != NULL)
		*pg_p = pg;

	if (pg == NULL)
		return (&pmap_pvo_unmanaged);

	return (&pg->mdpage.mdpg_pvoh);
}

/*
 * Given a vm_page, return a pointer to the head of the list
 * of virtual addresses which map to that page.
 */
static __inline struct pvo_head *
vm_page_to_pvoh(struct vm_page *pg)
{

	return (&pg->mdpage.mdpg_pvoh);
}

/*
 * Clear the specified bits in the cached attributes of a physical page.
 */
static __inline void
pmap_attr_clear(struct vm_page *pg, int ptebit)
{

	pg->mdpage.mdpg_attrs &= ~ptebit;
}

/*
 * Return the cached attributes of a physical page
 */
static __inline int
pmap_attr_fetch(struct vm_page *pg)
{

	return (pg->mdpage.mdpg_attrs);
}

/*
 * Cache the specified attributes for the physical page "pg"
 */
static __inline void
pmap_attr_save(struct vm_page *pg, ptel_t ptebit)
{

	pg->mdpage.mdpg_attrs |= (unsigned int)ptebit;
}

/*
 * Returns non-zero if the VSID and VA combination matches the
 * pvo_entry "pvo".
 *
 * That is, the mapping decribed by `pvo' is exactly the same
 * as described by `vsid' and `va'.
 */
static __inline int
pmap_pteh_match(struct pvo_entry *pvo, vsid_t vsid, vaddr_t va)
{

	if (pvo->pvo_pmap->pm_vsid == vsid && PVO_VADDR(pvo) == va)
		return (1);

	return (0);
}

/*
 * Recover the Referenced/Modified bits from the specified PTEL value
 * and cache them temporarily in the PVO.
 */
static __inline void
pmap_pteg_synch(ptel_t ptel, struct pvo_entry *pvo)
{

	if (PVO_ISMANAGED(pvo) && !PVO_ISWIRED(pvo))
		pvo->pvo_ptel |= (ptel & SH5_PTEL_RM_MASK);
}

/*
 * We're about to raise the protection of a mapping.
 * Make sure the cache is synchronised before the protection changes.
 */
static void
pmap_cache_sync_raise(vaddr_t va, ptel_t ptel, ptel_t clrbits)
{
	paddr_t pa;

	/*
	 * Just return if the mapping is not cacheable.
	 */
	if (!SH5_PTEL_CACHEABLE(ptel))
		return;

	/*
	 * Also just return if the page has never been referenced.
	 */
	if ((ptel & SH5_PTEL_R) == 0)
		return;

	pa = (paddr_t)(ptel & SH5_PTEL_PPN_MASK);

	switch ((ptel & clrbits) & (SH5_PTEL_PR_W | SH5_PTEL_PR_X)) {
	case SH5_PTEL_PR_W | SH5_PTEL_PR_X:
		/*
		 * The page is being made no-exec, rd-only.
		 * Purge the data cache and invalidate insn cache.
		 */
		cpu_cache_dpurge_iinv(va, pa, PAGE_SIZE);
		break;

	case SH5_PTEL_PR_W:
		/*
		 * The page is being made read-only.
		 * Purge the data-cache.
		 */
		cpu_cache_dpurge(va, pa, PAGE_SIZE);
		break;

	case SH5_PTEL_PR_X:
		/*
		 * The page is being made no-exec.
		 * Invalidate the instruction cache.
		 */
		cpu_cache_iinv(va, pa, PAGE_SIZE);
		break;

	case 0:
		/*
		 * The page already has the required protection.
		 * No need to touch the cache.
		 */
		break;
	}
}

/*
 * We're about to delete a mapping.
 * Make sure the cache is synchronised before the mapping disappears.
 */
static void
pmap_cache_sync_unmap(vaddr_t va, ptel_t ptel)
{
	paddr_t pa;

	/*
	 * Just return if the mapping was not cacheable.
	 */
	if (!SH5_PTEL_CACHEABLE(ptel))
		return;

	/*
	 * Also just return if the page has never been referenced.
	 */
	if ((ptel & SH5_PTEL_R) == 0)
		return;

	pa = (paddr_t)(ptel & SH5_PTEL_PPN_MASK);

	switch (ptel & (SH5_PTEL_PR_W | SH5_PTEL_PR_X)) {
	case SH5_PTEL_PR_W | SH5_PTEL_PR_X:
	case SH5_PTEL_PR_X:
		/*
		 * The page was executable, and possibly writable.
		 * Purge the data cache and invalidate insn cache.
		 */
		cpu_cache_dpurge_iinv(va, pa, PAGE_SIZE);
		break;

	case SH5_PTEL_PR_W:
		/*
		 * The page was writable.
		 * Purge the data-cache.
		 */
		cpu_cache_dpurge(va, pa, PAGE_SIZE);
		break;

	case 0:
		/*
		 * The page was read-only.
		 * Just invalidate the data cache.
		 *
		 * Note: We'd like to use cpu_cache_dinv() here, but
		 * since the mapping may still be in the TLB, the cache
		 * tag will contain the original protection bits.
		 * The invalidate operation will actually cause a write-
		 * protection fault (!!!!) in this case.
		 */
		cpu_cache_dpurge(va, pa, PAGE_SIZE);
		break;
	}
}

/*
 * Clear the specified bit(s) in the PTE (actually, the PTEL)
 *
 * In this case, the pte MUST be resident in the ipt, and it may
 * also be resident in the TLB.
 */
static void
pmap_pteg_clear_bit(volatile pte_t *pt, struct pvo_entry *pvo, u_int ptebit)
{
	pmap_t pm;
	ptel_t ptel;
	pteh_t pteh;

	pm = pvo->pvo_pmap;
	pteh = pt->pteh;
	ptel = pt->ptel;

	/*
	 * Before raising the protection of the mapping,
	 * make sure the cache is synchronised.
	 *
	 * Note: The cpu-specific cache handling code will ensure
	 * this doesn't cause a TLB miss exception.
	 */
	pmap_cache_sync_raise(PVO_VADDR(pvo), ptel, ptebit);

	/*
	 * Note:
	 * We clear the Referenced bit here so that subsequent calls to
	 * pmap_cache_sync_*() will only purge the cache for the page
	 * if it has been accessed between now and then.
	 */
	pt->ptel = ptel & ~(ptebit | SH5_PTEL_R);

	if ((ptel & SH5_PTEL_R) != 0 && pm->pm_asid != PMAP_ASID_UNASSIGNED &&
	    pm->pm_asidgen == pmap_asid_generation) {
		/*
		 * The mapping may be cached in the TLB. Call cpu-specific
		 * code to check and invalidate if necessary.
		 */
		cpu_tlbinv_cookie((pteh & SH5_PTEH_EPN_MASK) |
		    (pm->pm_asid << SH5_PTEH_ASID_SHIFT),
		    pt->tlbcookie);
	}

	pt->tlbcookie = 0;
	pmap_pteg_synch(ptel, pvo);
}

static void
pmap_kpte_clear_bit(int idx, struct pvo_entry *pvo, ptel_t ptebit)
{
	kpte_t *kpte;
	ptel_t ptel;

	kpte = &pmap_kernel_ipt[idx];
	ptel = pmap_kernel_ipt_get_ptel(kpte);

	if ((ptel & SH5_PTEL_R) != 0)
		cpu_tlbinv_cookie((pteh_t)PVO_VADDR(pvo) | SH5_PTEH_SH,
		    pmap_kernel_ipt_get_tlbcookie(kpte));

	pmap_kernel_ipt_set_tlbcookie(kpte, 0);
	pmap_pteg_synch(ptel, pvo);

	/*
	 * Syncronise the cache in readiness for raising the protection.
	 */
	pmap_cache_sync_raise(PVO_VADDR(pvo), ptel, ptebit);

	/*
	 * It's now safe to change the page table.
	 * We clear the Referenced bit here so that subsequent calls to
	 * pmap_cache_sync_*() will only purge the cache for the page
	 * if it has been accessed between now and then.
	 */
	pmap_kernel_ipt_set_ptel(kpte, ptel & ~(ptebit | SH5_PTEL_R));
}

/*
 * Copy the mapping specified in pvo to the PTE structure `pt'.
 * `pt' is assumed to be an entry in the pmap_pteg_table array.
 * 
 * This makes the mapping directly available to the TLB miss
 * handler.
 *
 * It is assumed that the mapping is not currently in the TLB, and
 * hence not in the cache either. Therefore neither need to be
 * synchronised.
 */
static __inline void
pmap_pteg_set(volatile pte_t *pt, struct pvo_entry *pvo)
{

	pt->ptel = pvo->pvo_ptel & ~SH5_PTEL_R;
	pt->pteh = (pteh_t) PVO_VADDR(pvo);
	pt->vsid = pvo->pvo_pmap->pm_vsid;
}

/*
 * Remove the mapping specified by `pt', which is assumed to point to
 * an entry in the pmap_pteg_table.
 *
 * This function will preserve Referenced/Modified state from the PTE
 * before ensuring there is no reference to the mapping in the TLB.
 */
static void
pmap_pteg_unset(volatile pte_t *pt, struct pvo_entry *pvo)
{
	pmap_t pm;
	pteh_t pteh;
	ptel_t ptel;

	pm = pvo->pvo_pmap;
	ptel = pt->ptel;
	pteh = pt->pteh;
	pt->pteh = 0;

	/*
	 * Before deleting the mapping from the PTEG/TLB,
	 * make sure the cache is synchronised.
	 *
	 * Note: The cpu-specific cache handling code must ensure
	 * this doesn't cause a TLB miss exception.
	 */
	pmap_cache_sync_unmap(PVO_VADDR(pvo), ptel);

	if ((ptel & SH5_PTEL_R) != 0 && pm->pm_asid != PMAP_ASID_UNASSIGNED &&
	    pm->pm_asidgen == pmap_asid_generation) {
		/*
		 * The mapping may be in the TLB. Call cpu-specific
		 * code to check and invalidate if necessary.
		 */
		cpu_tlbinv_cookie((pteh & SH5_PTEH_EPN_MASK) |
		    (pm->pm_asid << SH5_PTEH_ASID_SHIFT),
		    pt->tlbcookie);
	}

	pt->tlbcookie = 0;
	pmap_pteg_synch(ptel, pvo);
}

/*
 * This function is called when the mapping described by pt/pvo has
 * had its attributes modified in some way. For example, it may have
 * been a read-only mapping but is now read/write.
 */
static __inline void
pmap_pteg_change(volatile pte_t *pt, struct pvo_entry *pvo)
{

	pmap_pteg_unset(pt, pvo);
	pmap_pteg_set(pt, pvo);
}

/*
 * This function inserts the mapping described by `pvo' into the first
 * available vacant slot of the specified PTE group.
 *
 * If a vacant slot was found, this function returns its index.
 * Otherwise, -1 is returned.
 */
static int
pmap_pteg_insert(pteg_t *pteg, struct pvo_entry *pvo)
{
	volatile pte_t *pt;
	int i;

	for (pt = &pteg->pte[0], i = 0; i < SH5_PTEG_SIZE; i++, pt++) {
		if (pt->pteh == 0) {
			pmap_pteg_set(pt, pvo);
			return (i);
		}
	}

	return (-1);
}

/*
 * Spill handler.
 *
 * Tries to spill a page table entry from the overflow area into the
 * main pmap_pteg_table array.
 *
 * This runs on a dedicated TLB miss stack, with exceptions disabled.
 *
 * Note: This is only called for mappings in user virtual address space.
 */
volatile pte_t *
pmap_pte_spill(u_int ptegidx, vsid_t vsid, vaddr_t va)
{
	struct pvo_entry *source_pvo, *victim_pvo;
	struct pvo_entry *pvo;
	int idx, j;
	pteg_t *ptg;
	volatile pte_t *pt;

	/*
	 * Have to substitute some entry.
	 *
	 * Use low bits of the tick counter as random generator
	 */
	idx = (sh5_getctc() >> 2) & 7;
	ptg = &pmap_pteg_table[ptegidx];
	pt = &ptg->pte[idx];

	source_pvo = NULL;
	victim_pvo = NULL;

	LIST_FOREACH(pvo, &pmap_upvo_table[ptegidx], pvo_olink) {
		if (source_pvo == NULL && pmap_pteh_match(pvo, vsid, va)) {
			/*
			 * Found the entry to be spilled into the pteg.
			 */
			if (pt->pteh == 0) {
				/* Our victim was already available */
				pmap_pteg_set(pt, pvo);
				j = idx;
			} else {
				/* See if the pteg has a free entry */
				j = pmap_pteg_insert(ptg, pvo);
			}

			if (j >= 0) {
				/* Excellent. No need to evict anyone! */
				PVO_PTEGIDX_SET(pvo, j);
				pmap_pteg_idx_events[j].ev_count++;
				pmap_pte_spill_events.ev_count++;
				return (&ptg->pte[j]);
			}

			/*
			 * Found a match, but no free entries in the pte group.
			 * This means we have to search for the victim's pvo
			 * if we haven't already found it.
			 */
			source_pvo = pvo;
			if (victim_pvo != NULL)
				break;
		}

		/*
		 * We also need the pvo entry of the victim we are replacing
		 * to save the R & M bits of the PTE.
		 */
		if (victim_pvo == NULL &&
		    PVO_PTEGIDX_ISSET(pvo) && PVO_PTEGIDX_GET(pvo) == idx) {
			/* Found the victim's pvo */
			victim_pvo = pvo;

			/*
			 * If we already have the source pvo, we're all set.
			 */
			if (source_pvo != NULL)
				break;
		}
	}

	/*
	 * If source_pvo is NULL, we have no mapping at the target address
	 * for the current VSID.
	 *
	 * Returning NULL will force the TLB miss code to invoke the full-
	 * blown page fault handler.
	 */
	if (source_pvo == NULL)
		return (NULL);

	/*
	 * If we couldn't find the victim, however, then the pmap module
	 * has become very confused...
	 *
	 * XXX: This panic is pointless; SR.BL is set ...
	 */
	if (victim_pvo == NULL)
		panic("pmap_pte_spill: victim p-pte (%p) has no pvo entry!",pt);

	/*
	 * Update the cached Referenced/Modified bits for the victim and
	 * remove it from the TLB/cache.
	 */
	pmap_pteg_unset(pt, victim_pvo);
	PVO_PTEGIDX_CLR(victim_pvo);

	/*
	 * Copy the source pvo's details into the PTE.
	 */
	pmap_pteg_set(pt, source_pvo);
	PVO_PTEGIDX_SET(source_pvo, idx);

	pmap_pte_spill_evict_events.ev_count++;
	pmap_pte_spill_events.ev_count++;

	return (pt);
}

void
pmap_bootstrap(vaddr_t avail, paddr_t kseg0base, struct mem_region *mr)
{
	struct mem_region *mp;
	psize_t size;
	int i;

	uvmexp.pagesize = NBPG;
	uvm_setpagesize();

	pmap_kseg0_pa = kseg0base;

	for (mp = mr; mp->mr_size; mp++)
		physmem += btoc(mp->mr_size);

	/*
	 * Allow for either a hard-coded size for pmap_pteg_table, or
	 * calculate it based on the physical memory in the system.
	 * We allocate one pteg_t for every two physical pages of
	 * memory.
	 */
#ifdef PTEGCOUNT
	pmap_pteg_cnt = PTEGCOUNT;
#else
	pmap_pteg_cnt = 0x1000;

	while (pmap_pteg_cnt < physmem)
		pmap_pteg_cnt <<= 1;
	pmap_pteg_cnt >>= 1;
#endif

	/*
	 * Calculate some important constants.
	 * These help to speed up TLB lookups, hash and vsid generation.
	 */
	pmap_pteg_mask = pmap_pteg_cnt - 1;
	pmap_pteg_bits = ffs(pmap_pteg_cnt) - 1;

	/*
	 * Allocate wired memory in KSEG0 for pmap_pteg_table.
	 */
	size = pmap_pteg_cnt * sizeof(pteg_t);
	pmap_pteg_table = (pteg_t *)avail;
	memset(pmap_pteg_table, 0, size);
	avail += size;
	mr[0].mr_start += size;
	mr[0].mr_kvastart += size;
	mr[0].mr_size -= size;

	/*
	 * Allocate and initialise PVO structures now
	 */
	size = sh5_round_page(sizeof(struct pvo_head) * pmap_pteg_cnt);
	pmap_upvo_table = (struct pvo_head *) avail;

	for (i = 0; i < pmap_pteg_cnt; i++)
		LIST_INIT(&pmap_upvo_table[i]);

	for (i = 0; i < KERNEL_IPT_SIZE; i++)
		LIST_INIT(&pmap_kpvo_table[i]);

	avail = sh5_round_page(avail + size);
	mr[0].mr_start += size;
	mr[0].mr_kvastart += size;
	mr[0].mr_size -= size;

	/*
	 * Allocate the kernel message buffer
	 */
	size = sh5_round_page(MSGBUFSIZE);
	initmsgbuf((caddr_t)avail, size);

	avail = sh5_round_page(avail + size);
	mr[0].mr_start += size;
	mr[0].mr_kvastart += size;
	mr[0].mr_size -= size;

	mem = mr;

	/*
	 * XXX: Need to sort the memory regions by start address here...
	 */

	avail_start = mr[0].mr_start;

	/*
	 * It should now be safe to take TLB miss exceptions.
	 */
	__asm __volatile("putcon %0, sr" :: "r"(SH5_CONREG_SR_IMASK_ALL));

	/*
	 * Tell UVM about physical memory
	 */
	for (mp = mr; mp->mr_size; mp++) {
		uvm_page_physload(atop(mp->mr_start),
		    atop(mp->mr_start + mp->mr_size),
		    atop(mp->mr_start), atop(mp->mr_start + mp->mr_size),
		    VM_FREELIST_DEFAULT);
		avail_end = mp->mr_start + mp->mr_size;
	}

	pmap_zero_page_kva = SH5_KSEG1_BASE;
	pmap_copy_page_src_kva = pmap_zero_page_kva + PAGE_SIZE;
	pmap_copy_page_dst_kva = pmap_copy_page_src_kva + PAGE_SIZE;
	vmmap = pmap_copy_page_dst_kva + PAGE_SIZE;
	pmap_device_kva_start = vmmap + PAGE_SIZE;

	pmap_kva_avail_start = pmap_device_kva_start +
	    PMAP_BOOTSTRAP_DEVICE_KVA;

	pmap_asid_next = PMAP_ASID_USER_START;
	pmap_asid_max = SH5_PTEH_ASID_MASK;	/* XXX Should be cpu specific */

	pmap_pinit(pmap_kernel());
	pmap_kernel()->pm_asid = PMAP_ASID_KERNEL;
	pmap_kernel()->pm_asidgen = 0;

	pool_init(&pmap_upvo_pool, sizeof(struct pvo_entry),
	    0, 0, 0, "pmap_upvopl", &pmap_pool_uallocator);

	pool_setlowat(&pmap_upvo_pool, 252);
}

/*
 * This function is used, before pmap_init() is called, to set up fixed
 * wired mappings mostly for the benefit of the bus_space(9) implementation.
 *
 * XXX: Make this more general; it's useful for more than just pre-init
 * mappings. Particularly the big page support.
 */
vaddr_t
pmap_map_device(paddr_t pa, u_int len)
{
	vaddr_t va, rv;
	ptel_t ptel;
	int idx;

	len = sh5_round_page(len);

	if (pmap_initialized == 0) {
		rv = va = pmap_device_kva_start;
		if ((va + len) >= pmap_kva_avail_start)
			panic("pmap_map_device: out of device bootstrap kva");
		pmap_device_kva_start += len;
	} else
		rv = va = uvm_km_valloc(kernel_map, len);

	while (len) {
		idx = kva_to_iptidx(va);

		ptel = SH5_PTEL_CB_DEVICE | SH5_PTEL_PR_R | SH5_PTEL_PR_W;
		ptel |= (ptel_t)(pa & SH5_PTEL_PPN_MASK);

		pmap_kernel_ipt_set_ptel(&pmap_kernel_ipt[idx], ptel);

		va += PAGE_SIZE;
		pa += PAGE_SIZE;
		len -= PAGE_SIZE;
	}

	return (rv);
}

/*
 * Returns non-zero if the specified mapping is cacheable
 */
int
pmap_page_is_cacheable(pmap_t pm, vaddr_t va)
{
	struct pvo_entry *pvo;
	ptel_t ptel = 0;
	int s;

	if (va < SH5_KSEG1_BASE && va >= SH5_KSEG0_BASE)
		return (1);

	s = splvm();
	pvo = pmap_pvo_find_va(pm, va, NULL);
	if (pvo != NULL)
		ptel = pvo->pvo_ptel;
	else
	if (pm == pmap_kernel()) {
		int idx = kva_to_iptidx(va);
		if (idx >= 0)
			ptel = pmap_kernel_ipt_get_ptel(&pmap_kernel_ipt[idx]);
	}
	splx(s);

	return (SH5_PTEL_CACHEABLE(ptel));
}

void
pmap_init(void)
{
	int s, i;

	s = splvm();

	pool_init(&pmap_pool, sizeof(struct pmap),
	    sizeof(void *), 0, 0, "pmap_pl", NULL);

	pool_init(&pmap_mpvo_pool, sizeof(struct pvo_entry),
	    0, 0, 0, "pmap_mpvopl", NULL);

	pool_setlowat(&pmap_mpvo_pool, 1008);

	pmap_initialized = 1;

	splx(s);

	evcnt_attach_static(&pmap_pte_spill_events);
	evcnt_attach_static(&pmap_pte_spill_evict_events);
	evcnt_attach_static(&pmap_asid_regen_events);
	evcnt_attach_static(&pmap_shared_cache_downgrade_events);
	evcnt_attach_static(&pmap_shared_cache_upgrade_events);
	evcnt_attach_static(&pmap_zero_page_events);
	evcnt_attach_static(&pmap_copy_page_events);
	evcnt_attach_static(&pmap_zero_page_dpurge_events);
	evcnt_attach_static(&pmap_copy_page_dpurge_src_events);
	evcnt_attach_static(&pmap_copy_page_dpurge_dst_events);
	for (i = 0; i < SH5_PTEG_SIZE; i++)
		evcnt_attach_static(&pmap_pteg_idx_events[i]);
}

/*
 * How much virtual space does the kernel get?
 */
void
pmap_virtual_space(vaddr_t *start, vaddr_t *end)
{

	*start = pmap_kva_avail_start;
	*end = SH5_KSEG1_BASE + ((KERNEL_IPT_SIZE - 1) * PAGE_SIZE);
}

/*
 * Allocate, initialize, and return a new physical map.
 */
pmap_t
pmap_create(void)
{
	pmap_t pm;
	
	pm = pool_get(&pmap_pool, PR_WAITOK);
	memset((caddr_t)pm, 0, sizeof *pm);
	pmap_pinit(pm);
	return (pm);
}

/*
 * Initialize a preallocated and zeroed pmap structure.
 */
static void
pmap_pinit(pmap_t pm)
{
	u_int entropy = (sh5_getctc() >> 2) + (u_int)time.tv_sec;
	int i;

	pm->pm_refs = 1;
	pm->pm_asid = PMAP_ASID_UNASSIGNED;

	for (i = 0; i < NPMAPS; i += VSID_NBPW) {
		static u_int pmap_vsidcontext;
		u_int32_t mask;
		u_int hash, n;

		pmap_vsidcontext = (pmap_vsidcontext * 0x1105) + entropy;

		/*
		 * The hash only has to be unique in *at least* the
		 * first log2(NPMAPS) bits. However, if there are
		 * more than log2(NPMAPS) significant bits in the
		 * pmap_pteg_table index, then we can make use of
		 * higher-order bits in this hash as additional
		 * entropy for the pmap_pteg hashing function.
		 */
		hash = pmap_vsidcontext & ((pmap_pteg_mask < (NPMAPS - 1)) ?
		    (NPMAPS - 1) : pmap_pteg_mask);
		if (hash == 0)
			continue;

		/*
		 * n    == The index into the pmap_vsid_bitmap[] array.
		 *         This narrows things down to a single uint32_t.
		 * mask == The bit within that u_int32_t which we're
		 *         going to try to allocate.
		 */
		n = (hash & (NPMAPS - 1)) / VSID_NBPW;
		mask = 1 << (hash & (VSID_NBPW - 1));

		if (pmap_vsid_bitmap[n] & mask) {
			/*
			 * Hash collision.
			 * Anything free in this bucket?
			 */
			if (pmap_vsid_bitmap[n] == 0xffffffffu) {
				entropy = pmap_vsidcontext >> 20;
				continue;
			}

			/*
			 * Yup. Adjust the low-order bits of the hash
			 * to match the index of the free slot.
			 */
			i = ffs(~pmap_vsid_bitmap[n]) - 1;
			hash = (hash & ~(VSID_NBPW - 1)) | i;
			mask = 1 << i;
		}

		pmap_vsid_bitmap[n] |= mask;
		pm->pm_vsid = hash;
		return;
	}

	panic("pmap_pinit: out of VSIDs!");
}

/*
 * Add a reference to the given pmap.
 */
void
pmap_reference(pmap_t pm)
{
	pm->pm_refs++;
}

/*
 * Retire the given pmap from service.
 * Should only be called if the map contains no valid mappings.
 */
void
pmap_destroy(pmap_t pm)
{
	if (--pm->pm_refs == 0) {
		pmap_release(pm);
		pool_put(&pmap_pool, pm);
	}
}

/*
 * Release any resources held by the given physical map.
 * Called when a pmap initialized by pmap_pinit is being released.
 */
void
pmap_release(pmap_t pm)
{
	int idx;
	uint32_t mask;

	idx = (pm->pm_vsid & (NPMAPS - 1)) / VSID_NBPW;
	mask = 1 << (pm->pm_vsid & (VSID_NBPW - 1));

	pmap_vsid_bitmap[idx] &= ~mask;
}

/*
 * Copy the range specified by src_addr/len
 * from the source map to the range dst_addr/len
 * in the destination map.
 *
 * This routine is only advisory and need not do anything.
 */
void
pmap_copy(pmap_t dst_pmap, pmap_t src_pmap, vaddr_t dst_addr,
	vsize_t len, vaddr_t src_addr)
{
}

/*
 * Require that all active physical maps contain no
 * incorrect entries NOW.
 */
void
pmap_update(struct pmap *pmap)
{
}

/*
 * Garbage collects the physical map system for
 * pages which are no longer used.
 * Success need not be guaranteed -- that is, there
 * may well be pages which are not referenced, but
 * others may be collected.
 * Called by the pageout daemon when pages are scarce.
 */
void
pmap_collect(pmap_t pm)
{
}

/*
 * Fill the given physical page with zeroes.
 */
void
pmap_zero_page(paddr_t pa)
{

	if (!pmap_initialized)
		panic("pmap_zero_page: pmap_initialized is false!");

	pmap_zero_page_events.ev_count++;

	/*
	 * Purge/invalidate the cache for any other mappings to this PA.
	 *
	 * XXX: This should not be necessary, but is here as a fail-safe.
	 * We should really panic if there are any existing mappings...
	 */
	pmap_copyzero_page_dpurge(pa, &pmap_zero_page_dpurge_events);

	/*
	 * Note: We could use the KSEG0 address of PA, assuming it resides
	 * there. However, it really doesn't buy us anything; we still have
	 * to muck around with the cache (which is where the real performance
	 * hit of this comes from)).
	 *
	 * It would be nice if the SH5 had a Write-back-and-inv-all-
	 * operand-cache instruction...
	 */

	pmap_pa_map_kva(pmap_zero_page_kva, pa, SH5_PTEL_PR_W);
	pmap_asm_zero_page(pmap_zero_page_kva);
	(void) pmap_pa_unmap_kva(pmap_zero_page_kva, NULL);
}

/*
 * Copy the given physical source page to its destination.
 */
void
pmap_copy_page(paddr_t src, paddr_t dst)
{

	if (!pmap_initialized)
		panic("pmap_copy_page: pmap_initialized is false!");

	PMPRINTF(("pmap_copy_page: copying 0x%lx -> 0x%lx\n", src, dst));

	pmap_copy_page_events.ev_count++;

	/*
	 * Purge/invalidate the cache for any other mappings to the source
	 * and destination PAs.
	 *
	 * XXX: This should not be necessary for the destination PA, but is
	 * here as a fail-safe. We should really panic if there are any
	 * existing mappings for dst...
	 */
	pmap_copyzero_page_dpurge(src, &pmap_copy_page_dpurge_src_events);
	pmap_copyzero_page_dpurge(dst, &pmap_copy_page_dpurge_dst_events);

	/*
	 * Note: We could use the KSEG0 addresses of src and dst, assuming
	 * they reside there. However, it really doesn't buy us anything;
	 * we still have to muck around with the cache (which is where the
	 * real performance hit of this comes from)).
	 *
	 * It would be nice if the SH5 had a Write-back-and-inv-all-
	 * operand-cache instruction...
	 */

	pmap_pa_map_kva(pmap_copy_page_dst_kva, dst, SH5_PTEL_PR_W);
	pmap_pa_map_kva(pmap_copy_page_src_kva, src, 0);

	pmap_asm_copy_page(pmap_copy_page_dst_kva, pmap_copy_page_src_kva);

	(void) pmap_pa_unmap_kva(pmap_copy_page_src_kva, NULL);
	(void) pmap_pa_unmap_kva(pmap_copy_page_dst_kva, NULL);
}

static void
pmap_copyzero_page_dpurge(paddr_t pa, struct evcnt *ev)
{
	struct pvo_head *pvo_head;
	struct pvo_entry *pvo;

	if ((pvo_head = pa_to_pvoh(pa, NULL)) == NULL ||
	    (pvo = LIST_FIRST(pvo_head)) == NULL ||
	    !SH5_PTEL_CACHEABLE(pvo->pvo_ptel))
		return;

	/*
	 * One or more cacheable mappings already exist for this
	 * physical page. We now have to take preventative measures
	 * to purge all dirty data back to the page and ensure no
	 * valid cache lines remain which reference the page.
	 */
	LIST_FOREACH(pvo, pvo_head, pvo_vlink) {
		KDASSERT((paddr_t)(pvo->pvo_ptel & SH5_PTEL_PPN_MASK) == pa);

		if (PVO_VADDR(pvo) < SH5_KSEG0_BASE && !PVO_PTEGIDX_ISSET(pvo))
			continue;

		cpu_cache_dpurge_iinv(PVO_VADDR(pvo), pa, PAGE_SIZE);

		ev->ev_count++;

		/*
		 * If the first mapping is writable, then we don't need
		 * to purge the others; they must all be at the same VA.
		 */
		if (PVO_ISWRITABLE(pvo))
			break;
	}
}

/*
 * Find the PTE described by "pvo" and "ptegidx" within the
 * pmap_pteg_table[] array.
 *
 * If the PTE is not resident, return NULL.
 */
static volatile pte_t *
pmap_pvo_to_pte(const struct pvo_entry *pvo, int ptegidx)
{
	if (!PVO_PTEGIDX_ISSET(pvo))
		return (NULL);

	/*
	 * If we haven't been supplied the ptegidx, calculate it.
	 */
	if (ptegidx < 0) {
		ptegidx = va_to_pteg(pvo->pvo_pmap->pm_vsid, pvo->pvo_vaddr);
		if (ptegidx < 0)
			return (NULL);
	}

	return (&pmap_pteg_table[ptegidx].pte[PVO_PTEGIDX_GET(pvo)]);
}

/*
 * Find the pvo_entry associated with the given virtual address and pmap.
 * If no mapping is found, the function returns NULL. Otherwise:
 *
 * For user pmaps, if pteidx_p is non-NULL and the PTE for the mapping
 * is resident, the index of the PTE in pmap_pteg_table is written
 * to *pteidx_p.
 *
 * For kernel pmap, if pteidx_p is non-NULL, the index of the PTEL in
 * pmap_kernel_ipt is written to *pteidx_p.
 */
static struct pvo_entry *
pmap_pvo_find_va(pmap_t pm, vaddr_t va, int *ptegidx_p)
{
	struct pvo_head *pvo_head;
	struct pvo_entry *pvo;
	int idx;

	va = sh5_trunc_page(va);

	if (va < SH5_KSEG0_BASE) {
		idx = va_to_pteg(pm->pm_vsid, va);
		pvo_head = &pmap_upvo_table[idx];
	} else {
		if ((idx = kva_to_iptidx(va)) < 0)
			return (NULL);
		pvo_head = &pmap_kpvo_table[idx];
	}

	LIST_FOREACH(pvo, pvo_head, pvo_olink) {
		if (pvo->pvo_pmap == pm && PVO_VADDR(pvo) == va) {
			if (ptegidx_p)
				*ptegidx_p = idx;
			return (pvo);
		}
	}

	return (NULL);
}

/*
 * Sets up a single-page kernel mapping such that the Kernel Virtual
 * Address `kva' maps to the specific physical address `pa'.
 *
 * Note: The mapping is set up for read/write, with write-back cacheing
 * enabled,
 */
static void
pmap_pa_map_kva(vaddr_t kva, paddr_t pa, ptel_t wprot)
{
	ptel_t prot;
	int idx;

	/*
	 * Get the index into pmap_kernel_ipt.
	 */
	if ((idx = kva_to_iptidx(kva)) < 0)
		panic("pmap_pa_map_kva: Invalid KVA %p", (void *)kva);

	prot = SH5_PTEL_CB_WRITEBACK | SH5_PTEL_SZ_4KB | SH5_PTEL_PR_R;

	pmap_kernel_ipt_set_ptel(&pmap_kernel_ipt[idx],
	    (ptel_t)(pa & SH5_PTEL_PPN_MASK) | prot | wprot);
}

/*
 * The opposite of the previous function, this removes temporary and/or
 * unmanaged mappings.
 *
 * The contents of the PTEL which described the mapping are returned.
 */
static ptel_t
pmap_pa_unmap_kva(vaddr_t kva, kpte_t *kpte)
{
	ptel_t oldptel;
	int idx;

	if (kpte == NULL) {
		if ((idx = kva_to_iptidx(kva)) < 0)
			panic("pmap_pa_unmap_kva: Invalid KVA %p", (void *)kva);

		kpte = &pmap_kernel_ipt[idx];
	}

	oldptel = pmap_kernel_ipt_get_ptel(kpte);
	pmap_kernel_ipt_set_ptel(kpte, 0);

	pmap_cache_sync_unmap(kva, oldptel);

	if ((oldptel & SH5_PTEL_R) != 0) {
		cpu_tlbinv_cookie(
		    ((pteh_t)kva & SH5_PTEH_EPN_MASK) | SH5_PTEH_SH,
		    pmap_kernel_ipt_get_tlbcookie(kpte));
	}

	pmap_kernel_ipt_set_tlbcookie(kpte, 0);

	return (oldptel);
}

static void
pmap_change_cache_attr(struct pvo_entry *pvo, ptel_t new_mode)
{
	volatile pte_t *pt;
	vaddr_t va;
	int idx;
	ptel_t ptel;

	va = PVO_VADDR(pvo);

	PMPRINTF(("pmap_change_cache_attr: mode 0x%x for VA 0x%lx\n",
	    new_mode, va));

	if (va < SH5_KSEG0_BASE) {
		/* This has to be a user-space mapping.  */
		PMPRINTF(("pmap_change_cache_attr: user mapping "));
		/*
		 * There's a chance the mapping is currently
		 * live in the PTEG, and also in the TLB. We
		 * must remove it before changing the cache
		 * attribute.
		 */
		pt = pmap_pvo_to_pte(pvo, -1);
		if (pt != NULL) {
			pmap_pteg_unset(pt, pvo);
			PMPRINTF((" unset\n"));
		}
#ifdef DEBUG
		else
			PMPRINTF((" not in PTEG\n"));
#endif

		/* Change the cache attribute */
		pvo->pvo_ptel &= ~SH5_PTEL_CB_MASK;
		pvo->pvo_ptel |= new_mode;

		/* Re-insert it back into the page table if necessary */
		if (pt) {
			pmap_pteg_set(pt, pvo);
			PMPRINTF(("pmap_change_cache_attr: re-inserted\n"));
		}
	} else {
		/* Kernel mapping */
		idx = kva_to_iptidx(va);
		KDASSERT(idx >= 0);
		PMPRINTF(("pmap_change_cache_attr: kernel mapping at idx %d\n",
		    idx));
		ptel = pmap_pa_unmap_kva(va, &pmap_kernel_ipt[idx]);
		pmap_pteg_synch(ptel, pvo);

		/* Change the cache attribute */
		pvo->pvo_ptel &= ~SH5_PTEL_CB_MASK;
		pvo->pvo_ptel |= new_mode;

		/* Re-insert it back into the page table */
		pmap_kernel_ipt_set_ptel(&pmap_kernel_ipt[idx],
		    pvo->pvo_ptel & ~SH5_PTEL_R);
	}

	PMPRINTF(("pmap_change_cache_attr: done\n"));
}

/*
 * This returns whether this is the first mapping of a page.
 */
static int
pmap_pvo_enter(pmap_t pm, struct pool *pl, struct pvo_head *pvo_head,
	vaddr_t va, paddr_t pa, ptel_t ptel, int flags)
{
	struct pvo_head *pvo_table_head;
	struct pvo_entry *pvo, *pvop;
	int idx;
	int i, s;
	int poolflags = PR_NOWAIT;

	va = sh5_trunc_page(va);

#ifdef PMAP_DIAG
	if (pm == pmap_kernel() && va < SH5_KSEG1_BASE) {
		printf("pmap_pvo_enter: pmap_kernel() with va 0x%lx!\n", va);
		pmap_debugger();
	}

	if (pmap_pvo_remove_depth > 0) {
		printf("pmap_pvo_enter: pmap_pvo_remove active, for va 0x%lx\n",
		    va);
		pmap_debugger();
	}
	if (pmap_pvo_enter_depth) {
		printf("pmap_pvo_enter: called recursively for va 0x%lx\n", va);
		pmap_debugger();
	}
	pmap_pvo_enter_depth++;
#endif

	if (va < SH5_KSEG0_BASE) {
		idx = va_to_pteg(pm->pm_vsid, va);
		pvo_table_head = &pmap_upvo_table[idx];
	} else {
		idx = kva_to_iptidx(va);
		KDASSERT(idx >= 0);
		pvo_table_head = &pmap_kpvo_table[idx];
	}

	s = splvm();

	/*
	 * Remove any existing mapping for this virtual page.
	 */
	LIST_FOREACH(pvo, pvo_table_head, pvo_olink) {
		if (pvo->pvo_pmap == pm && PVO_VADDR(pvo) == va) {
			pmap_pvo_remove(pvo, idx);
			break;
		}
	}

	/*
	 * If we aren't overwriting a mapping, try to allocate
	 */
	pvo = pool_get(pl, poolflags);

	if (pvo == NULL) {
		if ((flags & PMAP_CANFAIL) == 0)
			panic("pmap_pvo_enter: failed");
#ifdef PMAP_DIAG
		pmap_pvo_enter_depth--;
#endif
		splx(s);
		return (ENOMEM);
	}

	ptel |= (ptel_t) (pa & SH5_PTEL_PPN_MASK);
	pvo->pvo_vaddr = va;
	pvo->pvo_pmap = pm;

	if (flags & VM_PROT_WRITE)
		pvo->pvo_vaddr |= PVO_WRITABLE;

	if (flags & PMAP_WIRED) {
		pvo->pvo_vaddr |= PVO_WIRED;
		pm->pm_stats.wired_count++;
	}

	if ((flags & PMAP_UNMANAGED) == 0)
		pvo->pvo_vaddr |= PVO_MANAGED; 

	if (SH5_PTEL_CACHEABLE(ptel))
		pvo->pvo_vaddr |= PVO_CACHEABLE;

	/*
	 * Check that the new mapping's cache-mode and VA/writable-attribute
	 * agree with any existing mappings for this physical page.
	 */
	if ((pvop = LIST_FIRST(pvo_head)) != NULL) {
		int cache_mode, pvo_cache_mode;

		pvo_cache_mode = PVO_ISCACHEABLE(pvop);
		cache_mode = PVO_ISCACHEABLE(pvo);

		PMPRINTF((
		   "pmap_pvo_enter: shared mapping: newva=0x%lx, pvova=0x%lx\n",
		    va, PVO_VADDR(pvop)));
		PMPRINTF((
		    "pmap_pvo_enter: shared mapping: newcm=%d, pvocm=%d\n",
		    cache_mode, pvo_cache_mode));

		if (cache_mode > pvo_cache_mode) {
			/*
			 * The new mapping is "cacheable", the existing
			 * mappings are "uncacheable". Simply downgrade
			 * the new mapping to match.
			 */
			cache_mode = 0;
		} else
		if (cache_mode < pvo_cache_mode) {
			/*
			 * The new mapping is "uncachable", the existing
			 * mappings are "cacheable". We need to downgrade
			 * all the existing mappings...
			 */
			pvo_cache_mode = 0;
		}

		if (va != PVO_VADDR(pvop) &&
		    (PVO_ISWRITABLE(pvo) || PVO_ISWRITABLE(pvop))) {
			/*
			 * Adding a writable mapping at different VA.
			 * Downgrade all the mappings to uncacheable.
			 *
			 * XXX: We could probably use the less-restrictive
			 * nsynbits/nsynmax as defined in the SH5
			 * documentation. In practice, it's probably not
			 * worth it.
			 */
			cache_mode = 0;
			pvo_cache_mode = 0;
		}

		if (pvo_cache_mode != PVO_ISCACHEABLE(pvop)) {
			/*
			 * Downgrade the cache-mode of the existing mappings.
			 */
			LIST_FOREACH(pvop, pvo_head, pvo_vlink) {
				if (PVO_ISCACHEABLE(pvop) <= pvo_cache_mode)
					continue;

				pmap_change_cache_attr(pvop,
				    SH5_PTEL_CB_NOCACHE);
				pmap_shared_cache_downgrade_events.ev_count++;
			}
		}

		ptel &= ~SH5_PTEL_CB_MASK;
		ptel |= cache_mode ?
		    SH5_PTEL_CB_WRITEBACK : SH5_PTEL_CB_NOCACHE;
	}

	pvo->pvo_ptel = ptel;

	LIST_INSERT_HEAD(pvo_table_head, pvo, pvo_olink);
	LIST_INSERT_HEAD(pvo_head, pvo, pvo_vlink);

	pm->pm_stats.resident_count++;

	if (va < SH5_KSEG0_BASE) {
		/*
		 * We hope this succeeds but it isn't required.
		 */
		i = pmap_pteg_insert(&pmap_pteg_table[idx], pvo);
		PMPRINTF((
              "pmap_pvo_enter: va 0x%lx vsid %x ptel 0x%lx group 0x%x idx %d\n",
		    va, pm->pm_vsid, (u_long)ptel, idx, i));
		if (i >= 0) {
			pmap_pteg_idx_events[i].ev_count++;
			PVO_PTEGIDX_SET(pvo, i);
		}
	} else {
		pmap_kernel_ipt_set_ptel(&pmap_kernel_ipt[idx],
		    ptel & ~SH5_PTEL_R);
		PMPRINTF((
		    "pmap_pvo_enter: kva 0x%lx, ptel 0x%lx, kipt (idx %d)\n",
		    va, (u_long)ptel, idx));
	}

#ifdef PMAP_DIAG
	pmap_pvo_enter_depth--;
#endif
	splx(s);

	return (0);
}

static void
pmap_pvo_remove(struct pvo_entry *pvo, int idx)
{
	struct vm_page *pg = NULL;
	vaddr_t va;

	va = PVO_VADDR(pvo);

#ifdef PMAP_DIAG
	if (pmap_pvo_remove_depth > 0) {
		printf("pmap_pvo_remove: called recusively, for va 0x%lx\n",va);
		pmap_debugger();
	}
	pmap_pvo_remove_depth++;
#endif

	PMPRINTF(("pmap_pvo_remove: removing 0x%lx at idx %d\n", va, idx));

	if (va < SH5_KSEG0_BASE) {
		volatile pte_t *pt;

		/*
		 * Lookup the pte and unmap it.
		 */
		pt = pmap_pvo_to_pte(pvo, idx);
		if (pt != NULL) {
			PMPRINTF(("pmap_pvo_remove: removing user mapping\n"));
			pmap_pteg_unset(pt, pvo);
		}
#ifdef DEBUG
		else
			PMPRINTF((
			    "pmap_pvo_remove: user mapping not in PTEG\n"));
#endif
	} else {
		ptel_t ptel;

		if (idx < 0) {
			idx = kva_to_iptidx(va);
			PMPRINTF(("pmap_pvo_remove: real idx is %d\n", idx));
		}

		KDASSERT(idx >= 0);
		ptel = pmap_pa_unmap_kva(va, &pmap_kernel_ipt[idx]);
		pmap_pteg_synch(ptel, pvo);
		PMPRINTF(("pmap_pvo_remove: old KVA ptel was 0x%x\n", ptel));
	}

	/*
	 * Update our statistics
	 */
	pvo->pvo_pmap->pm_stats.resident_count--;
	if (PVO_ISWIRED(pvo))
		pvo->pvo_pmap->pm_stats.wired_count--;

	/*
	 * Save the REF/CHG bits into their cache if the page is managed.
	 */
	pg = PHYS_TO_VM_PAGE((paddr_t)(pvo->pvo_ptel & SH5_PTEL_PPN_MASK));
	if (pg && PVO_ISMANAGED(pvo) && !PVO_ISWIRED(pvo))
		pmap_attr_save(pg, pvo->pvo_ptel & (SH5_PTEL_R | SH5_PTEL_M));

	/*
	 * Remove this PVO from the PV list
	 */
	LIST_REMOVE(pvo, pvo_vlink);

	/*
	 * Remove this from the Overflow list and return it to the pool...
	 * ... if we aren't going to reuse it.
	 */
	LIST_REMOVE(pvo, pvo_olink);

	pool_put(PVO_ISMANAGED(pvo) ? &pmap_mpvo_pool : &pmap_upvo_pool, pvo);

	/*
	 * If there are/were multiple mappings for the same physical
	 * page, we may just have removed one which had caused the
	 * others to be made uncacheable.
	 */
	if (pg)
		pmap_sharing_policy(&pg->mdpage.mdpg_pvoh);

#ifdef PMAP_DIAG
	pmap_pvo_remove_depth--;
#endif
	PMPRINTF(("pmap_pvo_remove: done\n"));
}

void
pmap_sharing_policy(struct pvo_head *pvo_head)
{
	struct pvo_entry *pvo;
	vaddr_t va;
	int writable;

	/*
	 * XXX: Make sure this doesn't happen
	 */
	KDASSERT(pvo_head != &pmap_pvo_unmanaged);

	PMPRINTF(("pmap_sharing_policy: "));

	if ((pvo = LIST_FIRST(pvo_head)) == NULL || !PVO_ISCACHEABLE(pvo)) {
		/*
		 * There are no more mappings for this physical page,
		 * or the existing mappings are all non-cacheable anyway.
		 */
#ifdef DEBUG
		if (pvo == NULL)
			PMPRINTF(("no shared mappings\n"));
		else
			PMPRINTF(("all mappings uncacheable\n"));
#endif
		return;
	}

	if (SH5_PTEL_CACHEABLE(pvo->pvo_ptel)) {
		/*
		 * The remaining mappings must already be cacheable, so
		 * nothing more needs doing.
		 */
		PMPRINTF(("all mappings already cacheable:\n"));
#ifdef DEBUG
		if (pmap_debug == 0)
			return;
		LIST_FOREACH(pvo, pvo_head, pvo_vlink) {
			PMPRINTF((
			    "                   pm %p, va 0x%lx, ptel 0x%x\n",
			    pvo->pvo_pmap, pvo->pvo_vaddr, pvo->pvo_ptel));
		}
#endif
		return;
	}

	/*
	 * We have to scan the remaining mappings to see if they can all
	 * be made cacheable.
	 */

	va = PVO_VADDR(pvo);
	writable = 0;

	/*
	 * First pass: See if all the mappings are cache-coherent
	 */
	LIST_FOREACH(pvo, pvo_head, pvo_vlink) {
		if (!PVO_ISCACHEABLE(pvo)) {
			/*
			 * At least one of the mappings really is
			 * non-cacheable. This means all the others
			 * must remain uncacheable. Since this must
			 * already be the case, there's nothing more
			 * to do.
			 *
			 * XXX: This test is probably unecessary; we check
			 * the first mapping at the top of the function
			 * anyway...
			 */
			PMPRINTF(("all mappings uncacheable (XXX)\n"));
			return;
		}

		if (PVO_ISWRITABLE(pvo)) {
			/*
			 * Record the fact that at least one of the
			 * mappings is writable.
			 */
			writable = 1;
		}

		if (va != PVO_VADDR(pvo) && writable) {
			/*
			 * Oops, one or more of the mappings are writable
			 * and one or more of the VAs are different.
			 * There's nothing more to do.
			 *
			 * XXX: Could relax this to checking nsynmax/nsynbits.
			 */
			PMPRINTF(("writable at different VAs\n"));
			return;
		}
	}

	PMPRINTF(("cache can be enabled\n"));

	/*
	 * Looks like we can re-enable cacheing for all the
	 * remaining mappings.
	 */
	LIST_FOREACH(pvo, pvo_head, pvo_vlink) {
		PMPRINTF(("pmap_sharing_policy: enabling cache for 0x%lx\n",
		    PVO_VADDR(pvo)));
		pmap_change_cache_attr(pvo, SH5_PTEL_CB_WRITEBACK);
		pmap_shared_cache_upgrade_events.ev_count++;
	}

	PMPRINTF(("pmap_sharing_policy: done\n"));
}

int
pmap_enter(pmap_t pm, vaddr_t va, paddr_t pa, vm_prot_t prot, int flags)
{
	struct pvo_head *pvo_head;
	struct mem_region *mp;
	struct vm_page *pg;
	struct pool *pl;
	ptel_t ptel;
	int error;
	int s;

	PMPRINTF((
	    "pmap_enter: %s: va 0x%lx, pa 0x%lx, prot 0x%x, flags 0x%x\n",
	    PMSTR(pm), va, pa, (u_int)prot, (u_int)flags));

#ifdef PMAP_DIAG
	if (pm == pmap_kernel() && va < SH5_KSEG1_BASE) {
		printf("pmap_enter: pmap_kernel() with va 0x%08x!!\n",
		    (u_int)va);
		pmap_debugger();
	}
#endif

	if (__predict_false(!pmap_initialized)) {
		pvo_head = &pmap_pvo_unmanaged;
		pl = &pmap_upvo_pool;
		pg = NULL;
		flags |= PMAP_UNMANAGED;
	} else {
		pvo_head = pa_to_pvoh(pa, &pg);
		pl = &pmap_mpvo_pool;
	}

	/*
	 * Mark the page as Device memory, unless it's in our
	 * available memory array.
	 */
	ptel = SH5_PTEL_CB_DEVICE;
	for (mp = mem; mp->mr_size; mp++) {
		if (pa >= mp->mr_start &&
		    pa < (mp->mr_start + mp->mr_size)) {
			if ((flags & PMAP_NC) == 0)
				ptel = SH5_PTEL_CB_WRITEBACK;
			else
				ptel = SH5_PTEL_CB_NOCACHE;
			break;
		}
	}

	/* Pages are always readable */
	ptel |= SH5_PTEL_PR_R;

	if (pg && (flags & PMAP_UNMANAGED) == 0) {
		/* Only managed pages can be user-accessible */
		if (va <= VM_MAXUSER_ADDRESS)
			ptel |= SH5_PTEL_PR_U;

		/*
		 * Pre-load mod/ref status according to the hint in `flags'.
		 *
		 * Note that managed pages are initially read-only, unless
		 * the hint indicates they are writable. This allows us to
		 * track page modification status by taking a write-protect
		 * fault later on.
		 */
		if (flags & VM_PROT_WRITE)
			ptel |= SH5_PTEL_R | SH5_PTEL_M | SH5_PTEL_PR_W;
		else
		if (flags & VM_PROT_ALL)
			ptel |= SH5_PTEL_R;
	} else {
		flags |= PMAP_UNMANAGED;
		pl = &pmap_upvo_pool;
		if (prot & VM_PROT_WRITE) {
			/* Unmanaged pages are writable from the start.  */
			ptel |= SH5_PTEL_PR_W;
		}
	}

	if (prot & VM_PROT_EXECUTE)
		ptel |= SH5_PTEL_PR_X;

	flags |= (prot & (VM_PROT_EXECUTE | VM_PROT_WRITE));

	s = splvm();
	if (pg && (flags & PMAP_UNMANAGED) == 0)
		pmap_attr_save(pg, ptel & (SH5_PTEL_R | SH5_PTEL_M));
	error = pmap_pvo_enter(pm, pl, pvo_head, va, pa, ptel, flags);
	splx(s);

	return (error);
}

void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot)
{
	struct mem_region *mp;
	ptel_t ptel;

	PMPRINTF(("pmap_kenter_pa: va 0x%lx, pa 0x%lx, prot 0x%x\n",
	    va, pa, prot));

	if (va < pmap_kva_avail_start)
		panic("pmap_kenter_pa: Entering non-kernel VA: 0x%lx", va);

	ptel = SH5_PTEL_CB_DEVICE;
	for (mp = mem; mp->mr_size; mp++) {
		if (pa >= mp->mr_start && pa < mp->mr_start + mp->mr_size) {
			ptel = SH5_PTEL_CB_WRITEBACK;
			break;
		}
	}

	ptel |= SH5_PTEL_PR_R | (ptel_t)(pa & SH5_PTEL_PPN_MASK);
	if (prot & VM_PROT_WRITE)
		ptel |= SH5_PTEL_PR_W;

	pmap_kernel_ipt_set_ptel(&pmap_kernel_ipt[kva_to_iptidx(va)], ptel);

	PMPRINTF(("pmap_kenter_pa: done\n"));
}

void
pmap_kremove(vaddr_t va, vsize_t len)
{

	if (va < pmap_device_kva_start)
		panic("pmap_kremove: Entering non-kernel VA: 0x%lx", va);

	PMPRINTF(("pmap_kremove: va 0x%lx, len 0x%lx\n", va, len));

	pmap_remove(pmap_kernel(), va, va + len);

	PMPRINTF(("pmap_kremove: done\n"));
}

/*
 * Remove the given range of mapping entries.
 */
void
pmap_remove(pmap_t pm, vaddr_t va, vaddr_t endva)
{
	struct pvo_entry *pvo;
	int idx;
	int s;

	PMPRINTF(("pmap_remove: %s: va 0x%lx, endva 0x%lx\n",
	    PMSTR(pm), va, endva));

#ifdef PMAP_DIAG
	if (pm == pmap_kernel() && va < SH5_KSEG1_BASE) {
		printf("pmap_remove: pmap_kernel() with va 0x%08x!!\n",
		    (u_int)va);
		pmap_debugger();
	}
#endif

	for (; va < endva; va += PAGE_SIZE) {
		s = splvm();
		pvo = pmap_pvo_find_va(pm, va, &idx);
		if (pvo != NULL)
			pmap_pvo_remove(pvo, idx);
		else
		if (pm == pmap_kernel() && (idx = kva_to_iptidx(va)) >= 0) {
#ifdef PMAP_DIAG
			if ((pmap_kernel_ipt[idx].ptel & SH5_PTEL_CB_MASK) !=
			    SH5_PTEL_CB_DEVICE) {
			printf("pmap_remove: no pvo for non-device mapping!\n");
				pmap_debugger();
			}
#endif
			if (pmap_kernel_ipt_get_ptel(&pmap_kernel_ipt[idx]))
		    		pmap_pa_unmap_kva(va, &pmap_kernel_ipt[idx]);
		}
		splx(s);
	}
}

/*
 * Get the physical page address for the given pmap/virtual address.
 */
boolean_t
pmap_extract(pmap_t pm, vaddr_t va, paddr_t *pap)
{
	struct pvo_entry *pvo;
	int s, idx;
	boolean_t found = FALSE;

	PMPRINTF(("pmap_extract: %s: va 0x%lx - ", PMSTR(pm), va));

	/*
	 * We can be called from the bus_dma code for addresses in KSEG0.
	 * This would be the case, for example, of bus_dmamap_load() being
	 * called for a buffer allocated by pool(9).
	 */
	if (pm == pmap_kernel() &&
	    va < SH5_KSEG1_BASE && va >= SH5_KSEG0_BASE) {
		*pap = (paddr_t)(va - SH5_KSEG0_BASE) + pmap_kseg0_pa;
		PMPRINTF(("KSEG0. pa 0x%lx\n", *pap));
		return (TRUE);
	}

	s = splvm();
	pvo = pmap_pvo_find_va(pm, va, NULL);
	if (pvo != NULL) {
		*pap = (pvo->pvo_ptel & SH5_PTEL_PPN_MASK) |
		    sh5_page_offset(va);
		found = TRUE;
		PMPRINTF(("%smanaged pvo. pa 0x%lx\n",
		    PVO_ISMANAGED(pvo) ? "" : "un", *pap));
	} else
	if (pm == pmap_kernel()) {
		ptel_t ptel;
		idx = kva_to_iptidx(va);
		if (idx >= 0 &&
		    (ptel = pmap_kernel_ipt_get_ptel(&pmap_kernel_ipt[idx]))) {
			*pap = (ptel & SH5_PTEL_PPN_MASK) | sh5_page_offset(va);
			found = TRUE;
			PMPRINTF(("no pvo, but kipt pa 0x%lx\n", *pap));
		}
	}

#ifdef DEBUG
	if (!found)
		PMPRINTF(("not found.\n"));
#endif

	splx(s);

	return (found);
}

/*
 * Lower the protection on the specified range of this pmap.
 *
 * There are only two cases: either the protection is going to 0,
 * or it is going to read-only.
 */
void
pmap_protect(pmap_t pm, vaddr_t va, vaddr_t endva, vm_prot_t prot)
{
	struct pvo_entry *pvo;
	volatile pte_t *pt;
	ptel_t clrbits;
	int s, idx;

	PMPRINTF(("pmap_protect: %s: va 0x%lx, endva 0x%lx, prot 0x%x\n",
	    PMSTR(pm), va, endva, (u_int)prot));

#ifdef PMAP_DIAG
	if (pm == pmap_kernel() && va < SH5_KSEG1_BASE) {
		printf("pmap_protect: pmap_kernel() with va 0x%lx!!\n", va);
		pmap_debugger();
	}
#endif

	/*
	 * If there is no protection, this is equivalent to
	 * removing the VA range from the pmap.
	 */
	if ((prot & VM_PROT_READ) == VM_PROT_NONE) {
		pmap_remove(pm, va, endva);
		PMPRINTF(("pmap_protect: done removing mapping\n"));
		return;
	}

	if ((prot & VM_PROT_EXECUTE) == 0)
		clrbits = SH5_PTEL_PR_W | SH5_PTEL_PR_X;
	else
		clrbits = SH5_PTEL_PR_W;

	/*
	 * We're doing a write-protect, and perhaps an execute-revoke.
	 */

	for (; va < endva; va += PAGE_SIZE) {
		s = splvm();
		pvo = pmap_pvo_find_va(pm, va, &idx);
		if (pvo == NULL) {
			PMPRINTF(("pmap_protect: no pvo for VA 0x%lx\n", va));
			splx(s);
			continue;
		}

		PMPRINTF(("pmap_protect: protecting VA 0x%lx, PA 0x%lx\n",
		    va, (paddr_t)(pvo->pvo_ptel & SH5_PTEL_PPN_MASK)));

		KDASSERT(!PVO_ISWIRED(pvo));

		pvo->pvo_ptel &= ~clrbits;
		pvo->pvo_vaddr &= ~PVO_WRITABLE;

		/*
		 * Propagate the change to the page-table/TLB
		 */
		if (PVO_VADDR(pvo) < SH5_KSEG0_BASE) {
			pt = pmap_pvo_to_pte(pvo, idx);
			if (pt != NULL)
				pmap_pteg_change(pt, pvo);
		} else {
			KDASSERT(idx >= 0);
			if (pmap_kernel_ipt_get_ptel(&pmap_kernel_ipt[idx]) &
			    clrbits)
				pmap_kpte_clear_bit(idx, pvo, clrbits);
		}

		/*
		 * Since the mapping is being made readonly,
		 * if there are other mappings for the same
		 * physical page, we may now be able to
		 * make them cacheable.
		 *
		 * XXX: This could end up re-doing the cache-
		 * purge/TLB sync for the above mapping.
		 */
		pmap_sharing_policy(
		    pa_to_pvoh((paddr_t)(pvo->pvo_ptel & SH5_PTEL_PPN_MASK),
		    NULL));

		splx(s);
	}

	PMPRINTF(("pmap_protect: done lowering protection.\n"));
}

void
pmap_unwire(pmap_t pm, vaddr_t va)
{
	struct pvo_entry *pvo;
	int s;

	PMPRINTF(("pmap_unwire: %s: va 0x%lx\n", PMSTR(pm), va));

#ifdef PMAP_DIAG
	if (pm == pmap_kernel() && va < SH5_KSEG1_BASE) {
		printf("pmap_unwire: pmap_kernel() with va 0x%lx!!\n", va);
		pmap_debugger();
	}
#endif

	s = splvm();

	pvo = pmap_pvo_find_va(pm, va, NULL);
	if (pvo != NULL) {
		if (PVO_ISWIRED(pvo)) {
			pvo->pvo_vaddr &= ~PVO_WIRED;
			pm->pm_stats.wired_count--;
		}
	}

	splx(s);
}

/*
 * Lower the protection on the specified physical page.
 *
 * There are only two cases: either the protection is going to 0,
 * or it is going to read-only.
 */
void
pmap_page_protect(struct vm_page *pg, vm_prot_t prot)
{
	struct pvo_head *pvo_head;
	struct pvo_entry *pvo, *next_pvo;
	volatile pte_t *pt;
	ptel_t clrbits;
	int idx, s;

	PMPRINTF(("pmap_page_protect: pa 0x%lx prot %x: ",
	    pg->phys_addr, prot));

	/*
	 * Since this routine only downgrades protection, if the
	 * maximal protection is desired, there isn't any change
	 * to be made.
	 */
	if ((prot & (VM_PROT_READ|VM_PROT_WRITE)) ==
	    (VM_PROT_READ|VM_PROT_WRITE)) {
		PMPRINTF(("maximum requested. all done.\n"));
		return;
	}

	if ((prot & VM_PROT_EXECUTE) == 0)
		clrbits = SH5_PTEL_PR_W | SH5_PTEL_PR_X;
	else
		clrbits = SH5_PTEL_PR_W;

	s = splvm();

	pvo_head = vm_page_to_pvoh(pg);
	for (pvo = LIST_FIRST(pvo_head); pvo != NULL; pvo = next_pvo) {
		next_pvo = LIST_NEXT(pvo, pvo_vlink);

		/*
		 * Mappings entered using pmap_kenter_pa() can be ignored
		 */
		if (!PVO_ISMANAGED(pvo) || PVO_ISWIRED(pvo)) {
			PMPRINTF((
			    "pmap_page_protect: ignoring wired VA 0x%lx\n",
			    PVO_VADDR(pvo)));
			continue;
		}

		/*
		 * Downgrading to no mapping at all, we just remove the entry.
		 */
		if ((prot & VM_PROT_READ) == 0) {
			PMPRINTF(("pmap_page_protect: removing VA 0x%lx\n",
			    PVO_VADDR(pvo)));
			pmap_pvo_remove(pvo, -1);
			continue;
		} 

		pvo->pvo_ptel &= ~clrbits;
		pvo->pvo_vaddr &= ~PVO_WRITABLE;

		/*
		 * Propagate the change to the page-table/TLB
		 */
		if (PVO_VADDR(pvo) < SH5_KSEG0_BASE) {
			pt = pmap_pvo_to_pte(pvo, -1);
			if (pt != NULL)
				pmap_pteg_change(pt, pvo);
		} else {
			idx = kva_to_iptidx(PVO_VADDR(pvo));
			KDASSERT(idx >= 0);
			if (pmap_kernel_ipt_get_ptel(&pmap_kernel_ipt[idx]) &
			    clrbits)
				pmap_kpte_clear_bit(idx, pvo, clrbits);
		}

		/*
		 * Since the mapping is being made readonly,
		 * if there are other mappings for the same
		 * physical page, we may now be able to
		 * make them cacheable.
		 *
		 * XXX: This could end up re-doing the cache-
		 * purge/TLB sync for the above mapping.
		 */
		pmap_sharing_policy(pvo_head);
	}

	PMPRINTF(("all done.\n"));

	splx(s);
}

/*
 * Activate the address space for the specified process.  If the process
 * is the current process, load the new MMU context.
 */
void
pmap_activate(struct lwp *l)
{
	pmap_t pm = l->l_proc->p_vmspace->vm_map.pmap;
	vsid_t old_vsid;

	pmap_asid_alloc(pm);

	if (l->l_proc == curlwp->l_proc) {
		old_vsid = curcpu()->ci_curvsid;
		curcpu()->ci_curvsid = pm->pm_vsid;
		sh5_setasid(pm->pm_asid);

		/*
		 * If the CPU has to invalidate the instruction cache
		 * on a context switch, do it now. But only if we're
		 * not resuming in the same pmap context.
		 */
		if (cpu_cache_iinv_all && old_vsid != pm->pm_vsid)
			cpu_cache_iinv_all();
	}
}


/*
 * Deactivate the specified LWP's address space.
 */
void
pmap_deactivate(struct lwp *p)
{
}

boolean_t
pmap_query_bit(struct vm_page *pg, ptel_t ptebit)
{
	struct pvo_entry *pvo;
	volatile pte_t *pt;
	int s, idx;

	PMPRINTF(("pmap_query_bit: pa 0x%lx %s? - ", pg->phys_addr,
	    (ptebit == SH5_PTEL_M) ? "modified" : "referenced"));

	if ((ptel_t)pmap_attr_fetch(pg) & ptebit) {
		PMPRINTF(("yes. Cached in pg attr.\n"));
		return (TRUE);
	}

	s = splvm();

	LIST_FOREACH(pvo, vm_page_to_pvoh(pg), pvo_vlink) {
		/*
		 * See if we saved the bit off.  If so cache, it and return
		 * success.
		 */
		if (PVO_ISMANAGED(pvo) && !PVO_ISWIRED(pvo) &&
		    pvo->pvo_ptel & ptebit) {
			pmap_attr_save(pg, ptebit);
			PMPRINTF(("yes. Cached in pvo for 0x%lx.\n",
			    PVO_VADDR(pvo)));
			splx(s);
			return (TRUE);
		}
	}

	/*
	 * No luck, now go thru the hard part of looking at the ptes
	 * themselves.  Sync so any pending REF/CHG bits are flushed
	 * to the PTEs.
	 */
	LIST_FOREACH(pvo, vm_page_to_pvoh(pg), pvo_vlink) {
		if (!PVO_ISMANAGED(pvo) || PVO_ISWIRED(pvo))
			continue;

		if (PVO_VADDR(pvo) < SH5_KSEG0_BASE) {
			/*
			 * See if this pvo have a valid PTE.  If so, fetch the
			 * REF/CHG bits from the valid PTE.  If the appropriate
			 * ptebit is set, cache, it and return success.
			 */
			pt = pmap_pvo_to_pte(pvo, -1);
			if (pt != NULL) {
				pmap_pteg_synch(pt->ptel, pvo);
				if (pvo->pvo_ptel & ptebit) {
					pmap_attr_save(pg, ptebit);
					PMPRINTF((
					    "yes. Cached in ptel for 0x%lx.\n",
					    PVO_VADDR(pvo)));
					splx(s);
					return (TRUE);
				}
			}
		} else {
			ptel_t ptel;
			idx = kva_to_iptidx(PVO_VADDR(pvo));
			KDASSERT(idx >= 0);
			ptel = pmap_kernel_ipt_get_ptel(&pmap_kernel_ipt[idx]);
			if (ptel & ptebit) {
				pmap_pteg_synch(ptel, pvo);
				pmap_attr_save(pg, ptebit);
				PMPRINTF(("yes. Cached in kipt for 0x%lx.\n",
				    PVO_VADDR(pvo)));
				splx(s);
				return (TRUE);
			}
		}
	}

	PMPRINTF(("no.\n"));

	splx(s);

	return (FALSE);
}

boolean_t
pmap_clear_bit(struct vm_page *pg, ptel_t ptebit)
{
	struct pvo_entry *pvo;
	volatile pte_t *pt;
	int rv = 0;
	int s, idx;

	PMPRINTF(("pmap_clear_bit: pa 0x%lx %s - ", pg->phys_addr,
	    (ptebit == SH5_PTEL_M) ? "modified" : "referenced"));

	s = splvm();

	/*
	 * Clear the cached value.
	 */
	rv |= pmap_attr_fetch(pg);
	pmap_attr_clear(pg, ptebit);

	/*
	 * If clearing the Modified bit, make sure to clear the
	 * writable bit in the PTEL/TLB so we can track future
	 * modifications to the page via pmap_write_trap().
	 */
	if (ptebit & SH5_PTEL_M)
		ptebit |= SH5_PTEL_PR_W;

	/*
	 * For each pvo entry, clear pvo's ptebit.  If this pvo has a
	 * valid PTE.  If so, clear the ptebit from the valid PTE.
	 */
	LIST_FOREACH(pvo, vm_page_to_pvoh(pg), pvo_vlink) {
		if (!PVO_ISMANAGED(pvo) || PVO_ISWIRED(pvo))
			continue;

		if (PVO_VADDR(pvo) < SH5_KSEG0_BASE) {
			pt = pmap_pvo_to_pte(pvo, -1);
			if (pt != NULL) {
				if ((pvo->pvo_ptel & ptebit) == 0)
					pmap_pteg_synch(pt->ptel, pvo);
				if (pvo->pvo_ptel & ptebit)
					pmap_pteg_clear_bit(pt, pvo, ptebit);
			}
		} else {
			ptel_t ptel;
			idx = kva_to_iptidx(PVO_VADDR(pvo));
			KDASSERT(idx >= 0);
			ptel = pmap_kernel_ipt_get_ptel(&pmap_kernel_ipt[idx]);
			if (ptel != 0) {
				if ((pvo->pvo_ptel & ptebit) == 0)
					pmap_pteg_synch( ptel, pvo);
				if (pvo->pvo_ptel & ptebit)
					pmap_kpte_clear_bit(idx, pvo, ptebit);
			}
		}

		rv |= pvo->pvo_ptel & ptebit;
		pvo->pvo_ptel &= ~ptebit;
	}

	ptebit &= ~SH5_PTEL_PR_W;
	PMPRINTF(("was %s.\n", (rv & ptebit) ? "cleared" : "not set"));

	splx(s);

	return ((rv & ptebit) != 0);
}

static void
pmap_asid_alloc(pmap_t pm)
{
	int s;

	if (pm == pmap_kernel())
		return;

	if (pm->pm_asid == PMAP_ASID_UNASSIGNED ||
	    pm->pm_asidgen != pmap_asid_generation) {
		s = splvm();
		if (pmap_asid_next > pmap_asid_max) {
			PMPRINTF(("pmap_asid_alloc: wrapping ASIDs\n"));

			/*
			 * Note: Current SH5 cpus do not use the ASID when
			 * matching cache tags (even though they keep a copy
			 * of the ASID which allocated each cacheline, in
			 * the tag itself).
			 *
			 * Therefore, we don't need to purge the cache since
			 * the virtual to physical mappings themselves haven't
			 * changed. So the cache contents are still valid.
			 *
			 * We do need to invalidate the TLB, however.
			 */
#if 0
			cpu_cache_purge_all();
#endif
			cpu_tlbinv_all();

			pmap_asid_generation++;
			pmap_asid_next = PMAP_ASID_USER_START;
			pmap_asid_regen_events.ev_count++;
		}

		pm->pm_asid = pmap_asid_next++;
		pm->pm_asidgen = pmap_asid_generation;
		splx(s);
	}
}

/*
 * Called from trap() when a write-protection trap occurs.
 * Return non-zero if we made the page writable so that the faulting
 * instruction can be restarted.
 * Otherwise, the page really is read-only.
 */
int
pmap_write_trap(struct proc *p, int usermode, vaddr_t va)
{
	struct pvo_entry *pvo;
	volatile pte_t *pt;
	kpte_t *kpte;
	pmap_t pm;
	int s, idx;

	if (p)
		pm = p->p_vmspace->vm_map.pmap;
	else {
		KDASSERT(usermode == 0);
		pm = pmap_kernel();
	}

	PMPRINTF(("pmap_write_trap: %s: %smode va 0x%lx - ",
	    PMSTR(pm), usermode ? "user" : "kernel", va));

	s = splvm();
	pvo = pmap_pvo_find_va(pm, va, &idx);
	if (pvo == NULL) {
		splx(s);
		PMPRINTF(("page has no pvo.\n"));
		return (0);
	}

	if (!PVO_ISWRITABLE(pvo)) {
		splx(s);
		PMPRINTF(("page is read-only.\n"));
		return (0);
	}

	pvo->pvo_ptel |= SH5_PTEL_PR_W | SH5_PTEL_R | SH5_PTEL_M;
	pvo->pvo_vaddr |= PVO_MODIFIED | PVO_REFERENCED;

	if (PVO_VADDR(pvo) < SH5_KSEG0_BASE) {
		pt = pmap_pvo_to_pte(pvo, idx);
		if (pt != NULL)
			pmap_pteg_change(pt, pvo);
	} else {
		KDASSERT(idx >= 0);
		kpte = &pmap_kernel_ipt[idx];

		/*
		 * XXX: Technically, we shouldn't need to purge the dcache
		 * here as we're lowering the protection of this page
		 * (enabling write).
		 *
		 * See SH-5 Volume 1: Architecture, Section 17.8.3, Point 5.
		 *
		 * However, since we're also going to invalidate the TLB
		 * entry, we're technically *increasing* the protection of
		 * the page, albeit temporarily, so the purge is required.
		 *
		 * If we were able to poke the new PTEL right into the same
		 * TLB slot at this point, we could avoid messing with the
		 * cache (assuming the entry hasn't been evicted by some
		 * other TLB miss in the meantime).
		 */
		if (SH5_PTEL_CACHEABLE(pvo->pvo_ptel)) {
			cpu_cache_dpurge(PVO_VADDR(pvo),
			    (paddr_t)(pvo->pvo_ptel & SH5_PTEL_PPN_MASK),
			    PAGE_SIZE);
		}

		cpu_tlbinv_cookie((pteh_t)PVO_VADDR(pvo) | SH5_PTEH_SH,
		    pmap_kernel_ipt_get_tlbcookie(kpte));

		pmap_kernel_ipt_set_tlbcookie(kpte, 0);
		pmap_kernel_ipt_set_ptel(kpte, pvo->pvo_ptel);
	}

	splx(s);

	PMPRINTF(("page made writable.\n"));

	return (1);
}

#ifdef PMAP_MAP_POOLPAGE
vaddr_t
pmap_map_poolpage(paddr_t pa)
{
	struct mem_region *mp;

	for (mp = mem; mp->mr_size; mp++) {
		if (pa >= mp->mr_start && pa < (mp->mr_start + mp->mr_size))
			break;
	}

	if (mp->mr_size && mp->mr_kvastart < SH5_KSEG1_BASE)
		return (mp->mr_kvastart + (vaddr_t)(pa - mp->mr_start));

	/*
	 * We should really be allowed to fail here to force allocation
	 * using the default method.
	 */
	panic("pmap_map_poolpage: non-kseg0 page: pa = 0x%lx", pa);
	return (0);
}
#endif

#ifdef PMAP_UNMAP_POOLPAGE
paddr_t
pmap_unmap_poolpage(vaddr_t va)
{
	struct mem_region *mp;
	paddr_t pa;

	for (mp = mem; mp->mr_size; mp++) {
		if (va >= mp->mr_kvastart &&
		    va < (mp->mr_kvastart + mp->mr_size))
			break;
	}

	if (mp->mr_size && mp->mr_kvastart < SH5_KSEG1_BASE) {
		pa = mp->mr_start + (paddr_t)(va - mp->mr_kvastart);
		cpu_cache_dpurge(va, pa, PAGE_SIZE);
		return (pa);
	}

	/*
	 * We should really be allowed to fail here
	 */
	panic("pmap_unmap_poolpage: non-kseg0 page: va = 0x%lx", va);
	return (0);
}
#endif

static void *
pmap_pool_ualloc(struct pool *pp, int flags)
{
	struct pvo_page *pvop;

	pvop = SIMPLEQ_FIRST(&pmap_upvop_head);
	if (pvop != NULL) {
		pmap_upvop_free--;
		SIMPLEQ_REMOVE_HEAD(&pmap_upvop_head, pvop_link);
		return (pvop);
	}

	if (uvm.page_init_done != TRUE)
		return ((void *)uvm_pageboot_alloc(PAGE_SIZE));

	return ((void *)uvm_km_alloc_poolpage((flags & PR_WAITOK)?TRUE:FALSE));
}

static void
pmap_pool_ufree(struct pool *pp, void *va)
{
	struct pvo_page *pvop = va;

	SIMPLEQ_INSERT_HEAD(&pmap_upvop_head, pvop, pvop_link);
	pmap_upvop_free++;
}

vaddr_t
pmap_steal_memory(vsize_t vsize, vaddr_t *vstartp, vaddr_t *vendp)
{
	vsize_t size;
	vaddr_t va;
	paddr_t pa = 0;
	paddr_t kseg0;
	int npgs, bank;
	struct vm_physseg *ps;

	if (uvm.page_init_done == TRUE)
		panic("pmap_steal_memory: called _after_ bootstrap");

	size = round_page(vsize);
	npgs = atop(size);
	kseg0 = atop(pmap_kseg0_pa);

	/*
	 * PA 0 will never be among those given to UVM so we can use it
	 * to indicate we couldn't steal any memory.
	 */
	for (ps = vm_physmem, bank = 0; bank < vm_nphysseg; bank++, ps++) {
		if (ps->avail_start >= kseg0 &&
		    (ps->avail_start + npgs) < (kseg0 + atop(0x20000000)) &&
		    ps->avail_end - ps->avail_start >= npgs) {
			pa = ptoa(ps->avail_start);
			break;
		}
	}

	if (pa == 0)
		panic("pmap_steal_memory: no approriate memory to steal!");

	ps->avail_start += npgs;
	ps->start += npgs;

	/*
	 * If we've used up all the pages in the segment, remove it and
	 * compact the list.
	 */
	if (ps->avail_start == ps->end) {
		/*
		 * If this was the last one, then a very bad thing has occurred
		 */
		if (--vm_nphysseg == 0)
			panic("pmap_steal_memory: out of memory!");

		printf("pmap_steal_memory: consumed bank %d\n", bank);
		for (; bank < vm_nphysseg; bank++, ps++) {
			ps[0] = ps[1];
		}
	}

	va = (vaddr_t)(SH5_KSEG0_BASE + (pa - pmap_kseg0_pa));
	memset((caddr_t) va, 0, size);

	return (va);
}

#ifdef DDB
void dump_kipt(void);
void
dump_kipt(void)
{
	vaddr_t va;
	kpte_t *pt;

	printf("\nKernel KSEG1 mappings:\n\n");

	for (pt = &pmap_kernel_ipt[0], va = SH5_KSEG1_BASE;
	    pt != &pmap_kernel_ipt[KERNEL_IPT_SIZE]; pt++, va += PAGE_SIZE) {
		if (pt->ptel && pt->ptel < 0x80000000u)
			printf("KVA: 0x%lx -> PTEL: 0x%lx\n", va,
			    (u_long)pt->ptel);
	}

	printf("\n");
}
#endif

#if defined(PMAP_DIAG) && defined(DDB)
int
validate_kipt(int cookie)
{
	kpte_t *kpte;
	ptel_t pt;
	vaddr_t va;
	paddr_t pa;
	int errors = 0;

	for (kpte = &pmap_kernel_ipt[0], va = SH5_KSEG1_BASE;
	    kpte != &pmap_kernel_ipt[KERNEL_IPT_SIZE];
	    kpte++, va += PAGE_SIZE) {
		if ((pt = kpte->ptel) == 0)
			continue;

		pa = (paddr_t)(pt & SH5_PTEL_PPN_MASK);

		if (pt & 0x24) {
			printf("kva 0x%lx has reserved bits set: 0x%lx\n",
			    va, (u_long)pt);
			errors++;
		}

		switch (pt & SH5_PTEL_CB_MASK) {
		case SH5_PTEL_CB_NOCACHE:
		case SH5_PTEL_CB_WRITEBACK:
		case SH5_PTEL_CB_WRITETHRU:
			if (pa < 0x80000000ul || pa >= 0x88000000) {
				printf("kva 0x%lx has bad DRAM pa: 0x%lx\n",
				    va, (u_long)pt);
				errors++;
			}
			break;

		case SH5_PTEL_CB_DEVICE:
			if (pa >= 0x80000000) {
				printf("kva 0x%lx has bad device pa: 0x%lx\n",
				    va, (u_long)pt);
				errors++;
			}
			break;
		}

		if ((pt & SH5_PTEL_SZ_MASK) != SH5_PTEL_SZ_4KB) {
			printf("kva 0x%lx has bad page size: 0x%lx\n",
			    va, (u_long)pt);
			errors++;
		}

		if (pt & SH5_PTEL_PR_U) {
			printf("kva 0x%lx has usermode bit set: 0x%lx\n",
			    va, (u_long)pt);
			errors++;
		}

		switch (pt & (SH5_PTEL_PR_R|SH5_PTEL_PR_X|SH5_PTEL_PR_W)) {
		case SH5_PTEL_PR_R:
		case SH5_PTEL_PR_R|SH5_PTEL_PR_W:
		case SH5_PTEL_PR_R|SH5_PTEL_PR_W|SH5_PTEL_PR_X:
			break;

		case SH5_PTEL_PR_W:
			printf("kva 0x%lx has only write bit set: 0x%lx\n",
			    va, (u_long)pt);
			errors++;
			break;

		case SH5_PTEL_PR_X:
			printf("kva 0x%lx has only execute bit set: 0x%lx\n",
			    va, (u_long)pt);
			errors++;
			break;

		case SH5_PTEL_PR_W|SH5_PTEL_PR_X:
			printf("kva 0x%lx is not readable: 0x%lx\n",
			    va, (u_long)pt);
			errors++;
			break;

		default:
			printf("kva 0x%lx has no protection bits set: 0x%lx\n",
			    va, (u_long)pt);
			errors++;
			break;
		}
	}

	if (errors)
		printf("Reference cookie: 0x%x\n", cookie);
	return (errors);
}
#endif
