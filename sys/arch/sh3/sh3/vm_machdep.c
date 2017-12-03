/*	$NetBSD: vm_machdep.c,v 1.75.2.2 2017/12/03 11:36:42 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: vm_machdep.c,v 1.75.2.2 2017/12/03 11:36:42 jdolecek Exp $");

#include "opt_kstack_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/buf.h>
#include <sys/core.h>
#include <sys/exec.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/kauth.h>
#include <sys/ktrace.h>

#include <dev/mm.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_page.h>
#include <uvm/uvm_physseg.h>

#include <sh3/locore.h>
#include <sh3/cpu.h>
#include <sh3/pcb.h>
#include <sh3/mmu.h>
#include <sh3/cache.h>
#include <sh3/userret.h>

extern void lwp_trampoline(void);

static void sh3_setup_uarea(struct lwp *);

/*
 * Finish a fork operation, with lwp l2 nearly set up.  Copy and
 * update the pcb and trap frame, making the child ready to run.
 *
 * Rig the child's kernel stack so that it will start out in
 * lwp_trampoline() and call child_return() with l2 as an argument.
 * This causes the newly-created lwp to go directly to user level with
 * an apparent return value of 0 from fork(), while the parent lwp
 * returns normally.
 *
 * l1 is the lwp being forked; if l1 == &lwp0, we are creating a
 * kernel thread, and the return path and argument are specified with
 * `func' and `arg'.
 *
 * If an alternate user-level stack is requested (with non-zero values
 * in both the stack and stacksize args), set up the user stack
 * pointer accordingly.
 */
void
cpu_lwp_fork(struct lwp *l1, struct lwp *l2, void *stack,
    size_t stacksize, void (*func)(void *), void *arg)
{
	struct pcb *pcb;
	struct switchframe *sf;

#if 0 /* FIXME: probably wrong for yamt-idlelwp */
	KDASSERT(l1 == curlwp || l1 == &lwp0);
#endif

	sh3_setup_uarea(l2);

	l2->l_md.md_flags = l1->l_md.md_flags;
	l2->l_md.md_astpending = 0;

	/* Copy user context, may be give a different stack */
	memcpy(l2->l_md.md_regs, l1->l_md.md_regs, sizeof(struct trapframe));
	if (stack != NULL)
		l2->l_md.md_regs->tf_r15 = (u_int)stack + stacksize;

	/* When l2 is switched to, jump to the trampoline */
	pcb = lwp_getpcb(l2);
	sf = &pcb->pcb_sf;
	sf->sf_pr  = (int)lwp_trampoline;
	sf->sf_r10 = (int)l2;	/* "new" lwp for lwp_startup() */
	sf->sf_r11 = (int)arg;	/* hook function/argument */
	sf->sf_r12 = (int)func;
}

static void
sh3_setup_uarea(struct lwp *l)
{
	struct pcb *pcb;
	struct trapframe *tf;
	struct switchframe *sf;
	vaddr_t uv, spbase, fptop;
#define	P1ADDR(x)	(SH3_PHYS_TO_P1SEG(*__pmap_kpte_lookup(x) & PG_PPN))

	pcb = lwp_getpcb(l);
	pcb->pcb_onfault = NULL;
	pcb->pcb_faultbail = 0;
#ifdef SH3
	/*
	 * Accessing context store space must not cause exceptions.
	 * SH4 can make wired TLB entries so P3 address for PCB is ok.
	 * SH3 cannot, so we need to convert to P1.  P3/P1 conversion
	 * doesn't cause virtual-aliasing.
	 */
	if (CPU_IS_SH3)
		pcb = (struct pcb *)P1ADDR((vaddr_t)pcb);
#endif /* SH3 */
	l->l_md.md_pcb = pcb;

	/* stack for trapframes */
	fptop = (vaddr_t)pcb + PAGE_SIZE;
	tf = (struct trapframe *)fptop - 1;
	l->l_md.md_regs = tf;

	/* set up the kernel stack pointer */
	uv = uvm_lwp_getuarea(l);
	spbase = uv + PAGE_SIZE;
#ifdef P1_STACK
	/*
	 * wbinv u-area to avoid cache-aliasing, since kernel stack
	 * is accessed from P1 instead of P3.
	 */
	if (SH_HAS_VIRTUAL_ALIAS)
		sh_dcache_wbinv_range(uv, USPACE);
	spbase = P1ADDR(spbase);
#else /* !P1_STACK */
#ifdef SH4
	/* Prepare u-area PTEs */
	if (CPU_IS_SH4)
		sh4_switch_setup(l);
#endif
#endif /* !P1_STACK */

#ifdef KSTACK_DEBUG
	/* Fill magic number for tracking */
	memset((char *)fptop - PAGE_SIZE + sizeof(struct pcb), 0x5a,
	    PAGE_SIZE - sizeof(struct pcb));
	memset((char *)spbase, 0xa5, (USPACE - PAGE_SIZE));
	memset(&pcb->pcb_sf, 0xb4, sizeof(struct switchframe));
#endif /* KSTACK_DEBUG */

	/* Setup kernel stack and trapframe stack */
	sf = &pcb->pcb_sf;
	sf->sf_r6_bank = (vaddr_t)tf;
	sf->sf_r7_bank = spbase + USPACE - PAGE_SIZE;
	sf->sf_r15 = sf->sf_r7_bank;

	/*
	 * Enable interrupts when switch frame is restored, since
	 * kernel thread begins to run without restoring trapframe.
	 */
	sf->sf_sr = PSL_MD;	/* kernel mode, interrupt enable */
}


/*
 * fork &co pass this routine to newlwp to finish off child creation
 * (see cpu_lwp_fork above and lwp_trampoline for details).
 *
 * When this function returns, new lwp returns to user mode.
 */
void
child_return(void *arg)
{
	struct lwp *l = arg;
	struct trapframe *tf = l->l_md.md_regs;

	tf->tf_r0 = 0;		/* fork(2) returns 0 in child */
	tf->tf_ssr |= PSL_TBIT; /* syscall succeeded */

	userret(l);
	ktrsysret(SYS_fork, 0, 0);
}

/*
 * Process the tail end of a posix_spawn() for the child.
 */
void
cpu_spawn_return(struct lwp *l)
{

	userret(l);
}
 
/*
 * struct emul e_startlwp (for _lwp_create(2))
 */
void
startlwp(void *arg)
{
	ucontext_t *uc = arg;
	lwp_t *l = curlwp;
	int error __diagused;

	error = cpu_setmcontext(l, &uc->uc_mcontext, uc->uc_flags);
	KASSERT(error == 0);

	kmem_free(uc, sizeof(ucontext_t));
	userret(l);
}

/*
 * Exit hook
 */
void
cpu_lwp_free(struct lwp *l, int proc)
{

	/* Nothing to do */
}


/*
 * lwp_free() hook
 */
void
cpu_lwp_free2(struct lwp *l)
{

	/* Nothing to do */
}

/*
 * Map an IO request into kernel virtual address space.  Requests fall into
 * one of five catagories:
 *
 *	B_PHYS|B_UAREA:	User u-area swap.
 *			Address is relative to start of u-area.
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

int
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
	bp->b_data = (void *)(taddr + off);
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

	return 0;
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

int
mm_md_physacc(paddr_t pa, vm_prot_t prot)
{

	if (atop(pa) < uvm_physseg_get_start(uvm_physseg_get_first()) || PHYS_TO_VM_PAGE(pa) != NULL) {
		return 0;
	}
	return EFAULT;
}

int
mm_md_kernacc(void *ptr, vm_prot_t prot, bool *handled)
{
	const vaddr_t va = (vaddr_t)ptr;

	if (va < SH3_P1SEG_BASE) {
		return EFAULT;
	}
	if (va < SH3_P2SEG_BASE) {
		*handled = true;
		return 0;
	}
	if (va < SH3_P3SEG_BASE) {
		return EFAULT;
	}
	*handled = false;
	return 0;
}

bool
mm_md_direct_mapped_io(void *ptr, paddr_t *paddr)
{
	vaddr_t va = (vaddr_t)ptr;

	if (va >= SH3_P1SEG_BASE && va < SH3_P2SEG_BASE) {
		*paddr = SH3_P1SEG_TO_PHYS(va);
		return true;
	}
	return false;
}

bool
mm_md_direct_mapped_phys(paddr_t paddr, vaddr_t *vaddr)
{

	*vaddr = SH3_PHYS_TO_P1SEG(paddr);
	return true;
}
