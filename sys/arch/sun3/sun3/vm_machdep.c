/*	$NetBSD: vm_machdep.c,v 1.25 1995/04/03 22:06:11 gwr Exp $	*/

/*
 * Copyright (c) 1994 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass 
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	from: Utah $Hdr: vm_machdep.c 1.21 91/04/06$
 *	from: @(#)vm_machdep.c	7.10 (Berkeley) 5/7/91
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
#include <vm/vm_map.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/pte.h>
#include <machine/pmap.h>

extern int fpu_type;

/*
 * Finish a fork operation, with process p2 nearly set up.
 * Copy and update the kernel stack and pcb, making the child
 * ready to run, and marking it so that it can return differently
 * than the parent.  Returns 1 in the child process, 0 in the parent.
 * We currently double-map the user area so that the stack is at the same
 * address in each process; in the future we will probably relocate
 * the frame pointers on the stack after copying.
 */
cpu_fork(p1, p2)
	register struct proc *p1, *p2;
{
	/* kernel virtual address of new u-area */
	register struct user *up = p2->p_addr;
	int offset;
	extern caddr_t getsp();
	extern char kstack[];

	/* copy over the machdep part of struct proc */
	p2->p_md.md_regs = p1->p_md.md_regs;
	p2->p_md.md_flags = p1->p_md.md_flags & ~MDP_HPUXTRACE;

	/*
	 * Copy pcb and stack from proc p1 to p2. 
	 * We do this as cheaply as possible, copying only the active
	 * part of the stack.  The stack and pcb need to agree;
	 * this is tricky, as the final pcb is constructed by savectx,
	 * but its frame isn't yet on the stack when the stack is copied.
	 * cpu_switch compensates for this when the child eventually runs.
	 * This should be done differently, with a single call
	 * that copies and updates the pcb+stack,
	 * replacing the bcopy and savectx.
	 *
	 * XXX - Need to flush cache for kstack first?
	 * XXX - Note that all proc->p_addr mappings are
	 *       NON-CACHEABLE aliases of kstack...
	 */
	p2->p_addr->u_pcb = p1->p_addr->u_pcb;
	offset = getsp() - kstack;
	bcopy((caddr_t)kstack + offset, (caddr_t)p2->p_addr + offset,
	    (unsigned) ctob(UPAGES) - offset);

	/*
	 * Cache the PTEs for the user area in the machine dependent
	 * part of the proc struct so cpu_switch() can quickly map in
	 * the user struct and kernel stack. Note: if the virtual address
	 * translation changes (e.g. swapout) we have to update this.
	 */
	save_u_area(p2, p2->p_addr);

	/*
	 * Arrange for a non-local goto when the new process
	 * is started, to resume here, returning nonzero from setjmp.
	 */
	if (savectx(up, 1)) {
		/*
		 * Return 1 in child.
		 */
		return (1);
	}
	return (0);
}

/*
 * Finish a swapin operation.
 * We neded to update the cached PTEs for the user area in the
 * machine dependent part of the proc structure.
 */
void
cpu_swapin(p)
	register struct proc *p;
{
	/*
	 * Cache the PTEs for the user area in the machine dependent
	 * part of the proc struct so cpu_switch() can quickly map in
	 * the user struct and kernel stack.
	 */
	save_u_area(p, p->p_addr);
}

/*
 * cpu_exit is called as the last action during exit.
 * We release the address space and machine-dependent resources,
 * including the memory for the user structure and kernel stack.
 * Once finished, we call switch_exit, which switches to a temporary
 * pcb and stack and never returns.  We block memory allocation
 * until switch_exit has made things safe again.
 */
void
cpu_exit(p)
	struct proc *p;
{
	extern volatile void switch_exit();
	vmspace_free(p->p_vmspace);

	(void) splimp();
	kmem_free(kernel_map, (vm_offset_t)p->p_addr, ctob(UPAGES));
	switch_exit(p);
	/* NOTREACHED */
}

/*
 * Dump the machine specific segment at the start of a core dump.
 * This means the CPU and FPU registers.  The format used here is
 * the same one ptrace uses, so gdb can be machine independent.
 *
 * XXX - Generate Sun format core dumps for Sun executables?
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
	int error;
	struct md_core md_core;
	struct coreseg cseg;
	register struct user *up = p->p_addr;
	register i;

	CORE_SETMAGIC(*chdr, COREMAGIC, MID_M68K, 0);
	chdr->c_hdrsize = ALIGN(sizeof(*chdr));
	chdr->c_seghdrsize = ALIGN(sizeof(cseg));
	chdr->c_cpusize = sizeof(md_core);

	/* Save integer registers. */
	{
		register struct frame *f;

		f = (struct frame*) p->p_md.md_regs;
		for (i = 0; i < 16; i++) {
			md_core.intreg.r_regs[i] = f->f_regs[i];
		}
		md_core.intreg.r_sr = f->f_sr;
		md_core.intreg.r_pc = f->f_pc;
	}
	if (fpu_type) {
		register struct fpframe *f;

		f = &up->u_pcb.pcb_fpregs;
		m68881_save(f);
		for (i = 0; i < (8*3); i++) {
			md_core.freg.r_regs[i] = f->fpf_regs[i];
		}
		md_core.freg.r_fpcr  = f->fpf_fpcr;
		md_core.freg.r_fpsr  = f->fpf_fpsr;
		md_core.freg.r_fpiar = f->fpf_fpiar;
	} else {
		bzero((caddr_t)&md_core.freg, sizeof(md_core.freg));
	}

	CORE_SETMAGIC(cseg, CORESEGMAGIC, MID_M68K, CORE_CPU);
	cseg.c_addr = 0;
	cseg.c_size = chdr->c_cpusize;

	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)&cseg, chdr->c_seghdrsize,
	    (off_t)chdr->c_hdrsize, UIO_SYSSPACE,
	    IO_NODELOCKED|IO_UNIT, cred, (int *)NULL, p);
	if (error)
		return error;

	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)&md_core, sizeof(md_core),
	    (off_t)(chdr->c_hdrsize + chdr->c_seghdrsize), UIO_SYSSPACE,
	    IO_NODELOCKED|IO_UNIT, cred, (int *)NULL, p);

	if (!error)
		chdr->c_nseg++;

	return error;
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
 *
 * XXX we allocate KVA space by using kmem_alloc_wait which we know
 * allocates space without backing physical memory.  This implementation
 * is a total crock, the multiple mappings of these physical pages should
 * be reflected in the higher-level VM structures to avoid problems.
 */
vmapbuf(bp)
	register struct buf *bp;
{
	register int npf;
	register caddr_t addr;
	register long flags = bp->b_flags;
	struct proc *p;
	int off;
	vm_offset_t kva;
	register vm_offset_t pa;

	if ((flags & B_PHYS) == 0)
		panic("vmapbuf");
	addr = bp->b_saveaddr = bp->b_data;
	off = (int)addr & PGOFSET;
	p = bp->b_proc;
	npf = btoc(round_page(bp->b_bcount + off));
	kva = kmem_alloc_wait(phys_map, ctob(npf));
	bp->b_data = (caddr_t) (kva + off);
	while (npf--) {
		pa = pmap_extract(vm_map_pmap(&p->p_vmspace->vm_map),
		    (vm_offset_t)addr);
		if (pa == 0)
			panic("vmapbuf: null page frame");
		pmap_enter(vm_map_pmap(phys_map), kva,
			trunc_page(pa) | PMAP_NC,
			VM_PROT_READ|VM_PROT_WRITE, TRUE);
		addr += PAGE_SIZE;
		kva += PAGE_SIZE;
	}
}

/*
 * Free the io map PTEs associated with this IO operation.
 * We also invalidate the TLB entries and restore the original b_addr.
 */
vunmapbuf(bp)
	register struct buf *bp;
{
	register caddr_t addr;
	vm_offset_t pgva;
	register int off, npf;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vunmapbuf");
	addr = bp->b_data;
	off = (int)addr & PGOFSET;
	pgva = (vm_offset_t)((int)addr & ~PGOFSET);

	npf = btoc(round_page(bp->b_bcount + off));
	kmem_free_wakeup(phys_map, pgva, ctob(npf));
	bp->b_data = bp->b_saveaddr;
	bp->b_saveaddr = NULL;
#ifdef	HAVECACHE
	cache_flush(bp->b_data, bp->b_bcount - bp->b_resid);
#endif
}

/*
 * Move pages from one kernel virtual address to another.
 * Both addresses are assumed to reside in the kernel map,
 * and size must be a multiple of CLSIZE.
 */
void pagemove(from, to, size)
	register caddr_t from, to;
	int size;
{
	register vm_offset_t pa;

#ifdef DIAGNOSTIC
	if (size & CLOFSET || (int)from & CLOFSET || (int)to & CLOFSET)
		panic("pagemove 1");
#endif
	while (size > 0) {
		pa = pmap_extract(kernel_pmap, (vm_offset_t)from);
#ifdef DIAGNOSTIC
		if (pa == 0)
			panic("pagemove 2");
#endif
		pmap_remove(kernel_pmap,
			(vm_offset_t)from, (vm_offset_t)from + NBPG);
		pmap_enter(kernel_pmap,
			(vm_offset_t)to, pa, VM_PROT_READ|VM_PROT_WRITE, 1);
		from += NBPG;
		to += NBPG;
		size -= NBPG;
	}
}

/*
 * Allocate actual memory pages in DVMA space.
 * (idea for implementation borrowed from Chris Torek.)
 */
caddr_t dvma_malloc(bytes)
     size_t bytes;
{
    caddr_t new_mem;
    vm_size_t new_size;

    if (!bytes)
		return NULL;
    new_size = sun3_round_page(bytes);
    new_mem = (caddr_t) kmem_alloc(phys_map, new_size);
    if (!new_mem)
		panic("dvma_malloc: no space in phys_map");
    /* The pmap code always makes DVMA pages non-cached. */
    return new_mem;
}

/*
 * Allocate virtual space from the DVMA map,
 * but do not map anything into that space.
 */
caddr_t dvma_vm_alloc(bytes)
     int bytes;
{
	vm_offset_t va;

	if (bytes <= 0 || bytes > DVMA_SPACE_SIZE)
		return NULL;
	if (bytes & PGOFSET)
		panic("dvma_vm_alloc");

	va = kmem_alloc_wait(phys_map, bytes);
    return (caddr_t) va;
}

/*
 * Free virtual space from the DVMA map.
 */
void dvma_vm_free(caddr_t va, int bytes)
{
	kmem_free_wakeup(phys_map, (vm_offset_t)va, bytes);
}

/*
 * Given a DVMA address, return the physical address that
 * would be used by some OTHER bus-master besides the CPU.
 * (Examples: on-board ie/le, VME xy board).
 */
long dvma_kvtopa(long kva)
{
	if (kva < DVMA_SPACE_START || kva >= DVMA_SPACE_END)
		panic("dvma_kvtopa: bad dmva addr=0x%x\n", kva);

	return(kva - DVMA_SLAVE_BASE);
}

/*
 * Given a range of kernel virtual space, remap all the
 * pages found there into the DVMA space (dup mappings).
 * XXX - Perhaps this should take a list of pages...
 */
caddr_t dvma_remap(char *kva, int len)
{
	vm_offset_t phys, dvma;
	caddr_t dvma1st;

	/* Get sufficient DVMA space. */
	dvma1st = dvma_vm_alloc(len);
	if (dvma1st == NULL)
		return(NULL);	/* EAGAIN? */

	dvma = (vm_offset_t) dvma1st;
	while (len > 0) {
		phys = pmap_extract(kernel_pmap, (vm_offset_t)kva);
		if (phys == 0)
			panic("dvma_remap: phys=0");

		/* Duplicate the mapping */
		pmap_enter(kernel_pmap, dvma,
				   phys | PMAP_NC,
				   VM_PROT_ALL, FALSE);

		kva  += NBPG;
		dvma += NBPG;
		len  -= NBPG;
	}

	return(dvma1st);
}

void dvma_unmap(caddr_t dvma_addr, int len)
{
	pmap_remove(kernel_pmap, (vm_offset_t)dvma_addr,
			(vm_offset_t)dvma_addr + len);
	dvma_vm_free(dvma_addr, len);
}

