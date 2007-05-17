/*	$NetBSD: vm_machdep.c,v 1.117 2007/05/17 14:51:25 yamt Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah Hdr: vm_machdep.c 1.21 91/04/06
 *
 *	@(#)vm_machdep.c	8.3 (Berkeley) 1/4/94
 */
/*
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah Hdr: vm_machdep.c 1.21 91/04/06
 *
 *	@(#)vm_machdep.c	8.3 (Berkeley) 1/4/94
 */

#include "opt_ddb.h"
#include "opt_coredump.h"

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: vm_machdep.c,v 1.117 2007/05/17 14:51:25 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/user.h>
#include <sys/core.h>
#include <sys/exec.h>

#include <uvm/uvm_extern.h>

#include <mips/cache.h>
#include <mips/regnum.h>
#include <mips/locore.h>
#include <mips/pte.h>
#include <mips/psl.h>
#include <machine/cpu.h>

paddr_t kvtophys(vaddr_t);	/* XXX */

/*
 * Finish a fork operation, with process p2 nearly set up.
 * Copy and update the pcb and trap frame, making the child ready to run.
 *
 * Rig the child's kernel stack so that it will start out in
 * lwp_trampoline() and call child_return() with p2 as an
 * argument. This causes the newly-created child process to go
 * directly to user level with an apparent return value of 0 from
 * fork(), while the parent process returns normally.
 *
 * p1 is the process being forked; if p1 == &proc0, we are creating
 * a kernel thread, and the return path and argument are specified with
 * `func' and `arg'.
 *
 * If an alternate user-level stack is requested (with non-zero values
 * in both the stack and stacksize args), set up the user stack pointer
 * accordingly.
 */
void
cpu_lwp_fork(struct lwp *l1, struct lwp *l2, void *stack, size_t stacksize,
    void (*func)(void *), void *arg)
{
	struct pcb *pcb;
	struct frame *f;
	pt_entry_t *pte;
	int i, x;

#ifdef DIAGNOSTIC
	/*
	 * If l1 != curlwp && l1 == &lwp0, we're creating a kernel thread.
	 */
	if (l1 != curlwp && l1 != &lwp0)
		panic("cpu_lwp_fork: curlwp");
#endif
	if ((l1->l_md.md_flags & MDP_FPUSED) && l1 == fpcurlwp)
		savefpregs(l1);

	/*
	 * Copy pcb from proc p1 to p2.
	 * Copy p1 trapframe atop on p2 stack space, so return to user mode
	 * will be to right address, with correct registers.
	 */
	memcpy(&l2->l_addr->u_pcb, &l1->l_addr->u_pcb, sizeof(struct pcb));
	f = (struct frame *)((char *)l2->l_addr + USPACE) - 1;
	memcpy(f, l1->l_md.md_regs, sizeof(struct frame));

	/*
	 * If specified, give the child a different stack.
	 */
	if (stack != NULL)
		f->f_regs[_R_SP] = (uintptr_t)stack + stacksize;

	l2->l_md.md_regs = (void *)f;
	l2->l_md.md_flags = l1->l_md.md_flags & MDP_FPUSED;
	x = (MIPS_HAS_R4K_MMU) ?
	    (MIPS3_PG_G | MIPS3_PG_RO | MIPS3_PG_WIRED) :
	    MIPS1_PG_G;
	pte = kvtopte(l2->l_addr);
	for (i = 0; i < UPAGES; i++)
		l2->l_md.md_upte[i] = pte[i].pt_entry &~ x;

	pcb = &l2->l_addr->u_pcb;
	pcb->pcb_context[0] = (intptr_t)func;		/* S0 */
	pcb->pcb_context[1] = (intptr_t)arg;		/* S1 */
	pcb->pcb_context[MIPS_CURLWP_CARD - 16] = (intptr_t)l2;/* S? */
	pcb->pcb_context[8] = (intptr_t)f;		/* SP */
	pcb->pcb_context[10] = (intptr_t)lwp_trampoline;/* RA */
	pcb->pcb_context[11] |= PSL_LOWIPL;		/* SR */
#ifdef IPL_ICU_MASK
	pcb->pcb_ppl = 0;	/* machine dependent interrupt mask */
#endif
}

/*
 * Set the given LWP to start at the given function via the
 * lwp_trampoline.
 */
void
cpu_setfunc(struct lwp *l, void (*func)(void *), void *arg)
{
	struct pcb *pcb;
	struct frame *f;

	f = (struct frame *)((char *)l->l_addr + USPACE) - 1;
	KASSERT(l->l_md.md_regs == f);

	pcb = &l->l_addr->u_pcb;
	pcb->pcb_context[0] = (intptr_t)func;			/* S0 */
	pcb->pcb_context[1] = (intptr_t)arg;			/* S1 */
	pcb->pcb_context[MIPS_CURLWP_CARD - 16] = (intptr_t)l;	/* S? */
	pcb->pcb_context[8] = (intptr_t)f;			/* SP */
	pcb->pcb_context[10] = (intptr_t)lwp_trampoline;	/* RA */
	pcb->pcb_context[11] |= PSL_LOWIPL;			/* SR */
#ifdef IPL_ICU_MASK
	pcb->pcb_ppl = 0;	/* machine depenedend interrupt mask */
#endif
}	

/*
 * Finish a swapin operation.
 * We neded to update the cached PTEs for the user area in the
 * machine dependent part of the proc structure.
 */
void
cpu_swapin(struct lwp *l)
{
	pt_entry_t *pte;
	int i, x;

	/*
	 * Cache the PTEs for the user area in the machine dependent
	 * part of the proc struct so cpu_switchto() can quickly map
	 * in the user struct and kernel stack.
	 */
	x = (MIPS_HAS_R4K_MMU) ?
	    (MIPS3_PG_G | MIPS3_PG_RO | MIPS3_PG_WIRED) :
	    MIPS1_PG_G;
	pte = kvtopte(l->l_addr);
	for (i = 0; i < UPAGES; i++)
		l->l_md.md_upte[i] = pte[i].pt_entry &~ x;
}

void
cpu_lwp_free(struct lwp *l, int proc)
{

	if ((l->l_md.md_flags & MDP_FPUSED) && l == fpcurlwp)
		fpcurlwp = NULL;
}

void
cpu_lwp_free2(struct lwp *l)
{

	(void)l;
}

#ifdef COREDUMP
/*
 * Dump the machine specific segment at the start of a core dump.
 */
int
cpu_coredump(struct lwp *l, void *iocookie, struct core *chdr)
{
	int error;
	struct coreseg cseg;
	struct cpustate {
		struct frame frame;
		struct fpreg fpregs;
	} cpustate;

	if (iocookie == NULL) {
		CORE_SETMAGIC(*chdr, COREMAGIC, MID_MACHINE, 0);
		chdr->c_hdrsize = ALIGN(sizeof(struct core));
		chdr->c_seghdrsize = ALIGN(sizeof(struct coreseg));
		chdr->c_cpusize = sizeof(struct cpustate);
		chdr->c_nseg++;
		return 0;
	}

	if ((l->l_md.md_flags & MDP_FPUSED) && l == fpcurlwp)
		savefpregs(l);
	cpustate.frame = *(struct frame *)l->l_md.md_regs;
	cpustate.fpregs = l->l_addr->u_pcb.pcb_fpregs;

	CORE_SETMAGIC(cseg, CORESEGMAGIC, MID_MACHINE, CORE_CPU);
	cseg.c_addr = 0;
	cseg.c_size = chdr->c_cpusize;

	error = coredump_write(iocookie, UIO_SYSSPACE, &cseg,
	    chdr->c_seghdrsize);
	if (error)
		return error;

	return coredump_write(iocookie, UIO_SYSSPACE, &cpustate,
	    chdr->c_cpusize);
}
#endif

/*
 * Map a user I/O request into kernel virtual address space.
 */
void
vmapbuf(struct buf *bp, vsize_t len)
{
	struct pmap *upmap;
	vaddr_t uva;	/* User VA (map from) */
	vaddr_t kva;	/* Kernel VA (new to) */
	paddr_t pa;	/* physical address */
	vsize_t off;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vmapbuf");

	uva = mips_trunc_page(bp->b_saveaddr = bp->b_data);
	off = (vaddr_t)bp->b_data - uva;
	len = mips_round_page(off + len);
	kva = uvm_km_alloc(phys_map, len, 0, UVM_KMF_VAONLY | UVM_KMF_WAITVA);
	bp->b_data = (void *)(kva + off);
	upmap = vm_map_pmap(&bp->b_proc->p_vmspace->vm_map);
	do {
		if (pmap_extract(upmap, uva, &pa) == false)
			panic("vmapbuf: null page frame");
		pmap_enter(vm_map_pmap(phys_map), kva, pa,
		    VM_PROT_READ | VM_PROT_WRITE, PMAP_WIRED);
		uva += PAGE_SIZE;
		kva += PAGE_SIZE;
		len -= PAGE_SIZE;
	} while (len);
	pmap_update(vm_map_pmap(phys_map));
}

/*
 * Unmap a previously-mapped user I/O request.
 */
void
vunmapbuf(struct buf *bp, vsize_t len)
{
	vaddr_t kva;
	vsize_t off;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vunmapbuf");

	kva = mips_trunc_page(bp->b_data);
	off = (vaddr_t)bp->b_data - kva;
	len = mips_round_page(off + len);
	pmap_remove(vm_map_pmap(phys_map), kva, kva + len);
	pmap_update(pmap_kernel());
	uvm_km_free(phys_map, kva, len, UVM_KMF_VAONLY);
	bp->b_data = bp->b_saveaddr;
	bp->b_saveaddr = NULL;
}

/*
 * Map a (kernel) virtual address to a physical address.
 *
 * MIPS processor has 3 distinct kernel address ranges:
 *
 * - kseg0 kernel "virtual address" for the   cached physical address space.
 * - kseg1 kernel "virtual address" for the uncached physical address space.
 * - kseg2 normal kernel "virtual address" mapped via the TLB.
 */
paddr_t
kvtophys(vaddr_t kva)
{
	pt_entry_t *pte;
	paddr_t phys;

	if (kva >= MIPS_KSEG2_START) {
		if (kva >= VM_MAX_KERNEL_ADDRESS)
			goto overrun;

		pte = kvtopte(kva);
		if ((size_t) (pte - Sysmap) > Sysmapsize)  {
			printf("oops: Sysmap overrun, max %d index %zd\n",
			       Sysmapsize, pte - Sysmap);
		}
		if (!mips_pg_v(pte->pt_entry)) {
			printf("kvtophys: pte not valid for %lx\n", kva);
		}
		phys = mips_tlbpfn_to_paddr(pte->pt_entry) | (kva & PGOFSET);
		return phys;
	}
	if (kva >= MIPS_KSEG1_START)
		return MIPS_KSEG1_TO_PHYS(kva);

	if (kva >= MIPS_KSEG0_START)
		return MIPS_KSEG0_TO_PHYS(kva);

overrun:
	printf("Virtual address %lx: cannot map to physical\n", kva);
#ifdef DDB
	Debugger();
	return 0;	/* XXX */
#endif
	panic("kvtophys");
}
