/*	$NetBSD: pmap.new.h,v 1.1 1996/07/09 22:22:27 cgd Exp $	*/

/*
 * Copyright (c) 1992, 1993, 1996 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

/*
 *	File:	pmap.h
 *
 *	Author: David Golub (mods for Alpha by Alessandro Forin)
		Mods for use in NetBSD/Alpha by Chris Demetriou.
 *	Date:	1988 ca.
 *
 *	Machine-dependent structures for the physical map module.
 */

#ifndef	_PMAP_MACHINE_
#define _PMAP_MACHINE_	1

#ifndef	ASSEMBLER

#include <kern/zalloc.h>
#include <kern/lock.h>
#include <mach/machine/vm_param.h>
#include <mach/vm_statistics.h>
#include <mach/kern_return.h>

/*
 *	Alpha Page Table Entry
 */

typedef unsigned long	pt_entry_t;
#define PT_ENTRY_NULL	((pt_entry_t *) 0)

#endif	ASSEMBLER

#define ALPHA_OFFMASK	(ALPHA_PGBYTES-1)	/* offset within page */

#define	SEG_MASK	((ALPHA_PGBYTES / 8)-1)	/* masks for segments */
#define	SEG3_SHIFT	(ALPHA_PGSHIFT)		/* shifts for segments */
#define	SEG2_SHIFT	(SEG3_SHIFT+(ALPHA_PGSHIFT-3))
#define	SEG1_SHIFT	(SEG2_SHIFT+(ALPHA_PGSHIFT-3))

/*
 *	Convert address offset to page descriptor index
 */
#define pdenum(a)	(((a) >> SEG1_SHIFT) & SEG_MASK)

/*
 *	Convert page descriptor index to user virtual address
 */
#define pdetova(a)	((vm_offset_t)(a) << SEG1_SHIFT)
#define pde2tova(a)	((vm_offset_t)(a) << SEG2_SHIFT)
#define pde3tova(a)	((vm_offset_t)(a) << SEG3_SHIFT)

/*
 *	Convert address offset to second level page table index
 */
#define pte2num(a)	(((a) >> SEG2_SHIFT) & SEG_MASK)

/*
 *	Convert address offset to third level page table index
 */
#define pte3num(a)	(((a) >> SEG3_SHIFT) & SEG_MASK)

#define NPTES	(alpha_ptob(1)/sizeof(pt_entry_t))
#define NPDES	(alpha_ptob(1)/sizeof(pt_entry_t))

/*
 *	Hardware pte bit definitions (to be used directly on the ptes
 *	without using the bit fields).
 */

#define	ALPHA_PTE_VALID		0x1

#define	ALPHA_PTE_FAULT_ON_x	0xe

#define	ALPHA_PTE_GLOBAL	0x10
#define	ALPHA_PTE_GRANULARITY	0x60

#define	ALPHA_PTE_PROT		0xff00
#define	ALPHA_PTE_PROTOFF	8
#define	ALPHA_PTE_KW		0x10
#define	ALPHA_PTE_UW		0x80
#define	ALPHA_PTE_KR		0x01
#define	ALPHA_PTE_UR		0x08

#define ALPHA_PTE_WRITE		0x00009000

#define	ALPHA_PTE_SOFTWARE	0xffff0000
#define ALPHA_PTE_WIRED		0x00010000
#define ALPHA_PTE_REF		0x00020000
#define ALPHA_PTE_MOD		0x00040000

#define ALPHA_PTE_PFN		0xffffffff00000000

#define	pa_to_pte(a)		(alpha_btop(a) << 32)
#define	pte_to_pa(p)		(alpha_ptob( (p) >> 32 ))
#define	pte_increment_pa(p)	((p) += pa_to_pte(ALPHA_PGBYTES))

/*
 *	Convert page table entry to kernel virtual address
 */
#define ptetokv(a)	(phystokv(pte_to_pa(a)))

#ifndef	ASSEMBLER
typedef	volatile long	cpu_set;	/* set of CPUs - must be <= 64 */
					/* changed by other processors */

struct pmap {
	pt_entry_t	*dirbase;	/* page directory pointer register */
	int		pid;		/* TLBPID when in use		*/
	int		ref_count;	/* reference count */
	decl_simple_lock_data(,lock)
					/* lock on map */
	struct pmap_statistics	stats;	/* map statistics */
	cpu_set		cpus_using;	/* bitmap of cpus using pmap */
	int		(*hacking)();	/* horrible things needed	*/
};

typedef struct pmap	*pmap_t;

#define PMAP_NULL	((pmap_t) 0)

extern vm_offset_t	kvtophys(vm_offset_t);
extern void		set_ptbr(/* pmap_t map, pcb_t pcb */);

#if	NCPUS > 1
/*
 *	List of cpus that are actively using mapped memory.  Any
 *	pmap update operation must wait for all cpus in this list.
 *	Update operations must still be queued to cpus not in this
 *	list.
 */
extern cpu_set		cpus_active;

/*
 *	List of cpus that are idle, but still operating, and will want
 *	to see any kernel pmap updates when they become active.
 */
extern cpu_set		cpus_idle;

/*
 *	Quick test for pmap update requests.
 */
extern volatile
boolean_t	cpu_update_needed[NCPUS];

/*
 *	External declarations for PMAP_ACTIVATE.
 */

void		process_pmap_updates();
void		pmap_update_interrupt();
extern	pmap_t	kernel_pmap;

#endif	NCPUS > 1

/*
 *	Machine dependent routines that are used only for Alpha.
 */

pt_entry_t	*pmap_pte();

/*
 *	Macros for speed.
 */

#if	NCPUS > 1

/*
 *	For multiple CPUS, PMAP_ACTIVATE and PMAP_DEACTIVATE must manage
 *	fields to control TLB invalidation on other CPUS.
 */

#define	PMAP_ACTIVATE_KERNEL(my_cpu)	{				\
									\
	/*								\
	 *	Let pmap updates proceed while we wait for this pmap.	\
	 */								\
	i_bit_clear((my_cpu), &cpus_active);				\
									\
	/*								\
	 *	Lock the pmap to put this cpu in its active set.	\
	 *	Wait for updates here.					\
	 */								\
	simple_lock(&kernel_pmap->lock);				\
									\
	/*								\
	 *	Process invalidate requests for the kernel pmap.	\
	 */								\
	if (cpu_update_needed[(my_cpu)])				\
	    process_pmap_updates(kernel_pmap);				\
									\
	/*								\
	 *	Mark that this cpu is using the pmap.			\
	 */								\
	i_bit_set((my_cpu), &kernel_pmap->cpus_using);			\
									\
	/*								\
	 *	Mark this cpu active - IPL will be lowered by		\
	 *	load_context().						\
	 */								\
	i_bit_set((my_cpu), &cpus_active);				\
									\
	simple_unlock(&kernel_pmap->lock);				\
}

#define	PMAP_DEACTIVATE_KERNEL(my_cpu)	{				\
	/*								\
	 *	Mark pmap no longer in use by this cpu even if		\
	 *	pmap is locked against updates.				\
	 */								\
	i_bit_clear((my_cpu), &kernel_pmap->cpus_using);		\
}

#define PMAP_ACTIVATE_USER(pmap, th, my_cpu)	{			\
	register pmap_t		tpmap = (pmap);				\
	register pcb_t		pcb = (th)->pcb;			\
									\
	if (tpmap == kernel_pmap) {					\
	    /*								\
	     *	If this is the kernel pmap, switch to its page tables.	\
	     */								\
	    set_ptbr(tpmap,pcb,TRUE);					\
	}								\
	else {								\
	    /*								\
	     *	Let pmap updates proceed while we wait for this pmap.	\
	     */								\
	    i_bit_clear((my_cpu), &cpus_active);			\
									\
	    /*								\
	     *	Lock the pmap to put this cpu in its active set.	\
	     *	Wait for updates here.					\
	     */								\
	    simple_lock(&tpmap->lock);					\
									\
	    /*								\
	     *	No need to invalidate the TLB - the entire user pmap	\
	     *	will be invalidated by reloading dirbase.		\
	     */								\
	    if (tpmap->pid < 0) pmap_assign_tlbpid(tpmap);		\
	    set_ptbr(tpmap, pcb, TRUE);					\
									\
	    /*								\
	     *	Mark that this cpu is using the pmap.			\
	     */								\
	    i_bit_set((my_cpu), &tpmap->cpus_using);			\
									\
	    /*								\
	     *	Mark this cpu active - IPL will be lowered by		\
	     *	load_context().						\
	     */								\
	    i_bit_set((my_cpu), &cpus_active);				\
									\
	    simple_unlock(&tpmap->lock);				\
	}								\
}

#define PMAP_DEACTIVATE_USER(pmap, thread, my_cpu)	{		\
	register pmap_t		tpmap = (pmap);				\
									\
	/*								\
	 *	Do nothing if this is the kernel pmap.			\
	 */								\
	if (tpmap != kernel_pmap) {					\
	    /*								\
	     *	Mark pmap no longer in use by this cpu even if		\
	     *	pmap is locked against updates.				\
	     */								\
	    i_bit_clear((my_cpu), &(pmap)->cpus_using);			\
	}								\
}

#define MARK_CPU_IDLE(my_cpu)	{					\
	/*								\
	 *	Mark this cpu idle, and remove it from the active set,	\
	 *	since it is not actively using any pmap.  Signal_cpus	\
	 *	will notice that it is idle, and avoid signaling it,	\
	 *	but will queue the update request for when the cpu	\
	 *	becomes active.						\
	 */								\
	spl_t	s = splvm();						\
	i_bit_set((my_cpu), &cpus_idle);				\
	i_bit_clear((my_cpu), &cpus_active);				\
	splx(s);							\
}

#define MARK_CPU_ACTIVE(my_cpu)	{					\
									\
	spl_t	s = splvm();						\
	/*								\
	 *	If a kernel_pmap update was requested while this cpu	\
	 *	was idle, process it as if we got the interrupt.	\
	 *	Before doing so, remove this cpu from the idle set.	\
	 *	Since we do not grab any pmap locks while we flush	\
	 *	our TLB, another cpu may start an update operation	\
	 *	before we finish.  Removing this cpu from the idle	\
	 *	set assures that we will receive another update		\
	 *	interrupt if this happens.				\
	 */								\
	i_bit_clear((my_cpu), &cpus_idle);				\
									\
	if (cpu_update_needed[(my_cpu)])				\
	    pmap_update_interrupt();					\
									\
	/*								\
	 *	Mark that this cpu is now active.			\
	 */								\
	i_bit_set((my_cpu), &cpus_active);				\
	splx(s);							\
}

#else	NCPUS > 1

/*
 *	With only one CPU, we just have to indicate whether the pmap is
 *	in use.
 */

#define	PMAP_ACTIVATE_KERNEL(my_cpu)	{				\
	kernel_pmap->cpus_using = TRUE;					\
}

#define	PMAP_DEACTIVATE_KERNEL(my_cpu)	{				\
	kernel_pmap->cpus_using = FALSE;				\
}

#define	PMAP_ACTIVATE_USER(pmap, th, my_cpu)	{			\
	register pmap_t		tpmap = (pmap);				\
	register pcb_t		pcb = (th)->pcb;			\
									\
	if (tpmap->pid < 0) pmap_assign_tlbpid(tpmap);			\
	set_ptbr(tpmap,pcb,TRUE);					\
	if (tpmap != kernel_pmap) {					\
	    tpmap->cpus_using = TRUE;					\
	}								\
}

#define PMAP_DEACTIVATE_USER(pmap, thread, cpu)	{			\
	if ((pmap) != kernel_pmap)					\
	    (pmap)->cpus_using = FALSE;					\
}

#endif	NCPUS > 1

#define	pmap_kernel()			(kernel_pmap)
#define pmap_resident_count(pmap)	((pmap)->stats.resident_count)
#define pmap_phys_address(frame)	((vm_offset_t) (alpha_ptob(frame)))
#define pmap_phys_to_frame(phys)	((alpha_btop(phys)))
#define	pmap_copy(dst_pmap,src_pmap,dst_addr,len,src_addr)
#define	pmap_attribute(pmap,addr,size,attr,value) \
					(KERN_INVALID_ADDRESS)

/*
 *	Data structures this module exports
 */
extern pmap_t		kernel_pmap;	/* pointer to the kernel pmap	*/


#endif	ASSEMBLER

/*
 *	We want to implement pmap_steal_memory and pmap_startup.
 */

#define	MACHINE_PAGES

#endif	_PMAP_MACHINE_
