/*	$NetBSD: pmap.c,v 1.8 2001/06/15 20:43:01 matt Exp $	*/
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
#include <sys/user.h>
#include <sys/pool.h>
#include <sys/queue.h>
#include <sys/systm.h>

#if __NetBSD_Version__ < 105010000
#include <vm/vm.h>
#include <vm/vm_kern.h>
#define	splvm()		splimp()
#endif

#include <uvm/uvm.h>

#include <machine/pcb.h>
#include <machine/powerpc.h>
#if __NetBSD_Version__ > 105010000
#include <powerpc/mpc6xx/bat.h>
#else
#include <powerpc/bat.h>
#endif

/*#define PMAPCHECK*/

#if defined(DEBUG) || defined(PMAPCHECK)
#define	STATIC
#else
#define	STATIC	static
#endif

struct pteg {
	pte_t pt[8];
};
typedef struct pteg pteg_t;

volatile pteg_t *pmap_pteg_table;
unsigned int pmap_pteg_cnt;
unsigned int pmap_pteg_mask;

struct pmap kernel_pmap_;
unsigned int pmap_pages_stolen;
u_long pmap_pte_valid;
u_long pmap_pte_overflow;
u_long pmap_pte_replacements;
u_long pmap_pvo_entries;
u_long pmap_pvo_enter_calls;
u_long pmap_pvo_remove_calls;
u_int64_t pmap_pte_spills = 0;
struct pvo_entry *pmap_pvo_syncicache;
struct pvo_entry *pmap_pvo_zeropage;
struct pvo_entry *pmap_pvo_copypage_src;
struct pvo_entry *pmap_pvo_copypage_dst;

vaddr_t pmap_rkva_start = VM_MIN_KERNEL_ADDRESS;
unsigned int pmap_rkva_count = 4;

int physmem;
#ifndef MSGBUFADDR
extern paddr_t msgbuf_paddr;
#endif

static struct mem_region *mem, *avail;

#ifdef __HAVE_PMAP_PHYSSEG
/*
 * This is a cache of referenced/modified bits.
 * Bits herein are shifted by ATTRSHFT.
 */
#define	ATTR_SHFT	4
struct pmap_physseg pmap_physseg;
#endif

/*
 * The following structure is exactly 32 bytes long (one cacheline).
 */
struct pvo_entry {
	LIST_ENTRY(pvo_entry) pvo_vlink;	/* Link to common virt page */
	LIST_ENTRY(pvo_entry) pvo_olink;	/* Link to overflow entry */
	struct pte pvo_pte;			/* Prebuilt PTE */
	pmap_t pvo_pmap;			/* ptr to owning pmap */
	vaddr_t pvo_vaddr;			/* VA of entry */
#define	PVO_PTEGIDX_MASK	0x0007		/* which PTEG slot */
#define	PVO_WIRED		0x0010		/* PVO entry is wired */
#define	PVO_MANAGED		0x0020		/* PVO entyy for managed page */
};
#define	PVO_VADDR(pvo)		((pvo)->pvo_vaddr & ~ADDR_POFF)
#define	PVO_PTEGIDX_GET(pvo)	((pvo)->pvo_vaddr & PVO_PTEGIDX_MASK)
#define	PVO_PTEGIDX_CLR(pvo)	((void)((pvo)->pvo_vaddr &= ~PVO_PTEGIDX_MASK))
#define	PVO_PTEGIDX_SET(pvo,i)	((void)((pvo)->pvo_vaddr |= (i)))

struct pvo_head *pmap_pvo_table;	/* pvo entries by ptegroup index */
struct pvo_head pmap_pvo_kunmanaged = LIST_HEAD_INITIALIZER(pmap_pvo_kunmanaged);	/* list of unmanaged pages */
struct pvo_head pmap_pvo_unmanaged = LIST_HEAD_INITIALIZER(pmap_pvo_unmanaged);	/* list of unmanaged pages */

struct pool pmap_pool;		/* pool for pmap structures */
struct pool pmap_upvo_pool;	/* pool for pvo entries for unmanaged pages */
struct pool pmap_mpvo_pool;	/* pool for pvo entries for managed pages */

/*
 * We keep a cache of unmanaged pages to be used for pvo entries for
 * unmanaged pages.
 */
struct pvo_page {
	SIMPLEQ_ENTRY(pvo_page) pvop_link;
};
SIMPLEQ_HEAD(pvop_head, pvo_page);
struct pvop_head pmap_upvop_head = SIMPLEQ_HEAD_INITIALIZER(pmap_upvop_head);
struct pvop_head pmap_mpvop_head = SIMPLEQ_HEAD_INITIALIZER(pmap_mpvop_head);
u_long pmap_upvop_free;
u_long pmap_upvop_maxfree;
u_long pmap_mpvop_free;
u_long pmap_mpvop_maxfree;

STATIC void *pmap_pool_ualloc(unsigned long, int, int);
STATIC void *pmap_pool_malloc(unsigned long, int, int);
STATIC void pmap_pool_ufree(void *, unsigned long, int);
STATIC void pmap_pool_mfree(void *, unsigned long, int);

#if defined(DEBUG) || defined(PMAPCHECK)
#ifdef PMAPCHECK
int pmapcheck = 1;
#else
int pmapcheck = 0;
#endif
void pmap_pvo_verify(void);
void pmap_pte_print(volatile pte_t *pt);
void pmap_pteg_check(void);
void pmap_pteg_dist(void);
void pmap_print_pte(pmap_t, vaddr_t);
void pmap_print_mmuregs(void);
STATIC void pmap_pvo_check(const struct pvo_entry *);
#define	PMAP_PVO_CHECK(pvo)	 		\
	do {					\
		if (pmapcheck)			\
			pmap_pvo_check(pvo);	\
	} while (0)
#else
#define	PMAP_PVO_CHECK(pvo)	do { } while (/*CONSTCOND*/0)
#endif
STATIC int pmap_pvo_enter(pmap_t, struct pool *, struct pvo_head *,
	vaddr_t, paddr_t, u_int, int);
STATIC void pmap_pvo_remove(struct pvo_entry *, int, int);
STATIC struct pvo_entry *pmap_pvo_find_va(pmap_t, vaddr_t, int *); 
STATIC volatile pte_t *pmap_pvo_to_pte(const struct pvo_entry *, int);

STATIC struct pvo_entry *pmap_rkva_alloc(int);
STATIC void pmap_pa_map(struct pvo_entry *, paddr_t);
STATIC void pmap_pa_unmap(struct pvo_entry *);
STATIC void tlbia(void);

STATIC void pmap_syncicache(paddr_t);
STATIC void pmap_release (pmap_t);

#define	VSID_NBPW	(sizeof(uint32_t) * 8)
static uint32_t pmap_vsid_bitmap[NPMAPS / VSID_NBPW];

static int pmap_initialized;

#if defined(DEBUG)
unsigned int pmapdebug = 0;
# define DPRINTF(x)		printf x
# define DPRINTFN(n, x)		if (pmapdebug >= (n)) printf x
#else
# define DPRINTF(x)
# define DPRINTFN(n, x)
#endif

#define	TLBIE(va)	__asm __volatile("tlbie %0" :: "r"(va))
#define	TLBSYNC()	__asm __volatile("tlbsync")
#define	SYNC()		__asm __volatile("sync")
#define	EIEIO()		__asm __volatile("eieio")
#define	MFTB()		mftb()

static __inline u_int
mftb(void)
{
	u_int tb;
	__asm __volatile("mftb %0" : "=r"(tb) : );
	return tb;
}
/*
 * These small routines may have to be replaced,
 * if/when we support processors other that the 604.
 */

void
tlbia(void)
{
	caddr_t i;
	
	SYNC();
	/*
	 * Why not use "tlbia"?  Because not all processors implement it.
	 *
	 * This needs to be a per-cpu callback to do the appropriate thing
	 * for the CPU. XXX
	 */
	for (i = 0; i < (caddr_t)0x00040000; i += 0x00001000) {
		TLBIE(i);
		EIEIO();
	}
	TLBSYNC();
	SYNC();
}

static __inline int
va_to_sr(sr_t *sr, vaddr_t va)
{
	return sr[(uintptr_t)va >> ADDR_SR_SHFT];
}

static __inline int
va_to_pteg(sr_t sr, vaddr_t addr)
{
	int hash;
	
	hash = (sr & SR_VSID) ^ (((u_int)addr & ADDR_PIDX) >> ADDR_PIDX_SHFT);
	return hash & pmap_pteg_mask;
}

#if defined(DEBUG) || defined(PMAPCHECK)
/*
 * Given a PTE in the page table, calculate the VADDR that hashes to it.
 * The only bit of magic is that the top 4 bits of the address doesn't
 * technically exist in the PTE.  But we know we reserved 4 bits of the
 * VSID for it so that's how we get it.
 */
static vaddr_t
pmap_pte_to_va(volatile const pte_t *pt)
{
	vaddr_t va;
	uintptr_t ptaddr = (uintptr_t) pt;

	if (pt->pte_hi & PTE_HID)
		ptaddr ^= (pmap_pteg_mask << 6);

	/* PPC Bits 10-19 */
	va = ((pt->pte_hi >> PTE_VSID_SHFT) ^ (ptaddr >> 6)) & 0x3ff;
	va <<= ADDR_PIDX_SHFT;

	/* PPC Bits 4-9 */
	va |= (pt->pte_hi & PTE_API) << ADDR_API_SHFT;

	/* PPC Bits 0-3 */
	va |= VSID_TO_SR(pt->pte_hi >> PTE_VSID_SHFT) << ADDR_SR_SHFT;

	return va;
}
#endif

static __inline struct pvo_head *
pa_to_pvoh(paddr_t pa)
{
#ifdef __HAVE_VM_PAGE_MD
	struct vm_page *pg;

	pg = PHYS_TO_VM_PAGE(pa);
	if (pg == NULL)
		return &pmap_pvo_unmanaged;
	return &pg->mdpage.mdpg_pvoh;
#endif
#ifdef __HAVE_PMAP_PHYSSEG
	int bank, pg;

	bank = vm_physseg_find(atop(pa), &pg);
	if (bank == -1)
		return &pmap_pvo_unmanaged;
	return &vm_physmem[bank].pmseg.pvoh[pg];
#endif
}

static __inline struct pvo_head *
vm_page_to_pvoh(struct vm_page *pg)
{
#ifdef __HAVE_VM_PAGE_MD
	return &pg->mdpage.mdpg_pvoh;
#endif
#ifdef __HAVE_PMAP_PHYSSEG
	return pa_to_pvoh(VM_PAGE_TO_PHYS(pg));
#endif
}


#ifdef __HAVE_PMAP_PHYSSEG
static __inline char *
pa_to_attr(paddr_t pa)
{
	int bank, pg;

	bank = vm_physseg_find(atop(pa), &pg);
	if (bank == -1)
		return NULL;
	return &vm_physmem[bank].pmseg.attrs[pg];
}
#endif

static __inline void
pmap_attr_clear(struct vm_page *pg, int ptebit)
{
#ifdef __HAVE_PMAP_PHYSSEG
	*pa_to_attr(VM_PAGE_TO_PHYS(pg)) &= ~(ptebit >> ATTR_SHFT);
#endif
#ifdef __HAVE_VM_PAGE_MD
	pg->mdpage.mdpg_attrs &= ~ptebit;
#endif
}

static __inline int
pmap_attr_fetch(struct vm_page *pg)
{
#ifdef __HAVE_PMAP_PHYSSEG
	return *pa_to_attr(VM_PAGE_TO_PHYS(pg)) << ATTR_SHFT;
#endif
#ifdef __HAVE_VM_PAGE_MD
	return pg->mdpage.mdpg_attrs;
#endif
}

static __inline void
pmap_attr_save(struct vm_page *pg, int ptebit)
{
#ifdef __HAVE_PMAP_PHYSSEG
	*pa_to_attr(VM_PAGE_TO_PHYS(pg)) |= (ptebit >> ATTR_SHFT);
#endif
#ifdef __HAVE_VM_PAGE_MD
	pg->mdpage.mdpg_attrs |= ptebit;
#endif
}

static __inline int
pmap_pte_compare(const volatile pte_t *pt, const pte_t *pvo_pt)
{
	if (pt->pte_hi == pvo_pt->pte_hi
#if 0
	    && ((pt->pte_lo ^ pvo_pt->pte_lo) &
	        ~(PTE_REF|PTE_CHG)) == 0
#endif
	    )
		return 1;
	return 0;
}

static __inline int
pmap_pte_match(volatile pte_t *pt, sr_t sr, vaddr_t va, int which)
{
	return (pt->pte_hi & ~PTE_VALID)
		== (  ((sr & SR_VSID) << PTE_VSID_SHFT)
		    | ((va >> ADDR_API_SHFT) & PTE_API)
		    | which);
}

static __inline void
pmap_pte_create(pte_t *pt, sr_t sr, vaddr_t va, u_int pte_lo)
{
	/*
	 * Construct the PTE.  Default to IMB initially.  Valid bit
	 * only gets set when the real pte is set in memory.
	 *
	 * Note: Don't set the valid bit for correct operation of tlb update.
	 */
	pt->pte_hi = ((sr & SR_VSID) << PTE_VSID_SHFT)
		| (((va & ADDR_PIDX) >> ADDR_API_SHFT) & PTE_API);
	pt->pte_lo = pte_lo;
}

static __inline void
pmap_pte_synch(volatile pte_t *pt, pte_t *pvo_pt)
{
	pvo_pt->pte_lo |= pt->pte_lo & (PTE_REF|PTE_CHG);
}

static __inline void
pmap_pte_clear(volatile pte_t *pt, int ptebit)
{
	/*
	 * As shown in Section 7.6.3.2.2
	 */
	pt->pte_lo &= ~ptebit;
	TLBIE(pt);
	EIEIO();
	TLBSYNC();
	SYNC();
}

static __inline void
pmap_pte_set(volatile pte_t *pt, pte_t *pvo_pt)
{
	if (pvo_pt->pte_hi & PTE_VALID)
		panic("pte_set: setting an already valid pte %p", pvo_pt);
	pvo_pt->pte_hi |= PTE_VALID;
	/*
	 * Update the PTE as defined in section 7.6.3.1
	 * Note that the REF/CHG bits are from pvo_pt and thus should
	 * have been saved so this routine can restore them (if desired).
	 */
	pt->pte_lo = pvo_pt->pte_lo;
	EIEIO();
	pt->pte_hi = pvo_pt->pte_hi;
	SYNC();
	pmap_pte_valid++;
}

static __inline void
pmap_pte_unset(volatile pte_t *pt, pte_t *pvo_pt, vaddr_t va)
{
	if ((pvo_pt->pte_hi & PTE_VALID) == 0)
		panic("pte_unset: attempt to unset an inactive pte#1 %p/%p", pvo_pt, pt);
	if ((pt->pte_hi & PTE_VALID) == 0)
		panic("pte_unset: attempt to unset an inactive pte#2 %p/%p", pvo_pt, pt);

	pvo_pt->pte_hi &= ~PTE_VALID;
	/*
	 * Force the ref & chg bits back into the PTEs.
	 */
	SYNC();
	/*
	 * Invalidate the pte ... (Section 7.6.3.3)
	 */
	pt->pte_hi &= ~PTE_VALID;
	SYNC();
	TLBIE(va);
	EIEIO();
	TLBSYNC();
	SYNC();
	/*
	 * Save the ref & chg bits ...
	 */
	pmap_pte_synch(pt, pvo_pt);
	pmap_pte_valid--;
}

static __inline void
pmap_pte_change(volatile pte_t *pt, pte_t *pvo_pt, vaddr_t va)
{
	/*
	 * Invalidate the PTE
	 */
	pmap_pte_unset(pt, pvo_pt, va);
	pmap_pte_set(pt, pvo_pt);
}

/*
 * Try to insert page table entry *pt into the pmap_pteg_table at idx.
 *
 * Note: *pt mustn't have PTE_VALID set.
 * This is done here as required by Book III, 4.12.
 */
static int
pmap_pte_insert(int ptegidx, pte_t *pvo_pt)
{
	volatile pte_t *pt;
	int i;
	
#if defined(DEBUG)
	DPRINTFN(7, ("pmap_pte_insert: idx 0x%x, pte 0x%x 0x%x\n",
		ptegidx, pvo_pt->pte_hi, pvo_pt->pte_lo));
#endif
	/*
	 * First try primary hash.
	 */
	for (pt = pmap_pteg_table[ptegidx].pt, i = 0; i < 8; i++, pt++) {
		if ((pt->pte_hi & PTE_VALID) == 0) {
			pvo_pt->pte_hi &= ~PTE_HID;
			pmap_pte_set(pt, pvo_pt);
			return i;
		}
	}
	/*
	 * Now try secondary hash.
	 */
	ptegidx ^= pmap_pteg_mask;
	for (pt = pmap_pteg_table[ptegidx].pt, i = 0; i < 8; i++, pt++) {
		if ((pt->pte_hi & PTE_VALID) == 0) {
			pvo_pt->pte_hi |= PTE_HID;
			pmap_pte_set(pt, pvo_pt);
			return i;
		}
	}
	return -1;
}

/*
 * Spill handler.
 *
 * Tries to spill a page table entry from the overflow area.
 * This runs in either real mode (if dealing with a exception spill)
 * or virtual mode when dealing with manually spilling one of the
 * kernel's pte entries.
 */
int
pmap_pte_spill(vaddr_t addr)
{
	struct pvo_entry *source_pvo, *victim_pvo;
	struct pvo_entry *pvo;
	int ptegidx, i;
	sr_t sr;
	volatile pteg_t *pteg;
	volatile pte_t *pt;

	pmap_pte_spills++;

	__asm __volatile ("mfsrin %0,%1" : "=r"(sr) : "r"(addr));
	ptegidx = va_to_pteg(sr, addr);

	/*
	 * Have to substitute some entry. Use the primary hash for this.
	 *
	 * Use low bits of timebase as random generator
	 */
	__asm __volatile ("mftb %0" : "=r"(i));
	pteg = &pmap_pteg_table[ptegidx];
	pt = &pteg->pt[i & 7];

	source_pvo = NULL;
	victim_pvo = NULL;
	LIST_FOREACH(pvo, &pmap_pvo_table[ptegidx], pvo_olink) {
		/*
		 * We need to find pvo entry for this address...
		 */
		PMAP_PVO_CHECK(pvo);		/* sanity check */
		if (source_pvo == NULL &&
		    pmap_pte_match(&pvo->pvo_pte, sr, addr, pvo->pvo_pte.pte_hi & PTE_HID)) {
			/*
			 * Now found an entry to be spilled into the pteg.
			 * The PTE is now be valid, so we know it's active;
			 */
			i = pmap_pte_insert(ptegidx, &pvo->pvo_pte);
			if (i >= 0) {
				source_pvo->pvo_vaddr |= i;
				pmap_pte_overflow--;
				return 1;
			}
			source_pvo = pvo;
			if (victim_pvo != NULL)
				break;
		}
		/*
		 * We also need the pvo entry of the victim we are replacing
		 * so save the R & C bits of the PTE.
		 */
		if (victim_pvo == NULL &&
		    pmap_pte_compare(pt, &pvo->pvo_pte)) {
			victim_pvo = pvo;
			if (source_pvo != NULL)
				break;
		}
	}

	if (source_pvo == NULL)
		return 0;

	if (victim_pvo == NULL)
		panic("pmap_pte_spill: victim pte has no pvo entry!");

	/*
	 * We are invalidating the TLB entry for the EA for the
	 * we are replacing even though its valid; If we don't
	 * we lose any ref/chg bit changes contained in the TLB
	 * entry.
	 */
	pmap_pte_unset(pt, &victim_pvo->pvo_pte, victim_pvo->pvo_vaddr);
	PVO_PTEGIDX_CLR(victim_pvo);

	source_pvo->pvo_pte.pte_hi &= ~PTE_HID;
	pmap_pte_set(pt, &source_pvo->pvo_pte);
	PVO_PTEGIDX_SET(source_pvo, i);
	pmap_pte_replacements++;
	return 1;
}

/*
 * Restrict given range to physical memory
 */
void
pmap_real_memory(start, size)
	paddr_t *start;
	psize_t *size;
{
	struct mem_region *mp;
	
	for (mp = mem; mp->size; mp++) {
		if (*start + *size > mp->start
		    && *start < mp->start + mp->size) {
			if (*start < mp->start) {
				*size -= mp->start - *start;
				*start = mp->start;
			}
			if (*start + *size > mp->start + mp->size)
				*size = mp->start + mp->size - *start;
			return;
		}
	}
	*size = 0;
}

/*
 * Initialize anything else for pmap handling.
 * Called during vm_init().
 */
void
pmap_init(void)
{
	int s;
#ifdef __HAVE_PMAP_PHYSSEG
	struct pvo_head *pvoh;
	int bank;
	long sz;
	char *attr;

	s = splvm();
	pvoh = pmap_physseg.pvoh;
	attr = pmap_physseg.attrs;
	for (bank = 0; bank < vm_nphysseg; bank++) {
		sz = vm_physmem[bank].end - vm_physmem[bank].start;
		vm_physmem[bank].pmseg.pvoh = pvoh;
		vm_physmem[bank].pmseg.attrs = attr;
		for (; sz > 0; sz--, pvoh++, attr++) {
			LIST_INIT(pvoh);
			*attr = 0;
		}
	}
	splx(s);
#endif

	s = splvm();
	pool_init(&pmap_mpvo_pool, sizeof(struct pvo_entry),
	    sizeof(struct pvo_entry), 0, 0, "pmap_mpvopl", NBPG,
	    pmap_pool_malloc, pmap_pool_mfree, M_VMPMAP);

	pool_setlowat(&pmap_mpvo_pool, 1008);

	pmap_initialized = 1;
	splx(s);
}

/*
 * How much virtual space is available to the kernel?
 */
void
pmap_virtual_space(vaddr_t *start, vaddr_t *end)
{
	/*
	 * Reserve one segment for kernel virtual memory
	 */
	*start = VM_MIN_KERNEL_ADDRESS + pmap_rkva_count * NBPG;
	*end = VM_MAX_KERNEL_ADDRESS;
}

/*
 * Create and return a physical map.
 */
pmap_t
pmap_create(void)
{
	pmap_t pm;
	
	pm = pool_get(&pmap_pool, PR_WAITOK);
	bzero((caddr_t)pm, sizeof *pm);
	pmap_pinit(pm);
	
	DPRINTFN(7,("pmap_create: pm %p:\n"
	    "\t%06x %06x %06x %06x    %06x %06x %06x %06x\n"
	    "\t%06x %06x %06x %06x    %06x %06x %06x %06x\n", pm,
	    pm->pm_sr[0], pm->pm_sr[1], pm->pm_sr[2], pm->pm_sr[3], 
	    pm->pm_sr[4], pm->pm_sr[5], pm->pm_sr[6], pm->pm_sr[7],
	    pm->pm_sr[8], pm->pm_sr[9], pm->pm_sr[10], pm->pm_sr[11], 
	    pm->pm_sr[12], pm->pm_sr[13], pm->pm_sr[14], pm->pm_sr[15]));
	return pm;
}

/*
 * Initialize a preallocated and zeroed pmap structure.
 */
unsigned short pmap_context = 0;
void
pmap_pinit(pmap_t pm)
{
	int i, mask;
	unsigned int entropy = MFTB();

	/*
	 * Allocate some segment registers for this pmap.
	 */
	pm->pm_refs = 1;
	for (i = 0; i < NPMAPS ; i += VSID_NBPW) {
		static unsigned int pmap_vsidcontext;
		unsigned int hash, n;

		/* Create a new value by multiplying by a prime adding in
		 * entropy from the timebase register.  This is to make the
		 * VSID more random so that the PT Hash function collides
		 * less often. (note that the prime causes gcc to do shifts
		 * instead of a multiply)
		 */
		pmap_vsidcontext = (pmap_vsidcontext * 0x1105) + entropy;
		hash = pmap_vsidcontext & (NPMAPS - 1);
		if (hash == 0)			/* 0 is special, avoid it */
			continue;
		n = hash >> 5;
		mask = 1 << (hash & (VSID_NBPW-1));
		hash = (pmap_vsidcontext & 0xfffff);
		if (pmap_vsid_bitmap[n] & mask) {	/* collision? */
			/* anything free in this bucket? */
			if (pmap_vsid_bitmap[n] == 0xffffffff) {
				entropy = (pmap_vsidcontext >> 20);
				continue;
			}
			i = ffs(~pmap_vsid_bitmap[i]) - 1;
			mask = 1 << i;
			hash &= 0xfffff & ~(VSID_NBPW-1);
			hash |= i;
		}
		pmap_vsid_bitmap[n] |= mask;
		for (i = 0; i < 16; i++)
			pm->pm_sr[i] = VSID_MAKE(i, hash);
		return;
	}
	panic("pmap_pinit: out of segments");
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
	int idx, mask;
	
	if (pm->pm_sr[0] == 0)
		panic("pmap_release");
	idx = VSID_TO_HASH(pm->pm_sr[0]) & (NPMAPS-1);
	mask = 1 << (idx % VSID_NBPW);
	idx /= VSID_NBPW;
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
pmap_update(void)
{
#ifdef MULTIPROCESSOR
	TLBSYNC();
#endif
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
	caddr_t va;
	int i;

	if (pa < SEGMENT_LENGTH) {
		va = (caddr_t) pa;
	} else if (pmap_initialized) {
		if (__predict_false(pmap_pvo_zeropage == NULL))
			pmap_pvo_zeropage = pmap_rkva_alloc(VM_PROT_READ|VM_PROT_WRITE);
		pmap_pa_map(pmap_pvo_zeropage, pa);
		va = (caddr_t) PVO_VADDR(pmap_pvo_zeropage);
	} else {
		panic("pmap_zero_page: can't zero pa %#lx", pa);
	}
#if 0
	bzero(va, NBPG);
#else

	for (i = NBPG/CACHELINESIZE; i > 0; i--) {
		__asm __volatile ("dcbz 0,%0" :: "r"(va));
		va += CACHELINESIZE;
	}
#endif
	if (pa >= SEGMENT_LENGTH)
		pmap_pa_unmap(pmap_pvo_zeropage);
}

/*
 * Copy the given physical source page to its destination.
 */
void
pmap_copy_page(paddr_t src, paddr_t dst)
{
	if (src < SEGMENT_LENGTH && dst < SEGMENT_LENGTH) {
		memcpy((void *) dst, (void *) src, NBPG);
		return;
	}
	if (pmap_initialized) {
		if (__predict_false(pmap_pvo_copypage_src == NULL))
			pmap_pvo_copypage_src = pmap_rkva_alloc(VM_PROT_READ);
		if (__predict_false(pmap_pvo_copypage_dst == NULL))
			pmap_pvo_copypage_dst = pmap_rkva_alloc(VM_PROT_READ|VM_PROT_WRITE);

		pmap_pa_map(pmap_pvo_copypage_src, src);
		pmap_pa_map(pmap_pvo_copypage_dst, dst);

		memcpy((caddr_t)PVO_VADDR(pmap_pvo_copypage_dst),
		    (caddr_t)PVO_VADDR(pmap_pvo_copypage_src),
		    NBPG);

		pmap_pa_unmap(pmap_pvo_copypage_src);
		pmap_pa_unmap(pmap_pvo_copypage_dst);
		return;
	}
	panic("pmap_copy_page: failed to copy contents of pa %#lx to pa %#lx", src, dst);
}

static __inline int
pmap_pvo_pte_index(const struct pvo_entry *pvo, int ptegidx)
{
	int pteidx;
	/*
	 * We can find the actual pte entry without searching by
	 * grabbing the PTEG index from 3 unused bits in pte_lo[11:9]
	 * and by noticing the HID bit.
	 */
	pteidx = ptegidx * 8 + PVO_PTEGIDX_GET(pvo);
	if (pvo->pvo_pte.pte_hi & PTE_HID)
		pteidx ^= pmap_pteg_mask * 8;
	return pteidx;
}

volatile pte_t *
pmap_pvo_to_pte(const struct pvo_entry *pvo, int pteidx)
{
	volatile pte_t *pt;

	/*
	 * If we haven't been supplied the ptegidx, calculate it.
	 */
	if (pteidx == -1) {
		int ptegidx;
		sr_t sr = va_to_sr(pvo->pvo_pmap->pm_sr, pvo->pvo_vaddr);
		ptegidx = va_to_pteg(sr, pvo->pvo_vaddr);
		pteidx = pmap_pvo_pte_index(pvo, ptegidx);
	}

	pt = &pmap_pteg_table[pteidx >> 3].pt[pteidx & 7];

	if ((pt->pte_hi ^ (pvo->pvo_pte.pte_hi & ~PTE_VALID)) == PTE_VALID) {
#ifdef DIAGNOSTIC
		if ((pvo->pvo_pte.pte_hi & PTE_VALID) == 0) {
#ifdef DEBUG
			pmap_pte_print(&pvo->pvo_pte);
			pmap_pte_print(pt);
#endif
			panic("pmap_pvo_to_pte: pvo %p: has valid pte in "
			    "pmap_pteg_table %p but invalid in pvo",
			    pvo, pt);
		}
		if (((pt->pte_lo ^ pvo->pvo_pte.pte_lo) & ~(PTE_CHG|PTE_REF)) != 0) {
#ifdef DEBUG
			pmap_pte_print(pt);
#endif
			panic("pmap_pvo_to_pte: pvo %p: pvo pte does "
			    "not match pte %p in pmap_pteg_table",
			    pvo, pt);
		}
#endif
		return pt;
	}

#ifdef DIAGNOSTIC
	if (pvo->pvo_pte.pte_hi & PTE_VALID)
#ifdef DEBUG
		pmap_pte_print(pt);
#endif
		panic("pmap_pvo_to_pte: pvo %p: has invalid pte in "
		    "pmap_pteg_table but valid in pvo", pvo);
#endif
	return NULL;
}

struct pvo_entry *
pmap_pvo_find_va(pmap_t pm, vaddr_t va, int *pteidx_p)
{
	struct pvo_entry *pvo;
	int ptegidx;
	sr_t sr;

	va &= ~ADDR_POFF;
	sr = va_to_sr(pm->pm_sr, va);
	ptegidx = va_to_pteg(sr, va);

	LIST_FOREACH(pvo, &pmap_pvo_table[ptegidx], pvo_olink) {
#ifdef DIAGNOSTIC
		if ((uintptr_t) pvo >= SEGMENT_LENGTH)
			panic("pmap_pvo_find_va: invalid pvo %p on list %#x",
			    pvo, ptegidx);
#endif
		if (pvo->pvo_pmap == pm && PVO_VADDR(pvo) == va) {
			if (pteidx_p)
				*pteidx_p = pmap_pvo_pte_index(pvo, ptegidx);
			return pvo;
		}
	}
	return NULL;
}

void
pmap_pa_map(struct pvo_entry *pvo, paddr_t pa)
{
	int s;
	s = splvm();
	pvo->pvo_pte.pte_lo |= pa;
	if (!pmap_pte_spill(pvo->pvo_vaddr))
		panic("pmap_pa_map: could not spill pvo %p", pvo);
	splx(s);
}

void
pmap_pa_unmap(struct pvo_entry *pvo)
{
	volatile pte_t *pt;
	int s;
	
	s = splvm();
	pt = pmap_pvo_to_pte(pvo, -1);
	if (pt != NULL) {
		pmap_pte_unset(pt, &pvo->pvo_pte, pvo->pvo_vaddr);
		PVO_PTEGIDX_CLR(pvo);
		pmap_pte_overflow++;
	}
	splx(s);
	pvo->pvo_pte.pte_lo &= ~PTE_RPGN;
}

void
pmap_syncicache(paddr_t pa)
{
	DPRINTFN(6,("pmap_syncicache: pa %#lx\n", pa));
	if (pa < SEGMENT_LENGTH) {
		__syncicache((void *)pa, NBPG);
		return;
	}
	if (pmap_initialized) {
		if (__predict_false(pmap_pvo_syncicache == NULL))
			pmap_pvo_syncicache = pmap_rkva_alloc(VM_PROT_READ|VM_PROT_WRITE);
		pmap_pa_map(pmap_pvo_syncicache, pa);
		__syncicache((void *)PVO_VADDR(pmap_pvo_syncicache), NBPG);
		pmap_pa_unmap(pmap_pvo_syncicache);
		return;
	}
	panic("pmap_syncicache: can't sync the icache @ pa %#lx", pa);
}

/*
 * Return a unmapped pvo for a kernel virtual address.
 * Used by pmap function that operate of physical pages.
 */
struct pvo_entry *
pmap_rkva_alloc(int prot)
{
	struct pvo_entry *pvo;
	volatile pte_t *pt;
	vaddr_t kva;
	int pteidx;

	if (pmap_rkva_count == 0)
		panic("pmap_kva_alloc: no more reserved KVAs!");
	
	kva = pmap_rkva_start + (NBPG * --pmap_rkva_count);
	pmap_kenter_pa(kva, 0, prot);
	pvo = pmap_pvo_find_va(pmap_kernel(), kva, &pteidx);
	if (pvo == NULL)
		panic("pmap_kva_alloc: pmap_pvo_find_va failed!");
	pt = pmap_pvo_to_pte(pvo, pteidx);
	if (pt == NULL)
		panic("pmap_kva_alloc: pmap_pvo_to_pte failed!");
	pmap_pte_unset(pt, &pvo->pvo_pte, pvo->pvo_vaddr);
	PVO_PTEGIDX_CLR(pvo);
	pmap_pte_overflow++;
	return pvo;
}

#if defined(DEBUG) || defined(PMAPCHECK)
void
pmap_pvo_check(const struct pvo_entry *pvo)
{
	struct pvo_head *pvo_head;
	struct pvo_entry *pvo0;
	volatile pte_t *pt;
	int failed = 0;

	if ((uintptr_t)(pvo+1) >= SEGMENT_LENGTH)
		panic("pmap_pvo_check: pvo %p: invalid address", pvo);

	if ((uintptr_t)(pvo->pvo_pmap+1) >= SEGMENT_LENGTH) {
		printf("pmap_pvo_check: pvo %p: invalid pmap address %p\n",
		    pvo, pvo->pvo_pmap);
		failed = 1;
	}

	if ((uintptr_t)pvo->pvo_olink.le_next >= SEGMENT_LENGTH ||
	    (((uintptr_t)pvo->pvo_olink.le_next) & 0x1f) != 0) {
		printf("pmap_pvo_check: pvo %p: invalid ovlink address %p\n",
		    pvo, pvo->pvo_olink.le_next);
		failed = 1;
	}

	if ((uintptr_t)pvo->pvo_vlink.le_next >= SEGMENT_LENGTH ||
	    (((uintptr_t)pvo->pvo_vlink.le_next) & 0x1f) != 0) {
		printf("pmap_pvo_check: pvo %p: invalid ovlink address %p\n",
		    pvo, pvo->pvo_vlink.le_next);
		failed = 1;
	}

	if (pvo->pvo_vaddr & PVO_MANAGED) {
		pvo_head = pa_to_pvoh(pvo->pvo_pte.pte_lo & PTE_RPGN);
	} else {
		if (pvo->pvo_vaddr < VM_MIN_KERNEL_ADDRESS) {
			printf("pmap_pvo_check: pvo %p: non kernel address "
			    "on kernel unmanaged list\n", pvo);
			failed = 1;
		}
		pvo_head = &pmap_pvo_kunmanaged;
	}
	LIST_FOREACH(pvo0, pvo_head, pvo_vlink) {
		if (pvo0 == pvo)
			break;
	}
	if (pvo0 == NULL) {
		printf("pmap_pvo_check: pvo %p: not present "
		    "on its vlist head %p\n", pvo, pvo_head);
		failed = 1;
	}
	if (pvo != pmap_pvo_find_va(pvo->pvo_pmap, pvo->pvo_vaddr, NULL)) {
		printf("pmap_pvo_check: pvo %p: not present "
		    "on its olist head\n", pvo);
		failed = 1;
	}
	pt = pmap_pvo_to_pte(pvo, -1);
	if (pt == NULL) {
		if (pvo->pvo_pte.pte_hi & PTE_VALID) {
			printf("pmap_pvo_check: pvo %p: pte_hi VALID but no PTE\n", pvo);
			failed = 1;
		}
	} else {
		if ((uintptr_t) pt < (uintptr_t) &pmap_pteg_table ||
		    (uintptr_t) pt >= (uintptr_t) &pmap_pteg_table[pmap_pteg_cnt]) {
			printf("pmap_pvo_check: pvo %p: pte %p not in pteg table\n", pvo, pt);
			failed = 1;
		}
		if (((((uintptr_t) pt) >> 3) & 7) != PVO_PTEGIDX_GET(pvo)) {
			printf("pmap_pvo_check: pvo %p: pte_hi VALID but no PTE\n", pvo);
			failed = 1;
		}
		if (pvo->pvo_pte.pte_hi != pt->pte_hi) {
			printf("pmap_pvo_check: pvo %p: pte_hi differ: %#x/%#x\n", pvo,
			    pvo->pvo_pte.pte_hi, pt->pte_hi);
			failed = 1;
		}
		if (((pvo->pvo_pte.pte_lo ^ pt->pte_lo) &
		    (PTE_PP|PTE_W|PTE_I|PTE_G|PTE_RPGN)) != 0) {
			printf("pmap_pvo_check: pvo %p: pte_lo differ: %#x/%#x\n",
			    pvo,
			    pvo->pvo_pte.pte_lo & (PTE_PP|PTE_W|PTE_I|PTE_G|PTE_RPGN),
			    pt->pte_lo & (PTE_PP|PTE_W|PTE_I|PTE_G|PTE_RPGN));
			failed = 1;
		}
		if (pmap_pte_to_va(pt) != PVO_VADDR(pvo)) {
			printf("pmap_pvo_check: pvo %p: PTE %p derived VA %#lx"
			    " doesn't not match PVO's VA %#lx\n",
			    pvo, pt, pmap_pte_to_va(pt), PVO_VADDR(pvo));
			failed = 1;
		}
		if (failed)
			pmap_pte_print(pt);
	}
	if (failed)
		panic("pmap_pvo_check: pvo %p, pm %p: bugcheck!", pvo,
		    pvo->pvo_pmap);
}
#endif /* DEBUG || PMAPCHECK */

/*
 * This returns whether this is the first mapping of a page.
 */
int
pmap_pvo_enter(pmap_t pm, struct pool *pl, struct pvo_head *pvo_head,
	vaddr_t va, paddr_t pa, u_int pte_lo, int flags)
{
	struct pvo_entry *pvo;
	sr_t sr;
	int first;
	int ptegidx;
	int i;

	pmap_pvo_enter_calls++;
	/*
	 * Compute the HTAB index.
	 */
	sr = va_to_sr(pm->pm_sr, va);
	ptegidx = va_to_pteg(sr, va);

	/*
	 * Remove any existing mapping for this page.  Reuse the
	 * pvo entry if there a mapping.
	 */
	LIST_FOREACH(pvo, &pmap_pvo_table[ptegidx], pvo_olink) {
		if (pvo->pvo_pmap == pm && PVO_VADDR(pvo) == va) {
#ifdef DEBUG
			if (((pvo->pvo_pte.pte_lo ^ (pa|pte_lo)) &
			    (PTE_RPGN|PTE_W|PTE_M|PTE_I|PTE_G|PTE_PP)) == 0 &&
			   va < VM_MIN_KERNEL_ADDRESS) {
				printf("pmap_pvo_enter: pvo %p: dup %#x/%#lx\n",
				    pvo, pvo->pvo_pte.pte_lo, pte_lo|pa);
				printf("pmap_pvo_enter: pte_hi=%#x sr=%#x\n",
				    pvo->pvo_pte.pte_hi,
				    pm->pm_sr[va >> ADDR_SR_SHFT]);
				pmap_pte_print(pmap_pvo_to_pte(pvo, -1));
#ifdef DDB
				Debugger();
#endif
			}
#endif
			pmap_pvo_remove(pvo, -1, FALSE);
			break;
		}
	}

	/*
	 * Remember is the list was empty and therefore will be
	 * the first item.
	 */
	first = LIST_FIRST(pvo_head) == NULL;

	/*
	 * If we aren't overwriting an mapping, try to allocate
	 */
	if (pvo == NULL) {
		int poolflags = PR_NOWAIT;
		if ((flags & PMAP_CANFAIL) == 0)
			poolflags |= PR_URGENT;
		pvo = pool_get(pl, poolflags);
		if (pvo == NULL) {
#if 0
			pvo = pmap_pvo_reclaim(pm);
			if (pvo == NULL) {
#endif
				if ((flags & PMAP_CANFAIL) == 0)
					panic("pmap_pvo_enter: failed");
				return ENOMEM;
#if 0
			}
#endif
		}
		pmap_pvo_entries++;
		pvo->pvo_vaddr = va;
		pvo->pvo_pmap = pm;
		LIST_INSERT_HEAD(&pmap_pvo_table[ptegidx], pvo, pvo_olink);
#if 0
	} else {
		if (pmap_initialized && pm != pmap_kernel())
			printf("pmap_pvo_enter: pmap %p: reusing pvo %p for va %#x\n", pm, pvo, va);
#endif
	}
	pvo->pvo_vaddr &= ~ADDR_POFF;
	if (flags & PMAP_WIRED)
		pvo->pvo_vaddr |= PVO_WIRED;
	if (pvo_head != &pmap_pvo_kunmanaged)
		pvo->pvo_vaddr |= PVO_MANAGED; 
	pmap_pte_create(&pvo->pvo_pte, sr, va, pa | pte_lo);
	LIST_INSERT_HEAD(pvo_head, pvo, pvo_vlink);
	if (pvo->pvo_pte.pte_lo & PMAP_WIRED)
		pvo->pvo_pmap->pm_stats.wired_count++;
	pvo->pvo_pmap->pm_stats.resident_count++;
#if defined(DEBUG)
	if (pm != pmap_kernel() && va < VM_MIN_KERNEL_ADDRESS)
		DPRINTFN(4,("pmap_pvo_enter: pvo %p: pm %p va %#lx pa %#lx\n",
			pvo, pm, va, pa));
#endif
	/*
	 * We hope this succeeds but it isn't required.
	 */
	i = pmap_pte_insert(ptegidx, &pvo->pvo_pte);
	if (i >= 0) {
		PVO_PTEGIDX_SET(pvo, i);
	} else {
		pmap_pte_overflow++;
#if 0
		if ((flags & VM_PROT_ALL) != VM_PROT_NONE)
			pmap_pte_evict(pvo, ptegidx, MFTB() & 7);
#endif
	}
	PMAP_PVO_CHECK(pvo);		/* sanity check */
	return first ? ENOENT : 0;
}

void
pmap_pvo_remove(struct pvo_entry *pvo, int pteidx, int freeit)
{
	volatile pte_t *pt;

	PMAP_PVO_CHECK(pvo);		/* sanity check */
	/* 
	 * If there is an active pte entry, we need to deactivate it
	 * (and save the ref & chg bits).
	 */
	pt = pmap_pvo_to_pte(pvo, pteidx);
	if (pt != NULL) {
		pmap_pte_unset(pt, &pvo->pvo_pte, pvo->pvo_vaddr);
		PVO_PTEGIDX_CLR(pvo);
	} else {
		pmap_pte_overflow--;
	}

	/*
	 * Update our statistics
	 */
	pvo->pvo_pmap->pm_stats.resident_count--;
	if (pvo->pvo_pte.pte_lo & PMAP_WIRED)
		pvo->pvo_pmap->pm_stats.wired_count--;

	/*
	 * Save the REF/CHG bits into their cache if the page is managed.
	 */
	if (pvo->pvo_vaddr & PVO_MANAGED) {
		struct vm_page *pg = PHYS_TO_VM_PAGE(pvo->pvo_pte.pte_lo & PTE_RPGN);
		if (pg != NULL) {
			pmap_attr_save(pg, pvo->pvo_pte.pte_lo & (PTE_REF|PTE_CHG));
		}
	}

	/*
	 * Remove this PVO from the PV list
	 */
	LIST_REMOVE(pvo, pvo_vlink);

	/*
	 * Remove this from the Overflow list and return it to the pool...
	 * ... if we aren't going to reuse it.
	 */
	if (freeit) {
		LIST_REMOVE(pvo, pvo_olink);
		pool_put(pvo->pvo_vaddr & PVO_MANAGED
		    ? &pmap_mpvo_pool
		    : &pmap_upvo_pool,
		    pvo);
		pmap_pvo_entries--;
		pmap_pvo_remove_calls++;
	}
}

/*
 * Insert physical page at pa into the given pmap at virtual address va.
 */
int
pmap_enter(pmap_t pm, vaddr_t va, paddr_t pa, vm_prot_t prot, int flags)
{
	struct mem_region *mp;
	struct pvo_head *pvo_head;
	struct pool *pl;
	u_int pte_lo;
	int s;
	int error;
	u_int pvo_flags;

	if (__predict_false(!pmap_initialized)) {
		pvo_head = &pmap_pvo_kunmanaged;
		pl = &pmap_upvo_pool;
		pvo_flags = 0;
	} else {
		pvo_head = pa_to_pvoh(pa);
		pl = &pmap_mpvo_pool;
		pvo_flags = PVO_MANAGED;
	}

	DPRINTFN(16, ("pmap_enter(0x%p, 0x%lx, 0x%lx, 0x%x, 0x%x) ",
		pm, va, pa, prot, flags));

	pte_lo = PTE_I | PTE_G;
	if ((flags & PMAP_NC) == 0) {
		for (mp = mem; mp->size; mp++) {
			if (pa >= mp->start && pa < mp->start + mp->size) {
				pte_lo &= ~(PTE_I | PTE_G);
				break;
			}
		}
	}

	if (prot & VM_PROT_WRITE)
		pte_lo |= PTE_RW;
	else
		pte_lo |= PTE_RO;

	/*
	 * Record mapping for later back-translation and pte spilling.
	 * This will overwrite any existing mapping.
	 */
	s = splvm();
	error = pmap_pvo_enter(pm, pl, pvo_head, va, pa, pte_lo, flags);
	splx(s);

	if (error == ENOENT) {
		/* 
		 * Flush the real memory from the cache.
		 */
		if (((prot|flags) & VM_PROT_EXECUTE) && (pte_lo & PTE_I) == 0) {
			pmap_syncicache(pa);
		}
		error = 0;
	}

	return error;
}

void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot)
{
	struct mem_region *mp;
	u_int pte_lo;
	int error;
	int s;

	if (va < VM_MIN_KERNEL_ADDRESS)
		panic("pmap_kenter_pa: attempt to enter "
		    "non-kernel address %#lx!", va);

	DPRINTFN(5,("pmap_kenter_pa(%#lx,%#lx,%#x)\n", va, pa, prot));

	pte_lo = PTE_I | PTE_G;
	for (mp = mem; mp->size; mp++) {
		if (pa >= mp->start && pa < mp->start + mp->size) {
			pte_lo &= ~(PTE_I | PTE_G);
			break;
		}
	}

	if (prot & VM_PROT_WRITE)
		pte_lo |= PTE_RW;
	else
		pte_lo |= PTE_RO;

	s = splvm();
	error = pmap_pvo_enter(pmap_kernel(), &pmap_upvo_pool, &pmap_pvo_kunmanaged,
	    va, pa, pte_lo, prot|PMAP_WIRED);
	splx(s);

	if (error != 0 && error != ENOENT)
		panic("pmap_kenter_pa: failed to enter va %#lx pa %#lx: %d", va, pa, error);

	/* 
	 * Flush the real memory from the instruction cache.
	 */
	if ((prot & VM_PROT_EXECUTE) && (pte_lo & (PTE_I|PTE_G)) == 0) {
		pmap_syncicache(pa);
	}
}

void
pmap_kremove(vaddr_t va, vsize_t len)
{
	if (va < VM_MIN_KERNEL_ADDRESS)
		panic("pmap_kremove: attempt to remove "
		    "non-kernel address %#lx!", va);

	DPRINTFN(5,("pmap_kremove(%#lx,%#lx)\n", va, len));
	pmap_remove(pmap_kernel(), va, va + len);
}

/*
 * Remove the given range of mapping entries.
 */
void
pmap_remove(pmap_t pm, vaddr_t va, vaddr_t endva)
{
	struct pvo_entry *pvo;
	int pteidx;
	int s;

	for (; va < endva; va += PAGE_SIZE) {
		s = splvm();
		pvo = pmap_pvo_find_va(pm, va, &pteidx);
		if (pvo != NULL)
			pmap_pvo_remove(pvo, pteidx, TRUE);
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
	int s;
	
	s = splvm();
	pvo = pmap_pvo_find_va(pm, va & ~ADDR_POFF, NULL);
	if (pvo != NULL) {
		PMAP_PVO_CHECK(pvo);		/* sanity check */
		*pap = (pvo->pvo_pte.pte_lo & PTE_RPGN) | (va & ADDR_POFF);
#ifdef DEBUG
	} else {
		if (pm == pmap_kernel()) {
			if (va >= VM_MIN_KERNEL_ADDRESS) {
				printf("pmap_extract: va=%#lx: no pa\n", va);
#ifdef DDB
				Debugger();
#endif
			}
		}
#endif
	}
	splx(s);
	return pvo != NULL;
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
	int s;
	int pteidx;

#if 0
	/*
	 * Since this routine only downgrades protection, if the
	 * maximal protection is desired, there isn't any change
	 * to be made.
	 */
	if ((prot & (VM_PROT_READ|VM_PROT_WRITE)) == (VM_PROT_READ|VM_PROT_WRITE))
		return;
#endif

	/*
	 * If there is no protection, this is equivalent to
	 * remove the pmap from the pmap.
	 */
	if ((prot & VM_PROT_READ) == 0) {
		pmap_remove(pm, va, endva);
		return;
	}

	s = splvm();
	for (; va < endva; va += NBPG) {
		pvo = pmap_pvo_find_va(pm, va, &pteidx);
		if (pvo == NULL)
			continue;
		PMAP_PVO_CHECK(pvo);		/* sanity check */

#if 0
		/*
		 * If the page is already read-only, no change
		 * needs to be made.
		 */
		if ((pvo->pvo_pte.pte_lo & PTE_PP) == PTE_RO)
			continue;
#endif

		/*
		 * Change the protection of the page.
		 */
		pvo->pvo_pte.pte_lo &= ~PTE_PP;
		pvo->pvo_pte.pte_lo |= PTE_RO;

		/*
		 * If the PVO is in the page table, update
		 * that pte at well.
		 */
		pt = pmap_pvo_to_pte(pvo, pteidx);
		if (pt != NULL)
			pmap_pte_change(pt, &pvo->pvo_pte, pvo->pvo_vaddr);

		PMAP_PVO_CHECK(pvo);		/* sanity check */
	}
	splx(s);
}

void
pmap_unwire(pmap_t pm, vaddr_t va)
{
	struct pvo_entry *pvo;
	int s;

	s = splvm();
	pvo = pmap_pvo_find_va(pm, va, NULL);
	if (pvo != NULL) {
		if (pvo->pvo_vaddr & PVO_WIRED) {
			pvo->pvo_vaddr &= ~PVO_WIRED;
			pm->pm_stats.wired_count--;
		}
		PMAP_PVO_CHECK(pvo);		/* sanity check */
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
	int s;

	/*
	 * Since this routine only downgrades protection, if the
	 * maximal protection is desired, there isn't any change
	 * to be made.
	 */
	if ((prot & (VM_PROT_READ|VM_PROT_WRITE)) == (VM_PROT_READ|VM_PROT_WRITE))
		return;

	s = splvm();
	pvo_head = vm_page_to_pvoh(pg);
	for (pvo = LIST_FIRST(pvo_head); pvo != NULL; pvo = next_pvo) {
		next_pvo = LIST_NEXT(pvo, pvo_vlink);
		PMAP_PVO_CHECK(pvo);		/* sanity check */
		/*
		 * Downgrading to no mapping at all, we just remove
		 * the entry EXCEPT if its WIRED.  If it WIRED, we
		 * just leave it alone.
		 */
		if ((prot & VM_PROT_READ) == 0) {
			if ((pvo->pvo_vaddr & PVO_WIRED) == 0)
				pmap_pvo_remove(pvo, -1, TRUE);
			continue;
		}
		pvo->pvo_pte.pte_lo &= ~PTE_PP;
		pvo->pvo_pte.pte_lo |= PTE_RO;
		pt = pmap_pvo_to_pte(pvo, -1);
		if (pt != NULL)
			pmap_pte_change(pt, &pvo->pvo_pte, pvo->pvo_vaddr);
		PMAP_PVO_CHECK(pvo);		/* sanity check */
	}
	splx(s);
}

/*
 * Activate the address space for the specified process.  If the process
 * is the current process, load the new MMU context.
 */
void
pmap_activate(struct proc *p)
{
	struct pcb *pcb = &p->p_addr->u_pcb;
	pmap_t pmap = p->p_vmspace->vm_map.pmap;
	int psl, i, seg;

	DPRINTFN(6,("pmap_activate: proc %p (curproc %p)\n", p, curproc));

	/*
	 * XXX Normally performed in cpu_fork().
	 */
	if (pcb->pcb_pm != pmap) {
		pcb->pcb_pm = pmap;
		pcb->pcb_pmreal = pmap;
	}

	if (p == curproc) {
		/* Disable interrupts while switching. */
		__asm __volatile("mfmsr %0" : "=r"(psl) :);
		psl &= ~PSL_EE;
		__asm __volatile("mtmsr %0" :: "r"(psl));
		__asm __volatile("isync");

		/* Store pointer to new current pmap. */
		curpm = pmap;

		/*
		 * Set new segment registers.  Pmap's are always in
		 * BAT0 so they are always accessible.  Don't load
		 * the kernel SR.
		 */
		DPRINTFN(7,("pmap_activate: pm %p:", pmap));
		for (i = 0; i < 16; i++) {
			if (i == KERNEL_SR)
				continue;
			seg = pmap->pm_sr[i];
			__asm __volatile("mtsrin %0,%1"
			    :: "r"(seg), "r"(i << ADDR_SR_SHFT));
			DPRINTFN(7,(" sr[%d]=%#x", i, seg));
		}
		DPRINTFN(7,("\n"));

		/* Interrupts are OK again. */
		psl |= PSL_EE;
		__asm __volatile("mtmsr %0" :: "r"(psl));
	}
}

/*
 * Deactivate the specified process's address space.
 */
void
pmap_deactivate(struct proc *p)
{
}

boolean_t
pmap_query_bit(struct vm_page *pg, int ptebit)
{
	struct pvo_entry *pvo;
	volatile pte_t *pt;
	int s;

	if (pmap_attr_fetch(pg) & ptebit)
		return TRUE;
	s = splvm();
	LIST_FOREACH(pvo, vm_page_to_pvoh(pg), pvo_vlink) {
		PMAP_PVO_CHECK(pvo);		/* sanity check */
		/*
		 * See if we saved the bit off.  If so cache, it and return
		 * success.
		 */
		if (pvo->pvo_pte.pte_lo & ptebit) {
			pmap_attr_save(pg, ptebit);
			PMAP_PVO_CHECK(pvo);		/* sanity check */
			splx(s);
			return TRUE;
		}
	}
	/*
	 * No luck, now go thru the hard part of looking at the ptes
	 * themselves.  Sync so any pending REF/CHG bits are flushed
	 * to the PTEs.
	 */
	__asm __volatile("sync");
	LIST_FOREACH(pvo, vm_page_to_pvoh(pg), pvo_vlink) {
		PMAP_PVO_CHECK(pvo);		/* sanity check */
		/*
		 * See if this pvo have a valid PTE.  If so, fetch the
		 * REF/CHG bits from the valid PTE.  If the appropriate
		 * ptebit is set, cache, it and return success.
		 */
		pt = pmap_pvo_to_pte(pvo, -1);
		if (pt != NULL) {
			pmap_pte_synch(pt, &pvo->pvo_pte);
			if (pvo->pvo_pte.pte_lo & ptebit) {
				pmap_attr_save(pg, ptebit);
				PMAP_PVO_CHECK(pvo);		/* sanity check */
				splx(s);
				return TRUE;
			}
		}
	}
	splx(s);
	return FALSE;
}

boolean_t
pmap_clear_bit(struct vm_page *pg, int ptebit)
{
	struct pvo_entry *pvo;
	volatile pte_t *pt;
	int rv = 0;
	int s;

	s = splvm();
	/*
	 * Clear the cached value.
	 */
	rv |= pmap_attr_fetch(pg);
	pmap_attr_clear(pg, ptebit);

	/*
	 * Sync so any pending REF/CHG bits are flushed to the PTEs (so we
	 * can reset the right ones).  Note that since the pvo entries and
	 * list heads are accessed via BAT0 and are never placed in the 
	 * page table, we don't have to worry about further accesses setting
	 * the REF/CHG bits.
	 */
	__asm __volatile("sync");

	/*
	 * For each pvo entry, clear pvo's ptebit.  If this pvo have a
	 * valid PTE.  If so, clear the ptebit from the valid PTE.
	 */
	LIST_FOREACH(pvo, vm_page_to_pvoh(pg), pvo_vlink) {
		PMAP_PVO_CHECK(pvo);		/* sanity check */
		pt = pmap_pvo_to_pte(pvo, -1);
		if (pt != NULL) {
			pmap_pte_synch(pt, &pvo->pvo_pte);
			pmap_pte_clear(pt, ptebit);
		}
		rv |= pvo->pvo_pte.pte_lo;
		pvo->pvo_pte.pte_lo &= ~ptebit;
		PMAP_PVO_CHECK(pvo);		/* sanity check */
	}
	splx(s);
	return (rv & ptebit) != 0;
}

#ifdef ALLEGRO
/* 
 * for ALLEGRO large memory suport, do pmap_procwr()
 * icache invalidation through user mappings via USER_SR
 * following the method used in copyout()\trap.c
 */
static __inline sr_t
setusr(int content)
{
	sr_t usr;
	unsigned int psl;

	/* Disable interrupts while switching. */
	__asm __volatile("mfmsr %0" : "=r"(psl) :);
	psl &= ~PSL_EE;
	__asm __volatile("mtmsr %0" :: "r"(psl));
	
	__asm __volatile("mfsr %0,%1" : "=r"(usr) : "n"(USER_SR));
	__asm __volatile ("isync; mtsr %0,%1; isync"
		:: "n"(USER_SR), "r"(content));

	/* Interrupts are OK again. */
	psl |= PSL_EE;
	__asm __volatile("mtmsr %0" :: "r"(psl));

	return usr;
}

void
pmap_procwr(struct proc *p, vaddr_t uva, size_t len)
{
	vaddr_t kva;
	sr_t usr, ssr;
	faultbuf env;
	paddr_t pa;
	int s;

	s = splvm();
#ifdef DIAGNOSTIC
	if (trunc_page(uva + len - 1) != trunc_page(uva))
		panic("pmap_procwr >1 pg");
#endif
	/*
	 * if page is not mapped, nothing to invalidate
	 * this happens sometimes in the procfs_domem() case
	 * (we don't actually use pa)
	 */
	if (! pmap_extract(p->p_vmspace->vm_map.pmap, uva, &pa)){
		splx(s);
		return;
	}

	if (setfault(env)) {
		curpcb->pcb_onfault = 0;
		splx(s);
		return;		/* EFAULT XXX */
	}

	kva = (vaddr_t)USER_ADDR + ((u_int)uva & ~SEGMENT_MASK);
	ssr = p->p_vmspace->vm_map.pmap->pm_sr[(u_int)uva >> ADDR_SR_SHFT];
	ssr &= SR_VSID;
	ssr |= SR_SUKEY;

	usr = setusr(ssr);
	__syncicache((void *)kva, len);
	(void)setusr(usr);

	curpcb->pcb_onfault = 0;
	splx(s);
}
#else
void
pmap_procwr(struct proc *p, vaddr_t va, size_t len)
{
	paddr_t pa;

	(void) pmap_extract(p->p_vmspace->vm_map.pmap, va, &pa);
	__syncicache((void *)pa, len);
}
#endif	/* ALLEGRO */

#if defined(DEBUG) || defined(PMAPCHECK)
void
pmap_pte_print(volatile pte_t *pt)
{
	printf("PTE %p: ", pt);
	/* High word: */
	printf("0x%08x: [", pt->pte_hi);
	printf("%c ", (pt->pte_hi & PTE_VALID) ? 'v' : 'i');
	printf("%c ", (pt->pte_hi & PTE_HID) ? 'h' : '-');
	printf("0x%06x 0x%02X",
	    (pt->pte_hi &~ PTE_VALID)>>PTE_VSID_SHFT,
	    pt->pte_hi & PTE_API);
	printf(" (va 0x%08lx)] ", pmap_pte_to_va(pt));
	/* Low word: */
	printf(" 0x%08x: [", pt->pte_lo);
	printf("0x%05x... ", pt->pte_lo >> 12);
	printf("%c ", (pt->pte_lo & PTE_REF) ? 'r' : 'u');
	printf("%c ", (pt->pte_lo & PTE_CHG) ? 'c' : 'n');
	printf("%c", (pt->pte_lo & PTE_W) ? 'w' : '.');
	printf("%c", (pt->pte_lo & PTE_I) ? 'i' : '.');
	printf("%c", (pt->pte_lo & PTE_M) ? 'm' : '.');
	printf("%c ", (pt->pte_lo & PTE_G) ? 'g' : '.');
	switch (pt->pte_lo & PTE_PP) {
	case PTE_RO:
		printf("ro]\n");
		break;
	case PTE_RW:
		printf("rw]\n");
		break;
	default:
		printf("na]\n");
		break;
	}
}

void
pmap_pteg_check(void)
{
	volatile pte_t *pt;
	int i;
	int ptegidx;
	u_int p_valid = 0;
	u_int s_valid = 0;
	u_int invalid = 0;
	
	for (ptegidx = 0; ptegidx < pmap_pteg_cnt; ptegidx++) {
		for (pt = pmap_pteg_table[ptegidx].pt, i = 8; --i >= 0; pt++) {
			if (pt->pte_hi & PTE_VALID) {
				if (pt->pte_hi & PTE_HID)
					s_valid++;
				else
					p_valid++;
			} else
				invalid++;
		}
	}
	printf("pteg_check: v(p) %#x (%d), v(s) %#x (%d), i %#x (%d)\n",
		p_valid, p_valid, s_valid, s_valid,
		invalid, invalid);
}

void
pmap_print_mmuregs(void)
{
	int i;
	sr_t sr;
	u_int32_t x;
	unsigned int addr;
	sr_t soft_sr[16];
	struct bat soft_ibat[4];
	struct bat soft_dbat[4];
	u_int32_t sdr1;
	
	asm ("mfsdr1 %0" : "=r"(sdr1));
	for (i=0; i<16; i++) {
		asm ("mfsrin %0,%1" : "=r"(sr) : "r"(addr));
		soft_sr[i] = sr;
		addr += (1 << ADDR_SR_SHFT);
	}
	/* read iBAT registers */
	i = 0;
	asm ("mfibatu %0,0" : "=r"(x));
	soft_ibat[i].batu = x;
	asm ("mfibatl %0,0" : "=r"(x));
	soft_ibat[i++].batl = x;
	asm ("mfibatu %0,1" : "=r"(x));
	soft_ibat[i].batu = x;
	asm ("mfibatl %0,1" : "=r"(x));
	soft_ibat[i++].batl = x;
	asm ("mfibatu %0,2" : "=r"(x));
	soft_ibat[i].batu = x;
	asm ("mfibatl %0,2" : "=r"(x));
	soft_ibat[i++].batl = x;
	asm ("mfibatu %0,3" : "=r"(x));
	soft_ibat[i].batu = x;
	asm ("mfibatl %0,3" : "=r"(x));
	soft_ibat[i].batl = x;


	/* read dBAT registers */
	i = 0;
	__asm __volatile ("mfdbatu %0,0" : "=r"(x));
	soft_ibat[i].batu = x;
	__asm __volatile ("mfdbatl %0,0" : "=r"(x));
	soft_ibat[i++].batl = x;
	__asm __volatile ("mfdbatu %0,1" : "=r"(x));
	soft_ibat[i].batu = x;
	__asm __volatile ("mfdbatl %0,1" : "=r"(x));
	soft_ibat[i++].batl = x;
	__asm __volatile ("mfdbatu %0,2" : "=r"(x));
	soft_ibat[i].batu = x;
	__asm __volatile ("mfdbatl %0,2" : "=r"(x));
	soft_ibat[i++].batl = x;
	__asm __volatile ("mfdbatu %0,3" : "=r"(x));
	soft_ibat[i].batu = x;
	__asm __volatile ("mfdbatl %0,3" : "=r"(x));
	soft_ibat[i].batl = x;


	printf("SDR1 0x%x\n", sdr1);
	printf("SR[]:\t");
	addr = 0;
	for (i=0; i<4; i++)
		printf("0x%06x, ", soft_sr[i]);
	printf("\n\t");
	for ( ; i<8; i++)
		printf("0x%06x, ", soft_sr[i]);
	printf("\n\t");
	for ( ; i<12; i++)
		printf("0x%06x, ", soft_sr[i]);
	printf("\n\t");
	for ( ; i<16; i++)
		printf("0x%06x, ", soft_sr[i]);
	printf("\n");

	printf("iBAT[]:\t");
	for (i=0; i<4; i++)
		printf("0x%-8x 0x%-8x, ",
			soft_ibat[i].batu, soft_ibat[i].batl);
	printf("\ndBAT[]:\t");
	for (i=0; i<4; i++)
		printf("0x%-8x 0x%-8x, ",
			soft_ibat[i].batu, soft_dbat[i].batl);
	printf("\n");
}

void
pmap_print_pte(pmap_t pm, vaddr_t va)
{
	struct pvo_entry *pvo;
	volatile pte_t *pt;
	int pteidx;

	pvo = pmap_pvo_find_va(pm, va, &pteidx);
	if (pvo != NULL) {
		pt = pmap_pvo_to_pte(pvo, pteidx);
		if (pt != NULL) {
			printf("VA %#lx -> %p -> %s %#x, %#x\n",
				va, pt,
				pt->pte_hi & PTE_HID ? "(sec)" : "(pri)",
				pt->pte_hi, pt->pte_lo);
		} else {
			printf("No valid PTE found\n");
		}
	} else {
		printf("Address not in pmap\n");
	}
}

void
pmap_pteg_dist(void)
{
	struct pvo_entry *pvo;
	int ptegidx;
	int depth;
	int max_depth = 0;
	unsigned int depths[64];

	memset(depths, 0, sizeof(depths));
	for (ptegidx = 0; ptegidx < pmap_pteg_cnt; ptegidx++) {
		depth = 0;
		LIST_FOREACH(pvo, &pmap_pvo_table[ptegidx], pvo_olink) {
			depth++;
		}
		if (depth > max_depth)
			max_depth = depth;
		if (depth > 63)
			depth = 63;
		depths[depth]++;
	}

	for (depth = 0; depth < 64; depth++) {
		printf("  [%2d]: %8u", depth, depths[depth]);
		if ((depth & 3) == 3)
			printf("\n");
		if (depth == max_depth)
			break;
	}
	if ((depth & 3) != 3)
		printf("\n");
	printf("Max depth found was %d\n", max_depth);
}
#endif /* DEBUG */

#ifdef PMAPCHECK
void
pmap_pvo_verify(void)
{
	int ptegidx;
	int s;

	s = splvm();
	for (ptegidx = 0; ptegidx < pmap_pteg_cnt; ptegidx++) {
		struct pvo_entry *pvo;
		LIST_FOREACH(pvo, &pmap_pvo_table[ptegidx], pvo_olink) {
			if ((uintptr_t) pvo >= SEGMENT_LENGTH)
				panic("pmap_pvo_find_va: invalid pvo %p "
				    "on list %#x", pvo, ptegidx);
			pmap_pvo_check(pvo);
		}
	}
	splx(s);
}
#endif /* PMAPCHECK */


void *
pmap_pool_ualloc(unsigned long size, int flags, int tag)
{
	struct pvo_page *pvop;

	if (size != PAGE_SIZE)
		panic("pmap_pool_alloc: size(%lu) != PAGE_SIZE", size);

	pvop = SIMPLEQ_FIRST(&pmap_upvop_head);
	if (pvop != NULL) {
		pmap_upvop_free--;
		SIMPLEQ_REMOVE_HEAD(&pmap_upvop_head, pvop, pvop_link);
		return pvop;
	}
	if (uvm.page_init_done != TRUE) {
		return (void *) uvm_pageboot_alloc(PAGE_SIZE);
	}
	return pmap_pool_malloc(size, flags, tag);
}

void *
pmap_pool_malloc(unsigned long size, int flags, int tag)
{
	struct pvo_page *pvop;
	struct vm_page *pg;

	if (size != PAGE_SIZE)
		panic("pmap_pool_alloc: size(%lu) != PAGE_SIZE", size);

	pvop = SIMPLEQ_FIRST(&pmap_mpvop_head);
	if (pvop != NULL) {
		pmap_mpvop_free--;
		SIMPLEQ_REMOVE_HEAD(&pmap_mpvop_head, pvop, pvop_link);
		return pvop;
	}
 again:
	pg = uvm_pagealloc_strat(NULL, 0, NULL, UVM_PGA_USERESERVE,
	    UVM_PGA_STRAT_ONLY, VM_FREELIST_FIRST256);
	if (__predict_false(pg == NULL)) {
		if (flags & PR_WAITOK) {
			uvm_wait("plpg");
			goto again;
		} else {
			return (0);
		}
	}
	return (void *) VM_PAGE_TO_PHYS(pg);
}

void
pmap_pool_ufree(void *va, unsigned long size, int tag)
{
	struct pvo_page *pvop;
#if 0
	if (PHYS_TO_VM_PAGE((paddr_t) va) != NULL) {
		pmap_pool_mfree(va, size, tag);
		return;
	}
#endif
	pvop = va;
	SIMPLEQ_INSERT_HEAD(&pmap_upvop_head, pvop, pvop_link);
	pmap_upvop_free++;
	if (pmap_upvop_free > pmap_upvop_maxfree)
		pmap_upvop_maxfree = pmap_upvop_free;
}

void
pmap_pool_mfree(void *va, unsigned long size, int tag)
{
	struct pvo_page *pvop;
	if (size != PAGE_SIZE)
		panic("pmap_pool_alloc: size(%lu) != PAGE_SIZE", size);

	pvop = va;
	SIMPLEQ_INSERT_HEAD(&pmap_mpvop_head, pvop, pvop_link);
	pmap_mpvop_free++;
	if (pmap_mpvop_free > pmap_mpvop_maxfree)
		pmap_mpvop_maxfree = pmap_mpvop_free;
#if 0
	uvm_pagefree(PHYS_TO_VM_PAGE((paddr_t) va));
#endif
}

/*
 * This routine in bootstraping to steal to-be-managed memory (which will
 * then be unmanaged).  We use it to grab from the first 256MB for our
 * pmap needs and above 256MB for other stuff.
 */
vaddr_t
pmap_steal_memory(vsize_t vsize, vaddr_t *vstartp, vaddr_t *vendp)
{
	vsize_t size;
	vaddr_t va;
	paddr_t pa = 0;
	int npgs, bank;
	struct vm_physseg *ps;

	if (uvm.page_init_done == TRUE)
		panic("pmap_steal_memory: called _after_ bootstrap");

	*vstartp = VM_MIN_KERNEL_ADDRESS + pmap_rkva_count * NBPG;
	*vendp = VM_MAX_KERNEL_ADDRESS;

	size = round_page(vsize);
	npgs = atop(size);

	/*
	 * PA 0 will never be among those given to UVM so we can use it
	 * to indicate we couldn't steal any memory.
	 */
	for (ps = vm_physmem, bank = 0; bank < vm_nphysseg; bank++, ps++) {
		if (ps->free_list == VM_FREELIST_FIRST256 && 
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
		 * If this was the last one, then a very bad thing has occured
		 */
		if (--vm_nphysseg == 0)
			panic("pmap_steal_memory: out of memory!");

		printf("pmap_steal_memory: consumed bank %d\n", bank);
		for (; bank < vm_nphysseg; bank++, ps++) {
			ps[0] = ps[1];
		}
	}

	va = (vaddr_t) pa;
	memset((caddr_t) va, 0, size);
	pmap_pages_stolen += npgs;
#ifdef DEBUG
	if (pmapdebug && npgs > 1) {
		u_int cnt = 0;
		for (bank = 0, ps = vm_physmem; bank < vm_nphysseg; bank++, ps++)
			cnt += ps->avail_end - ps->avail_start;
		printf("pmap_steal_memory: stole %u (total %u) pages (%u left)\n",
		    npgs, pmap_pages_stolen, cnt);
	}
#endif

	return va;
}

/*
 * This is not part of the defined PMAP interface and is specific to the
 * PowerPC architecture.
 * This is called during initppc, before the system is really initialized.
 */
void
pmap_bootstrap(vaddr_t kernelstart, vaddr_t kernelend)
{
	struct mem_region *mp, *mp1;
	int cnt, i;
	u_int npgs = 0;
	u_int s, e, sz;

	/*
	 * Get memory.
	 */
	mem_regions(&mem, &avail);
#if defined(DEBUG)
	printf("pmap_bootstrap: memory configuration:\n");
	for (mp = mem; mp->size; mp++) {
		printf("pmap_bootstrap: mem start 0x%lx size 0x%lx\n",
			mp->start, mp->size);
	}
	for (mp = avail; mp->size; mp++) {
		printf("pmap_bootstrap: avail start 0x%lx size 0x%lx\n",
			mp->start, mp->size);
	}
#endif

	for (mp = mem; mp->size; mp++)
		physmem += btoc(mp->size);

	/*
	 * Count the number of available entries.
	 */
	for (cnt = 0, mp = avail; mp->size; mp++)
		cnt++;

	/*
	 * Page align all regions.
	 * Non-page aligned memory isn't very interesting to us.
	 * Also, sort the entries for ascending addresses.
	 */
	kernelstart &= ~PGOFSET;
	kernelend = (kernelend + PGOFSET) & ~PGOFSET;
	for (mp = avail; mp->size; mp++) {
		s = mp->start;
		e = mp->start + mp->size;
#if defined(ALLEGRO) && defined(discovery)
#define HOLESTART 0x1fa6000
#define HOLEEND 0x2000000
		/*
		 * Check whether this region holds all of
		 * the mysterious discovery memory hole
		 */
		if (s < HOLESTART && e > HOLEEND) {
			avail[cnt].start = HOLESTART;
			avail[cnt++].size = e - HOLEEND;
			e = HOLESTART;
		}
		/*
		 * Look whether this regions starts within 
		 * the mysterious discovery memory hole
		 */
		if (s >= HOLESTART && s < HOLEEND) {
			if (e <= HOLEEND)
				goto empty;
			s = HOLEEND;
		}
		/*
		 * Now look whether this region ends within
		 * the mysterious discovery memory hole
		 */
		if (e > HOLESTART && e <= HOLEEND) {
			if (s >= HOLESTART)
				goto empty;
			e = HOLESTART;
		}
#endif
		/*
		 * Check whether this region holds all of the kernel.
		 */
		if (s < kernelstart && e > kernelend) {
			avail[cnt].start = kernelend;
			avail[cnt++].size = e - kernelend;
			e = kernelstart;
		}
		/*
		 * Look whether this regions starts within the kernel.
		 */
		if (s >= kernelstart && s < kernelend) {
			if (e <= kernelend)
				goto empty;
			s = kernelend;
		}
		/*
		 * Now look whether this region ends within the kernel.
		 */
		if (e > kernelstart && e <= kernelend) {
			if (s >= kernelstart)
				goto empty;
			e = kernelstart;
		}
		/*
		 * Now page align the start and size of the region.
		 */
		s = round_page(s);
		e = trunc_page(e);
		if (e < s)
			e = s;
		sz = e - s;
		/*
		 * Check whether some memory is left here.
		 */
		if (sz == 0) {
		empty:
			bcopy(mp + 1, mp,
			      (cnt - (mp - avail)) * sizeof *mp);
			cnt--;
			mp--;
			continue;
		}
		/*
		 * Do an insertion sort.
		 */
		npgs += btoc(sz);
		for (mp1 = avail; mp1 < mp; mp1++)
			if (s < mp1->start)
				break;
		if (mp1 < mp) {
			bcopy(mp1, mp1 + 1, (char *)mp - (char *)mp1);
			mp1->start = s;
			mp1->size = sz;
		} else {
			mp->start = s;
			mp->size = sz;
		}
	}

#ifdef	HTABENTS
	pmap_pteg_cnt = HTABENTS;
#else /* HTABENTS */
	pmap_pteg_cnt = 1024;
	while ((pmap_pteg_cnt * sizeof(pteg_t) << 7) < ctob((u_int)physmem))
		pmap_pteg_cnt <<= 1;
#ifdef ALLEGRO
#ifndef discovery
	pmap_pteg_cnt <<= 1;		/* twice the minimum size */
#endif
#endif
#endif /* HTABENTS */

	/*
	 * Find suitably aligned memory for HTAB.
	 */
	for (mp = avail; mp->size; mp++) {
		s = roundup(mp->start, pmap_pteg_cnt * sizeof(pteg_t)) - mp->start;
		if (mp->size < s + pmap_pteg_cnt * sizeof(pteg_t))
			continue;
		pmap_pteg_table = (volatile pteg_t *)(mp->start + s);
#ifdef DIAGNOSTIC
		if ((((uintptr_t)pmap_pteg_table) + pmap_pteg_cnt * sizeof(pteg_t)) > SEGMENT_LENGTH)	/* sanity */
			panic("pmap_bootstrap: pmap_pteg_table end > 256MB");
#endif
		if (mp->size == s + pmap_pteg_cnt * sizeof(pteg_t)) {
			if (s)
				mp->size = s;
			else {
				bcopy(mp + 1, mp,
				      (cnt - (mp - avail)) * sizeof *mp);
				mp = avail;
			}
			break;
		}
		if (s != 0) {
			bcopy(mp, mp + 1,
			      (cnt - (mp - avail)) * sizeof *mp);
			mp++->size = s;
			cnt++;
		}
		mp->start += s + pmap_pteg_cnt * sizeof(pteg_t);
		mp->size -= s + pmap_pteg_cnt * sizeof(pteg_t);
		break;
	}
	if (!mp->size)
		panic("not enough memory?");

	npgs -= btoc(pmap_pteg_cnt * sizeof(pteg_t));
	bzero((void *)pmap_pteg_table, pmap_pteg_cnt * sizeof(pteg_t));
	pmap_pteg_mask = pmap_pteg_cnt - 1;

	/*
	 * We cannot do pmap_steal_memory here,
	 * since we don't run with translation enabled yet.
	 */
	s = sizeof(struct pvo_head) * pmap_pteg_cnt;
	sz = round_page(s);
	for (mp = avail; mp->size; mp++)
		if (mp->size >= sz)
			break;
	if (!mp->size)
		panic("not enough memory?");

	npgs -= btoc(sz);
	pmap_pvo_table = (struct pvo_head *)mp->start;
	mp->size -= sz;
	mp->start += sz;
	if (mp->size <= 0)
		bcopy(mp + 1, mp, (cnt - (mp - avail)) * sizeof *mp);
	for (i = 0; i < pmap_pteg_cnt; i++)
		LIST_INIT(&pmap_pvo_table[i]);

#ifndef MSGBUFADDR
	/*
	 * allow for msgbuf
	 */
	sz = round_page(MSGBUFSIZE);
	mp = NULL;
	for (mp1 = avail; mp1->size; mp1++)
		if (mp1->size >= sz)
			mp = mp1;
	if (mp == NULL)
		panic("not enough memory?");

	npgs -= btoc(sz);
	msgbuf_paddr = mp->start + mp->size - sz;
	mp->size -= sz;
	if (mp->size <= 0)
		bcopy(mp + 1, mp, (cnt - (mp - avail)) * sizeof *mp);
#endif


	sz = (sizeof(struct pvo_head *) + 1) * npgs;
	for (mp = avail; mp->size; mp++)
		if (mp->size >= sz)
			break;
	if (!mp->size)
		panic("pmap_bootstrap: not enough memory for (BAT) buffers");

#ifdef __HAVE_PMAP_PHYSSEG
	pmap_physseg.pvoh = (struct pvo_head *) mp->start;
	pmap_physseg.attrs = (char *) &pmap_physseg.pvoh[npgs];
	mp->size -= sz;
	mp->start += sz;
	if (mp->size == 0)
		bcopy(mp + 1, mp, (cnt - (mp - avail)) * sizeof *mp);
	if (((uintpr_t)pmap_physseg.pvoh + sz) > SEGMENT_LENGTH) /* sanity */
		panic("pmap_bootstrap: PVO list end > 256MB");
#endif

	for (mp = avail; mp->size; mp++) {
		paddr_t pfstart = atop(mp->start);
		paddr_t pfend = atop(mp->start + mp->size);
		if (mp->start + mp->size <= SEGMENT_LENGTH) {
			uvm_page_physload(pfstart, pfend, pfstart, pfend,
				VM_FREELIST_FIRST256);
		} else if (mp->start >= SEGMENT_LENGTH) {
			uvm_page_physload(pfstart, pfend, pfstart, pfend,
				VM_FREELIST_DEFAULT);
		} else {
			pfend = atop(SEGMENT_LENGTH);
			uvm_page_physload(pfstart, pfend, pfstart, pfend,
				VM_FREELIST_FIRST256);
			pfstart = atop(SEGMENT_LENGTH);
			pfend = atop(mp->start + mp->size);
			uvm_page_physload(pfstart, pfend, pfstart, pfend,
				VM_FREELIST_DEFAULT);
		}
	}

	/*
	 * Make sure kernel vsid is allocated as well as VSID 0.
	 */
	pmap_vsid_bitmap[(KERNEL_VSIDBITS / VSID_NBPW) & (NPMAPS-1)]
		|= 1 << (KERNEL_VSIDBITS % VSID_NBPW);
	pmap_vsid_bitmap[0] |= 1;

	/*
	 * Initialize kernel pmap and hardware.
	 */
	for (i = 0; i < 16; i++) {
		pmap_kernel()->pm_sr[i] = EMPTY_SEGMENT;
		__asm __volatile ("mtsrin %0,%1"
			      :: "r"(EMPTY_SEGMENT), "r"(i << ADDR_SR_SHFT));
	}

	pmap_kernel()->pm_sr[KERNEL_SR] = KERNEL_SEGMENT;
	__asm __volatile ("mtsr %0,%1"
		      :: "n"(KERNEL_SR), "r"(KERNEL_SEGMENT));
	__asm __volatile ("sync; mtsdr1 %0; isync"
		      :: "r"((u_int)pmap_pteg_table | (pmap_pteg_mask >> 10)));
	tlbia();

#ifdef DEBUG
	if (pmapdebug > 3) {
		u_int cnt;
		int bank;
		char pbuf[9];
		for (cnt = 0, bank = 0; bank < vm_nphysseg; bank++) {
			cnt += vm_physmem[bank].avail_end - vm_physmem[bank].avail_start;
			printf("pmap_bootstrap: vm_physmem[%d]=%#lx-%#lx/%#lx\n",
			    bank,
			    ptoa(vm_physmem[bank].avail_start),
			    ptoa(vm_physmem[bank].avail_end),
			    ptoa(vm_physmem[bank].avail_end - vm_physmem[bank].avail_start));
		}
		format_bytes(pbuf, sizeof(pbuf), ptoa((u_int64_t) cnt));
		printf("pmap_bootstrap: UVM memory = %s (%u pages)\n",
		    pbuf, cnt);
	}
#endif

	pool_init(&pmap_upvo_pool, sizeof(struct pvo_entry),
	    sizeof(struct pvo_entry), 0, 0, "pmap_upvopl", NBPG,
	    pmap_pool_ualloc, pmap_pool_ufree, M_VMPMAP);

	pool_setlowat(&pmap_upvo_pool, 252);

	pool_init(&pmap_pool, sizeof(struct pmap),
	    sizeof(void *), 0, 0, "pmap_pl", NBPG,
	    pmap_pool_ualloc, pmap_pool_ufree, M_VMPMAP);
}
