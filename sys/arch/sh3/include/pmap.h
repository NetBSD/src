/*	$NetBSD: pmap.h,v 1.21 2002/04/04 18:12:23 uch Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgment:
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
 * pmap.h: see pmap.c for the history of this pmap module.
 */

#ifndef _SH3_PMAP_H_
#define _SH3_PMAP_H_

#include <sh3/cache.h>
#include <sh3/psl.h>
#include <sh3/pte.h>
#include <uvm/uvm_object.h>

/*
 * see pte.h for a description of i386 MMU terminology and hardware
 * interface.
 *
 * a pmap describes a processes' 4GB virtual address space.  this
 * virtual address space can be broken up into 1024 4MB regions which
 * are described by PDEs in the PDP.  the PDEs are defined as follows:
 *
 * (ranges are inclusive -> exclusive, just like vm_map_entry start/end)
 * (the following assumes that KERNBASE is 0xf0000000)
 *
 * PDE#s	VA range		usage
 * 0->959	0x0 -> 0xefc00000	user address space, note that the
 *					max user address is 0xefbfe000
 *					the final two pages in the last 4MB
 *					used to be reserved for the UAREA
 *					but now are no longer used
 * 959		0xefc00000->		recursive mapping of PDP (used for
 *			0xf0000000	linear mapping of PTPs)
 * 960->1023	0xf0000000->		kernel address space (constant
 *			0xffc00000	across all pmap's/processes)
 * 1023		0xffc00000->		"alternate" recursive PDP mapping
 *			<end>		(for other pmaps)
 *
 *
 * note: a recursive PDP mapping provides a way to map all the PTEs for
 * a 4GB address space into a linear chunk of virtual memory.  in other
 * words, the PTE for page 0 is the first int mapped into the 4MB recursive
 * area.  the PTE for page 1 is the second int.  the very last int in the
 * 4MB range is the PTE that maps VA 0xffffe000 (the last page in a 4GB
 * address).
 *
 * all pmap's PD's must have the same values in slots 960->1023 so that
 * the kernel is always mapped in every process.  these values are loaded
 * into the PD at pmap creation time.
 *
 * at any one time only one pmap can be active on a processor.  this is
 * the pmap whose PDP is pointed to by processor register %cr3.  this pmap
 * will have all its PTEs mapped into memory at the recursive mapping
 * point (slot #959 as show above).  when the pmap code wants to find the
 * PTE for a virtual address, all it has to do is the following:
 *
 * address of PTE = (959 * 4MB) + (VA / NBPG) * sizeof(pt_entry_t)
 *                = 0xefc00000 + (VA / 4096) * 4
 *
 * what happens if the pmap layer is asked to perform an operation
 * on a pmap that is not the one which is currently active?  in that
 * case we take the PA of the PDP of non-active pmap and put it in
 * slot 1023 of the active pmap.  this causes the non-active pmap's
 * PTEs to get mapped in the final 4MB of the 4GB address space
 * (e.g. starting at 0xffc00000).
 *
 * the following figure shows the effects of the recursive PDP mapping:
 *
 *   PDP (%cr3)
 *   +----+
 *   |   0| -> PTP#0 that maps VA 0x0 -> 0x400000
 *   |    |
 *   |    |
 *   | 959| -> points back to PDP (%cr3) mapping VA 0xefc00000 -> 0xf0000000
 *   | 960| -> first kernel PTP (maps 0xf0000000 -> 0xf0400000)
 *   |    |
 *   |1023| -> points to alternate pmap's PDP (maps 0xffc00000 -> end)
 *   +----+
 *
 * note that the PDE#959 VA (0xefc00000) is defined as "PTE_BASE"
 * note that the PDE#1023 VA (0xffc00000) is defined as "APTE_BASE"
 *
 * starting at VA 0xefc00000 the current active PDP (%cr3) acts as a
 * PTP:
 *
 * PTP#959 == PDP(%cr3) => maps VA 0xefc00000 -> 0xf0000000
 *   +----+
 *   |   0| -> maps the contents of PTP#0 at VA 0xefc00000->0xefc01000
 *   |    |
 *   |    |
 *   | 959| -> maps contents of PTP#959 (the PDP) at VA 0xeffbf000
 *   | 960| -> maps contents of first kernel PTP
 *   |    |
 *   |1023|
 *   +----+
 *
 * note that mapping of the PDP at PTP#959's VA (0xeffbf000) is
 * defined as "PDP_BASE".... within that mapping there are two
 * defines:
 *   "PDP_PDE" (0xeffbfefc) is the VA of the PDE in the PDP
 *      which points back to itself.
 *   "APDP_PDE" (0xeffbfffc) is the VA of the PDE in the PDP which
 *      establishes the recursive mapping of the alternate pmap.
 *      to set the alternate PDP, one just has to put the correct
 *	PA info in *APDP_PDE.
 *
 * note that in the APTE_BASE space, the APDP appears at VA
 * "APDP_BASE" (0xfffff000).
 */

/*
 * the following defines identify the slots used as described above.
 */

#define PDSLOT_PTE	((u_int)0x33f)	/* PTDPTDI for recursive PDP map */
#define PDSLOT_KERN	((u_int)0x340)	/* KPTDI start of kernel space */
#define PDSLOT_APTE	((u_int)0x37f)	/* alternative recursive slot */

/*
 * the following defines give the virtual addresses of various MMU
 * data structures:
 * PTE_BASE and APTE_BASE: the base VA of the linear PTE mappings
 * PTD_BASE and APTD_BASE: the base VA of the recursive mapping of the PTD
 * PDP_PDE and APDP_PDE: the VA of the PDE that points back to the PDP/APDP
 */

#define PTE_BASE	((pt_entry_t *)  (PDSLOT_PTE * NBPD) )
#define APTE_BASE	((pt_entry_t *)  (PDSLOT_APTE * NBPD) )
#define PDP_BASE ((pd_entry_t *)(((char *)PTE_BASE) + (PDSLOT_PTE * NBPG)))
#define APDP_BASE ((pd_entry_t *)(((char *)APTE_BASE) + (PDSLOT_APTE * NBPG)))
#define PDP_PDE		(PDP_BASE + PDSLOT_PTE)
#define APDP_PDE	(PDP_BASE + PDSLOT_APTE)

/*
 * XXXCDC: tmp xlate from old names:
 * PTDPTDI -> PDSLOT_PTE
 * KPTDI -> PDSLOT_KERN
 * APTDPTDI -> PDSLOT_APTE
 */

/*
 * the follow define determines how many PTPs should be set up for the
 * kernel by locore.s at boot time.  this should be large enough to
 * get the VM system running.  once the VM system is running, the
 * pmap module can add more PTPs to the kernel area on demand.
 */

#ifndef NKPTP
#define NKPTP		8	/* 32MB to start */
#endif
#define NKPTP_MIN	8	/* smallest value we allow */
#define NKPTP_MAX	63	/* (1024 - (0xd0000000/NBPD) - 1) */
				/* largest value (-1 for APTP space) */

/*
 * various address macros
 *
 *  vtopte: return a pointer to the PTE mapping a VA
 *  kvtopte: same as above (takes a KVA, but doesn't matter with this pmap)
 *  ptetov: given a pointer to a PTE, return the VA that it maps
 *  vtophys: translate a VA to the PA mapped to it
 *
 * plus alternative versions of the above
 */

#define vtopte(VA)	(PTE_BASE + sh3_btop(VA))
#define kvtopte(VA)	vtopte(VA)
#define ptetov(PT)	(sh3_ptob(PT - PTE_BASE))
#define avtopte(VA)	(APTE_BASE + sh3_btop(VA))
#define ptetoav(PT)	(sh3_ptob(PT - APTE_BASE))
#define avtophys(VA)	((*avtopte(VA) & PG_FRAME) | \
			 ((unsigned)(VA) & ~PG_FRAME))

/*
 * pdei/ptei: generate index into PDP/PTP from a VA
 */
#define	pdei(VA)	(((VA) & PD_MASK) >> PDSHIFT)
#define	ptei(VA)	(((VA) & PT_MASK) >> PGSHIFT)

/*
 * PTP macros:
 *   a PTP's index is the PD index of the PDE that points to it
 *   a PTP's offset is the byte-offset in the PTE space that this PTP is at
 *   a PTP's VA is the first VA mapped by that PTP
 *
 * note that NBPG == number of bytes in a PTP (4096 bytes == 1024 entries)
 *           NBPD == number of bytes a PTP can map (4MB)
 */

#define ptp_i2o(I)	((I) * NBPG)	/* index => offset */
#define ptp_o2i(O)	((O) / NBPG)	/* offset => index */
#define ptp_i2v(I)	((I) * NBPD)	/* index => VA */
#define ptp_v2i(V)	((V) / NBPD)	/* VA => index (same as pdei) */

#ifdef _KERNEL
/*
 * pmap data structures: see pmap.c for details of locking.
 */

struct pmap;
typedef struct pmap *pmap_t;

/*
 * we maintain a list of all non-kernel pmaps
 */

LIST_HEAD(pmap_head, pmap); /* struct pmap_head: head of a pmap list */

/*
 * the pmap structure
 *
 * note that the pm_obj contains the simple_lock, the reference count,
 * page list, and number of PTPs within the pmap.
 */

struct pmap {
	struct uvm_object pm_obj;	/* object (lck by object lock) */
#define	pm_lock	pm_obj.vmobjlock
	LIST_ENTRY(pmap) pm_list;	/* list (lck by pm_list lock) */
	pd_entry_t *pm_pdir;		/* VA of PD (lck by object lock) */
	u_int32_t pm_pdirpa;		/* PA of PD (read-only after create) */
	struct vm_page *pm_ptphint;	/* pointer to a PTP in our pmap */
	struct pmap_statistics pm_stats;  /* pmap stats (lck by object lock) */

	int pm_flags;			/* see below */
};

/* pm_flags */
#define	PMF_USER_LDT	0x01	/* pmap has user-set LDT */

/*
 * for each managed physical page we maintain a list of <PMAP,VA>'s
 * which it is mapped at.  the list is headed by a pv_head structure.
 * there is one pv_head per managed phys page (allocated at boot time).
 * the pv_head structure points to a list of pv_entry structures (each
 * describes one mapping).
 */

struct pv_entry;

struct pv_head {
	struct simplelock pvh_lock;	/* locks every pv on this list */
	struct pv_entry *pvh_list;	/* head of list (locked by pvh_lock) */
};

/* These are kept in the vm_physseg array. */
#define	PGA_REFERENCED	0x01		/* page is referenced */
#define	PGA_MODIFIED	0x02		/* page is modified */

struct pv_entry {			/* locked by its list's pvh_lock */
	struct pv_entry *pv_next;	/* next entry */
	struct pmap *pv_pmap;		/* the pmap */
	vaddr_t pv_va;			/* the virtual address */
	struct vm_page *pv_ptp;		/* the vm_page of the PTP */
};

/*
 * pv_entrys are dynamically allocated in chunks from a single page.
 * we keep track of how many pv_entrys are in use for each page and
 * we can free pv_entry pages if needed.  there is one lock for the
 * entire allocation system.
 */

struct pv_page_info {
	TAILQ_ENTRY(pv_page) pvpi_list;
	struct pv_entry *pvpi_pvfree;
	int pvpi_nfree;
};

/*
 * number of pv_entry's in a pv_page
 * (note: won't work on systems where NPBG isn't a constant)
 */

#define PVE_PER_PVPAGE ((NBPG - sizeof(struct pv_page_info)) / \
			sizeof(struct pv_entry))

/*
 * a pv_page: where pv_entrys are allocated from
 */

struct pv_page {
	struct pv_page_info pvinfo;
	struct pv_entry pvents[PVE_PER_PVPAGE];
};

/*
 * pmap_remove_record: a record of VAs that have been unmapped, used to
 * flush TLB.  if we have more than PMAP_RR_MAX then we stop recording.
 */

#define PMAP_RR_MAX	16	/* max of 16 pages (64K) */

struct pmap_remove_record {
	int prr_npages;
	vaddr_t prr_vas[PMAP_RR_MAX];
};

/*
 * global kernel variables
 */

/* PTDpaddr: is the physical address of the kernel's PDP */
extern u_long PTDpaddr;

extern struct pmap kernel_pmap_store;	/* kernel pmap */
extern int nkpde;			/* current # of PDEs for kernel */
extern int pmap_pg_g;			/* do we support PG_G? */

/*
 * macros
 */

#define	pmap_kernel()			(&kernel_pmap_store)
#define	pmap_resident_count(pmap)	((pmap)->pm_stats.resident_count)
#define	pmap_wired_count(pmap)		((pmap)->pm_stats.wired_count)
#define	pmap_update(pmap)		/* nothing (yet) */

#define	pmap_is_referenced(pg)		pmap_test_attrs(pg, PGA_REFERENCED)
#define	pmap_is_modified(pg)		pmap_test_attrs(pg, PGA_MODIFIED)

#define pmap_copy(DP,SP,D,L,S)
#define pmap_move(DP,SP,D,L,S)
#define pmap_phys_address(ppn)		sh3_ptob(ppn)
#define pmap_valid_entry(E) 		((E) & PG_V) /* is PDE or PTE valid? */


/*
 * prototypes
 */

void		pmap_activate(struct proc *);
void		pmap_bootstrap(vaddr_t);
boolean_t	pmap_change_attrs(struct vm_page *, int, int);
void		pmap_deactivate(struct proc *);
void		pmap_page_remove (struct vm_page *);
void		pmap_protect(struct pmap *, vaddr_t,
				vaddr_t, vm_prot_t);
void		pmap_remove(struct pmap *, vaddr_t, vaddr_t);
boolean_t	pmap_test_attrs(struct vm_page *, int);
void		pmap_update_pg(vaddr_t);
void		pmap_update_2pg(vaddr_t,vaddr_t);
void		pmap_write_protect(struct pmap *, vaddr_t,
				vaddr_t, vm_prot_t);

vaddr_t reserve_dumppages(vaddr_t); /* XXX: not a pmap fn */

#define PMAP_GROWKERNEL		/* turn on pmap_growkernel interface */

/*
 * Alternate mapping hooks for pool pages.  Avoids thrashing the TLB.
 */
/*
 * XXX Indeed, first, we should refine physical address v.s. virtual
 *	address mapping.
 * See
 *	uvm_km.c:uvm_km_free_poolpage1,
 *	vm_page.h:PHYS_TO_VM_PAGE, vm_physseg_find
 *	machdep.c:pmap_bootstrap (uvm_page_physload, etc)
 */
/* XXX broken */
#define PMAP_MAP_POOLPAGE(pa)	(pa)
#define PMAP_UNMAP_POOLPAGE(va)	(va)

vaddr_t pmap_map(vaddr_t, paddr_t, paddr_t, vm_prot_t);
paddr_t vtophys(vaddr_t);
void pmap_emulate_reference(struct proc *, vaddr_t, int, int);

#endif /* _KERNEL */
#endif /* _SH3_PMAP_H_ */
