/*	$NetBSD: vm_machdep.c,v 1.9 2004/01/04 11:33:30 jdolecek Exp $	*/

/*	$OpenBSD: vm_machdep.c,v 1.25 2001/09/19 20:50:56 mickey Exp $	*/

/*
 * Copyright (c) 1999-2000 Michael Shalayeff
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
 *      This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vm_machdep.c,v 1.9 2004/01/04 11:33:30 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <sys/exec.h>
#include <sys/core.h>

#include <machine/cpufunc.h>
#include <machine/pmap.h>
#include <machine/pcb.h>

#include <uvm/uvm.h>

#include <hppa/hppa/machdep.h>

/*
 * Dump the machine specific header information at the start of a core dump.
 */
int
cpu_coredump(struct lwp *l, struct vnode *vp, struct ucred *cred,
    struct core *core)
{
	struct proc *p = l->l_proc;
	struct md_coredump md_core;
	struct coreseg cseg;
	off_t off;
	int error;

	CORE_SETMAGIC(*core, COREMAGIC, MID_ZERO, 0);
	core->c_hdrsize = ALIGN(sizeof(*core));
	core->c_seghdrsize = ALIGN(sizeof(cseg));
	core->c_cpusize = sizeof(md_core);

	process_read_regs(l, &md_core.md_reg);

	CORE_SETMAGIC(cseg, CORESEGMAGIC, MID_ZERO, CORE_CPU);
	cseg.c_addr = 0;
	cseg.c_size = core->c_cpusize;

#define	write(vp, addr, n) vn_rdwr(UIO_WRITE, (vp), (caddr_t)(addr), (n), off, \
			     UIO_SYSSPACE, IO_NODELOCKED|IO_UNIT, cred, NULL, p)
	
	off = core->c_hdrsize;
	if ((error = write(vp, &cseg, core->c_seghdrsize)))
		return error;
	off += core->c_seghdrsize;
	if ((error = write(vp, &md_core, sizeof md_core)))
		return error;

#undef write
	core->c_nseg++;

	return error;
}

/*
 * Move pages from one kernel virtual address to another.
 * Both addresses are assumed to reside in the Sysmap.
 */
void
pagemove(caddr_t from, caddr_t to, size_t size)
{
	paddr_t pa;
	boolean_t rv;

	KASSERT(((vaddr_t)from & PGOFSET) == 0);
	KASSERT(((vaddr_t)to & PGOFSET) == 0);
	KASSERT((size & PGOFSET) == 0);
	while (size > 0) {
		rv = pmap_extract(pmap_kernel(), (vaddr_t)from, &pa);
		KASSERT(rv);
		KASSERT(!pmap_extract(pmap_kernel(), (vaddr_t)to, NULL));
		pmap_kremove((vaddr_t)from, PAGE_SIZE);
		pmap_kenter_pa((vaddr_t)to, pa,
			       VM_PROT_READ|VM_PROT_WRITE);
		from += PAGE_SIZE;
		to += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
}

void
cpu_swapin(struct lwp *l)
{
	struct trapframe *tf = l->l_md.md_regs;

	/*
	 * Stash the physical for the pcb of U for later perusal
	 */
	l->l_addr->u_pcb.pcb_uva = (vaddr_t)l->l_addr;
	tf->tf_cr30 = kvtop((caddr_t)l->l_addr);
	fdcache(HPPA_SID_KERNEL, (vaddr_t)l->l_addr, sizeof(l->l_addr->u_pcb));

#ifdef HPPA_REDZONE
	/* Create the kernel stack red zone. */
	pmap_redzone((vaddr_t)l->l_addr + HPPA_REDZONE,
		(vaddr_t)l->l_addr + USPACE, 1);
#endif
}

void
cpu_swapout(struct lwp *l)
{

	/* Flush this LWP out of the FPU. */
	hppa_fpu_flush(l);
}

void
cpu_lwp_fork(struct lwp *l1, struct lwp *l2, void *stack, size_t stacksize,
    void (*func)(void *), void *arg)
{
	struct pcb *pcbp;
	struct trapframe *tf;
	register_t sp, osp;

#ifdef DIAGNOSTIC
	if (round_page(sizeof(struct user)) > PAGE_SIZE)
		panic("USPACE too small for user");
#endif

	/* Flush the parent process out of the FPU. */
	hppa_fpu_flush(l1);

	/* Now copy the parent PCB into the child. */
	pcbp = &l2->l_addr->u_pcb;
	bcopy(&l1->l_addr->u_pcb, pcbp, sizeof(*pcbp));

	sp = (register_t)l2->l_addr + PAGE_SIZE;
	l2->l_md.md_regs = tf = (struct trapframe *)sp;
	sp += sizeof(struct trapframe);
	bcopy(l1->l_md.md_regs, tf, sizeof(*tf));

	/*
	 * cpu_swapin() is supposed to fill out all the PAs
	 * we gonna need in locore
	 */
	cpu_swapin(l2);

	/* Activate this process' pmap. */
	pmap_activate(l2);

	/*
	 * theoretically these could be inherited from the father,
	 * but just in case.
	 */
	tf->tf_sr7 = HPPA_SID_KERNEL;
	mfctl(CR_EIEM, tf->tf_eiem);
	tf->tf_ipsw = PSW_C | PSW_Q | PSW_P | PSW_D | PSW_I /* | PSW_L */;
	pcbp->pcb_fpregs[HPPA_NFPREGS] = 0;

	/*
	 * Set up return value registers as libc:fork() expects
	 */
	tf->tf_ret0 = l1->l_proc->p_pid;
	tf->tf_ret1 = 1;	/* ischild */
	tf->tf_t1 = 0;		/* errno */

	/*
	 * If specified, give the child a different stack.
	 */
	if (stack != NULL)
		tf->tf_sp = (register_t)stack;

	/*
	 * Build a stack frame for the cpu_switch & co.
	 */
	osp = sp;
	sp += HPPA_FRAME_SIZE + 16*4; /* std frame + calee-save registers */
	*HPPA_FRAME_CARG(0, sp) = tf->tf_sp;
	*HPPA_FRAME_CARG(1, sp) = KERNMODE(func);
	*HPPA_FRAME_CARG(2, sp) = (register_t)arg;
	*(register_t*)(sp + HPPA_FRAME_PSP) = osp;
	*(register_t*)(sp + HPPA_FRAME_CRP) =
		(register_t)switch_trampoline;
	tf->tf_sp = sp;
	fdcache(HPPA_SID_KERNEL, (vaddr_t)l2->l_addr, sp - (vaddr_t)l2->l_addr);
}

void
cpu_setfunc(struct lwp *l, void (*func)(void *), void *arg)
{
	printf("cpu_setfunc not implemented\n");
}

void
cpu_lwp_free(struct lwp *l, int proc)
{

	/* Flush the LWP out of the FPU. */
	hppa_fpu_flush(l);
}

void
cpu_exit(struct lwp *l)
{
	(void) splsched();
	switch_exit(l, lwp_exit2);
}

/*
 * Map an IO request into kernel virtual address space.
 */
void
vmapbuf(struct buf *bp, vsize_t len)
{
	vaddr_t uva, kva;
	paddr_t pa;
	vsize_t size, off;
	int npf;
	struct proc *p;
	struct vm_map *map;
	struct pmap *upmap, *kpmap;

#ifdef DIAGNOSTIC
	if ((bp->b_flags & B_PHYS) == 0)
		panic("vmapbuf");
#endif
	p = bp->b_proc;
	map = &p->p_vmspace->vm_map;
	upmap = vm_map_pmap(map);
	kpmap = vm_map_pmap(phys_map);
	bp->b_saveaddr = bp->b_data;
	uva = trunc_page((vaddr_t)bp->b_data);
	off = (vaddr_t)bp->b_data - uva;
	size = round_page(off + len);

	kva = uvm_km_valloc_prefer_wait(phys_map, size, uva);
	bp->b_data = (caddr_t)(kva + off);
	npf = btoc(size);
	while (npf--) {
		if (pmap_extract(upmap, uva, &pa) == FALSE)
			panic("vmapbuf: null page frame");
		pmap_enter(kpmap, kva, pa,
		    VM_PROT_READ | VM_PROT_WRITE, PMAP_WIRED);
		uva += PAGE_SIZE;
		kva += PAGE_SIZE;
	}
	pmap_update(kpmap);
}

/*
 * Unmap IO request from the kernel virtual address space.
 */
void
vunmapbuf(struct buf *bp, vsize_t len)
{
	struct pmap *pmap;
	vaddr_t addr;
	vsize_t off;

#ifdef DIAGNOSTIC
	if ((bp->b_flags & B_PHYS) == 0)
		panic("vunmapbuf");
#endif
	addr = trunc_page((vaddr_t)bp->b_data);
	off = (vaddr_t)bp->b_data - addr;
	len = round_page(off + len);
	pmap = vm_map_pmap(phys_map);
	pmap_remove(pmap, addr, addr + len);
	pmap_update(pmap);
	uvm_km_free_wakeup(phys_map, addr, len);
	bp->b_data = bp->b_saveaddr;
	bp->b_saveaddr = NULL;
}
