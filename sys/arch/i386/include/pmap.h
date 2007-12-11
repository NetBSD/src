/*	$NetBSD: pmap.h,v 1.97.6.1 2007/12/11 23:02:56 bouyer Exp $	*/

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
 * Copyright (c) 2001 Wasabi Systems, Inc.
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

#ifndef	_I386_PMAP_H_
#define	_I386_PMAP_H_

#if defined(_KERNEL_OPT)
#include "opt_user_ldt.h"
#include "opt_xen.h"
#endif

#include <sys/atomic.h>

#include <machine/pte.h>
#include <machine/segments.h>
#if defined(_KERNEL)
#include <machine/cpufunc.h>
#endif

#include <uvm/uvm_object.h>
#ifdef XEN
#include <xen/xenfunc.h>
#include <xen/xenpmap.h>
#endif /* XEN */

/*
 * see pte.h for a description of i386 MMU terminology and hardware
 * interface.
 *
 * a pmap describes a processes' 4GB virtual address space.  this
 * virtual address space can be broken up into 1024 4MB regions which
 * are described by PDEs in the PDP.  the PDEs are defined as follows:
 *
 * (ranges are inclusive -> exclusive, just like vm_map_entry start/end)
 * (the following assumes that KERNBASE is 0xc0000000)
 *
 * PDE#s	VA range		usage
 * 0->766	0x0 -> 0xbfc00000	user address space
 * 767		0xbfc00000->		recursive mapping of PDP (used for
 *			0xc0000000	linear mapping of PTPs)
 * 768->1023	0xc0000000->		kernel address space (constant
 *			0xffc00000	across all pmap's/processes)
 * 1023		0xffc00000->		"alternate" recursive PDP mapping
 *			<end>		(for other pmaps)
 *
 *
 * note: a recursive PDP mapping provides a way to map all the PTEs for
 * a 4GB address space into a linear chunk of virtual memory.  in other
 * words, the PTE for page 0 is the first int mapped into the 4MB recursive
 * area.  the PTE for page 1 is the second int.  the very last int in the
 * 4MB range is the PTE that maps VA 0xfffff000 (the last page in a 4GB
 * address).
 *
 * all pmap's PD's must have the same values in slots 768->1023 so that
 * the kernel is always mapped in every process.  these values are loaded
 * into the PD at pmap creation time.
 *
 * at any one time only one pmap can be active on a processor.  this is
 * the pmap whose PDP is pointed to by processor register %cr3.  this pmap
 * will have all its PTEs mapped into memory at the recursive mapping
 * point (slot #767 as show above).  when the pmap code wants to find the
 * PTE for a virtual address, all it has to do is the following:
 *
 * address of PTE = (767 * 4MB) + (VA / PAGE_SIZE) * sizeof(pt_entry_t)
 *                = 0xbfc00000 + (VA / 4096) * 4
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
 *   | 767| -> points back to PDP (%cr3) mapping VA 0xbfc00000 -> 0xc0000000
 *   | 768| -> first kernel PTP (maps 0xc0000000 -> 0xc0400000)
 *   |    |
 *   |1023| -> points to alternate pmap's PDP (maps 0xffc00000 -> end)
 *   +----+
 *
 * note that the PDE#767 VA (0xbfc00000) is defined as "PTE_BASE"
 * note that the PDE#1023 VA (0xffc00000) is defined as "APTE_BASE"
 *
 * starting at VA 0xbfc00000 the current active PDP (%cr3) acts as a
 * PTP:
 *
 * PTP#767 == PDP(%cr3) => maps VA 0xbfc00000 -> 0xc0000000
 *   +----+
 *   |   0| -> maps the contents of PTP#0 at VA 0xbfc00000->0xbfc01000
 *   |    |
 *   |    |
 *   | 767| -> maps contents of PTP#767 (the PDP) at VA 0xbfeff000
 *   | 768| -> maps contents of first kernel PTP
 *   |    |
 *   |1023|
 *   +----+
 *
 * note that mapping of the PDP at PTP#767's VA (0xbfeff000) is
 * defined as "PDP_BASE".... within that mapping there are two
 * defines:
 *   "PDP_PDE" (0xbfeffbfc) is the VA of the PDE in the PDP
 *      which points back to itself.
 *   "APDP_PDE" (0xbfeffffc) is the VA of the PDE in the PDP which
 *      establishes the recursive mapping of the alternate pmap.
 *      to set the alternate PDP, one just has to put the correct
 *	PA info in *APDP_PDE.
 *
 * note that in the APTE_BASE space, the APDP appears at VA
 * "APDP_BASE" (0xfffff000).
 */
/* XXX MP should we allocate one APDP_PDE per processor?? */

/*
 * Mask to get rid of the sign-extended part of addresses.
 */
#define VA_SIGN_MASK		0
#define VA_SIGN_NEG(va)		((va) | VA_SIGN_MASK)
/*
 * XXXfvdl this one's not right.
 */
#define VA_SIGN_POS(va)		((va) & ~VA_SIGN_MASK)

/*
 * the following defines identify the slots used as described above.
 */

#define L2_SLOT_PTE	(KERNBASE/NBPD_L2-1)	/* 767: for recursive PDP map */
#define L2_SLOT_KERN	(KERNBASE/NBPD_L2)	/* 768: start of kernel space */
#define	L2_SLOT_KERNBASE L2_SLOT_KERN
#ifndef XEN
#define L2_SLOT_APTE	1023		/* 1023: alternative recursive slot */
#else
#define L2_SLOT_APTE	1007		/* 1008-1023 reserved by Xen */
#endif


#define PDIR_SLOT_KERN	L2_SLOT_KERN
#define PDIR_SLOT_PTE	L2_SLOT_PTE
#define PDIR_SLOT_APTE	L2_SLOT_APTE

/*
 * the following defines give the virtual addresses of various MMU
 * data structures:
 * PTE_BASE and APTE_BASE: the base VA of the linear PTE mappings
 * PDP_BASE and APDP_BASE: the base VA of the recursive mapping of the PDP
 * PDP_PDE and APDP_PDE: the VA of the PDE that points back to the PDP/APDP
 */

#define PTE_BASE  ((pt_entry_t *) (L2_SLOT_PTE * NBPD_L2))
#define APTE_BASE ((pt_entry_t *) (VA_SIGN_NEG((L2_SLOT_APTE * NBPD_L2))))

#define L1_BASE		PTE_BASE
#define AL1_BASE	APTE_BASE

#define L2_BASE ((pd_entry_t *)((char *)L1_BASE + L2_SLOT_PTE * NBPD_L1))

#define AL2_BASE ((pd_entry_t *)((char *)AL1_BASE + L2_SLOT_PTE * NBPD_L1))

#define PDP_PDE		(L2_BASE + PDIR_SLOT_PTE)
#define APDP_PDE	(L2_BASE + PDIR_SLOT_APTE)

#define PDP_BASE	L2_BASE
#define APDP_BASE	AL2_BASE

/* largest value (-1 for APTP space) */
#define NKL2_MAX_ENTRIES	(NTOPLEVEL_PDES - (KERNBASE/NBPD_L2) - 1)
#define NKL1_MAX_ENTRIES	(unsigned long)(NKL2_MAX_ENTRIES * NPDPG)

#define NKL2_KIMG_ENTRIES	0	/* XXX unused */

#define NKL2_START_ENTRIES	0	/* XXX computed on runtime */
#define NKL1_START_ENTRIES	0	/* XXX unused */

#define NTOPLEVEL_PDES		(PAGE_SIZE / (sizeof (pd_entry_t)))

#define NPDPG			(PAGE_SIZE / sizeof (pd_entry_t))

#define PTP_MASK_INITIALIZER	{ L1_FRAME, L2_FRAME }
#define PTP_SHIFT_INITIALIZER	{ L1_SHIFT, L2_SHIFT }
#define NKPTP_INITIALIZER	{ NKL1_START_ENTRIES, NKL2_START_ENTRIES }
#define NKPTPMAX_INITIALIZER	{ NKL1_MAX_ENTRIES, NKL2_MAX_ENTRIES }
#define NBPD_INITIALIZER	{ NBPD_L1, NBPD_L2 }
#define PDES_INITIALIZER	{ L2_BASE }
#define APDES_INITIALIZER	{ AL2_BASE }

#define PTP_LEVELS	2

/*
 * PG_AVAIL usage: we make use of the ignored bits of the PTE
 */

#define PG_W		PG_AVAIL1	/* "wired" mapping */
#define PG_PVLIST	PG_AVAIL2	/* mapping has entry on pvlist */
#define PG_X		PG_AVAIL3	/* executable mapping */

/*
 * Number of PTE's per cache line.  4 byte pte, 32-byte cache line
 * Used to avoid false sharing of cache lines.
 */
#define NPTECL		8

#include <x86/pmap.h>

#ifndef XEN
#define pmap_pa2pte(a)			(a)
#define pmap_pte2pa(a)			((a) & PG_FRAME)
#define pmap_pte_set(p, n)		do { *(p) = (n); } while (0)
#define pmap_pte_testset(p, n)		\
    atomic_swap_ulong((volatile unsigned long *)p, n)
#define pmap_pte_setbits(p, b)		\
    atomic_or_ulong((volatile unsigned long *)p, b)
#define pmap_pte_clearbits(p, b)	\
    atomic_and_ulong((volatile unsigned long *)p, ~(b))
#define pmap_pte_flush()		/* nothing */
#else
static __inline pt_entry_t
pmap_pa2pte(paddr_t pa)
{
	return (pt_entry_t)xpmap_ptom_masked(pa);
}

static __inline paddr_t
pmap_pte2pa(pt_entry_t pte)
{
	return xpmap_mtop_masked(pte & PG_FRAME);
}
static __inline void
pmap_pte_set(pt_entry_t *pte, pt_entry_t npte)
{
	int s = splvm();
	xpq_queue_pte_update((pt_entry_t *)xpmap_ptetomach(pte), npte);
	splx(s);
}

static __inline pt_entry_t
pmap_pte_testset(volatile pt_entry_t *pte, pt_entry_t npte)
{
	int s = splvm();
	pt_entry_t opte = *pte;
	xpq_queue_pte_update((pt_entry_t *)xpmap_ptetomach(__UNVOLATILE(pte)),
	    npte);
	xpq_flush_queue();
	splx(s);
	return opte;
}

static __inline void
pmap_pte_setbits(volatile pt_entry_t *pte, pt_entry_t bits)
{
	int s = splvm();
	xpq_queue_pte_update((pt_entry_t *)xpmap_ptetomach(__UNVOLATILE(pte)),
	    (*pte) | bits);
	xpq_flush_queue();
	splx(s);
}

static __inline void
pmap_pte_clearbits(volatile pt_entry_t *pte, pt_entry_t bits)
{	
	int s = splvm();
	xpq_queue_pte_update((pt_entry_t *)xpmap_ptetomach(__UNVOLATILE(pte)),
	    (*pte) & ~bits);
	xpq_flush_queue();
	splx(s);
}

static __inline void
pmap_pte_flush(void)
{
	int s = splvm();
	xpq_flush_queue();
	splx(s);
}
#endif

struct trapframe;

int	pmap_exec_fixup(struct vm_map *, struct trapframe *, struct pcb *);
void	pmap_ldt_cleanup(struct lwp *);

#ifdef XEN
#define NKPTP_MIN       4       /* smallest value we allow */
#define NKPTP_MAX       4
#endif /* XXX has to die ! */


#endif	/* _I386_PMAP_H_ */
