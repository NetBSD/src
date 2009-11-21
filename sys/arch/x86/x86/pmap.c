/*	$NetBSD: pmap.c,v 1.97 2009/11/21 03:11:01 rmind Exp $	*/

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
 *
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Charles D. Cranor and
 *      Washington University.
 * 4. The name of the author may not be used to endorse or promote products
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
 * Chuck Cranor <chuck@ccrc.wustl.edu>
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
__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.97 2009/11/21 03:11:01 rmind Exp $");

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

#include <uvm/uvm.h>

#include <dev/isa/isareg.h>

#include <machine/specialreg.h>
#include <machine/gdt.h>
#include <machine/isa_machdep.h>
#include <machine/cpuvar.h>

#include <x86/pmap.h>
#include <x86/pmap_pv.h>

#include <x86/i82489reg.h>
#include <x86/i82489var.h>

#ifdef XEN
#include <xen/xen3-public/xen.h>
#include <xen/hypervisor.h>
#endif

/* flag to be used for kernel mappings: PG_u on Xen/amd64, 0 otherwise */
#if defined(XEN) && defined(__x86_64__)
#define PG_k PG_u
#else
#define PG_k 0
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
 *
 * tlb shootdown
 *
 * tlb shootdowns are hard interrupts that operate outside the spl
 * framework: they don't need to be blocked provided that the pmap module
 * gets the order of events correct.  the calls are made by talking directly
 * to the lapic.  the stubs to handle the interrupts are quite short and do
 * one of the following: invalidate a single page, a range of pages, all
 * user tlb entries or the entire tlb.
 * 
 * the cpus synchronize with each other using pmap_mbox structures which are
 * aligned on 64-byte cache lines.  tlb shootdowns against the kernel pmap
 * use a global mailbox and are generated using a broadcast ipi (broadcast
 * to all but the sending cpu).  shootdowns against regular pmaps use
 * per-cpu mailboxes and are multicast.  kernel and user shootdowns can
 * execute simultaneously, as can shootdowns within different multithreaded
 * processes.  TODO:
 * 
 *   1. figure out which waitpoints can be deferered to pmap_update().
 *   2. see if there is a cheap way to batch some updates.
 */

const vaddr_t ptp_masks[] = PTP_MASK_INITIALIZER;
const int ptp_shifts[] = PTP_SHIFT_INITIALIZER;
const long nkptpmax[] = NKPTPMAX_INITIALIZER;
const long nbpd[] = NBPD_INITIALIZER;
pd_entry_t * const normal_pdes[] = PDES_INITIALIZER;
pd_entry_t * const alternate_pdes[] = APDES_INITIALIZER;

long nkptp[] = NKPTP_INITIALIZER;

static kmutex_t pmaps_lock;

static vaddr_t pmap_maxkvaddr;

#define COUNT(x)	/* nothing */

/*
 * XXX kludge: dummy locking to make KASSERTs in uvm_page.c comfortable.
 * actual locking is done by pm_lock.
 */
#if defined(DIAGNOSTIC)
#define	PMAP_SUBOBJ_LOCK(pm, idx) \
	KASSERT(mutex_owned(&(pm)->pm_lock)); \
	if ((idx) != 0) \
		mutex_enter(&(pm)->pm_obj[(idx)].vmobjlock)
#define	PMAP_SUBOBJ_UNLOCK(pm, idx) \
	KASSERT(mutex_owned(&(pm)->pm_lock)); \
	if ((idx) != 0) \
		mutex_exit(&(pm)->pm_obj[(idx)].vmobjlock)
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
 * Global TLB shootdown mailbox.
 */
struct evcnt pmap_tlb_evcnt __aligned(64);
struct pmap_mbox pmap_mbox __aligned(64);

/*
 * Per-CPU data.  The pmap mailbox is cache intensive so gets its
 * own line.  Note that the mailbox must be the first item.
 */
struct pmap_cpu {
	/* TLB shootdown */
	struct pmap_mbox pc_mbox;
};

union {
	struct pmap_cpu pc;
	uint8_t padding[64];
} pmap_cpu[MAXCPUS] __aligned(64);

/*
 * global data structures
 */

static struct pmap kernel_pmap_store;	/* the kernel's pmap (proc0) */
struct pmap *const kernel_pmap_ptr = &kernel_pmap_store;

/*
 * pmap_pg_g: if our processor supports PG_G in the PTE then we
 * set pmap_pg_g to PG_G (otherwise it is zero).
 */

int pmap_pg_g = 0;

/*
 * pmap_largepages: if our processor supports PG_PS and we are
 * using it, this is set to true.
 */

int pmap_largepages;

/*
 * i386 physical memory comes in a big contig chunk with a small
 * hole toward the front of it...  the following two paddr_t's
 * (shared with machdep.c) describe the physical address space
 * of this machine.
 */
paddr_t avail_start;	/* PA of first available physical page */
paddr_t avail_end;	/* PA of last available physical page */

#ifdef XEN
#ifdef __x86_64__
/* Dummy PGD for user cr3, used between pmap_deacivate() and pmap_activate() */
static paddr_t xen_dummy_user_pgd;
/* Currently active user PGD (can't use rcr3()) */
static paddr_t xen_current_user_pgd = 0;
#endif /* __x86_64__ */
paddr_t pmap_pa_start; /* PA of first physical page for this domain */
paddr_t pmap_pa_end;   /* PA of last physical page for this domain */
#endif /* XEN */

#define	VM_PAGE_TO_PP(pg)	(&(pg)->mdpage.mp_pp)

#define	pp_lock(pp)	mutex_spin_enter(&(pp)->pp_lock)
#define	pp_unlock(pp)	mutex_spin_exit(&(pp)->pp_lock)
#define	pp_locked(pp)	mutex_owned(&(pp)->pp_lock)

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

static pt_entry_t protection_codes[8];	/* maps MI prot to i386 prot code */
static bool pmap_initialized = false;	/* pmap_init done yet? */

/*
 * the following two vaddr_t's are used during system startup
 * to keep track of how much of the kernel's VM space we have used.
 * once the system is started, the management of the remaining kernel
 * VM space is turned over to the kernel_map vm_map.
 */

static vaddr_t virtual_avail;	/* VA of first free KVA */
static vaddr_t virtual_end;	/* VA of last free KVA */

/*
 * linked list of all non-kernel pmaps
 */

static struct pmap_head pmaps;

/*
 * pool that pmap structures are allocated from
 */

static struct pool_cache pmap_cache;

/*
 * pv_entry cache
 */

static struct pool_cache pmap_pv_cache;

/*
 * MULTIPROCESSOR: special VA's/ PTE's are actually allocated inside a
 * maxcpus*NPTECL array of PTE's, to avoid cache line thrashing
 * due to false sharing.
 */

#ifdef MULTIPROCESSOR
#define PTESLEW(pte, id) ((pte)+(id)*NPTECL)
#define VASLEW(va,id) ((va)+(id)*NPTECL*PAGE_SIZE)
#else
#define PTESLEW(pte, id) (pte)
#define VASLEW(va,id) (va)
#endif

/*
 * special VAs and the PTEs that map them
 */
static pt_entry_t *csrc_pte, *cdst_pte, *zero_pte, *ptp_pte, *early_zero_pte;
static char *csrcp, *cdstp, *zerop, *ptpp, *early_zerop;

/*
 * pool and cache that PDPs are allocated from
 */

static struct pool_cache pmap_pdp_cache;
int	pmap_pdp_ctor(void *, void *, int);
void	pmap_pdp_dtor(void *, void *);
#ifdef PAE
/* need to allocate items of 4 pages */
void *pmap_pdp_alloc(struct pool *, int);
void pmap_pdp_free(struct pool *, void *);
static struct pool_allocator pmap_pdp_allocator = {
	.pa_alloc = pmap_pdp_alloc,
	.pa_free = pmap_pdp_free,
	.pa_pagesz = PAGE_SIZE * PDP_SIZE,
};
#endif /* PAE */

void *vmmap; /* XXX: used by mem.c... it should really uvm_map_reserve it */

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
static bool		 pmap_is_curpmap(struct pmap *);
static bool		 pmap_is_active(struct pmap *, struct cpu_info *, bool);
static void		 pmap_map_ptes(struct pmap *, struct pmap **,
				       pt_entry_t **, pd_entry_t * const **);
static bool		 pmap_remove_pte(struct pmap *, struct vm_page *,
					 pt_entry_t *, vaddr_t,
					 struct pv_entry **);
static pt_entry_t	 pmap_remove_ptes(struct pmap *, struct vm_page *,
					  vaddr_t, vaddr_t, vaddr_t,
					  struct pv_entry **);

static void		 pmap_unmap_ptes(struct pmap *, struct pmap *);
static bool		 pmap_get_physpage(vaddr_t, int, paddr_t *);
static int		 pmap_pdes_invalid(vaddr_t, pd_entry_t * const *,
					   pd_entry_t *);
#define	pmap_pdes_valid(va, pdes, lastpde)	\
	(pmap_pdes_invalid((va), (pdes), (lastpde)) == 0)
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
		KASSERT(mutex_owned(&pmap->pm_lock));
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

	KASSERT(pp_locked(pp));
	if ((pp->pp_flags & PP_EMBEDDED) != 0) {
		return &pp->pp_pte;
	}
	return pve_to_pvpte(LIST_FIRST(&pp->pp_head.pvh_list));
}

static struct pv_pte *
pv_pte_next(struct pmap_page *pp, struct pv_pte *pvpte)
{

	KASSERT(pvpte != NULL);
	KASSERT(pp_locked(pp));
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

inline static bool
pmap_is_curpmap(struct pmap *pmap)
{
#if defined(XEN) && defined(__x86_64__)
	/*
	 * Only kernel pmap is physically loaded.
	 * User PGD may be active, but TLB will be flushed
	 * with HYPERVISOR_iret anyway, so let's say no
	 */
	return(pmap == pmap_kernel());
#else /* XEN && __x86_64__*/
	return((pmap == pmap_kernel()) ||
	       (pmap == curcpu()->ci_pmap));
#endif
}

/*
 * pmap_is_active: is this pmap loaded into the specified processor's %cr3?
 */

inline static bool
pmap_is_active(struct pmap *pmap, struct cpu_info *ci, bool kernel)
{

	return (pmap == pmap_kernel() ||
	    (pmap->pm_cpus & ci->ci_cpumask) != 0 ||
	    (kernel && (pmap->pm_kernel_cpus & ci->ci_cpumask) != 0));
}

static void
pmap_apte_flush(struct pmap *pmap)
{

	KASSERT(kpreempt_disabled());

	/*
	 * Flush the APTE mapping from all other CPUs that
	 * are using the pmap we are using (who's APTE space
	 * is the one we've just modified).
	 *
	 * XXXthorpej -- find a way to defer the IPI.
	 */
	pmap_tlb_shootdown(pmap, (vaddr_t)-1LL, 0, 0);
	pmap_tlb_shootwait();
}

/*
 *	Add a reference to the specified pmap.
 */

inline void
pmap_reference(struct pmap *pmap)
{

	atomic_inc_uint(&pmap->pm_obj[0].uo_refs);
}

/*
 * pmap_map_ptes: map a pmap's PTEs into KVM and lock them in
 *
 * => we lock enough pmaps to keep things locked in
 * => must be undone with pmap_unmap_ptes before returning
 */

static void
pmap_map_ptes(struct pmap *pmap, struct pmap **pmap2,
    pd_entry_t **ptepp, pd_entry_t * const **pdeppp)
{
	pd_entry_t opde, npde;
	struct pmap *ourpmap;
	struct cpu_info *ci;
	struct lwp *l;
	bool iscurrent;
	uint64_t ncsw;
#ifdef XEN
	int s;
#endif

	/* the kernel's pmap is always accessible */
	if (pmap == pmap_kernel()) {
		*pmap2 = NULL;
		*ptepp = PTE_BASE;
		*pdeppp = normal_pdes;
		return;
	}
	KASSERT(kpreempt_disabled());

 retry:
	l = curlwp;
	ncsw = l->l_ncsw;
 	ourpmap = NULL;
	ci = curcpu();
#if defined(XEN) && defined(__x86_64__)
	/*
	 * curmap can only be pmap_kernel so at this point
	 * pmap_is_curpmap is always false
	 */
	iscurrent = 0;
	ourpmap = pmap_kernel();
#else /* XEN && __x86_64__*/
	if (ci->ci_want_pmapload &&
	    vm_map_pmap(&l->l_proc->p_vmspace->vm_map) == pmap) {
		pmap_load();
		if (l->l_ncsw != ncsw)
			goto retry;
	}
	iscurrent = pmap_is_curpmap(pmap);
	/* if curpmap then we are always mapped */
	if (iscurrent) {
		mutex_enter(&pmap->pm_lock);
		*pmap2 = NULL;
		*ptepp = PTE_BASE;
		*pdeppp = normal_pdes;
		goto out;
	}
	ourpmap = ci->ci_pmap;
#endif /* XEN && __x86_64__ */

	/* need to lock both curpmap and pmap: use ordered locking */
	pmap_reference(ourpmap);
	if ((uintptr_t) pmap < (uintptr_t) ourpmap) {
		mutex_enter(&pmap->pm_lock);
		mutex_enter(&ourpmap->pm_lock);
	} else {
		mutex_enter(&ourpmap->pm_lock);
		mutex_enter(&pmap->pm_lock);
	}

	if (l->l_ncsw != ncsw)
		goto unlock_and_retry;

	/* need to load a new alternate pt space into curpmap? */
	COUNT(apdp_pde_map);
	opde = *APDP_PDE;
#ifdef XEN
	if (!pmap_valid_entry(opde) ||
	    pmap_pte2pa(opde) != pmap_pdirpa(pmap, 0)) {
		int i;
		s = splvm();
		/* Make recursive entry usable in user PGD */
		for (i = 0; i < PDP_SIZE; i++) {
			npde = pmap_pa2pte(
			    pmap_pdirpa(pmap, i * NPDPG)) | PG_k | PG_V;
			xpq_queue_pte_update(
			    xpmap_ptom(pmap_pdirpa(pmap, PDIR_SLOT_PTE + i)),
			    npde);
			xpq_queue_pte_update(xpmap_ptetomach(&APDP_PDE[i]),
			    npde);
#ifdef PAE
			/* update shadow entry too */
			xpq_queue_pte_update(
			    xpmap_ptetomach(&APDP_PDE_SHADOW[i]), npde);
#endif /* PAE */
			xpq_queue_invlpg(
			    (vaddr_t)&pmap->pm_pdir[PDIR_SLOT_PTE + i]);
		}
		xpq_flush_queue();
		if (pmap_valid_entry(opde))
			pmap_apte_flush(ourpmap);
		splx(s);
	}
#else /* XEN */
	npde = pmap_pa2pte(pmap_pdirpa(pmap, 0)) | PG_RW | PG_V;
	if (!pmap_valid_entry(opde) ||
	    pmap_pte2pa(opde) != pmap_pdirpa(pmap, 0)) {
		pmap_pte_set(APDP_PDE, npde);
		pmap_pte_flush();
		if (pmap_valid_entry(opde))
			pmap_apte_flush(ourpmap);
	}
#endif /* XEN */
	*pmap2 = ourpmap;
	*ptepp = APTE_BASE;
	*pdeppp = alternate_pdes;
	KASSERT(l->l_ncsw == ncsw);
#if !defined(XEN) || !defined(__x86_64__)
 out:
#endif
 	/*
 	 * might have blocked, need to retry?
 	 */
	if (l->l_ncsw != ncsw) {
 unlock_and_retry:
	    	if (ourpmap != NULL) {
			mutex_exit(&ourpmap->pm_lock);
			pmap_destroy(ourpmap);
		}
		mutex_exit(&pmap->pm_lock);
		goto retry;
	}

	return;
}

/*
 * pmap_unmap_ptes: unlock the PTE mapping of "pmap"
 */

static void
pmap_unmap_ptes(struct pmap *pmap, struct pmap *pmap2)
{

	if (pmap == pmap_kernel()) {
		return;
	}
	KASSERT(kpreempt_disabled());
	if (pmap2 == NULL) {
		mutex_exit(&pmap->pm_lock);
	} else {
#if defined(XEN) && defined(__x86_64__)
		KASSERT(pmap2 == pmap_kernel());
#else
		KASSERT(curcpu()->ci_pmap == pmap2);
#endif
#if defined(MULTIPROCESSOR)
		pmap_pte_set(APDP_PDE, 0);
		pmap_pte_flush();
		pmap_apte_flush(pmap2);
#endif
		COUNT(apdp_pde_unmap);
		mutex_exit(&pmap->pm_lock);
		mutex_exit(&pmap2->pm_lock);
		pmap_destroy(pmap2);
	}
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
		printk("pmap_kenter_pa: pa 0x%" PRIx64 " for va 0x%" PRIx64
		    " outside range\n", (int64_t)pa, (int64_t)va);
#endif /* DEBUG */
		npte = pa;
	} else
#endif /* DOM0OPS */
		npte = pmap_pa2pte(pa);
	npte |= protection_codes[prot] | PG_k | PG_V | pmap_pg_g;
	if (flags & PMAP_NOCACHE)
		npte |= PG_N;
	opte = pmap_pte_testset(pte, npte); /* zap! */
#if defined(DIAGNOSTIC)
	/* XXX For now... */
	if (opte & PG_PS)
		panic("pmap_kenter_pa: PG_PS");
#endif
	if ((opte & (PG_V | PG_U)) == (PG_V | PG_U)) {
		/* This should not happen, so no need to batch updates. */
		kpreempt_disable();
		pmap_tlb_shootdown(pmap_kernel(), va, 0, opte);
		kpreempt_enable();
	}
}

void
pmap_emap_enter(vaddr_t va, paddr_t pa, vm_prot_t prot)
{
	pt_entry_t *pte, opte, npte;

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
	opte = pmap_pte_testset(pte, npte);
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
			const uint32_t cpumask = ci->ci_cpumask;
			atomic_and_32(&pmap->pm_cpus, ~cpumask);
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
	pt_entry_t *pte, xpte;
	vaddr_t va, eva = sva + len;

	for (va = sva; va < eva; va += PAGE_SIZE) {
		pte = (va < VM_MIN_KERNEL_ADDRESS) ? vtopte(va) : kvtopte(va);
		xpte |= pmap_pte_testset(pte, 0);
	}
}

#ifdef XEN
/*
 * pmap_kenter_ma: enter a kernel mapping without R/M (pv_entry) tracking
 *
 * => no need to lock anything, assume va is already allocated
 * => should be faster than normal pmap enter function
 * => we expect a MACHINE address
 */

void
pmap_kenter_ma(vaddr_t va, paddr_t ma, vm_prot_t prot, u_int flags)
{
	pt_entry_t *pte, opte, npte;

	if (va < VM_MIN_KERNEL_ADDRESS)
		pte = vtopte(va);
	else
		pte = kvtopte(va);

	npte = ma | ((prot & VM_PROT_WRITE) ? PG_RW : PG_RO) |
	     PG_V | PG_k;
	if (flags & PMAP_NOCACHE)
		npte |= PG_N;

#ifndef XEN
	if ((cpu_feature & CPUID_NOX) && !(prot & VM_PROT_EXECUTE))
		npte |= PG_NX;
#endif
	opte = pmap_pte_testset (pte, npte); /* zap! */

	if (pmap_valid_entry(opte)) {
#if defined(MULTIPROCESSOR)
		kpreempt_disable();
		pmap_tlb_shootdown(pmap_kernel(), va, 0, opte);
		kpreempt_enable();
#else
		/* Don't bother deferring in the single CPU case. */
		pmap_update_pg(va);
#endif
	}
}
#endif	/* XEN */

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

void
pmap_kremove(vaddr_t sva, vsize_t len)
{
	pt_entry_t *pte, xpte;
	vaddr_t va, eva;

	eva = sva + len;
	xpte = 0;

	for (va = sva; va < eva; va += PAGE_SIZE) {
		if (va < VM_MIN_KERNEL_ADDRESS)
			pte = vtopte(va);
		else
			pte = kvtopte(va);
		xpte |= pmap_pte_testset(pte, 0); /* zap! */
#if defined(DIAGNOSTIC)
		/* XXX For now... */
		if (xpte & PG_PS)
			panic("pmap_kremove: PG_PS");
		if (xpte & PG_PVLIST)
			panic("pmap_kremove: PG_PVLIST mapping for 0x%lx",
			      va);
#endif
	}
	if ((xpte & (PG_V | PG_U)) == (PG_V | PG_U)) {
		kpreempt_disable();
		pmap_tlb_shootdown(pmap_kernel(), sva, eva, xpte);
		kpreempt_enable();
	}
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
	struct pcb *pcb;
	int i;
	vaddr_t kva;
#ifdef XEN
	pt_entry_t pg_nx = 0;
#else
	unsigned long p1i;
	vaddr_t kva_end;
	pt_entry_t pg_nx = (cpu_feature & CPUID_NOX ? PG_NX : 0);
#endif

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
		UVM_OBJ_INIT(&kpm->pm_obj[i], NULL, 1);
		kpm->pm_ptphint[i] = NULL;
	}
	memset(&kpm->pm_list, 0, sizeof(kpm->pm_list));  /* pm_list not used */
	pcb = lwp_getpcb(&lwp0);
	kpm->pm_pdir = (pd_entry_t *)(pcb->pcb_cr3 + KERNBASE);
#ifdef PAE
	for (i = 0; i < PDP_SIZE; i++)
		kpm->pm_pdirpa[i] = (paddr_t)pcb->pcb_cr3 + PAGE_SIZE * i;
#else
	kpm->pm_pdirpa = (paddr_t)pcb->pcb_cr3;
#endif
	kpm->pm_stats.wired_count = kpm->pm_stats.resident_count =
		x86_btop(kva_start - VM_MIN_KERNEL_ADDRESS);

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

	if (cpu_feature & CPUID_PGE) {
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

	if (cpu_feature & CPUID_PSE) {
		paddr_t pa;
		pd_entry_t *pde;
		extern char __data_start;

		lcr4(rcr4() | CR4_PSE);	/* enable hardware (via %cr4) */
		pmap_largepages = 1;	/* enable software */

		/*
		 * the TLB must be flushed after enabling large pages
		 * on Pentium CPUs, according to section 3.6.2.2 of
		 * "Intel Architecture Software Developer's Manual,
		 * Volume 3: System Programming".
		 */
		tlbflush();

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
			tlbflush();
		}
#if defined(DEBUG)
		printf("kernel text is mapped with "
		    "%lu large pages and %lu normal pages\n",
		    (unsigned long)howmany(kva - KERNBASE, NBPD_L2),
		    (unsigned long)howmany((vaddr_t)&__data_start - kva,
		    NBPD_L1));
#endif /* defined(DEBUG) */
	}
#endif /* !XEN */

	if (VM_MIN_KERNEL_ADDRESS != KERNBASE) {
		/*
		 * zero_pte is stuck at the end of mapped space for the kernel
		 * image (disjunct from kva space). This is done so that it
		 * can safely be used in pmap_growkernel (pmap_get_physpage),
		 * when it's called for the first time.
		 * XXXfvdl fix this for MULTIPROCESSOR later.
		 */

		early_zerop = (void *)(KERNBASE + NKL2_KIMG_ENTRIES * NBPD_L2);
		early_zero_pte = PTE_BASE + pl1_i((unsigned long)early_zerop);
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

	/*
	 * Nothing after this point actually needs pte;
	 */
	pte = (void *)0xdeadbeef;

	/* XXX: vmmap used by mem.c... should be uvm_map_reserve */
	/* XXXfvdl PTEs not needed here */
	vmmap = (char *)virtual_avail;			/* don't need pte */
	virtual_avail += PAGE_SIZE; pte++;

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
	xpq_queue_pin_table(xpmap_ptom_masked(xen_dummy_user_pgd));
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
	virtual_avail += 2 * PAGE_SIZE; pte += 2;
	avail_start += 2 * PAGE_SIZE;
#else /* defined(__x86_64__) */
	virtual_avail += PAGE_SIZE; pte++;
	avail_start += PAGE_SIZE;
	/* pentium f00f bug stuff */
	pentium_idt_vaddr = virtual_avail;		/* don't need pte */
	virtual_avail += PAGE_SIZE; pte++;
#endif /* defined(__x86_64__) */
#endif /* XEN */

#ifdef _LP64
	/*
	 * Grab a page below 4G for things that need it (i.e.
	 * having an initial %cr3 for the MP trampoline).
	 */
	lo32_vaddr = virtual_avail;
	virtual_avail += PAGE_SIZE; pte++;
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
	pmap_cpu_init_early(curcpu());

	/*
	 * initialize caches.
	 */

	pool_cache_bootstrap(&pmap_cache, sizeof(struct pmap), 0, 0, 0,
	    "pmappl", NULL, IPL_NONE, NULL, NULL, NULL);
#ifdef PAE
	pool_cache_bootstrap(&pmap_pdp_cache, PAGE_SIZE * PDP_SIZE, 0, 0, 0,
	    "pdppl", &pmap_pdp_allocator, IPL_NONE,
	    pmap_pdp_ctor, pmap_pdp_dtor, NULL);
#else /* PAE */
	pool_cache_bootstrap(&pmap_pdp_cache, PAGE_SIZE, 0, 0, 0,
	    "pdppl", NULL, IPL_NONE, pmap_pdp_ctor, pmap_pdp_dtor, NULL);
#endif /* PAE */
	pool_cache_bootstrap(&pmap_pv_cache, sizeof(struct pv_entry), 0, 0,
	    PR_LARGECACHE, "pvpl", &pool_allocator_meta, IPL_NONE, NULL,
	    NULL, NULL);

	/*
	 * ensure the TLB is sync'd with reality by flushing it...
	 */

	tlbflush();

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
#ifdef XEN
	int level;
	paddr_t newp;
	paddr_t pdes_pa;

	pdes_pa = pmap_kernel()->pm_pdirpa;
	level = PTP_LEVELS;
	for (;;) {
		newp = avail_start;
		avail_start += PAGE_SIZE;
		HYPERVISOR_update_va_mapping ((vaddr_t)early_zerop,
		    xpmap_ptom_masked(newp) | PG_u | PG_V | PG_RW, UVMF_INVLPG);
		memset((void *)early_zerop, 0, PAGE_SIZE);
		/* Mark R/O before installing */
		HYPERVISOR_update_va_mapping ((vaddr_t)early_zerop,
		    xpmap_ptom_masked(newp) | PG_u | PG_V, UVMF_INVLPG);
		if (newp < (NKL2_KIMG_ENTRIES * NBPD_L2))
			HYPERVISOR_update_va_mapping (newp + KERNBASE,
			    xpmap_ptom_masked(newp) | PG_u | PG_V, UVMF_INVLPG);
		xpq_queue_pte_update (
			xpmap_ptom_masked(pdes_pa)
			+ (pl_i(0, level) * sizeof (pd_entry_t)),
			xpmap_ptom_masked(newp) | PG_RW | PG_u | PG_V);
		level--;
		if (level <= 1)
			break;
		pdes_pa = newp;
	}
#else /* XEN */
	pd_entry_t *pdes;
	int level;
	paddr_t newp;

	pdes = pmap_kernel()->pm_pdir;
	level = PTP_LEVELS;
	for (;;) {
		newp = avail_start;
		avail_start += PAGE_SIZE;
		*early_zero_pte = (newp & PG_FRAME) | PG_V | PG_RW;
		pmap_update_pg((vaddr_t)early_zerop);
		memset(early_zerop, 0, PAGE_SIZE);
		pdes[pl_i(0, level)] = (newp & PG_FRAME) | PG_V | PG_RW;
		level--;
		if (level <= 1)
			break;
		pdes = normal_pdes[level - 2];
	}
#endif /* XEN */
}
#endif /* defined(__x86_64__) */

/*
 * pmap_init: called from uvm_init, our job is to get the pmap
 * system ready to manage mappings...
 */

void
pmap_init(void)
{
	int i;

	for (i = 0; i < PV_HASH_SIZE; i++) {
		SLIST_INIT(&pv_hash_heads[i].hh_list);
	}
	for (i = 0; i < PV_HASH_LOCK_CNT; i++) {
		mutex_init(&pv_hash_locks[i].lock, MUTEX_NODEBUG, IPL_VM);
	}

	/*
	 * done: pmap module is up (and ready for business)
	 */

	pmap_initialized = true;
}

/*
 * pmap_cpu_init_early: perform early per-CPU initialization.
 */

void
pmap_cpu_init_early(struct cpu_info *ci)
{
	struct pmap_cpu *pc;
	static uint8_t pmap_cpu_alloc;

	pc = &pmap_cpu[pmap_cpu_alloc++].pc;
	ci->ci_pmap_cpu = pc;
}

/*
 * pmap_cpu_init_late: perform late per-CPU initialization.
 */

void
pmap_cpu_init_late(struct cpu_info *ci)
{

	if (ci == &cpu_info_primary) {
		evcnt_attach_dynamic(&pmap_tlb_evcnt, EVCNT_TYPE_INTR,
		    NULL, "global", "TLB IPI");
		evcnt_attach_dynamic(&pmap_iobmp_evcnt, EVCNT_TYPE_MISC,
		    NULL, "x86", "io bitmap copy");
		evcnt_attach_dynamic(&pmap_ldt_evcnt, EVCNT_TYPE_MISC,
		    NULL, "x86", "ldt sync");
	}

	evcnt_attach_dynamic(&ci->ci_tlb_evcnt, EVCNT_TYPE_MISC,
	    NULL, device_xname(ci->ci_dev), "TLB IPI");
}

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

	KASSERT(pp_locked(pp));

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
 * => caller should have the pp_lock locked
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
	KASSERT(pp_locked(pp));

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
 * => caller should hold pp_lock [so that attrs can be adjusted]
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
	KASSERT(pp_locked(pp));

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

	KASSERT(mutex_owned(&pmap->pm_lock));

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
	int lidx;
	struct uvm_object *obj;

	KASSERT(ptp->wire_count == 1);

	lidx = level - 1;

	obj = &pmap->pm_obj[lidx];
	pmap_stats_update(pmap, -1, 0);
	if (lidx != 0)
		mutex_enter(&obj->vmobjlock);
	if (pmap->pm_ptphint[lidx] == ptp)
		pmap->pm_ptphint[lidx] = TAILQ_FIRST(&obj->memq);
	ptp->wire_count = 0;
	uvm_pagerealloc(ptp, NULL, 0);
	VM_PAGE_TO_PP(ptp)->pp_link = curlwp->l_md.md_gc_ptp;
	curlwp->l_md.md_gc_ptp = ptp;
	if (lidx != 0)
		mutex_exit(&obj->vmobjlock);
}

static void
pmap_free_ptp(struct pmap *pmap, struct vm_page *ptp, vaddr_t va,
	      pt_entry_t *ptes, pd_entry_t * const *pdes)
{
	unsigned long index;
	int level;
	vaddr_t invaladdr;
#ifdef MULTIPROCESSOR
	vaddr_t invaladdr2;
#endif
	pd_entry_t opde;
	struct pmap *curpmap = vm_map_pmap(&curlwp->l_proc->p_vmspace->vm_map);

	KASSERT(pmap != pmap_kernel());
	KASSERT(mutex_owned(&pmap->pm_lock));
	KASSERT(kpreempt_disabled());

	level = 1;
	do {
		index = pl_i(va, level + 1);
		opde = pmap_pte_testset(&pdes[level - 1][index], 0);
#if defined(XEN) && defined(__x86_64__)
		/*
		 * If ptp is a L3 currently mapped in kernel space,
		 * clear it before freeing
		 */
		if (pmap->pm_pdirpa == xen_current_user_pgd
		    && level == PTP_LEVELS - 1)
			pmap_pte_set(&pmap_kernel()->pm_pdir[index], 0);
#endif /* XEN && __x86_64__ */
		pmap_freepage(pmap, ptp, level);
		invaladdr = level == 1 ? (vaddr_t)ptes :
		    (vaddr_t)pdes[level - 2];
		pmap_tlb_shootdown(curpmap, invaladdr + index * PAGE_SIZE,
		    0, opde);
#if defined(MULTIPROCESSOR)
		invaladdr2 = level == 1 ? (vaddr_t)PTE_BASE :
		    (vaddr_t)normal_pdes[level - 2];
		if (pmap != curpmap || invaladdr != invaladdr2) {
			pmap_tlb_shootdown(pmap, invaladdr2 + index * PAGE_SIZE,
			    0, opde);
		}
#endif
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
	KASSERT(mutex_owned(&pmap->pm_lock));
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
		/*
		 * In Xen we must enter the mapping in kernel map too
		 * if pmap is curmap and modifying top level (PGD)
		 */
		if(i == PTP_LEVELS && pmap != pmap_kernel()) {
		        pmap_pte_set(&pmap_kernel()->pm_pdir[index],
		                (pd_entry_t) (pmap_pa2pte(pa)
		                        | PG_u | PG_RW | PG_V));
		}
#endif /* XEN && __x86_64__ */
		pmap_pte_flush();
		pmap_stats_update(pmap, 1, 0);
		/*
		 * If we're not in the top level, increase the
		 * wire count of the parent page.
		 */
		if (i < PTP_LEVELS) {
			if (pptp == NULL)
				pptp = pmap_find_ptp(pmap, va, ppa, i);
#ifdef DIAGNOSTIC
			if (pptp == NULL)
				panic("pde page disappeared");
#endif
			pptp->wire_count++;
		}
	}

	/*
	 * ptp is not NULL if we just allocated a new ptp. If it's
	 * still NULL, we must look up the existing one.
	 */
	if (ptp == NULL) {
		ptp = pmap_find_ptp(pmap, va, ppa, 1);
#ifdef DIAGNOSTIC
		if (ptp == NULL) {
			printf("va %lx ppa %lx\n", (unsigned long)va,
			    (unsigned long)ppa);
			panic("pmap_get_ptp: unmanaged user PTP");
		}
#endif
	}

	pmap->pm_ptphint[0] = ptp;
	return(ptp);
}

/*
 * p m a p  l i f e c y c l e   f u n c t i o n s
 */

/*
 * pmap_pdp_ctor: constructor for the PDP cache.
 */

int
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
	 * NOTE: The `pmap_lock' is held when the PDP is allocated.
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
	     (unsigned long)-1 & PG_FRAME;
#else /* XEN  && __x86_64__*/
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
	memset(&pdir[PDIR_SLOT_KERN + npde], 0,
	    (NTOPLEVEL_PDES - (PDIR_SLOT_KERN + npde)) * sizeof(pd_entry_t));

	if (VM_MIN_KERNEL_ADDRESS != KERNBASE) {
		int idx = pl_i(KERNBASE, PTP_LEVELS);

		pdir[idx] = PDP_BASE[idx];
	}
#endif /* XEN  && __x86_64__*/
#ifdef XEN
	s = splvm();
	object = (vaddr_t)v;
	for (i = 0; i < PDP_SIZE; i++, object += PAGE_SIZE) {
		(void) pmap_extract(pmap_kernel(), object, &pdirpa);
		/* remap this page RO */
		pmap_kenter_pa(object, pdirpa, VM_PROT_READ, 0);
		pmap_update(pmap_kernel());
		/*
		 * pin as L2/L4 page, we have to do the page with the
		 * PDIR_SLOT_PTE entries last
		 */
#ifdef PAE
		if (i == l2tol3(PDIR_SLOT_PTE))
			continue;
#endif
		xpq_queue_pin_table(xpmap_ptom_masked(pdirpa));
	}
#ifdef PAE
	object = ((vaddr_t)pdir) + PAGE_SIZE  * l2tol3(PDIR_SLOT_PTE);
	(void)pmap_extract(pmap_kernel(), object, &pdirpa);
	xpq_queue_pin_table(xpmap_ptom_masked(pdirpa));
#endif
	xpq_flush_queue();
	splx(s);
#endif /* XEN */

	return (0);
}

/*
 * pmap_pdp_dtor: destructor for the PDP cache.
 */

void
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
		xpq_queue_pte_update(xpmap_ptetomach(pte), *pte | PG_RW);
		xpq_queue_invlpg((vaddr_t)object);
	}
	xpq_flush_queue();
	splx(s);
#endif  /* XEN */
}

#ifdef PAE

/* pmap_pdp_alloc: Allocate a page for the pdp memory pool. */

void *
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

void
pmap_pdp_free(struct pool *pp, void *v)
{
	uvm_km_free(kernel_map, (vaddr_t)v, PAGE_SIZE * PDP_SIZE,
	    UVM_KMF_WIRED);
}
#endif /* PAE */

/*
 * pmap_create: create a pmap
 *
 * => note: old pmap interface took a "size" args which allowed for
 *	the creation of "software only" pmaps (not in bsd).
 */

struct pmap *
pmap_create(void)
{
	struct pmap *pmap;
	int i;

	pmap = pool_cache_get(&pmap_cache, PR_WAITOK);

	/* init uvm_object */
	for (i = 0; i < PTP_LEVELS - 1; i++) {
		UVM_OBJ_INIT(&pmap->pm_obj[i], NULL, 1);
		pmap->pm_ptphint[i] = NULL;
	}
	pmap->pm_stats.wired_count = 0;
	pmap->pm_stats.resident_count = 1;	/* count the PDP allocd below */
#if !defined(__x86_64__)
	pmap->pm_hiexec = 0;
#endif /* !defined(__x86_64__) */
	pmap->pm_flags = 0;
	pmap->pm_cpus = 0;
	pmap->pm_kernel_cpus = 0;

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

#ifdef PAE
	for (i = 0; i < PDP_SIZE; i++)
		pmap->pm_pdirpa[i] =
		    pmap_pte2pa(pmap->pm_pdir[PDIR_SLOT_PTE + i]);
#else
	pmap->pm_pdirpa = pmap_pte2pa(pmap->pm_pdir[PDIR_SLOT_PTE]);
#endif

	LIST_INSERT_HEAD(&pmaps, pmap, pm_list);

	mutex_exit(&pmaps_lock);

	return (pmap);
}

/*
 * pmap_destroy: drop reference count on pmap.   free pmap if
 *	reference count goes to zero.
 */

void
pmap_destroy(struct pmap *pmap)
{
	int i;
#ifdef DIAGNOSTIC
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;
#endif /* DIAGNOSTIC */

	/*
	 * if we have torn down this pmap, process deferred frees and
	 * invalidations now.
	 */
	if (__predict_false(curlwp->l_md.md_gc_pmap == pmap)) {
		pmap_update(pmap);
	}

	/*
	 * drop reference count
	 */

	if (atomic_dec_uint_nv(&pmap->pm_obj[0].uo_refs) > 0) {
		return;
	}

#ifdef DIAGNOSTIC
	for (CPU_INFO_FOREACH(cii, ci))
		if (ci->ci_pmap == pmap)
			panic("destroying pmap being used");
#endif /* DIAGNOSTIC */

	/*
	 * reference count is zero, free pmap resources and then free pmap.
	 */
#ifdef XEN
	/*
	 * Xen lazy APDP handling:
	 * clear APDP_PDE if pmap is the currently mapped
	 */
	if (xpmap_ptom_masked(pmap_pdirpa(pmap, 0)) == (*APDP_PDE & PG_FRAME)) {
		kpreempt_disable();
		for (i = 0; i < PDP_SIZE; i++) {
	        	pmap_pte_set(&APDP_PDE[i], 0);
#ifdef PAE
			/* clear shadow entry too */
	    		pmap_pte_set(&APDP_PDE_SHADOW[i], 0);
#endif
		}
		pmap_pte_flush();
	        pmap_apte_flush(pmap_kernel());
	        kpreempt_enable();
	}
#endif

	/*
	 * remove it from global list of pmaps
	 */

	mutex_enter(&pmaps_lock);
	LIST_REMOVE(pmap, pm_list);
	mutex_exit(&pmaps_lock);

	/*
	 * destroyed pmap shouldn't have remaining PTPs
	 */

	for (i = 0; i < PTP_LEVELS - 1; i++) {
		KASSERT(pmap->pm_obj[i].uo_npages == 0);
		KASSERT(TAILQ_EMPTY(&pmap->pm_obj[i].memq));
	}

	/*
	 * MULTIPROCESSOR -- no need to flush out of other processors'
	 * APTE space because we do that in pmap_unmap_ptes().
	 */
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

	for (i = 0; i < PTP_LEVELS - 1; i++)
		mutex_destroy(&pmap->pm_obj[i].vmobjlock);
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
		struct pcb *pcb;

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

		pcb = lwp_getpcb(l);
		ci->ci_want_pmapload = 1;

#if defined(__x86_64__)
		if (pcb->pcb_flags & PCB_GS64)
			wrmsr(MSR_KERNELGSBASE, pcb->pcb_gs);
		if (pcb->pcb_flags & PCB_FS64)
			wrmsr(MSR_FSBASE, pcb->pcb_fs);
#endif /* defined(__x86_64__) */
	}
}

/*
 * pmap_reactivate: try to regain reference to the pmap.
 *
 * => must be called with kernel preemption disabled
 */

static bool
pmap_reactivate(struct pmap *pmap)
{
	struct cpu_info *ci;
	uint32_t cpumask;
	bool result;	
	uint32_t oldcpus;

	ci = curcpu();
	cpumask = ci->ci_cpumask;

	KASSERT(kpreempt_disabled());
#if defined(XEN) && defined(__x86_64__)
	KASSERT(pmap->pm_pdirpa == xen_current_user_pgd);
#elif defined(PAE)
	KASSERT(pmap_pdirpa(pmap, 0) == pmap_pte2pa(pmap_l3pd[0]));
#elif !defined(XEN) 
	KASSERT(pmap->pm_pdirpa == pmap_pte2pa(rcr3()));
#endif

	/*
	 * if we still have a lazy reference to this pmap,
	 * we can assume that there was no tlb shootdown
	 * for this pmap in the meantime.
	 *
	 * the order of events here is important as we must
	 * synchronize with TLB shootdown interrupts.  declare
	 * interest in invalidations (TLBSTATE_VALID) and then
	 * check the cpumask, which the IPIs can change only
	 * when the state is TLBSTATE_LAZY.
	 */

	ci->ci_tlbstate = TLBSTATE_VALID;
	oldcpus = pmap->pm_cpus;
	KASSERT((pmap->pm_kernel_cpus & cpumask) != 0);
	if (oldcpus & cpumask) {
		/* got it */
		result = true;
	} else {
		/* must reload */
		atomic_or_32(&pmap->pm_cpus, cpumask);
		result = false;
	}

	return result;
}

/*
 * pmap_load: actually switch pmap.  (fill in %cr3 and LDT info)
 */

void
pmap_load(void)
{
	struct cpu_info *ci;
	uint32_t cpumask;
	struct pmap *pmap;
	struct pmap *oldpmap;
	struct lwp *l;
	struct pcb *pcb;
	uint64_t ncsw;

	kpreempt_disable();
 retry:
	ci = curcpu();
	if (!ci->ci_want_pmapload) {
		kpreempt_enable();
		return;
	}
	cpumask = ci->ci_cpumask;
	l = ci->ci_curlwp;
	ncsw = l->l_ncsw;

	/* should be able to take ipis. */
	KASSERT(ci->ci_ilevel < IPL_IPI); 
#ifdef XEN
	/* XXX not yet KASSERT(x86_read_psl() != 0); */
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
	 * grab a reference to the new pmap.
	 */

	pmap_reference(pmap);

	/*
	 * actually switch pmap.
	 */

	atomic_and_32(&oldpmap->pm_cpus, ~cpumask);
	atomic_and_32(&oldpmap->pm_kernel_cpus, ~cpumask);

#if defined(XEN) && defined(__x86_64__)
	KASSERT(oldpmap->pm_pdirpa == xen_current_user_pgd ||
	    oldpmap == pmap_kernel());
#elif defined(PAE)
	KASSERT(pmap_pdirpa(oldpmap, 0) == pmap_pte2pa(pmap_l3pd[0]));
#elif !defined(XEN)
	KASSERT(oldpmap->pm_pdirpa == pmap_pte2pa(rcr3()));
#endif
	KASSERT((pmap->pm_cpus & cpumask) == 0);
	KASSERT((pmap->pm_kernel_cpus & cpumask) == 0);

	/*
	 * mark the pmap in use by this processor.  again we must
	 * synchronize with TLB shootdown interrupts, so set the
	 * state VALID first, then register us for shootdown events
	 * on this pmap.
	 */

	ci->ci_tlbstate = TLBSTATE_VALID;
	atomic_or_32(&pmap->pm_cpus, cpumask);
	atomic_or_32(&pmap->pm_kernel_cpus, cpumask);
	ci->ci_pmap = pmap;

	/*
	 * update tss.  now that we have registered for invalidations
	 * from other CPUs, we're good to load the page tables.
	 */
#ifdef PAE
	pcb->pcb_cr3 = pmap_l3paddr;
#else
	pcb->pcb_cr3 = pmap->pm_pdirpa;
#endif
#if defined(XEN) && defined(__x86_64__)
	/* kernel pmap always in cr3 and should never go in user cr3 */
	if (pmap_pdirpa(pmap, 0) != pmap_pdirpa(pmap_kernel(), 0)) {
		/*
		 * Map user space address in kernel space and load
		 * user cr3
		 */
		int i, s;
		pd_entry_t *old_pgd, *new_pgd;
		paddr_t addr;
		s = splvm();
		new_pgd  = pmap->pm_pdir;
		old_pgd = pmap_kernel()->pm_pdir;
		addr = xpmap_ptom(pmap_pdirpa(pmap_kernel(), 0));
		for (i = 0; i < PDIR_SLOT_PTE;
		    i++, addr += sizeof(pd_entry_t)) {
			if ((new_pgd[i] & PG_V) || (old_pgd[i] & PG_V))
				xpq_queue_pte_update(addr, new_pgd[i]);
		}
		xpq_flush_queue(); /* XXXtlb */
		tlbflush();
		xen_set_user_pgd(pmap_pdirpa(pmap, 0));
		xen_current_user_pgd = pmap_pdirpa(pmap, 0);
		splx(s);
	}
#else /* XEN && x86_64 */
#if defined(XEN)
	/*
	 * clear APDP slot, in case it points to a page table that has 
	 * been freed
	 */
	if (*APDP_PDE) {
		int i;
		for (i = 0; i < PDP_SIZE; i++) {
			pmap_pte_set(&APDP_PDE[i], 0);
#ifdef PAE
			/* clear shadow entry too */
			pmap_pte_set(&APDP_PDE_SHADOW[i], 0);
#endif
		}
	}
	/* lldt() does pmap_pte_flush() */
#else /* XEN */
#if defined(i386)
	ci->ci_tss.tss_ldt = pmap->pm_ldt_sel;
	ci->ci_tss.tss_cr3 = pcb->pcb_cr3;
#endif
#endif /* XEN */
	lldt(pmap->pm_ldt_sel);
#ifdef PAE
	{
	paddr_t l3_pd = xpmap_ptom_masked(pmap_l3paddr);
	int i;
	int s = splvm();
	/* don't update the kernel L3 slot */
	for (i = 0 ; i < PDP_SIZE - 1  ; i++, l3_pd += sizeof(pd_entry_t)) {
		xpq_queue_pte_update(l3_pd,
		    xpmap_ptom(pmap->pm_pdirpa[i]) | PG_V);
	}
	tlbflush();
	xpq_flush_queue();
	splx(s);
	}
#else /* PAE */
	{
	u_int gen = uvm_emap_gen_return();
	lcr3(pcb->pcb_cr3);
	uvm_emap_update(gen);
	}
#endif /* PAE */
#endif /* XEN && x86_64 */

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
 * pmap_deactivate: deactivate a process' pmap
 *
 * => must be called with kernel preemption disabled (high SPL is enough)
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
	 * wait for pending TLB shootdowns to complete.  necessary
	 * because TLB shootdown state is per-CPU, and the LWP may
	 * be coming off the CPU before it has a chance to call
	 * pmap_update().
	 */
	pmap_tlb_shootwait();

	ci = curcpu();

	if (ci->ci_want_pmapload) {
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
	KASSERT(pmap->pm_pdirpa == xen_current_user_pgd);
#elif defined(PAE)
	KASSERT(pmap_pdirpa(pmap, 0) == pmap_pte2pa(pmap_l3pd[0]));
#elif !defined(XEN) 
	KASSERT(pmap->pm_pdirpa == pmap_pte2pa(rcr3()));
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

static int
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
	vaddr_t pa;
	lwp_t *l;
	bool hard, rv;

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

#ifdef XEN
/*
 * pmap_extract_ma: extract a MA for the given VA
 */

bool
pmap_extract_ma(struct pmap *pmap, vaddr_t va, paddr_t *pap)
{
	pt_entry_t *ptes, pte;
	pd_entry_t pde;
	pd_entry_t * const *pdes;
	struct pmap *pmap2;
 
	kpreempt_disable();
	pmap_map_ptes(pmap, &pmap2, &ptes, &pdes);
	if (!pmap_pdes_valid(va, pdes, &pde)) {
		pmap_unmap_ptes(pmap, pmap2);
		kpreempt_enable();
		return false;
	}
 
	pte = ptes[pl1_i(va)];
	pmap_unmap_ptes(pmap, pmap2);
	kpreempt_enable();
 
	if (__predict_true((pte & PG_V) != 0)) {
		if (pap != NULL)
			*pap = (pte & PG_FRAME) | (va & (NBPD_L1 - 1));
		return true;
	}
				 
	return false;
}

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
 * pmap_map: map a range of PAs into kvm.
 *
 * => used during crash dump
 * => XXX: pmap_map() should be phased out?
 */

vaddr_t
pmap_map(vaddr_t va, paddr_t spa, paddr_t epa, vm_prot_t prot)
{
	while (spa < epa) {
		pmap_kenter_pa(va, spa, prot, 0);
		va += PAGE_SIZE;
		spa += PAGE_SIZE;
	}
	pmap_update(pmap_kernel());
	return va;
}

/*
 * pmap_zero_page: zero a page
 */

void
pmap_zero_page(paddr_t pa)
{
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
}

/*
 * pmap_pagezeroidle: the same, for the idle loop page zero'er.
 * Returns true if the page was zero'd, false if we aborted for
 * some reason.
 */

bool
pmap_pageidlezero(paddr_t pa)
{
	pt_entry_t *zpte;
	void *zerova;
	bool rv;
	int id;

	id = cpu_number();
	zpte = PTESLEW(zero_pte, id);
	zerova = VASLEW(zerop, id);

	KASSERT(cpu_feature & CPUID_SSE2);
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
}

/*
 * pmap_copy_page: copy a page
 */

void
pmap_copy_page(paddr_t srcpa, paddr_t dstpa)
{
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
}

static pt_entry_t *
pmap_map_ptp(struct vm_page *ptp)
{
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
}

static void
pmap_unmap_ptp(void)
{
#if defined(DIAGNOSTIC) || defined(XEN)
	pt_entry_t *pte;

	KASSERT(kpreempt_disabled());

	pte = PTESLEW(ptp_pte, cpu_number());
	if (*pte != 0) {
		pmap_pte_set(pte, 0);
		pmap_pte_flush();
	}
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
 * => must have proper locking on pmap_master_lock
 * => caller must hold pmap's lock
 * => PTP must be mapped into KVA
 * => PTP should be null if pmap == pmap_kernel()
 * => must be called with kernel preemption disabled
 * => returns composite pte if at least one page should be shot down
 */

static pt_entry_t
pmap_remove_ptes(struct pmap *pmap, struct vm_page *ptp, vaddr_t ptpva,
		 vaddr_t startva, vaddr_t endva, struct pv_entry **pv_tofree)
{
	struct pv_entry *pve;
	pt_entry_t *pte = (pt_entry_t *) ptpva;
	pt_entry_t opte, xpte = 0;

	KASSERT(pmap == pmap_kernel() || mutex_owned(&pmap->pm_lock));
	KASSERT(kpreempt_disabled());

	/*
	 * note that ptpva points to the PTE that maps startva.   this may
	 * or may not be the first PTE in the PTP.
	 *
	 * we loop through the PTP while there are still PTEs to look at
	 * and the wire_count is greater than 1 (because we use the wire_count
	 * to keep track of the number of real PTEs in the PTP).
	 */

	for (/*null*/; startva < endva && (ptp == NULL || ptp->wire_count > 1)
			     ; pte++, startva += PAGE_SIZE) {
		struct vm_page *pg;
		struct pmap_page *pp;

		if (!pmap_valid_entry(*pte))
			continue;			/* VA not mapped */

		/* atomically save the old PTE and zap! it */
		opte = pmap_pte_testset(pte, 0);
		if (!pmap_valid_entry(opte)) {
			continue;
		}

		pmap_exec_account(pmap, startva, opte, 0);
		pmap_stats_update_bypte(pmap, 0, opte);
		xpte |= opte;

		if (ptp) {
			ptp->wire_count--;		/* dropping a PTE */
			/* Make sure that the PDE is flushed */
			if (ptp->wire_count <= 1)
				xpte |= PG_U;
		}

		/*
		 * if we are not on a pv_head list we are done.
		 */

		if ((opte & PG_PVLIST) == 0) {
#if defined(DIAGNOSTIC) && !defined(DOM0OPS)
			if (PHYS_TO_VM_PAGE(pmap_pte2pa(opte)) != NULL)
				panic("pmap_remove_ptes: managed page without "
				      "PG_PVLIST for 0x%lx", startva);
#endif
			continue;
		}

		pg = PHYS_TO_VM_PAGE(pmap_pte2pa(opte));
#ifdef DIAGNOSTIC
		if (pg == NULL)
			panic("pmap_remove_ptes: unmanaged page marked "
			      "PG_PVLIST, va = 0x%lx, pa = 0x%lx",
			      startva, (u_long)pmap_pte2pa(opte));
#endif

		/* sync R/M bits */
		pp = VM_PAGE_TO_PP(pg);
		pp_lock(pp);
		pp->pp_attrs |= opte;
		pve = pmap_remove_pv(pp, ptp, startva);
		pp_unlock(pp);

		if (pve != NULL) {
			pve->pve_next = *pv_tofree;
			*pv_tofree = pve;
		}

		/* end of "for" loop: time for next pte */
	}

	return xpte;
}


/*
 * pmap_remove_pte: remove a single PTE from a PTP
 *
 * => must have proper locking on pmap_master_lock
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
	pt_entry_t opte;
	struct pv_entry *pve;
	struct vm_page *pg;
	struct pmap_page *pp;

	KASSERT(pmap == pmap_kernel() || mutex_owned(&pmap->pm_lock));
	KASSERT(pmap == pmap_kernel() || kpreempt_disabled());

	if (!pmap_valid_entry(*pte))
		return(false);		/* VA not mapped */

	/* atomically save the old PTE and zap! it */
	opte = pmap_pte_testset(pte, 0);
	if (!pmap_valid_entry(opte)) {
		return false;
	}

	pmap_exec_account(pmap, va, opte, 0);
	pmap_stats_update_bypte(pmap, 0, opte);

	if (opte & PG_U)
		pmap_tlb_shootdown(pmap, va, 0, opte);

	if (ptp) {
		ptp->wire_count--;		/* dropping a PTE */
		/* Make sure that the PDE is flushed */
		if ((ptp->wire_count <= 1) && !(opte & PG_U))
			pmap_tlb_shootdown(pmap, va, 0, opte);
	}

	/*
	 * if we are not on a pv_head list we are done.
	 */

	if ((opte & PG_PVLIST) == 0) {
#if defined(DIAGNOSTIC) && !defined(DOM0OPS)
		if (PHYS_TO_VM_PAGE(pmap_pte2pa(opte)) != NULL)
			panic("pmap_remove_pte: managed page without "
			      "PG_PVLIST for 0x%lx", va);
#endif
		return(true);
	}

	pg = PHYS_TO_VM_PAGE(pmap_pte2pa(opte));
#ifdef DIAGNOSTIC
	if (pg == NULL)
		panic("pmap_remove_pte: unmanaged page marked "
		    "PG_PVLIST, va = 0x%lx, pa = 0x%lx", va,
		    (u_long)(pmap_pte2pa(opte)));
#endif

	/* sync R/M bits */
	pp = VM_PAGE_TO_PP(pg);
	pp_lock(pp);
	pp->pp_attrs |= opte;
	pve = pmap_remove_pv(pp, ptp, va);
	pp_unlock(pp);

	if (pve) { 
		pve->pve_next = *pv_tofree;
		*pv_tofree = pve;
	}

	return(true);
}

/*
 * pmap_remove: mapping removal function.
 *
 * => caller should not be holding any pmap locks
 */

void
pmap_remove(struct pmap *pmap, vaddr_t sva, vaddr_t eva)
{
	pt_entry_t *ptes, xpte = 0;
	pd_entry_t pde;
	pd_entry_t * const *pdes;
	struct pv_entry *pv_tofree = NULL;
	bool result;
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

			/* get PTP if non-kernel mapping */
			if (pmap == pmap_kernel()) {
				/* we never free kernel PTPs */
				ptp = NULL;
			} else {
				ptp = pmap_find_ptp(pmap, va, ptppa, 1);
#ifdef DIAGNOSTIC
				if (ptp == NULL)
					panic("pmap_remove: unmanaged "
					      "PTP detected");
#endif
			}

			/* do it! */
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

		if (pl_i(va, PTP_LEVELS) == PDIR_SLOT_PTE)
			/* XXXCDC: ugly hack to avoid freeing PDP here */
			continue;

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

		/* get PTP if non-kernel mapping */
		if (pmap == pmap_kernel()) {
			/* we never free kernel PTPs */
			ptp = NULL;
		} else {
			ptp = pmap_find_ptp(pmap, va, ptppa, 1);
#ifdef DIAGNOSTIC
			if (ptp == NULL)
				panic("pmap_remove: unmanaged PTP "
				      "detected");
#endif
		}
		xpte |= pmap_remove_ptes(pmap, ptp,
		    (vaddr_t)&ptes[pl1_i(va)], va, blkendva, &pv_tofree);

		/* if PTP is no longer being used, free it! */
		if (ptp && ptp->wire_count <= 1) {
			pmap_free_ptp(pmap, ptp, va, ptes, pdes);
		}
		if ((xpte & PG_U) != 0)
			pmap_tlb_shootdown(pmap, sva, eva, xpte);
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
 * => called with pp_lock held. (thus preemption disabled)
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
				pmap_tlb_shootdown(pmap, va, 0,
				    (pmap == pmap_kernel() ? PG_G : 0));
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
		pmap_tlb_shootdown(pmap, va, 0, opte);
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
	lwp_t *l;
	int count;

	l = curlwp;
	pp = VM_PAGE_TO_PP(pg);
	expect = pmap_pa2pte(VM_PAGE_TO_PHYS(pg)) | PG_V;
	count = SPINLOCK_BACKOFF_MIN;
	kpreempt_disable();
startover:
	pp_lock(pp);
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
			pp_unlock(pp);
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
		pp_unlock(pp);

		/* update the PTP reference count.  free if last reference. */
		if (ptp != NULL) {
			struct pmap *pmap2;
			pt_entry_t *ptes;
			pd_entry_t * const *pdes;

			KASSERT(pmap != pmap_kernel());

			pmap_tlb_shootwait();
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
		pp_lock(pp);
	}
	pp_unlock(pp);
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

	pp = VM_PAGE_TO_PP(pg);
	if ((pp->pp_attrs & testbits) != 0) {
		return true;
	}
	expect = pmap_pa2pte(VM_PAGE_TO_PHYS(pg)) | PG_V;
	pp_lock(pp);
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
	pp_unlock(pp);

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

	pp = VM_PAGE_TO_PP(pg);
	expect = pmap_pa2pte(VM_PAGE_TO_PHYS(pg)) | PG_V;
	count = SPINLOCK_BACKOFF_MIN;
	kpreempt_disable();
startover:
	pp_lock(pp);
	for (pvpte = pv_pte_first(pp); pvpte; pvpte = pv_pte_next(pp, pvpte)) {
		pt_entry_t opte;
		int error;

		error = pmap_sync_pv(pvpte, expect, clearbits, &opte);
		if (error == EAGAIN) {
			int hold_count;
			pp_unlock(pp);
			KERNEL_UNLOCK_ALL(curlwp, &hold_count);
			SPINLOCK_BACKOFF(count);
			KERNEL_LOCK(hold_count, curlwp);
			goto startover;
		}
		pp->pp_attrs |= opte;
	}
	result = pp->pp_attrs & clearbits;
	pp->pp_attrs &= ~clearbits;
	pp_unlock(pp);
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
 * pmap_write_protect: write-protect pages in a pmap
 */

void
pmap_write_protect(struct pmap *pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
	pt_entry_t *ptes, *epte;
	pt_entry_t *spte;
	pd_entry_t * const *pdes;
	vaddr_t blockend, va;
	pt_entry_t opte;
	struct pmap *pmap2;

	KASSERT(curlwp->l_md.md_gc_pmap != pmap);

	kpreempt_disable();
	pmap_map_ptes(pmap, &pmap2, &ptes, &pdes);	/* locks pmap */

	/* should be ok, but just in case ... */
	sva &= PG_FRAME;
	eva &= PG_FRAME;

	for (va = sva ; va < eva ; va = blockend) {

		blockend = (va & L2_FRAME) + NBPD_L2;
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
		if (pl_i(va, PTP_LEVELS) == PDIR_SLOT_PTE)
			continue;

		/* empty block? */
		if (!pmap_pdes_valid(va, pdes, NULL))
			continue;

#ifdef DIAGNOSTIC
		if (va >= VM_MAXUSER_ADDRESS &&
		    va < VM_MAX_ADDRESS)
			panic("pmap_write_protect: PTE space");
#endif

		spte = &ptes[pl1_i(va)];
		epte = &ptes[pl1_i(blockend)];

		for (/*null */; spte < epte ; spte++) {
			pt_entry_t npte;

			do {
				opte = *spte;
				if ((~opte & (PG_RW | PG_V)) != 0) {
					goto next;
				}
				npte = opte & ~PG_RW;
			} while (pmap_pte_cas(spte, opte, npte) != opte);
			if ((opte & PG_M) != 0) {
				vaddr_t tva;

				tva = x86_ptob(spte - ptes);
				pmap_tlb_shootdown(pmap, tva, 0, opte);
			}
next:;
		}
	}

	pmap_unmap_ptes(pmap, pmap2);	/* unlocks pmap */
	kpreempt_enable();
}

/*
 * end of protection functions
 */

/*
 * pmap_unwire: clear the wired bit in the PTE
 *
 * => mapping should already be in map
 */

void
pmap_unwire(struct pmap *pmap, vaddr_t va)
{
	pt_entry_t *ptes;
	pd_entry_t * const *pdes;
	struct pmap *pmap2;

	kpreempt_disable();
	pmap_map_ptes(pmap, &pmap2, &ptes, &pdes);	/* locks pmap */

	if (pmap_pdes_valid(va, pdes, NULL)) {
		pt_entry_t *ptep = &ptes[pl1_i(va)];
		pt_entry_t opte = *ptep;

#ifdef DIAGNOSTIC
		if (!pmap_valid_entry(opte))
			panic("pmap_unwire: invalid (unmapped) va 0x%lx", va);
#endif
		if ((opte & PG_W) != 0) {
			pt_entry_t npte = opte & ~PG_W;

			opte = pmap_pte_testset(ptep, npte);
			pmap_stats_update_bypte(pmap, npte, opte);
		}
#ifdef DIAGNOSTIC
		else {
			printf("pmap_unwire: wiring for pmap %p va 0x%lx "
			       "didn't change!\n", pmap, va);
		}
#endif
		pmap_unmap_ptes(pmap, pmap2);		/* unlocks map */
	}
#ifdef DIAGNOSTIC
	else {
		panic("pmap_unwire: invalid PDE");
	}
#endif
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

/*
 * pmap_enter: enter a mapping into a pmap
 *
 * => must be done "now" ... no lazy-evaluation
 * => we set pmap => pv_head locking
 */
#ifdef XEN
int
pmap_enter_ma(struct pmap *pmap, vaddr_t va, paddr_t ma, paddr_t pa,
	   vm_prot_t prot, u_int flags, int domid)
{
#else /* XEN */
int
pmap_enter(struct pmap *pmap, vaddr_t va, paddr_t pa, vm_prot_t prot,
	   u_int flags)
{
	paddr_t ma = pa;
#endif /* XEN */
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

#ifdef DIAGNOSTIC
	/* sanity check: totally out of range? */
	if (va >= VM_MAX_KERNEL_ADDRESS)
		panic("pmap_enter: too big");

	if (va == (vaddr_t) PDP_BASE || va == (vaddr_t) APDP_BASE)
		panic("pmap_enter: trying to map over PDP/APDP!");

	/* sanity check: kernel PTPs should already have been pre-allocated */
	if (va >= VM_MIN_KERNEL_ADDRESS &&
	    !pmap_valid_entry(pmap->pm_pdir[pl_i(va, PTP_LEVELS)]))
		panic("pmap_enter: missing kernel PTP for va %lx!", va);
#endif /* DIAGNOSTIC */
#ifdef XEN
	KASSERT(domid == DOMID_SELF || pa == 0);
#endif /* XEN */

	npte = ma | protection_codes[prot] | PG_V;
	if (wired)
	        npte |= PG_W;
	if (flags & PMAP_NOCACHE)
		npte |= PG_N;
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
#ifdef DIAGNOSTIC
		if (pg == NULL)
			panic("pmap_enter: PG_PVLIST mapping with "
			      "unmanaged page "
			      "pa = 0x%" PRIx64 " (0x%" PRIx64 ")",
			      (int64_t)pa, (int64_t)atop(pa));
#endif
		old_pp = VM_PAGE_TO_PP(pg);

		pp_lock(old_pp);
		old_pve = pmap_remove_pv(old_pp, ptp, va);
		old_pp->pp_attrs |= opte;
		pp_unlock(old_pp);
	}

	/*
	 * if new page is managed, insert pv_entry into its list.
	 */

	if (new_pp) {
		pp_lock(new_pp);
		new_pve = pmap_enter_pv(new_pp, new_pve, &new_pve2, ptp, va);
		pp_unlock(new_pp);
	}

same_pa:
	pmap_unmap_ptes(pmap, pmap2);

	/*
	 * shootdown tlb if necessary.
	 */

	if ((~opte & (PG_V | PG_U)) == 0 &&
	    ((opte ^ npte) & (PG_FRAME | PG_RW)) != 0) {
		pmap_tlb_shootdown(pmap, va, 0, opte);
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

#ifdef XEN
int
pmap_enter(struct pmap *pmap, vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
        paddr_t ma;

	if (__predict_false(pa < pmap_pa_start || pmap_pa_end <= pa)) {
		ma = pa; /* XXX hack */
	} else {
		ma = xpmap_ptom(pa);
	}

	return pmap_enter_ma(pmap, va, ma, pa, prot, flags, DOMID_SELF);
}
#endif /* XEN */

static bool
pmap_get_physpage(vaddr_t va, int level, paddr_t *paddrp)
{
	struct vm_page *ptp;
	struct pmap *kpm = pmap_kernel();

	if (uvm.page_init_done == false) {
		/*
		 * we're growing the kernel pmap early (from
		 * uvm_pageboot_alloc()).  this case must be
		 * handled a little differently.
		 */

		if (uvm_page_physget(paddrp) == false)
			panic("pmap_get_physpage: out of memory");
		kpreempt_disable();
		pmap_pte_set(early_zero_pte,
		    pmap_pa2pte(*paddrp) | PG_V | PG_RW | PG_k);
		pmap_pte_flush();
		pmap_update_pg((vaddr_t)early_zerop);
		memset(early_zerop, 0, PAGE_SIZE);
#if defined(DIAGNOSTIC) || defined (XEN)
		pmap_pte_set(early_zero_pte, 0);
		pmap_pte_flush();
#endif /* defined(DIAGNOSTIC) */
		kpreempt_enable();
	} else {
		/* XXX */
		PMAP_SUBOBJ_LOCK(kpm, level - 1);
		ptp = uvm_pagealloc(&kpm->pm_obj[level - 1],
				    ptp_va2o(va, level), NULL,
				    UVM_PGA_USERESERVE|UVM_PGA_ZERO);
		PMAP_SUBOBJ_UNLOCK(kpm, level - 1);
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
			KASSERT(!pmap_valid_entry(pdep[i]));
			pmap_get_physpage(va, level - 1, &pa);
#ifdef XEN
			xpq_queue_pte_update((level == PTP_LEVELS) ?
			    xpmap_ptom(pmap_pdirpa(pmap_kernel(), i)) :
			    xpmap_ptetomach(&pdep[i]),
			    pmap_pa2pte(pa) | PG_k | PG_V | PG_RW);
#ifdef PAE
			if (level == PTP_LEVELS &&  i > L2_SLOT_KERN) {
				/* update real kernel PD too */
				xpq_queue_pte_update(
				    xpmap_ptetomach(&pmap_kl2pd[l2tol2(i)]),
				    pmap_pa2pte(pa) | PG_k | PG_V | PG_RW);
			}
#endif
#else /* XEN */
			pdep[i] = pa | PG_RW | PG_V;
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
#endif
	int s, i;
	long needed_kptp[PTP_LEVELS], target_nptp, old;
	bool invalidate = false;

	s = splvm();	/* to be safe */
	mutex_enter(&kpm->pm_lock);

	if (maxkvaddr <= pmap_maxkvaddr) {
		mutex_exit(&kpm->pm_lock);
		splx(s);
		return pmap_maxkvaddr;
	}

	maxkvaddr = x86_round_pdr(maxkvaddr);
	old = nkptp[PTP_LEVELS - 1];
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
				xpq_queue_pte_update(
				    xpmap_ptom(pmap_pdirpa(pm, pdkidx)),
				    kpm->pm_pdir[pdkidx]);
			}
			xpq_flush_queue();
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
	mutex_exit(&kpm->pm_lock);
	splx(s);

	if (invalidate) {
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
			printf("va %#lx -> pa %#lx (pte=%#lx)\n",
			       sva, (unsigned long)*pte,
			       (unsigned long)pmap_pte2pa(*pte));
		}
	}
	pmap_unmap_ptes(pmap, pmap2);
	kpreempt_enable();
}
#endif

/*
 * pmap_tlb_shootdown: invalidate pages on all CPUs using pmap 'pm'
 *
 * => always invalidates locally before returning
 * => returns before remote CPUs have invalidated
 * => must be called with preemption disabled
 */

void
pmap_tlb_shootdown(struct pmap *pm, vaddr_t sva, vaddr_t eva, pt_entry_t pte)
{
#ifdef MULTIPROCESSOR
	extern bool x86_mp_online;
	struct cpu_info *ci;
	struct pmap_mbox *mb, *selfmb;
	CPU_INFO_ITERATOR cii;
	uintptr_t head;
	u_int count;
	int s;
#endif	/* MULTIPROCESSOR */
	struct cpu_info *self;
	bool kernel;

	KASSERT(eva == 0 || eva >= sva);
	KASSERT(kpreempt_disabled());

	if (pte & PG_PS)
		sva &= PG_LGFRAME;
	pte &= PG_G;
	self = curcpu();

	if (sva == (vaddr_t)-1LL) {
		kernel = true;
	} else {
		if (eva == 0)
			eva = sva + PAGE_SIZE;
		kernel = sva >= VM_MAXUSER_ADDRESS;
		KASSERT(kernel == (eva > VM_MAXUSER_ADDRESS));
	}

	/*
	 * if tearing down the pmap, do nothing.  we'll flush later
	 * when we're ready to recycle/destroy it.
	 */
	if (__predict_false(curlwp->l_md.md_gc_pmap == pm)) {
		return;
	}

	/*
	 * If the range is larger than 32 pages, then invalidate
	 * everything.
	 */
	if (sva != (vaddr_t)-1LL && eva - sva > (32 * PAGE_SIZE)) {
		sva = (vaddr_t)-1LL;
		eva = sva;
	}

#ifdef MULTIPROCESSOR
	if (ncpu > 1 && x86_mp_online) {
		selfmb = &self->ci_pmap_cpu->pc_mbox;

		/*
		 * If the CPUs have no notion of global pages then
		 * reload of %cr3 is sufficient.
		 */
		if (pte != 0 && (cpu_feature & CPUID_PGE) == 0)
			pte = 0;

		if (pm == pmap_kernel()) {
			/*
			 * Mapped on all CPUs: use the broadcast mechanism.
			 * Once we have the lock, increment the counter.
			 */
			s = splvm();
			mb = &pmap_mbox;
			count = SPINLOCK_BACKOFF_MIN;
			do {
				if ((head = mb->mb_head) != mb->mb_tail) {
					splx(s);
					while ((head = mb->mb_head) !=
					    mb->mb_tail)
						SPINLOCK_BACKOFF(count);
					s = splvm();
				}
			} while (atomic_cas_ulong(
			    (volatile u_long *)&mb->mb_head,
			    head, head + ncpu - 1) != head);

			/*
			 * Once underway we must stay at IPL_VM until the
			 * IPI is dispatched.  Otherwise interrupt handlers
			 * on this CPU can deadlock against us.
			 */
			pmap_tlb_evcnt.ev_count++;
			mb->mb_pointer = self;
			mb->mb_addr1 = sva;
			mb->mb_addr2 = eva;
			mb->mb_global = pte;
			x86_ipi(LAPIC_TLB_BCAST_VECTOR, LAPIC_DEST_ALLEXCL,
			    LAPIC_DLMODE_FIXED);
			self->ci_need_tlbwait = 1;
			splx(s);
		} else if ((pm->pm_cpus & ~self->ci_cpumask) != 0 ||
		    (kernel && (pm->pm_kernel_cpus & ~self->ci_cpumask) != 0)) {
			/*
			 * We don't bother traversing the CPU list if only
			 * used by this CPU.
			 *
			 * We can't do global flushes with the multicast
			 * mechanism.
			 */
			KASSERT(pte == 0);

			/*
			 * Take ownership of the shootdown mailbox on each
			 * CPU, fill the details and fire it off.
			 */
			s = splvm();
			for (CPU_INFO_FOREACH(cii, ci)) {
				if (ci == self ||
				    !pmap_is_active(pm, ci, kernel) ||
				    !(ci->ci_flags & CPUF_RUNNING))
					continue;
				selfmb->mb_head++;
				mb = &ci->ci_pmap_cpu->pc_mbox;
				count = SPINLOCK_BACKOFF_MIN;
				while (atomic_cas_ulong(
				    (u_long *)&mb->mb_pointer,
				    0, (u_long)&selfmb->mb_tail) != 0) {
				    	splx(s);
					while (mb->mb_pointer != 0)
						SPINLOCK_BACKOFF(count);
					s = splvm();
				}
				mb->mb_addr1 = sva;
				mb->mb_addr2 = eva;
				mb->mb_global = pte;
				if (x86_ipi(LAPIC_TLB_MCAST_VECTOR,
				    ci->ci_cpuid, LAPIC_DLMODE_FIXED))
					panic("pmap_tlb_shootdown: ipi failed");
			}
			self->ci_need_tlbwait = 1;
			splx(s);
		}
	}
#endif	/* MULTIPROCESSOR */

	/* Update the current CPU before waiting for others. */
	if (!pmap_is_active(pm, self, kernel))
		return;

	if (sva == (vaddr_t)-1LL) {
		u_int gen = uvm_emap_gen_return();
		if (pte != 0) {
			tlbflushg();
		} else {
			tlbflush();
		}
		uvm_emap_update(gen);
	} else {
		do {
			pmap_update_pg(sva);
			sva += PAGE_SIZE;
		} while (sva < eva);
	}
}

/*
 * pmap_tlb_shootwait: wait for pending TLB shootdowns to complete
 *
 * => only waits for operations generated by the current CPU
 * => must be called with preemption disabled
 */

void
pmap_tlb_shootwait(void)
{
	struct cpu_info *self;
	struct pmap_mbox *mb;

	KASSERT(kpreempt_disabled());

	/*
	 * Anything to do?  XXX Really we want to avoid touching the cache
	 * lines of the two mailboxes, but the processor may read ahead.
	 */
	self = curcpu();
	if (!self->ci_need_tlbwait)
		return;
	self->ci_need_tlbwait = 0;

	/* If we own the global mailbox, wait for it to drain. */
	mb = &pmap_mbox;
	while (mb->mb_pointer == self && mb->mb_head != mb->mb_tail)
		x86_pause();

	/* If we own other CPU's mailboxes, wait for them to drain. */
	mb = &self->ci_pmap_cpu->pc_mbox;
	KASSERT(mb->mb_pointer != &mb->mb_tail);
	while (mb->mb_head != mb->mb_tail)
		x86_pause();
}

/*
 * pmap_update: process deferred invalidations
 */

void
pmap_update(struct pmap *pmap)
{
	struct vm_page *ptp, *empty_ptps;
	struct pmap_page *pp;
	lwp_t *l;

	/*
	 * if we have torn down this pmap, invalidate non-global TLB
	 * entries on any processors using it.
	 */
	l = curlwp;
	if (__predict_false(l->l_md.md_gc_pmap == pmap)) {
		l->l_md.md_gc_pmap = NULL;
		KPREEMPT_DISABLE(l);
		pmap_tlb_shootdown(pmap, -1, -1, 0);
		KPREEMPT_ENABLE(l);
	}

	/*
	 * wait for tlb shootdowns to complete before returning control
	 * to the caller.
	 */
	kpreempt_disable();
	pmap_tlb_shootwait();
	kpreempt_enable();

	/*
	 * now that shootdowns are complete, process deferred frees,
	 * but not from interrupt context.
	 */
	if (l->l_md.md_gc_ptp != NULL) {
		if (cpu_intr_p() || (l->l_pflag & LP_INTR) != 0) {
			return;
		}

		empty_ptps = l->l_md.md_gc_ptp;
		l->l_md.md_gc_ptp = NULL;

		while ((ptp = empty_ptps) != NULL) {
			ptp->flags |= PG_ZERO;
			pp = VM_PAGE_TO_PP(ptp);
			empty_ptps = pp->pp_link;
			LIST_INIT(&pp->pp_head.pvh_list);
			uvm_pagefree(ptp);
		}
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

	/* Hook our own level 3 in */
	tmp_pml[pl_i(pg, PTP_LEVELS)] =
	    (x86_tmp_pml_paddr[PTP_LEVELS - 2] & PG_FRAME) | PG_RW | PG_V;

	for (level = PTP_LEVELS - 1; level > 0; --level) {
		tmp_pml = (void *)x86_tmp_pml_vaddr[level];

		tmp_pml[pl_i(pg, level + 1)] =
		    (x86_tmp_pml_paddr[level - 1] & PG_FRAME) | PG_RW | PG_V;
	}

	tmp_pml = (void *)x86_tmp_pml_vaddr[0];
	tmp_pml[pl_i(pg, 1)] = (pg & PG_FRAME) | PG_RW | PG_V;

	return x86_tmp_pml_paddr[PTP_LEVELS - 1];
}
