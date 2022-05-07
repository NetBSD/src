/*	$NetBSD: pmap.c,v 1.414 2022/05/07 14:59:25 bouyer Exp $	*/

/*
 * Copyright (c) 2008, 2010, 2016, 2017, 2019, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran, and by Maxime Villard.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.414 2022/05/07 14:59:25 bouyer Exp $");

#include "opt_user_ldt.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"
#include "opt_xen.h"
#include "opt_svs.h"
#include "opt_kaslr.h"

#define	__MUTEX_PRIVATE	/* for assertions */

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
#include <sys/kmem.h>
#include <sys/asan.h>
#include <sys/msan.h>
#include <sys/entropy.h>

#include <uvm/uvm.h>
#include <uvm/pmap/pmap_pvt.h>

#include <dev/isa/isareg.h>

#include <machine/specialreg.h>
#include <machine/gdt.h>
#include <machine/isa_machdep.h>
#include <machine/cpuvar.h>
#include <machine/cputypes.h>

#include <x86/pmap_pv.h>

#include <x86/i82489reg.h>
#include <x86/i82489var.h>

#ifdef XEN
#include <xen/include/public/xen.h>
#include <xen/hypervisor.h>
#include <xen/xenpmap.h>
#endif

/*
 * general info:
 *
 *  - for an explanation of how the x86 MMU hardware works see
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
 *  - struct pmap_page: describes one pv-tracked page, without
 *    necessarily a corresponding vm_page
 *  - struct pv_entry: describes one <PMAP,VA> mapping of a PA
 *  - pmap_page::pp_pvlist: there is one list per pv-tracked page of
 *    physical memory.   the pp_pvlist points to a list of pv_entry
 *    structures which describe all the <PMAP,VA> pairs that this
 *    page is mapped in.    this is critical for page based operations
 *    such as pmap_page_protect() [change protection on _all_ mappings
 *    of a page]
 */

/*
 * Locking
 *
 * We have the following locks that we must deal with, listed in the order
 * that they are acquired:
 *
 * pg->uobject->vmobjlock, pg->uanon->an_lock
 *
 *	For managed pages, these per-object locks are taken by the VM system
 *	before calling into the pmap module - either a read or write hold.
 *	The lock hold prevent pages from changing identity while the pmap is
 *	operating on them.  For example, the same lock is held across a call
 *	to pmap_remove() and the following call to pmap_update(), so that a
 *	page does not gain a new identity while its TLB visibility is stale.
 *
 * pmap->pm_lock
 *
 *	This lock protects the fields in the pmap structure including the
 *	non-kernel PDEs in the PDP, the PTEs, and PTPs and connected data
 *	structures.  For modifying unmanaged kernel PTEs it is not needed as
 *	kernel PDEs are never freed, and the kernel is expected to be self
 *	consistent (and the lock can't be taken for unmanaged kernel PTEs,
 *	because they can be modified from interrupt context).
 *
 * pmaps_lock
 *
 *	This lock protects the list of active pmaps (headed by "pmaps").
 *	It's acquired when adding or removing pmaps or adjusting kernel PDEs.
 *
 * pp_lock
 *
 *	This per-page lock protects PV entry lists and the embedded PV entry
 *	in each vm_page, allowing for concurrent operation on pages by
 *	different pmaps.  This is a spin mutex at IPL_VM, because at the
 *	points it is taken context switching is usually not tolerable, and
 *	spin mutexes must block out interrupts that could take kernel_lock.
 */

/* uvm_object is abused here to index pmap_pages; make assertions happy. */
#ifdef DIAGNOSTIC
#define	PMAP_DUMMY_LOCK(pm)	rw_enter(&(pm)->pm_dummy_lock, RW_WRITER)
#define	PMAP_DUMMY_UNLOCK(pm)	rw_exit(&(pm)->pm_dummy_lock)
#else
#define	PMAP_DUMMY_LOCK(pm)
#define	PMAP_DUMMY_UNLOCK(pm)
#endif

static const struct uvm_pagerops pmap_pager = {
	/* nothing */
};

const vaddr_t ptp_masks[] = PTP_MASK_INITIALIZER;
const vaddr_t ptp_frames[] = PTP_FRAME_INITIALIZER;
const int ptp_shifts[] = PTP_SHIFT_INITIALIZER;
const long nkptpmax[] = NKPTPMAX_INITIALIZER;
const long nbpd[] = NBPD_INITIALIZER;
#ifdef i386
pd_entry_t * const normal_pdes[] = PDES_INITIALIZER;
#else
pd_entry_t *normal_pdes[3];
#endif

long nkptp[] = NKPTP_INITIALIZER;

struct pmap_head pmaps;
kmutex_t pmaps_lock __cacheline_aligned;

struct pcpu_area *pcpuarea __read_mostly;

static vaddr_t pmap_maxkvaddr;

/*
 * Misc. event counters.
 */
struct evcnt pmap_iobmp_evcnt;
struct evcnt pmap_ldt_evcnt;

/*
 * PAT
 */
static bool cpu_pat_enabled __read_mostly = false;

/*
 * Global data structures
 */

static struct pmap kernel_pmap_store __cacheline_aligned; /* kernel's pmap */
struct pmap *const kernel_pmap_ptr = &kernel_pmap_store;
static rb_tree_t pmap_kernel_rb __cacheline_aligned;

struct bootspace bootspace __read_mostly;
struct slotspace slotspace __read_mostly;

/* Set to PTE_NX if supported. */
pd_entry_t pmap_pg_nx __read_mostly = 0;

/* Set to PTE_G if supported. */
pd_entry_t pmap_pg_g __read_mostly = 0;

/* Set to true if large pages are supported. */
int pmap_largepages __read_mostly = 0;

paddr_t lowmem_rsvd __read_mostly;
paddr_t avail_start __read_mostly; /* PA of first available physical page */
paddr_t avail_end __read_mostly; /* PA of last available physical page */

#ifdef XENPV
paddr_t pmap_pa_start; /* PA of first physical page for this domain */
paddr_t pmap_pa_end;   /* PA of last physical page for this domain */
#endif

#define	VM_PAGE_TO_PP(pg)	(&(pg)->mdpage.mp_pp)
#define	PMAP_CHECK_PP(pp) \
    KASSERTMSG((pp)->pp_lock.mtx_ipl._ipl == IPL_VM, "bad pmap_page %p", pp)

#define PAGE_ALIGNED(pp)	\
	__builtin_assume_aligned((void *)(pp), PAGE_SIZE)

/*
 * Other data structures
 */

static pt_entry_t protection_codes[8] __read_mostly;

static bool pmap_initialized __read_mostly = false; /* pmap_init done yet? */

/*
 * The following two vaddr_t's are used during system startup to keep track of
 * how much of the kernel's VM space we have used. Once the system is started,
 * the management of the remaining kernel VM space is turned over to the
 * kernel_map vm_map.
 */
static vaddr_t virtual_avail __read_mostly;	/* VA of first free KVA */
static vaddr_t virtual_end __read_mostly;	/* VA of last free KVA */

#ifndef XENPV
/*
 * LAPIC virtual address, and fake physical address.
 */
volatile vaddr_t local_apic_va __read_mostly;
paddr_t local_apic_pa __read_mostly;
#endif

/*
 * pool that pmap structures are allocated from
 */
struct pool_cache pmap_cache;
static int  pmap_ctor(void *, void *, int);
static void pmap_dtor(void *, void *);

/*
 * pv_page cache
 */
static struct pool_cache pmap_pvp_cache;

#ifdef __HAVE_DIRECT_MAP
vaddr_t pmap_direct_base __read_mostly;
vaddr_t pmap_direct_end __read_mostly;
#endif

#ifndef __HAVE_DIRECT_MAP
/*
 * Special VAs and the PTEs that map them
 */
static pt_entry_t *early_zero_pte;
static void pmap_vpage_cpualloc(struct cpu_info *);
#ifdef XENPV
char *early_zerop; /* also referenced from xen_locore() */
#else
static char *early_zerop;
#endif
#endif

int pmap_enter_default(pmap_t, vaddr_t, paddr_t, vm_prot_t, u_int);

/* PDP pool and its callbacks */
static struct pool pmap_pdp_pool;
static void pmap_pdp_init(pd_entry_t *);
static void pmap_pdp_fini(pd_entry_t *);

#ifdef PAE
/* need to allocate items of 4 pages */
static void *pmap_pdp_alloc(struct pool *, int);
static void pmap_pdp_free(struct pool *, void *);
static struct pool_allocator pmap_pdp_allocator = {
	.pa_alloc = pmap_pdp_alloc,
	.pa_free = pmap_pdp_free,
	.pa_pagesz = PAGE_SIZE * PDP_SIZE,
};
#endif

extern vaddr_t idt_vaddr;
extern paddr_t idt_paddr;
extern vaddr_t gdt_vaddr;
extern paddr_t gdt_paddr;
extern vaddr_t ldt_vaddr;
extern paddr_t ldt_paddr;

#ifdef i386
/* stuff to fix the pentium f00f bug */
extern vaddr_t pentium_idt_vaddr;
#endif

/* Array of freshly allocated PTPs, for pmap_get_ptp(). */
struct pmap_ptparray {
	struct vm_page *pg[PTP_LEVELS + 1];
	bool alloced[PTP_LEVELS + 1];
};

/*
 * PV entries are allocated in page-sized chunks and cached per-pmap to
 * avoid intense pressure on memory allocators.
 */

struct pv_page {
	LIST_HEAD(, pv_entry)	pvp_pves;
	LIST_ENTRY(pv_page)	pvp_list;
	long			pvp_nfree;
	struct pmap		*pvp_pmap;
};

#define	PVE_PER_PVP	((PAGE_SIZE / sizeof(struct pv_entry)) - 1)

/*
 * PV tree prototypes
 */

static int	pmap_compare_key(void *, const void *, const void *);
static int	pmap_compare_nodes(void *, const void *, const void *);

/* Read-black tree */
static const rb_tree_ops_t pmap_rbtree_ops = {
	.rbto_compare_nodes = pmap_compare_nodes,
	.rbto_compare_key = pmap_compare_key,
	.rbto_node_offset = offsetof(struct pv_entry, pve_rb),
	.rbto_context = NULL
};

/*
 * Local prototypes
 */

#ifdef __HAVE_PCPU_AREA
static void pmap_init_pcpu(void);
#endif
#ifdef __HAVE_DIRECT_MAP
static void pmap_init_directmap(struct pmap *);
#endif
#if !defined(XENPV)
static void pmap_remap_global(void);
#endif
#ifndef XENPV
static void pmap_init_lapic(void);
static void pmap_remap_largepages(void);
#endif

static int pmap_get_ptp(struct pmap *, struct pmap_ptparray *, vaddr_t, int,
    struct vm_page **);
static void pmap_unget_ptp(struct pmap *, struct pmap_ptparray *);
static void pmap_install_ptp(struct pmap *, struct pmap_ptparray *, vaddr_t,
    pd_entry_t * const *);
static struct vm_page *pmap_find_ptp(struct pmap *, vaddr_t, int);
static void pmap_freepage(struct pmap *, struct vm_page *, int);
static void pmap_free_ptp(struct pmap *, struct vm_page *, vaddr_t,
    pt_entry_t *, pd_entry_t * const *);
static bool pmap_remove_pte(struct pmap *, struct vm_page *, pt_entry_t *,
    vaddr_t);
static void pmap_remove_ptes(struct pmap *, struct vm_page *, vaddr_t, vaddr_t,
    vaddr_t);
static int pmap_pvp_ctor(void *, void *, int);
static void pmap_pvp_dtor(void *, void *);
static struct pv_entry *pmap_alloc_pv(struct pmap *);
static void pmap_free_pv(struct pmap *, struct pv_entry *);
static void pmap_drain_pv(struct pmap *);

static void pmap_alloc_level(struct pmap *, vaddr_t, long *);

static void pmap_load1(struct lwp *, struct pmap *, struct pmap *);
static void pmap_reactivate(struct pmap *);

/*
 * p m a p   h e l p e r   f u n c t i o n s
 */

static inline void
pmap_stats_update(struct pmap *pmap, int resid_diff, int wired_diff)
{

	KASSERT(cold || mutex_owned(&pmap->pm_lock));
	pmap->pm_stats.resident_count += resid_diff;
	pmap->pm_stats.wired_count += wired_diff;
}

static inline void
pmap_stats_update_bypte(struct pmap *pmap, pt_entry_t npte, pt_entry_t opte)
{
	int resid_diff = ((npte & PTE_P) ? 1 : 0) - ((opte & PTE_P) ? 1 : 0);
	int wired_diff = ((npte & PTE_WIRED) ? 1 : 0) - ((opte & PTE_WIRED) ? 1 : 0);

	KASSERT((npte & (PTE_P | PTE_WIRED)) != PTE_WIRED);
	KASSERT((opte & (PTE_P | PTE_WIRED)) != PTE_WIRED);

	pmap_stats_update(pmap, resid_diff, wired_diff);
}

/*
 * ptp_to_pmap: lookup pmap by ptp
 */
static inline struct pmap *
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

	if (pve == NULL)
		return NULL;
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
 * Return true if the pmap page has an embedded PV entry.
 */
static inline bool
pv_pte_embedded(struct pmap_page *pp)
{

	KASSERT(mutex_owned(&pp->pp_lock));
	return (bool)((vaddr_t)pp->pp_pte.pte_ptp | pp->pp_pte.pte_va);
}

/*
 * pv_pte_first, pv_pte_next: PV list iterator.
 */
static inline struct pv_pte *
pv_pte_first(struct pmap_page *pp)
{

	KASSERT(mutex_owned(&pp->pp_lock));
	if (pv_pte_embedded(pp)) {
		return &pp->pp_pte;
	}
	return pve_to_pvpte(LIST_FIRST(&pp->pp_pvlist));
}

static inline struct pv_pte *
pv_pte_next(struct pmap_page *pp, struct pv_pte *pvpte)
{

	KASSERT(mutex_owned(&pp->pp_lock));
	KASSERT(pvpte != NULL);
	if (pvpte == &pp->pp_pte) {
		return pve_to_pvpte(LIST_FIRST(&pp->pp_pvlist));
	}
	return pve_to_pvpte(LIST_NEXT(pvpte_to_pve(pvpte), pve_list));
}

static inline uint8_t
pmap_pte_to_pp_attrs(pt_entry_t pte)
{
	uint8_t ret = 0;
	if (pte & PTE_D)
		ret |= PP_ATTRS_D;
	if (pte & PTE_A)
		ret |= PP_ATTRS_A;
	if (pte & PTE_W)
		ret |= PP_ATTRS_W;
	return ret;
}

static inline pt_entry_t
pmap_pp_attrs_to_pte(uint8_t attrs)
{
	pt_entry_t pte = 0;
	if (attrs & PP_ATTRS_D)
		pte |= PTE_D;
	if (attrs & PP_ATTRS_A)
		pte |= PTE_A;
	if (attrs & PP_ATTRS_W)
		pte |= PTE_W;
	return pte;
}

/*
 * pmap_is_curpmap: is this pmap the one currently loaded [in %cr3]?
 * of course the kernel is always loaded
 */
bool
pmap_is_curpmap(struct pmap *pmap)
{
	return ((pmap == pmap_kernel()) || (pmap == curcpu()->ci_pmap));
}

inline void
pmap_reference(struct pmap *pmap)
{

	atomic_inc_uint(&pmap->pm_obj[0].uo_refs);
}

/*
 * rbtree: compare two nodes.
 */
static int
pmap_compare_nodes(void *context, const void *n1, const void *n2)
{
	const struct pv_entry *pve1 = n1;
	const struct pv_entry *pve2 = n2;

	KASSERT(pve1->pve_pte.pte_ptp == pve2->pve_pte.pte_ptp);

	if (pve1->pve_pte.pte_va < pve2->pve_pte.pte_va) {
		return -1;
	}
	if (pve1->pve_pte.pte_va > pve2->pve_pte.pte_va) {
		return 1;
	}
	return 0;
}

/*
 * rbtree: compare a node and a key.
 */
static int
pmap_compare_key(void *context, const void *n, const void *k)
{
	const struct pv_entry *pve = n;
	const vaddr_t key = (vaddr_t)k;

	if (pve->pve_pte.pte_va < key) {
		return -1;
	}
	if (pve->pve_pte.pte_va > key) {
		return 1;
	}
	return 0;
}

/*
 * pmap_ptp_range_set: abuse ptp->uanon to record minimum VA of PTE
 */
static inline void
pmap_ptp_range_set(struct vm_page *ptp, vaddr_t va)
{
	vaddr_t *min = (vaddr_t *)&ptp->uanon;

	if (va < *min) {
		*min = va;
	}
}

/*
 * pmap_ptp_range_clip: abuse ptp->uanon to clip range of PTEs to remove
 */
static inline void
pmap_ptp_range_clip(struct vm_page *ptp, vaddr_t *startva, pt_entry_t **pte)
{
	vaddr_t sclip;

	if (ptp == NULL) {
		return;
	}

	sclip = (vaddr_t)ptp->uanon;
	sclip = (*startva < sclip ? sclip : *startva);
	*pte += (sclip - *startva) / PAGE_SIZE;
	*startva = sclip;
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
 * => caller must lock pmap first (if not the kernel pmap)
 * => must be undone with pmap_unmap_ptes before returning
 * => disables kernel preemption
 */
void
pmap_map_ptes(struct pmap *pmap, struct pmap **pmap2, pd_entry_t **ptepp,
    pd_entry_t * const **pdeppp)
{
	struct pmap *curpmap;
	struct cpu_info *ci;
	lwp_t *l;

	kpreempt_disable();

	/* The kernel's pmap is always accessible. */
	if (pmap == pmap_kernel()) {
		*pmap2 = NULL;
		*ptepp = PTE_BASE;
		*pdeppp = normal_pdes;
		return;
	}

	KASSERT(mutex_owned(&pmap->pm_lock));

	l = curlwp;
	ci = l->l_cpu;
	curpmap = ci->ci_pmap;
	if (pmap == curpmap) {
		/*
		 * Already on the CPU: make it valid.  This is very
		 * often the case during exit(), when we have switched
		 * to the kernel pmap in order to destroy a user pmap.
		 */
		if (__predict_false(ci->ci_tlbstate != TLBSTATE_VALID)) {
			pmap_reactivate(pmap);
		}
		*pmap2 = NULL;
	} else {
		/*
		 * Toss current pmap from CPU and install new pmap, but keep
		 * a reference to the old one.  Dropping the reference can
		 * can block as it needs to take locks, so defer that to
		 * pmap_unmap_ptes().
		 */
		pmap_reference(pmap);
		pmap_load1(l, pmap, curpmap);
		*pmap2 = curpmap;
	}
	KASSERT(ci->ci_tlbstate == TLBSTATE_VALID);
#ifdef DIAGNOSTIC
	pmap->pm_ncsw = lwp_pctr();
#endif
	*ptepp = PTE_BASE;

#if defined(XENPV) && defined(__x86_64__)
	KASSERT(ci->ci_normal_pdes[PTP_LEVELS - 2] == L4_BASE);
	ci->ci_normal_pdes[PTP_LEVELS - 2] = pmap->pm_pdir;
	*pdeppp = ci->ci_normal_pdes;
#else
	*pdeppp = normal_pdes;
#endif
}

/*
 * pmap_unmap_ptes: unlock the PTE mapping of "pmap"
 *
 * => we cannot tolerate context switches while mapped in: assert this.
 * => reenables kernel preemption.
 * => does not unlock pmap.
 */
void
pmap_unmap_ptes(struct pmap *pmap, struct pmap * pmap2)
{
	struct cpu_info *ci;
	struct pmap *mypmap;
	struct lwp *l;

	KASSERT(kpreempt_disabled());

	/* The kernel's pmap is always accessible. */
	if (pmap == pmap_kernel()) {
		kpreempt_enable();
		return;
	}

	l = curlwp;
	ci = l->l_cpu;

	KASSERT(mutex_owned(&pmap->pm_lock));
	KASSERT(pmap->pm_ncsw == lwp_pctr());

#if defined(XENPV) && defined(__x86_64__)
	KASSERT(ci->ci_normal_pdes[PTP_LEVELS - 2] != L4_BASE);
	ci->ci_normal_pdes[PTP_LEVELS - 2] = L4_BASE;
#endif

	/* If not our own pmap, mark whatever's on the CPU now as lazy. */
	KASSERT(ci->ci_tlbstate == TLBSTATE_VALID);
	mypmap = vm_map_pmap(&l->l_proc->p_vmspace->vm_map);
	if (ci->ci_pmap == vm_map_pmap(&l->l_proc->p_vmspace->vm_map)) {
		ci->ci_want_pmapload = 0;
	} else {
		ci->ci_want_pmapload = (mypmap != pmap_kernel());
		ci->ci_tlbstate = TLBSTATE_LAZY;
	}

	/* Now safe to re-enable preemption. */
	kpreempt_enable();

	/* Toss reference to other pmap taken earlier. */
	if (pmap2 != NULL) {
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

	if ((opte ^ npte) & PTE_X)
		pmap_update_pg(va);

	/*
	 * Executability was removed on the last executable change.
	 * Reset the code segment to something conservative and
	 * let the trap handler deal with setting the right limit.
	 * We can't do that because of locking constraints on the vm map.
	 */

	if ((opte & PTE_X) && (npte & PTE_X) == 0 && va == pm->pm_hiexec) {
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
		return 0;

	pm->pm_hiexec = va;
	if (pm->pm_hiexec > I386_MAX_EXE_ADDR) {
		tf->tf_cs = GSEL(GUCODEBIG_SEL, SEL_UPL);
	} else {
		tf->tf_cs = GSEL(GUCODE_SEL, SEL_UPL);
		return 0;
	}
	return 1;
}
#endif /* !defined(__x86_64__) */

void
pat_init(struct cpu_info *ci)
{
#ifndef XENPV
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
#endif
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
			return PTE_PCD;
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
#if defined(XENPV) && defined(DOM0OPS)
	if (pa < pmap_pa_start || pa >= pmap_pa_end) {
#ifdef DEBUG
		printf_nolog("%s: pa %#" PRIxPADDR " for va %#" PRIxVADDR
		    " outside range\n", __func__, pa, va);
#endif /* DEBUG */
		npte = pa;
	} else
#endif /* XENPV && DOM0OPS */
		npte = pmap_pa2pte(pa);
	npte |= protection_codes[prot] | PTE_P | pmap_pg_g;
	npte |= pmap_pat_flags(flags);
	opte = pmap_pte_testset(pte, npte); /* zap! */

	/*
	 * XXX: make sure we are not dealing with a large page, since the only
	 * large pages created are for the kernel image, and they should never
	 * be kentered.
	 */
	KASSERTMSG(!(opte & PTE_PS), "PTE_PS va=%#"PRIxVADDR, va);

	if ((opte & (PTE_P | PTE_A)) == (PTE_P | PTE_A)) {
		/* This should not happen. */
		printf_nolog("%s: mapping already present\n", __func__);
		kpreempt_disable();
		pmap_tlb_shootdown(pmap_kernel(), va, opte, TLBSHOOT_KENTER);
		kpreempt_enable();
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
		npte |= PTE_W;
	else
		npte &= ~(PTE_W|PTE_D);

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
static void
pmap_kremove1(vaddr_t sva, vsize_t len, bool localonly)
{
	pt_entry_t *pte, opte;
	vaddr_t va, eva;

	eva = sva + len;

	kpreempt_disable();
	for (va = sva; va < eva; va += PAGE_SIZE) {
		pte = kvtopte(va);
		opte = pmap_pte_testset(pte, 0); /* zap! */
		if ((opte & (PTE_P | PTE_A)) == (PTE_P | PTE_A) && !localonly) {
			pmap_tlb_shootdown(pmap_kernel(), va, opte,
			    TLBSHOOT_KREMOVE);
		}
		KASSERTMSG((opte & PTE_PS) == 0,
		    "va %#" PRIxVADDR " is a large page", va);
		KASSERTMSG((opte & PTE_PVLIST) == 0,
		    "va %#" PRIxVADDR " is a pv tracked page", va);
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
 * for use while writing kernel crash dumps, either after panic
 * or via reboot -d.
 */
void
pmap_kremove_local(vaddr_t sva, vsize_t len)
{

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
 * pmap_bootstrap_valloc: allocate a virtual address in the bootstrap area.
 * This function is to be used before any VM system has been set up.
 *
 * The va is taken from virtual_avail.
 */
static vaddr_t
pmap_bootstrap_valloc(size_t npages)
{
	vaddr_t va = virtual_avail;
	virtual_avail += npages * PAGE_SIZE;
	return va;
}

/*
 * pmap_bootstrap_palloc: allocate a physical address in the bootstrap area.
 * This function is to be used before any VM system has been set up.
 *
 * The pa is taken from avail_start.
 */
static paddr_t
pmap_bootstrap_palloc(size_t npages)
{
	paddr_t pa = avail_start;
	avail_start += npages * PAGE_SIZE;
	return pa;
}

/*
 * pmap_bootstrap: get the system in a state where it can run with VM properly
 * enabled (called before main()). The VM system is fully init'd later.
 *
 * => on i386, locore.S has already enabled the MMU by allocating a PDP for the
 *    kernel, and nkpde PTP's for the kernel.
 * => kva_start is the first free virtual address in kernel space.
 */
void
pmap_bootstrap(vaddr_t kva_start)
{
	struct pmap *kpm;
	int i;
	vaddr_t kva;

	pmap_pg_nx = (cpu_feature[2] & CPUID_NOX ? PTE_NX : 0);

	/*
	 * Set up our local static global vars that keep track of the usage of
	 * KVM before kernel_map is set up.
	 */
	virtual_avail = kva_start;		/* first free KVA */
	virtual_end = VM_MAX_KERNEL_ADDRESS;	/* last KVA */

	/*
	 * Set up protection_codes: we need to be able to convert from a MI
	 * protection code (some combo of VM_PROT...) to something we can jam
	 * into a x86 PTE.
	 */
	protection_codes[VM_PROT_NONE] = pmap_pg_nx;
	protection_codes[VM_PROT_EXECUTE] = PTE_X;
	protection_codes[VM_PROT_READ] = pmap_pg_nx;
	protection_codes[VM_PROT_READ|VM_PROT_EXECUTE] = PTE_X;
	protection_codes[VM_PROT_WRITE] = PTE_W | pmap_pg_nx;
	protection_codes[VM_PROT_WRITE|VM_PROT_EXECUTE] = PTE_W | PTE_X;
	protection_codes[VM_PROT_WRITE|VM_PROT_READ] = PTE_W | pmap_pg_nx;
	protection_codes[VM_PROT_ALL] = PTE_W | PTE_X;

	/*
	 * Now we init the kernel's pmap.
	 *
	 * The kernel pmap's pm_obj is not used for much. However, in user pmaps
	 * the pm_obj contains the list of active PTPs.
	 */
	kpm = pmap_kernel();
	mutex_init(&kpm->pm_lock, MUTEX_DEFAULT, IPL_NONE);
	rw_init(&kpm->pm_dummy_lock);
	for (i = 0; i < PTP_LEVELS - 1; i++) {
		uvm_obj_init(&kpm->pm_obj[i], &pmap_pager, false, 1);
		uvm_obj_setlock(&kpm->pm_obj[i], &kpm->pm_dummy_lock);
		kpm->pm_ptphint[i] = NULL;
	}
	memset(&kpm->pm_list, 0, sizeof(kpm->pm_list));  /* pm_list not used */

	kpm->pm_pdir = (pd_entry_t *)bootspace.pdir;
	for (i = 0; i < PDP_SIZE; i++)
		kpm->pm_pdirpa[i] = PDPpaddr + PAGE_SIZE * i;

	kpm->pm_stats.wired_count = kpm->pm_stats.resident_count =
		x86_btop(kva_start - VM_MIN_KERNEL_ADDRESS);

	kcpuset_create(&kpm->pm_cpus, true);
	kcpuset_create(&kpm->pm_kernel_cpus, true);

	kpm->pm_ldt = NULL;
	kpm->pm_ldt_sel = GSYSSEL(GLDT_SEL, SEL_KPL);

	/*
	 * the above is just a rough estimate and not critical to the proper
	 * operation of the system.
	 */

#if !defined(XENPV)
	/*
	 * Begin to enable global TLB entries if they are supported: add PTE_G
	 * attribute to already mapped kernel pages. Do that only if SVS is
	 * disabled.
	 *
	 * The G bit has no effect until the CR4_PGE bit is set in CR4, which
	 * happens later in cpu_init().
	 */
#ifdef SVS
	if (!svs_enabled && (cpu_feature[0] & CPUID_PGE)) {
#else
	if (cpu_feature[0] & CPUID_PGE) {
#endif
		pmap_pg_g = PTE_G;
		pmap_remap_global();
	}
#endif

#ifndef XENPV
	/*
	 * Enable large pages if they are supported.
	 */
	if (cpu_feature[0] & CPUID_PSE) {
		lcr4(rcr4() | CR4_PSE);	/* enable hardware (via %cr4) */
		pmap_largepages = 1;	/* enable software */

		/*
		 * The TLB must be flushed after enabling large pages on Pentium
		 * CPUs, according to section 3.6.2.2 of "Intel Architecture
		 * Software Developer's Manual, Volume 3: System Programming".
		 */
		tlbflushg();

		/* Remap the kernel. */
		pmap_remap_largepages();
	}
	pmap_init_lapic();
#endif /* !XENPV */

#ifdef __HAVE_PCPU_AREA
	pmap_init_pcpu();
#endif

#ifdef __HAVE_DIRECT_MAP
	pmap_init_directmap(kpm);
#else
	pmap_vpage_cpualloc(&cpu_info_primary);

	if (VM_MIN_KERNEL_ADDRESS == KERNBASE) { /* i386 */
		early_zerop = (void *)cpu_info_primary.vpage[VPAGE_ZER];
		early_zero_pte = cpu_info_primary.vpage_pte[VPAGE_ZER];
	} else { /* amd64 */
		/*
		 * zero_pte is stuck at the end of mapped space for the kernel
		 * image (disjunct from kva space). This is done so that it
		 * can safely be used in pmap_growkernel (pmap_get_physpage),
		 * when it's called for the first time.
		 * XXXfvdl fix this for MULTIPROCESSOR later.
		 */
#ifdef XENPV
		/* early_zerop initialized in xen_locore() */
#else
		early_zerop = (void *)bootspace.spareva;
#endif
		early_zero_pte = PTE_BASE + pl1_i((vaddr_t)early_zerop);
	}
#endif

#if defined(XENPV) && defined(__x86_64__)
	extern vaddr_t xen_dummy_page;
	paddr_t xen_dummy_user_pgd;

	/*
	 * We want a dummy page directory for Xen: when deactivating a pmap,
	 * Xen will still consider it active. So we set user PGD to this one
	 * to lift all protection on the now inactive page tables set.
	 */
	xen_dummy_user_pgd = xen_dummy_page - KERNBASE;

	/* Zero fill it, the less checks in Xen it requires the better */
	memset(PAGE_ALIGNED(xen_dummy_user_pgd + KERNBASE), 0, PAGE_SIZE);
	/* Mark read-only */
	HYPERVISOR_update_va_mapping(xen_dummy_user_pgd + KERNBASE,
	    pmap_pa2pte(xen_dummy_user_pgd) | PTE_P | pmap_pg_nx,
	    UVMF_INVLPG);
	/* Pin as L4 */
	xpq_queue_pin_l4_table(xpmap_ptom_masked(xen_dummy_user_pgd));
#endif

	/*
	 * Allocate space for the IDT, GDT and LDT.
	 */
	idt_vaddr = pmap_bootstrap_valloc(1);
	idt_paddr = pmap_bootstrap_palloc(1);

	gdt_vaddr = pmap_bootstrap_valloc(1);
	gdt_paddr = pmap_bootstrap_palloc(1);

#ifdef __HAVE_PCPU_AREA
	ldt_vaddr = (vaddr_t)&pcpuarea->ldt;
#else
	ldt_vaddr = pmap_bootstrap_valloc(1);
#endif
	ldt_paddr = pmap_bootstrap_palloc(1);

#if !defined(__x86_64__)
	/* pentium f00f bug stuff */
	pentium_idt_vaddr = pmap_bootstrap_valloc(1);
#endif

#if defined(XENPVHVM)
	/* XXX: move to hypervisor.c with appropriate API adjustments */
	extern paddr_t HYPERVISOR_shared_info_pa;
	extern volatile struct xencons_interface *xencons_interface; /* XXX */
	extern struct xenstore_domain_interface *xenstore_interface; /* XXX */

	if (vm_guest != VM_GUEST_XENPVH) {
		HYPERVISOR_shared_info = (void *) pmap_bootstrap_valloc(1);
		HYPERVISOR_shared_info_pa = pmap_bootstrap_palloc(1);
	}
	xencons_interface = (void *) pmap_bootstrap_valloc(1);
	xenstore_interface = (void *) pmap_bootstrap_valloc(1);
#endif
	/*
	 * Now we reserve some VM for mapping pages when doing a crash dump.
	 */
	virtual_avail = reserve_dumppages(virtual_avail);

	/*
	 * Init the global lock and global list.
	 */
	mutex_init(&pmaps_lock, MUTEX_DEFAULT, IPL_NONE);
	LIST_INIT(&pmaps);

	/*
	 * Ensure the TLB is sync'd with reality by flushing it...
	 */
	tlbflushg();

	/*
	 * Calculate pmap_maxkvaddr from nkptp[].
	 */
	kva = VM_MIN_KERNEL_ADDRESS;
	for (i = PTP_LEVELS - 1; i >= 1; i--) {
		kva += nkptp[i] * nbpd[i];
	}
	pmap_maxkvaddr = kva;
}

#ifndef XENPV
static void
pmap_init_lapic(void)
{
	/*
	 * On CPUs that have no LAPIC, local_apic_va is never kentered. But our
	 * x86 implementation relies a lot on this address to be valid; so just
	 * allocate a fake physical page that will be kentered into
	 * local_apic_va by machdep.
	 *
	 * If the LAPIC is present, the va will be remapped somewhere else
	 * later in lapic_map.
	 */
	local_apic_va = pmap_bootstrap_valloc(1);
	local_apic_pa = pmap_bootstrap_palloc(1);
}
#endif

#ifdef __x86_64__
static size_t
pmap_pagetree_nentries_range(vaddr_t startva, vaddr_t endva, size_t pgsz)
{
	size_t npages;
	npages = (roundup(endva, pgsz) / pgsz) -
	    (rounddown(startva, pgsz) / pgsz);
	return npages;
}
#endif

#if defined(__HAVE_DIRECT_MAP) || defined(KASAN) || defined(KMSAN)
static inline void
slotspace_copy(int type, pd_entry_t *dst, pd_entry_t *src)
{
	size_t sslot = slotspace.area[type].sslot;
	size_t nslot = slotspace.area[type].nslot;

	memcpy(&dst[sslot], &src[sslot], nslot * sizeof(pd_entry_t));
}
#endif

#ifdef __x86_64__
/*
 * Randomize the location of an area. We count the holes in the VM space. We
 * randomly select one hole, and then randomly select an area within that hole.
 * Finally we update the associated entry in the slotspace structure.
 */
vaddr_t
slotspace_rand(int type, size_t sz, size_t align, size_t randhole,
    vaddr_t randva)
{
	struct {
		int start;
		int end;
	} holes[SLSPACE_NAREAS+1];
	size_t i, nholes, hole;
	size_t startsl, endsl, nslots, winsize;
	vaddr_t startva, va;

	sz = roundup(sz, align);

	/*
	 * Take one more slot with +NBPD_L4, because we may end up choosing
	 * an area that crosses slots:
	 *     +------+------+------+
	 *     | Slot | Slot | Slot |
	 *     +------+------+------+
	 *        [Chosen Area]
	 * And in that case we must take into account the additional slot
	 * consumed.
	 */
	nslots = roundup(sz+NBPD_L4, NBPD_L4) / NBPD_L4;

	/* Get the holes. */
	nholes = 0;
	size_t curslot = 0 + 256; /* end of SLAREA_USER */
	while (1) {
		/*
		 * Find the first occupied slot after the current one.
		 * The area between the two is a hole.
		 */
		size_t minsslot = 512;
		size_t minnslot = 0;
		for (i = 0; i < SLSPACE_NAREAS; i++) {
			if (!slotspace.area[i].active)
				continue;
			if (slotspace.area[i].sslot >= curslot &&
			    slotspace.area[i].sslot < minsslot) {
				minsslot = slotspace.area[i].sslot;
				minnslot = slotspace.area[i].nslot;
			}
		}

		/* No hole anymore, stop here. */
		if (minsslot == 512) {
			break;
		}

		/* Register the hole. */
		if (minsslot - curslot >= nslots) {
			holes[nholes].start = curslot;
			holes[nholes].end = minsslot;
			nholes++;
		}

		/* Skip that hole, and iterate again. */
		curslot = minsslot + minnslot;
	}

	if (nholes == 0) {
		panic("%s: impossible", __func__);
	}

	/* Select a hole. */
	hole = randhole;
#ifdef NO_X86_ASLR
	hole = 0;
#endif
	hole %= nholes;
	startsl = holes[hole].start;
	endsl = holes[hole].end;
	startva = VA_SIGN_NEG(startsl * NBPD_L4);

	/* Select an area within the hole. */
	va = randva;
#ifdef NO_X86_ASLR
	va = 0;
#endif
	winsize = ((endsl - startsl) * NBPD_L4) - sz;
	va %= winsize;
	va = rounddown(va, align);
	va += startva;

	/* Update the entry. */
	slotspace.area[type].sslot = pl4_i(va);
	slotspace.area[type].nslot =
	    pmap_pagetree_nentries_range(va, va+sz, NBPD_L4);
	slotspace.area[type].active = true;

	return va;
}
#endif

#ifdef __HAVE_PCPU_AREA
static void
pmap_init_pcpu(void)
{
	const vaddr_t startva = PMAP_PCPU_BASE;
	size_t nL4e, nL3e, nL2e, nL1e;
	size_t L4e_idx, L3e_idx, L2e_idx, L1e_idx __diagused;
	paddr_t pa;
	vaddr_t endva;
	vaddr_t tmpva;
	pt_entry_t *pte;
	size_t size;
	int i;

	const pd_entry_t pteflags = PTE_P | PTE_W | pmap_pg_nx;

	size = sizeof(struct pcpu_area);

	endva = startva + size;

	/* We will use this temporary va. */
	tmpva = bootspace.spareva;
	pte = PTE_BASE + pl1_i(tmpva);

	/* Build L4 */
	L4e_idx = pl4_i(startva);
	nL4e = pmap_pagetree_nentries_range(startva, endva, NBPD_L4);
	KASSERT(nL4e  == 1);
	for (i = 0; i < nL4e; i++) {
		KASSERT(L4_BASE[L4e_idx+i] == 0);

		pa = pmap_bootstrap_palloc(1);
		*pte = (pa & PTE_FRAME) | pteflags;
		pmap_update_pg(tmpva);
		memset(PAGE_ALIGNED(tmpva), 0, PAGE_SIZE);

		L4_BASE[L4e_idx+i] = pa | pteflags | PTE_A;
	}

	/* Build L3 */
	L3e_idx = pl3_i(startva);
	nL3e = pmap_pagetree_nentries_range(startva, endva, NBPD_L3);
	for (i = 0; i < nL3e; i++) {
		KASSERT(L3_BASE[L3e_idx+i] == 0);

		pa = pmap_bootstrap_palloc(1);
		*pte = (pa & PTE_FRAME) | pteflags;
		pmap_update_pg(tmpva);
		memset(PAGE_ALIGNED(tmpva), 0, PAGE_SIZE);

		L3_BASE[L3e_idx+i] = pa | pteflags | PTE_A;
	}

	/* Build L2 */
	L2e_idx = pl2_i(startva);
	nL2e = pmap_pagetree_nentries_range(startva, endva, NBPD_L2);
	for (i = 0; i < nL2e; i++) {

		KASSERT(L2_BASE[L2e_idx+i] == 0);

		pa = pmap_bootstrap_palloc(1);
		*pte = (pa & PTE_FRAME) | pteflags;
		pmap_update_pg(tmpva);
		memset(PAGE_ALIGNED(tmpva), 0, PAGE_SIZE);

		L2_BASE[L2e_idx+i] = pa | pteflags | PTE_A;
	}

	/* Build L1 */
	L1e_idx = pl1_i(startva);
	nL1e = pmap_pagetree_nentries_range(startva, endva, NBPD_L1);
	for (i = 0; i < nL1e; i++) {
		/*
		 * Nothing to do, the PTEs will be entered via
		 * pmap_kenter_pa.
		 */
		KASSERT(L1_BASE[L1e_idx+i] == 0);
	}

	*pte = 0;
	pmap_update_pg(tmpva);

	pcpuarea = (struct pcpu_area *)startva;

	tlbflush();
}
#endif

#ifdef __HAVE_DIRECT_MAP
/*
 * Create the amd64 direct map. Called only once at boot time. We map all of
 * the physical memory contiguously using 2MB large pages, with RW permissions.
 * However there is a hole: the kernel is mapped with RO permissions.
 */
static void
pmap_init_directmap(struct pmap *kpm)
{
	extern phys_ram_seg_t mem_clusters[];
	extern int mem_cluster_cnt;

	vaddr_t startva;
	size_t nL4e, nL3e, nL2e;
	size_t L4e_idx, L3e_idx, L2e_idx;
	size_t spahole, epahole;
	paddr_t lastpa, pa;
	vaddr_t endva;
	vaddr_t tmpva;
	pt_entry_t *pte;
	phys_ram_seg_t *mc;
	int i;
	size_t randhole;
	vaddr_t randva;

	const pd_entry_t pteflags = PTE_P | PTE_W | pmap_pg_nx;
	const pd_entry_t holepteflags = PTE_P | pmap_pg_nx;

	CTASSERT(NL4_SLOT_DIRECT * NBPD_L4 == MAXPHYSMEM);

	spahole = roundup(bootspace.head.pa, NBPD_L2);
	epahole = rounddown(bootspace.boot.pa, NBPD_L2);

	/* Get the last physical address available */
	lastpa = 0;
	for (i = 0; i < mem_cluster_cnt; i++) {
		mc = &mem_clusters[i];
		lastpa = MAX(lastpa, mc->start + mc->size);
	}

	/*
	 * x86_add_cluster should have truncated the memory to MAXPHYSMEM.
	 */
	if (lastpa > MAXPHYSMEM) {
		panic("pmap_init_directmap: lastpa incorrect");
	}

	entropy_extract(&randhole, sizeof randhole, 0);
	entropy_extract(&randva, sizeof randva, 0);
	startva = slotspace_rand(SLAREA_DMAP, lastpa, NBPD_L2,
	    randhole, randva);
	endva = startva + lastpa;

	/* We will use this temporary va. */
	tmpva = bootspace.spareva;
	pte = PTE_BASE + pl1_i(tmpva);

	/* Build L4 */
	L4e_idx = pl4_i(startva);
	nL4e = pmap_pagetree_nentries_range(startva, endva, NBPD_L4);
	KASSERT(nL4e <= NL4_SLOT_DIRECT);
	for (i = 0; i < nL4e; i++) {
		KASSERT(L4_BASE[L4e_idx+i] == 0);

		pa = pmap_bootstrap_palloc(1);
		*pte = (pa & PTE_FRAME) | pteflags;
		pmap_update_pg(tmpva);
		memset(PAGE_ALIGNED(tmpva), 0, PAGE_SIZE);

		L4_BASE[L4e_idx+i] = pa | pteflags | PTE_A;
	}

	/* Build L3 */
	L3e_idx = pl3_i(startva);
	nL3e = pmap_pagetree_nentries_range(startva, endva, NBPD_L3);
	for (i = 0; i < nL3e; i++) {
		KASSERT(L3_BASE[L3e_idx+i] == 0);

		pa = pmap_bootstrap_palloc(1);
		*pte = (pa & PTE_FRAME) | pteflags;
		pmap_update_pg(tmpva);
		memset(PAGE_ALIGNED(tmpva), 0, PAGE_SIZE);

		L3_BASE[L3e_idx+i] = pa | pteflags | PTE_A;
	}

	/* Build L2 */
	L2e_idx = pl2_i(startva);
	nL2e = pmap_pagetree_nentries_range(startva, endva, NBPD_L2);
	for (i = 0; i < nL2e; i++) {
		KASSERT(L2_BASE[L2e_idx+i] == 0);

		pa = (paddr_t)(i * NBPD_L2);

		if (spahole <= pa && pa < epahole) {
			L2_BASE[L2e_idx+i] = pa | holepteflags | PTE_A |
			    PTE_PS | pmap_pg_g;
		} else {
			L2_BASE[L2e_idx+i] = pa | pteflags | PTE_A |
			    PTE_PS | pmap_pg_g;
		}
	}

	*pte = 0;
	pmap_update_pg(tmpva);

	pmap_direct_base = startva;
	pmap_direct_end = endva;

	tlbflush();
}
#endif /* __HAVE_DIRECT_MAP */

#if !defined(XENPV)
/*
 * Remap all of the virtual pages created so far with the PTE_G bit.
 */
static void
pmap_remap_global(void)
{
	vaddr_t kva, kva_end;
	unsigned long p1i;
	size_t i;

	/* head */
	kva = bootspace.head.va;
	kva_end = kva + bootspace.head.sz;
	for ( ; kva < kva_end; kva += PAGE_SIZE) {
		p1i = pl1_i(kva);
		if (pmap_valid_entry(PTE_BASE[p1i]))
			PTE_BASE[p1i] |= pmap_pg_g;
	}

	/* kernel segments */
	for (i = 0; i < BTSPACE_NSEGS; i++) {
		if (bootspace.segs[i].type == BTSEG_NONE) {
			continue;
		}
		kva = bootspace.segs[i].va;
		kva_end = kva + bootspace.segs[i].sz;
		for ( ; kva < kva_end; kva += PAGE_SIZE) {
			p1i = pl1_i(kva);
			if (pmap_valid_entry(PTE_BASE[p1i]))
				PTE_BASE[p1i] |= pmap_pg_g;
		}
	}

	/* boot space */
	kva = bootspace.boot.va;
	kva_end = kva + bootspace.boot.sz;
	for ( ; kva < kva_end; kva += PAGE_SIZE) {
		p1i = pl1_i(kva);
		if (pmap_valid_entry(PTE_BASE[p1i]))
			PTE_BASE[p1i] |= pmap_pg_g;
	}
}
#endif

#ifndef XENPV
/*
 * Remap several kernel segments with large pages. We cover as many pages as we
 * can. Called only once at boot time, if the CPU supports large pages.
 */
static void
pmap_remap_largepages(void)
{
	pd_entry_t *pde;
	vaddr_t kva, kva_end;
	paddr_t pa;
	size_t i;

	/* Remap the kernel text using large pages. */
	for (i = 0; i < BTSPACE_NSEGS; i++) {
		if (bootspace.segs[i].type != BTSEG_TEXT) {
			continue;
		}
		kva = roundup(bootspace.segs[i].va, NBPD_L2);
		if (kva < bootspace.segs[i].va) {
			continue;
		}
		kva_end = rounddown(bootspace.segs[i].va +
			bootspace.segs[i].sz, NBPD_L2);
		pa = roundup(bootspace.segs[i].pa, NBPD_L2);
		for (/* */; kva < kva_end; kva += NBPD_L2, pa += NBPD_L2) {
			pde = &L2_BASE[pl2_i(kva)];
			*pde = pa | pmap_pg_g | PTE_PS | PTE_P;
			tlbflushg();
		}
	}

	/* Remap the kernel rodata using large pages. */
	for (i = 0; i < BTSPACE_NSEGS; i++) {
		if (bootspace.segs[i].type != BTSEG_RODATA) {
			continue;
		}
		kva = roundup(bootspace.segs[i].va, NBPD_L2);
		if (kva < bootspace.segs[i].va) {
			continue;
		}
		kva_end = rounddown(bootspace.segs[i].va +
			bootspace.segs[i].sz, NBPD_L2);
		pa = roundup(bootspace.segs[i].pa, NBPD_L2);
		for (/* */; kva < kva_end; kva += NBPD_L2, pa += NBPD_L2) {
			pde = &L2_BASE[pl2_i(kva)];
			*pde = pa | pmap_pg_g | PTE_PS | pmap_pg_nx | PTE_P;
			tlbflushg();
		}
	}

	/* Remap the kernel data+bss using large pages. */
	for (i = 0; i < BTSPACE_NSEGS; i++) {
		if (bootspace.segs[i].type != BTSEG_DATA) {
			continue;
		}
		kva = roundup(bootspace.segs[i].va, NBPD_L2);
		if (kva < bootspace.segs[i].va) {
			continue;
		}
		kva_end = rounddown(bootspace.segs[i].va +
			bootspace.segs[i].sz, NBPD_L2);
		pa = roundup(bootspace.segs[i].pa, NBPD_L2);
		for (/* */; kva < kva_end; kva += NBPD_L2, pa += NBPD_L2) {
			pde = &L2_BASE[pl2_i(kva)];
			*pde = pa | pmap_pg_g | PTE_PS | pmap_pg_nx | PTE_W | PTE_P;
			tlbflushg();
		}
	}
}
#endif /* !XENPV */

/*
 * pmap_init: called from uvm_init, our job is to get the pmap system ready
 * to manage mappings.
 */
void
pmap_init(void)
{
	int flags;

	/*
	 * initialize caches.
	 */

	pool_cache_bootstrap(&pmap_cache, sizeof(struct pmap), COHERENCY_UNIT,
	    0, 0, "pmappl", NULL, IPL_NONE, pmap_ctor, pmap_dtor, NULL);

#ifdef XENPV
	/*
	 * pool_cache(9) should not touch cached objects, since they
	 * are pinned on xen and R/O for the domU
	 */
	flags = PR_NOTOUCH;
#else
	flags = 0;
#endif

#ifdef PAE
	pool_init(&pmap_pdp_pool, PAGE_SIZE * PDP_SIZE, 0, 0, flags,
	    "pdppl", &pmap_pdp_allocator, IPL_NONE);
#else
	pool_init(&pmap_pdp_pool, PAGE_SIZE, 0, 0, flags,
	    "pdppl", NULL, IPL_NONE);
#endif
	pool_cache_bootstrap(&pmap_pvp_cache, PAGE_SIZE, PAGE_SIZE,
	     0, 0, "pvpage", &pool_allocator_kmem,
	    IPL_NONE, pmap_pvp_ctor, pmap_pvp_dtor, NULL);

	pmap_tlb_init();

	/* XXX: Since cpu_hatch() is only for secondary CPUs. */
	pmap_tlb_cpu_init(curcpu());

	evcnt_attach_dynamic(&pmap_iobmp_evcnt, EVCNT_TYPE_MISC,
	    NULL, "x86", "io bitmap copy");
	evcnt_attach_dynamic(&pmap_ldt_evcnt, EVCNT_TYPE_MISC,
	    NULL, "x86", "ldt sync");

	/*
	 * The kernel doesn't keep track of PTPs, so there's nowhere handy
	 * to hang a tree of pv_entry records.  Dynamically allocated
	 * pv_entry lists are not heavily used in the kernel's pmap (the
	 * usual case is embedded), so cop out and use a single RB tree
	 * to cover them.
	 */
	rb_tree_init(&pmap_kernel_rb, &pmap_rbtree_ops);

	/*
	 * done: pmap module is up (and ready for business)
	 */

	pmap_initialized = true;
}

#ifndef XENPV
/*
 * pmap_cpu_init_late: perform late per-CPU initialization.
 */
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

#ifndef __HAVE_DIRECT_MAP
CTASSERT(CACHE_LINE_SIZE > sizeof(pt_entry_t));
CTASSERT(CACHE_LINE_SIZE % sizeof(pt_entry_t) == 0);

static void
pmap_vpage_cpualloc(struct cpu_info *ci)
{
	bool primary = (ci == &cpu_info_primary);
	size_t i, npages;
	vaddr_t vabase;
	vsize_t vrange;

	npages = (CACHE_LINE_SIZE / sizeof(pt_entry_t));
	KASSERT(npages >= VPAGE_MAX);
	vrange = npages * PAGE_SIZE;

	if (primary) {
		while ((vabase = pmap_bootstrap_valloc(1)) % vrange != 0) {
			/* Waste some pages to align properly */
		}
		/* The base is aligned, allocate the rest (contiguous) */
		pmap_bootstrap_valloc(npages - 1);
	} else {
		vabase = uvm_km_alloc(kernel_map, vrange, vrange,
		    UVM_KMF_VAONLY);
		if (vabase == 0) {
			panic("%s: failed to allocate tmp VA for CPU %d\n",
			    __func__, cpu_index(ci));
		}
	}

	KASSERT((vaddr_t)&PTE_BASE[pl1_i(vabase)] % CACHE_LINE_SIZE == 0);

	for (i = 0; i < VPAGE_MAX; i++) {
		ci->vpage[i] = vabase + i * PAGE_SIZE;
		ci->vpage_pte[i] = PTE_BASE + pl1_i(ci->vpage[i]);
	}
}

void
pmap_vpage_cpu_init(struct cpu_info *ci)
{
	if (ci == &cpu_info_primary) {
		/* cpu0 already taken care of in pmap_bootstrap */
		return;
	}

	pmap_vpage_cpualloc(ci);
}
#endif

/*
 * p v _ e n t r y   f u n c t i o n s
 */

/*
 * pmap_pvp_dtor: pool_cache constructor for PV pages.
 */
static int
pmap_pvp_ctor(void *arg, void *obj, int flags)
{
	struct pv_page *pvp = (struct pv_page *)obj;
	struct pv_entry *pve = (struct pv_entry *)obj + 1;
	struct pv_entry *maxpve = pve + PVE_PER_PVP;

	KASSERT(sizeof(struct pv_page) <= sizeof(struct pv_entry));
	KASSERT(trunc_page((vaddr_t)obj) == (vaddr_t)obj);

	LIST_INIT(&pvp->pvp_pves);
	pvp->pvp_nfree = PVE_PER_PVP;
	pvp->pvp_pmap = NULL;

	for (; pve < maxpve; pve++) {
		LIST_INSERT_HEAD(&pvp->pvp_pves, pve, pve_list);
	}

	return 0;
}

/*
 * pmap_pvp_dtor: pool_cache destructor for PV pages.
 */
static void
pmap_pvp_dtor(void *arg, void *obj)
{
	struct pv_page *pvp __diagused = obj;

	KASSERT(pvp->pvp_pmap == NULL);
	KASSERT(pvp->pvp_nfree == PVE_PER_PVP);
}

/*
 * pmap_alloc_pv: allocate a PV entry (likely cached with pmap).
 */
static struct pv_entry *
pmap_alloc_pv(struct pmap *pmap)
{
	struct pv_entry *pve;
	struct pv_page *pvp;

	KASSERT(mutex_owned(&pmap->pm_lock));

	if (__predict_false((pvp = LIST_FIRST(&pmap->pm_pvp_part)) == NULL)) {
		if ((pvp = LIST_FIRST(&pmap->pm_pvp_full)) != NULL) {
			LIST_REMOVE(pvp, pvp_list);
		} else {
			pvp = pool_cache_get(&pmap_pvp_cache, PR_NOWAIT);
		}
		if (__predict_false(pvp == NULL)) {
			return NULL;
		}
		/* full -> part */
		LIST_INSERT_HEAD(&pmap->pm_pvp_part, pvp, pvp_list);
		pvp->pvp_pmap = pmap;
	}

	KASSERT(pvp->pvp_pmap == pmap);
	KASSERT(pvp->pvp_nfree > 0);

	pve = LIST_FIRST(&pvp->pvp_pves);
	LIST_REMOVE(pve, pve_list);
	pvp->pvp_nfree--;

	if (__predict_false(pvp->pvp_nfree == 0)) {
		/* part -> empty */
		KASSERT(LIST_EMPTY(&pvp->pvp_pves));
		LIST_REMOVE(pvp, pvp_list);
		LIST_INSERT_HEAD(&pmap->pm_pvp_empty, pvp, pvp_list);
	} else {
		KASSERT(!LIST_EMPTY(&pvp->pvp_pves));
	}

	return pve;
}

/*
 * pmap_free_pv: delayed free of a PV entry.
 */
static void
pmap_free_pv(struct pmap *pmap, struct pv_entry *pve)
{
	struct pv_page *pvp = (struct pv_page *)trunc_page((vaddr_t)pve);

	KASSERT(mutex_owned(&pmap->pm_lock));
	KASSERT(pvp->pvp_pmap == pmap);
	KASSERT(pvp->pvp_nfree >= 0);

	LIST_INSERT_HEAD(&pvp->pvp_pves, pve, pve_list);
	pvp->pvp_nfree++;

	if (__predict_false(pvp->pvp_nfree == 1)) {
		/* empty -> part */
		LIST_REMOVE(pvp, pvp_list);
		LIST_INSERT_HEAD(&pmap->pm_pvp_part, pvp, pvp_list);
	} else if (__predict_false(pvp->pvp_nfree == PVE_PER_PVP)) {
		/* part -> full */
		LIST_REMOVE(pvp, pvp_list);
		LIST_INSERT_HEAD(&pmap->pm_pvp_full, pvp, pvp_list);
	}
}

/*
 * pmap_drain_pv: free full PV pages.
 */
static void
pmap_drain_pv(struct pmap *pmap)
{
	struct pv_page *pvp;

	KASSERT(mutex_owned(&pmap->pm_lock));

	while ((pvp = LIST_FIRST(&pmap->pm_pvp_full)) != NULL) {
		LIST_REMOVE(pvp, pvp_list);
		KASSERT(pvp->pvp_pmap == pmap);
		KASSERT(pvp->pvp_nfree == PVE_PER_PVP);
		pvp->pvp_pmap = NULL;
		pool_cache_put(&pmap_pvp_cache, pvp);
	}
}

/*
 * pmap_check_pv: verify {VA, PTP} pair is either tracked/untracked by page
 */
static void
pmap_check_pv(struct pmap *pmap, struct vm_page *ptp, struct pmap_page *pp,
    vaddr_t va, bool tracked)
{
#ifdef DEBUG
	struct pv_pte *pvpte;

	PMAP_CHECK_PP(pp);

	mutex_spin_enter(&pp->pp_lock);
	for (pvpte = pv_pte_first(pp); pvpte; pvpte = pv_pte_next(pp, pvpte)) {
		if (pvpte->pte_ptp == ptp && pvpte->pte_va == va) {
			break;
		}
	}
	mutex_spin_exit(&pp->pp_lock);

	if (pvpte && !tracked) {
		panic("pmap_check_pv: %p/%lx found on pp %p", ptp, va, pp);
	} else if (!pvpte && tracked) {
		panic("pmap_check_pv: %p/%lx missing on pp %p", ptp, va, pp);
	}
#endif
}

/*
 * pmap_treelookup_pv: search the PV tree for a dynamic entry
 *
 * => pmap must be locked
 */
static struct pv_entry *
pmap_treelookup_pv(const struct pmap *pmap, const struct vm_page *ptp,
    const rb_tree_t *tree, const vaddr_t va)
{
	struct pv_entry *pve;
	rb_node_t *node;

	/*
	 * Inlined lookup tailored for exactly what's needed here that is
	 * quite a bit faster than using rb_tree_find_node().
	 */
	for (node = tree->rbt_root;;) {
		if (__predict_false(RB_SENTINEL_P(node))) {
			return NULL;
		}
		pve = (struct pv_entry *)
		    ((uintptr_t)node - offsetof(struct pv_entry, pve_rb));
		if (pve->pve_pte.pte_va == va) {
			KASSERT(pve->pve_pte.pte_ptp == ptp);
			return pve;
		}
		node = node->rb_nodes[pve->pve_pte.pte_va < va];
	}
}

/*
 * pmap_lookup_pv: look up a non-embedded pv entry for the given pmap
 *
 * => a PV entry must be known present (doesn't check for existence)
 * => pmap must be locked
 */
static struct pv_entry *
pmap_lookup_pv(const struct pmap *pmap, const struct vm_page *ptp,
    const struct pmap_page * const old_pp, const vaddr_t va)
{
	struct pv_entry *pve;
	const rb_tree_t *tree;

	KASSERT(mutex_owned(&pmap->pm_lock));
	KASSERT(ptp != NULL || pmap == pmap_kernel());

	/*
	 * [This mostly deals with the case of process-private pages, i.e.
	 * anonymous memory allocations or COW.]
	 *
	 * If the page is tracked with an embedded entry then the tree
	 * lookup can be avoided.  It's safe to check for this specific
	 * set of values without pp_lock because both will only ever be
	 * set together for this pmap.
	 *
	 */
	if (atomic_load_relaxed(&old_pp->pp_pte.pte_ptp) == ptp &&
	    atomic_load_relaxed(&old_pp->pp_pte.pte_va) == va) {
		return NULL;
	}

	/*
	 * [This mostly deals with shared mappings, for example shared libs
	 * and executables.]
	 *
	 * Optimise for pmap_remove_ptes() which works by ascending scan:
	 * look at the lowest numbered node in the tree first.  The tree is
	 * known non-empty because of the check above.  For short lived
	 * processes where pmap_remove() isn't used much this gets close to
	 * a 100% hit rate.
	 */
	tree = (ptp != NULL ? &VM_PAGE_TO_PP(ptp)->pp_rb : &pmap_kernel_rb);
	KASSERT(!RB_SENTINEL_P(tree->rbt_root));
	pve = (struct pv_entry *)
	    ((uintptr_t)tree->rbt_minmax[RB_DIR_LEFT] -
	    offsetof(struct pv_entry, pve_rb));
	if (__predict_true(pve->pve_pte.pte_va == va)) {
		KASSERT(pve->pve_pte.pte_ptp == ptp);
		return pve;
	}

	/* Search the RB tree for the key (uncommon). */
	return pmap_treelookup_pv(pmap, ptp, tree, va);
}

/*
 * pmap_enter_pv: enter a mapping onto a pmap_page lst
 *
 * => pmap must be locked
 * => does NOT insert dynamic entries to tree (pmap_enter() does later)
 */
static int
pmap_enter_pv(struct pmap *pmap, struct pmap_page *pp, struct vm_page *ptp,
    vaddr_t va, struct pv_entry **new_pve, struct pv_entry **old_pve,
    bool *samepage, bool *new_embedded, rb_tree_t *tree)
{
	struct pv_entry *pve;
	int error;

	KASSERT(mutex_owned(&pmap->pm_lock));
	KASSERT(ptp_to_pmap(ptp) == pmap);
	KASSERT(ptp == NULL || ptp->uobject != NULL);
	KASSERT(ptp == NULL || ptp_va2o(va, 1) == ptp->offset);
	PMAP_CHECK_PP(pp);

	/*
	 * If entering the same page and it's already tracked with an
	 * embedded entry, we can avoid the expense below.  It's safe
	 * to check for this very specific set of values without a lock
	 * because both will only ever be set together for this pmap.
	 */
	if (atomic_load_relaxed(&pp->pp_pte.pte_ptp) == ptp &&
	    atomic_load_relaxed(&pp->pp_pte.pte_va) == va) {
		*samepage = true;
		pmap_check_pv(pmap, ptp, pp, va, true);
		return 0;
	}

	/*
	 * Check for an existing dynamic mapping at this address.  If it's
	 * for the same page, then it will be reused and nothing needs to be
	 * changed.
	 */
	*old_pve = pmap_treelookup_pv(pmap, ptp, tree, va);
	if (*old_pve != NULL && (*old_pve)->pve_pp == pp) {
		*samepage = true;
		pmap_check_pv(pmap, ptp, pp, va, true);
		return 0;
	}

	/*
	 * Need to put a new mapping in place.  Grab a spare pv_entry in
	 * case it's needed; won't know for sure until the lock is taken.
	 */
	if (pmap->pm_pve == NULL) {
		pmap->pm_pve = pmap_alloc_pv(pmap);
	}

	error = 0;
	pmap_check_pv(pmap, ptp, pp, va, false);
	mutex_spin_enter(&pp->pp_lock);
	if (!pv_pte_embedded(pp)) {
		/*
		 * Embedded PV tracking available - easy.
		 */
		pp->pp_pte.pte_ptp = ptp;
		pp->pp_pte.pte_va = va;
		*new_embedded = true;
	} else if (__predict_false(pmap->pm_pve == NULL)) {
		/*
		 * No memory.
		 */
		error = ENOMEM;
	} else {
		/*
		 * Install new pv_entry on the page.
		 */
		pve = pmap->pm_pve;
		pmap->pm_pve = NULL;
		*new_pve = pve;
		pve->pve_pte.pte_ptp = ptp;
		pve->pve_pte.pte_va = va;
		pve->pve_pp = pp;
		LIST_INSERT_HEAD(&pp->pp_pvlist, pve, pve_list);
	}
	mutex_spin_exit(&pp->pp_lock);
	if (error == 0) {
		pmap_check_pv(pmap, ptp, pp, va, true);
	}

	return error;
}

/*
 * pmap_remove_pv: try to remove a mapping from a pv_list
 *
 * => pmap must be locked
 * => removes dynamic entries from tree and frees them
 * => caller should adjust ptp's wire_count and free PTP if needed
 */
static void
pmap_remove_pv(struct pmap *pmap, struct pmap_page *pp, struct vm_page *ptp,
    vaddr_t va, struct pv_entry *pve, uint8_t oattrs)
{
	rb_tree_t *tree = (ptp != NULL ?
	    &VM_PAGE_TO_PP(ptp)->pp_rb : &pmap_kernel_rb);

	KASSERT(mutex_owned(&pmap->pm_lock));
	KASSERT(ptp_to_pmap(ptp) == pmap);
	KASSERT(ptp == NULL || ptp->uobject != NULL);
	KASSERT(ptp == NULL || ptp_va2o(va, 1) == ptp->offset);
	KASSERT(ptp != NULL || pmap == pmap_kernel());

	pmap_check_pv(pmap, ptp, pp, va, true);

	if (pve == NULL) {
		mutex_spin_enter(&pp->pp_lock);
		KASSERT(pp->pp_pte.pte_ptp == ptp);
		KASSERT(pp->pp_pte.pte_va == va);
		pp->pp_attrs |= oattrs;
		pp->pp_pte.pte_ptp = NULL;
		pp->pp_pte.pte_va = 0;
		mutex_spin_exit(&pp->pp_lock);
	} else {
		mutex_spin_enter(&pp->pp_lock);
		KASSERT(pp->pp_pte.pte_ptp != ptp ||
		    pp->pp_pte.pte_va != va);
		KASSERT(pve->pve_pte.pte_ptp == ptp);
		KASSERT(pve->pve_pte.pte_va == va);
		KASSERT(pve->pve_pp == pp);
		pp->pp_attrs |= oattrs;
		LIST_REMOVE(pve, pve_list);
		mutex_spin_exit(&pp->pp_lock);

		KASSERT(pmap_treelookup_pv(pmap, ptp, tree, va) == pve);
		rb_tree_remove_node(tree, pve);
#ifdef DIAGNOSTIC
		memset(pve, 0, sizeof(*pve));
#endif
		pmap_free_pv(pmap, pve);
	}

	KASSERT(pmap_treelookup_pv(pmap, ptp, tree, va) == NULL);
	pmap_check_pv(pmap, ptp, pp, va, false);
}

/*
 * p t p   f u n c t i o n s
 */

static struct vm_page *
pmap_find_ptp(struct pmap *pmap, vaddr_t va, int level)
{
	int lidx = level - 1;
	off_t off = ptp_va2o(va, level);
	struct vm_page *pg;

	KASSERT(mutex_owned(&pmap->pm_lock));

	if (pmap->pm_ptphint[lidx] && off == pmap->pm_ptphint[lidx]->offset) {
		KASSERT(pmap->pm_ptphint[lidx]->wire_count > 0);
		pg = pmap->pm_ptphint[lidx];
		PMAP_CHECK_PP(VM_PAGE_TO_PP(pg));
		return pg;
	}
	PMAP_DUMMY_LOCK(pmap);
	pg = uvm_pagelookup(&pmap->pm_obj[lidx], off);
	PMAP_DUMMY_UNLOCK(pmap);
	if (pg != NULL && __predict_false(pg->wire_count == 0)) {
		/* This page is queued to be freed - ignore. */
		pg = NULL;
	}
	if (pg != NULL) {
		PMAP_CHECK_PP(VM_PAGE_TO_PP(pg));
	}
	pmap->pm_ptphint[lidx] = pg;
	return pg;
}

static inline void
pmap_freepage(struct pmap *pmap, struct vm_page *ptp, int level)
{
	int lidx;

	KASSERT(ptp->wire_count <= 1);
	PMAP_CHECK_PP(VM_PAGE_TO_PP(ptp));

	lidx = level - 1;
	pmap_stats_update(pmap, -ptp->wire_count, 0);
	if (pmap->pm_ptphint[lidx] == ptp)
		pmap->pm_ptphint[lidx] = NULL;
	ptp->wire_count = 0;
	ptp->uanon = NULL;
	KASSERT(RB_TREE_MIN(&VM_PAGE_TO_PP(ptp)->pp_rb) == NULL);

	/*
	 * Enqueue the PTP to be freed by pmap_update().  We can't remove
	 * the page from the uvm_object, as that can take further locks
	 * (intolerable right now because the PTEs are likely mapped in).
	 * Instead mark the PTP as free and if we bump into it again, we'll
	 * either ignore or reuse (depending on what's useful at the time).
	 */
	LIST_INSERT_HEAD(&pmap->pm_gc_ptp, ptp, mdpage.mp_pp.pp_link);
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
	KASSERT(mutex_owned(&pmap->pm_lock));
	KASSERT(kpreempt_disabled());

	level = 1;
	do {
		index = pl_i(va, level + 1);
		opde = pmap_pte_testset(&pdes[level - 1][index], 0);

		/*
		 * On Xen-amd64 or SVS, we need to sync the top level page
		 * directory on each CPU.
		 */
#if defined(XENPV) && defined(__x86_64__)
		if (level == PTP_LEVELS - 1) {
			xen_kpm_sync(pmap, index);
		}
#elif defined(SVS)
		if (svs_enabled && level == PTP_LEVELS - 1) {
			svs_pmap_sync(pmap, index);
		}
#endif

		invaladdr = level == 1 ? (vaddr_t)ptes :
		    (vaddr_t)pdes[level - 2];
		pmap_tlb_shootdown(pmap, invaladdr + index * PAGE_SIZE,
		    opde, TLBSHOOT_FREE_PTP);

#if defined(XENPV)
		pmap_tlb_shootnow();
#endif

		pmap_freepage(pmap, ptp, level);
		if (level < PTP_LEVELS - 1) {
			ptp = pmap_find_ptp(pmap, va, level + 1);
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
 * => we are not touching any PTEs yet, so they need not be mapped in
 */
static int
pmap_get_ptp(struct pmap *pmap, struct pmap_ptparray *pt, vaddr_t va,
    int flags, struct vm_page **resultp)
{
	struct vm_page *ptp;
	int i, aflags;
	struct uvm_object *obj;
	voff_t off;

	KASSERT(pmap != pmap_kernel());
	KASSERT(mutex_owned(&pmap->pm_lock));

	/*
	 * Loop through all page table levels allocating a page
	 * for any level where we don't already have one.
	 */
	memset(pt, 0, sizeof(*pt));
	aflags = ((flags & PMAP_CANFAIL) ? 0 : UVM_PGA_USERESERVE) |
		UVM_PGA_ZERO;
	for (i = PTP_LEVELS; i > 1; i--) {
		obj = &pmap->pm_obj[i - 2];
		off = ptp_va2o(va, i - 1);

		PMAP_DUMMY_LOCK(pmap);
		pt->pg[i] = uvm_pagelookup(obj, off);

		if (pt->pg[i] == NULL) {
			pt->pg[i] = uvm_pagealloc(obj, off, NULL, aflags);
			pt->alloced[i] = (pt->pg[i] != NULL);
		} else if (pt->pg[i]->wire_count == 0) {
			/* This page was queued to be freed; dequeue it. */
			LIST_REMOVE(pt->pg[i], mdpage.mp_pp.pp_link);
			pt->alloced[i] = true;
		}
		PMAP_DUMMY_UNLOCK(pmap);
		if (pt->pg[i] == NULL) {
			pmap_unget_ptp(pmap, pt);
			return ENOMEM;
		} else if (pt->alloced[i]) {
			pt->pg[i]->uanon = (struct vm_anon *)(vaddr_t)~0L;
			rb_tree_init(&VM_PAGE_TO_PP(pt->pg[i])->pp_rb,
			    &pmap_rbtree_ops);
			PMAP_CHECK_PP(VM_PAGE_TO_PP(pt->pg[i]));
		}
	}
	ptp = pt->pg[2];
	KASSERT(ptp != NULL);
	*resultp = ptp;
	pmap->pm_ptphint[0] = ptp;
	return 0;
}

/*
 * pmap_install_ptp: install any freshly allocated PTPs
 *
 * => pmap should NOT be pmap_kernel()
 * => pmap should be locked
 * => PTEs must be mapped
 * => preemption must be disabled
 */
static void
pmap_install_ptp(struct pmap *pmap, struct pmap_ptparray *pt, vaddr_t va,
    pd_entry_t * const *pdes)
{
	struct vm_page *ptp;
	unsigned long index;
	pd_entry_t *pva;
	paddr_t pa;
	int i;

	KASSERT(pmap != pmap_kernel());
	KASSERT(mutex_owned(&pmap->pm_lock));
	KASSERT(kpreempt_disabled());

	/*
	 * Now that we have all the pages looked up or allocated,
	 * loop through again installing any new ones into the tree.
	 */
	for (i = PTP_LEVELS; i > 1; i--) {
		index = pl_i(va, i);
		pva = pdes[i - 2];

		if (pmap_valid_entry(pva[index])) {
			KASSERT(!pt->alloced[i]);
			continue;
		}

		ptp = pt->pg[i];
		ptp->flags &= ~PG_BUSY; /* never busy */
		ptp->wire_count = 1;
		pmap->pm_ptphint[i - 2] = ptp;
		pa = VM_PAGE_TO_PHYS(ptp);
		pmap_pte_set(&pva[index], (pd_entry_t)
		    (pmap_pa2pte(pa) | PTE_U | PTE_W | PTE_P));

		/*
		 * On Xen-amd64 or SVS, we need to sync the top level page
		 * directory on each CPU.
		 */
#if defined(XENPV) && defined(__x86_64__)
		if (i == PTP_LEVELS) {
			xen_kpm_sync(pmap, index);
		}
#elif defined(SVS)
		if (svs_enabled && i == PTP_LEVELS) {
			svs_pmap_sync(pmap, index);
		}
#endif

		pmap_pte_flush();
		pmap_stats_update(pmap, 1, 0);

		/*
		 * If we're not in the top level, increase the
		 * wire count of the parent page.
		 */
		if (i < PTP_LEVELS) {
			pt->pg[i + 1]->wire_count++;
		}
	}
}

/*
 * pmap_unget_ptp: free unusued PTPs
 *
 * => pmap should NOT be pmap_kernel()
 * => pmap should be locked
 */
static void
pmap_unget_ptp(struct pmap *pmap, struct pmap_ptparray *pt)
{
	int i;

	KASSERT(pmap != pmap_kernel());
	KASSERT(mutex_owned(&pmap->pm_lock));

	for (i = PTP_LEVELS; i > 1; i--) {
		if (!pt->alloced[i]) {
			continue;
		}
		KASSERT(pt->pg[i]->wire_count == 0);
		PMAP_CHECK_PP(VM_PAGE_TO_PP(pt->pg[i]));
		pmap_freepage(pmap, pt->pg[i], i - 1);
	}
}

/*
 * p m a p   l i f e c y c l e   f u n c t i o n s
 */

/*
 * pmap_pdp_init: constructor a new PDP.
 */
static void
pmap_pdp_init(pd_entry_t *pdir)
{
	paddr_t pdirpa = 0;
	vaddr_t object;
	int i;

#if !defined(XENPV) || !defined(__x86_64__)
	int npde;
#endif
#ifdef XENPV
	int s;
#endif

	memset(PAGE_ALIGNED(pdir), 0, PDP_SIZE * PAGE_SIZE);

	/*
	 * NOTE: This is all done unlocked, but we will check afterwards
	 * if we have raced with pmap_growkernel().
	 */

#if defined(XENPV) && defined(__x86_64__)
	/* Fetch the physical address of the page directory */
	(void)pmap_extract(pmap_kernel(), (vaddr_t)pdir, &pdirpa);

	/*
	 * This pdir will NEVER be active in kernel mode, so mark
	 * recursive entry invalid.
	 */
	pdir[PDIR_SLOT_PTE] = pmap_pa2pte(pdirpa);

	/*
	 * PDP constructed this way won't be for the kernel, hence we
	 * don't put kernel mappings on Xen.
	 *
	 * But we need to make pmap_create() happy, so put a dummy
	 * (without PTE_P) value at the right place.
	 */
	pdir[PDIR_SLOT_KERN + nkptp[PTP_LEVELS - 1] - 1] =
	     (pd_entry_t)-1 & PTE_FRAME;
#else /* XENPV && __x86_64__*/
	object = (vaddr_t)pdir;
	for (i = 0; i < PDP_SIZE; i++, object += PAGE_SIZE) {
		/* Fetch the physical address of the page directory */
		(void)pmap_extract(pmap_kernel(), object, &pdirpa);

		/* Put in recursive PDE to map the PTEs */
		pdir[PDIR_SLOT_PTE + i] = pmap_pa2pte(pdirpa) | PTE_P |
		    pmap_pg_nx;
#ifndef XENPV
		pdir[PDIR_SLOT_PTE + i] |= PTE_W;
#endif
	}

	/* Copy the kernel's top level PDE */
	npde = nkptp[PTP_LEVELS - 1];

	memcpy(&pdir[PDIR_SLOT_KERN], &PDP_BASE[PDIR_SLOT_KERN],
	    npde * sizeof(pd_entry_t));

	if (VM_MIN_KERNEL_ADDRESS != KERNBASE) {
		int idx = pl_i(KERNBASE, PTP_LEVELS);
		pdir[idx] = PDP_BASE[idx];
	}

#ifdef __HAVE_PCPU_AREA
	pdir[PDIR_SLOT_PCPU] = PDP_BASE[PDIR_SLOT_PCPU];
#endif
#ifdef __HAVE_DIRECT_MAP
	slotspace_copy(SLAREA_DMAP, pdir, PDP_BASE);
#endif
#ifdef KASAN
	slotspace_copy(SLAREA_ASAN, pdir, PDP_BASE);
#endif
#ifdef KMSAN
	slotspace_copy(SLAREA_MSAN, pdir, PDP_BASE);
#endif
#endif /* XENPV  && __x86_64__*/

#ifdef XENPV
	s = splvm();
	object = (vaddr_t)pdir;
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
#endif /* XENPV */
}

/*
 * pmap_pdp_fini: destructor for the PDPs.
 */
static void
pmap_pdp_fini(pd_entry_t *pdir)
{
#ifdef XENPV
	paddr_t pdirpa = 0;	/* XXX: GCC */
	vaddr_t object = (vaddr_t)pdir;
	int i;
	int s = splvm();
	pt_entry_t *pte;

	for (i = 0; i < PDP_SIZE; i++, object += PAGE_SIZE) {
		/* fetch the physical address of the page directory. */
		(void) pmap_extract(pmap_kernel(), object, &pdirpa);
		/* unpin page table */
		xpq_queue_unpin_table(xpmap_ptom_masked(pdirpa));
	}
	object = (vaddr_t)pdir;
	for (i = 0; i < PDP_SIZE; i++, object += PAGE_SIZE) {
		/* Set page RW again */
		pte = kvtopte(object);
		pmap_pte_set(pte, *pte | PTE_W);
		xen_bcast_invlpg((vaddr_t)object);
	}
	splx(s);
#endif  /* XENPV */
}

#ifdef PAE
static void *
pmap_pdp_alloc(struct pool *pp, int flags)
{
	return (void *)uvm_km_alloc(kernel_map,
	    PAGE_SIZE * PDP_SIZE, PAGE_SIZE * PDP_SIZE,
	    ((flags & PR_WAITOK) ? 0 : UVM_KMF_NOWAIT | UVM_KMF_TRYLOCK) |
	    UVM_KMF_WIRED);
}

static void
pmap_pdp_free(struct pool *pp, void *v)
{
	uvm_km_free(kernel_map, (vaddr_t)v, PAGE_SIZE * PDP_SIZE,
	    UVM_KMF_WIRED);
}
#endif /* PAE */

/*
 * pmap_ctor: constructor for the pmap cache.
 */
static int
pmap_ctor(void *arg, void *obj, int flags)
{
	struct pmap *pmap = obj;
	pt_entry_t p;
	int i;

	KASSERT((flags & PR_WAITOK) != 0);

	mutex_init(&pmap->pm_lock, MUTEX_DEFAULT, IPL_NONE);
	rw_init(&pmap->pm_dummy_lock);
	kcpuset_create(&pmap->pm_cpus, true);
	kcpuset_create(&pmap->pm_kernel_cpus, true);
#ifdef XENPV
	kcpuset_create(&pmap->pm_xen_ptp_cpus, true);
#endif
	LIST_INIT(&pmap->pm_gc_ptp);
	pmap->pm_pve = NULL;
	LIST_INIT(&pmap->pm_pvp_full);
	LIST_INIT(&pmap->pm_pvp_part);
	LIST_INIT(&pmap->pm_pvp_empty);

	/* allocate and init PDP */
	pmap->pm_pdir = pool_get(&pmap_pdp_pool, PR_WAITOK);

	for (;;) {
		pmap_pdp_init(pmap->pm_pdir);
		mutex_enter(&pmaps_lock);
		p = pmap->pm_pdir[PDIR_SLOT_KERN + nkptp[PTP_LEVELS - 1] - 1];
		if (__predict_true(p != 0)) {
			break;
		}
		mutex_exit(&pmaps_lock);
	}

	for (i = 0; i < PDP_SIZE; i++)
		pmap->pm_pdirpa[i] =
		    pmap_pte2pa(pmap->pm_pdir[PDIR_SLOT_PTE + i]);

	LIST_INSERT_HEAD(&pmaps, pmap, pm_list);
	mutex_exit(&pmaps_lock);

	return 0;
}

/*
 * pmap_ctor: destructor for the pmap cache.
 */
static void
pmap_dtor(void *arg, void *obj)
{
	struct pmap *pmap = obj;

	mutex_enter(&pmaps_lock);
	LIST_REMOVE(pmap, pm_list);
	mutex_exit(&pmaps_lock);

	pmap_pdp_fini(pmap->pm_pdir);
	pool_put(&pmap_pdp_pool, pmap->pm_pdir);
	mutex_destroy(&pmap->pm_lock);
	rw_destroy(&pmap->pm_dummy_lock);
	kcpuset_destroy(pmap->pm_cpus);
	kcpuset_destroy(pmap->pm_kernel_cpus);
#ifdef XENPV
	kcpuset_destroy(pmap->pm_xen_ptp_cpus);
#endif
}

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
		uvm_obj_init(&pmap->pm_obj[i], &pmap_pager, false, 1);
		uvm_obj_setlock(&pmap->pm_obj[i], &pmap->pm_dummy_lock);
		pmap->pm_ptphint[i] = NULL;
	}
	pmap->pm_stats.wired_count = 0;
	/* count the PDP allocd below */
	pmap->pm_stats.resident_count = PDP_SIZE;
#if !defined(__x86_64__)
	pmap->pm_hiexec = 0;
#endif

	/* Used by NVMM and Xen */
	pmap->pm_enter = NULL;
	pmap->pm_extract = NULL;
	pmap->pm_remove = NULL;
	pmap->pm_sync_pv = NULL;
	pmap->pm_pp_remove_ent = NULL;
	pmap->pm_write_protect = NULL;
	pmap->pm_unwire = NULL;
	pmap->pm_tlb_flush = NULL;
	pmap->pm_data = NULL;

	/* init the LDT */
	pmap->pm_ldt = NULL;
	pmap->pm_ldt_sel = GSYSSEL(GLDT_SEL, SEL_KPL);

	return pmap;
}

/*
 * pmap_check_ptps: verify that none of the pmap's page table objects
 * have any pages allocated to them.
 */
static void
pmap_check_ptps(struct pmap *pmap)
{
	int i;

	for (i = 0; i < PTP_LEVELS - 1; i++) {
		KASSERTMSG(pmap->pm_obj[i].uo_npages == 0,
		    "pmap %p level %d still has %d pages",
		    pmap, i, (int)pmap->pm_obj[i].uo_npages);
	}
}

static void
pmap_check_inuse(struct pmap *pmap)
{
#ifdef DEBUG
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;

	for (CPU_INFO_FOREACH(cii, ci)) {
		if (ci->ci_pmap == pmap)
			panic("destroying pmap being used");
#if defined(XENPV) && defined(__x86_64__)
		for (int i = 0; i < PDIR_SLOT_USERLIM; i++) {
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
				panic("%s: used pmap", __func__);
			}
		}
#endif
	}
#endif /* DEBUG */
}

/*
 * pmap_destroy:  drop reference count on pmap.  free pmap if reference
 * count goes to zero.
 *
 * => we can be called from pmap_unmap_ptes() with a different, unrelated
 *    pmap's lock held.  be careful!
 */
void
pmap_destroy(struct pmap *pmap)
{
	int i;

	/*
	 * drop reference count and verify not in use.
	 */

	if (atomic_dec_uint_nv(&pmap->pm_obj[0].uo_refs) > 0) {
		return;
	}
	pmap_check_inuse(pmap);

	/*
	 * handle any deferred frees.
	 */

	mutex_enter(&pmap->pm_lock);
	if (pmap->pm_pve != NULL) {
		pmap_free_pv(pmap, pmap->pm_pve);
		pmap->pm_pve = NULL;
	}
	pmap_drain_pv(pmap);
	mutex_exit(&pmap->pm_lock);
	pmap_update(pmap);

	/*
	 * Reference count is zero, free pmap resources and then free pmap.
	 */

	pmap_check_ptps(pmap);
	KASSERT(LIST_EMPTY(&pmap->pm_gc_ptp));

#ifdef USER_LDT
	if (pmap->pm_ldt != NULL) {
		/*
		 * No need to switch the LDT; this address space is gone,
		 * nothing is using it.
		 *
		 * No need to lock the pmap for ldt_free (or anything else),
		 * we're the last one to use it.
		 */
		/* XXXAD can't take cpu_lock here - fix soon. */
		mutex_enter(&cpu_lock);
		ldt_free(pmap->pm_ldt_sel);
		mutex_exit(&cpu_lock);
		uvm_km_free(kernel_map, (vaddr_t)pmap->pm_ldt,
		    MAX_USERLDT_SIZE, UVM_KMF_WIRED);
	}
#endif

	for (i = 0; i < PTP_LEVELS - 1; i++) {
		uvm_obj_destroy(&pmap->pm_obj[i], false);
	}
	kcpuset_zero(pmap->pm_cpus);
	kcpuset_zero(pmap->pm_kernel_cpus);
#ifdef XENPV
	kcpuset_zero(pmap->pm_xen_ptp_cpus);
#endif

	KASSERT(LIST_EMPTY(&pmap->pm_pvp_full));
	KASSERT(LIST_EMPTY(&pmap->pm_pvp_part));
	KASSERT(LIST_EMPTY(&pmap->pm_pvp_empty));

	pmap_check_ptps(pmap);
	if (__predict_false(pmap->pm_enter != NULL)) {
		/* XXX make this a different cache */
		pool_cache_destruct_object(&pmap_cache, pmap);
	} else {
		pool_cache_put(&pmap_cache, pmap);
	}
}

/*
 * pmap_zap_ptp: clear out an entire PTP without modifying PTEs
 *
 * => caller must hold pmap's lock
 * => PTP must be mapped into KVA
 * => must be called with kernel preemption disabled
 * => does as little work as possible
 */
static void
pmap_zap_ptp(struct pmap *pmap, struct vm_page *ptp, pt_entry_t *pte,
    vaddr_t startva, vaddr_t blkendva)
{
#ifndef XENPV
	struct pv_entry *pve;
	struct vm_page *pg;
	struct pmap_page *pp;
	pt_entry_t opte;
	rb_tree_t *tree;
	vaddr_t va;
	int wired;
	uint8_t oattrs;
	u_int cnt;

	KASSERT(mutex_owned(&pmap->pm_lock));
	KASSERT(kpreempt_disabled());
	KASSERT(pmap != pmap_kernel());
	KASSERT(ptp->wire_count > 1);
	KASSERT(ptp->wire_count - 1 <= PAGE_SIZE / sizeof(pt_entry_t));

	/*
	 * Start at the lowest entered VA, and scan until there are no more
	 * PTEs in the PTPs.
	 */
	tree = &VM_PAGE_TO_PP(ptp)->pp_rb;
	pve = RB_TREE_MIN(tree);
	wired = 0;
	va = (vaddr_t)ptp->uanon;
	pte += ((va - startva) >> PAGE_SHIFT);

	for (cnt = ptp->wire_count; cnt > 1; pte++, va += PAGE_SIZE) {
		/*
		 * No need for an atomic to clear the PTE.  Nothing else can
		 * see the address space any more and speculative access (if
		 * possible) won't modify.  Therefore there's no need to
		 * track the accessed/dirty bits.
		 */
		opte = *pte;
		if (!pmap_valid_entry(opte)) {
			continue;
		}

		/*
		 * Count the PTE.  If it's not for a managed mapping
		 * there's noting more to do.
		 */
		cnt--;
		wired -= (opte & PTE_WIRED);
		if ((opte & PTE_PVLIST) == 0) {
#ifndef DOM0OPS
			KASSERTMSG((PHYS_TO_VM_PAGE(pmap_pte2pa(opte)) == NULL),
			    "managed page without PTE_PVLIST for %#"
			    PRIxVADDR, va);
			KASSERTMSG((pmap_pv_tracked(pmap_pte2pa(opte)) == NULL),
			    "pv-tracked page without PTE_PVLIST for %#"
			    PRIxVADDR, va);
#endif
			KASSERT(pmap_treelookup_pv(pmap, ptp, (ptp != NULL ?
			    &VM_PAGE_TO_PP(ptp)->pp_rb : &pmap_kernel_rb),
			    va) == NULL);
			continue;
		}

		/*
		 * "pve" now points to the lowest (by VA) dynamic PV entry
		 * in the PTP.  If it's for this VA, take advantage of it to
		 * avoid calling PHYS_TO_VM_PAGE().  Avoid modifying the RB
		 * tree by skipping to the next VA in the tree whenever
		 * there is a match here.  The tree will be cleared out in
		 * one pass before return to pmap_remove_all().
		 */
		oattrs = pmap_pte_to_pp_attrs(opte);
		if (pve != NULL && pve->pve_pte.pte_va == va) {
			pp = pve->pve_pp;
			KASSERT(pve->pve_pte.pte_ptp == ptp);
			KASSERT(pp->pp_pte.pte_ptp != ptp ||
			    pp->pp_pte.pte_va != va);
			mutex_spin_enter(&pp->pp_lock);
			pp->pp_attrs |= oattrs;
			LIST_REMOVE(pve, pve_list);
			mutex_spin_exit(&pp->pp_lock);

			/*
			 * pve won't be touched again until pmap_drain_pv(),
			 * so it's still safe to traverse the tree.
			 */
			pmap_free_pv(pmap, pve);
			pve = RB_TREE_NEXT(tree, pve);
			continue;
		}

		/*
		 * No entry in the tree so it must be embedded.  Look up the
		 * page and cancel the embedded entry.
		 */
		if ((pg = PHYS_TO_VM_PAGE(pmap_pte2pa(opte))) != NULL) {
			pp = VM_PAGE_TO_PP(pg);
		} else if ((pp = pmap_pv_tracked(pmap_pte2pa(opte))) == NULL) {
			paddr_t pa = pmap_pte2pa(opte);
			panic("%s: PTE_PVLIST with pv-untracked page"
			    " va = %#"PRIxVADDR"pa = %#"PRIxPADDR
			    "(%#"PRIxPADDR")", __func__, va, pa, atop(pa));
		}
		mutex_spin_enter(&pp->pp_lock);
		KASSERT(pp->pp_pte.pte_ptp == ptp);
		KASSERT(pp->pp_pte.pte_va == va);
		pp->pp_attrs |= oattrs;
		pp->pp_pte.pte_ptp = NULL;
		pp->pp_pte.pte_va = 0;
		mutex_spin_exit(&pp->pp_lock);
	}

	/* PTP now empty - adjust the tree & stats to match. */
	pmap_stats_update(pmap, -(ptp->wire_count - 1), wired / PTE_WIRED);
	ptp->wire_count = 1;
#ifdef DIAGNOSTIC
	rb_tree_init(tree, &pmap_rbtree_ops);
#endif
#else	/* !XENPV */
	/*
	 * XXXAD For XEN, it's not clear to me that we can do this, because
	 * I guess the hypervisor keeps track of PTEs too.
	 */
	pmap_remove_ptes(pmap, ptp, (vaddr_t)pte, startva, blkendva);
#endif	/* !XENPV */
}

/*
 * pmap_remove_all: remove all mappings from pmap in bulk.
 *
 * Ordinarily when removing mappings it's important to hold the UVM object's
 * lock, so that pages do not gain a new identity while retaining stale TLB
 * entries (the same lock hold covers both pmap_remove() and pmap_update()).
 * Here it's known that the address space is no longer visible to any user
 * process, so we don't need to worry about that.
 */
bool
pmap_remove_all(struct pmap *pmap)
{
	struct vm_page *ptps[32];
	vaddr_t va, blkendva;
	struct pmap *pmap2;
	pt_entry_t *ptes;
	pd_entry_t pde __diagused;
	pd_entry_t * const *pdes;
	int lvl __diagused, i, n;

	/* XXX Can't handle EPT just yet. */
	if (pmap->pm_remove != NULL) {
		return false;
	}

	for (;;) {
		/* Fetch a block of PTPs from tree. */
		mutex_enter(&pmap->pm_lock);
		n = radix_tree_gang_lookup_node(&pmap->pm_obj[0].uo_pages, 0,
		    (void **)ptps, __arraycount(ptps), false);
		if (n == 0) {
			mutex_exit(&pmap->pm_lock);
			break;
		}

		/* Remove all mappings in the set of PTPs. */
		pmap_map_ptes(pmap, &pmap2, &ptes, &pdes);
		for (i = 0; i < n; i++) {
			if (ptps[i]->wire_count == 0) {
				/* It's dead: pmap_update() will expunge. */
				continue;
			}

			/* Determine range of block. */
			va = ptps[i]->offset * PAGE_SIZE / sizeof(pt_entry_t);
			blkendva = x86_round_pdr(va + 1);

			/* Make sure everything squares up... */
			KASSERT(pmap_pdes_valid(va, pdes, &pde, &lvl));
			KASSERT(lvl == 1);
			KASSERT(pmap_find_ptp(pmap, va, 1) == ptps[i]);

			/* Zap! */
			pmap_zap_ptp(pmap, ptps[i], &ptes[pl1_i(va)], va,
			    blkendva);

			/* PTP should now be unused - free it. */
			KASSERT(ptps[i]->wire_count == 1);
			pmap_free_ptp(pmap, ptps[i], va, ptes, pdes);
		}
		pmap_unmap_ptes(pmap, pmap2);
		pmap_drain_pv(pmap);
		pmap_tlb_shootdown(pmap, -1L, 0, TLBSHOOT_REMOVE_ALL);
		mutex_exit(&pmap->pm_lock);

		/* Process deferred frees. */
		pmap_update(pmap);

		/* A breathing point. */
		preempt_point();
	}

	/* Verify that the pmap is now completely empty. */
	pmap_check_ptps(pmap);
	KASSERTMSG(pmap->pm_stats.resident_count == PDP_SIZE,
	    "pmap %p not empty", pmap);

	return true;
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
	int sel;

	if (__predict_true(pmap1->pm_ldt == NULL)) {
		return;
	}

	/*
	 * Copy the LDT into the new process.
	 *
	 * Read pmap1's ldt pointer unlocked; if it changes behind our back
	 * we'll retry. This will starve if there's a stream of LDT changes
	 * in another thread but that should not happen.
	 */

retry:
	if (pmap1->pm_ldt != NULL) {
		/* Allocate space for the new process's LDT */
		new_ldt = (union descriptor *)uvm_km_alloc(kernel_map,
		    MAX_USERLDT_SIZE, 0, UVM_KMF_WIRED);
		if (new_ldt == NULL) {
			printf("WARNING: %s: unable to allocate LDT space\n",
			    __func__);
			return;
		}
		mutex_enter(&cpu_lock);
		/* Get a GDT slot for it */
		sel = ldt_alloc(new_ldt, MAX_USERLDT_SIZE);
		if (sel == -1) {
			mutex_exit(&cpu_lock);
			uvm_km_free(kernel_map, (vaddr_t)new_ldt,
			    MAX_USERLDT_SIZE, UVM_KMF_WIRED);
			printf("WARNING: %s: unable to allocate LDT selector\n",
			    __func__);
			return;
		}
	} else {
		/* Wasn't anything there after all. */
		new_ldt = NULL;
		sel = -1;
		mutex_enter(&cpu_lock);
	}

	/*
	 * Now that we have cpu_lock, ensure the LDT status is the same.
	 */
	if (pmap1->pm_ldt != NULL) {
		if (new_ldt == NULL) {
			/* A wild LDT just appeared. */
			mutex_exit(&cpu_lock);
			goto retry;
		}

		/* Copy the LDT data and install it in pmap2 */
		memcpy(new_ldt, pmap1->pm_ldt, MAX_USERLDT_SIZE);
		pmap2->pm_ldt = new_ldt;
		pmap2->pm_ldt_sel = sel;
		mutex_exit(&cpu_lock);
	} else {
		if (new_ldt != NULL) {
			/* The LDT disappeared, drop what we did. */
			ldt_free(sel);
			mutex_exit(&cpu_lock);
			uvm_km_free(kernel_map, (vaddr_t)new_ldt,
			    MAX_USERLDT_SIZE, UVM_KMF_WIRED);
			return;
		}

		/* We're good, just leave. */
		mutex_exit(&cpu_lock);
	}
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
#if defined(SVS)
		if (svs_enabled) {
			svs_ldt_sync(pm);
		} else
#endif
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
	union descriptor *ldt;
	int sel;

	if (__predict_true(pmap->pm_ldt == NULL)) {
		return;
	}

	mutex_enter(&cpu_lock);
	if (pmap->pm_ldt != NULL) {
		sel = pmap->pm_ldt_sel;
		ldt = pmap->pm_ldt;
		pmap->pm_ldt_sel = GSYSSEL(GLDT_SEL, SEL_KPL);
		pmap->pm_ldt = NULL;
		pmap_ldt_sync(pmap);
		ldt_free(sel);
		uvm_km_free(kernel_map, (vaddr_t)ldt, MAX_USERLDT_SIZE,
		    UVM_KMF_WIRED);
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

	if (l != ci->ci_curlwp)
		return;

	KASSERT(ci->ci_want_pmapload == 0);
	KASSERT(ci->ci_tlbstate != TLBSTATE_VALID);

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

#if defined(XENPV) && defined(__x86_64__)
#define	KASSERT_PDIRPA(pmap) \
	KASSERT(pmap_pdirpa(pmap, 0) == ci->ci_xen_current_user_pgd || \
	    pmap == pmap_kernel())
#elif defined(PAE)
#define	KASSERT_PDIRPA(pmap) \
	KASSERT(pmap_pdirpa(pmap, 0) == pmap_pte2pa(ci->ci_pae_l3_pdir[0]))
#elif !defined(XENPV)
#define	KASSERT_PDIRPA(pmap) \
	KASSERT(pmap_pdirpa(pmap, 0) == pmap_pte2pa(rcr3()))
#else
#define	KASSERT_PDIRPA(pmap)	KASSERT(true)	/* nothing to do */
#endif

/*
 * pmap_reactivate: try to regain reference to the pmap.
 *
 * => Must be called with kernel preemption disabled.
 */
static void
pmap_reactivate(struct pmap *pmap)
{
	struct cpu_info * const ci = curcpu();
	const cpuid_t cid = cpu_index(ci);

	KASSERT(kpreempt_disabled());
	KASSERT_PDIRPA(pmap);

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

	if (__predict_true(kcpuset_isset(pmap->pm_cpus, cid))) {
		/* We have the reference, state is valid. */
	} else {
		/*
		 * Must reload the TLB, pmap has been changed during
		 * deactivated.
		 */
		kcpuset_atomic_set(pmap->pm_cpus, cid);

		tlbflush();
	}
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
	__insn_barrier();

	/* should be able to take ipis. */
	KASSERT(ci->ci_ilevel < IPL_HIGH);
#ifdef XENPV
	/* Check to see if interrupts are enabled (ie; no events are masked) */
	KASSERT(x86_read_psl() == 0);
#else
	KASSERT((x86_read_psl() & PSL_I) != 0);
#endif

	KASSERT(l != NULL);
	pmap = vm_map_pmap(&l->l_proc->p_vmspace->vm_map);
	KASSERT(pmap != pmap_kernel());
	oldpmap = ci->ci_pmap;

	if (pmap == oldpmap) {
		pmap_reactivate(pmap);
		ci->ci_want_pmapload = 0;
		kpreempt_enable();
		return;
	}

	/*
	 * Acquire a reference to the new pmap and perform the switch.
	 */

	pmap_reference(pmap);
	pmap_load1(l, pmap, oldpmap);
	ci->ci_want_pmapload = 0;

	/*
	 * we're now running with the new pmap.  drop the reference
	 * to the old pmap.  if we block, we need to go around again.
	 */

	pmap_destroy(oldpmap);
	__insn_barrier();
	if (l->l_ncsw != ncsw) {
		goto retry;
	}

	kpreempt_enable();
}

/*
 * pmap_load1: the guts of pmap load, shared by pmap_map_ptes() and
 * pmap_load().  It's critically important that this function does not
 * block.
 */
static void
pmap_load1(struct lwp *l, struct pmap *pmap, struct pmap *oldpmap)
{
	struct cpu_info *ci;
	struct pcb *pcb;
	cpuid_t cid;

	KASSERT(kpreempt_disabled());

	pcb = lwp_getpcb(l);
	ci = l->l_cpu;
	cid = cpu_index(ci);

	kcpuset_atomic_clear(oldpmap->pm_cpus, cid);
	kcpuset_atomic_clear(oldpmap->pm_kernel_cpus, cid);

	KASSERT_PDIRPA(oldpmap);
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
#ifndef XENPV
	ci->ci_tss->tss.tss_ldt = pmap->pm_ldt_sel;
	ci->ci_tss->tss.tss_cr3 = pcb->pcb_cr3;
#endif
#endif

#if defined(SVS) && defined(USER_LDT)
	if (svs_enabled) {
		svs_ldt_sync(pmap);
	} else
#endif
	lldt(pmap->pm_ldt_sel);

	cpu_load_pmap(pmap, oldpmap);
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

	KASSERT_PDIRPA(pmap);
	KASSERT(ci->ci_pmap == pmap);

	/*
	 * we aren't interested in TLB invalidations for this pmap,
	 * at least for the time being.
	 */

	KASSERT(ci->ci_tlbstate == TLBSTATE_VALID);
	ci->ci_tlbstate = TLBSTATE_LAZY;
}

/*
 * some misc. functions
 */

bool
pmap_pdes_valid(vaddr_t va, pd_entry_t * const *pdes, pd_entry_t *lastpde,
    int *lastlvl)
{
	unsigned long index;
	pd_entry_t pde;
	int i;

	for (i = PTP_LEVELS; i > 1; i--) {
		index = pl_i(va, i);
		pde = pdes[i - 2][index];
		if ((pde & PTE_P) == 0) {
			*lastlvl = i;
			return false;
		}
		if (pde & PTE_PS)
			break;
	}
	if (lastpde != NULL)
		*lastpde = pde;
	*lastlvl = i;
	return true;
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
	paddr_t pa;
	bool rv;
	int lvl;

	if (__predict_false(pmap->pm_extract != NULL)) {
		return (*pmap->pm_extract)(pmap, va, pap);
	}

#ifdef __HAVE_DIRECT_MAP
	if (va >= PMAP_DIRECT_BASE && va < PMAP_DIRECT_END) {
		if (pap != NULL) {
			*pap = PMAP_DIRECT_UNMAP(va);
		}
		return true;
	}
#endif

	rv = false;
	pa = 0;

	if (pmap != pmap_kernel()) {
		mutex_enter(&pmap->pm_lock);
	}
	pmap_map_ptes(pmap, &pmap2, &ptes, &pdes);
	if (pmap_pdes_valid(va, pdes, &pde, &lvl)) {
		if (lvl == 2) {
			pa = (pde & PTE_LGFRAME) | (va & (NBPD_L2 - 1));
			rv = true;
		} else {
			KASSERT(lvl == 1);
			pte = ptes[pl1_i(va)];
			if (__predict_true((pte & PTE_P) != 0)) {
				pa = pmap_pte2pa(pte) | (va & (NBPD_L1 - 1));
				rv = true;
			}
		}
	}
	pmap_unmap_ptes(pmap, pmap2);
	if (pmap != pmap_kernel()) {
		mutex_exit(&pmap->pm_lock);
	}
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
		return pa;
	return 0;
}

__strict_weak_alias(pmap_extract_ma, pmap_extract);

#ifdef XENPV
/*
 * vtomach: virtual address to machine address.  For use by
 * machine-dependent code only.
 */
paddr_t
vtomach(vaddr_t va)
{
	paddr_t pa;

	if (pmap_extract_ma(pmap_kernel(), va, &pa) == true)
		return pa;
	return 0;
}
#endif

/*
 * pmap_virtual_space: used during bootup [pmap_steal_memory] to
 * determine the bounds of the kernel virtual address space.
 */
void
pmap_virtual_space(vaddr_t *startp, vaddr_t *endp)
{
	*startp = virtual_avail;
	*endp = virtual_end;
}

void
pmap_zero_page(paddr_t pa)
{
#if defined(__HAVE_DIRECT_MAP)
	memset(PAGE_ALIGNED(PMAP_DIRECT_MAP(pa)), 0, PAGE_SIZE);
#else
#if defined(XENPV)
	if (XEN_VERSION_SUPPORTED(3, 4)) {
		xen_pagezero(pa);
		return;
	}
#endif
	struct cpu_info *ci;
	pt_entry_t *zpte;
	vaddr_t zerova;

	const pd_entry_t pteflags = PTE_P | PTE_W | pmap_pg_nx | PTE_D | PTE_A;

	kpreempt_disable();

	ci = curcpu();
	zerova = ci->vpage[VPAGE_ZER];
	zpte = ci->vpage_pte[VPAGE_ZER];

	KASSERTMSG(!*zpte, "pmap_zero_page: lock botch");

	pmap_pte_set(zpte, pmap_pa2pte(pa) | pteflags);
	pmap_pte_flush();
	pmap_update_pg(zerova);		/* flush TLB */

	memset(PAGE_ALIGNED(zerova), 0, PAGE_SIZE);

#if defined(DIAGNOSTIC) || defined(XENPV)
	pmap_pte_set(zpte, 0);				/* zap ! */
	pmap_pte_flush();
#endif

	kpreempt_enable();
#endif /* defined(__HAVE_DIRECT_MAP) */
}

void
pmap_copy_page(paddr_t srcpa, paddr_t dstpa)
{
#if defined(__HAVE_DIRECT_MAP)
	vaddr_t srcva = PMAP_DIRECT_MAP(srcpa);
	vaddr_t dstva = PMAP_DIRECT_MAP(dstpa);

	memcpy(PAGE_ALIGNED(dstva), PAGE_ALIGNED(srcva), PAGE_SIZE);
#else
#if defined(XENPV)
	if (XEN_VERSION_SUPPORTED(3, 4)) {
		xen_copy_page(srcpa, dstpa);
		return;
	}
#endif
	struct cpu_info *ci;
	pt_entry_t *srcpte, *dstpte;
	vaddr_t srcva, dstva;

	const pd_entry_t pteflags = PTE_P | PTE_W | pmap_pg_nx | PTE_A;

	kpreempt_disable();

	ci = curcpu();
	srcva = ci->vpage[VPAGE_SRC];
	dstva = ci->vpage[VPAGE_DST];
	srcpte = ci->vpage_pte[VPAGE_SRC];
	dstpte = ci->vpage_pte[VPAGE_DST];

	KASSERT(*srcpte == 0 && *dstpte == 0);

	pmap_pte_set(srcpte, pmap_pa2pte(srcpa) | pteflags);
	pmap_pte_set(dstpte, pmap_pa2pte(dstpa) | pteflags | PTE_D);
	pmap_pte_flush();
	pmap_update_pg(srcva);
	pmap_update_pg(dstva);

	memcpy(PAGE_ALIGNED(dstva), PAGE_ALIGNED(srcva), PAGE_SIZE);

#if defined(DIAGNOSTIC) || defined(XENPV)
	pmap_pte_set(srcpte, 0);
	pmap_pte_set(dstpte, 0);
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
	struct cpu_info *ci;
	pt_entry_t *ptppte;
	vaddr_t ptpva;

	KASSERT(kpreempt_disabled());

#ifndef XENPV
	const pd_entry_t pteflags = PTE_P | PTE_W | pmap_pg_nx | PTE_A | PTE_D;
#else
	const pd_entry_t pteflags = PTE_P | pmap_pg_nx | PTE_A | PTE_D;
#endif

	ci = curcpu();
	ptpva = ci->vpage[VPAGE_PTP];
	ptppte = ci->vpage_pte[VPAGE_PTP];

	pmap_pte_set(ptppte, pmap_pa2pte(VM_PAGE_TO_PHYS(ptp)) | pteflags);

	pmap_pte_flush();
	pmap_update_pg(ptpva);

	return (pt_entry_t *)ptpva;
#endif
}

static void
pmap_unmap_ptp(void)
{
#ifndef __HAVE_DIRECT_MAP
#if defined(DIAGNOSTIC) || defined(XENPV)
	struct cpu_info *ci;
	pt_entry_t *pte;

	KASSERT(kpreempt_disabled());

	ci = curcpu();
	pte = ci->vpage_pte[VPAGE_PTP];

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
    vaddr_t startva, vaddr_t endva)
{
	pt_entry_t *pte = (pt_entry_t *)ptpva;

	KASSERT(mutex_owned(&pmap->pm_lock));
	KASSERT(kpreempt_disabled());

	/*
	 * mappings are very often sparse, so clip the given range to the
	 * range of PTEs that are known present in the PTP.
	 */
	pmap_ptp_range_clip(ptp, &startva, &pte);

	/*
	 * note that ptpva points to the PTE that maps startva.   this may
	 * or may not be the first PTE in the PTP.
	 *
	 * we loop through the PTP while there are still PTEs to look at
	 * and the wire_count is greater than 1 (because we use the wire_count
	 * to keep track of the number of real PTEs in the PTP).
	 */
	while (startva < endva && (ptp == NULL || ptp->wire_count > 1)) {
		(void)pmap_remove_pte(pmap, ptp, pte, startva);
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
    vaddr_t va)
{
	struct pv_entry *pve;
	struct vm_page *pg;
	struct pmap_page *pp;
	pt_entry_t opte;

	KASSERT(mutex_owned(&pmap->pm_lock));
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
			opte |= PTE_A;
		}
	}

	if ((opte & PTE_A) != 0) {
		pmap_tlb_shootdown(pmap, va, opte, TLBSHOOT_REMOVE_PTE);
	}

	/*
	 * If we are not on a pv list - we are done.
	 */
	if ((opte & PTE_PVLIST) == 0) {
#ifndef DOM0OPS
		KASSERTMSG((PHYS_TO_VM_PAGE(pmap_pte2pa(opte)) == NULL),
		    "managed page without PTE_PVLIST for %#"PRIxVADDR, va);
		KASSERTMSG((pmap_pv_tracked(pmap_pte2pa(opte)) == NULL),
		    "pv-tracked page without PTE_PVLIST for %#"PRIxVADDR, va);
#endif
		KASSERT(pmap_treelookup_pv(pmap, ptp, (ptp != NULL ?
		    &VM_PAGE_TO_PP(ptp)->pp_rb : &pmap_kernel_rb), va) == NULL);
		return true;
	}

	if ((pg = PHYS_TO_VM_PAGE(pmap_pte2pa(opte))) != NULL) {
		pp = VM_PAGE_TO_PP(pg);
	} else if ((pp = pmap_pv_tracked(pmap_pte2pa(opte))) == NULL) {
		paddr_t pa = pmap_pte2pa(opte);
		panic("%s: PTE_PVLIST with pv-untracked page"
		    " va = %#"PRIxVADDR"pa = %#"PRIxPADDR" (%#"PRIxPADDR")",
		    __func__, va, pa, atop(pa));
	}

	/* Sync R/M bits. */
	pve = pmap_lookup_pv(pmap, ptp, pp, va);
	pmap_remove_pv(pmap, pp, ptp, va, pve, pmap_pte_to_pp_attrs(opte));
	return true;
}

static void
pmap_remove_locked(struct pmap *pmap, vaddr_t sva, vaddr_t eva)
{
	pt_entry_t *ptes;
	pd_entry_t pde;
	pd_entry_t * const *pdes;
	bool result;
	vaddr_t blkendva, va = sva;
	struct vm_page *ptp;
	struct pmap *pmap2;
	int lvl;

	KASSERT(mutex_owned(&pmap->pm_lock));

	pmap_map_ptes(pmap, &pmap2, &ptes, &pdes);

	/*
	 * removing one page?  take shortcut function.
	 */

	if (va + PAGE_SIZE == eva) {
		if (pmap_pdes_valid(va, pdes, &pde, &lvl)) {
			KASSERT(lvl == 1);

			/* Get PTP if non-kernel mapping. */
			if (pmap != pmap_kernel()) {
				ptp = pmap_find_ptp(pmap, va, 1);
				KASSERTMSG(ptp != NULL,
				    "%s: unmanaged PTP detected", __func__);
			} else {
				/* Never free kernel PTPs. */
				ptp = NULL;
			}

			result = pmap_remove_pte(pmap, ptp,
			    &ptes[pl1_i(va)], va);

			/*
			 * if mapping removed and the PTP is no longer
			 * being used, free it!
			 */

			if (result && ptp && ptp->wire_count <= 1)
				pmap_free_ptp(pmap, ptp, va, ptes, pdes);
		}
	} else for (/* null */ ; va < eva ; va = blkendva) {
		/* determine range of block */
		blkendva = x86_round_pdr(va+1);
		if (blkendva > eva)
			blkendva = eva;

		if (!pmap_pdes_valid(va, pdes, &pde, &lvl)) {
			/* Skip a range corresponding to an invalid pde. */
			blkendva = (va & ptp_frames[lvl - 1]) + nbpd[lvl - 1];
			continue;
		}
		KASSERT(lvl == 1);

		/* Get PTP if non-kernel mapping. */
		if (pmap != pmap_kernel()) {
			ptp = pmap_find_ptp(pmap, va, 1);
			KASSERTMSG(ptp != NULL, "%s: unmanaged PTP detected",
			    __func__);
		} else {
			/* Never free kernel PTPs. */
			ptp = NULL;
		}

		pmap_remove_ptes(pmap, ptp, (vaddr_t)&ptes[pl1_i(va)], va,
		    blkendva);

		/* If PTP is no longer being used, free it. */
		if (ptp && ptp->wire_count <= 1) {
			pmap_free_ptp(pmap, ptp, va, ptes, pdes);
		}
	}
	pmap_unmap_ptes(pmap, pmap2);
	pmap_drain_pv(pmap);
}

/*
 * pmap_remove: mapping removal function.
 *
 * => caller should not be holding any pmap locks
 */
void
pmap_remove(struct pmap *pmap, vaddr_t sva, vaddr_t eva)
{
	if (__predict_false(pmap->pm_remove != NULL)) {
		(*pmap->pm_remove)(pmap, sva, eva);
		return;
	}

	mutex_enter(&pmap->pm_lock);
	pmap_remove_locked(pmap, sva, eva);
	mutex_exit(&pmap->pm_lock);
}

/*
 * pmap_sync_pv: clear pte bits and return the old value of the pp_attrs.
 *
 * => The 'clearbits' parameter is either ~0 or PP_ATTRS_...
 * => Caller should disable kernel preemption.
 * => issues tlb shootdowns if necessary.
 */
static int
pmap_sync_pv(struct pv_pte *pvpte, paddr_t pa, int clearbits, uint8_t *oattrs,
    pt_entry_t *optep)
{
	struct pmap *pmap;
	struct vm_page *ptp;
	vaddr_t va;
	pt_entry_t *ptep;
	pt_entry_t opte;
	pt_entry_t npte;
	pt_entry_t expect;
	bool need_shootdown;

	ptp = pvpte->pte_ptp;
	va = pvpte->pte_va;
	KASSERT(ptp == NULL || ptp->uobject != NULL);
	KASSERT(ptp == NULL || ptp_va2o(va, 1) == ptp->offset);
	pmap = ptp_to_pmap(ptp);
	KASSERT(kpreempt_disabled());

	if (__predict_false(pmap->pm_sync_pv != NULL)) {
		return (*pmap->pm_sync_pv)(ptp, va, pa, clearbits, oattrs,
		    optep);
	}

	expect = pmap_pa2pte(pa) | PTE_P;

	if (clearbits != ~0) {
		KASSERT((clearbits & ~(PP_ATTRS_D|PP_ATTRS_A|PP_ATTRS_W)) == 0);
		clearbits = pmap_pp_attrs_to_pte(clearbits);
	}

	ptep = pmap_map_pte(pmap, ptp, va);
	do {
		opte = *ptep;
		KASSERT((opte & (PTE_D | PTE_A)) != PTE_D);
		KASSERT((opte & (PTE_A | PTE_P)) != PTE_A);
		KASSERT(opte == 0 || (opte & PTE_P) != 0);
		if ((opte & (PTE_FRAME | PTE_P)) != expect) {
			/*
			 * We lost a race with a V->P operation like
			 * pmap_remove().  Wait for the competitor
			 * reflecting pte bits into mp_attrs.
			 */
			pmap_unmap_pte();
			return EAGAIN;
		}

		/*
		 * Check if there's anything to do on this PTE.
		 */
		if ((opte & clearbits) == 0) {
			need_shootdown = false;
			break;
		}

		/*
		 * We need a shootdown if the PTE is cached (PTE_A) ...
		 * ... Unless we are clearing only the PTE_W bit and
		 * it isn't cached as RW (PTE_D).
		 */
		need_shootdown = (opte & PTE_A) != 0 &&
		    !(clearbits == PTE_W && (opte & PTE_D) == 0);

		npte = opte & ~clearbits;

		/*
		 * If we need a shootdown anyway, clear PTE_A and PTE_D.
		 */
		if (need_shootdown) {
			npte &= ~(PTE_A | PTE_D);
		}
		KASSERT((npte & (PTE_D | PTE_A)) != PTE_D);
		KASSERT((npte & (PTE_A | PTE_P)) != PTE_A);
		KASSERT(npte == 0 || (opte & PTE_P) != 0);
	} while (pmap_pte_cas(ptep, opte, npte) != opte);

	if (need_shootdown) {
		pmap_tlb_shootdown(pmap, va, opte, TLBSHOOT_SYNC_PV);
	}
	pmap_unmap_pte();

	*oattrs = pmap_pte_to_pp_attrs(opte);
	if (optep != NULL)
		*optep = opte;
	return 0;
}

static void
pmap_pp_remove_ent(struct pmap *pmap, struct vm_page *ptp, pt_entry_t opte,
    vaddr_t va)
{
	struct pmap *pmap2;
	pt_entry_t *ptes;
	pd_entry_t * const *pdes;

	KASSERT(mutex_owned(&pmap->pm_lock));

	pmap_map_ptes(pmap, &pmap2, &ptes, &pdes);
	pmap_stats_update_bypte(pmap, 0, opte);
	ptp->wire_count--;
	if (ptp->wire_count <= 1) {
		pmap_free_ptp(pmap, ptp, va, ptes, pdes);
	}
	pmap_unmap_ptes(pmap, pmap2);
}

static void
pmap_pp_remove(struct pmap_page *pp, paddr_t pa)
{
	struct pv_pte *pvpte;
	struct vm_page *ptp;
	uintptr_t sum;
	uint8_t oattrs;
	bool locked;

	/*
	 * Do an unlocked check to see if the page has no mappings, eg when
	 * pmap_remove_all() was called before amap_wipeout() for a process
	 * private amap - common.  The page being removed must be on the way
	 * out, so we don't have to worry about concurrent attempts to enter
	 * it (otherwise the caller either doesn't care or has screwed up).
	 */
	sum = (uintptr_t)atomic_load_relaxed(&pp->pp_pte.pte_va);
	sum |= (uintptr_t)atomic_load_relaxed(&pp->pp_pte.pte_ptp);
	sum |= (uintptr_t)atomic_load_relaxed(&pp->pp_pvlist.lh_first);
	if (sum == 0) {
		return;
	}

	kpreempt_disable();
	for (;;) {
		struct pmap *pmap;
		struct pv_entry *pve;
		pt_entry_t opte;
		vaddr_t va;

		mutex_spin_enter(&pp->pp_lock);
		if ((pvpte = pv_pte_first(pp)) == NULL) {
			mutex_spin_exit(&pp->pp_lock);
			break;
		}

		/*
		 * Add a reference to the pmap before clearing the pte.
		 * Otherwise the pmap can disappear behind us.
		 */
		ptp = pvpte->pte_ptp;
		pmap = ptp_to_pmap(ptp);
		KASSERT(pmap->pm_obj[0].uo_refs > 0);
		if (ptp != NULL) {
			pmap_reference(pmap);
		}

		/*
		 * Now try to lock it.  We need a direct handoff between
		 * pp_lock and pm_lock to know the pv_entry is kept intact
		 * and kept associated with this pmap.  If that can't be
		 * had, wait for the pmap's lock to become free and then
		 * retry.
		 */
		locked = mutex_tryenter(&pmap->pm_lock);
		mutex_spin_exit(&pp->pp_lock);
		if (!locked) {
			mutex_enter(&pmap->pm_lock);
			/* nothing, just wait for it */
			mutex_exit(&pmap->pm_lock);
			if (ptp != NULL) {
				pmap_destroy(pmap);
			}
			continue;
		}
		va = pvpte->pte_va;

		KASSERTMSG(pmap->pm_stats.resident_count > PDP_SIZE,
		    "va %lx pmap %p ptp %p is empty", va, pmap, ptp);
		KASSERTMSG(ptp == NULL || (ptp->flags & PG_FREE) == 0,
		    "va %lx pmap %p ptp %p is free", va, pmap, ptp);
		KASSERTMSG(ptp == NULL || ptp->wire_count > 1,
		    "va %lx pmap %p ptp %p is empty", va, pmap, ptp);

#ifdef DEBUG
		pmap_check_pv(pmap, ptp, pp, pvpte->pte_va, true);
		rb_tree_t *tree = (ptp != NULL ?
		    &VM_PAGE_TO_PP(ptp)->pp_rb : &pmap_kernel_rb);
		pve = pmap_treelookup_pv(pmap, ptp, tree, va);
		if (pve == NULL) {
			KASSERTMSG(&pp->pp_pte == pvpte,
			    "va %lx pmap %p ptp %p pvpte %p pve %p oops 1",
			    va, pmap, ptp, pvpte, pve);
		} else {
			KASSERTMSG(&pve->pve_pte == pvpte,
			    "va %lx pmap %p ptp %p pvpte %p pve %p oops 2",
			    va, pmap, ptp, pvpte, pve);
		}
#endif

		if (pmap_sync_pv(pvpte, pa, ~0, &oattrs, &opte)) {
			panic("pmap_pp_remove: mapping not present");
		}

		pve = pmap_lookup_pv(pmap, ptp, pp, va);
		pmap_remove_pv(pmap, pp, ptp, va, pve, oattrs);

		/* Update the PTP reference count. Free if last reference. */
		if (ptp != NULL) {
			KASSERT(pmap != pmap_kernel());
			pmap_tlb_shootnow();
			if (__predict_false(pmap->pm_pp_remove_ent != NULL)) {
				(*pmap->pm_pp_remove_ent)(pmap, ptp, opte, va);
			} else {
				pmap_pp_remove_ent(pmap, ptp, opte, va);
			}
		} else {
			KASSERT(pmap == pmap_kernel());
			pmap_stats_update_bypte(pmap, 0, opte);
		}
		pmap_tlb_shootnow();
		pmap_drain_pv(pmap);
		mutex_exit(&pmap->pm_lock);
		if (ptp != NULL) {
			pmap_destroy(pmap);
		}
	}
	kpreempt_enable();
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
	paddr_t pa;

	pp = VM_PAGE_TO_PP(pg);
	pa = VM_PAGE_TO_PHYS(pg);
	pmap_pp_remove(pp, pa);
}

/*
 * pmap_pv_remove: remove an unmanaged pv-tracked page from all pmaps
 * that map it
 */
void
pmap_pv_remove(paddr_t pa)
{
	struct pmap_page *pp;

	pp = pmap_pv_tracked(pa);
	if (pp == NULL)
		panic("%s: page not pv-tracked: %#"PRIxPADDR, __func__, pa);
	pmap_pp_remove(pp, pa);
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
	struct pmap *pmap;
	uint8_t oattrs;
	u_int result;
	paddr_t pa;

	pp = VM_PAGE_TO_PP(pg);
	if ((pp->pp_attrs & testbits) != 0) {
		return true;
	}
	pa = VM_PAGE_TO_PHYS(pg);
 startover:
	mutex_spin_enter(&pp->pp_lock);
	for (pvpte = pv_pte_first(pp); pvpte; pvpte = pv_pte_next(pp, pvpte)) {
		if ((pp->pp_attrs & testbits) != 0) {
			break;
		}
		if (pmap_sync_pv(pvpte, pa, 0, &oattrs, NULL)) {
			/*
			 * raced with a V->P operation.  wait for the other
			 * side to finish by acquiring pmap's lock.  if no
			 * wait, updates to pp_attrs by the other side may
			 * go unseen.
			 */
			pmap = ptp_to_pmap(pvpte->pte_ptp);
			pmap_reference(pmap);
			mutex_spin_exit(&pp->pp_lock);
			mutex_enter(&pmap->pm_lock);
			/* nothing. */
			mutex_exit(&pmap->pm_lock);
			pmap_destroy(pmap);
			goto startover;
		}
		pp->pp_attrs |= oattrs;
	}
	result = pp->pp_attrs & testbits;
	mutex_spin_exit(&pp->pp_lock);

	/*
	 * note that we will exit the for loop with a non-null pve if
	 * we have found the bits we are testing for.
	 */

	return result != 0;
}

static bool
pmap_pp_clear_attrs(struct pmap_page *pp, paddr_t pa, unsigned clearbits)
{
	struct pv_pte *pvpte;
	struct pmap *pmap;
	uint8_t oattrs;
	u_int result;

startover:
	mutex_spin_enter(&pp->pp_lock);
	for (pvpte = pv_pte_first(pp); pvpte; pvpte = pv_pte_next(pp, pvpte)) {
		if (pmap_sync_pv(pvpte, pa, clearbits, &oattrs, NULL)) {
			/*
			 * raced with a V->P operation.  wait for the other
			 * side to finish by acquiring pmap's lock.  it is
			 * probably unmapping the page, and it will be gone
			 * when the loop is restarted.
			 */
			pmap = ptp_to_pmap(pvpte->pte_ptp);
			pmap_reference(pmap);
			mutex_spin_exit(&pp->pp_lock);
			mutex_enter(&pmap->pm_lock);
			/* nothing. */
			mutex_exit(&pmap->pm_lock);
			pmap_destroy(pmap);
			goto startover;
		}
		pp->pp_attrs |= oattrs;
	}
	result = pp->pp_attrs & clearbits;
	pp->pp_attrs &= ~clearbits;
	pmap_tlb_shootnow();
	mutex_spin_exit(&pp->pp_lock);

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
	paddr_t pa;

	pp = VM_PAGE_TO_PP(pg);
	pa = VM_PAGE_TO_PHYS(pg);

	/*
	 * If this is a new page, assert it has no mappings and simply zap
	 * the stored attributes without taking any locks.
	 */
	if ((pg->flags & PG_FAKE) != 0) {
		KASSERT(atomic_load_relaxed(&pp->pp_pte.pte_va) == 0);
		KASSERT(atomic_load_relaxed(&pp->pp_pte.pte_ptp) == NULL);
		KASSERT(atomic_load_relaxed(&pp->pp_pvlist.lh_first) == NULL);
		atomic_store_relaxed(&pp->pp_attrs, 0);
		return false;
	} else {
		return pmap_pp_clear_attrs(pp, pa, clearbits);
	}
}

/*
 * pmap_pv_clear_attrs: clear the specified attributes for an unmanaged
 * pv-tracked page.
 */
bool
pmap_pv_clear_attrs(paddr_t pa, unsigned clearbits)
{
	struct pmap_page *pp;

	pp = pmap_pv_tracked(pa);
	if (pp == NULL)
		panic("%s: page not pv-tracked: %#"PRIxPADDR, __func__, pa);

	return pmap_pp_clear_attrs(pp, pa, clearbits);
}

/*
 * p m a p   p r o t e c t i o n   f u n c t i o n s
 */

/*
 * pmap_page_protect: change the protection of all recorded mappings
 * of a managed page
 *
 * => NOTE: this is an inline function in pmap.h
 */

/* see pmap.h */

/*
 * pmap_pv_protect: change the protection of all recorded mappings
 * of an unmanaged pv-tracked page
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
 *
 * Note for Xen-amd64. Xen automatically adds PTE_U to the kernel pages, but we
 * don't need to remove this bit when re-entering the PTEs here: Xen tracks the
 * kernel pages with a reserved bit (_PAGE_GUEST_KERNEL), so even if PTE_U is
 * present the page will still be considered as a kernel page, and the privilege
 * separation will be enforced correctly.
 */
void
pmap_write_protect(struct pmap *pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
	pt_entry_t bit_rem, bit_put;
	pt_entry_t *ptes;
	pt_entry_t * const *pdes;
	struct pmap *pmap2;
	vaddr_t blockend, va;
	int lvl, i;

	if (__predict_false(pmap->pm_write_protect != NULL)) {
		(*pmap->pm_write_protect)(pmap, sva, eva, prot);
		return;
	}

	bit_rem = 0;
	if (!(prot & VM_PROT_WRITE))
		bit_rem = PTE_W;

	bit_put = 0;
	if (!(prot & VM_PROT_EXECUTE))
		bit_put = pmap_pg_nx;

	sva &= ~PAGE_MASK;
	eva &= ~PAGE_MASK;

	/*
	 * Acquire pmap.  No need to lock the kernel pmap as we won't
	 * be touching PV entries nor stats and kernel PDEs aren't
	 * freed.
	 */
	if (pmap != pmap_kernel()) {
		mutex_enter(&pmap->pm_lock);
	}
	pmap_map_ptes(pmap, &pmap2, &ptes, &pdes);

	for (va = sva ; va < eva; va = blockend) {
		pt_entry_t *spte, *epte;

		blockend = x86_round_pdr(va + 1);
		if (blockend > eva)
			blockend = eva;

		/* Is it a valid block? */
		if (!pmap_pdes_valid(va, pdes, NULL, &lvl)) {
			continue;
		}
		KASSERT(va < VM_MAXUSER_ADDRESS || va >= VM_MAX_ADDRESS);
		KASSERT(lvl == 1);

		spte = &ptes[pl1_i(va)];
		epte = &ptes[pl1_i(blockend)];

		for (i = 0; spte < epte; spte++, i++) {
			pt_entry_t opte, npte;

			do {
				opte = *spte;
				if (!pmap_valid_entry(opte)) {
					goto next;
				}
				npte = (opte & ~bit_rem) | bit_put;
			} while (pmap_pte_cas(spte, opte, npte) != opte);

			if ((opte & PTE_D) != 0) {
				vaddr_t tva = va + x86_ptob(i);
				pmap_tlb_shootdown(pmap, tva, opte,
				    TLBSHOOT_WRITE_PROTECT);
			}
next:;
		}
	}

	/* Release pmap. */
	pmap_unmap_ptes(pmap, pmap2);
	if (pmap != pmap_kernel()) {
		mutex_exit(&pmap->pm_lock);
	}
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
	int lvl;

	if (__predict_false(pmap->pm_unwire != NULL)) {
		(*pmap->pm_unwire)(pmap, va);
		return;
	}

	/*
	 * Acquire pmap.  Need to lock the kernel pmap only to protect the
	 * statistics.
	 */
	mutex_enter(&pmap->pm_lock);
	pmap_map_ptes(pmap, &pmap2, &ptes, &pdes);

	if (!pmap_pdes_valid(va, pdes, NULL, &lvl)) {
		panic("%s: invalid PDE va=%#" PRIxVADDR, __func__, va);
	}
	KASSERT(lvl == 1);

	ptep = &ptes[pl1_i(va)];
	opte = *ptep;
	KASSERT(pmap_valid_entry(opte));

	if (opte & PTE_WIRED) {
		pt_entry_t npte = opte & ~PTE_WIRED;

		opte = pmap_pte_testset(ptep, npte);
		pmap_stats_update_bypte(pmap, npte, opte);
	} else {
		printf("%s: wiring for pmap %p va %#" PRIxVADDR
		    " did not change!\n", __func__, pmap, va);
	}

	/* Release pmap. */
	pmap_unmap_ptes(pmap, pmap2);
	mutex_exit(&pmap->pm_lock);
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
	if (__predict_false(pmap->pm_enter != NULL)) {
		return (*pmap->pm_enter)(pmap, va, pa, prot, flags);
	}

	return pmap_enter_ma(pmap, va, pa, pa, prot, flags, 0);
}

/*
 * pmap_enter: enter a mapping into a pmap
 *
 * => must be done "now" ... no lazy-evaluation
 */
int
pmap_enter_ma(struct pmap *pmap, vaddr_t va, paddr_t ma, paddr_t pa,
	   vm_prot_t prot, u_int flags, int domid)
{
	pt_entry_t *ptes, opte, npte;
	pt_entry_t *ptep;
	pd_entry_t * const *pdes;
	struct vm_page *ptp;
	struct vm_page *new_pg, *old_pg;
	struct pmap_page *new_pp, *old_pp;
	struct pv_entry *old_pve, *new_pve;
	bool wired = (flags & PMAP_WIRED) != 0;
	struct pmap *pmap2;
	struct pmap_ptparray pt;
	int error;
	bool getptp, samepage, new_embedded;
	rb_tree_t *tree;

	KASSERT(pmap_initialized);
	KASSERT(va < VM_MAX_KERNEL_ADDRESS);
	KASSERTMSG(va != (vaddr_t)PDP_BASE, "%s: trying to map va=%#"
	    PRIxVADDR " over PDP!", __func__, va);
	KASSERTMSG(va < VM_MIN_KERNEL_ADDRESS ||
	    pmap_valid_entry(pmap->pm_pdir[pl_i(va, PTP_LEVELS)]),
	    "%s: missing kernel PTP for va=%#" PRIxVADDR, __func__, va);

#ifdef XENPV
	KASSERT(domid == DOMID_SELF || pa == 0);
#endif

	npte = ma | protection_codes[prot] | PTE_P;
	npte |= pmap_pat_flags(flags);
	if (wired)
		npte |= PTE_WIRED;
	if (va < VM_MAXUSER_ADDRESS)
		npte |= PTE_U;

	if (pmap == pmap_kernel())
		npte |= pmap_pg_g;
	if (flags & VM_PROT_ALL) {
		npte |= PTE_A;
		if (flags & VM_PROT_WRITE) {
			KASSERT((npte & PTE_W) != 0);
			npte |= PTE_D;
		}
	}

#ifdef XENPV
	if (domid != DOMID_SELF)
		new_pg = NULL;
	else
#endif
		new_pg = PHYS_TO_VM_PAGE(pa);

	if (new_pg != NULL) {
		/* This is a managed page */
		npte |= PTE_PVLIST;
		new_pp = VM_PAGE_TO_PP(new_pg);
		PMAP_CHECK_PP(new_pp);
	} else if ((new_pp = pmap_pv_tracked(pa)) != NULL) {
		/* This is an unmanaged pv-tracked page */
		npte |= PTE_PVLIST;
		PMAP_CHECK_PP(new_pp);
	} else {
		new_pp = NULL;
	}

	/* Begin by locking the pmap. */
	mutex_enter(&pmap->pm_lock);

	/* Look up the PTP.  Allocate if none present. */
	ptp = NULL;
	getptp = false;
	if (pmap != pmap_kernel()) {
		ptp = pmap_find_ptp(pmap, va, 1);
		if (ptp == NULL) {
			getptp = true;
			error = pmap_get_ptp(pmap, &pt, va, flags, &ptp);
			if (error != 0) {
				if (flags & PMAP_CANFAIL) {
					mutex_exit(&pmap->pm_lock);
					return error;
				}
				panic("%s: get ptp failed, error=%d", __func__,
				    error);
			}
		}
		tree = &VM_PAGE_TO_PP(ptp)->pp_rb;
	} else {
		/* Embedded PV entries rely on this. */
		KASSERT(va != 0);
		tree = &pmap_kernel_rb;
	}

	/*
	 * Look up the old PV entry at this VA (if any), and insert a new PV
	 * entry if required for the new mapping.  Temporarily track the old
	 * and new mappings concurrently.  Only after the old mapping is
	 * evicted from the pmap will we remove its PV entry.  Otherwise,
	 * our picture of modified/accessed state for either page could get
	 * out of sync (we need any P->V operation for either page to stall
	 * on pmap->pm_lock until done here).
	 */
	new_pve = NULL;
	old_pve = NULL;
	samepage = false;
	new_embedded = false;

	if (new_pp != NULL) {
		error = pmap_enter_pv(pmap, new_pp, ptp, va, &new_pve,
		    &old_pve, &samepage, &new_embedded, tree);

		/*
		 * If a new pv_entry was needed and none was available, we
		 * can go no further.
		 */
		if (error != 0) {
			if (flags & PMAP_CANFAIL) {
				if (getptp) {
					pmap_unget_ptp(pmap, &pt);
				}
				mutex_exit(&pmap->pm_lock);
				return error;
			}
			panic("%s: alloc pve failed", __func__);
		}
	} else {
		old_pve = pmap_treelookup_pv(pmap, ptp, tree, va);
	}

	/* Map PTEs into address space. */
	pmap_map_ptes(pmap, &pmap2, &ptes, &pdes);

	/* Install any newly allocated PTPs. */
	if (getptp) {
		pmap_install_ptp(pmap, &pt, va, pdes);
	}

	/* Check if there is an existing mapping. */
	ptep = &ptes[pl1_i(va)];
	opte = *ptep;
	bool have_oldpa = pmap_valid_entry(opte);
	paddr_t oldpa = pmap_pte2pa(opte);

	/*
	 * Update the pte.
	 */
	do {
		opte = *ptep;

		/*
		 * if the same page, inherit PTE_A and PTE_D.
		 */
		if (((opte ^ npte) & (PTE_FRAME | PTE_P)) == 0) {
			npte |= opte & (PTE_A | PTE_D);
		}
#if defined(XENPV)
		if (domid != DOMID_SELF) {
			/* pmap_pte_cas with error handling */
			int s = splvm();
			if (opte != *ptep) {
				splx(s);
				continue;
			}
			error = xpq_update_foreign(
			    vtomach((vaddr_t)ptep), npte, domid, flags);
			splx(s);
			if (error) {
				/* Undo pv_entry tracking - oof. */
				if (new_pp != NULL) {
					mutex_spin_enter(&new_pp->pp_lock);
					if (new_pve != NULL) {
						LIST_REMOVE(new_pve, pve_list);
						KASSERT(pmap->pm_pve == NULL);
						pmap->pm_pve = new_pve;
					} else if (new_embedded) {
						new_pp->pp_pte.pte_ptp = NULL;
						new_pp->pp_pte.pte_va = 0;
					}
					mutex_spin_exit(&new_pp->pp_lock);
				}
				pmap_unmap_ptes(pmap, pmap2);
				/* Free new PTP. */
				if (ptp != NULL && ptp->wire_count <= 1) {
					pmap_free_ptp(pmap, ptp, va, ptes,
					    pdes);
				}
				mutex_exit(&pmap->pm_lock);
				return error;
			}
			break;
		}
#endif /* defined(XENPV) */
	} while (pmap_pte_cas(ptep, opte, npte) != opte);

	/*
	 * Done with the PTEs: they can now be unmapped.
	 */
	pmap_unmap_ptes(pmap, pmap2);

	/*
	 * Update statistics and PTP's reference count.
	 */
	pmap_stats_update_bypte(pmap, npte, opte);
	if (ptp != NULL) {
		if (!have_oldpa) {
			ptp->wire_count++;
		}
		/* Remember minimum VA in PTP. */
		pmap_ptp_range_set(ptp, va);
	}
	KASSERT(ptp == NULL || ptp->wire_count > 1);

	/*
	 * If the same page, we can skip pv_entry handling.
	 */
	if (((opte ^ npte) & (PTE_FRAME | PTE_P)) == 0) {
		KASSERT(((opte ^ npte) & PTE_PVLIST) == 0);
		if ((npte & PTE_PVLIST) != 0) {
			KASSERT(samepage);
			pmap_check_pv(pmap, ptp, new_pp, va, true);
		}
		goto same_pa;
	} else if ((npte & PTE_PVLIST) != 0) {
		KASSERT(!samepage);
	}

	/*
	 * If old page is pv-tracked, remove pv_entry from its list.
	 */
	if ((~opte & (PTE_P | PTE_PVLIST)) == 0) {
		if ((old_pg = PHYS_TO_VM_PAGE(oldpa)) != NULL) {
			old_pp = VM_PAGE_TO_PP(old_pg);
		} else if ((old_pp = pmap_pv_tracked(oldpa)) == NULL) {
			panic("%s: PTE_PVLIST with pv-untracked page"
			    " va = %#"PRIxVADDR
			    " pa = %#" PRIxPADDR " (%#" PRIxPADDR ")",
			    __func__, va, oldpa, atop(pa));
		}

		pmap_remove_pv(pmap, old_pp, ptp, va, old_pve,
		    pmap_pte_to_pp_attrs(opte));
	} else {
		KASSERT(old_pve == NULL);
		KASSERT(pmap_treelookup_pv(pmap, ptp, tree, va) == NULL);
	}

	/*
	 * If new page is dynamically PV tracked, insert to tree.
	 */
	if (new_pve != NULL) {
		KASSERT(pmap_treelookup_pv(pmap, ptp, tree, va) == NULL);
		old_pve = rb_tree_insert_node(tree, new_pve);
		KASSERT(old_pve == new_pve);
		pmap_check_pv(pmap, ptp, new_pp, va, true);
	}

same_pa:
	/*
	 * shootdown tlb if necessary.
	 */

	if ((~opte & (PTE_P | PTE_A)) == 0 &&
	    ((opte ^ npte) & (PTE_FRAME | PTE_W)) != 0) {
		pmap_tlb_shootdown(pmap, va, opte, TLBSHOOT_ENTER);
	}
	pmap_drain_pv(pmap);
	mutex_exit(&pmap->pm_lock);
	return 0;
}

#if defined(XEN) && defined(DOM0OPS)

struct pmap_data_gnt {
	SLIST_ENTRY(pmap_data_gnt) pd_gnt_list;
	vaddr_t pd_gnt_sva;
	vaddr_t pd_gnt_eva; /* range covered by this gnt */
	int pd_gnt_refs; /* ref counter */
	struct gnttab_map_grant_ref pd_gnt_ops[1]; /* variable length */
};
SLIST_HEAD(pmap_data_gnt_head, pmap_data_gnt);

static void pmap_remove_gnt(struct pmap *, vaddr_t, vaddr_t);

static struct pmap_data_gnt *
pmap_find_gnt(struct pmap *pmap, vaddr_t sva, vaddr_t eva)
{
	struct pmap_data_gnt_head *headp;
	struct pmap_data_gnt *pgnt;

	KASSERT(mutex_owned(&pmap->pm_lock));
	headp = pmap->pm_data;
	KASSERT(headp != NULL);
	SLIST_FOREACH(pgnt, headp, pd_gnt_list) {
		if (pgnt->pd_gnt_sva <= sva && eva <= pgnt->pd_gnt_eva)
			return pgnt;
		/* check that we're not overlapping part of a region */
		KASSERT(pgnt->pd_gnt_sva >= eva || pgnt->pd_gnt_eva <= sva);
	}
	return NULL;
}

static void
pmap_alloc_gnt(struct pmap *pmap, vaddr_t sva, int nentries,
    const struct gnttab_map_grant_ref *ops)
{
	struct pmap_data_gnt_head *headp;
	struct pmap_data_gnt *pgnt;
	vaddr_t eva = sva + nentries * PAGE_SIZE;
	KASSERT(mutex_owned(&pmap->pm_lock));
	KASSERT(nentries >= 1);
	if (pmap->pm_remove == NULL) {
		pmap->pm_remove = pmap_remove_gnt;
		KASSERT(pmap->pm_data == NULL);
		headp = kmem_alloc(sizeof(*headp), KM_SLEEP);
		SLIST_INIT(headp);
		pmap->pm_data = headp;
	} else {
		KASSERT(pmap->pm_remove == pmap_remove_gnt);
		KASSERT(pmap->pm_data != NULL);
		headp = pmap->pm_data;
	}

	pgnt = pmap_find_gnt(pmap, sva, eva);
	if (pgnt != NULL) {
		KASSERT(pgnt->pd_gnt_sva == sva);
		KASSERT(pgnt->pd_gnt_eva == eva);
		return;
	}

	/* new entry */
	pgnt = kmem_alloc(sizeof(*pgnt) +
	    (nentries - 1) * sizeof(struct gnttab_map_grant_ref), KM_SLEEP);
	pgnt->pd_gnt_sva = sva;
	pgnt->pd_gnt_eva = eva;
	pgnt->pd_gnt_refs = 0;
	memcpy(pgnt->pd_gnt_ops, ops,
	    sizeof(struct gnttab_map_grant_ref) * nentries);
	SLIST_INSERT_HEAD(headp, pgnt, pd_gnt_list);
}

static void
pmap_free_gnt(struct pmap *pmap, struct pmap_data_gnt *pgnt)
{
	struct pmap_data_gnt_head *headp = pmap->pm_data;
	int nentries = (pgnt->pd_gnt_eva - pgnt->pd_gnt_sva) / PAGE_SIZE;
	KASSERT(nentries >= 1);
	KASSERT(mutex_owned(&pmap->pm_lock));
	KASSERT(pgnt->pd_gnt_refs == 0);
	SLIST_REMOVE(headp, pgnt, pmap_data_gnt, pd_gnt_list);
	kmem_free(pgnt, sizeof(*pgnt) +
		    (nentries - 1) * sizeof(struct gnttab_map_grant_ref));
	if (SLIST_EMPTY(headp)) {
		kmem_free(headp, sizeof(*headp));
		pmap->pm_data = NULL;
		pmap->pm_remove = NULL;
	}
}

/*
 * pmap_enter_gnt: enter a grant entry into a pmap
 *
 * => must be done "now" ... no lazy-evaluation
 */
int
pmap_enter_gnt(struct pmap *pmap, vaddr_t va, vaddr_t sva, int nentries,
    const struct gnttab_map_grant_ref *oops)
{
	struct pmap_data_gnt *pgnt;
	pt_entry_t *ptes, opte;
	pt_entry_t *ptep;
	pd_entry_t * const *pdes;
	struct vm_page *ptp;
	struct vm_page *old_pg;
	struct pmap_page *old_pp;
	struct pv_entry *old_pve;
	struct pmap *pmap2;
	struct pmap_ptparray pt;
	int error;
	bool getptp;
	rb_tree_t *tree;
	struct gnttab_map_grant_ref *op;
	int ret;
	int idx;

	KASSERT(pmap_initialized);
	KASSERT(va < VM_MAX_KERNEL_ADDRESS);
	KASSERTMSG(va != (vaddr_t)PDP_BASE, "%s: trying to map va=%#"
	    PRIxVADDR " over PDP!", __func__, va);
	KASSERT(pmap != pmap_kernel());

	/* Begin by locking the pmap. */
	mutex_enter(&pmap->pm_lock);
	pmap_alloc_gnt(pmap, sva, nentries, oops);

	pgnt = pmap_find_gnt(pmap, va, va + PAGE_SIZE);
	KASSERT(pgnt != NULL);

	/* Look up the PTP.  Allocate if none present. */
	ptp = NULL;
	getptp = false;
	ptp = pmap_find_ptp(pmap, va, 1);
	if (ptp == NULL) {
		getptp = true;
		error = pmap_get_ptp(pmap, &pt, va, PMAP_CANFAIL, &ptp);
		if (error != 0) {
			mutex_exit(&pmap->pm_lock);
			return error;
		}
	}
	tree = &VM_PAGE_TO_PP(ptp)->pp_rb;

	/*
	 * Look up the old PV entry at this VA (if any), and insert a new PV
	 * entry if required for the new mapping.  Temporarily track the old
	 * and new mappings concurrently.  Only after the old mapping is
	 * evicted from the pmap will we remove its PV entry.  Otherwise,
	 * our picture of modified/accessed state for either page could get
	 * out of sync (we need any P->V operation for either page to stall
	 * on pmap->pm_lock until done here).
	 */
	old_pve = NULL;

	old_pve = pmap_treelookup_pv(pmap, ptp, tree, va);

	/* Map PTEs into address space. */
	pmap_map_ptes(pmap, &pmap2, &ptes, &pdes);

	/* Install any newly allocated PTPs. */
	if (getptp) {
		pmap_install_ptp(pmap, &pt, va, pdes);
	}

	/* Check if there is an existing mapping. */
	ptep = &ptes[pl1_i(va)];
	opte = *ptep;
	bool have_oldpa = pmap_valid_entry(opte);
	paddr_t oldpa = pmap_pte2pa(opte);

	/*
	 * Update the pte.
	 */

	idx = (va - pgnt->pd_gnt_sva) / PAGE_SIZE;
	op = &pgnt->pd_gnt_ops[idx];

#ifdef XENPV /* XXX */
	op->host_addr = xpmap_ptetomach(ptep);
#endif
	op->dev_bus_addr = 0;
	op->status = GNTST_general_error;
	ret = HYPERVISOR_grant_table_op(GNTTABOP_map_grant_ref, op, 1);
	if (__predict_false(ret)) {
		printf("%s: GNTTABOP_map_grant_ref failed: %d\n",
		    __func__, ret);
		op->status = GNTST_general_error;
	}
	for (int d = 0; d < 256 && op->status == GNTST_eagain; d++) {
		kpause("gntmap", false, mstohz(1), NULL);
		ret = HYPERVISOR_grant_table_op(GNTTABOP_map_grant_ref, op, 1);
		if (__predict_false(ret)) {
			printf("%s: GNTTABOP_map_grant_ref failed: %d\n",
			    __func__, ret);
			op->status = GNTST_general_error;
		}
	}
	if (__predict_false(op->status != GNTST_okay)) {
		printf("%s: GNTTABOP_map_grant_ref status: %d\n",
		    __func__, op->status);
		if (have_oldpa) {
			ptp->wire_count--;
		}
	} else {
		pgnt->pd_gnt_refs++;
		if (!have_oldpa) {
			ptp->wire_count++;
		}
		KASSERT(ptp->wire_count > 1);
		/* Remember minimum VA in PTP. */
		pmap_ptp_range_set(ptp, va);
	}
	if (ptp->wire_count <= 1)
		pmap_free_ptp(pmap, ptp, va, ptes, pdes);

	/*
	 * Done with the PTEs: they can now be unmapped.
	 */
	pmap_unmap_ptes(pmap, pmap2);

	/*
	 * Update statistics and PTP's reference count.
	 */
	pmap_stats_update_bypte(pmap, 0, opte);

	/*
	 * If old page is pv-tracked, remove pv_entry from its list.
	 */
	if ((~opte & (PTE_P | PTE_PVLIST)) == 0) {
		if ((old_pg = PHYS_TO_VM_PAGE(oldpa)) != NULL) {
			old_pp = VM_PAGE_TO_PP(old_pg);
		} else if ((old_pp = pmap_pv_tracked(oldpa)) == NULL) {
			panic("%s: PTE_PVLIST with pv-untracked page"
			    " va = %#"PRIxVADDR " pa = %#" PRIxPADDR,
			    __func__, va, oldpa);
		}

		pmap_remove_pv(pmap, old_pp, ptp, va, old_pve,
		    pmap_pte_to_pp_attrs(opte));
	} else {
		KASSERT(old_pve == NULL);
		KASSERT(pmap_treelookup_pv(pmap, ptp, tree, va) == NULL);
	}

	pmap_drain_pv(pmap);
	mutex_exit(&pmap->pm_lock);
	return op->status;
}

/*
 * pmap_remove_gnt: grant mapping removal function.
 *
 * => caller should not be holding any pmap locks
 */
static void
pmap_remove_gnt(struct pmap *pmap, vaddr_t sva, vaddr_t eva)
{
	struct pmap_data_gnt *pgnt;
	pt_entry_t *ptes;
	pd_entry_t pde;
	pd_entry_t * const *pdes;
	struct vm_page *ptp;
	struct pmap *pmap2;
	vaddr_t va;
	int lvl;
	int idx;
	struct gnttab_map_grant_ref *op;
	struct gnttab_unmap_grant_ref unmap_op;
	int ret;

	KASSERT(pmap != pmap_kernel());
	KASSERT(pmap->pm_remove == pmap_remove_gnt);

	mutex_enter(&pmap->pm_lock);
	for (va = sva; va < eva; va += PAGE_SIZE) {
		pgnt = pmap_find_gnt(pmap, va, va + PAGE_SIZE);
		if (pgnt == NULL) {
			pmap_remove_locked(pmap, sva, eva);
			continue;
		}

		pmap_map_ptes(pmap, &pmap2, &ptes, &pdes);
		if (!pmap_pdes_valid(va, pdes, &pde, &lvl)) {
			panic("pmap_remove_gnt pdes not valid");
		}

		idx = (va - pgnt->pd_gnt_sva) / PAGE_SIZE;
		op = &pgnt->pd_gnt_ops[idx];
		KASSERT(lvl == 1);
		KASSERT(op->status == GNTST_okay);

		/* Get PTP if non-kernel mapping. */
		ptp = pmap_find_ptp(pmap, va, 1);
		KASSERTMSG(ptp != NULL,
		    "%s: unmanaged PTP detected", __func__);

		if (op->status == GNTST_okay)  {
			KASSERT(pmap_valid_entry(ptes[pl1_i(va)]));
			unmap_op.handle = op->handle;
			unmap_op.dev_bus_addr = 0;
#ifdef XENPV /* XXX */
			unmap_op.host_addr = xpmap_ptetomach(&ptes[pl1_i(va)]);
#endif
			ret = HYPERVISOR_grant_table_op(
			    GNTTABOP_unmap_grant_ref, &unmap_op, 1);
			if (ret) {
				printf("%s: GNTTABOP_unmap_grant_ref "
				    "failed: %d\n", __func__, ret);
			}

			ptp->wire_count--;
			pgnt->pd_gnt_refs--;
			if (pgnt->pd_gnt_refs == 0) {
				pmap_free_gnt(pmap, pgnt);
			}
		}
		/*
		 * if mapping removed and the PTP is no longer
		 * being used, free it!
		 */

		if (ptp->wire_count <= 1)
			pmap_free_ptp(pmap, ptp, va, ptes, pdes);
		pmap_unmap_ptes(pmap, pmap2);
	}
	mutex_exit(&pmap->pm_lock);
}
#endif /* XEN && DOM0OPS */

paddr_t
pmap_get_physpage(void)
{
	struct vm_page *ptp;
	struct pmap *kpm = pmap_kernel();
	paddr_t pa;

	if (!uvm.page_init_done) {
		/*
		 * We're growing the kernel pmap early (from
		 * uvm_pageboot_alloc()). This case must be
		 * handled a little differently.
		 */

		if (!uvm_page_physget(&pa))
			panic("%s: out of memory", __func__);
#if defined(__HAVE_DIRECT_MAP)
		memset(PAGE_ALIGNED(PMAP_DIRECT_MAP(pa)), 0, PAGE_SIZE);
#else
#if defined(XENPV)
		if (XEN_VERSION_SUPPORTED(3, 4)) {
			xen_pagezero(pa);
			return pa;
		}
#endif
		kpreempt_disable();
		pmap_pte_set(early_zero_pte, pmap_pa2pte(pa) | PTE_P |
		    PTE_W | pmap_pg_nx);
		pmap_pte_flush();
		pmap_update_pg((vaddr_t)early_zerop);
		memset(PAGE_ALIGNED(early_zerop), 0, PAGE_SIZE);
#if defined(DIAGNOSTIC) || defined(XENPV)
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
			panic("%s: out of memory", __func__);
		ptp->flags &= ~PG_BUSY;
		ptp->wire_count = 1;
		pa = VM_PAGE_TO_PHYS(ptp);
	}
	pmap_stats_update(kpm, 1, 0);

	return pa;
}

/*
 * Expand the page tree with the specified amount of PTPs, mapping virtual
 * addresses starting at kva. We populate all the levels but the last one
 * (L1). The nodes of the tree are created as RW, but the pages covered
 * will be kentered in L1, with proper permissions.
 *
 * Used only by pmap_growkernel.
 */
static void
pmap_alloc_level(struct pmap *cpm, vaddr_t kva, long *needed_ptps)
{
	unsigned long i;
	paddr_t pa;
	unsigned long index, endindex;
	int level;
	pd_entry_t *pdep;
#ifdef XENPV
	int s = splvm(); /* protect xpq_* */
#endif

	for (level = PTP_LEVELS; level > 1; level--) {
		if (level == PTP_LEVELS)
			pdep = cpm->pm_pdir;
		else
			pdep = normal_pdes[level - 2];
		index = pl_i_roundup(kva, level);
		endindex = index + needed_ptps[level - 1] - 1;

		for (i = index; i <= endindex; i++) {
			pt_entry_t pte;

			KASSERT(!pmap_valid_entry(pdep[i]));
			pa = pmap_get_physpage();
			pte = pmap_pa2pte(pa) | PTE_P | PTE_W;
#ifdef __x86_64__
			pte |= pmap_pg_nx;
#endif
			pmap_pte_set(&pdep[i], pte);

#ifdef XENPV
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
#ifdef __x86_64__
					pd_entry_t *cpu_pdep =
						&cpu_info_primary.ci_kpm_pdir[i];
#else
					pd_entry_t *cpu_pdep =
					    &cpu_info_primary.ci_kpm_pdir[l2tol2(i)];
#endif
					pmap_pte_set(cpu_pdep, pte);
				}
			}
#endif

			KASSERT(level != PTP_LEVELS || nkptp[level - 1] +
			    pl_i(VM_MIN_KERNEL_ADDRESS, level) == i);
			nkptp[level - 1]++;
		}
		pmap_pte_flush();
	}
#ifdef XENPV
	splx(s);
#endif
}

/*
 * pmap_growkernel: increase usage of KVM space.
 *
 * => we allocate new PTPs for the kernel and install them in all
 *    the pmaps on the system.
 */
vaddr_t
pmap_growkernel(vaddr_t maxkvaddr)
{
	struct pmap *kpm = pmap_kernel();
	struct pmap *cpm;
#if !defined(XENPV) || !defined(__x86_64__)
	struct pmap *pm;
	long old;
#endif
	int s, i;
	long needed_kptp[PTP_LEVELS], target_nptp;
	bool invalidate = false;

	s = splvm();	/* to be safe */
	mutex_enter(&kpm->pm_lock);

	if (maxkvaddr <= pmap_maxkvaddr) {
		mutex_exit(&kpm->pm_lock);
		splx(s);
		return pmap_maxkvaddr;
	}

	maxkvaddr = x86_round_pdr(maxkvaddr);
#if !defined(XENPV) || !defined(__x86_64__)
	old = nkptp[PTP_LEVELS - 1];
#endif

	/* Initialize needed_kptp. */
	for (i = PTP_LEVELS - 1; i >= 1; i--) {
		target_nptp = pl_i_roundup(maxkvaddr, i + 1) -
		    pl_i_roundup(VM_MIN_KERNEL_ADDRESS, i + 1);

		if (target_nptp > nkptpmax[i])
			panic("out of KVA space");
		KASSERT(target_nptp >= nkptp[i]);
		needed_kptp[i] = target_nptp - nkptp[i];
	}

#ifdef XENPV
	/* only pmap_kernel(), or the per-cpu map, has kernel entries */
	cpm = kpm;
#else
	/* Get the current pmap */
	if (__predict_true(cpu_info_primary.ci_flags & CPUF_PRESENT)) {
		cpm = curcpu()->ci_pmap;
	} else {
		cpm = kpm;
	}
#endif

	kasan_shadow_map((void *)pmap_maxkvaddr,
	    (size_t)(maxkvaddr - pmap_maxkvaddr));
	kmsan_shadow_map((void *)pmap_maxkvaddr,
	    (size_t)(maxkvaddr - pmap_maxkvaddr));

	pmap_alloc_level(cpm, pmap_maxkvaddr, needed_kptp);

	/*
	 * If the number of top level entries changed, update all pmaps.
	 */
	if (needed_kptp[PTP_LEVELS - 1] != 0) {
#ifdef XENPV
#ifdef __x86_64__
		/* nothing, kernel entries are never entered in user pmap */
#else
		int pdkidx;

		mutex_enter(&pmaps_lock);
		LIST_FOREACH(pm, &pmaps, pm_list) {
			for (pdkidx = PDIR_SLOT_KERN + old;
			    pdkidx < PDIR_SLOT_KERN + nkptp[PTP_LEVELS - 1];
			    pdkidx++) {
				pmap_pte_set(&pm->pm_pdir[pdkidx],
				    kpm->pm_pdir[pdkidx]);
			}
			pmap_pte_flush();
		}
		mutex_exit(&pmaps_lock);
#endif /* __x86_64__ */
#else /* XENPV */
		size_t newpdes;
		newpdes = nkptp[PTP_LEVELS - 1] - old;
		if (cpm != kpm) {
			memcpy(&kpm->pm_pdir[PDIR_SLOT_KERN + old],
			    &cpm->pm_pdir[PDIR_SLOT_KERN + old],
			    newpdes * sizeof(pd_entry_t));
		}

		mutex_enter(&pmaps_lock);
		LIST_FOREACH(pm, &pmaps, pm_list) {
			if (__predict_false(pm->pm_enter != NULL)) {
				/*
				 * Not a native pmap, the kernel is not mapped,
				 * so nothing to synchronize.
				 */
				continue;
			}
			memcpy(&pm->pm_pdir[PDIR_SLOT_KERN + old],
			    &kpm->pm_pdir[PDIR_SLOT_KERN + old],
			    newpdes * sizeof(pd_entry_t));
		}
		mutex_exit(&pmaps_lock);
#endif
		invalidate = true;
	}
	pmap_maxkvaddr = maxkvaddr;
	mutex_exit(&kpm->pm_lock);
	splx(s);

	if (invalidate && pmap_initialized) {
		/* Invalidate the pmap cache. */
		pool_cache_invalidate(&pmap_cache);
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
	int lvl;

	/*
	 * if end is out of range truncate.
	 * if (end == start) update to max.
	 */

	if (eva > VM_MAXUSER_ADDRESS || eva <= sva)
		eva = VM_MAXUSER_ADDRESS;

	mutex_enter(&pmap->pm_lock);
	pmap_map_ptes(pmap, &pmap2, &ptes, &pdes);

	/*
	 * dumping a range of pages: we dump in PTP sized blocks (4MB)
	 */

	for (/* null */ ; sva < eva ; sva = blkendva) {

		/* determine range of block */
		blkendva = x86_round_pdr(sva+1);
		if (blkendva > eva)
			blkendva = eva;

		/* valid block? */
		if (!pmap_pdes_valid(sva, pdes, NULL, &lvl))
			continue;
		KASSERT(lvl == 1);

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
	mutex_exit(&pmap->pm_lock);
}
#endif

/*
 * pmap_update: process deferred invalidations and frees.
 */
void
pmap_update(struct pmap *pmap)
{
	struct pmap_page *pp;
	struct vm_page *ptp;

	/*
	 * Initiate any pending TLB shootdowns.  Wait for them to
	 * complete before returning control to the caller.
	 */
	kpreempt_disable();
	pmap_tlb_shootnow();
	kpreempt_enable();

	/*
	 * Now that shootdowns are complete, process deferred frees.  This
	 * is an unlocked check, but is safe as we're only interested in
	 * work done in this LWP - we won't get a false negative.
	 */
	if (atomic_load_relaxed(&pmap->pm_gc_ptp.lh_first) == NULL) {
		return;
	}

	mutex_enter(&pmap->pm_lock);
	while ((ptp = LIST_FIRST(&pmap->pm_gc_ptp)) != NULL) {
		KASSERT(ptp->wire_count == 0);
		KASSERT(ptp->uanon == NULL);
		LIST_REMOVE(ptp, mdpage.mp_pp.pp_link);
		pp = VM_PAGE_TO_PP(ptp);
		LIST_INIT(&pp->pp_pvlist);
		pp->pp_attrs = 0;
		pp->pp_pte.pte_ptp = NULL;
		pp->pp_pte.pte_va = 0;
		PMAP_CHECK_PP(VM_PAGE_TO_PP(ptp));

		/*
		 * XXX Hack to avoid extra locking, and lock
		 * assertions in uvm_pagefree().  Despite uobject
		 * being set, this isn't a managed page.
		 */
		PMAP_DUMMY_LOCK(pmap);
		uvm_pagerealloc(ptp, NULL, 0);
		PMAP_DUMMY_UNLOCK(pmap);
		uvm_pagefree(ptp);
	}
	mutex_exit(&pmap->pm_lock);
}

#if PTP_LEVELS > 4
#error "Unsupported number of page table mappings"
#endif

paddr_t
pmap_init_tmp_pgtbl(paddr_t pg)
{
	static bool maps_loaded;
	static const paddr_t x86_tmp_pml_paddr[] = {
	    4 * PAGE_SIZE,	/* L1 */
	    5 * PAGE_SIZE,	/* L2 */
	    6 * PAGE_SIZE,	/* L3 */
	    7 * PAGE_SIZE	/* L4 */
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
		}
		pmap_update(pmap_kernel());
		maps_loaded = true;
	}

	/* Zero levels 1-3 */
	for (level = 0; level < PTP_LEVELS - 1; ++level) {
		tmp_pml = (void *)x86_tmp_pml_vaddr[level];
		memset(PAGE_ALIGNED(tmp_pml), 0, PAGE_SIZE);
	}

	/* Copy PML4 */
	kernel_pml = pmap_kernel()->pm_pdir;
	tmp_pml = (void *)x86_tmp_pml_vaddr[PTP_LEVELS - 1];
	memcpy(PAGE_ALIGNED(tmp_pml), PAGE_ALIGNED(kernel_pml), PAGE_SIZE);

#ifdef PAE
	/*
	 * Use the last 4 entries of the L2 page as L3 PD entries. These
	 * last entries are unlikely to be used for temporary mappings.
	 * 508: maps 0->1GB (userland)
	 * 509: unused
	 * 510: unused
	 * 511: maps 3->4GB (kernel)
	 */
	tmp_pml[508] = x86_tmp_pml_paddr[PTP_LEVELS - 1] | PTE_P;
	tmp_pml[509] = 0;
	tmp_pml[510] = 0;
	tmp_pml[511] = pmap_pdirpa(pmap_kernel(), PDIR_SLOT_KERN) | PTE_P;
#endif

	for (level = PTP_LEVELS - 1; level > 0; --level) {
		tmp_pml = (void *)x86_tmp_pml_vaddr[level];

		tmp_pml[pl_i(pg, level + 1)] =
		    (x86_tmp_pml_paddr[level - 1] & PTE_FRAME) | PTE_W | PTE_P;
	}

	tmp_pml = (void *)x86_tmp_pml_vaddr[0];
	tmp_pml[pl_i(pg, 1)] = (pg & PTE_FRAME) | PTE_W | PTE_P;

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

#if defined(__HAVE_DIRECT_MAP) && defined(__x86_64__) && !defined(XENPV)

/*
 * -----------------------------------------------------------------------------
 * *****************************************************************************
 * *****************************************************************************
 * *****************************************************************************
 * *****************************************************************************
 * **************** HERE BEGINS THE EPT CODE, USED BY INTEL-VMX ****************
 * *****************************************************************************
 * *****************************************************************************
 * *****************************************************************************
 * *****************************************************************************
 * -----------------------------------------------------------------------------
 *
 * These functions are invoked as callbacks from the code above. Contrary to
 * native, EPT does not have a recursive slot; therefore, it is not possible
 * to call pmap_map_ptes(). Instead, we use the direct map and walk down the
 * tree manually.
 *
 * Apart from that, the logic is mostly the same as native. Once a pmap has
 * been created, NVMM calls pmap_ept_transform() to make it an EPT pmap.
 * After that we're good, and the callbacks will handle the translations
 * for us.
 *
 * -----------------------------------------------------------------------------
 */

/* Hardware bits. */
#define EPT_R		__BIT(0)	/* read */
#define EPT_W		__BIT(1)	/* write */
#define EPT_X		__BIT(2)	/* execute */
#define EPT_T		__BITS(5,3)	/* type */
#define		TYPE_UC	0
#define		TYPE_WC	1
#define		TYPE_WT	4
#define		TYPE_WP	5
#define		TYPE_WB	6
#define EPT_NOPAT	__BIT(6)
#define EPT_L		__BIT(7)	/* large */
#define EPT_A		__BIT(8)	/* accessed */
#define EPT_D		__BIT(9)	/* dirty */
/* Software bits. */
#define EPT_PVLIST	__BIT(60)
#define EPT_WIRED	__BIT(61)

#define pmap_ept_valid_entry(pte)	(pte & EPT_R)

bool pmap_ept_has_ad __read_mostly;

static inline void
pmap_ept_stats_update_bypte(struct pmap *pmap, pt_entry_t npte, pt_entry_t opte)
{
	int resid_diff = ((npte & EPT_R) ? 1 : 0) - ((opte & EPT_R) ? 1 : 0);
	int wired_diff = ((npte & EPT_WIRED) ? 1 : 0) - ((opte & EPT_WIRED) ? 1 : 0);

	KASSERT((npte & (EPT_R | EPT_WIRED)) != EPT_WIRED);
	KASSERT((opte & (EPT_R | EPT_WIRED)) != EPT_WIRED);

	pmap_stats_update(pmap, resid_diff, wired_diff);
}

static pt_entry_t
pmap_ept_type(u_int flags)
{
	u_int cacheflags = (flags & PMAP_CACHE_MASK);
	pt_entry_t ret;

	switch (cacheflags) {
	case PMAP_NOCACHE:
	case PMAP_NOCACHE_OVR:
		ret = __SHIFTIN(TYPE_UC, EPT_T);
		break;
	case PMAP_WRITE_COMBINE:
		ret = __SHIFTIN(TYPE_WC, EPT_T);
		break;
	case PMAP_WRITE_BACK:
	default:
		ret = __SHIFTIN(TYPE_WB, EPT_T);
		break;
	}

	ret |= EPT_NOPAT;
	return ret;
}

static inline pt_entry_t
pmap_ept_prot(vm_prot_t prot)
{
	pt_entry_t res = 0;

	if (prot & VM_PROT_READ)
		res |= EPT_R;
	if (prot & VM_PROT_WRITE)
		res |= EPT_W;
	if (prot & VM_PROT_EXECUTE)
		res |= EPT_X;

	return res;
}

static inline uint8_t
pmap_ept_to_pp_attrs(pt_entry_t ept)
{
	uint8_t ret = 0;
	if (pmap_ept_has_ad) {
		if (ept & EPT_D)
			ret |= PP_ATTRS_D;
		if (ept & EPT_A)
			ret |= PP_ATTRS_A;
	} else {
		ret |= (PP_ATTRS_D|PP_ATTRS_A);
	}
	if (ept & EPT_W)
		ret |= PP_ATTRS_W;
	return ret;
}

static inline pt_entry_t
pmap_pp_attrs_to_ept(uint8_t attrs)
{
	pt_entry_t ept = 0;
	if (attrs & PP_ATTRS_D)
		ept |= EPT_D;
	if (attrs & PP_ATTRS_A)
		ept |= EPT_A;
	if (attrs & PP_ATTRS_W)
		ept |= EPT_W;
	return ept;
}

/*
 * Helper for pmap_ept_free_ptp.
 * tree[0] = &L2[L2idx]
 * tree[1] = &L3[L3idx]
 * tree[2] = &L4[L4idx]
 */
static void
pmap_ept_get_tree(struct pmap *pmap, vaddr_t va, pd_entry_t **tree)
{
	pt_entry_t *pteva;
	paddr_t ptepa;
	int i, index;

	ptepa = pmap->pm_pdirpa[0];
	for (i = PTP_LEVELS; i > 1; i--) {
		index = pl_pi(va, i);
		pteva = (pt_entry_t *)PMAP_DIRECT_MAP(ptepa);
		KASSERT(pmap_ept_valid_entry(pteva[index]));
		tree[i - 2] = &pteva[index];
		ptepa = pmap_pte2pa(pteva[index]);
	}
}

static void
pmap_ept_free_ptp(struct pmap *pmap, struct vm_page *ptp, vaddr_t va)
{
	pd_entry_t *tree[3];
	int level;

	KASSERT(pmap != pmap_kernel());
	KASSERT(mutex_owned(&pmap->pm_lock));
	KASSERT(kpreempt_disabled());

	pmap_ept_get_tree(pmap, va, tree);

	level = 1;
	do {
		(void)pmap_pte_testset(tree[level - 1], 0);

		pmap_freepage(pmap, ptp, level);
		if (level < PTP_LEVELS - 1) {
			ptp = pmap_find_ptp(pmap, va, level + 1);
			ptp->wire_count--;
			if (ptp->wire_count > 1)
				break;
		}
	} while (++level < PTP_LEVELS);
	pmap_pte_flush();
}

/* Allocate L4->L3->L2. Return L2. */
static void
pmap_ept_install_ptp(struct pmap *pmap, struct pmap_ptparray *pt, vaddr_t va)
{
	struct vm_page *ptp;
	unsigned long index;
	pd_entry_t *pteva;
	paddr_t ptepa;
	int i;

	KASSERT(pmap != pmap_kernel());
	KASSERT(mutex_owned(&pmap->pm_lock));
	KASSERT(kpreempt_disabled());

	/*
	 * Now that we have all the pages looked up or allocated,
	 * loop through again installing any new ones into the tree.
	 */
	ptepa = pmap->pm_pdirpa[0];
	for (i = PTP_LEVELS; i > 1; i--) {
		index = pl_pi(va, i);
		pteva = (pt_entry_t *)PMAP_DIRECT_MAP(ptepa);

		if (pmap_ept_valid_entry(pteva[index])) {
			KASSERT(!pt->alloced[i]);
			ptepa = pmap_pte2pa(pteva[index]);
			continue;
		}

		ptp = pt->pg[i];
		ptp->flags &= ~PG_BUSY; /* never busy */
		ptp->wire_count = 1;
		pmap->pm_ptphint[i - 2] = ptp;
		ptepa = VM_PAGE_TO_PHYS(ptp);
		pmap_pte_set(&pteva[index], ptepa | EPT_R | EPT_W | EPT_X);

		pmap_pte_flush();
		pmap_stats_update(pmap, 1, 0);

		/*
		 * If we're not in the top level, increase the
		 * wire count of the parent page.
		 */
		if (i < PTP_LEVELS) {
			pt->pg[i + 1]->wire_count++;
		}
	}
}

static int
pmap_ept_enter(struct pmap *pmap, vaddr_t va, paddr_t pa, vm_prot_t prot,
    u_int flags)
{
	pt_entry_t *ptes, opte, npte;
	pt_entry_t *ptep;
	struct vm_page *ptp;
	struct vm_page *new_pg, *old_pg;
	struct pmap_page *new_pp, *old_pp;
	struct pv_entry *old_pve, *new_pve;
	bool wired = (flags & PMAP_WIRED) != 0;
	bool accessed;
	struct pmap_ptparray pt;
	int error;
	bool getptp, samepage, new_embedded;
	rb_tree_t *tree;

	KASSERT(pmap_initialized);
	KASSERT(va < VM_MAXUSER_ADDRESS);

	npte = pa | pmap_ept_prot(prot) | pmap_ept_type(flags);

	if (wired)
		npte |= EPT_WIRED;
	if (flags & VM_PROT_ALL) {
		npte |= EPT_A;
		if (flags & VM_PROT_WRITE) {
			KASSERT((npte & EPT_W) != 0);
			npte |= EPT_D;
		}
	}

	new_pg = PHYS_TO_VM_PAGE(pa);
	if (new_pg != NULL) {
		/* This is a managed page */
		npte |= EPT_PVLIST;
		new_pp = VM_PAGE_TO_PP(new_pg);
	} else if ((new_pp = pmap_pv_tracked(pa)) != NULL) {
		/* This is an unmanaged pv-tracked page */
		npte |= EPT_PVLIST;
	} else {
		new_pp = NULL;
	}

	/* Begin by locking the pmap. */
	mutex_enter(&pmap->pm_lock);

	/* Look up the PTP.  Allocate if none present. */
	ptp = NULL;
	getptp = false;
	if (pmap != pmap_kernel()) {
		ptp = pmap_find_ptp(pmap, va, 1);
		if (ptp == NULL) {
			getptp = true;
			error = pmap_get_ptp(pmap, &pt, va, flags, &ptp);
			if (error != 0) {
				if (flags & PMAP_CANFAIL) {
					mutex_exit(&pmap->pm_lock);
					return error;
				}
				panic("%s: get ptp failed, error=%d", __func__,
				    error);
			}
		}
		tree = &VM_PAGE_TO_PP(ptp)->pp_rb;
	} else {
		/* Embedded PV entries rely on this. */
		KASSERT(va != 0);
		tree = &pmap_kernel_rb;
	}

	/*
	 * Look up the old PV entry at this VA (if any), and insert a new PV
	 * entry if required for the new mapping.  Temporarily track the old
	 * and new mappings concurrently.  Only after the old mapping is
	 * evicted from the pmap will we remove its PV entry.  Otherwise,
	 * our picture of modified/accessed state for either page could get
	 * out of sync (we need any P->V operation for either page to stall
	 * on pmap->pm_lock until done here).
	 */
	new_pve = NULL;
	old_pve = NULL;
	samepage = false;
	new_embedded = false;

	if (new_pp != NULL) {
		error = pmap_enter_pv(pmap, new_pp, ptp, va, &new_pve,
		    &old_pve, &samepage, &new_embedded, tree);

		/*
		 * If a new pv_entry was needed and none was available, we
		 * can go no further.
		 */
		if (error != 0) {
			if (flags & PMAP_CANFAIL) {
				if (getptp) {
					pmap_unget_ptp(pmap, &pt);
				}
				mutex_exit(&pmap->pm_lock);
				return error;
			}
			panic("%s: alloc pve failed", __func__);
		}
	} else {
		old_pve = pmap_treelookup_pv(pmap, ptp, tree, va);
	}

	/* Map PTEs into address space. */
	kpreempt_disable();

	/* Install any newly allocated PTPs. */
	if (getptp) {
		pmap_ept_install_ptp(pmap, &pt, va);
	}

	/* Check if there is an existing mapping. */
	ptes = (pt_entry_t *)PMAP_DIRECT_MAP(VM_PAGE_TO_PHYS(ptp));
	ptep = &ptes[pl1_pi(va)];
	opte = *ptep;
	bool have_oldpa = pmap_ept_valid_entry(opte);
	paddr_t oldpa = pmap_pte2pa(opte);

	/*
	 * Update the pte.
	 */
	do {
		opte = *ptep;

		/*
		 * if the same page, inherit PTE_A and PTE_D.
		 */
		if (((opte ^ npte) & (PTE_FRAME | EPT_R)) == 0) {
			npte |= opte & (EPT_A | EPT_D);
		}
	} while (pmap_pte_cas(ptep, opte, npte) != opte);

	/*
	 * Done with the PTEs: they can now be unmapped.
	 */
	kpreempt_enable();

	/*
	 * Update statistics and PTP's reference count.
	 */
	pmap_ept_stats_update_bypte(pmap, npte, opte);
	if (ptp != NULL) {
		if (!have_oldpa) {
			ptp->wire_count++;
		}
		/* Remember minimum VA in PTP. */
		pmap_ptp_range_set(ptp, va);
	}
	KASSERT(ptp == NULL || ptp->wire_count > 1);

	/*
	 * If the same page, we can skip pv_entry handling.
	 */
	if (((opte ^ npte) & (PTE_FRAME | EPT_R)) == 0) {
		KASSERT(((opte ^ npte) & EPT_PVLIST) == 0);
		if ((npte & EPT_PVLIST) != 0) {
			KASSERT(samepage);
			pmap_check_pv(pmap, ptp, new_pp, va, true);
		}
		goto same_pa;
	} else if ((npte & EPT_PVLIST) != 0) {
		KASSERT(!samepage);
	}

	/*
	 * If old page is pv-tracked, remove pv_entry from its list.
	 */
	if ((~opte & (EPT_R | EPT_PVLIST)) == 0) {
		if ((old_pg = PHYS_TO_VM_PAGE(oldpa)) != NULL) {
			old_pp = VM_PAGE_TO_PP(old_pg);
		} else if ((old_pp = pmap_pv_tracked(oldpa)) == NULL) {
			panic("%s: EPT_PVLIST with pv-untracked page"
			    " va = %#"PRIxVADDR
			    " pa = %#" PRIxPADDR " (%#" PRIxPADDR ")",
			    __func__, va, oldpa, atop(pa));
		}

		pmap_remove_pv(pmap, old_pp, ptp, va, old_pve,
		    pmap_ept_to_pp_attrs(opte));
	} else {
		KASSERT(old_pve == NULL);
		KASSERT(pmap_treelookup_pv(pmap, ptp, tree, va) == NULL);
	}

	/*
	 * If new page is dynamically PV tracked, insert to tree.
	 */
	if (new_pve != NULL) {
		KASSERT(pmap_treelookup_pv(pmap, ptp, tree, va) == NULL);
		old_pve = rb_tree_insert_node(tree, new_pve);
		KASSERT(old_pve == new_pve);
		pmap_check_pv(pmap, ptp, new_pp, va, true);
	}

same_pa:
	/*
	 * shootdown tlb if necessary.
	 */

	if (pmap_ept_has_ad) {
		accessed = (~opte & (EPT_R | EPT_A)) == 0;
	} else {
		accessed = (opte & EPT_R) != 0;
	}
	if (accessed && ((opte ^ npte) & (PTE_FRAME | EPT_W)) != 0) {
		pmap_tlb_shootdown(pmap, va, 0, TLBSHOOT_ENTER);
	}
	pmap_drain_pv(pmap);
	mutex_exit(&pmap->pm_lock);
	return 0;
}

/* Pay close attention, this returns L2. */
static int
pmap_ept_pdes_invalid(struct pmap *pmap, vaddr_t va, pd_entry_t *lastpde)
{
	pt_entry_t *pteva;
	paddr_t ptepa;
	int i, index;

	KASSERT(mutex_owned(&pmap->pm_lock));

	ptepa = pmap->pm_pdirpa[0];
	for (i = PTP_LEVELS; i > 1; i--) {
		pteva = (pt_entry_t *)PMAP_DIRECT_MAP(ptepa);
		index = pl_pi(va, i);
		if (!pmap_ept_valid_entry(pteva[index]))
			return i;
		ptepa = pmap_pte2pa(pteva[index]);
	}
	if (lastpde != NULL) {
		*lastpde = pteva[index];
	}

	return 0;
}

static bool
pmap_ept_extract(struct pmap *pmap, vaddr_t va, paddr_t *pap)
{
	pt_entry_t *ptes, pte;
	pd_entry_t pde;
	paddr_t ptppa, pa;
	bool rv;

#ifdef __HAVE_DIRECT_MAP
	if (va >= PMAP_DIRECT_BASE && va < PMAP_DIRECT_END) {
		if (pap != NULL) {
			*pap = PMAP_DIRECT_UNMAP(va);
		}
		return true;
	}
#endif

	rv = false;
	pa = 0;

	mutex_enter(&pmap->pm_lock);
	kpreempt_disable();

	if (!pmap_ept_pdes_invalid(pmap, va, &pde)) {
		ptppa = pmap_pte2pa(pde);
		ptes = (pt_entry_t *)PMAP_DIRECT_MAP(ptppa);
		pte = ptes[pl1_pi(va)];
		if (__predict_true((pte & EPT_R) != 0)) {
			pa = pmap_pte2pa(pte) | (va & (NBPD_L1 - 1));
			rv = true;
		}
	}

	kpreempt_enable();
	mutex_exit(&pmap->pm_lock);

	if (pap != NULL) {
		*pap = pa;
	}
	return rv;
}

static bool
pmap_ept_remove_pte(struct pmap *pmap, struct vm_page *ptp, pt_entry_t *pte,
    vaddr_t va)
{
	struct pv_entry *pve;
	struct vm_page *pg;
	struct pmap_page *pp;
	pt_entry_t opte;
	bool accessed;

	KASSERT(pmap != pmap_kernel());
	KASSERT(mutex_owned(&pmap->pm_lock));
	KASSERT(kpreempt_disabled());

	if (!pmap_ept_valid_entry(*pte)) {
		/* VA not mapped. */
		return false;
	}

	/* Atomically save the old PTE and zap it. */
	opte = pmap_pte_testset(pte, 0);
	if (!pmap_ept_valid_entry(opte)) {
		return false;
	}

	pmap_ept_stats_update_bypte(pmap, 0, opte);

	if (ptp) {
		/*
		 * Dropping a PTE.  Make sure that the PDE is flushed.
		 */
		ptp->wire_count--;
		if (ptp->wire_count <= 1) {
			opte |= EPT_A;
		}
	}

	if (pmap_ept_has_ad) {
		accessed = (opte & EPT_A) != 0;
	} else {
		accessed = true;
	}
	if (accessed) {
		pmap_tlb_shootdown(pmap, va, 0, TLBSHOOT_REMOVE_PTE);
	}

	/*
	 * If we are not on a pv list - we are done.
	 */
	if ((opte & EPT_PVLIST) == 0) {
		KASSERTMSG((PHYS_TO_VM_PAGE(pmap_pte2pa(opte)) == NULL),
		    "managed page without EPT_PVLIST for %#"PRIxVADDR, va);
		KASSERTMSG((pmap_pv_tracked(pmap_pte2pa(opte)) == NULL),
		    "pv-tracked page without EPT_PVLIST for %#"PRIxVADDR, va);
		KASSERT(pmap_treelookup_pv(pmap, ptp, (ptp != NULL ?
		    &VM_PAGE_TO_PP(ptp)->pp_rb : &pmap_kernel_rb), va) == NULL);
		return true;
	}

	if ((pg = PHYS_TO_VM_PAGE(pmap_pte2pa(opte))) != NULL) {
		pp = VM_PAGE_TO_PP(pg);
	} else if ((pp = pmap_pv_tracked(pmap_pte2pa(opte))) == NULL) {
		paddr_t pa = pmap_pte2pa(opte);
		panic("%s: EPT_PVLIST with pv-untracked page"
		    " va = %#"PRIxVADDR"pa = %#"PRIxPADDR" (%#"PRIxPADDR")",
		    __func__, va, pa, atop(pa));
	}

	/* Sync R/M bits. */
	pve = pmap_lookup_pv(pmap, ptp, pp, va);
	pmap_remove_pv(pmap, pp, ptp, va, pve, pmap_ept_to_pp_attrs(opte));
	return true;
}

static void
pmap_ept_remove_ptes(struct pmap *pmap, struct vm_page *ptp, vaddr_t ptpva,
    vaddr_t startva, vaddr_t endva)
{
	pt_entry_t *pte = (pt_entry_t *)ptpva;

	KASSERT(pmap != pmap_kernel());
	KASSERT(mutex_owned(&pmap->pm_lock));
	KASSERT(kpreempt_disabled());

	/*
	 * mappings are very often sparse, so clip the given range to the
	 * range of PTEs that are known present in the PTP.
	 */
	pmap_ptp_range_clip(ptp, &startva, &pte);

	/*
	 * note that ptpva points to the PTE that maps startva.   this may
	 * or may not be the first PTE in the PTP.
	 *
	 * we loop through the PTP while there are still PTEs to look at
	 * and the wire_count is greater than 1 (because we use the wire_count
	 * to keep track of the number of real PTEs in the PTP).
	 */
	while (startva < endva && (ptp == NULL || ptp->wire_count > 1)) {
		(void)pmap_ept_remove_pte(pmap, ptp, pte, startva);
		startva += PAGE_SIZE;
		pte++;
	}
}

static void
pmap_ept_remove(struct pmap *pmap, vaddr_t sva, vaddr_t eva)
{
	pt_entry_t *ptes;
	pd_entry_t pde;
	paddr_t ptppa;
	vaddr_t blkendva, va = sva;
	struct vm_page *ptp;

	mutex_enter(&pmap->pm_lock);
	kpreempt_disable();

	for (/* null */ ; va < eva ; va = blkendva) {
		int lvl;

		/* determine range of block */
		blkendva = x86_round_pdr(va+1);
		if (blkendva > eva)
			blkendva = eva;

		lvl = pmap_ept_pdes_invalid(pmap, va, &pde);
		if (lvl != 0) {
			/* Skip a range corresponding to an invalid pde. */
			blkendva = (va & ptp_frames[lvl - 1]) + nbpd[lvl - 1];
			continue;
		}

		/* PA of the PTP */
		ptppa = pmap_pte2pa(pde);

		ptp = pmap_find_ptp(pmap, va, 1);
		KASSERTMSG(ptp != NULL, "%s: unmanaged PTP detected",
		    __func__);

		ptes = (pt_entry_t *)PMAP_DIRECT_MAP(ptppa);

		pmap_ept_remove_ptes(pmap, ptp, (vaddr_t)&ptes[pl1_pi(va)], va,
		    blkendva);

		/* If PTP is no longer being used, free it. */
		if (ptp && ptp->wire_count <= 1) {
			pmap_ept_free_ptp(pmap, ptp, va);
		}
	}

	kpreempt_enable();
	pmap_drain_pv(pmap);
	mutex_exit(&pmap->pm_lock);
}

static int
pmap_ept_sync_pv(struct vm_page *ptp, vaddr_t va, paddr_t pa, int clearbits,
    uint8_t *oattrs, pt_entry_t *optep)
{
	struct pmap *pmap;
	pt_entry_t *ptep;
	pt_entry_t opte;
	pt_entry_t npte;
	pt_entry_t expect;
	bool need_shootdown;

	expect = pmap_pa2pte(pa) | EPT_R;
	pmap = ptp_to_pmap(ptp);

	if (clearbits != ~0) {
		KASSERT((clearbits & ~(PP_ATTRS_D|PP_ATTRS_A|PP_ATTRS_W)) == 0);
		clearbits = pmap_pp_attrs_to_ept(clearbits);
	}

	ptep = pmap_map_pte(pmap, ptp, va);
	do {
		opte = *ptep;
		KASSERT((opte & (EPT_D | EPT_A)) != EPT_D);
		KASSERT((opte & (EPT_A | EPT_R)) != EPT_A);
		KASSERT(opte == 0 || (opte & EPT_R) != 0);
		if ((opte & (PTE_FRAME | EPT_R)) != expect) {
			/*
			 * We lost a race with a V->P operation like
			 * pmap_remove().  Wait for the competitor
			 * reflecting pte bits into mp_attrs.
			 */
			pmap_unmap_pte();
			return EAGAIN;
		}

		/*
		 * Check if there's anything to do on this PTE.
		 */
		if ((opte & clearbits) == 0) {
			need_shootdown = false;
			break;
		}

		/*
		 * We need a shootdown if the PTE is cached (EPT_A) ...
		 * ... Unless we are clearing only the EPT_W bit and
		 * it isn't cached as RW (EPT_D).
		 */
		if (pmap_ept_has_ad) {
			need_shootdown = (opte & EPT_A) != 0 &&
			    !(clearbits == EPT_W && (opte & EPT_D) == 0);
		} else {
			need_shootdown = true;
		}

		npte = opte & ~clearbits;

		/*
		 * If we need a shootdown anyway, clear EPT_A and EPT_D.
		 */
		if (need_shootdown) {
			npte &= ~(EPT_A | EPT_D);
		}
		KASSERT((npte & (EPT_D | EPT_A)) != EPT_D);
		KASSERT((npte & (EPT_A | EPT_R)) != EPT_A);
		KASSERT(npte == 0 || (opte & EPT_R) != 0);
	} while (pmap_pte_cas(ptep, opte, npte) != opte);

	if (need_shootdown) {
		pmap_tlb_shootdown(pmap, va, 0, TLBSHOOT_SYNC_PV);
	}
	pmap_unmap_pte();

	*oattrs = pmap_ept_to_pp_attrs(opte);
	if (optep != NULL)
		*optep = opte;
	return 0;
}

static void
pmap_ept_pp_remove_ent(struct pmap *pmap, struct vm_page *ptp, pt_entry_t opte,
    vaddr_t va)
{

	KASSERT(mutex_owned(&pmap->pm_lock));

	pmap_ept_stats_update_bypte(pmap, 0, opte);
	ptp->wire_count--;
	if (ptp->wire_count <= 1) {
		pmap_ept_free_ptp(pmap, ptp, va);
	}
}

static void
pmap_ept_write_protect(struct pmap *pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
	pt_entry_t bit_rem;
	pt_entry_t *ptes, *spte;
	pt_entry_t opte, npte;
	pd_entry_t pde;
	paddr_t ptppa;
	vaddr_t va;
	bool modified;

	bit_rem = 0;
	if (!(prot & VM_PROT_WRITE))
		bit_rem = EPT_W;

	sva &= PTE_FRAME;
	eva &= PTE_FRAME;

	/* Acquire pmap. */
	mutex_enter(&pmap->pm_lock);
	kpreempt_disable();

	for (va = sva; va < eva; va += PAGE_SIZE) {
		if (pmap_ept_pdes_invalid(pmap, va, &pde)) {
			continue;
		}

		ptppa = pmap_pte2pa(pde);
		ptes = (pt_entry_t *)PMAP_DIRECT_MAP(ptppa);
		spte = &ptes[pl1_pi(va)];

		do {
			opte = *spte;
			if (!pmap_ept_valid_entry(opte)) {
				goto next;
			}
			npte = (opte & ~bit_rem);
		} while (pmap_pte_cas(spte, opte, npte) != opte);

		if (pmap_ept_has_ad) {
			modified = (opte & EPT_D) != 0;
		} else {
			modified = true;
		}
		if (modified) {
			vaddr_t tva = x86_ptob(spte - ptes);
			pmap_tlb_shootdown(pmap, tva, 0,
			    TLBSHOOT_WRITE_PROTECT);
		}
next:;
	}

	kpreempt_enable();
	mutex_exit(&pmap->pm_lock);
}

static void
pmap_ept_unwire(struct pmap *pmap, vaddr_t va)
{
	pt_entry_t *ptes, *ptep, opte;
	pd_entry_t pde;
	paddr_t ptppa;

	/* Acquire pmap. */
	mutex_enter(&pmap->pm_lock);
	kpreempt_disable();

	if (pmap_ept_pdes_invalid(pmap, va, &pde)) {
		panic("%s: invalid PDE va=%#" PRIxVADDR, __func__, va);
	}

	ptppa = pmap_pte2pa(pde);
	ptes = (pt_entry_t *)PMAP_DIRECT_MAP(ptppa);
	ptep = &ptes[pl1_pi(va)];
	opte = *ptep;
	KASSERT(pmap_ept_valid_entry(opte));

	if (opte & EPT_WIRED) {
		pt_entry_t npte = opte & ~EPT_WIRED;

		opte = pmap_pte_testset(ptep, npte);
		pmap_ept_stats_update_bypte(pmap, npte, opte);
	} else {
		printf("%s: wiring for pmap %p va %#" PRIxVADDR
		    "did not change!\n", __func__, pmap, va);
	}

	/* Release pmap. */
	kpreempt_enable();
	mutex_exit(&pmap->pm_lock);
}

/* -------------------------------------------------------------------------- */

void
pmap_ept_transform(struct pmap *pmap)
{
	pmap->pm_enter = pmap_ept_enter;
	pmap->pm_extract = pmap_ept_extract;
	pmap->pm_remove = pmap_ept_remove;
	pmap->pm_sync_pv = pmap_ept_sync_pv;
	pmap->pm_pp_remove_ent = pmap_ept_pp_remove_ent;
	pmap->pm_write_protect = pmap_ept_write_protect;
	pmap->pm_unwire = pmap_ept_unwire;

	memset(PAGE_ALIGNED(pmap->pm_pdir), 0, PAGE_SIZE);
}

#endif /* __HAVE_DIRECT_MAP && __x86_64__ && !XENPV */
