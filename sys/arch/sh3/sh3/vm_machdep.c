/*	$NetBSD: vm_machdep.c,v 1.52 2006/05/10 06:24:03 skrll Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc. All rights reserved.
 * Copyright (c) 1982, 1986 The Regents of the University of California.
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
 *	@(#)vm_machdep.c	7.3 (Berkeley) 5/13/91
 */

/*-
 * Copyright (c) 1995 Charles M. Hannum.  All rights reserved.
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

/*
 *	Utah $Hdr: vm_machdep.c 1.16.1.1 89/06/23$
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vm_machdep.c,v 1.52 2006/05/10 06:24:03 skrll Exp $");

#include "opt_kstack_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/buf.h>
#include <sys/user.h>
#include <sys/core.h>
#include <sys/exec.h>
#include <sys/ptrace.h>

#include <uvm/uvm_extern.h>

#include <sh3/locore.h>
#include <sh3/cpu.h>
#include <sh3/reg.h>
#include <sh3/mmu.h>
#include <sh3/cache.h>

extern void proc_trampoline(void);

/*
 * Finish a fork operation, with process p2 nearly set up.
 * Copy and update the pcb and trap frame, making the child ready to run.
 *
 * Rig the child's kernel stack so that it will start out in
 * proc_trampoline() and call child_return() with p2 as an
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
cpu_lwp_fork(struct lwp *l1, struct lwp *l2, void *stack,
    size_t stacksize, void (*func)(void *), void *arg)
{
	struct pcb *pcb;
	struct trapframe *tf;
	struct switchframe *sf;
	vaddr_t spbase, fptop;
#define	P1ADDR(x)	(SH3_PHYS_TO_P1SEG(*__pmap_kpte_lookup(x) & PG_PPN))

	KDASSERT(l1 == curlwp || l1 == &lwp0);

	/* Copy flags */
	l2->l_md.md_flags = l1->l_md.md_flags;

	pcb = NULL;		/* XXXGCC: -Wuninitialized */
#ifdef SH3
	/*
	 * Convert frame pointer top to P1. because SH3 can't make
	 * wired TLB entry, context store space accessing must not cause
	 * exception. For SH3, we are 4K page, P3/P1 conversion don't
	 * cause virtual-aliasing.
	 */
	if (CPU_IS_SH3)
		pcb = (struct pcb *)P1ADDR((vaddr_t)&l2->l_addr->u_pcb);
#endif /* SH3 */
#ifdef SH4
	/* SH4 can make wired entry, no need to convert to P1. */
	if (CPU_IS_SH4)
		pcb = &l2->l_addr->u_pcb;
#endif /* SH4 */

	l2->l_md.md_pcb = pcb;
	fptop = (vaddr_t)pcb + PAGE_SIZE;

	/* set up the kernel stack pointer */
	spbase = (vaddr_t)l2->l_addr + PAGE_SIZE;
#ifdef P1_STACK
	/* Convert to P1 from P3 */
	/*
	 * wbinv u-area to avoid cache-aliasing, since kernel stack
	 * is accessed from P1 instead of P3.
	 */
	if (SH_HAS_VIRTUAL_ALIAS)
		sh_dcache_wbinv_range((vaddr_t)l2->l_addr, USPACE);
	spbase = P1ADDR(spbase);
#else /* !P1_STACK */
	/* Prepare u-area PTEs */
#ifdef SH3
	if (CPU_IS_SH3)
		sh3_switch_setup(l2);
#endif
#ifdef SH4
	if (CPU_IS_SH4)
		sh4_switch_setup(l2);
#endif
#endif /* !P1_STACK */

#ifdef KSTACK_DEBUG
	/* Fill magic number for tracking */
	memset((char *)fptop - PAGE_SIZE + sizeof(struct user), 0x5a,
	    PAGE_SIZE - sizeof(struct user));
	memset((char *)spbase, 0xa5, (USPACE - PAGE_SIZE));
	memset(&pcb->pcb_sf, 0xb4, sizeof(struct switchframe));
#endif /* KSTACK_DEBUG */

	/*
	 * Copy the user context.
	 */
	l2->l_md.md_regs = tf = (struct trapframe *)fptop - 1;
	memcpy(tf, l1->l_md.md_regs, sizeof(struct trapframe));

	/*
	 * If specified, give the child a different stack.
	 */
	if (stack != NULL)
		tf->tf_r15 = (u_int)stack + stacksize;

	/* Setup switch frame */
	sf = &pcb->pcb_sf;
	sf->sf_r11 = (int)arg;		/* proc_trampoline hook func */
	sf->sf_r12 = (int)func;		/* proc_trampoline hook func's arg */
	sf->sf_r15 = spbase + USPACE - PAGE_SIZE;/* current stack pointer */
	sf->sf_r7_bank = sf->sf_r15;	/* stack top */
	sf->sf_r6_bank = (vaddr_t)tf;	/* current frame pointer */
	/* when switch to me, jump to proc_trampoline */
	sf->sf_pr  = (int)proc_trampoline;
	/*
	 * Enable interrupt when switch frame is restored, since
	 * kernel thread begin to run without restoring trapframe.
	 */
	sf->sf_sr = PSL_MD;		/* kernel mode, interrupt enable */
}

/*
 * void cpu_setfunc(struct lwp *l, void (*func)(void *), void *arg):
 *	+ Reset the stack pointer for the process.
 *	+ Arrange to the process to call the specified func
 *	  with argument via the proc_trampoline.
 *
 *	XXX There is a lot of duplicated code here (with cpu_lwp_fork()).
 */
void
cpu_setfunc(struct lwp *l, void (*func)(void *), void *arg)
{
	struct pcb *pcb;
	struct trapframe *tf;
	struct switchframe *sf;
	vaddr_t fptop, spbase;

	pcb = NULL;		/* XXXGCC: -Wuninitialized */
#ifdef SH3
	/*
	 * Convert frame pointer top to P1. because SH3 can't make
	 * wired TLB entry, context store space accessing must not cause
	 * exception. For SH3, we are 4K page, P3/P1 conversion don't
	 * cause virtual-aliasing.
	 */
	if (CPU_IS_SH3)
		pcb = (struct pcb *)P1ADDR((vaddr_t)&l->l_addr->u_pcb);
#endif /* SH3 */
#ifdef SH4
	/* SH4 can make wired entry, no need to convert to P1. */
	if (CPU_IS_SH4)
		pcb = &l->l_addr->u_pcb;
#endif /* SH4 */

	fptop = (vaddr_t)pcb + PAGE_SIZE;
	l->l_md.md_pcb = pcb;

	/* set up the kernel stack pointer */
	spbase = (vaddr_t)l->l_addr + PAGE_SIZE;
#ifdef P1_STACK
	/* Convert to P1 from P3 */
	/*
	 * wbinv u-area to avoid cache-aliasing, since kernel stack
	 * is accessed from P1 instead of P3.
	 */
	if (SH_HAS_VIRTUAL_ALIAS)
		sh_dcache_wbinv_range((vaddr_t)l->l_addr, USPACE);
	spbase = P1ADDR(spbase);
#else /* !P1_STACK */
	/* Prepare u-area PTEs */
#ifdef SH3
	if (CPU_IS_SH3)
		sh3_switch_setup(l);
#endif
#ifdef SH4
	if (CPU_IS_SH4)
		sh4_switch_setup(l);
#endif
#endif /* !P1_STACK */

#ifdef KSTACK_DEBUG
	/* Fill magic number for tracking */
	memset((char *)fptop - PAGE_SIZE + sizeof(struct user), 0x5a,
	    PAGE_SIZE - sizeof(struct user));
	memset((char *)spbase, 0xa5, (USPACE - PAGE_SIZE));
	memset(&pcb->pcb_sf, 0xb4, sizeof(struct switchframe));
#endif /* KSTACK_DEBUG */

	l->l_md.md_regs = tf = (struct trapframe *)fptop - 1;
	tf->tf_ssr = PSL_USERSET;

	/* Setup switch frame */
	sf = &pcb->pcb_sf;
	sf->sf_r11 = (int)arg;		/* proc_trampoline hook func */
	sf->sf_r12 = (int)func;		/* proc_trampoline hook func's arg */
	sf->sf_r15 = spbase + USPACE - PAGE_SIZE;/* current stack pointer */
	sf->sf_r7_bank = sf->sf_r15;	/* stack top */
	sf->sf_r6_bank = (vaddr_t)tf;	/* current frame pointer */
	/* when switch to me, jump to proc_trampoline */
	sf->sf_pr  = (int)proc_trampoline;
	/*
	 * Enable interrupt when switch frame is restored, since
	 * kernel thread begin to run without restoring trapframe.
	 */
	sf->sf_sr = PSL_MD;		/* kernel mode, interrupt enable */
}

void
cpu_lwp_free(struct lwp *l, int proc)
{

	/* Nothing to do */
}

/*
 * Dump the machine specific segment at the start of a core dump.
 */
struct md_core {
	struct reg intreg;
};

int
cpu_coredump(struct lwp *l, void *iocookie, struct core *chdr)
{
	struct md_core md_core;
	struct coreseg cseg;
	int error;

	if (iocookie == NULL) {
		CORE_SETMAGIC(*chdr, COREMAGIC, MID_MACHINE, 0);
		chdr->c_hdrsize = ALIGN(sizeof(*chdr));
		chdr->c_seghdrsize = ALIGN(sizeof(cseg));
		chdr->c_cpusize = sizeof(md_core);
		chdr->c_nseg++;
		return 0;
	}

	/* Save integer registers. */
	error = process_read_regs(l, &md_core.intreg);
	if (error)
		return error;

	CORE_SETMAGIC(cseg, CORESEGMAGIC, MID_MACHINE, CORE_CPU);
	cseg.c_addr = 0;
	cseg.c_size = chdr->c_cpusize;

	error = coredump_write(iocookie, UIO_SYSSPACE, &cseg,
	    chdr->c_seghdrsize);
	if (error)
		return error;

	return coredump_write(iocookie, UIO_SYSSPACE, &md_core,
	    sizeof(md_core));
}

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
 * All requests are (re)mapped into kernel VA space via the phys_map
 * (a name with only slightly more meaning than "kernel_map")
 */

void
vmapbuf(struct buf *bp, vsize_t len)
{
	vaddr_t faddr, taddr, off;
	paddr_t fpa;
	pmap_t kpmap, upmap;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vmapbuf");
	bp->b_saveaddr = bp->b_data;
	faddr = trunc_page((vaddr_t)bp->b_data);
	off = (vaddr_t)bp->b_data - faddr;
	len = round_page(off + len);
	taddr = uvm_km_alloc(phys_map, len, 0, UVM_KMF_VAONLY | UVM_KMF_WAITVA);
	bp->b_data = (caddr_t)(taddr + off);
	/*
	 * The region is locked, so we expect that pmap_pte() will return
	 * non-NULL.
	 * XXX: unwise to expect this in a multithreaded environment.
	 * anything can happen to a pmap between the time we lock a
	 * region, release the pmap lock, and then relock it for
	 * the pmap_extract().
	 *
	 * no need to flush TLB since we expect nothing to be mapped
	 * where we we just allocated (TLB will be flushed when our
	 * mapping is removed).
	 */
	upmap = vm_map_pmap(&bp->b_proc->p_vmspace->vm_map);
	kpmap = vm_map_pmap(phys_map);
	while (len) {
		pmap_extract(upmap, faddr, &fpa);
		pmap_enter(kpmap, taddr, fpa,
		    VM_PROT_READ | VM_PROT_WRITE, PMAP_WIRED);
		faddr += PAGE_SIZE;
		taddr += PAGE_SIZE;
		len -= PAGE_SIZE;
	}
	pmap_update(kpmap);
}

/*
 * Free the io map PTEs associated with this IO operation.
 * We also invalidate the TLB entries and restore the original b_addr.
 */
void
vunmapbuf(struct buf *bp, vsize_t len)
{
	vaddr_t addr, off;
	pmap_t kpmap;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vunmapbuf");
	addr = trunc_page((vaddr_t)bp->b_data);
	off = (vaddr_t)bp->b_data - addr;
	len = round_page(off + len);
	kpmap = vm_map_pmap(phys_map);
	pmap_remove(kpmap, addr, addr + len);
	pmap_update(kpmap);
	uvm_km_free(phys_map, addr, len, UVM_KMF_VAONLY);
	bp->b_data = bp->b_saveaddr;
	bp->b_saveaddr = 0;
}
