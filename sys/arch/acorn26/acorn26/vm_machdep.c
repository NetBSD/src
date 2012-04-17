/* $NetBSD: vm_machdep.c,v 1.27.4.1 2012/04/17 00:05:51 yamt Exp $ */

/*-
 * Copyright (c) 2000, 2001 Ben Harris
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vm_machdep.c,v 1.27.4.1 2012/04/17 00:05:51 yamt Exp $");

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/mount.h> /* XXX syscallargs.h uses fhandle_t and fsid_t */
#include <sys/proc.h>
#include <sys/syscallargs.h>
#include <sys/sched.h>
#include <sys/mutex.h>

#include <uvm/uvm_extern.h>

#include <arm/armreg.h>
#include <machine/frame.h>
#include <machine/intr.h>
#include <machine/machdep.h>

/*
 * Finish a fork operation, with thread l2 nearly set up.
 * Copy and update the pcb and trap frame, making the child ready to run.
 *
 * l1 is the thread being forked; if l1 == &lwp0, we are creating
 * a kernel thread, and the return path and argument are specified with
 * `func' and `arg'.
 *
 * If an alternate user-level stack is requested (with non-zero values
 * in both the stack and stacksize args), set up the user stack pointer
 * accordingly.
 */

/*
 * Note: the pcb structure has to be at the start of the uarea -- we start
 * the kernel stack from the end.
 */

void
cpu_lwp_fork(struct lwp *l1, struct lwp *l2, void *stack, size_t stacksize,
    void (*func)(void *), void *arg)
{
	struct pcb *pcb1, *pcb2;
	struct trapframe *tf;
	struct switchframe *sf;

#if 0
	printf("cpu_lwp_fork: %p -> %p\n", p1, p2);
#endif
	pcb1 = lwp_getpcb(l1);
	pcb2 = lwp_getpcb(l2);

	/* Copy the pcb */
	*pcb2 = *pcb1;

	/* pmap_activate(l2); XXX Other ports do.  Why?  */

	/* Set up the kernel stack */
	tf = (struct trapframe *)(uvm_lwp_getuarea(l2) + USPACE) - 1;
	sf = (struct switchframe *)tf - 1;
	/* Duplicate old process's trapframe (if it had one) */
	if (pcb1->pcb_tf == NULL)
		memset(tf, 0, sizeof(*tf));
	else
		*tf = *pcb1->pcb_tf;
	/* If specified, give the child a different stack. */
	if (stack != NULL)
		tf->tf_usr_sp = (u_int)stack + stacksize;
	pcb2->pcb_tf = tf;
	/* Fabricate a new switchframe */
	memset(sf, 0, sizeof(*sf));

	sf->sf_r13 = (register_t)tf; /* Initial stack pointer */
	sf->sf_pc  = (register_t)lwp_trampoline | R15_MODE_SVC;

	pcb2->pcb_tf = tf;
	pcb2->pcb_sf = sf;
	pcb2->pcb_onfault = NULL;
	sf->sf_r4 = (register_t)func;
	sf->sf_r5 = (register_t)arg;
}

void
cpu_lwp_free(struct lwp *l, int proc)
{

	/* Nothing to do here? */
}

void
cpu_lwp_free2(struct lwp *l)
{

	/* Nothing to do here? */
}

/*
 * Map a user I/O request into kernel virtual address space.
 * Note: the pages are already locked by uvm_vslock(), so we
 * do not need to pass an access_type to pmap_enter().
 */
/* This code was originally stolen from the alpha port. */
int
vmapbuf(struct buf *bp, vsize_t len)
{
	vaddr_t faddr, taddr, off;
	paddr_t pa;
	struct proc *p;
	vm_prot_t prot;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vmapbuf");
	p = bp->b_proc;
	bp->b_saveaddr = bp->b_data;
	faddr = trunc_page((vaddr_t)bp->b_data);
	off = (vaddr_t)bp->b_data - faddr;
	len = round_page(off + len);
	taddr = uvm_km_alloc(phys_map, len, 0, UVM_KMF_VAONLY | UVM_KMF_WAITVA);
	bp->b_data = (void *)(taddr + off);
	len = atop(len);
	prot = bp->b_flags & B_READ ? VM_PROT_READ | VM_PROT_WRITE :
				      VM_PROT_READ;
	while (len--) {
		if (pmap_extract(vm_map_pmap(&p->p_vmspace->vm_map), faddr,
		    &pa) == false)
			panic("vmapbuf: null page frame");
		pmap_enter(vm_map_pmap(phys_map), taddr, trunc_page(pa),
		    prot, prot | PMAP_WIRED);
		faddr += PAGE_SIZE;
		taddr += PAGE_SIZE;
	}
	pmap_update(vm_map_pmap(phys_map));

	return 0;
}

/*
 * Unmap a previously-mapped user I/O request.
 */
void
vunmapbuf(struct buf *bp, vsize_t len)
{
	vaddr_t addr, off;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vunmapbuf");
	addr = trunc_page((vaddr_t)bp->b_data);
	off = (vaddr_t)bp->b_data - addr;
	len = round_page(off + len);
	pmap_remove(vm_map_pmap(phys_map), addr, addr + len);
	pmap_update(vm_map_pmap(phys_map));
	uvm_km_free(phys_map, addr, len, UVM_KMF_VAONLY);
	bp->b_data = bp->b_saveaddr;
	bp->b_saveaddr = NULL;
}
