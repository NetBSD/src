/*	$NetBSD: vm_machdep.c,v 1.12 1996/01/31 21:34:06 phil Exp $	*/

/*-
 * Copyright (c) 1996 Matthias Pfaller.
 * Copyright (c) 1995 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1993 Philip A. Nelson.
 * Copyright (c) 1982, 1986 The Regents of the University of California.
 * Copyright (c) 1989, 1990 William Jolitz
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, and William Jolitz.
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
 *	@(#)vm_machdep.c	7.3 (Berkeley) 5/13/91
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/buf.h>
#include <sys/user.h>
#include <sys/core.h>
#include <sys/exec.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>

extern struct proc *fpu_proc;

/*
 * Finish a fork operation, with process p2 nearly set up.
 * Copy the pcb and setup the kernel stack for the child.
 * Setup the child's stackframe to return to child_return
 * via proc_trampoline from cpu_switch.
 */
cpu_fork(p1, p2)
	register struct proc *p1, *p2;
{
	register struct pcb *pcb = &p2->p_addr->u_pcb;
	register struct syscframe *tf;
	register struct switchframe *sf;
	extern void proc_trampoline(), child_return();

	/* Copy curpcb (which is presumably p1's PCB) to p2. */
	*pcb = p1->p_addr->u_pcb;
	pcb->pcb_onstack = (struct on_stack *)((u_int)p2->p_addr + USPACE) - 1;
	*pcb->pcb_onstack = *p1->p_addr->u_pcb.pcb_onstack;
	/* If p1 is holding the FPU, update the FPU context of p2. */
	if (fpu_proc == p1)
		save_fpu_context(pcb);
	pmap_activate(&p2->p_vmspace->vm_pmap, pcb);

	/*
	 * Copy the syscframe, and arrange for the child to return directly
	 * through rei().
	 */
	tf = (struct syscframe *)((u_int)p2->p_addr + USPACE) - 1;
	p2->p_md.md_regs = (int *)&(tf->sf_reg);
	sf = (struct switchframe *)tf - 1;
	sf->sf_pc = (long) proc_trampoline;
	sf->sf_fp = (long) &tf->sf_fp;
	sf->sf_reg[REG_R3] = (long) child_return;
	sf->sf_reg[REG_R4] = (long) p2;
	sf->sf_pl = imask[IPL_ZERO];
	pcb->pcb_ksp = (long) sf;
	pcb->pcb_kfp = (long) &sf->sf_fp;
}

/*
 * cpu_set_kpc:
 *
 * Arrange for in-kernel execution of a process to continue at the
 * named pc, as if the code at that address were called as a function
 * with argument, the current process's process pointer.
 *
 * Note that it's assumed that when the named process returns, rei()
 * should be invoked, to return to user mode.
 */
void
cpu_set_kpc(p, pc)
	struct proc *p;
	u_long pc;
{
	struct pcb *pcbp;
	struct switchframe *sf;
	extern void proc_trampoline();

	pcbp = &p->p_addr->u_pcb;
	sf = (struct switchframe *) pcbp->pcb_ksp;
	sf->sf_pc = (long) proc_trampoline;
	sf->sf_reg[REG_R3] = pc;
	sf->sf_reg[REG_R4] = (long) p;
}

/*
 * cpu_swapout is called immediately before a process's 'struct user'
 * and kernel stack are unwired (which are in turn done immediately
 * before it's P_INMEM flag is cleared).  If the process is the
 * current owner of the floating point unit, the FP state has to be
 * saved, so that it goes out with the pcb, which is in the user area.
 */
void
cpu_swapout(p)
	struct proc *p;
{
	/*
	 * Make sure we save the FP state before the user area vanishes.
	 */
	if (fpu_proc != p)
		return;
	save_fpu_context(&p->p_addr->u_pcb);
	fpu_proc = 0;
}

/*
 * cpu_exit is called as the last action during exit.
 *
 * We switch to a temorary stack and address space. Then we release
 * release the original address space and machine-dependent resources,
 * including the memory for the user structure and kernel stack.
 * Once finished, we call cpu_exit, which never returns.
 * We block interrupts until cpu_switch has made things safe again.
 */
void
cpu_exit(arg)
	struct proc *arg;
{
	register struct proc *p __asm("r3");
	cnt.v_swtch++;

	/* Copy arg into a register. */
	movd(arg, p);

	/* If we were using the FPU, forget about it. */
	if (fpu_proc == p)
		fpu_proc = 0;

	/* Switch to temporary stack and address space. */
	lprd(sp, INTSTACK);
	load_ptb(PTDpaddr);

	/* Free resources. */
	vmspace_free(p->p_vmspace);
	(void) splhigh();
	kmem_free(kernel_map, (vm_offset_t)p->p_addr, USPACE);

	/* Don't update pcb in cpu_switch. */
	curproc = NULL;
	cpu_switch();
	/* NOTREACHED */
}

/*
 * Dump the machine specific segment at the start of a core dump.
 */     
struct md_core {
	struct reg intreg;
	struct fpreg freg;
};
int
cpu_coredump(p, vp, cred, chdr)
	struct proc *p;
	struct vnode *vp;
	struct ucred *cred;
	struct core *chdr;
{
	struct md_core md_core;
	struct coreseg cseg;
	int error;

	CORE_SETMAGIC(*chdr, COREMAGIC, MID_NS32532, 0);
	chdr->c_hdrsize = ALIGN(sizeof(*chdr));
	chdr->c_seghdrsize = ALIGN(sizeof(cseg));
	chdr->c_cpusize = sizeof(md_core);

	/* Save integer registers. */
	error = process_read_regs(p, &md_core.intreg);
	if (error)
		return error;

	/* Save floating point registers. */
	error = process_read_fpregs(p, &md_core.freg);
	if (error)
		return error;

	CORE_SETMAGIC(cseg, CORESEGMAGIC, MID_NS32532, CORE_CPU);
	cseg.c_addr = 0;
	cseg.c_size = chdr->c_cpusize;

	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)&cseg, chdr->c_seghdrsize,
	    (off_t)chdr->c_hdrsize, UIO_SYSSPACE, IO_NODELOCKED|IO_UNIT, cred,
	    (int *)0, p);
	if (error)
		return error;

	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)&md_core, sizeof(md_core),
	    (off_t)(chdr->c_hdrsize + chdr->c_seghdrsize), UIO_SYSSPACE,
	    IO_NODELOCKED|IO_UNIT, cred, (int *)0, p);
	if (error)
		return error;

	chdr->c_nseg++;
	return 0;
}


/*
 * Set a red zone in the kernel stack after the u. area.
 */
setredzone(pte, vaddr)
	u_short *pte;
	caddr_t vaddr;
{
/* eventually do this by setting up an expand-down stack segment
   for ss0: selector, allowing stack access down to top of u.
   this means though that protection violations need to be handled
   thru a double fault exception that must do an integral task
   switch to a known good context, within which a dump can be
   taken. a sensible scheme might be to save the initial context
   used by sched (that has physical memory mapped 1:1 at bottom)
   and take the dump while still in mapped mode */
}

/*
 * Move pages from one kernel virtual address to another.
 * Both addresses are assumed to reside in the Sysmap,
 * and size must be a multiple of CLSIZE.
 */
pagemove(from, to, size)
	register caddr_t from, to;
	int size;
{
	int *fpte, *tpte;

	if (size % CLBYTES)
		panic("pagemove");
	fpte = kvtopte(from);
	tpte = kvtopte(to);
	while (size > 0) {
		*tpte++ = *fpte;
		*(int *)fpte++ = 0;
		from += NBPG;
		to += NBPG;
		size -= NBPG;
	}
	pmap_update();
}

/*
 * Convert kernel VA to physical address
 */
kvtop(addr)
	register caddr_t addr;
{
	vm_offset_t va;

	va = pmap_extract(pmap_kernel(), (vm_offset_t)addr);
	if (va == 0)
		panic("kvtop: zero page frame");
	return((int)va);
}

extern vm_map_t phys_map;

/*
 * Map an IO request into kernel virtual address space.  Requests fall into
 * one of five catagories:
 *
 *	B_PHYS|B_UAREA:	User u-area swap.
 *			Address is relative to start of u-area (p_addr).
 *	B_PHYS|B_PAGET:	User page table swap.
 *			Address is a kernel VA in usrpt (Usrptmap).
 *	B_PHYS|B_DIRTY:	Dirty page push.
 *			Address is a VA in proc2's address space.
 *	B_PHYS|B_PGIN:	Kernel pagein of user pages.
 *			Address is VA in user's address space.
 *	B_PHYS:		User "raw" IO request.
 *			Address is VA in user's address space.
 *
 * All requests are (re)mapped into kernel VA space via the useriomap
 * (a name with only slightly more meaning than "kernelmap")
 */
vmapbuf(bp, len)
	struct buf *bp;
	vm_size_t len;
{
	vm_offset_t faddr, taddr, off;
	pt_entry_t *fpte, *tpte;
	pt_entry_t *pmap_pte __P((pmap_t, vm_offset_t));

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vmapbuf");
	faddr = trunc_page(bp->b_saveaddr = bp->b_data);
	off = (vm_offset_t)bp->b_data - faddr;
	len = round_page(off + len);
	taddr = kmem_alloc_wait(phys_map, len);
	bp->b_data = (caddr_t)(taddr + off);
	/*
	 * The region is locked, so we expect that pmap_pte() will return
	 * non-NULL.
	 */
	fpte = pmap_pte(vm_map_pmap(&bp->b_proc->p_vmspace->vm_map), faddr);
	tpte = pmap_pte(vm_map_pmap(phys_map), taddr);
	do {
		*tpte++ = *fpte++;
		len -= PAGE_SIZE;
	} while (len);
}

/*
 * Free the io map PTEs associated with this IO operation.
 * We also invalidate the TLB entries and restore the original b_addr.
 */
vunmapbuf(bp, len)
	struct buf *bp;
	vm_size_t len;
{
	vm_offset_t addr, off;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vunmapbuf");
	addr = trunc_page(bp->b_data);
	off = (vm_offset_t)bp->b_data - addr;
	len = round_page(off + len);
	kmem_free_wakeup(phys_map, addr, len);
	bp->b_data = bp->b_saveaddr;
	bp->b_saveaddr = 0;
}
