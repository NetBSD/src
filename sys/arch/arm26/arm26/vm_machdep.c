/* $NetBSD: vm_machdep.c,v 1.18.2.1 2001/10/01 12:37:45 fvdl Exp $ */

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
/* Following is for vmapbuf/vunmapbuf */
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
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */

#include <sys/param.h>

__RCSID("$NetBSD: vm_machdep.c,v 1.18.2.1 2001/10/01 12:37:45 fvdl Exp $");

#include <sys/buf.h>
#include <sys/mount.h> /* XXX syscallargs.h uses fhandle_t and fsid_t */
#include <sys/proc.h>
#include <sys/syscallargs.h>
#include <sys/user.h>

#include <uvm/uvm_extern.h>

#include <arm/armreg.h>
#include <machine/frame.h>
#include <machine/intr.h>
#include <machine/machdep.h>

extern struct vm_map *phys_map; /* XXX where? */

/*
 * Finish a fork operation, with process p2 nearly set up.
 * Copy and update the pcb and trap frame, making the child ready to run.
 *
 * p1 is the process being forked; if p1 == &proc0, we are creating
 * a kernel thread, and the return path and argument are specified with
 * `func' and `arg'.
 *
 * If an alternate user-level stack is requested (with non-zero values
 * in both the stack and stacksize args), set up the user stack pointer
 * accordingly.
 */

/*
 * Note:
 * 
 * p->p_addr points to a page containing the user structure
 * (see <sys/user.h>) and the kernel stack.  The user structure has to be
 * at the start of the area -- we start the kernel stack from the end.
 */

void
cpu_fork(struct proc *p1, struct proc *p2, void *stack, size_t stacksize,
    void (*func)(void *), void *arg)
{
	struct pcb *pcb;
	struct trapframe *tf;
	struct switchframe *sf;
	char *stacktop;

#if 0
	printf("cpu_fork: %p -> %p\n", p1, p2);
#endif
	pcb = &p2->p_addr->u_pcb;
	/* Copy the pcb */
	*pcb = p1->p_addr->u_pcb;

	/* pmap_activate(p2); XXX Other ports do.  Why?  */

	/* Set up the kernel stack */
	stacktop = (char *)p2->p_addr + USPACE;
	tf = (struct trapframe *)stacktop - 1;
	sf = (struct switchframe *)tf - 1;
	/* Duplicate old process's trapframe (if it had one) */
	if (p1->p_addr->u_pcb.pcb_tf == NULL)
		bzero(tf, sizeof(*tf));
	else
		*tf = *p1->p_addr->u_pcb.pcb_tf;
	p2->p_addr->u_pcb.pcb_tf = tf;
	/* Fabricate a new switchframe */
	bzero(sf, sizeof(*sf));
	sf->sf_r13 = (register_t)tf; /* Initial stack pointer */
	sf->sf_r14 = (register_t)proc_trampoline | R15_MODE_SVC;

	pcb->pcb_tf = tf;
	pcb->pcb_sf = sf;
	pcb->pcb_onfault = NULL;
	sf->sf_r4 = (register_t)func;
	sf->sf_r5 = (register_t)arg;
}

void
cpu_exit(struct proc *p)
{
	int s;

	/* Nothing to do here? */
	exit2(p); /* I think this is safe on a uniprocessor machine */
	SCHED_LOCK(s);		/* expected by cpu_switch */
	cpu_switch(p);
}

void
cpu_swapin(struct proc *p)
{

	/* Can anyone think of anything I should do here? */
}

void
cpu_swapout(struct proc *p)
{

	/* ... or here, for that matter. */
}

/*
 * Map a user I/O request into kernel virtual address space.
 * Note: the pages are already locked by uvm_vslock(), so we
 * do not need to pass an access_type to pmap_enter().
 */
/* This code was originally stolen from the alpha port. */
void
vmapbuf(struct buf *bp, vsize_t len)
{
	vaddr_t faddr, taddr, off;
	paddr_t pa;
	struct proc *p;
	vm_prot_t prot;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vmapbuf");
	p = bp->b_proc;
	faddr = trunc_page((vaddr_t)bp->b_saveaddr = bp->b_data);
	off = (vaddr_t)bp->b_data - faddr;
	len = round_page(off + len);
	taddr = uvm_km_valloc_wait(phys_map, len);
	bp->b_data = (caddr_t)(taddr + off);
	len = atop(len);
	prot = bp->b_flags & B_READ ? VM_PROT_READ | VM_PROT_WRITE :
				      VM_PROT_READ;
	while (len--) {
		if (pmap_extract(vm_map_pmap(&p->p_vmspace->vm_map), faddr,
		    &pa) == FALSE)
			panic("vmapbuf: null page frame");
		pmap_enter(vm_map_pmap(phys_map), taddr, trunc_page(pa),
		    prot, prot | PMAP_WIRED);
		faddr += PAGE_SIZE;
		taddr += PAGE_SIZE;
	}
	pmap_update(vm_map_pmap(phys_map));
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
	uvm_km_free_wakeup(phys_map, addr, len);
	bp->b_data = bp->b_saveaddr;
	bp->b_saveaddr = NULL;
}
