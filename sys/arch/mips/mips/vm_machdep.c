/*	$NetBSD: vm_machdep.c,v 1.22 1997/06/21 04:24:45 mhitch Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/user.h>
#include <sys/core.h>
#include <sys/exec.h>

#include <mips/locore.h>
#include <mips/pte.h>
#include <machine/cpu.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>

extern struct proc *fpcurproc;			/* trap.c */

extern void savefpregs __P((struct proc *));

extern vm_offset_t kvtophys __P((vm_offset_t kva));	/* XXX */

/*
 * cpu_fork() now returns just once.
 */
void
cpu_fork(p1, p2)
	struct proc *p1, *p2;
{
	struct pcb *pcb;
	pt_entry_t *pte;
	struct frame *tf;
	int i;
	extern void child_return __P((void));		/* trap.c */

	tf = (struct frame *)(KERNELSTACK - 24);
	p2->p_md.md_regs = p2->p_addr->u_pcb.pcb_regs;
	p2->p_md.md_flags = p1->p_md.md_flags & MDP_FPUSED;

#ifdef MIPS3
	if (CPUISMIPS3)
		mips3_HitFlushDCache((vm_offset_t)p2->p_addr, UPAGES * NBPG);
#endif

	/* XXX save pte mask outside loop ? */
	for (i = 0, pte = kvtopte(p2->p_addr); i < UPAGES; i++, pte++) {
		if (CPUISMIPS3)
			p2->p_md.md_upte[i] = pte->pt_entry &
		            ~(MIPS3_PG_G | MIPS3_PG_RO | MIPS3_PG_WIRED);
		else
			p2->p_md.md_upte[i] = pte->pt_entry &~ MIPS1_PG_G;
	}
	
	pcb = &p2->p_addr->u_pcb;
	if (p1 == fpcurproc)
		savefpregs(p1);
	*pcb = p1->p_addr->u_pcb;
	pcb->pcb_segtab = (void *)p2->p_vmspace->vm_map.pmap->pm_segtab;
	pcb->pcb_context[10] = (int)proc_trampoline;	/* RA */
	pcb->pcb_context[8] = (int)tf;			/* SP */
	pcb->pcb_context[0] = (int)child_return;	/* S0 */
	pcb->pcb_context[1] = (int)p2;			/* S1 */
}

void
cpu_set_kpc(p, pc)
	struct proc *p;
	void (*pc) __P((struct proc *));
{
	p->p_addr->u_pcb.pcb_context[0] = (int)pc;	/* S0 */
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
	register struct user *up = p->p_addr;
	register pt_entry_t *pte;
	register int i;

	/*
	 * Cache the PTEs for the user area in the machine dependent
	 * part of the proc struct so cpu_switch() can quickly map in
	 * the user struct and kernel stack.
	 */
	pte = kvtopte(up);
	for (i = 0; i < UPAGES; i++) {
		if (CPUISMIPS3)
			p->p_md.md_upte[i] = pte->pt_entry &
			    ~(MIPS3_PG_G | MIPS3_PG_RO | MIPS3_PG_WIRED);
		else
			p->p_md.md_upte[i] = pte->pt_entry & ~MIPS1_PG_G;
		pte++;
	}
}

/*
 * cpu_exit is called as the last action during exit.
 * We release the address space of the process, block interrupts,
 * and call switch_exit.  switch_exit switches to nullproc's PCB and stack,
 * then jumps into the middle of cpu_switch, as if it were switching
 * from nullproc.
 */
void
cpu_exit(p)
	struct proc *p;
{
	if (fpcurproc == p)
		fpcurproc = (struct proc *)0;

	vmspace_free(p->p_vmspace);

	cnt.v_swtch++;
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

	CORE_SETMAGIC(*chdr, COREMAGIC, MID_MIPS, 0);
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
		bzero((caddr_t)&cpustate.fpregs, sizeof(struct fpreg));

	CORE_SETMAGIC(cseg, CORESEGMAGIC, MID_MIPS, CORE_CPU);
	cseg.c_addr = 0;
	cseg.c_size = chdr->c_cpusize;
	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)&cseg, chdr->c_seghdrsize,
	    (off_t)chdr->c_hdrsize, UIO_SYSSPACE,
	    IO_NODELOCKED|IO_UNIT, cred, (int *)NULL, p);
	if (error)
		return error;

	error = vn_rdwr(UIO_WRITE, vp,
			(caddr_t)(&(p -> p_addr -> u_pcb.pcb_regs)),
			(off_t)chdr -> c_cpusize,
	    		(off_t)(chdr->c_hdrsize + chdr->c_seghdrsize),
			UIO_SYSSPACE, IO_NODELOCKED|IO_UNIT,
			cred, (int *)NULL, p);

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
		mips3_HitFlushDCache((vm_offset_t)from, size);
	}
#endif
	while (size > 0) {
		MachTLBFlushAddr((vm_offset_t)from);
		MachTLBUpdate((vm_offset_t)to, fpte->pt_entry);
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
 * Map an IO request into kernel virtual address space.
 *
 * Called by physio() in kern/kern_physio.c for raw device I/O
 * between user address and device driver bypassing filesystem cache.
 */
void
vmapbuf(bp, len)
	struct buf *bp;
	vm_size_t len;
{
	vm_offset_t faddr, taddr, off;
	pt_entry_t *fpte, *tpte;
	pt_entry_t *pmap_pte __P((pmap_t, vm_offset_t));
	register u_int pt_mask;

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
	bp->b_saveaddr = NULL;
}

/*XXX*/

/*
 * Map a (kernel) virtual address to a physical address.
 * There are four cases: 
 * A kseg0 kernel "virtual address" for the   cached physical address space;
 * A kseg1 kernel "virtual address" for the uncached physical address space;
 * A kseg2 normal kernel "virtual address" for the kernel stack or
 *   "u area".  These ARE NOT necessarily in sysmap, since processes 0
 *    and 1 are handcrafted before the sysmap is set up.
 * A kseg2 normal kernel "virtual address" mapped via the TLB, which
 *   IS NOT in Sysmap (eg., an mbuf).
 * The first two are so cheap they could just be macros. The last two
 * overlap, so we must check for UADDR pages first.
 *
 * XXX the double-mapped u-area holding the current process's kernel stack
 * and u-area at a fixed address should be fixed.
 */
vm_offset_t
kvtophys(vm_offset_t kva)
{
	pt_entry_t *pte;
	vm_offset_t phys;

        if (kva >= MACH_CACHED_MEMORY_ADDR && kva < MACH_UNCACHED_MEMORY_ADDR)
	{
		return (MACH_CACHED_TO_PHYS(kva));
	}
	else if (kva >= MACH_UNCACHED_MEMORY_ADDR && kva < MACH_KSEG2_ADDR) {
		return (MACH_UNCACHED_TO_PHYS(kva));
	}
	else if (kva >= UADDR && kva < KERNELSTACK) {
		int upage = (kva - UADDR) >> PGSHIFT;

		pte = (pt_entry_t *)&curproc->p_md.md_upte[upage];
		phys = pfn_to_vad(pte->pt_entry) | (kva & PGOFSET);
	}
	else if (kva >= MACH_KSEG2_ADDR /*&& kva < VM_MAX_KERNEL_ADDRESS*/) {
		pte = kvtopte(kva);

		if ((pte - Sysmap) > Sysmapsize)  {
			printf("oops: Sysmap overrun, max %d index %d\n",
			       Sysmapsize, pte - Sysmap);
		}
		if (!mips_pg_v(pte->pt_entry)) {
			printf("kvtophys: pte not valid for %lx\n", kva);
		}
		phys = pfn_to_vad(pte->pt_entry) | (kva & PGOFSET);
#ifdef DEBUG_VIRTUAL_TO_PHYSICAL
		printf("kvtophys: kv %p, phys %x", kva, phys);
#endif
	}
	else {
		printf("Virtual address %lx: cannot map to physical\n",
		       kva);
                phys = 0;
		/*panic("non-kernel address to kvtophys\n");*/
		return(kva); /* XXX -- while debugging ASC */
        }
        return(phys);
}
