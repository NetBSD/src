/*	$NetBSD: vm_machdep.c,v 1.38 1999/05/26 22:07:39 thorpej Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: vm_machdep.c,v 1.38 1999/05/26 22:07:39 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/user.h>
#include <sys/core.h>
#include <sys/exec.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>

#include <uvm/uvm_extern.h>

#include <mips/regnum.h>
#include <mips/locore.h>
#include <mips/pte.h>
#include <machine/cpu.h>

/* XXX will be declared in mips/include/cpu.h XXX */
extern struct proc *fpcurproc;

extern paddr_t kvtophys __P((vaddr_t));	/* XXX */

/*
 * Finish a fork operation, with process p2 nearly set up.  Copy and
 * update the kernel stack and pcb, making the child ready to run,
 * and marking it so that it can return differently than the parent.
 * When scheduled, child p2 will start from proc_trampoline(). cpu_fork()
 * returns once for forking parent p1.
 */
void
cpu_fork(p1, p2, stack, stacksize)
	struct proc *p1, *p2;
	void *stack;
	size_t stacksize;
{
	struct pcb *pcb;
	struct frame *f;
	pt_entry_t *pte;
	int i, x;

#ifdef MIPS3
	/* ? make sense ? */
	if (CPUISMIPS3)
		mips3_HitFlushDCache((vaddr_t)p2->p_addr, USPACE);
#endif


#ifdef DIAGNOSTIC
	/*
	 * If p1 != curproc && p1 == &proc0, we're creating a kernel thread.
	 */
	if (p1 != curproc && p1 != &proc0)
		panic("cpu_fork: curproc");
#endif
	if (p1 == fpcurproc)
		savefpregs(p1);

	/*
	 * Copy pcb from proc p1 to p2.
	 * Copy p1 trapframe atop on p2 stack space, so return to user mode
	 * will be to right address, with correct registers.
	 */
	memcpy(&p2->p_addr->u_pcb, &p1->p_addr->u_pcb, sizeof(struct pcb));
	f = (struct frame *)((caddr_t)p2->p_addr + USPACE) - 1;
	memcpy(f, p1->p_md.md_regs, sizeof(struct frame));
	memset((caddr_t)f - 24, 0, 24);		/* ? required ? */

	/*
	 * If specified, give the child a different stack.
	 */
	if (stack != NULL)
		f->f_regs[SP] = (u_int)stack + stacksize;

	p2->p_md.md_regs = (void *)f;
	p2->p_md.md_flags = p1->p_md.md_flags & MDP_FPUSED;
	x = (CPUISMIPS3) ? (MIPS3_PG_G|MIPS3_PG_RO|MIPS3_PG_WIRED) : MIPS1_PG_G;
	pte = kvtopte(p2->p_addr);
	for (i = 0; i < UPAGES; i++)
		p2->p_md.md_upte[i] = pte[i].pt_entry &~ x;

	/*
	 * Arrange for continuation at child_return(), which will return to
	 * user process soon.
	 */
	pcb = &p2->p_addr->u_pcb;
	pcb->pcb_segtab = (void *)p2->p_vmspace->vm_map.pmap->pm_segtab;
	pcb->pcb_context[10] = (int)proc_trampoline;	/* RA */
	pcb->pcb_context[8] = (int)f - 24;		/* SP */
	pcb->pcb_context[0] = (int)child_return;	/* S0 */
	pcb->pcb_context[1] = (int)p2;			/* S1 */
}

/*
 * Arrange for in-kernel execution of a process to continue at the
 * named pc, as if the code at that address were called as a function
 * with argument, the current process's process pointer.
 */
void
cpu_set_kpc(p, pc, arg)
	struct proc *p;
	void (*pc) __P((void *));
	void *arg;
{
	struct pcb *pcb = &p->p_addr->u_pcb;

	pcb->pcb_context[10] = (int)proc_trampoline;	/* RA */
	pcb->pcb_context[0] = (int)pc;			/* S0 */
	pcb->pcb_context[1] = (int)arg;			/* S1 */
}

/*
 * Finish a swapin operation.
 * We neded to update the cached PTEs for the user area in the
 * machine dependent part of the proc structure.
 */
void
cpu_swapin(p)
	struct proc *p;
{
	pt_entry_t *pte;
	int i, x;

	/*
	 * Cache the PTEs for the user area in the machine dependent
	 * part of the proc struct so cpu_switch() can quickly map in
	 * the user struct and kernel stack.
	 */
	x = (CPUISMIPS3) ? (MIPS3_PG_G|MIPS3_PG_RO|MIPS3_PG_WIRED) : MIPS1_PG_G;
	pte = kvtopte(p->p_addr);
	for (i = 0; i < UPAGES; i++)
		p->p_md.md_upte[i] = pte[i].pt_entry &~ x;
}

/*
 * cpu_exit is called as the last action during exit.
 *
 * We clean up a little and then call switch_exit() with the old proc as an
 * argument.  switch_exit() first switches to proc0's PCB and stack,
 * schedules the dead proc's vmspace and stack to be freed, then jumps
 * into the middle of cpu_switch(), as if it were switching from proc0.
 */
void
cpu_exit(p)
	struct proc *p;
{
	extern void switch_exit __P((struct proc *));

	if (fpcurproc == p)
		fpcurproc = (struct proc *)0;

	uvmexp.swtch++;
	(void)splhigh();
	switch_exit(p);
	/* NOTREACHED */
}

/*
 * Dump the machine specific segment at the start of a core dump.
 */
int
cpu_coredump(p, vp, cred, chdr)
	struct proc *p;
	struct vnode *vp;
	struct ucred *cred;
	struct core *chdr;
{
	int error;
	struct coreseg cseg;
	struct cpustate {
		struct frame frame;
		struct fpreg fpregs;
	} cpustate;

	CORE_SETMAGIC(*chdr, COREMAGIC, MID_MACHINE, 0);
	chdr->c_hdrsize = ALIGN(sizeof(struct core));
	chdr->c_seghdrsize = ALIGN(sizeof(struct coreseg));
	chdr->c_cpusize = sizeof(struct cpustate);

	cpustate.frame = *(struct frame *)p->p_md.md_regs;
	if (p->p_md.md_flags & MDP_FPUSED) {
		if (p == fpcurproc)
			savefpregs(p);
		cpustate.fpregs = p->p_addr->u_pcb.pcb_fpregs;
	}
	else
		memset(&cpustate.fpregs, 0, sizeof(struct fpreg));

	CORE_SETMAGIC(cseg, CORESEGMAGIC, MID_MACHINE, CORE_CPU);
	cseg.c_addr = 0;
	cseg.c_size = chdr->c_cpusize;
	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)&cseg, chdr->c_seghdrsize,
	    (off_t)chdr->c_hdrsize, UIO_SYSSPACE,
	    IO_NODELOCKED|IO_UNIT, cred, NULL, p);
	if (error)
		return error;

	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)&cpustate,
			(off_t)chdr->c_cpusize,
			(off_t)(chdr->c_hdrsize + chdr->c_seghdrsize),
			UIO_SYSSPACE, IO_NODELOCKED|IO_UNIT,
			cred, NULL, p);

	if (!error)
		chdr->c_nseg++;

	return error;
}

/*
 * Move pages from one kernel virtual address to another.
 * Both addresses are assumed to reside in the Sysmap,
 * and size must be a multiple of CLSIZE.
 */
void
pagemove(from, to, size)
	caddr_t from, to;
	size_t size;
{
	pt_entry_t *fpte, *tpte;

	if (size % CLBYTES)
		panic("pagemove");
	fpte = kvtopte(from);
	tpte = kvtopte(to);
#ifdef MIPS3
	if (CPUISMIPS3 &&
	    ((int)from & mips_CacheAliasMask) !=
	    ((int)to & mips_CacheAliasMask)) {
		mips3_HitFlushDCache((vaddr_t)from, size);
	}
#endif
	while (size > 0) {
		MachTLBFlushAddr((vaddr_t)from);
		MachTLBUpdate((vaddr_t)to, fpte->pt_entry);
		*tpte = *fpte;
		if (CPUISMIPS3)
			fpte->pt_entry = MIPS3_PG_NV | MIPS3_PG_G;
		else
			fpte->pt_entry = MIPS1_PG_NV;

		fpte++; tpte++;
		size -= PAGE_SIZE;
		from += PAGE_SIZE;
		to += NBPG;
	}
}

extern vm_map_t phys_map;

/*
 * Map a user I/O request into kernel virtual address space.
 * Note: the pages are already locked by uvm_vslock(), so we
 * do not need to pass an access_type to pmap_enter().   
 */
void
vmapbuf(bp, len)
	struct buf *bp;
	vsize_t len;
{
	vaddr_t faddr, taddr;
	vsize_t off;
	pt_entry_t *fpte, *tpte;
	pt_entry_t *pmap_pte __P((pmap_t, vaddr_t));
	u_int pt_mask;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vmapbuf");
	faddr = trunc_page(bp->b_saveaddr = bp->b_data);
	off = (vaddr_t)bp->b_data - faddr;
	len = round_page(off + len);
	taddr = uvm_km_valloc_wait(phys_map, len);
	bp->b_data = (caddr_t)(taddr + off);
	/*
	 * The region is locked, so we expect that pmap_pte() will return
	 * non-NULL.
	 */
	fpte = pmap_pte(vm_map_pmap(&bp->b_proc->p_vmspace->vm_map), faddr);
	tpte = pmap_pte(vm_map_pmap(phys_map), taddr);
	pt_mask = (CPUISMIPS3) ? (MIPS3_PG_V|MIPS3_PG_G|MIPS3_PG_M) :
				 (MIPS1_PG_V|MIPS1_PG_G|MIPS1_PG_M);
	do {
		/* XXX should mark them PG_WIRED? */
		tpte->pt_entry = fpte->pt_entry | pt_mask;
		MachTLBUpdate(taddr, tpte->pt_entry);
		tpte++, fpte++, taddr += PAGE_SIZE;
		len -= PAGE_SIZE;
	} while (len);
}

/*
 * Free the io map PTEs associated with this IO operation.
 * We also invalidate the TLB entries and restore the original b_addr.
 */
void
vunmapbuf(bp, len)
	struct buf *bp;
	vsize_t len;
{
	vaddr_t addr;
	vsize_t off;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vunmapbuf");
	addr = trunc_page(bp->b_data);
	off = (vaddr_t)bp->b_data - addr;
	len = round_page(off + len);
	uvm_km_free_wakeup(phys_map, addr, len);
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
kvtophys(kva)
	vaddr_t kva;
{
	pt_entry_t *pte;
	paddr_t phys;

	if (kva >= MIPS_KSEG2_START) {
		if (kva >= VM_MAX_KERNEL_ADDRESS)
			goto overrun;

		pte = kvtopte(kva);
		if ((pte - Sysmap) > Sysmapsize)  {
			printf("oops: Sysmap overrun, max %d index %d\n",
			       Sysmapsize, pte - Sysmap);
		}
		if (!mips_pg_v(pte->pt_entry)) {
			printf("kvtophys: pte not valid for %lx\n", kva);
		}
		phys = pfn_to_vad(pte->pt_entry) | (kva & PGOFSET);
		return phys;
	}
	if (kva >= MIPS_KSEG1_START)
		return MIPS_KSEG0_TO_PHYS(kva);

	if (kva >= MIPS_KSEG0_START)
		return MIPS_KSEG0_TO_PHYS(kva);

overrun:
#ifdef DDB
	printf("Virtual address %lx: cannot map to physical\n", kva);
	Debugger();
#else
	panic("Virtual address %lx: cannot map to physical\n", kva);
#endif
}
