/*	$NetBSD: pmap.h,v 1.4.4.2 2001/03/12 13:27:25 bouyer Exp $	*/

/*
 * Copyright (c) 1994,1995 Mark Brinicombe.
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

#ifndef	_ARM32_PMAP_H_
#define	_ARM32_PMAP_H_

#include <machine/cpufunc.h>
#include <machine/pte.h>

/*
 * Data structures used by pmap
 */

/*
 * Structure that describes a Level 1 page table and the flags
 * associated with it.
 */
struct l1pt {
	SIMPLEQ_ENTRY(l1pt)	pt_queue;	/* Queue pointers */
	struct pglist		pt_plist;	/* Allocated page list */
	vaddr_t			pt_va;		/* Allocated virtual address */
	int	                pt_flags;	/* Flags */
};
#define	PTFLAG_STATIC		1		/* Statically allocated */
#define PTFLAG_KPT		2		/* Kernel pt's are mapped */
#define PTFLAG_CLEAN		4		/* L1 is clean */

/*
 * The pmap structure itself.
 */
struct pmap {
	pd_entry_t		*pm_pdir;	/* KVA of page directory */
	struct l1pt		*pm_l1pt;	/* L1 descriptor */
	void			*pm_unused1;	/* Reserved for l2 map */
	paddr_t			pm_pptpt;	/* PA of pt's page table */
	vaddr_t			pm_vptpt;	/* VA of pt's page table */
	short			pm_dref;	/* page directory ref count */
	short			pm_count;	/* pmap reference count */
	simple_lock_data_t	pm_lock;	/* lock on pmap */
	struct pmap_statistics	pm_stats;	/* pmap statistics */
};

typedef struct pmap *pmap_t;

/*
 * For each vm_page_t, there is a list of all currently valid virtual
 * mappings of that page.  An entry is a pv_entry_t, the list is pv_table.
 */
typedef struct pv_entry {
	struct pv_entry *pv_next;       /* next pv_entry */
	pmap_t          pv_pmap;        /* pmap where mapping lies */
	vaddr_t         pv_va;          /* virtual address for mapping */
	int             pv_flags;       /* flags */
} *pv_entry_t;

/*
 * A pv_page_info struture looks like this. It is used to contain status
 * information for pv_entry freelists.
 */
struct pv_page;

struct pv_page_info {
	TAILQ_ENTRY(pv_page) pgi_list;
	struct pv_entry *pgi_freelist;
	int pgi_nfree;
};

/*
 * A pv_page itself looks like this. pv_entries are requested from the VM a
 * pv_page at a time.
 *
 * We also define a macro that states the number of pv_entries per page
 * allocated.
 */
#define NPVPPG	((NBPG - sizeof(struct pv_page_info)) / sizeof(struct pv_entry))

struct pv_page {
	struct pv_page_info pvp_pgi;
	struct pv_entry pvp_pv[NPVPPG];
};

/*
 * Page hooks. I'll eliminate these sometime soon :-)
 *
 * For speed we store the both the virtual address and the page table
 * entry address for each page hook.
 */
typedef struct {
        vaddr_t va;
        pt_entry_t *pte;
} pagehook_t;

/*
 * Physical / virtual address structure. In a number of places (particularly
 * during bootstrapping) we need to keep track of the physical and virtual
 * addresses of various pages
 */
typedef struct {
	paddr_t pv_pa;
	vaddr_t pv_va;
} pv_addr_t;

/*
 * _KERNEL specific macros, functions and prototypes
 */

#ifdef  _KERNEL

/*
 * Commonly referenced structures
 */
extern pv_entry_t	pv_table;	/* Phys to virt mappings, per page. */
extern pmap_t		kernel_pmap;	/* pmap pointer used for the kernel */
extern struct pmap	kernel_pmap_store;  /* kernel_pmap points to this */
extern int		pmap_debug_level; /* Only exists if PMAP_DEBUG */

/*
 * Macros that we need to export
 */
#define pmap_kernel()			(&kernel_pmap_store)
#define pmap_update()			/*cpu_tlb_flushID()*/
#define	pmap_resident_count(pmap)	((pmap)->pm_stats.resident_count)
#define	pmap_wired_count(pmap)		((pmap)->pm_stats.wired_count)

#define pmap_phys_address(ppn)		(arm_page_to_byte((ppn)))

/*
 * Functions that we need to export
 */
extern boolean_t pmap_testbit __P((paddr_t, int));
extern void pmap_changebit __P((paddr_t, int, int));
extern vaddr_t pmap_map __P((vaddr_t, vaddr_t, vaddr_t, int));
extern void pmap_procwr __P((struct proc *, vaddr_t, int));
#define	PMAP_NEED_PROCWR

/*
 * Functions we use internally
 */
extern void pmap_bootstrap __P((pd_entry_t *, pv_addr_t));
extern void pmap_debug	__P((int));
extern int pmap_handled_emulation __P((pmap_t, vaddr_t));
extern int pmap_modified_emulation __P((pmap_t, vaddr_t));
extern void pmap_postinit __P((void));
extern pt_entry_t *pmap_pte __P((pmap_t, vaddr_t));

#endif	/* _KERNEL */

/*
 * Useful macros and constants 
 */

/* Virtual address to page table entry */
#define vtopte(va) \
	((pt_entry_t *)(PROCESS_PAGE_TBLS_BASE + \
	(arm_byte_to_page((unsigned int)(va)) << 2)))

/* Virtual address to physical address */
#define vtophys(va) \
	((*vtopte(va) & PG_FRAME) | ((unsigned int)(va) & ~PG_FRAME))

/* L1 and L2 page table macros */
#define pmap_pde(m, v) (&((m)->pm_pdir[((vaddr_t)(v) >> PDSHIFT)&4095]))
#define pmap_pte_pa(pte)	(*(pte) & PG_FRAME)
#define pmap_pde_v(pde)		(*(pde) != 0)
#define pmap_pte_v(pte)		(*(pte) != 0)

/* Size of the kernel part of the L1 page table */
#define KERNEL_PD_SIZE	\
	(PD_SIZE - (KERNEL_SPACE_START >> PDSHIFT) * sizeof(pd_entry_t))

#endif	/* _ARM32_PMAP_H_ */
