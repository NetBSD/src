/*	$NetBSD: pmap.c,v 1.181.2.1 2014/08/10 06:54:11 tls Exp $	*/

/*-
 * Copyright (c) 2008, 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

/*
 * Copyright (c) 2007 Manuel Bouyer.
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
 *
 */

/*
 * Copyright (c) 2006 Mathieu Ropert <mro@adviseo.fr>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
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
 * Copyright 2001 (c) Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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

/*
 * This is the i386 pmap modified and generalized to support x86-64
 * as well. The idea is to hide the upper N levels of the page tables
 * inside pmap_get_ptp, pmap_free_ptp and pmap_growkernel. The rest
 * is mostly untouched, except that it uses some more generalized
 * macros and interfaces.
 *
 * This pmap has been tested on the i386 as well, and it can be easily
 * adapted to PAE.
 *
 * fvdl@wasabisystems.com 18-Jun-2001
 */

/*
 * pmap.c: i386 pmap module rewrite
 * Chuck Cranor <chuck@netbsd>
 * 11-Aug-97
 *
 * history of this pmap module: in addition to my own input, i used
 *    the following references for this rewrite of the i386 pmap:
 *
 * [1] the NetBSD i386 pmap.   this pmap appears to be based on the
 *     BSD hp300 pmap done by Mike Hibler at University of Utah.
 *     it was then ported to the i386 by William Jolitz of UUNET
 *     Technologies, Inc.   Then Charles M. Hannum of the NetBSD
 *     project fixed some bugs and provided some speed ups.
 *
 * [2] the FreeBSD i386 pmap.   this pmap seems to be the
 *     Hibler/Jolitz pmap, as modified for FreeBSD by John S. Dyson
 *     and David Greenman.
 *
 * [3] the Mach pmap.   this pmap, from CMU, seems to have migrated
 *     between several processors.   the VAX version was done by
 *     Avadis Tevanian, Jr., and Michael Wayne Young.    the i386
 *     version was done by Lance Berc, Mike Kupfer, Bob Baron,
 *     David Golub, and Richard Draves.    the alpha version was
 *     done by Alessandro Forin (CMU/Mach) and Chris Demetriou
 *     (NetBSD/alpha).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.181.2.1 2014/08/10 06:54:11 tls Exp $");

#include "opt_user_ldt.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"
#include "opt_xen.h"
#if !defined(__x86_64__)
#include "opt_kstack_dr0.h"
#endif /* !defined(__x86_64__) */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/pool.h>
#include <sys/kernel.h>
#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/intr.h>
#include <sys/xcall.h>
#include <sys/kcore.h>

#include <uvm/uvm.h>

#include <dev/isa/isareg.h>

#include <machine/specialreg.h>
#include <machine/gdt.h>
#include <machine/isa_machdep.h>
#include <machine/cpuvar.h>
#include <machine/cputypes.h>

#include <x86/pmap.h>
#include <x86/pmap_pv.h>

#include <x86/i82489reg.h>
#include <x86/i82489var.h>

#ifdef XEN
#include <xen/xen-public/xen.h>
#include <xen/hypervisor.h>
#endif

/*
 * general info:
 *
 *  - for an explanation of how the i386 MMU hardware works see
 *    the comments in <machine/pte.h>.
 *
 *  - for an explanation of the general memory structure used by
 *    this pmap (including the recursive mapping), see the comments
 *    in <machine/pmap.h>.
 *
 * this file contains the code for the "pmap module."   the module's
 * job is to manage the hardware's virtual to physical address mappings.
 * note that there are two levels of mapping in the VM system:
 *
 *  [1] the upper layer of the VM system uses vm_map's and vm_map_entry's
 *      to map ranges of virtual address space to objects/files.  for
 *      example, the vm_map may say: "map VA 0x1000 to 0x22000 read-only
 *      to the file /bin/ls starting at offset zero."   note that
 *      the upper layer mapping is not concerned with how individual
 *      vm_pages are mapped.
 *
 *  [2] the lower layer of the VM system (the pmap) maintains the mappings
 *      from virtual addresses.   it is concerned with which vm_page is
 *      mapped where.   for example, when you run /bin/ls and start
 *      at page 0x1000 the fault routine may lookup the correct page
 *      of the /bin/ls file and then ask the pmap layer to establish
 *      a mapping for it.
 *
 * note that information in the lower layer of the VM system can be
 * thrown away since it can easily be reconstructed from the info
 * in the upper layer.
 *
 * data structures we use include:
 *
 *  - struct pmap: describes the address space of one thread
 *  - struct pv_entry: describes one <PMAP,VA> mapping of a PA
 *  - struct pv_head: there is one pv_head per managed page of
 *	physical memory.   the pv_head points to a list of pv_entry
 *	structures which describe all the <PMAP,VA> pairs that this
 *      page is mapped in.    this is critical for page based operations
 *      such as pmap_page_protect() [change protection on _all_ mappings
 *      of a page]
 */

/*
 * memory allocation
 *
 *  - there are three data structures that we must dynamically allocate:
 *
 * [A] new process' page directory page (PDP)
 *	- plan 1: done at pmap_create() we use
 *	  uvm_km_alloc(kernel_map, PAGE_SIZE)  [fka kmem_alloc] to do this
 *	  allocation.
 *
 * if we are low in free physical memory then we sleep in
 * uvm_km_alloc -- in this case this is ok since we are creating
 * a new pmap and should not be holding any locks.
 *
 * if the kernel is totally out of virtual space
 * (i.e. uvm_km_alloc returns NULL), then we panic.
 *
 * [B] new page tables pages (PTP)
 * 	- call uvm_pagealloc()
 * 		=> success: zero page, add to pm_pdir
 * 		=> failure: we are out of free vm_pages, let pmap_enter()
 *		   tell UVM about it.
 *
 * note: for kernel PTPs, we start with NKPTP of them.   as we map
 * kernel memory (at uvm_map time) we check to see if we've grown
 * the kernel pmap.   if so, we call the optional function
 * pmap_growkernel() to grow the kernel PTPs in advance.
 *
 * [C] pv_entry structures
 */

/*
 * locking
 *
 * we have the following locks that we must contend with:
 *
 * mutexes:
 *
 * - pmap lock (per pmap, part of uvm_object)
 *   this lock protects the fields in the pmap structure including
 *   the non-kernel PDEs in the PDP, and the PTEs.  it also locks
 *   in the alternate PTE space (since that is determined by the
 *   entry in the PDP).
 *
 * - pvh_lock (per pv_head)
 *   this lock protects the pv_entry list which is chained off the
 *   pv_head structure for a specific managed PA.   it is locked
 *   when traversing the list (e.g. adding/removing mappings,
 *   syncing R/M bits, etc.)
 *
 * - pmaps_lock
 *   this lock protects the list of active pmaps (headed by "pmaps").
 *   we lock it when adding or removing pmaps from this list.
 */

const vaddr_t ptp_masks[] = PTP_MASK_INITIALIZER;
const int ptp_shifts[] = PTP_SHIFT_INITIALIZER;
const long nkptpmax[] = NKPTPMAX_INITIALIZER;
const long nbpd[] = NBPD_INITIALIZER;
pd_entry_t * const normal_pdes[] = PDES_INITIALIZER;

long nkptp[] = NKPTP_INITIALIZER;

struct pmap_head pmaps;
kmutex_t pmaps_lock;

static vaddr_t pmap_maxkvaddr;

/*
 * XXX kludge: dummy locking to make KASSERTs in uvm_page.c comfortable.
 * actual locking is done by pm_lock.
 */
#if defined(DIAGNOSTIC)
#define	PMAP_SUBOBJ_LOCK(pm, idx) \
	KASSERT(mutex_owned((pm)->pm_lock)); \
	if ((idx) != 0) \
		mutex_enter((pm)->pm_obj[(idx)].vmobjlock)
#define	PMAP_SUBOBJ_UNLOCK(pm, idx) \
	KASSERT(mutex_owned((pm)->pm_lock)); \
	if ((idx) != 0) \
		mutex_exit((pm)->pm_obj[(idx)].vmobjlock)
#else /* defined(DIAGNOSTIC) */
#define	PMAP_SUBOBJ_LOCK(pm, idx)	/* nothing */
#define	PMAP_SUBOBJ_UNLOCK(pm, idx)	/* nothing */
#endif /* defined(DIAGNOSTIC) */

/*
 * Misc. event counters.
 */
struct evcnt pmap_iobmp_evcnt;
struct evcnt pmap_ldt_evcnt;

/*
 * PAT
 */
#define	PATENTRY(n, type)	(type << ((n) * 8))
#define	PAT_UC		0x0ULL
#define	PAT_WC		0x1ULL
#define	PAT_WT		0x4ULL
#define	PAT_WP		0x5ULL
#define	PAT_WB		0x6ULL
#define	PAT_UCMINUS	0x7ULL

static bool cpu_pat_enabled __read_mostly = false;

/*
 * global data structures
 */

static struct pmap kernel_pmap_store;	/* the kernel's pmap (proc0) */
struct pmap *const kernel_pmap_ptr = &kernel_pmap_store;

/*
 * pmap_pg_g: if our processor supports PG_G in the PTE then we
 * set pmap_pg_g to PG_G (otherwise it is zero).
 */

int pmap_pg_g __read_mostly = 0;

/*
 * pmap_largepages: if our processor supports PG_PS and we are
 * using it, this is set to true.
 */

int pmap_largepages __read_mostly;

/*
 * i386 physical memory comes in a big contig chunk with a small
 * hole toward the front of it...  the following two paddr_t's
 * (shared with machdep.c) describe the physical address space
 * of this machine.
 */
paddr_t avail_start __read_mostly; /* PA of first available physical page */
paddr_t avail_end __read_mostly; /* PA of last available physical page */

#ifdef XEN
#ifdef __x86_64__
/* Dummy PGD for user cr3, used between pmap_deactivate() and pmap_activate() */
static paddr_t xen_dummy_user_pgd;
#endif /* __x86_64__ */
paddr_t pmap_pa_start; /* PA of first physical page for this domain */
paddr_t pmap_pa_end;   /* PA of last physical page for this domain */
#endif /* XEN */

#define	VM_PAGE_TO_PP(pg)	(&(pg)->mdpage.mp_pp)

#define	PV_HASH_SIZE		32768
#define	PV_HASH_LOCK_CNT	32

struct pv_hash_lock {
	kmutex_t lock;
} __aligned(CACHE_LINE_SIZE) pv_hash_locks[PV_HASH_LOCK_CNT]
    __aligned(CACHE_LINE_SIZE);

struct pv_hash_head {
	SLIST_HEAD(, pv_entry) hh_list;
} pv_hash_heads[PV_HASH_SIZE];

static u_int
pvhash_hash(struct vm_page *ptp, vaddr_t va)
{

	return (uintptr_t)ptp / sizeof(*ptp) + (va >> PAGE_SHIFT);
}

static struct pv_hash_head *
pvhash_head(u_int hash)
{

	return &pv_hash_heads[hash % PV_HASH_SIZE];
}

static kmutex_t *
pvhash_lock(u_int hash)
{

	return &pv_hash_locks[hash % PV_HASH_LOCK_CNT].lock;
}

static struct pv_entry *
pvhash_remove(struct pv_hash_head *hh, struct vm_page *ptp, vaddr_t va)
{
	struct pv_entry *pve;
	struct pv_entry *prev;

	prev = NULL;
	SLIST_FOREACH(pve, &hh->hh_list, pve_hash) {
		if (pve->pve_pte.pte_ptp == ptp &&
		    pve->pve_pte.pte_va == va) {
			if (prev != NULL) {
				SLIST_REMOVE_AFTER(prev, pve_hash);
			} else {
				SLIST_REMOVE_HEAD(&hh->hh_list, pve_hash);
			}
			break;
		}
		prev = pve;
	}
	return pve;
}

/*
 * other data structures
 */

static pt_entry_t protection_codes[8] __read_mostly; /* maps MI prot to i386
							prot code */
static bool pmap_initialized __read_mostly = false; /* pmap_init done yet? */

/*
 * the following two vaddr_t's are used during system startup
 * to keep track of how much of the kernel's VM space we have used.
 * once the system is started, the management of the remaining kernel
 * VM space is turned over to the kernel_map vm_map.
 */

static vaddr_t virtual_avail __read_mostly;	/* VA of first free KVA */
static vaddr_t virtual_end __read_mostly;	/* VA of last free KVA */

/*
 * pool that pmap structures are allocated from
 */

static struct pool_cache pmap_cache;

/*
 * pv_entry cache
 */

static struct pool_cache pmap_pv_cache;

#ifdef __HAVE_DIRECT_MAP

extern phys_ram_seg_t mem_clusters[];
extern int mem_cluster_cnt;

#else

/*
 * MULTIPROCESSOR: special VA's/ PTE's are actually allocated inside a
 * maxcpus*NPTECL array of PTE's, to avoid cache line thrashing
 * due to false sharing.
 */

#ifdef MULTIPROCESSOR
#define PTESLEW(pte, id) ((pte)+(id)*NPTECL)
#define VASLEW(va,id) ((va)+(id)*NPTECL*PAGE_SIZE)
#else
#define PTESLEW(pte, id) ((void)id, pte)
#define VASLEW(va,id) ((void)id, va)
#endif

/*
 * special VAs and the PTEs that map them
 */
static pt_entry_t *csrc_pte, *cdst_pte, *zero_pte, *ptp_pte, *early_zero_pte;
static char *csrcp, *cdstp, *zerop, *ptpp;
#ifdef XEN
char *early_zerop; /* also referenced from xen_pmap_bootstrap() */
#else
static char *early_zerop;
#endif

#endif

int pmap_enter_default(pmap_t, vaddr_t, paddr_t, vm_prot_t, u_int);

/* PDP pool_cache(9) and its callbacks */
struct pool_cache pmap_pdp_cache;
static int  pmap_pdp_ctor(void *, void *, int);
static void pmap_pdp_dtor(void *, void *);
#ifdef PAE
/* need to allocate items of 4 pages */
static void *pmap_pdp_alloc(struct pool *, int);
static void pmap_pdp_free(struct pool *, void *);
static struct pool_allocator pmap_pdp_allocator = {
	.pa_alloc = pmap_pdp_alloc,
	.pa_free = pmap_pdp_free,
	.pa_pagesz = PAGE_SIZE * PDP_SIZE,
};
#endif /* PAE */

extern vaddr_t idt_vaddr;			/* we allocate IDT early */
extern paddr_t idt_paddr;

#ifdef _LP64
extern vaddr_t lo32_vaddr;
extern vaddr_t lo32_paddr;
#endif

extern int end;

#ifdef i386
/* stuff to fix the pentium f00f bug */
extern vaddr_t pentium_idt_vaddr;
#endif


/*
 * local prototypes
 */

static struct vm_page	*pmap_get_ptp(struct pmap *, vaddr_t,
				      pd_entry_t * const *);
static struct vm_page	*pmap_find_ptp(struct pmap *, vaddr_t, paddr_t, int);
static void		 pmap_freepage(struct pmap *, struct vm_page *, int);
static void		 pmap_free_ptp(struct pmap *, struct vm_page *,
				       vaddr_t, pt_entry_t *,
				       pd_entry_t * const *);
static bool		 pmap_remove_pte(struct pmap *, struct vm_page *,
					 pt_entry_t *, vaddr_t,
					 struct pv_entry **);
static void		 pmap_remove_ptes(struct pmap *, struct vm_page *,
					  vaddr_t, vaddr_t, vaddr_t,
					  struct pv_entry **);

static bool		 pmap_get_physpage(vaddr_t, int, paddr_t *);
static void		 pmap_alloc_level(pd_entry_t * const *, vaddr_t, int,
					  long *);

static bool		 pmap_reactivate(struct pmap *);

/*
 * p m a p   h e l p e r   f u n c t i o n s
 */

static inline void
pmap_stats_update(struct pmap *pmap, int resid_diff, int wired_diff)
{

	if (pmap == pmap_kernel()) {
		atomic_add_long(&pmap->pm_stats.resident_count, resid_diff);
		atomic_add_long(&pmap->pm_stats.wired_count, wired_diff);
	} else {
		KASSERT(mutex_owned(pmap->pm_lock));
		pmap->pm_stats.resident_count += resid_diff;
		pmap->pm_stats.wired_count += wired_diff;
	}
}

static inline void
pmap_stats_update_bypte(struct pmap *pmap, pt_entry_t npte, pt_entry_t opte)
{
	int resid_diff = ((npte & PG_V) ? 1 : 0) - ((opte & PG_V) ? 1 : 0);
	int wired_diff = ((npte & PG_W) ? 1 : 0) - ((opte & PG_W) ? 1 : 0);

	KASSERT((npte & (PG_V | PG_W)) != PG_W);
	KASSERT((opte & (PG_V | PG_W)) != PG_W);

	pmap_stats_update(pmap, resid_diff, wired_diff);
}

/*
 * ptp_to_pmap: lookup pmap by ptp
 */

static struct pmap *
ptp_to_pmap(struct vm_page *ptp)
{
	struct pmap *pmap;

	if (ptp == NULL) {
		return pmap_kernel();
	}
	pmap = (struct pmap *)ptp->uobject;
	KASSERT(pmap != NULL);
	KASSERT(&pmap->pm_obj[0] == ptp->uobject);
	return pmap;
}

static inline struct pv_pte *
pve_to_pvpte(struct pv_entry *pve)
{

	KASSERT((void *)&pve->pve_pte == (void *)pve);
	return &pve->pve_pte;
}

static inline struct pv_entry *
pvpte_to_pve(struct pv_pte *pvpte)
{
	struct pv_entry *pve = (void *)pvpte;

	KASSERT(pve_to_pvpte(pve) == pvpte);
	return pve;
}

/*
 * pv_pte_first, pv_pte_next: PV list iterator.
 */

static struct pv_pte *
pv_pte_first(struct pmap_page *pp)
{

	if ((pp->pp_flags & PP_EMBEDDED) != 0) {
		return &pp->pp_pte;
	}
	return pve_to_pvpte(LIST_FIRST(&pp->pp_head.pvh_list));
}

static struct pv_pte *
pv_pte_next(struct pmap_page *pp, struct pv_pte *pvpte)
{

	KASSERT(pvpte != NULL);
	if (pvpte == &pp->pp_pte) {
		KASSERT((pp->pp_flags & PP_EMBEDDED) != 0);
		return NULL;
	}
	KASSERT((pp->pp_flags & PP_EMBEDDED) == 0);
	return pve_to_pvpte(LIST_NEXT(pvpte_to_pve(pvpte), pve_list));
}

/*
 * pmap_is_curpmap: is this pmap the one currently loaded [in %cr3]?
 *		of course the kernel is always loaded
 */

bool
pmap_is_curpmap(struct pmap *pmap)
{
	return((pmap == pmap_kernel()) ||
	       (pmap == curcpu()->ci_pmap));
}

/*
 *	Add a reference to the specified pmap.
 */

void
pmap_reference(struct pmap *pmap)
{

	atomic_inc_uint(&pmap->pm_obj[0].uo_refs);
}

/*
 * pmap_map_ptes: map a pmap's PTEs into KVM and lock them in
 *
 * there are several pmaps involved.  some or all of them might be same.
 *
 *	- the pmap given by the first argument
 *		our caller wants to access this pmap's PTEs.
 *
 *	- pmap_kernel()
 *		the kernel pmap.  note that it only contains the kernel part
 *		of the address space which is shared by any pmap.  ie. any
 *		pmap can be used instead of pmap_kernel() for our purpose.
 *
 *	- ci->ci_pmap
 *		pmap currently loaded on the cpu.
 *
 *	- vm_map_pmap(&curproc->p_vmspace->vm_map)
 *		current process' pmap.
 *
 * => we lock enough pmaps to keep things locked in
 * => must be undone with pmap_unmap_ptes before returning
 */

void
pmap_map_ptes(struct pmap *pmap, struct pmap **pmap2,
	      pd_entry_t **ptepp, pd_entry_t * const **pdeppp)
{
	struct pmap *curpmap;
	struct cpu_info *ci;
	lwp_t *l;

	/* The kernel's pmap is always accessible. */
	if (pmap == pmap_kernel()) {
		*pmap2 = NULL;
		*ptepp = PTE_BASE;
		*pdeppp = normal_pdes;
		return;
	}
	KASSERT(kpreempt_disabled());

	l = curlwp;
 retry:
	mutex_enter(pmap->pm_lock);
	ci = curcpu();
	curpmap = ci->ci_pmap;
	if (vm_map_pmap(&l->l_proc->p_vmspace->vm_map) == pmap) {
		/* Our own pmap so just load it: easy. */
		if (__predict_false(ci->ci_want_pmapload)) {
			mutex_exit(pmap->pm_lock);
			pmap_load();
			goto retry;
		}
		KASSERT(pmap == curpmap);
	} else if (pmap == curpmap) {
		/*
		 * Already on the CPU: make it valid.  This is very
		 * often the case during exit(), when we have switched
		 * to the kernel pmap in order to destroy a user pmap.
		 */
		if (!pmap_reactivate(pmap)) {
			u_int gen = uvm_emap_gen_return();
			tlbflush();
			uvm_emap_update(gen);
		}
	} else {
		/*
		 * Toss current pmap from CPU, but keep a reference to it.
		 * The reference will be dropped by pmap_unmap_ptes().
		 * Can happen if we block during exit().
		 */
		const cpuid_t cid = cpu_index(ci);

		kcpuset_atomic_clear(curpmap->pm_cpus, cid);
		kcpuset_atomic_clear(curpmap->pm_kernel_cpus, cid);
		ci->ci_pmap = pmap;
		ci->ci_tlbstate = TLBSTATE_VALID;
		kcpuset_atomic_set(pmap->pm_cpus, cid);
		kcpuset_atomic_set(pmap->pm_kernel_cpus, cid);
		cpu_load_pmap(pmap, curpmap);
	}
	pmap->pm_ncsw = l->l_ncsw;
	*pmap2 = curpmap;
	*ptepp = PTE_BASE;
#if defined(XEN) && defined(__x86_64__)
	KASSERT(ci->ci_normal_pdes[PTP_LEVELS - 2] == L4_BASE);
	ci->ci_normal_pdes[PTP_LEVELS - 2] = pmap->pm_pdir;
	*pdeppp = ci->ci_normal_pdes;
#else /* XEN && __x86_64__ */
	*pdeppp = normal_pdes;
#endif /* XEN && __x86_64__ */
}

/*
 * pmap_unmap_ptes: unlock the PTE mapping of "pmap"
 */

void
pmap_unmap_ptes(struct pmap *pmap, struct pmap *pmap2)
{
	struct cpu_info *ci;
	struct pmap *mypmap;

	KASSERT(kpreempt_disabled());

	/* The kernel's pmap is always accessible. */
	if (pmap == pmap_kernel()) {
		return;
	}

	ci = curcpu();
#if defined(XEN) && defined(__x86_64__)
	/* Reset per-cpu normal_pdes */
	KASSERT(ci->ci_normal_pdes[PTP_LEVELS - 2] != L4_BASE);
	ci->ci_normal_pdes[PTP_LEVELS - 2] = L4_BASE;
#endif /* XEN && __x86_64__ */
	/*
	 * We cannot tolerate context switches while mapped in.
	 * If it is our own pmap all we have to do is unlock.
	 */
	KASSERT(pmap->pm_ncsw == curlwp->l_ncsw);
	mypmap = vm_map_pmap(&curproc->p_vmspace->vm_map);
	if (pmap == mypmap) {
		mutex_exit(pmap->pm_lock);
		return;
	}

	/*
	 * Mark whatever's on the CPU now as lazy and unlock.
	 * If the pmap was already installed, we are done.
	 */
	ci->ci_tlbstate = TLBSTATE_LAZY;
	ci->ci_want_pmapload = (mypmap != pmap_kernel());
	mutex_exit(pmap->pm_lock);
	if (pmap == pmap2) {
		return;
	}

	/*
	 * We installed another pmap on the CPU.  Grab a reference to
	 * it and leave in place.  Toss the evicted pmap (can block).
	 */
	pmap_reference(pmap);
	pmap_destroy(pmap2);
}


inline static void
pmap_exec_account(struct pmap *pm, vaddr_t va, pt_entry_t opte, pt_entry_t npte)
{

#if !defined(__x86_64__)
	if (curproc == NULL || curproc->p_vmspace == NULL ||
	    pm != vm_map_pmap(&curproc->p_vmspace->vm_map))
		return;

	if ((opte ^ npte) & PG_X)
		pmap_update_pg(va);

	/*
	 * Executability was removed on the last executable change.
	 * Reset the code segment to something conservative and
	 * let the trap handler deal with setting the right limit.
	 * We can't do that because of locking constraints on the vm map.
	 */

	if ((opte & PG_X) && (npte & PG_X) == 0 && va == pm->pm_hiexec) {
		struct trapframe *tf = curlwp->l_md.md_regs;

		tf->tf_cs = GSEL(GUCODE_SEL, SEL_UPL);
		pm->pm_hiexec = I386_MAX_EXE_ADDR;
	}
#endif /* !defined(__x86_64__) */
}

#if !defined(__x86_64__)
/*
 * Fixup the code segment to cover all potential executable mappings.
 * returns 0 if no changes to the code segment were made.
 */

int
pmap_exec_fixup(struct vm_map *map, struct trapframe *tf, struct pcb *pcb)
{
	struct vm_map_entry *ent;
	struct pmap *pm = vm_map_pmap(map);
	vaddr_t va = 0;

	vm_map_lock_read(map);
	for (ent = (&map->header)->next; ent != &map->header; ent = ent->next) {

		/*
		 * This entry has greater va than the entries before.
		 * We need to make it point to the last page, not past it.
		 */

		if (ent->protection & VM_PROT_EXECUTE)
			va = trunc_page(ent->end) - PAGE_SIZE;
	}
	vm_map_unlock_read(map);
	if (va == pm->pm_hiexec && tf->tf_cs == GSEL(GUCODEBIG_SEL, SEL_UPL))
		return (0);

	pm->pm_hiexec = va;
	if (pm->pm_hiexec > I386_MAX_EXE_ADDR) {
		tf->tf_cs = GSEL(GUCODEBIG_SEL, SEL_UPL);
	} else {
		tf->tf_cs = GSEL(GUCODE_SEL, SEL_UPL);
		return (0);
	}
	return (1);
}
#endif /* !defined(__x86_64__) */

void
pat_init(struct cpu_info *ci)
{
	uint64_t pat;

	if (!(ci->ci_feat_val[0] & CPUID_PAT))
		return;

	/* We change WT to WC. Leave all other entries the default values. */
	pat = PATENTRY(0, PAT_WB) | PATENTRY(1, PAT_WC) |
	      PATENTRY(2, PAT_UCMINUS) | PATENTRY(3, PAT_UC) |
	      PATENTRY(4, PAT_WB) | PATENTRY(5, PAT_WC) |
	      PATENTRY(6, PAT_UCMINUS) | PATENTRY(7, PAT_UC);

	wrmsr(MSR_CR_PAT, pat);
	cpu_pat_enabled = true;
	aprint_debug_dev(ci->ci_dev, "PAT enabled\n");
}

static pt_entry_t
pmap_pat_flags(u_int flags)
{
	u_int cacheflags = (flags & PMAP_CACHE_MASK);

	if (!cpu_pat_enabled) {
		switch (cacheflags) {
		case PMAP_NOCACHE:
		case PMAP_NOCACHE_OVR:
			/* results in PGC_UCMINUS on cpus which have
			 * the cpuid PAT but PAT "disabled"
			 */
			return PG_N;
		default:
			return 0;
		}
	}

	switch (cacheflags) {
	case PMAP_NOCACHE:
		return PGC_UC;
	case PMAP_WRITE_COMBINE:
		return PGC_WC;
	case PMAP_WRITE_BACK:
		return PGC_WB;
	case PMAP_NOCACHE_OVR:
		return PGC_UCMINUS;
	}

	return 0;
}

/*
 * p m a p   k e n t e r   f u n c t i o n s
 *
 * functions to quickly enter/remove pages from the kernel address
 * space.   pmap_kremove is exported to MI kernel.  we make use of
 * the recursive PTE mappings.
 */

/*
 * pmap_kenter_pa: enter a kernel mapping without R/M (pv_entry) tracking
 *
 * => no need to lock anything, assume va is already allocated
 * => should be faster than normal pmap enter function
 */

void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	pt_entry_t *pte, opte, npte;

	KASSERT(!(prot & ~VM_PROT_ALL));

	if (va < VM_MIN_KERNEL_ADDRESS)
		pte = vtopte(va);
	else
		pte = kvtopte(va);
#ifdef DOM0OPS
	if (pa < pmap_pa_start || pa >= pmap_pa_end) {
#ifdef DEBUG
		printf_nolog("%s: pa 0x%" PRIx64 " for va 0x%" PRIx64
		    " outside range\n", __func__, (int64_t)pa, (int64_t)va);
#endif /* DEBUG */
		npte = pa;
	} else
#endif /* DOM0OPS */
		npte = pmap_pa2pte(pa);
	npte |= protection_codes[prot] | PG_k | PG_V | pmap_pg_g;
	npte |= pmap_pat_flags(flags);
	opte = pmap_pte_testset(pte, npte); /* zap! */
#if defined(DIAGNOSTIC)
	/* XXX For now... */
	if (opte & PG_PS)
		panic("%s: PG_PS", __func__);
#endif
	if ((opte & (PG_V | PG_U)) == (PG_V | PG_U)) {
		/* This should not happen. */
		printf_nolog("%s: mapping already present\n", __func__);
		kpreempt_disable();
		pmap_tlb_shootdown(pmap_kernel(), va, opte, TLBSHOOT_KENTER);
		kpreempt_enable();
	}
}

void
pmap_emap_enter(vaddr_t va, paddr_t pa, vm_prot_t prot)
{
	pt_entry_t *pte, npte;

	KASSERT((prot & ~VM_PROT_ALL) == 0);
	pte = (va < VM_MIN_KERNEL_ADDRESS) ? vtopte(va) : kvtopte(va);

#ifdef DOM0OPS
	if (pa < pmap_pa_start || pa >= pmap_pa_end) {
		npte = pa;
	} else
#endif
		npte = pmap_pa2pte(pa);

	npte = pmap_pa2pte(pa);
	npte |= protection_codes[prot] | PG_k | PG_V;
	pmap_pte_set(pte, npte);
}

/*
 * pmap_emap_sync: perform TLB flush or pmap load, if it was deferred.
 */
void
pmap_emap_sync(bool canload)
{
	struct cpu_info *ci = curcpu();
	struct pmap *pmap;

	KASSERT(kpreempt_disabled());
	if (__predict_true(ci->ci_want_pmapload && canload)) {
		/*
		 * XXX: Hint for pmap_reactivate(), which might suggest to
		 * not perform TLB flush, if state has not changed.
		 */
		pmap = vm_map_pmap(&curlwp->l_proc->p_vmspace->vm_map);
		if (__predict_false(pmap == ci->ci_pmap)) {
			kcpuset_atomic_clear(pmap->pm_cpus, cpu_index(ci));
		}
		pmap_load();
		KASSERT(ci->ci_want_pmapload == 0);
	} else {
		tlbflush();
	}

}

void
pmap_emap_remove(vaddr_t sva, vsize_t len)
{
	pt_entry_t *pte;
	vaddr_t va, eva = sva + len;

	for (va = sva; va < eva; va += PAGE_SIZE) {
		pte = (va < VM_MIN_KERNEL_ADDRESS) ? vtopte(va) : kvtopte(va);
		pmap_pte_set(pte, 0);
	}
}

__strict_weak_alias(pmap_kenter_ma, pmap_kenter_pa);

#if defined(__x86_64__)
/*
 * Change protection for a virtual address. Local for a CPU only, don't
 * care about TLB shootdowns.
 *
 * => must be called with preemption disabled
 */
void
pmap_changeprot_local(vaddr_t va, vm_prot_t prot)
{
	pt_entry_t *pte, opte, npte;

	KASSERT(kpreempt_disabled());

	if (va < VM_MIN_KERNEL_ADDRESS)
		pte = vtopte(va);
	else
		pte = kvtopte(va);

	npte = opte = *pte;

	if ((prot & VM_PROT_WRITE) != 0)
		npte |= PG_RW;
	else
		npte &= ~PG_RW;

	if (opte != npte) {
		pmap_pte_set(pte, npte);
		pmap_pte_flush();
		invlpg(va);
	}
}
#endif /* defined(__x86_64__) */

/*
 * pmap_kremove: remove a kernel mapping(s) without R/M (pv_entry) tracking
 *
 * => no need to lock anything
 * => caller must dispose of any vm_page mapped in the va range
 * => note: not an inline function
 * => we assume the va is page aligned and the len is a multiple of PAGE_SIZE
 * => we assume kernel only unmaps valid addresses and thus don't bother
 *    checking the valid bit before doing TLB flushing
 * => must be followed by call to pmap_update() before reuse of page
 */

static inline void
pmap_kremove1(vaddr_t sva, vsize_t len, bool localonly)
{
	pt_entry_t *pte, opte;
	vaddr_t va, eva;

	eva = sva + len;

	kpreempt_disable();
	for (va = sva; va < eva; va += PAGE_SIZE) {
		pte = kvtopte(va);
		opte = pmap_pte_testset(pte, 0); /* zap! */
		if ((opte & (PG_V | PG_U)) == (PG_V | PG_U) && !localonly) {
			pmap_tlb_shootdown(pmap_kernel(), va, opte,
			    TLBSHOOT_KREMOVE);
		}
		KASSERT((opte & PG_PS) == 0);
		KASSERT((opte & PG_PVLIST) == 0);
	}
	if (localonly) {
		tlbflushg();
	}
	kpreempt_enable();
}

void
pmap_kremove(vaddr_t sva, vsize_t len)
{

	pmap_kremove1(sva, len, false);
}

/*
 * pmap_kremove_local: like pmap_kremove(), but only worry about
 * TLB invalidations on the current CPU.  this is only intended
 * for use while writing kernel crash dumps.
 */

void
pmap_kremove_local(vaddr_t sva, vsize_t len)
{

	KASSERT(panicstr != NULL);
	pmap_kremove1(sva, len, true);
}

/*
 * p m a p   i n i t   f u n c t i o n s
 *
 * pmap_bootstrap and pmap_init are called during system startup
 * to init the pmap module.   pmap_bootstrap() does a low level
 * init just to get things rolling.   pmap_init() finishes the job.
 */

/*
 * pmap_bootstrap: get the system in a state where it can run with VM
 *	properly enabled (called before main()).   the VM system is
 *      fully init'd later...
 *
 * => on i386, locore.s has already enabled the MMU by allocating
 *	a PDP for the kernel, and nkpde PTP's for the kernel.
 * => kva_start is the first free virtual address in kernel space
 */

void
pmap_bootstrap(vaddr_t kva_start)
{
	struct pmap *kpm;
	pt_entry_t *pte;
	int i;
	vaddr_t kva;
#ifndef XEN
	pd_entry_t *pde;
	unsigned long p1i;
	vaddr_t kva_end;
#endif
#ifdef __HAVE_DIRECT_MAP
	phys_ram_seg_t *mc;
	long ndmpdp;
	paddr_t lastpa, dmpd, dmpdp, pdp;
	vaddr_t tmpva;
#endif

	pt_entry_t pg_nx = (cpu_feature[2] & CPUID_NOX ? PG_NX : 0);

	/*
	 * set up our local static global vars that keep track of the
	 * usage of KVM before kernel_map is set up
	 */

	virtual_avail = kva_start;		/* first free KVA */
	virtual_end = VM_MAX_KERNEL_ADDRESS;	/* last KVA */

	/*
	 * set up protection_codes: we need to be able to convert from
	 * a MI protection code (some combo of VM_PROT...) to something
	 * we can jam into a i386 PTE.
	 */

	protection_codes[VM_PROT_NONE] = pg_nx;			/* --- */
	protection_codes[VM_PROT_EXECUTE] = PG_RO | PG_X;	/* --x */
	protection_codes[VM_PROT_READ] = PG_RO | pg_nx;		/* -r- */
	protection_codes[VM_PROT_READ|VM_PROT_EXECUTE] = PG_RO | PG_X;/* -rx */
	protection_codes[VM_PROT_WRITE] = PG_RW | pg_nx;	/* w-- */
	protection_codes[VM_PROT_WRITE|VM_PROT_EXECUTE] = PG_RW | PG_X;/* w-x */
	protection_codes[VM_PROT_WRITE|VM_PROT_READ] = PG_RW | pg_nx;
								/* wr- */
	protection_codes[VM_PROT_ALL] = PG_RW | PG_X;		/* wrx */

	/*
	 * now we init the kernel's pmap
	 *
	 * the kernel pmap's pm_obj is not used for much.   however, in
	 * user pmaps the pm_obj contains the list of active PTPs.
	 * the pm_obj currently does not have a pager.   it might be possible
	 * to add a pager that would allow a process to read-only mmap its
	 * own page tables (fast user level vtophys?).   this may or may not
	 * be useful.
	 */

	kpm = pmap_kernel();
	for (i = 0; i < PTP_LEVELS - 1; i++) {
		mutex_init(&kpm->pm_obj_lock[i], MUTEX_DEFAULT, IPL_NONE);
		uvm_obj_init(&kpm->pm_obj[i], NULL, false, 1);
		uvm_obj_setlock(&kpm->pm_obj[i], &kpm->pm_obj_lock[i]);
		kpm->pm_ptphint[i] = NULL;
	}
	memset(&kpm->pm_list, 0, sizeof(kpm->pm_list));  /* pm_list not used */

	kpm->pm_pdir = (pd_entry_t *)(PDPpaddr + KERNBASE);
	for (i = 0; i < PDP_SIZE; i++)
		kpm->pm_pdirpa[i] = PDPpaddr + PAGE_SIZE * i;

	kpm->pm_stats.wired_count = kpm->pm_stats.resident_count =
		x86_btop(kva_start - VM_MIN_KERNEL_ADDRESS);

	kcpuset_create(&kpm->pm_cpus, true);
	kcpuset_create(&kpm->pm_kernel_cpus, true);

	/*
	 * the above is just a rough estimate and not critical to the proper
	 * operation of the system.
	 */

#ifndef XEN
	/*
	 * Begin to enable global TLB entries if they are supported.
	 * The G bit has no effect until the CR4_PGE bit is set in CR4,
	 * which happens in cpu_init(), which is run on each cpu
	 * (and happens later)
	 */

	if (cpu_feature[0] & CPUID_PGE) {
		pmap_pg_g = PG_G;		/* enable software */

		/* add PG_G attribute to already mapped kernel pages */
		if (KERNBASE == VM_MIN_KERNEL_ADDRESS) {
			kva_end = virtual_avail;
		} else {
			extern vaddr_t eblob, esym;
			kva_end = (vaddr_t)&end;
			if (esym > kva_end)
				kva_end = esym;
			if (eblob > kva_end)
				kva_end = eblob;
			kva_end = roundup(kva_end, PAGE_SIZE);
		}
		for (kva = KERNBASE; kva < kva_end; kva += PAGE_SIZE) {
			p1i = pl1_i(kva);
			if (pmap_valid_entry(PTE_BASE[p1i]))
				PTE_BASE[p1i] |= PG_G;
		}
	}

	/*
	 * enable large pages if they are supported.
	 */

	if (cpu_feature[0] & CPUID_PSE) {
		paddr_t pa;
		extern char __data_start;

		lcr4(rcr4() | CR4_PSE);	/* enable hardware (via %cr4) */
		pmap_largepages = 1;	/* enable software */

		/*
		 * the TLB must be flushed after enabling large pages
		 * on Pentium CPUs, according to section 3.6.2.2 of
		 * "Intel Architecture Software Developer's Manual,
		 * Volume 3: System Programming".
		 */
		tlbflushg();

		/*
		 * now, remap the kernel text using large pages.  we
		 * assume that the linker has properly aligned the
		 * .data segment to a NBPD_L2 boundary.
		 */
		kva_end = rounddown((vaddr_t)&__data_start, NBPD_L1);
		for (pa = 0, kva = KERNBASE; kva + NBPD_L2 <= kva_end;
		     kva += NBPD_L2, pa += NBPD_L2) {
			pde = &L2_BASE[pl2_i(kva)];
			*pde = pa | pmap_pg_g | PG_PS |
			    PG_KR | PG_V;	/* zap! */
			tlbflushg();
		}
#if defined(DEBUG)
		aprint_normal("kernel text is mapped with %" PRIuPSIZE " large "
		    "pages and %" PRIuPSIZE " normal pages\n",
		    howmany(kva - KERNBASE, NBPD_L2),
		    howmany((vaddr_t)&__data_start - kva, NBPD_L1));
#endif /* defined(DEBUG) */
	}
#endif /* !XEN */

#ifdef __HAVE_DIRECT_MAP

	tmpva = (KERNBASE + NKL2_KIMG_ENTRIES * NBPD_L2);
	pte = PTE_BASE + pl1_i(tmpva);

	/*
	 * Map the direct map.  Use 1GB pages if they are available,
	 * otherwise use 2MB pages.  Note that the unused parts of
	 * PTPs * must be zero outed, as they might be accessed due
	 * to speculative execution.  Also, PG_G is not allowed on
	 * non-leaf PTPs.
	 */

	lastpa = 0;
	for (i = 0; i < mem_cluster_cnt; i++) {
		mc = &mem_clusters[i];
		lastpa = MAX(lastpa, mc->start + mc->size);
	}

	ndmpdp = (lastpa + NBPD_L3 - 1) >> L3_SHIFT;
	dmpdp = avail_start;	avail_start += PAGE_SIZE;

	*pte = dmpdp | PG_V | PG_RW;
	pmap_update_pg(tmpva);
	memset((void *)tmpva, 0, PAGE_SIZE);

	if (cpu_feature[2] & CPUID_P1GB) {
		for (i = 0; i < ndmpdp; i++) {
			pdp = (paddr_t)&(((pd_entry_t *)dmpdp)[i]);
			*pte = (pdp & PG_FRAME) | PG_V | PG_RW;
			pmap_update_pg(tmpva);

			pde = (pd_entry_t *)(tmpva + (pdp & ~PG_FRAME));
			*pde = ((paddr_t)i << L3_SHIFT) |
				PG_RW | PG_V | PG_U | PG_PS | PG_G;
		}
	} else {
		dmpd = avail_start;	avail_start += ndmpdp * PAGE_SIZE;

		for (i = 0; i < ndmpdp; i++) {
			pdp = dmpd + i * PAGE_SIZE;
			*pte = (pdp & PG_FRAME) | PG_V | PG_RW;
			pmap_update_pg(tmpva);

			memset((void *)tmpva, 0, PAGE_SIZE);
		}
		for (i = 0; i < NPDPG * ndmpdp; i++) {
			pdp = (paddr_t)&(((pd_entry_t *)dmpd)[i]);
			*pte = (pdp & PG_FRAME) | PG_V | PG_RW;
			pmap_update_pg(tmpva);

			pde = (pd_entry_t *)(tmpva + (pdp & ~PG_FRAME));
			*pde = ((paddr_t)i << L2_SHIFT) |
				PG_RW | PG_V | PG_U | PG_PS | PG_G;
		}
		for (i = 0; i < ndmpdp; i++) {
			pdp = (paddr_t)&(((pd_entry_t *)dmpdp)[i]);
			*pte = (pdp & PG_FRAME) | PG_V | PG_RW;
			pmap_update_pg((vaddr_t)tmpva);

			pde = (pd_entry_t *)(tmpva + (pdp & ~PG_FRAME));
			*pde = (dmpd + (i << PAGE_SHIFT)) |
				PG_RW | PG_V | PG_U;
		}
	}

	kpm->pm_pdir[PDIR_SLOT_DIRECT] = dmpdp | PG_KW | PG_V | PG_U;

	tlbflush();

#else
	if (VM_MIN_KERNEL_ADDRESS != KERNBASE) {
		/*
		 * zero_pte is stuck at the end of mapped space for the kernel
		 * image (disjunct from kva space). This is done so that it
		 * can safely be used in pmap_growkernel (pmap_get_physpage),
		 * when it's called for the first time.
		 * XXXfvdl fix this for MULTIPROCESSOR later.
		 */
#ifdef XEN
		/* early_zerop initialized in xen_pmap_bootstrap() */
#else
		early_zerop = (void *)(KERNBASE + NKL2_KIMG_ENTRIES * NBPD_L2);
#endif
		early_zero_pte = PTE_BASE + pl1_i((vaddr_t)early_zerop);
	}

	/*
	 * now we allocate the "special" VAs which are used for tmp mappings
	 * by the pmap (and other modules).    we allocate the VAs by advancing
	 * virtual_avail (note that there are no pages mapped at these VAs).
	 * we find the PTE that maps the allocated VA via the linear PTE
	 * mapping.
	 */

	pte = PTE_BASE + pl1_i(virtual_avail);

#ifdef MULTIPROCESSOR
	/*
	 * Waste some VA space to avoid false sharing of cache lines
	 * for page table pages: Give each possible CPU a cache line
	 * of PTE's (8) to play with, though we only need 4.  We could
	 * recycle some of this waste by putting the idle stacks here
	 * as well; we could waste less space if we knew the largest
	 * CPU ID beforehand.
	 */
	csrcp = (char *) virtual_avail;  csrc_pte = pte;

	cdstp = (char *) virtual_avail+PAGE_SIZE;  cdst_pte = pte+1;

	zerop = (char *) virtual_avail+PAGE_SIZE*2;  zero_pte = pte+2;

	ptpp = (char *) virtual_avail+PAGE_SIZE*3;  ptp_pte = pte+3;

	virtual_avail += PAGE_SIZE * maxcpus * NPTECL;
	pte += maxcpus * NPTECL;
#else
	csrcp = (void *) virtual_avail;  csrc_pte = pte;	/* allocate */
	virtual_avail += PAGE_SIZE; pte++;			/* advance */

	cdstp = (void *) virtual_avail;  cdst_pte = pte;
	virtual_avail += PAGE_SIZE; pte++;

	zerop = (void *) virtual_avail;  zero_pte = pte;
	virtual_avail += PAGE_SIZE; pte++;

	ptpp = (void *) virtual_avail;  ptp_pte = pte;
	virtual_avail += PAGE_SIZE; pte++;
#endif

	if (VM_MIN_KERNEL_ADDRESS == KERNBASE) {
		early_zerop = zerop;
		early_zero_pte = zero_pte;
	}
#endif

	/*
	 * Nothing after this point actually needs pte.
	 */
	pte = (void *)0xdeadbeef;

#ifdef XEN
#ifdef __x86_64__
	/*
	 * We want a dummy page directory for Xen:
	 * when deactivate a pmap, Xen will still consider it active.
	 * So we set user PGD to this one to lift all protection on
	 * the now inactive page tables set.
	 */
	xen_dummy_user_pgd = avail_start;
	avail_start += PAGE_SIZE;
	
	/* Zero fill it, the less checks in Xen it requires the better */
	memset((void *) (xen_dummy_user_pgd + KERNBASE), 0, PAGE_SIZE);
	/* Mark read-only */
	HYPERVISOR_update_va_mapping(xen_dummy_user_pgd + KERNBASE,
	    pmap_pa2pte(xen_dummy_user_pgd) | PG_u | PG_V, UVMF_INVLPG);
	/* Pin as L4 */
	xpq_queue_pin_l4_table(xpmap_ptom_masked(xen_dummy_user_pgd));
#endif /* __x86_64__ */
	idt_vaddr = virtual_avail;                      /* don't need pte */
	idt_paddr = avail_start;                        /* steal a page */
	/*
	 * Xen require one more page as we can't store
	 * GDT and LDT on the same page
	 */
	virtual_avail += 3 * PAGE_SIZE;
	avail_start += 3 * PAGE_SIZE;
#else /* XEN */
	idt_vaddr = virtual_avail;			/* don't need pte */
	idt_paddr = avail_start;			/* steal a page */
#if defined(__x86_64__)
	virtual_avail += 2 * PAGE_SIZE;
	avail_start += 2 * PAGE_SIZE;
#else /* defined(__x86_64__) */
	virtual_avail += PAGE_SIZE;
	avail_start += PAGE_SIZE;
	/* pentium f00f bug stuff */
	pentium_idt_vaddr = virtual_avail;		/* don't need pte */
	virtual_avail += PAGE_SIZE;
#endif /* defined(__x86_64__) */
#endif /* XEN */

#ifdef _LP64
	/*
	 * Grab a page below 4G for things that need it (i.e.
	 * having an initial %cr3 for the MP trampoline).
	 */
	lo32_vaddr = virtual_avail;
	virtual_avail += PAGE_SIZE;
	lo32_paddr = avail_start;
	avail_start += PAGE_SIZE;
#endif

	/*
	 * now we reserve some VM for mapping pages when doing a crash dump
	 */

	virtual_avail = reserve_dumppages(virtual_avail);

	/*
	 * init the static-global locks and global lists.
	 *
	 * => pventry::pvh_lock (initialized elsewhere) must also be
	 *      a spin lock, again at IPL_VM to prevent deadlock, and
	 *	again is never taken from interrupt context.
	 */

	mutex_init(&pmaps_lock, MUTEX_DEFAULT, IPL_NONE);
	LIST_INIT(&pmaps);

	/*
	 * ensure the TLB is sync'd with reality by flushing it...
	 */

	tlbflushg();

	/*
	 * calculate pmap_maxkvaddr from nkptp[].
	 */

	kva = VM_MIN_KERNEL_ADDRESS;
	for (i = PTP_LEVELS - 1; i >= 1; i--) {
		kva += nkptp[i] * nbpd[i];
	}
	pmap_maxkvaddr = kva;
}

#if defined(__x86_64__)
/*
 * Pre-allocate PTPs for low memory, so that 1:1 mappings for various
 * trampoline code can be entered.
 */
void
pmap_prealloc_lowmem_ptps(void)
{
	int level;
	paddr_t newp;
	pd_entry_t *pdes;

	const pd_entry_t pteflags = PG_k | PG_V | PG_RW;

	pdes = pmap_kernel()->pm_pdir;
	level = PTP_LEVELS;
	for (;;) {
		newp = avail_start;
		avail_start += PAGE_SIZE;
#ifdef __HAVE_DIRECT_MAP
		memset((void *)PMAP_DIRECT_MAP(newp), 0, PAGE_SIZE);
#else
		pmap_pte_set(early_zero_pte, pmap_pa2pte(newp) | pteflags);
		pmap_pte_flush();
		pmap_update_pg((vaddr_t)early_zerop);
		memset(early_zerop, 0, PAGE_SIZE);
#endif

#ifdef XEN
		/* Mark R/O before installing */
		HYPERVISOR_update_va_mapping ((vaddr_t)early_zerop,
		    xpmap_ptom_masked(newp) | PG_u | PG_V, UVMF_INVLPG);
		if (newp < (NKL2_KIMG_ENTRIES * NBPD_L2))
			HYPERVISOR_update_va_mapping (newp + KERNBASE,
			    xpmap_ptom_masked(newp) | PG_u | PG_V, UVMF_INVLPG);


		if (level == PTP_LEVELS) { /* Top level pde is per-cpu */
			pd_entry_t *kpm_pdir;
			/* Reach it via recursive mapping */
			kpm_pdir = normal_pdes[PTP_LEVELS - 2];

			/* Set it as usual. We can't defer this
			 * outside the loop since recursive
			 * pte entries won't be accessible during
			 * further iterations at lower levels
			 * otherwise.
			 */
			pmap_pte_set(&kpm_pdir[pl_i(0, PTP_LEVELS)],
			    pmap_pa2pte(newp) | pteflags);
		}

#endif /* XEN */			
		pmap_pte_set(&pdes[pl_i(0, level)],
		    pmap_pa2pte(newp) | pteflags);
			
		pmap_pte_flush();
		
		level--;
		if (level <= 1)
			break;
		pdes = normal_pdes[level - 2];
	}
}
#endif /* defined(__x86_64__) */

/*
 * pmap_init: called from uvm_init, our job is to get the pmap
 * system ready to manage mappings...
 */

void
pmap_init(void)
{
	int i, flags;

	for (i = 0; i < PV_HASH_SIZE; i++) {
		SLIST_INIT(&pv_hash_heads[i].hh_list);
	}
	for (i = 0; i < PV_HASH_LOCK_CNT; i++) {
		mutex_init(&pv_hash_locks[i].lock, MUTEX_NODEBUG, IPL_VM);
	}

	/*
	 * initialize caches.
	 */

	pool_cache_bootstrap(&pmap_cache, sizeof(struct pmap), 0, 0, 0,
	    "pmappl", NULL, IPL_NONE, NULL, NULL, NULL);

#ifdef XEN
	/* 
	 * pool_cache(9) should not touch cached objects, since they
	 * are pinned on xen and R/O for the domU
	 */
	flags = PR_NOTOUCH;
#else /* XEN */
	flags = 0;
#endif /* XEN */
#ifdef PAE
	pool_cache_bootstrap(&pmap_pdp_cache, PAGE_SIZE * PDP_SIZE, 0, 0, flags,
	    "pdppl", &pmap_pdp_allocator, IPL_NONE,
	    pmap_pdp_ctor, pmap_pdp_dtor, NULL);
#else /* PAE */
	pool_cache_bootstrap(&pmap_pdp_cache, PAGE_SIZE, 0, 0, flags,
	    "pdppl", NULL, IPL_NONE, pmap_pdp_ctor, pmap_pdp_dtor, NULL);
#endif /* PAE */
	pool_cache_bootstrap(&pmap_pv_cache, sizeof(struct pv_entry), 0, 0,
	    PR_LARGECACHE, "pvpl", &pool_allocator_kmem, IPL_NONE, NULL,
	    NULL, NULL);

	pmap_tlb_init();

	/* XXX: Since cpu_hatch() is only for secondary CPUs. */
	pmap_tlb_cpu_init(curcpu());

	evcnt_attach_dynamic(&pmap_iobmp_evcnt, EVCNT_TYPE_MISC,
	    NULL, "x86", "io bitmap copy");
	evcnt_attach_dynamic(&pmap_ldt_evcnt, EVCNT_TYPE_MISC,
	    NULL, "x86", "ldt sync");

	/*
	 * done: pmap module is up (and ready for business)
	 */

	pmap_initialized = true;
}

/*
 * pmap_cpu_init_late: perform late per-CPU initialization.
 */

#ifndef XEN
void
pmap_cpu_init_late(struct cpu_info *ci)
{
	/*
	 * The BP has already its own PD page allocated during early
	 * MD startup.
	 */
	if (ci == &cpu_info_primary)
		return;

#ifdef PAE
	cpu_alloc_l3_page(ci);
#endif
}
#endif

/*
 * p v _ e n t r y   f u n c t i o n s
 */

/*
 * pmap_free_pvs: free a list of pv_entrys
 */

static void
pmap_free_pvs(struct pv_entry *pve)
{
	struct pv_entry *next;

	for ( /* null */ ; pve != NULL ; pve = next) {
		next = pve->pve_next;
		pool_cache_put(&pmap_pv_cache, pve);
	}
}

/*
 * main pv_entry manipulation functions:
 *   pmap_enter_pv: enter a mapping onto a pv_head list
 *   pmap_remove_pv: remove a mapping from a pv_head list
 *
 * NOTE: Both pmap_enter_pv and pmap_remove_pv expect the caller to lock 
 *       the pvh before calling
 */

/*
 * insert_pv: a helper of pmap_enter_pv
 */

static void
insert_pv(struct pmap_page *pp, struct pv_entry *pve)
{
	struct pv_hash_head *hh;
	kmutex_t *lock;
	u_int hash;

	hash = pvhash_hash(pve->pve_pte.pte_ptp, pve->pve_pte.pte_va);
	lock = pvhash_lock(hash);
	hh = pvhash_head(hash);
	mutex_spin_enter(lock);
	SLIST_INSERT_HEAD(&hh->hh_list, pve, pve_hash);
	mutex_spin_exit(lock);

	LIST_INSERT_HEAD(&pp->pp_head.pvh_list, pve, pve_list);
}

/*
 * pmap_enter_pv: enter a mapping onto a pv_head lst
 *
 * => caller should adjust ptp's wire_count before calling
 */

static struct pv_entry *
pmap_enter_pv(struct pmap_page *pp,
	      struct pv_entry *pve,	/* preallocated pve for us to use */
	      struct pv_entry **sparepve,
	      struct vm_page *ptp,
	      vaddr_t va)
{

	KASSERT(ptp == NULL || ptp->wire_count >= 2);
	KASSERT(ptp == NULL || ptp->uobject != NULL);
	KASSERT(ptp == NULL || ptp_va2o(va, 1) == ptp->offset);

	if ((pp->pp_flags & PP_EMBEDDED) == 0) {
		if (LIST_EMPTY(&pp->pp_head.pvh_list)) {
			pp->pp_flags |= PP_EMBEDDED;
			pp->pp_pte.pte_ptp = ptp;
			pp->pp_pte.pte_va = va;

			return pve;
		}
	} else {
		struct pv_entry *pve2;

		pve2 = *sparepve;
		*sparepve = NULL;

		pve2->pve_pte = pp->pp_pte;
		pp->pp_flags &= ~PP_EMBEDDED;
		LIST_INIT(&pp->pp_head.pvh_list);
		insert_pv(pp, pve2);
	}

	pve->pve_pte.pte_ptp = ptp;
	pve->pve_pte.pte_va = va;
	insert_pv(pp, pve);

	return NULL;
}

/*
 * pmap_remove_pv: try to remove a mapping from a pv_list
 *
 * => caller should adjust ptp's wire_count and free PTP if needed
 * => we return the removed pve
 */

static struct pv_entry *
pmap_remove_pv(struct pmap_page *pp, struct vm_page *ptp, vaddr_t va)
{
	struct pv_hash_head *hh;
	struct pv_entry *pve;
	kmutex_t *lock;
	u_int hash;

	KASSERT(ptp == NULL || ptp->uobject != NULL);
	KASSERT(ptp == NULL || ptp_va2o(va, 1) == ptp->offset);

	if ((pp->pp_flags & PP_EMBEDDED) != 0) {
		KASSERT(pp->pp_pte.pte_ptp == ptp);
		KASSERT(pp->pp_pte.pte_va == va);

		pp->pp_flags &= ~PP_EMBEDDED;
		LIST_INIT(&pp->pp_head.pvh_list);

		return NULL;
	}

	hash = pvhash_hash(ptp, va);
	lock = pvhash_lock(hash);
	hh = pvhash_head(hash);
	mutex_spin_enter(lock);
	pve = pvhash_remove(hh, ptp, va);
	mutex_spin_exit(lock);

	LIST_REMOVE(pve, pve_list);

	return pve;
}

/*
 * p t p   f u n c t i o n s
 */

static inline struct vm_page *
pmap_find_ptp(struct pmap *pmap, vaddr_t va, paddr_t pa, int level)
{
	int lidx = level - 1;
	struct vm_page *pg;

	KASSERT(mutex_owned(pmap->pm_lock));

	if (pa != (paddr_t)-1 && pmap->pm_ptphint[lidx] &&
	    pa == VM_PAGE_TO_PHYS(pmap->pm_ptphint[lidx])) {
		return (pmap->pm_ptphint[lidx]);
	}
	PMAP_SUBOBJ_LOCK(pmap, lidx);
	pg = uvm_pagelookup(&pmap->pm_obj[lidx], ptp_va2o(va, level));
	PMAP_SUBOBJ_UNLOCK(pmap, lidx);

	KASSERT(pg == NULL || pg->wire_count >= 1);
	return pg;
}

static inline void
pmap_freepage(struct pmap *pmap, struct vm_page *ptp, int level)
{
	lwp_t *l;
	int lidx;
	struct uvm_object *obj;

	KASSERT(ptp->wire_count == 1);

	lidx = level - 1;

	obj = &pmap->pm_obj[lidx];
	pmap_stats_update(pmap, -1, 0);
	if (lidx != 0)
		mutex_enter(obj->vmobjlock);
	if (pmap->pm_ptphint[lidx] == ptp)
		pmap->pm_ptphint[lidx] = TAILQ_FIRST(&obj->memq);
	ptp->wire_count = 0;
	uvm_pagerealloc(ptp, NULL, 0);
	l = curlwp;
	KASSERT((l->l_pflag & LP_INTR) == 0);
	VM_PAGE_TO_PP(ptp)->pp_link = l->l_md.md_gc_ptp;
	l->l_md.md_gc_ptp = ptp;
	if (lidx != 0)
		mutex_exit(obj->vmobjlock);
}

static void
pmap_free_ptp(struct pmap *pmap, struct vm_page *ptp, vaddr_t va,
	      pt_entry_t *ptes, pd_entry_t * const *pdes)
{
	unsigned long index;
	int level;
	vaddr_t invaladdr;
	pd_entry_t opde;

	KASSERT(pmap != pmap_kernel());
	KASSERT(mutex_owned(pmap->pm_lock));
	KASSERT(kpreempt_disabled());

	level = 1;
	do {
		index = pl_i(va, level + 1);
		opde = pmap_pte_testset(&pdes[level - 1][index], 0);
#if defined(XEN)
#  if defined(__x86_64__)
		/*
		 * If ptp is a L3 currently mapped in kernel space,
		 * on any cpu, clear it before freeing
		 */
		if (level == PTP_LEVELS - 1) {
			/*
			 * Update the per-cpu PD on all cpus the current
			 * pmap is active on 
			 */
			xen_kpm_sync(pmap, index);
		}
#  endif /*__x86_64__ */
		invaladdr = level == 1 ? (vaddr_t)ptes :
		    (vaddr_t)pdes[level - 2];
		pmap_tlb_shootdown(pmap, invaladdr + index * PAGE_SIZE,
		    opde, TLBSHOOT_FREE_PTP1);
		pmap_tlb_shootnow();
#else	/* XEN */
		invaladdr = level == 1 ? (vaddr_t)ptes :
		    (vaddr_t)pdes[level - 2];
		pmap_tlb_shootdown(pmap, invaladdr + index * PAGE_SIZE,
		    opde, TLBSHOOT_FREE_PTP1);
#endif	/* XEN */
		pmap_freepage(pmap, ptp, level);
		if (level < PTP_LEVELS - 1) {
			ptp = pmap_find_ptp(pmap, va, (paddr_t)-1, level + 1);
			ptp->wire_count--;
			if (ptp->wire_count > 1)
				break;
		}
	} while (++level < PTP_LEVELS);
	pmap_pte_flush();
}

/*
 * pmap_get_ptp: get a PTP (if there isn't one, allocate a new one)
 *
 * => pmap should NOT be pmap_kernel()
 * => pmap should be locked
 * => preemption should be disabled
 */

static struct vm_page *
pmap_get_ptp(struct pmap *pmap, vaddr_t va, pd_entry_t * const *pdes)
{
	struct vm_page *ptp, *pptp;
	int i;
	unsigned long index;
	pd_entry_t *pva;
	paddr_t ppa, pa;
	struct uvm_object *obj;

	KASSERT(pmap != pmap_kernel());
	KASSERT(mutex_owned(pmap->pm_lock));
	KASSERT(kpreempt_disabled());

	ptp = NULL;
	pa = (paddr_t)-1;

	/*
	 * Loop through all page table levels seeing if we need to
	 * add a new page to that level.
	 */
	for (i = PTP_LEVELS; i > 1; i--) {
		/*
		 * Save values from previous round.
		 */
		pptp = ptp;
		ppa = pa;

		index = pl_i(va, i);
		pva = pdes[i - 2];

		if (pmap_valid_entry(pva[index])) {
			ppa = pmap_pte2pa(pva[index]);
			ptp = NULL;
			continue;
		}

		obj = &pmap->pm_obj[i-2];
		PMAP_SUBOBJ_LOCK(pmap, i - 2);
		ptp = uvm_pagealloc(obj, ptp_va2o(va, i - 1), NULL,
		    UVM_PGA_USERESERVE|UVM_PGA_ZERO);
		PMAP_SUBOBJ_UNLOCK(pmap, i - 2);

		if (ptp == NULL)
			return NULL;

		ptp->flags &= ~PG_BUSY; /* never busy */
		ptp->wire_count = 1;
		pmap->pm_ptphint[i - 2] = ptp;
		pa = VM_PAGE_TO_PHYS(ptp);
		pmap_pte_set(&pva[index], (pd_entry_t)
		        (pmap_pa2pte(pa) | PG_u | PG_RW | PG_V));
#if defined(XEN) && defined(__x86_64__)
		if(i == PTP_LEVELS) {
			/*
			 * Update the per-cpu PD on all cpus the current
			 * pmap is active on 
			 */
			xen_kpm_sync(pmap, index);
		}
#endif
		pmap_pte_flush();
		pmap_stats_update(pmap, 1, 0);
		/*
		 * If we're not in the top level, increase the
		 * wire count of the parent page.
		 */
		if (i < PTP_LEVELS) {
			if (pptp == NULL) {
				pptp = pmap_find_ptp(pmap, va, ppa, i);
				KASSERT(pptp != NULL);
			}
			pptp->wire_count++;
		}
	}

	/*
	 * PTP is not NULL if we just allocated a new PTP.  If it is
	 * still NULL, we must look up the existing one.
	 */
	if (ptp == NULL) {
		ptp = pmap_find_ptp(pmap, va, ppa, 1);
		KASSERTMSG(ptp != NULL, "pmap_get_ptp: va %" PRIxVADDR
		    "ppa %" PRIxPADDR "\n", va, ppa);
	}

	pmap->pm_ptphint[0] = ptp;
	return ptp;
}

/*
 * p m a p  l i f e c y c l e   f u n c t i o n s
 */

/*
 * pmap_pdp_ctor: constructor for the PDP cache.
 */
static int
pmap_pdp_ctor(void *arg, void *v, int flags)
{
	pd_entry_t *pdir = v;
	paddr_t pdirpa = 0;	/* XXX: GCC */
	vaddr_t object;
	int i;

#if !defined(XEN) || !defined(__x86_64__)
	int npde;
#endif
#ifdef XEN
	int s;
#endif

	/*
	 * NOTE: The `pmaps_lock' is held when the PDP is allocated.
	 */

#if defined(XEN) && defined(__x86_64__)
	/* fetch the physical address of the page directory. */
	(void) pmap_extract(pmap_kernel(), (vaddr_t) pdir, &pdirpa);

	/* zero init area */
	memset (pdir, 0, PAGE_SIZE); /* Xen wants a clean page */
	/*
	 * this pdir will NEVER be active in kernel mode
	 * so mark recursive entry invalid
	 */
	pdir[PDIR_SLOT_PTE] = pmap_pa2pte(pdirpa) | PG_u;
	/*
	 * PDP constructed this way won't be for kernel,
	 * hence we don't put kernel mappings on Xen.
	 * But we need to make pmap_create() happy, so put a dummy (without
	 * PG_V) value at the right place.
	 */
	pdir[PDIR_SLOT_KERN + nkptp[PTP_LEVELS - 1] - 1] =
	     (pd_entry_t)-1 & PG_FRAME;
#else /* XEN && __x86_64__*/
	/* zero init area */
	memset(pdir, 0, PDIR_SLOT_PTE * sizeof(pd_entry_t));

	object = (vaddr_t)v;
	for (i = 0; i < PDP_SIZE; i++, object += PAGE_SIZE) {
		/* fetch the physical address of the page directory. */
		(void) pmap_extract(pmap_kernel(), object, &pdirpa);
		/* put in recursive PDE to map the PTEs */
		pdir[PDIR_SLOT_PTE + i] = pmap_pa2pte(pdirpa) | PG_V;
#ifndef XEN
		pdir[PDIR_SLOT_PTE + i] |= PG_KW;
#endif
	}

	/* copy kernel's PDE */
	npde = nkptp[PTP_LEVELS - 1];

	memcpy(&pdir[PDIR_SLOT_KERN], &PDP_BASE[PDIR_SLOT_KERN],
	    npde * sizeof(pd_entry_t));

	/* zero the rest */
	memset(&pdir[PDIR_SLOT_KERN + npde], 0, (PAGE_SIZE * PDP_SIZE) -
	    (PDIR_SLOT_KERN + npde) * sizeof(pd_entry_t));

	if (VM_MIN_KERNEL_ADDRESS != KERNBASE) {
		int idx = pl_i(KERNBASE, PTP_LEVELS);

		pdir[idx] = PDP_BASE[idx];
	}

#ifdef __HAVE_DIRECT_MAP
	pdir[PDIR_SLOT_DIRECT] = PDP_BASE[PDIR_SLOT_DIRECT];
#endif

#endif /* XEN  && __x86_64__*/
#ifdef XEN
	s = splvm();
	object = (vaddr_t)v;
	pmap_protect(pmap_kernel(), object, object + (PAGE_SIZE * PDP_SIZE),
	    VM_PROT_READ);
	pmap_update(pmap_kernel());
	for (i = 0; i < PDP_SIZE; i++, object += PAGE_SIZE) {
		/*
		 * pin as L2/L4 page, we have to do the page with the
		 * PDIR_SLOT_PTE entries last
		 */
#ifdef PAE
		if (i == l2tol3(PDIR_SLOT_PTE))
			continue;
#endif

		(void) pmap_extract(pmap_kernel(), object, &pdirpa);
#ifdef __x86_64__
		xpq_queue_pin_l4_table(xpmap_ptom_masked(pdirpa));
#else
		xpq_queue_pin_l2_table(xpmap_ptom_masked(pdirpa));
#endif
	}
#ifdef PAE
	object = ((vaddr_t)pdir) + PAGE_SIZE  * l2tol3(PDIR_SLOT_PTE);
	(void)pmap_extract(pmap_kernel(), object, &pdirpa);
	xpq_queue_pin_l2_table(xpmap_ptom_masked(pdirpa));
#endif
	splx(s);
#endif /* XEN */

	return (0);
}

/*
 * pmap_pdp_dtor: destructor for the PDP cache.
 */

static void
pmap_pdp_dtor(void *arg, void *v)
{
#ifdef XEN
	paddr_t pdirpa = 0;	/* XXX: GCC */
	vaddr_t object = (vaddr_t)v;
	int i;
	int s = splvm();
	pt_entry_t *pte;

	for (i = 0; i < PDP_SIZE; i++, object += PAGE_SIZE) {
		/* fetch the physical address of the page directory. */
		(void) pmap_extract(pmap_kernel(), object, &pdirpa);
		/* unpin page table */
		xpq_queue_unpin_table(xpmap_ptom_masked(pdirpa));
	}
	object = (vaddr_t)v;
	for (i = 0; i < PDP_SIZE; i++, object += PAGE_SIZE) {
		/* Set page RW again */
		pte = kvtopte(object);
		pmap_pte_set(pte, *pte | PG_RW);
		xen_bcast_invlpg((vaddr_t)object);
	}
	splx(s);
#endif  /* XEN */
}

#ifdef PAE

/* pmap_pdp_alloc: Allocate a page for the pdp memory pool. */

static void *
pmap_pdp_alloc(struct pool *pp, int flags)
{
	return (void *)uvm_km_alloc(kernel_map,
	    PAGE_SIZE * PDP_SIZE, PAGE_SIZE * PDP_SIZE,
	    ((flags & PR_WAITOK) ? 0 : UVM_KMF_NOWAIT | UVM_KMF_TRYLOCK)
	    | UVM_KMF_WIRED);
}

/*
 * pmap_pdp_free: free a PDP
 */

static void
pmap_pdp_free(struct pool *pp, void *v)
{
	uvm_km_free(kernel_map, (vaddr_t)v, PAGE_SIZE * PDP_SIZE,
	    UVM_KMF_WIRED);
}
#endif /* PAE */

/*
 * pmap_create: create a pmap object.
 */
struct pmap *
pmap_create(void)
{
	struct pmap *pmap;
	int i;

	pmap = pool_cache_get(&pmap_cache, PR_WAITOK);

	/* init uvm_object */
	for (i = 0; i < PTP_LEVELS - 1; i++) {
		mutex_init(&pmap->pm_obj_lock[i], MUTEX_DEFAULT, IPL_NONE);
		uvm_obj_init(&pmap->pm_obj[i], NULL, false, 1);
		uvm_obj_setlock(&pmap->pm_obj[i], &pmap->pm_obj_lock[i]);
		pmap->pm_ptphint[i] = NULL;
	}
	pmap->pm_stats.wired_count = 0;
	/* count the PDP allocd below */
	pmap->pm_stats.resident_count = PDP_SIZE;
#if !defined(__x86_64__)
	pmap->pm_hiexec = 0;
#endif /* !defined(__x86_64__) */
	pmap->pm_flags = 0;
	pmap->pm_gc_ptp = NULL;

	kcpuset_create(&pmap->pm_cpus, true);
	kcpuset_create(&pmap->pm_kernel_cpus, true);
#ifdef XEN
	kcpuset_create(&pmap->pm_xen_ptp_cpus, true);
#endif
	/* init the LDT */
	pmap->pm_ldt = NULL;
	pmap->pm_ldt_len = 0;
	pmap->pm_ldt_sel = GSYSSEL(GLDT_SEL, SEL_KPL);

	/* allocate PDP */
 try_again:
	pmap->pm_pdir = pool_cache_get(&pmap_pdp_cache, PR_WAITOK);

	mutex_enter(&pmaps_lock);

	if (pmap->pm_pdir[PDIR_SLOT_KERN + nkptp[PTP_LEVELS - 1] - 1] == 0) {
		mutex_exit(&pmaps_lock);
		pool_cache_destruct_object(&pmap_pdp_cache, pmap->pm_pdir);
		goto try_again;
	}

	for (i = 0; i < PDP_SIZE; i++)
		pmap->pm_pdirpa[i] =
		    pmap_pte2pa(pmap->pm_pdir[PDIR_SLOT_PTE + i]);

	LIST_INSERT_HEAD(&pmaps, pmap, pm_list);

	mutex_exit(&pmaps_lock);

	return (pmap);
}

/*
 * pmap_free_ptps: put a list of ptps back to the freelist.
 */

static void
pmap_free_ptps(struct vm_page *empty_ptps)
{
	struct vm_page *ptp;
	struct pmap_page *pp;

	while ((ptp = empty_ptps) != NULL) {
		pp = VM_PAGE_TO_PP(ptp);
		empty_ptps = pp->pp_link;
		LIST_INIT(&pp->pp_head.pvh_list);
		uvm_pagefree(ptp);
	}
}

/*
 * pmap_destroy: drop reference count on pmap.   free pmap if
 *	reference count goes to zero.
 */

void
pmap_destroy(struct pmap *pmap)
{
	lwp_t *l;
	int i;

	/*
	 * If we have torn down this pmap, process deferred frees and
	 * invalidations.  Free now if the system is low on memory.
	 * Otherwise, free when the pmap is destroyed thus avoiding a
	 * TLB shootdown.
	 */
	l = curlwp;
	if (__predict_false(l->l_md.md_gc_pmap == pmap)) {
		if (uvmexp.free < uvmexp.freetarg) {
			pmap_update(pmap);
		} else {
			KASSERT(pmap->pm_gc_ptp == NULL);
			pmap->pm_gc_ptp = l->l_md.md_gc_ptp;
			l->l_md.md_gc_ptp = NULL;
			l->l_md.md_gc_pmap = NULL;
		}
	}

	/*
	 * drop reference count
	 */

	if (atomic_dec_uint_nv(&pmap->pm_obj[0].uo_refs) > 0) {
		return;
	}

#ifdef DIAGNOSTIC
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;

	for (CPU_INFO_FOREACH(cii, ci)) {
		if (ci->ci_pmap == pmap)
			panic("destroying pmap being used");
#if defined(XEN) && defined(__x86_64__)
		for (i = 0; i < PDIR_SLOT_PTE; i++) {
			if (pmap->pm_pdir[i] != 0 &&
			    ci->ci_kpm_pdir[i] == pmap->pm_pdir[i]) {
				printf("pmap_destroy(%p) pmap_kernel %p "
				    "curcpu %d cpu %d ci_pmap %p "
				    "ci->ci_kpm_pdir[%d]=%" PRIx64
				    " pmap->pm_pdir[%d]=%" PRIx64 "\n",
				    pmap, pmap_kernel(), curcpu()->ci_index,
				    ci->ci_index, ci->ci_pmap,
				    i, ci->ci_kpm_pdir[i],
				    i, pmap->pm_pdir[i]);
				panic("pmap_destroy: used pmap");
			}
		}
#endif
	}
#endif /* DIAGNOSTIC */

	/*
	 * Reference count is zero, free pmap resources and then free pmap.
	 * First, remove it from global list of pmaps.
	 */

	mutex_enter(&pmaps_lock);
	LIST_REMOVE(pmap, pm_list);
	mutex_exit(&pmaps_lock);

	/*
	 * Process deferred PTP frees.  No TLB shootdown required, as the
	 * PTP pages are no longer visible to any CPU.
	 */

	pmap_free_ptps(pmap->pm_gc_ptp);

	/*
	 * destroyed pmap shouldn't have remaining PTPs
	 */

	for (i = 0; i < PTP_LEVELS - 1; i++) {
		KASSERT(pmap->pm_obj[i].uo_npages == 0);
		KASSERT(TAILQ_EMPTY(&pmap->pm_obj[i].memq));
	}

	pool_cache_put(&pmap_pdp_cache, pmap->pm_pdir);

#ifdef USER_LDT
	if (pmap->pm_ldt != NULL) {
		/*
		 * no need to switch the LDT; this address space is gone,
		 * nothing is using it.
		 *
		 * No need to lock the pmap for ldt_free (or anything else),
		 * we're the last one to use it.
		 */
		mutex_enter(&cpu_lock);
		ldt_free(pmap->pm_ldt_sel);
		mutex_exit(&cpu_lock);
		uvm_km_free(kernel_map, (vaddr_t)pmap->pm_ldt,
		    pmap->pm_ldt_len, UVM_KMF_WIRED);
	}
#endif

	for (i = 0; i < PTP_LEVELS - 1; i++) {
		uvm_obj_destroy(&pmap->pm_obj[i], false);
		mutex_destroy(&pmap->pm_obj_lock[i]);
	}
	kcpuset_destroy(pmap->pm_cpus);
	kcpuset_destroy(pmap->pm_kernel_cpus);
#ifdef XEN
	kcpuset_destroy(pmap->pm_xen_ptp_cpus);
#endif
	pool_cache_put(&pmap_cache, pmap);
}

/*
 * pmap_remove_all: pmap is being torn down by the current thread.
 * avoid unnecessary invalidations.
 */

void
pmap_remove_all(struct pmap *pmap)
{
	lwp_t *l = curlwp;

	KASSERT(l->l_md.md_gc_pmap == NULL);

	l->l_md.md_gc_pmap = pmap;
}

#if defined(PMAP_FORK)
/*
 * pmap_fork: perform any necessary data structure manipulation when
 * a VM space is forked.
 */

void
pmap_fork(struct pmap *pmap1, struct pmap *pmap2)
{
#ifdef USER_LDT
	union descriptor *new_ldt;
	size_t len;
	int sel;

	if (__predict_true(pmap1->pm_ldt == NULL)) {
		return;
	}

 retry:
	if (pmap1->pm_ldt != NULL) {
		len = pmap1->pm_ldt_len;
		new_ldt = (union descriptor *)uvm_km_alloc(kernel_map, len, 0,
		    UVM_KMF_WIRED);
		mutex_enter(&cpu_lock);
		sel = ldt_alloc(new_ldt, len);
		if (sel == -1) {
			mutex_exit(&cpu_lock);
			uvm_km_free(kernel_map, (vaddr_t)new_ldt, len,
			    UVM_KMF_WIRED);
			printf("WARNING: pmap_fork: unable to allocate LDT\n");
			return;
		}
	} else {
		len = -1;
		new_ldt = NULL;
		sel = -1;
		mutex_enter(&cpu_lock);
	}

 	/* Copy the LDT, if necessary. */
 	if (pmap1->pm_ldt != NULL) {
		if (len != pmap1->pm_ldt_len) {
			if (len != -1) {
				ldt_free(sel);
				uvm_km_free(kernel_map, (vaddr_t)new_ldt,
				    len, UVM_KMF_WIRED);
			}
			mutex_exit(&cpu_lock);
			goto retry;
		}
  
		memcpy(new_ldt, pmap1->pm_ldt, len);
		pmap2->pm_ldt = new_ldt;
		pmap2->pm_ldt_len = pmap1->pm_ldt_len;
		pmap2->pm_ldt_sel = sel;
		len = -1;
	}

	if (len != -1) {
		ldt_free(sel);
		uvm_km_free(kernel_map, (vaddr_t)new_ldt, len,
		    UVM_KMF_WIRED);
	}
	mutex_exit(&cpu_lock);
#endif /* USER_LDT */
}
#endif /* PMAP_FORK */

#ifdef USER_LDT

/*
 * pmap_ldt_xcall: cross call used by pmap_ldt_sync.  if the named pmap
 * is active, reload LDTR.
 */
static void
pmap_ldt_xcall(void *arg1, void *arg2)
{
	struct pmap *pm;

	kpreempt_disable();
	pm = arg1;
	if (curcpu()->ci_pmap == pm) {
		lldt(pm->pm_ldt_sel);
	}
	kpreempt_enable();
}

/*
 * pmap_ldt_sync: LDT selector for the named pmap is changing.  swap
 * in the new selector on all CPUs.
 */
void
pmap_ldt_sync(struct pmap *pm)
{
	uint64_t where;

	KASSERT(mutex_owned(&cpu_lock));

	pmap_ldt_evcnt.ev_count++;
	where = xc_broadcast(0, pmap_ldt_xcall, pm, NULL);
	xc_wait(where);
}

/*
 * pmap_ldt_cleanup: if the pmap has a local LDT, deallocate it, and
 * restore the default.
 */

void
pmap_ldt_cleanup(struct lwp *l)
{
	pmap_t pmap = l->l_proc->p_vmspace->vm_map.pmap;
	union descriptor *dp = NULL;
	size_t len = 0;
	int sel = -1;

	if (__predict_true(pmap->pm_ldt == NULL)) {
		return;
	}

	mutex_enter(&cpu_lock);
	if (pmap->pm_ldt != NULL) {
		sel = pmap->pm_ldt_sel;
		dp = pmap->pm_ldt;
		len = pmap->pm_ldt_len;
		pmap->pm_ldt_sel = GSYSSEL(GLDT_SEL, SEL_KPL);
		pmap->pm_ldt = NULL;
		pmap->pm_ldt_len = 0;
		pmap_ldt_sync(pmap);
		ldt_free(sel);
		uvm_km_free(kernel_map, (vaddr_t)dp, len, UVM_KMF_WIRED);
	}
	mutex_exit(&cpu_lock);
}
#endif /* USER_LDT */

/*
 * pmap_activate: activate a process' pmap
 *
 * => must be called with kernel preemption disabled
 * => if lwp is the curlwp, then set ci_want_pmapload so that
 *    actual MMU context switch will be done by pmap_load() later
 */

void
pmap_activate(struct lwp *l)
{
	struct cpu_info *ci;
	struct pmap *pmap = vm_map_pmap(&l->l_proc->p_vmspace->vm_map);

	KASSERT(kpreempt_disabled());

	ci = curcpu();

	if (l == ci->ci_curlwp) {
		KASSERT(ci->ci_want_pmapload == 0);
		KASSERT(ci->ci_tlbstate != TLBSTATE_VALID);
#ifdef KSTACK_CHECK_DR0
		/*
		 * setup breakpoint on the top of stack
		 */
		if (l == &lwp0)
			dr0(0, 0, 0, 0);
		else
			dr0(KSTACK_LOWEST_ADDR(l), 1, 3, 1);
#endif

		/*
		 * no need to switch to kernel vmspace because
		 * it's a subset of any vmspace.
		 */

		if (pmap == pmap_kernel()) {
			ci->ci_want_pmapload = 0;
			return;
		}

		ci->ci_want_pmapload = 1;
	}
}

/*
 * pmap_reactivate: try to regain reference to the pmap.
 *
 * => Must be called with kernel preemption disabled.
 */

static bool
pmap_reactivate(struct pmap *pmap)
{
	struct cpu_info * const ci = curcpu();
	const cpuid_t cid = cpu_index(ci);
	bool result;

	KASSERT(kpreempt_disabled());
#if defined(XEN) && defined(__x86_64__)
	KASSERT(pmap_pdirpa(pmap, 0) == ci->ci_xen_current_user_pgd);
#elif defined(PAE)
	KASSERT(pmap_pdirpa(pmap, 0) == pmap_pte2pa(ci->ci_pae_l3_pdir[0]));
#elif !defined(XEN) 
	KASSERT(pmap_pdirpa(pmap, 0) == pmap_pte2pa(rcr3()));
#endif

	/*
	 * If we still have a lazy reference to this pmap, we can assume
	 * that there was no TLB shootdown for this pmap in the meantime.
	 *
	 * The order of events here is important as we must synchronize
	 * with TLB shootdown interrupts.  Declare interest in invalidations
	 * (TLBSTATE_VALID) and then check the CPU set, which the IPIs can
	 * change only when the state is TLBSTATE_LAZY.
	 */

	ci->ci_tlbstate = TLBSTATE_VALID;
	KASSERT(kcpuset_isset(pmap->pm_kernel_cpus, cid));

	if (kcpuset_isset(pmap->pm_cpus, cid)) {
		/* We have the reference, state is valid. */
		result = true;
	} else {
		/* Must reload the TLB. */
		kcpuset_atomic_set(pmap->pm_cpus, cid);
		result = false;
	}
	return result;
}

/*
 * pmap_load: perform the actual pmap switch, i.e. fill in %cr3 register
 * and relevant LDT info.
 *
 * Ensures that the current process' pmap is loaded on the current CPU's
 * MMU and that there are no stale TLB entries.
 *
 * => The caller should disable kernel preemption or do check-and-retry
 *    to prevent a preemption from undoing our efforts.
 * => This function may block.
 */
void
pmap_load(void)
{
	struct cpu_info *ci;
	struct pmap *pmap, *oldpmap;
	struct lwp *l;
	struct pcb *pcb;
	cpuid_t cid;
	uint64_t ncsw;

	kpreempt_disable();
 retry:
	ci = curcpu();
	if (!ci->ci_want_pmapload) {
		kpreempt_enable();
		return;
	}
	l = ci->ci_curlwp;
	ncsw = l->l_ncsw;

	/* should be able to take ipis. */
	KASSERT(ci->ci_ilevel < IPL_HIGH); 
#ifdef XEN
	/* Check to see if interrupts are enabled (ie; no events are masked) */
	KASSERT(x86_read_psl() == 0);
#else
	KASSERT((x86_read_psl() & PSL_I) != 0);
#endif

	KASSERT(l != NULL);
	pmap = vm_map_pmap(&l->l_proc->p_vmspace->vm_map);
	KASSERT(pmap != pmap_kernel());
	oldpmap = ci->ci_pmap;
	pcb = lwp_getpcb(l);

	if (pmap == oldpmap) {
		if (!pmap_reactivate(pmap)) {
			u_int gen = uvm_emap_gen_return();

			/*
			 * pmap has been changed during deactivated.
			 * our tlb may be stale.
			 */

			tlbflush();
			uvm_emap_update(gen);
		}

		ci->ci_want_pmapload = 0;
		kpreempt_enable();
		return;
	}

	/*
	 * Acquire a reference to the new pmap and perform the switch.
	 */

	pmap_reference(pmap);

	cid = cpu_index(ci);
	kcpuset_atomic_clear(oldpmap->pm_cpus, cid);
	kcpuset_atomic_clear(oldpmap->pm_kernel_cpus, cid);

#if defined(XEN) && defined(__x86_64__)
	KASSERT(pmap_pdirpa(oldpmap, 0) == ci->ci_xen_current_user_pgd ||
	    oldpmap == pmap_kernel());
#elif defined(PAE)
	KASSERT(pmap_pdirpa(oldpmap, 0) == pmap_pte2pa(ci->ci_pae_l3_pdir[0]));
#elif !defined(XEN)
	KASSERT(pmap_pdirpa(oldpmap, 0) == pmap_pte2pa(rcr3()));
#endif
	KASSERT(!kcpuset_isset(pmap->pm_cpus, cid));
	KASSERT(!kcpuset_isset(pmap->pm_kernel_cpus, cid));

	/*
	 * Mark the pmap in use by this CPU.  Again, we must synchronize
	 * with TLB shootdown interrupts, so set the state VALID first,
	 * then register us for shootdown events on this pmap.
	 */
	ci->ci_tlbstate = TLBSTATE_VALID;
	kcpuset_atomic_set(pmap->pm_cpus, cid);
	kcpuset_atomic_set(pmap->pm_kernel_cpus, cid);
	ci->ci_pmap = pmap;

	/*
	 * update tss.  now that we have registered for invalidations
	 * from other CPUs, we're good to load the page tables.
	 */
#ifdef PAE
	pcb->pcb_cr3 = ci->ci_pae_l3_pdirpa;
#else
	pcb->pcb_cr3 = pmap_pdirpa(pmap, 0);
#endif

#ifdef i386
#ifndef XEN
	ci->ci_tss.tss_ldt = pmap->pm_ldt_sel;
	ci->ci_tss.tss_cr3 = pcb->pcb_cr3;
#endif /* !XEN */
#endif /* i386 */

	lldt(pmap->pm_ldt_sel);

	u_int gen = uvm_emap_gen_return();
	cpu_load_pmap(pmap, oldpmap);
	uvm_emap_update(gen);

	ci->ci_want_pmapload = 0;

	/*
	 * we're now running with the new pmap.  drop the reference
	 * to the old pmap.  if we block, we need to go around again.
	 */

	pmap_destroy(oldpmap);
	if (l->l_ncsw != ncsw) {
		goto retry;
	}

	kpreempt_enable();
}

/*
 * pmap_deactivate: deactivate a process' pmap.
 *
 * => Must be called with kernel preemption disabled (high IPL is enough).
 */
void
pmap_deactivate(struct lwp *l)
{
	struct pmap *pmap;
	struct cpu_info *ci;

	KASSERT(kpreempt_disabled());

	if (l != curlwp) {
		return;
	}

	/*
	 * Wait for pending TLB shootdowns to complete.  Necessary because
	 * TLB shootdown state is per-CPU, and the LWP may be coming off
	 * the CPU before it has a chance to call pmap_update(), e.g. due
	 * to kernel preemption or blocking routine in between.
	 */
	pmap_tlb_shootnow();

	ci = curcpu();

	if (ci->ci_want_pmapload) {
		/*
		 * ci_want_pmapload means that our pmap is not loaded on
		 * the CPU or TLB might be stale.  note that pmap_kernel()
		 * is always considered loaded.
		 */
		KASSERT(vm_map_pmap(&l->l_proc->p_vmspace->vm_map)
		    != pmap_kernel());
		KASSERT(vm_map_pmap(&l->l_proc->p_vmspace->vm_map)
		    != ci->ci_pmap || ci->ci_tlbstate != TLBSTATE_VALID);

		/*
		 * userspace has not been touched.
		 * nothing to do here.
		 */

		ci->ci_want_pmapload = 0;
		return;
	}

	pmap = vm_map_pmap(&l->l_proc->p_vmspace->vm_map);

	if (pmap == pmap_kernel()) {
		return;
	}

#if defined(XEN) && defined(__x86_64__)
	KASSERT(pmap_pdirpa(pmap, 0) == ci->ci_xen_current_user_pgd);
#elif defined(PAE)
	KASSERT(pmap_pdirpa(pmap, 0) == pmap_pte2pa(ci->ci_pae_l3_pdir[0]));
#elif !defined(XEN) 
	KASSERT(pmap_pdirpa(pmap, 0) == pmap_pte2pa(rcr3()));
#endif
	KASSERT(ci->ci_pmap == pmap);

	/*
	 * we aren't interested in TLB invalidations for this pmap,
	 * at least for the time being.
	 */

	KASSERT(ci->ci_tlbstate == TLBSTATE_VALID);
	ci->ci_tlbstate = TLBSTATE_LAZY;
}

/*
 * end of lifecycle functions
 */

/*
 * some misc. functions
 */

int
pmap_pdes_invalid(vaddr_t va, pd_entry_t * const *pdes, pd_entry_t *lastpde)
{
	int i;
	unsigned long index;
	pd_entry_t pde;

	for (i = PTP_LEVELS; i > 1; i--) {
		index = pl_i(va, i);
		pde = pdes[i - 2][index];
		if ((pde & PG_V) == 0)
			return i;
	}
	if (lastpde != NULL)
		*lastpde = pde;
	return 0;
}

/*
 * pmap_extract: extract a PA for the given VA
 */

bool
pmap_extract(struct pmap *pmap, vaddr_t va, paddr_t *pap)
{
	pt_entry_t *ptes, pte;
	pd_entry_t pde;
	pd_entry_t * const *pdes;
	struct pmap *pmap2;
	struct cpu_info *ci;
	paddr_t pa;
	lwp_t *l;
	bool hard, rv;

#ifdef __HAVE_DIRECT_MAP
	if (va >= PMAP_DIRECT_BASE && va < PMAP_DIRECT_END) {
		if (pap != NULL) {
			*pap = va - PMAP_DIRECT_BASE;
		}
		return true;
	}
#endif

	rv = false;
	pa = 0;
	l = curlwp;

	KPREEMPT_DISABLE(l);
	ci = l->l_cpu;
	if (__predict_true(!ci->ci_want_pmapload && ci->ci_pmap == pmap) ||
	    pmap == pmap_kernel()) {
		/*
		 * no need to lock, because it's pmap_kernel() or our
		 * own pmap and is active.  if a user pmap, the caller
		 * will hold the vm_map write/read locked and so prevent
		 * entries from disappearing while we are here.  ptps
		 * can disappear via pmap_remove() and pmap_protect(),
		 * but they are called with the vm_map write locked.
		 */
		hard = false;
		ptes = PTE_BASE;
		pdes = normal_pdes;
	} else {
		/* we lose, do it the hard way. */
		hard = true;
		pmap_map_ptes(pmap, &pmap2, &ptes, &pdes);
	}
	if (pmap_pdes_valid(va, pdes, &pde)) {
		pte = ptes[pl1_i(va)];
		if (pde & PG_PS) {
			pa = (pde & PG_LGFRAME) | (va & (NBPD_L2 - 1));
			rv = true;
		} else if (__predict_true((pte & PG_V) != 0)) {
			pa = pmap_pte2pa(pte) | (va & (NBPD_L1 - 1));
			rv = true;
		}
	}
	if (__predict_false(hard)) {
		pmap_unmap_ptes(pmap, pmap2);
	}
	KPREEMPT_ENABLE(l);
	if (pap != NULL) {
		*pap = pa;
	}
	return rv;
}


/*
 * vtophys: virtual address to physical address.  For use by
 * machine-dependent code only.
 */

paddr_t
vtophys(vaddr_t va)
{
	paddr_t pa;

	if (pmap_extract(pmap_kernel(), va, &pa) == true)
		return (pa);
	return (0);
}

__strict_weak_alias(pmap_extract_ma, pmap_extract);

#ifdef XEN

/*
 * vtomach: virtual address to machine address.  For use by
 * machine-dependent code only.
 */

paddr_t
vtomach(vaddr_t va)
{
	paddr_t pa;

	if (pmap_extract_ma(pmap_kernel(), va, &pa) == true)
		return (pa);
	return (0);
}

#endif /* XEN */

/*
 * pmap_virtual_space: used during bootup [pmap_steal_memory] to
 *	determine the bounds of the kernel virtual addess space.
 */

void
pmap_virtual_space(vaddr_t *startp, vaddr_t *endp)
{
	*startp = virtual_avail;
	*endp = virtual_end;
}

/*
 * pmap_zero_page: zero a page
 */

void
pmap_zero_page(paddr_t pa)
{
#if defined(__HAVE_DIRECT_MAP)
	pagezero(PMAP_DIRECT_MAP(pa));
#else
#if defined(XEN)
	if (XEN_VERSION_SUPPORTED(3, 4))
		xen_pagezero(pa);
#endif
	pt_entry_t *zpte;
	void *zerova;
	int id;

	kpreempt_disable();
	id = cpu_number();
	zpte = PTESLEW(zero_pte, id);
	zerova = VASLEW(zerop, id);

#ifdef DIAGNOSTIC
	if (*zpte)
		panic("pmap_zero_page: lock botch");
#endif

	pmap_pte_set(zpte, pmap_pa2pte(pa) | PG_V | PG_RW | PG_M | PG_U | PG_k);
	pmap_pte_flush();
	pmap_update_pg((vaddr_t)zerova);		/* flush TLB */

	memset(zerova, 0, PAGE_SIZE);

#if defined(DIAGNOSTIC) || defined(XEN)
	pmap_pte_set(zpte, 0);				/* zap ! */
	pmap_pte_flush();
#endif
	kpreempt_enable();
#endif /* defined(__HAVE_DIRECT_MAP) */
}

/*
 * pmap_pagezeroidle: the same, for the idle loop page zero'er.
 * Returns true if the page was zero'd, false if we aborted for
 * some reason.
 */

bool
pmap_pageidlezero(paddr_t pa)
{
#ifdef __HAVE_DIRECT_MAP
	KASSERT(cpu_feature[0] & CPUID_SSE2);
	return sse2_idlezero_page((void *)PMAP_DIRECT_MAP(pa));
#else
	pt_entry_t *zpte;
	void *zerova;
	bool rv;
	int id;

	id = cpu_number();
	zpte = PTESLEW(zero_pte, id);
	zerova = VASLEW(zerop, id);

	KASSERT(cpu_feature[0] & CPUID_SSE2);
	KASSERT(*zpte == 0);

	pmap_pte_set(zpte, pmap_pa2pte(pa) | PG_V | PG_RW | PG_M | PG_U | PG_k);
	pmap_pte_flush();
	pmap_update_pg((vaddr_t)zerova);		/* flush TLB */

	rv = sse2_idlezero_page(zerova);

#if defined(DIAGNOSTIC) || defined(XEN)
	pmap_pte_set(zpte, 0);				/* zap ! */
	pmap_pte_flush();
#endif

	return rv;
#endif
}

/*
 * pmap_copy_page: copy a page
 */

void
pmap_copy_page(paddr_t srcpa, paddr_t dstpa)
{
#if defined(__HAVE_DIRECT_MAP)
	vaddr_t srcva = PMAP_DIRECT_MAP(srcpa);
	vaddr_t dstva = PMAP_DIRECT_MAP(dstpa);

	memcpy((void *)dstva, (void *)srcva, PAGE_SIZE);
#else
#if defined(XEN)
	if (XEN_VERSION_SUPPORTED(3, 4)) {
		xen_copy_page(srcpa, dstpa);
		return;
	}
#endif
	pt_entry_t *spte;
	pt_entry_t *dpte;
	void *csrcva;
	void *cdstva;
	int id;

	kpreempt_disable();
	id = cpu_number();
	spte = PTESLEW(csrc_pte,id);
	dpte = PTESLEW(cdst_pte,id);
	csrcva = VASLEW(csrcp, id);
	cdstva = VASLEW(cdstp, id);
	
	KASSERT(*spte == 0 && *dpte == 0);

	pmap_pte_set(spte, pmap_pa2pte(srcpa) | PG_V | PG_RW | PG_U | PG_k);
	pmap_pte_set(dpte,
	    pmap_pa2pte(dstpa) | PG_V | PG_RW | PG_M | PG_U | PG_k);
	pmap_pte_flush();
	pmap_update_2pg((vaddr_t)csrcva, (vaddr_t)cdstva);

	memcpy(cdstva, csrcva, PAGE_SIZE);

#if defined(DIAGNOSTIC) || defined(XEN)
	pmap_pte_set(spte, 0);
	pmap_pte_set(dpte, 0);
	pmap_pte_flush();
#endif
	kpreempt_enable();
#endif /* defined(__HAVE_DIRECT_MAP) */
}

static pt_entry_t *
pmap_map_ptp(struct vm_page *ptp)
{
#ifdef __HAVE_DIRECT_MAP
	return (void *)PMAP_DIRECT_MAP(VM_PAGE_TO_PHYS(ptp));
#else
	pt_entry_t *ptppte;
	void *ptpva;
	int id;

	KASSERT(kpreempt_disabled());

	id = cpu_number();
	ptppte = PTESLEW(ptp_pte, id);
	ptpva = VASLEW(ptpp, id);
#if !defined(XEN)
	pmap_pte_set(ptppte, pmap_pa2pte(VM_PAGE_TO_PHYS(ptp)) | PG_V | PG_M |
	    PG_RW | PG_U | PG_k);
#else
	pmap_pte_set(ptppte, pmap_pa2pte(VM_PAGE_TO_PHYS(ptp)) | PG_V | PG_M |
	    PG_U | PG_k);
#endif
	pmap_pte_flush();
	pmap_update_pg((vaddr_t)ptpva);

	return (pt_entry_t *)ptpva;
#endif
}

static void
pmap_unmap_ptp(void)
{
#ifndef __HAVE_DIRECT_MAP
#if defined(DIAGNOSTIC) || defined(XEN)
	pt_entry_t *pte;

	KASSERT(kpreempt_disabled());

	pte = PTESLEW(ptp_pte, cpu_number());
	if (*pte != 0) {
		pmap_pte_set(pte, 0);
		pmap_pte_flush();
	}
#endif
#endif
}

static pt_entry_t *
pmap_map_pte(struct pmap *pmap, struct vm_page *ptp, vaddr_t va)
{

	KASSERT(kpreempt_disabled());
	if (pmap_is_curpmap(pmap)) {
		return &PTE_BASE[pl1_i(va)]; /* (k)vtopte */
	}
	KASSERT(ptp != NULL);
	return pmap_map_ptp(ptp) + pl1_pi(va);
}

static void
pmap_unmap_pte(void)
{

	KASSERT(kpreempt_disabled());

	pmap_unmap_ptp();
}

/*
 * p m a p   r e m o v e   f u n c t i o n s
 *
 * functions that remove mappings
 */

/*
 * pmap_remove_ptes: remove PTEs from a PTP
 *
 * => caller must hold pmap's lock
 * => PTP must be mapped into KVA
 * => PTP should be null if pmap == pmap_kernel()
 * => must be called with kernel preemption disabled
 * => returns composite pte if at least one page should be shot down
 */

static void
pmap_remove_ptes(struct pmap *pmap, struct vm_page *ptp, vaddr_t ptpva,
		 vaddr_t startva, vaddr_t endva, struct pv_entry **pv_tofree)
{
	pt_entry_t *pte = (pt_entry_t *)ptpva;

	KASSERT(pmap == pmap_kernel() || mutex_owned(pmap->pm_lock));
	KASSERT(kpreempt_disabled());

	/*
	 * note that ptpva points to the PTE that maps startva.   this may
	 * or may not be the first PTE in the PTP.
	 *
	 * we loop through the PTP while there are still PTEs to look at
	 * and the wire_count is greater than 1 (because we use the wire_count
	 * to keep track of the number of real PTEs in the PTP).
	 */
	while (startva < endva && (ptp == NULL || ptp->wire_count > 1)) {
		(void)pmap_remove_pte(pmap, ptp, pte, startva, pv_tofree);
		startva += PAGE_SIZE;
		pte++;
	}
}


/*
 * pmap_remove_pte: remove a single PTE from a PTP.
 *
 * => caller must hold pmap's lock
 * => PTP must be mapped into KVA
 * => PTP should be null if pmap == pmap_kernel()
 * => returns true if we removed a mapping
 * => must be called with kernel preemption disabled
 */
static bool
pmap_remove_pte(struct pmap *pmap, struct vm_page *ptp, pt_entry_t *pte,
		vaddr_t va, struct pv_entry **pv_tofree)
{
	struct pv_entry *pve;
	struct vm_page *pg;
	struct pmap_page *pp;
	pt_entry_t opte;

	KASSERT(pmap == pmap_kernel() || mutex_owned(pmap->pm_lock));
	KASSERT(kpreempt_disabled());

	if (!pmap_valid_entry(*pte)) {
		/* VA not mapped. */
		return false;
	}

	/* Atomically save the old PTE and zap it. */
	opte = pmap_pte_testset(pte, 0);
	if (!pmap_valid_entry(opte)) {
		return false;
	}

	pmap_exec_account(pmap, va, opte, 0);
	pmap_stats_update_bypte(pmap, 0, opte);

	if (ptp) {
		/*
		 * Dropping a PTE.  Make sure that the PDE is flushed.
		 */
		ptp->wire_count--;
		if (ptp->wire_count <= 1) {
			opte |= PG_U;
		}
	}

	if ((opte & PG_U) != 0) {
		pmap_tlb_shootdown(pmap, va, opte, TLBSHOOT_REMOVE_PTE);
	}

	/*
	 * If we are not on a pv_head list - we are done.
	 */
	if ((opte & PG_PVLIST) == 0) {
#if defined(DIAGNOSTIC) && !defined(DOM0OPS)
		if (PHYS_TO_VM_PAGE(pmap_pte2pa(opte)) != NULL)
			panic("pmap_remove_pte: managed page without "
			      "PG_PVLIST for %#" PRIxVADDR, va);
#endif
		return true;
	}

	pg = PHYS_TO_VM_PAGE(pmap_pte2pa(opte));

	KASSERTMSG(pg != NULL, "pmap_remove_pte: unmanaged page marked "
	    "PG_PVLIST, va = %#" PRIxVADDR ", pa = %#" PRIxPADDR,
	    va, (paddr_t)pmap_pte2pa(opte));

	KASSERT(uvm_page_locked_p(pg));

	/* Sync R/M bits. */
	pp = VM_PAGE_TO_PP(pg);
	pp->pp_attrs |= opte;
	pve = pmap_remove_pv(pp, ptp, va);

	if (pve) { 
		pve->pve_next = *pv_tofree;
		*pv_tofree = pve;
	}
	return true;
}

/*
 * pmap_remove: mapping removal function.
 *
 * => caller should not be holding any pmap locks
 */

void
pmap_remove(struct pmap *pmap, vaddr_t sva, vaddr_t eva)
{
	pt_entry_t *ptes;
	pd_entry_t pde;
	pd_entry_t * const *pdes;
	struct pv_entry *pv_tofree = NULL;
	bool result;
	int i;
	paddr_t ptppa;
	vaddr_t blkendva, va = sva;
	struct vm_page *ptp;
	struct pmap *pmap2;

	kpreempt_disable();
	pmap_map_ptes(pmap, &pmap2, &ptes, &pdes);	/* locks pmap */

	/*
	 * removing one page?  take shortcut function.
	 */

	if (va + PAGE_SIZE == eva) {
		if (pmap_pdes_valid(va, pdes, &pde)) {

			/* PA of the PTP */
			ptppa = pmap_pte2pa(pde);

			/* Get PTP if non-kernel mapping. */
			if (pmap != pmap_kernel()) {
				ptp = pmap_find_ptp(pmap, va, ptppa, 1);
				KASSERTMSG(ptp != NULL,
				    "pmap_remove: unmanaged PTP detected");
			} else {
				/* Never free kernel PTPs. */
				ptp = NULL;
			}

			result = pmap_remove_pte(pmap, ptp,
			    &ptes[pl1_i(va)], va, &pv_tofree);

			/*
			 * if mapping removed and the PTP is no longer
			 * being used, free it!
			 */

			if (result && ptp && ptp->wire_count <= 1)
				pmap_free_ptp(pmap, ptp, va, ptes, pdes);
		}
	} else for (/* null */ ; va < eva ; va = blkendva) {
		int lvl;

		/* determine range of block */
		blkendva = x86_round_pdr(va+1);
		if (blkendva > eva)
			blkendva = eva;

		/*
		 * XXXCDC: our PTE mappings should never be removed
		 * with pmap_remove!  if we allow this (and why would
		 * we?) then we end up freeing the pmap's page
		 * directory page (PDP) before we are finished using
		 * it when we hit in in the recursive mapping.  this
		 * is BAD.
		 *
		 * long term solution is to move the PTEs out of user
		 * address space.  and into kernel address space (up
		 * with APTE).  then we can set VM_MAXUSER_ADDRESS to
		 * be VM_MAX_ADDRESS.
		 */

		/* XXXCDC: ugly hack to avoid freeing PDP here */
		for (i = 0; i < PDP_SIZE; i++) {
			if (pl_i(va, PTP_LEVELS) == PDIR_SLOT_PTE+i)
				continue;
		}

		lvl = pmap_pdes_invalid(va, pdes, &pde);
		if (lvl != 0) {
			/*
			 * skip a range corresponding to an invalid pde.
			 */
			blkendva = (va & ptp_masks[lvl - 1]) + nbpd[lvl - 1]; 
 			continue;
		}

		/* PA of the PTP */
		ptppa = pmap_pte2pa(pde);

		/* Get PTP if non-kernel mapping. */
		if (pmap != pmap_kernel()) {
			ptp = pmap_find_ptp(pmap, va, ptppa, 1);
			KASSERTMSG(ptp != NULL,
			    "pmap_remove: unmanaged PTP detected");
		} else {
			/* Never free kernel PTPs. */
			ptp = NULL;
		}

		pmap_remove_ptes(pmap, ptp, (vaddr_t)&ptes[pl1_i(va)], va,
		    blkendva, &pv_tofree);

		/* if PTP is no longer being used, free it! */
		if (ptp && ptp->wire_count <= 1) {
			pmap_free_ptp(pmap, ptp, va, ptes, pdes);
		}
	}
	pmap_unmap_ptes(pmap, pmap2);		/* unlock pmap */
	kpreempt_enable();

	/* Now we free unused PVs */
	if (pv_tofree)
		pmap_free_pvs(pv_tofree);
}

/*
 * pmap_sync_pv: clear pte bits and return the old value of the pte.
 *
 * => Caller should disable kernel preemption.
 * => issues tlb shootdowns if necessary.
 */

static int
pmap_sync_pv(struct pv_pte *pvpte, pt_entry_t expect, int clearbits,
    pt_entry_t *optep)
{
	struct pmap *pmap;
	struct vm_page *ptp;
	vaddr_t va;
	pt_entry_t *ptep;
	pt_entry_t opte;
	pt_entry_t npte;
	bool need_shootdown;

	ptp = pvpte->pte_ptp;
	va = pvpte->pte_va;
	KASSERT(ptp == NULL || ptp->uobject != NULL);
	KASSERT(ptp == NULL || ptp_va2o(va, 1) == ptp->offset);
	pmap = ptp_to_pmap(ptp);

	KASSERT((expect & ~(PG_FRAME | PG_V)) == 0);
	KASSERT((expect & PG_V) != 0);
	KASSERT(clearbits == ~0 || (clearbits & ~(PG_M | PG_U | PG_RW)) == 0);
	KASSERT(kpreempt_disabled());

	ptep = pmap_map_pte(pmap, ptp, va);
	do {
		opte = *ptep;
		KASSERT((opte & (PG_M | PG_U)) != PG_M);
		KASSERT((opte & (PG_U | PG_V)) != PG_U);
		KASSERT(opte == 0 || (opte & PG_V) != 0);
		if ((opte & (PG_FRAME | PG_V)) != expect) {

			/*
			 * we lost a race with a V->P operation like
			 * pmap_remove().  wait for the competitor
			 * reflecting pte bits into mp_attrs.
			 *
			 * issue a redundant TLB shootdown so that
			 * we can wait for its completion.
			 */

			pmap_unmap_pte();
			if (clearbits != 0) {
				pmap_tlb_shootdown(pmap, va,
				    (pmap == pmap_kernel() ? PG_G : 0),
				    TLBSHOOT_SYNC_PV1);
			}
			return EAGAIN;
		}

		/*
		 * check if there's anything to do on this pte.
		 */

		if ((opte & clearbits) == 0) {
			need_shootdown = false;
			break;
		}

		/*
		 * we need a shootdown if the pte is cached. (PG_U)
		 *
		 * ...unless we are clearing only the PG_RW bit and
		 * it isn't cached as RW. (PG_M)
		 */

		need_shootdown = (opte & PG_U) != 0 &&
		    !(clearbits == PG_RW && (opte & PG_M) == 0);

		npte = opte & ~clearbits;

		/*
		 * if we need a shootdown anyway, clear PG_U and PG_M.
		 */

		if (need_shootdown) {
			npte &= ~(PG_U | PG_M);
		}
		KASSERT((npte & (PG_M | PG_U)) != PG_M);
		KASSERT((npte & (PG_U | PG_V)) != PG_U);
		KASSERT(npte == 0 || (opte & PG_V) != 0);
	} while (pmap_pte_cas(ptep, opte, npte) != opte);

	if (need_shootdown) {
		pmap_tlb_shootdown(pmap, va, opte, TLBSHOOT_SYNC_PV2);
	}
	pmap_unmap_pte();

	*optep = opte;
	return 0;
}

/*
 * pmap_page_remove: remove a managed vm_page from all pmaps that map it
 *
 * => R/M bits are sync'd back to attrs
 */

void
pmap_page_remove(struct vm_page *pg)
{
	struct pmap_page *pp;
	struct pv_pte *pvpte;
	struct pv_entry *killlist = NULL;
	struct vm_page *ptp;
	pt_entry_t expect;
	int count;

	KASSERT(uvm_page_locked_p(pg));

	pp = VM_PAGE_TO_PP(pg);
	expect = pmap_pa2pte(VM_PAGE_TO_PHYS(pg)) | PG_V;
	count = SPINLOCK_BACKOFF_MIN;
	kpreempt_disable();
startover:
	while ((pvpte = pv_pte_first(pp)) != NULL) {
		struct pmap *pmap;
		struct pv_entry *pve;
		pt_entry_t opte;
		vaddr_t va;
		int error;

		/*
		 * add a reference to the pmap before clearing the pte.
		 * otherwise the pmap can disappear behind us.
		 */

		ptp = pvpte->pte_ptp;
		pmap = ptp_to_pmap(ptp);
		if (ptp != NULL) {
			pmap_reference(pmap);
		}

		error = pmap_sync_pv(pvpte, expect, ~0, &opte);
		if (error == EAGAIN) {
			int hold_count;
			KERNEL_UNLOCK_ALL(curlwp, &hold_count);
			if (ptp != NULL) {
				pmap_destroy(pmap);
			}
			SPINLOCK_BACKOFF(count);
			KERNEL_LOCK(hold_count, curlwp);
			goto startover;
		}

		pp->pp_attrs |= opte;
		va = pvpte->pte_va;
		pve = pmap_remove_pv(pp, ptp, va);

		/* update the PTP reference count.  free if last reference. */
		if (ptp != NULL) {
			struct pmap *pmap2;
			pt_entry_t *ptes;
			pd_entry_t * const *pdes;

			KASSERT(pmap != pmap_kernel());

			pmap_tlb_shootnow();
			pmap_map_ptes(pmap, &pmap2, &ptes, &pdes);
			pmap_stats_update_bypte(pmap, 0, opte);
			ptp->wire_count--;
			if (ptp->wire_count <= 1) {
				pmap_free_ptp(pmap, ptp, va, ptes, pdes);
			}
			pmap_unmap_ptes(pmap, pmap2);
			pmap_destroy(pmap);
		} else {
			KASSERT(pmap == pmap_kernel());
			pmap_stats_update_bypte(pmap, 0, opte);
		}

		if (pve != NULL) {
			pve->pve_next = killlist;	/* mark it for death */
			killlist = pve;
		}
	}
	pmap_tlb_shootnow();
	kpreempt_enable();

	/* Now free unused pvs. */
	pmap_free_pvs(killlist);
}

/*
 * p m a p   a t t r i b u t e  f u n c t i o n s
 * functions that test/change managed page's attributes
 * since a page can be mapped multiple times we must check each PTE that
 * maps it by going down the pv lists.
 */

/*
 * pmap_test_attrs: test a page's attributes
 */

bool
pmap_test_attrs(struct vm_page *pg, unsigned testbits)
{
	struct pmap_page *pp;
	struct pv_pte *pvpte;
	pt_entry_t expect;
	u_int result;

	KASSERT(uvm_page_locked_p(pg));

	pp = VM_PAGE_TO_PP(pg);
	if ((pp->pp_attrs & testbits) != 0) {
		return true;
	}
	expect = pmap_pa2pte(VM_PAGE_TO_PHYS(pg)) | PG_V;
	kpreempt_disable();
	for (pvpte = pv_pte_first(pp); pvpte; pvpte = pv_pte_next(pp, pvpte)) {
		pt_entry_t opte;
		int error;

		if ((pp->pp_attrs & testbits) != 0) {
			break;
		}
		error = pmap_sync_pv(pvpte, expect, 0, &opte);
		if (error == 0) {
			pp->pp_attrs |= opte;
		}
	}
	result = pp->pp_attrs & testbits;
	kpreempt_enable();

	/*
	 * note that we will exit the for loop with a non-null pve if
	 * we have found the bits we are testing for.
	 */

	return result != 0;
}

/*
 * pmap_clear_attrs: clear the specified attribute for a page.
 *
 * => we return true if we cleared one of the bits we were asked to
 */

bool
pmap_clear_attrs(struct vm_page *pg, unsigned clearbits)
{
	struct pmap_page *pp;
	struct pv_pte *pvpte;
	u_int result;
	pt_entry_t expect;
	int count;

	KASSERT(uvm_page_locked_p(pg));

	pp = VM_PAGE_TO_PP(pg);
	expect = pmap_pa2pte(VM_PAGE_TO_PHYS(pg)) | PG_V;
	count = SPINLOCK_BACKOFF_MIN;
	kpreempt_disable();
startover:
	for (pvpte = pv_pte_first(pp); pvpte; pvpte = pv_pte_next(pp, pvpte)) {
		pt_entry_t opte;
		int error;

		error = pmap_sync_pv(pvpte, expect, clearbits, &opte);
		if (error == EAGAIN) {
			int hold_count;
			KERNEL_UNLOCK_ALL(curlwp, &hold_count);
			SPINLOCK_BACKOFF(count);
			KERNEL_LOCK(hold_count, curlwp);
			goto startover;
		}
		pp->pp_attrs |= opte;
	}
	result = pp->pp_attrs & clearbits;
	pp->pp_attrs &= ~clearbits;
	kpreempt_enable();

	return result != 0;
}


/*
 * p m a p   p r o t e c t i o n   f u n c t i o n s
 */

/*
 * pmap_page_protect: change the protection of all recorded mappings
 *	of a managed page
 *
 * => NOTE: this is an inline function in pmap.h
 */

/* see pmap.h */

/*
 * pmap_protect: set the protection in of the pages in a pmap
 *
 * => NOTE: this is an inline function in pmap.h
 */

/* see pmap.h */

/*
 * pmap_write_protect: write-protect pages in a pmap.
 */
void
pmap_write_protect(struct pmap *pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
	pt_entry_t *ptes;
	pt_entry_t * const *pdes;
	struct pmap *pmap2;
	vaddr_t blockend, va;

	KASSERT(curlwp->l_md.md_gc_pmap != pmap);

	sva &= PG_FRAME;
	eva &= PG_FRAME;

	/* Acquire pmap. */
	kpreempt_disable();
	pmap_map_ptes(pmap, &pmap2, &ptes, &pdes);

	for (va = sva ; va < eva ; va = blockend) {
		pt_entry_t *spte, *epte;
		int i;

		blockend = x86_round_pdr(va + 1);
		if (blockend > eva)
			blockend = eva;

		/*
		 * XXXCDC: our PTE mappings should never be write-protected!
		 *
		 * long term solution is to move the PTEs out of user
		 * address space.  and into kernel address space (up
		 * with APTE).  then we can set VM_MAXUSER_ADDRESS to
		 * be VM_MAX_ADDRESS.
		 */

		/* XXXCDC: ugly hack to avoid freeing PDP here */
		for (i = 0; i < PDP_SIZE; i++) {
			if (pl_i(va, PTP_LEVELS) == PDIR_SLOT_PTE+i)
				continue;
		}

		/* Is it a valid block? */
		if (!pmap_pdes_valid(va, pdes, NULL)) {
			continue;
		}
		KASSERT(va < VM_MAXUSER_ADDRESS || va >= VM_MAX_ADDRESS);

		spte = &ptes[pl1_i(va)];
		epte = &ptes[pl1_i(blockend)];

		for (/*null */; spte < epte ; spte++) {
			pt_entry_t opte, npte;

			do {
				opte = *spte;
				if ((~opte & (PG_RW | PG_V)) != 0) {
					goto next;
				}
				npte = opte & ~PG_RW;
			} while (pmap_pte_cas(spte, opte, npte) != opte);

			if ((opte & PG_M) != 0) {
				vaddr_t tva = x86_ptob(spte - ptes);
				pmap_tlb_shootdown(pmap, tva, opte,
				    TLBSHOOT_WRITE_PROTECT);
			}
next:;
		}
	}

	/* Release pmap. */
	pmap_unmap_ptes(pmap, pmap2);
	kpreempt_enable();
}

/*
 * pmap_unwire: clear the wired bit in the PTE.
 *
 * => Mapping should already be present.
 */
void
pmap_unwire(struct pmap *pmap, vaddr_t va)
{
	pt_entry_t *ptes, *ptep, opte;
	pd_entry_t * const *pdes;
	struct pmap *pmap2;

	/* Acquire pmap. */
	kpreempt_disable();
	pmap_map_ptes(pmap, &pmap2, &ptes, &pdes);

	if (!pmap_pdes_valid(va, pdes, NULL)) {
		panic("pmap_unwire: invalid PDE");
	}

	ptep = &ptes[pl1_i(va)];
	opte = *ptep;
	KASSERT(pmap_valid_entry(opte));

	if (opte & PG_W) {
		pt_entry_t npte = opte & ~PG_W;

		opte = pmap_pte_testset(ptep, npte);
		pmap_stats_update_bypte(pmap, npte, opte);
	} else {
		printf("pmap_unwire: wiring for pmap %p va 0x%lx "
		    "did not change!\n", pmap, va);
	}

	/* Release pmap. */
	pmap_unmap_ptes(pmap, pmap2);
	kpreempt_enable();
}

/*
 * pmap_copy: copy mappings from one pmap to another
 *
 * => optional function
 * void pmap_copy(dst_pmap, src_pmap, dst_addr, len, src_addr)
 */

/*
 * defined as macro in pmap.h
 */

__strict_weak_alias(pmap_enter, pmap_enter_default);

int
pmap_enter_default(pmap_t pmap, vaddr_t va, paddr_t pa, vm_prot_t prot,
    u_int flags)
{
	return pmap_enter_ma(pmap, va, pa, pa, prot, flags, 0);
}

/*
 * pmap_enter: enter a mapping into a pmap
 *
 * => must be done "now" ... no lazy-evaluation
 * => we set pmap => pv_head locking
 */
int
pmap_enter_ma(struct pmap *pmap, vaddr_t va, paddr_t ma, paddr_t pa,
	   vm_prot_t prot, u_int flags, int domid)
{
	pt_entry_t *ptes, opte, npte;
	pt_entry_t *ptep;
	pd_entry_t * const *pdes;
	struct vm_page *ptp, *pg;
	struct pmap_page *new_pp;
	struct pmap_page *old_pp;
	struct pv_entry *old_pve = NULL;
	struct pv_entry *new_pve;
	struct pv_entry *new_pve2;
	int error;
	bool wired = (flags & PMAP_WIRED) != 0;
	struct pmap *pmap2;

	KASSERT(pmap_initialized);
	KASSERT(curlwp->l_md.md_gc_pmap != pmap);
	KASSERT(va < VM_MAX_KERNEL_ADDRESS);
	KASSERTMSG(va != (vaddr_t)PDP_BASE,
	    "pmap_enter: trying to map over PDP!");
	KASSERTMSG(va < VM_MIN_KERNEL_ADDRESS ||
	    pmap_valid_entry(pmap->pm_pdir[pl_i(va, PTP_LEVELS)]),
	    "pmap_enter: missing kernel PTP for VA %lx!", va);

#ifdef XEN
	KASSERT(domid == DOMID_SELF || pa == 0);
#endif /* XEN */

	npte = ma | protection_codes[prot] | PG_V;
	npte |= pmap_pat_flags(flags);
	if (wired)
	        npte |= PG_W;
	if (va < VM_MAXUSER_ADDRESS)
		npte |= PG_u;
	else if (va < VM_MAX_ADDRESS)
		npte |= (PG_u | PG_RW);	/* XXXCDC: no longer needed? */
	else
		npte |= PG_k;
	if (pmap == pmap_kernel())
		npte |= pmap_pg_g;
	if (flags & VM_PROT_ALL) {
		npte |= PG_U;
		if (flags & VM_PROT_WRITE) {
			KASSERT((npte & PG_RW) != 0);
			npte |= PG_M;
		}
	}

#ifdef XEN
	if (domid != DOMID_SELF)
		pg = NULL;
	else
#endif
		pg = PHYS_TO_VM_PAGE(pa);
	if (pg != NULL) {
		/* This is a managed page */
		npte |= PG_PVLIST;
		new_pp = VM_PAGE_TO_PP(pg);
	} else {
		new_pp = NULL;
	}

	/* get pves. */
	new_pve = pool_cache_get(&pmap_pv_cache, PR_NOWAIT);
	new_pve2 = pool_cache_get(&pmap_pv_cache, PR_NOWAIT);
	if (new_pve == NULL || new_pve2 == NULL) {
		if (flags & PMAP_CANFAIL) {
			error = ENOMEM;
			goto out2;
		}
		panic("pmap_enter: pve allocation failed");
	}

	kpreempt_disable();
	pmap_map_ptes(pmap, &pmap2, &ptes, &pdes);	/* locks pmap */
	if (pmap == pmap_kernel()) {
		ptp = NULL;
	} else {
		ptp = pmap_get_ptp(pmap, va, pdes);
		if (ptp == NULL) {
			pmap_unmap_ptes(pmap, pmap2);
			if (flags & PMAP_CANFAIL) {
				error = ENOMEM;
				goto out;
			}
			panic("pmap_enter: get ptp failed");
		}
	}

	/*
	 * update the pte.
	 */

	ptep = &ptes[pl1_i(va)];
	do {
		opte = *ptep;

		/*
		 * if the same page, inherit PG_U and PG_M.
		 */
		if (((opte ^ npte) & (PG_FRAME | PG_V)) == 0) {
			npte |= opte & (PG_U | PG_M);
		}
#if defined(XEN)
		if (domid != DOMID_SELF) {
			/* pmap_pte_cas with error handling */
			int s = splvm();
			if (opte != *ptep) {
				splx(s);
				continue;
			}
			error = xpq_update_foreign(
			    vtomach((vaddr_t)ptep), npte, domid);
			splx(s);
			if (error) {
				if (ptp != NULL && ptp->wire_count <= 1) {
					pmap_free_ptp(pmap, ptp, va, ptes, pdes);
				}
				pmap_unmap_ptes(pmap, pmap2);
				goto out;
			}
			break;
		}
#endif /* defined(XEN) */
	} while (pmap_pte_cas(ptep, opte, npte) != opte);

	/*
	 * update statistics and PTP's reference count.
	 */

	pmap_stats_update_bypte(pmap, npte, opte);
	if (ptp != NULL && !pmap_valid_entry(opte)) {
		ptp->wire_count++;
	}
	KASSERT(ptp == NULL || ptp->wire_count > 1);

	/*
	 * if the same page, we can skip pv_entry handling.
	 */

	if (((opte ^ npte) & (PG_FRAME | PG_V)) == 0) {
		KASSERT(((opte ^ npte) & PG_PVLIST) == 0);
		goto same_pa;
	}

	/*
	 * if old page is managed, remove pv_entry from its list.
	 */

	if ((~opte & (PG_V | PG_PVLIST)) == 0) {
		pg = PHYS_TO_VM_PAGE(pmap_pte2pa(opte));

		KASSERTMSG(pg != NULL, "pmap_enter: PG_PVLIST mapping with "
		    "unmanaged page pa = 0x%" PRIx64 " (0x%" PRIx64 ")",
		    (int64_t)pa, (int64_t)atop(pa));

		KASSERT(uvm_page_locked_p(pg));

		old_pp = VM_PAGE_TO_PP(pg);
		old_pve = pmap_remove_pv(old_pp, ptp, va);
		old_pp->pp_attrs |= opte;
	}

	/*
	 * if new page is managed, insert pv_entry into its list.
	 */

	if (new_pp) {
		new_pve = pmap_enter_pv(new_pp, new_pve, &new_pve2, ptp, va);
	}

same_pa:
	pmap_unmap_ptes(pmap, pmap2);

	/*
	 * shootdown tlb if necessary.
	 */

	if ((~opte & (PG_V | PG_U)) == 0 &&
	    ((opte ^ npte) & (PG_FRAME | PG_RW)) != 0) {
		pmap_tlb_shootdown(pmap, va, opte, TLBSHOOT_ENTER);
	}

	error = 0;
out:
	kpreempt_enable();
out2:
	if (old_pve != NULL) {
		pool_cache_put(&pmap_pv_cache, old_pve);
	}
	if (new_pve != NULL) {
		pool_cache_put(&pmap_pv_cache, new_pve);
	}
	if (new_pve2 != NULL) {
		pool_cache_put(&pmap_pv_cache, new_pve2);
	}

	return error;
}

static bool
pmap_get_physpage(vaddr_t va, int level, paddr_t *paddrp)
{
	struct vm_page *ptp;
	struct pmap *kpm = pmap_kernel();

	if (!uvm.page_init_done) {

		/*
		 * we're growing the kernel pmap early (from
		 * uvm_pageboot_alloc()).  this case must be
		 * handled a little differently.
		 */

		if (!uvm_page_physget(paddrp))
			panic("pmap_get_physpage: out of memory");
#if defined(__HAVE_DIRECT_MAP)
		pagezero(PMAP_DIRECT_MAP(*paddrp));
#else
#if defined(XEN)
		if (XEN_VERSION_SUPPORTED(3, 4)) {
			xen_pagezero(*paddrp);
			return true;
		}
#endif
		kpreempt_disable();
		pmap_pte_set(early_zero_pte,
		    pmap_pa2pte(*paddrp) | PG_V | PG_RW | PG_k);
		pmap_pte_flush();
		pmap_update_pg((vaddr_t)early_zerop);
		memset(early_zerop, 0, PAGE_SIZE);
#if defined(DIAGNOSTIC)
		pmap_pte_set(early_zero_pte, 0);
		pmap_pte_flush();
#endif /* defined(DIAGNOSTIC) */
		kpreempt_enable();
#endif /* defined(__HAVE_DIRECT_MAP) */
	} else {
		/* XXX */
		ptp = uvm_pagealloc(NULL, 0, NULL,
				    UVM_PGA_USERESERVE|UVM_PGA_ZERO);
		if (ptp == NULL)
			panic("pmap_get_physpage: out of memory");
		ptp->flags &= ~PG_BUSY;
		ptp->wire_count = 1;
		*paddrp = VM_PAGE_TO_PHYS(ptp);
	}
	pmap_stats_update(kpm, 1, 0);
	return true;
}

/*
 * Allocate the amount of specified ptps for a ptp level, and populate
 * all levels below accordingly, mapping virtual addresses starting at
 * kva.
 *
 * Used by pmap_growkernel.
 */
static void
pmap_alloc_level(pd_entry_t * const *pdes, vaddr_t kva, int lvl,
    long *needed_ptps)
{
	unsigned long i;
	vaddr_t va;
	paddr_t pa;
	unsigned long index, endindex;
	int level;
	pd_entry_t *pdep;
#ifdef XEN
	int s = splvm(); /* protect xpq_* */
#endif

	for (level = lvl; level > 1; level--) {
		if (level == PTP_LEVELS)
			pdep = pmap_kernel()->pm_pdir;
		else
			pdep = pdes[level - 2];
		va = kva;
		index = pl_i_roundup(kva, level);
		endindex = index + needed_ptps[level - 1] - 1;


		for (i = index; i <= endindex; i++) {
			pt_entry_t pte;

			KASSERT(!pmap_valid_entry(pdep[i]));
			pmap_get_physpage(va, level - 1, &pa);
			pte = pmap_pa2pte(pa) | PG_k | PG_V | PG_RW;
#ifdef XEN
			pmap_pte_set(&pdep[i], pte);
#if defined(PAE) || defined(__x86_64__)
			if (level == PTP_LEVELS && i >= PDIR_SLOT_KERN) {
				if (__predict_true(
				    cpu_info_primary.ci_flags & CPUF_PRESENT)) {
					/* update per-cpu PMDs on all cpus */
					xen_kpm_sync(pmap_kernel(), i);
				} else {
					/*
					 * too early; update primary CPU
					 * PMD only (without locks)
					 */
#ifdef PAE
					pd_entry_t *cpu_pdep =
					    &cpu_info_primary.ci_kpm_pdir[l2tol2(i)];
#endif
#ifdef __x86_64__
					pd_entry_t *cpu_pdep =
						&cpu_info_primary.ci_kpm_pdir[i];
#endif
					pmap_pte_set(cpu_pdep, pte);
				}
			}
#endif /* PAE || __x86_64__ */
#else /* XEN */
			pdep[i] = pte;
#endif /* XEN */
			KASSERT(level != PTP_LEVELS || nkptp[level - 1] +
			    pl_i(VM_MIN_KERNEL_ADDRESS, level) == i);
			nkptp[level - 1]++;
			va += nbpd[level - 1];
		}
		pmap_pte_flush();
	}
#ifdef XEN
	splx(s);
#endif
}

/*
 * pmap_growkernel: increase usage of KVM space
 *
 * => we allocate new PTPs for the kernel and install them in all
 *	the pmaps on the system.
 */

vaddr_t
pmap_growkernel(vaddr_t maxkvaddr)
{
	struct pmap *kpm = pmap_kernel();
#if !defined(XEN) || !defined(__x86_64__)
	struct pmap *pm;
	long old;
#endif
	int s, i;
	long needed_kptp[PTP_LEVELS], target_nptp;
	bool invalidate = false;

	s = splvm();	/* to be safe */
	mutex_enter(kpm->pm_lock);

	if (maxkvaddr <= pmap_maxkvaddr) {
		mutex_exit(kpm->pm_lock);
		splx(s);
		return pmap_maxkvaddr;
	}

	maxkvaddr = x86_round_pdr(maxkvaddr);
#if !defined(XEN) || !defined(__x86_64__)
	old = nkptp[PTP_LEVELS - 1];
#endif

	/*
	 * This loop could be optimized more, but pmap_growkernel()
	 * is called infrequently.
	 */
	for (i = PTP_LEVELS - 1; i >= 1; i--) {
		target_nptp = pl_i_roundup(maxkvaddr, i + 1) -
		    pl_i_roundup(VM_MIN_KERNEL_ADDRESS, i + 1);
		/*
		 * XXX only need to check toplevel.
		 */
		if (target_nptp > nkptpmax[i])
			panic("out of KVA space");
		KASSERT(target_nptp >= nkptp[i]);
		needed_kptp[i] = target_nptp - nkptp[i];
	}

	pmap_alloc_level(normal_pdes, pmap_maxkvaddr, PTP_LEVELS, needed_kptp);

	/*
	 * If the number of top level entries changed, update all
	 * pmaps.
	 */
	if (needed_kptp[PTP_LEVELS - 1] != 0) {
#ifdef XEN
#ifdef __x86_64__
		/* nothing, kernel entries are never entered in user pmap */
#else /* __x86_64__ */
		mutex_enter(&pmaps_lock);
		LIST_FOREACH(pm, &pmaps, pm_list) {
			int pdkidx;
			for (pdkidx =  PDIR_SLOT_KERN + old;
			    pdkidx < PDIR_SLOT_KERN + nkptp[PTP_LEVELS - 1];
			    pdkidx++) {
				pmap_pte_set(&pm->pm_pdir[pdkidx],
				    kpm->pm_pdir[pdkidx]);
			}
			pmap_pte_flush();
		}
		mutex_exit(&pmaps_lock);
#endif /* __x86_64__ */
#else /* XEN */
		unsigned newpdes;
		newpdes = nkptp[PTP_LEVELS - 1] - old;
		mutex_enter(&pmaps_lock);
		LIST_FOREACH(pm, &pmaps, pm_list) {
			memcpy(&pm->pm_pdir[PDIR_SLOT_KERN + old],
			       &kpm->pm_pdir[PDIR_SLOT_KERN + old],
			       newpdes * sizeof (pd_entry_t));
		}
		mutex_exit(&pmaps_lock);
#endif
		invalidate = true;
	}
	pmap_maxkvaddr = maxkvaddr;
	mutex_exit(kpm->pm_lock);
	splx(s);

	if (invalidate && pmap_initialized) {
		/* Invalidate the PDP cache. */
		pool_cache_invalidate(&pmap_pdp_cache);
	}

	return maxkvaddr;
}

#ifdef DEBUG
void pmap_dump(struct pmap *, vaddr_t, vaddr_t);

/*
 * pmap_dump: dump all the mappings from a pmap
 *
 * => caller should not be holding any pmap locks
 */

void
pmap_dump(struct pmap *pmap, vaddr_t sva, vaddr_t eva)
{
	pt_entry_t *ptes, *pte;
	pd_entry_t * const *pdes;
	struct pmap *pmap2;
	vaddr_t blkendva;

	/*
	 * if end is out of range truncate.
	 * if (end == start) update to max.
	 */

	if (eva > VM_MAXUSER_ADDRESS || eva <= sva)
		eva = VM_MAXUSER_ADDRESS;

	/*
	 * we lock in the pmap => pv_head direction
	 */

	kpreempt_disable();
	pmap_map_ptes(pmap, &pmap2, &ptes, &pdes);	/* locks pmap */

	/*
	 * dumping a range of pages: we dump in PTP sized blocks (4MB)
	 */

	for (/* null */ ; sva < eva ; sva = blkendva) {

		/* determine range of block */
		blkendva = x86_round_pdr(sva+1);
		if (blkendva > eva)
			blkendva = eva;

		/* valid block? */
		if (!pmap_pdes_valid(sva, pdes, NULL))
			continue;

		pte = &ptes[pl1_i(sva)];
		for (/* null */; sva < blkendva ; sva += PAGE_SIZE, pte++) {
			if (!pmap_valid_entry(*pte))
				continue;
			printf("va %#" PRIxVADDR " -> pa %#" PRIxPADDR
			    " (pte=%#" PRIxPADDR ")\n",
			    sva, (paddr_t)pmap_pte2pa(*pte), (paddr_t)*pte);
		}
	}
	pmap_unmap_ptes(pmap, pmap2);
	kpreempt_enable();
}
#endif

/*
 * pmap_update: process deferred invalidations and frees.
 */

void
pmap_update(struct pmap *pmap)
{
	struct vm_page *empty_ptps;
	lwp_t *l = curlwp;

	/*
	 * If we have torn down this pmap, invalidate non-global TLB
	 * entries on any processors using it.
	 */
	KPREEMPT_DISABLE(l);
	if (__predict_false(l->l_md.md_gc_pmap == pmap)) {
		l->l_md.md_gc_pmap = NULL;
		pmap_tlb_shootdown(pmap, (vaddr_t)-1LL, 0, TLBSHOOT_UPDATE);
	}
	/*
	 * Initiate any pending TLB shootdowns.  Wait for them to
	 * complete before returning control to the caller.
	 */
	pmap_tlb_shootnow();
	KPREEMPT_ENABLE(l);

	/*
	 * Now that shootdowns are complete, process deferred frees,
	 * but not from interrupt context.
	 */
	if (l->l_md.md_gc_ptp != NULL) {
		KASSERT((l->l_pflag & LP_INTR) == 0);
		if (cpu_intr_p()) {
			return;
		}
		empty_ptps = l->l_md.md_gc_ptp;
		l->l_md.md_gc_ptp = NULL;
		pmap_free_ptps(empty_ptps);
	}
}

#if PTP_LEVELS > 4
#error "Unsupported number of page table mappings"
#endif

paddr_t
pmap_init_tmp_pgtbl(paddr_t pg)
{
	static bool maps_loaded;
	static const paddr_t x86_tmp_pml_paddr[] = {
	    4 * PAGE_SIZE,
	    5 * PAGE_SIZE,
	    6 * PAGE_SIZE,
	    7 * PAGE_SIZE
	};
	static vaddr_t x86_tmp_pml_vaddr[] = { 0, 0, 0, 0 };

	pd_entry_t *tmp_pml, *kernel_pml;
	
	int level;

	if (!maps_loaded) {
		for (level = 0; level < PTP_LEVELS; ++level) {
			x86_tmp_pml_vaddr[level] =
			    uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
			    UVM_KMF_VAONLY);

			if (x86_tmp_pml_vaddr[level] == 0)
				panic("mapping of real mode PML failed\n");
			pmap_kenter_pa(x86_tmp_pml_vaddr[level],
			    x86_tmp_pml_paddr[level],
			    VM_PROT_READ | VM_PROT_WRITE, 0);
			pmap_update(pmap_kernel());
		}
		maps_loaded = true;
	}

	/* Zero levels 1-3 */
	for (level = 0; level < PTP_LEVELS - 1; ++level) {
		tmp_pml = (void *)x86_tmp_pml_vaddr[level];
		memset(tmp_pml, 0, PAGE_SIZE);
	}

	/* Copy PML4 */
	kernel_pml = pmap_kernel()->pm_pdir;
	tmp_pml = (void *)x86_tmp_pml_vaddr[PTP_LEVELS - 1];
	memcpy(tmp_pml, kernel_pml, PAGE_SIZE);

#ifdef PAE
	/*
	 * Use the last 4 entries of the L2 page as L3 PD entries. These
	 * last entries are unlikely to be used for temporary mappings.
	 * 508: maps 0->1GB (userland)
	 * 509: unused
	 * 510: unused
	 * 511: maps 3->4GB (kernel)
	 */
	tmp_pml[508] = x86_tmp_pml_paddr[PTP_LEVELS - 1] | PG_V;
	tmp_pml[509] = 0;
	tmp_pml[510] = 0;
	tmp_pml[511] = pmap_pdirpa(pmap_kernel(), PDIR_SLOT_KERN) | PG_V;
#endif

	for (level = PTP_LEVELS - 1; level > 0; --level) {
		tmp_pml = (void *)x86_tmp_pml_vaddr[level];

		tmp_pml[pl_i(pg, level + 1)] =
		    (x86_tmp_pml_paddr[level - 1] & PG_FRAME) | PG_RW | PG_V;
	}

	tmp_pml = (void *)x86_tmp_pml_vaddr[0];
	tmp_pml[pl_i(pg, 1)] = (pg & PG_FRAME) | PG_RW | PG_V;

#ifdef PAE
	/* Return the PA of the L3 page (entry 508 of the L2 page) */
	return x86_tmp_pml_paddr[PTP_LEVELS - 1] + 508 * sizeof(pd_entry_t);
#endif

	return x86_tmp_pml_paddr[PTP_LEVELS - 1];
}

u_int
x86_mmap_flags(paddr_t mdpgno)
{
	u_int nflag = (mdpgno >> X86_MMAP_FLAG_SHIFT) & X86_MMAP_FLAG_MASK;
	u_int pflag = 0;

	if (nflag & X86_MMAP_FLAG_PREFETCH)
		pflag |= PMAP_WRITE_COMBINE;

	return pflag;
}
