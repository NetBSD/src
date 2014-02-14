/*	vm_machdep.c,v 1.121.6.1.2.19 2011/04/29 08:26:31 matt Exp	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "vm_machdep.c,v 1.121.6.1.2.19 2011/04/29 08:26:31 matt Exp");

#include "opt_ddb.h"
#include "opt_coredump.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/cpu.h>
#include <sys/vnode.h>
#include <sys/core.h>
#include <sys/exec.h>
#include <sys/sa.h>
#include <sys/savar.h>

#include <uvm/uvm.h>

#include <mips/cache.h>
#include <mips/pcb.h>
#include <mips/regnum.h>
#include <mips/locore.h>
#include <mips/pte.h>
#include <mips/psl.h>

paddr_t kvtophys(vaddr_t);	/* XXX */

size_t
mips_page_to_pggroup(struct vm_page *pg, size_t ncolors)
{
	const paddr_t pa = VM_PAGE_TO_PHYS(pg);
	const u_int color = VM_PGCOLOR_BUCKET(pg);
	u_int lcv;
#if VM_NFREELIST == 1
	return color;
#else
	CTASSERT(VM_FREELIST_DEFAULT == 0);
	if (mips_nfreelist == 1)
		return color;

#ifdef VM_FREELIST_FIRST512M
	if (VM_FREELIST_FIRST512M_P(pa)
	    && (mips_freelist_mask & (1 << VM_FREELIST_FIRST512M))) {
		lcv = VM_FREELIST_FIRST512M;
	} else
#ifdef ENABLE_MIPS_KSEGX
	if (mips_ksegx_start <= pa && pa < mips_ksegx_start + VM_KSEGX_SIZE
	    && (mips_freelist_mask & (1 << VM_FREELIST_FIRST512M))) {
		lcv = VM_FREELIST_FIRST512M;
	} else
#endif
#endif
#ifdef VM_FREELIST_FIRST4G
	if (VM_FREELIST_FIRST4G_P(pa)
	    && (mips_freelist_mask & (1 << VM_FREELIST_FIRST4G))) {
		lcv = VM_FREELIST_FIRST4G;
	} else
#endif
	{
		lcv = VM_FREELIST_DEFAULT;
	}
	KDASSERT(lcv == uvm_page_lookup_freelist(pg));
	KASSERT((1 << lcv) & mips_freelist_mask);
	return lcv * ncolors + color;
#endif
}

/*
 * cpu_lwp_fork: Finish a fork operation, with lwp l2 nearly set up.
 * Copy and update the pcb and trapframe, making the child ready to run.
 *
 * First LWP (l1) is the lwp being forked.  If it is &lwp0, then we are
 * creating a kthread, where return path and argument are specified
 * with `func' and `arg'.
 *
 * Rig the child's kernel stack so that it will start out in lwp_trampoline()
 * and call child_return() with l2 as an argument. This causes the
 * newly-created child process to go directly to user level with an apparent
 * return value of 0 from fork(), while the parent process returns normally.
 *
 * If an alternate user-level stack is requested (with non-zero values
 * in both the stack and stacksize arguments), then set up the user stack
 * pointer accordingly.
 */
void
cpu_lwp_fork(struct lwp *l1, struct lwp *l2, void *stack, size_t stacksize,
    void (*func)(void *), void *arg)
{
	struct pcb * const pcb1 = lwp_getpcb(l1);
	struct pcb * const pcb2 = lwp_getpcb(l2);
	struct trapframe *tf;

	KASSERT(l1 == curlwp || l1 == &lwp0);

	l2->l_md.md_ss_addr = 0;
	l2->l_md.md_ss_instr = 0;
	l2->l_md.md_astpending = 0;

#ifndef NOFPU
	/* If parent LWP was using FPU, then save the FPU h/w state. */
	fpu_save_lwp(l1);
#endif
#ifndef __mips_o32
	KASSERT(pcb1->pcb_context.val[_L_SR] & MIPS_SR_KX);
#endif

	/* Copy the PCB from parent. */
	*pcb2 = *pcb1;

	/*
	 * Copy the trapframe from parent, so that return to userspace
	 * will be to right address, with correct registers.
	 */
	vaddr_t ua2 = (vaddr_t)l2->l_addr;
	tf = (struct trapframe *)(ua2 + USPACE) - 1;
	*tf = *l1->l_md.md_utf;

	/* If specified, set a different user stack for a child. */
	if (stack != NULL)
		tf->tf_regs[_R_SP] = (intptr_t)stack + stacksize;

	l2->l_md.md_utf = tf;
	l2->l_md.md_flags = l1->l_md.md_flags & MDP_FPUSED;

	if (!mm_md_direct_mapped_virt(ua2, NULL, NULL)) {
		pt_entry_t * const pte = pmap_pte_lookup(pmap_kernel(), ua2);
		const uint32_t x = (MIPS_HAS_R4K_MMU) ?
		    (MIPS3_PG_G | MIPS3_PG_RO | MIPS3_PG_WIRED) : MIPS1_PG_G;

		for (u_int i = 0; i < UPAGES; i++) {
			l2->l_md.md_upte[i] = pte[i].pt_entry &~ x;
		}
	}

	cpu_setfunc(l2, func, arg);
}

void
cpu_setfunc(struct lwp *l, void (*func)(void *), void *arg)
{
	struct pcb * const pcb = lwp_getpcb(l);
	struct trapframe * const tf = l->l_md.md_utf;

	KASSERT(tf == (struct trapframe *)((char *)l->l_addr + USPACE) - 1);

	/*
	 * Rig kernel stack so that it would start out in lwp_trampoline()
	 * and call child_return() with l as an argument.  This causes the
	 * newly-created child process to go directly to user level with a
	 * parent return value of 0 from fork(), while the parent process
	 * returns normally.
	 */

	pcb->pcb_context.val[_L_S0] = (intptr_t)func;			/* S0 */
	pcb->pcb_context.val[_L_S1] = (intptr_t)arg;			/* S1 */
	pcb->pcb_context.val[MIPS_CURLWP_LABEL] = (intptr_t)l;		/* T8 */
	pcb->pcb_context.val[_L_SP] = (intptr_t)tf;			/* SP */
	pcb->pcb_context.val[_L_RA] =
	   mips_locore_jumpvec.ljv_lwp_trampoline;			/* RA */
#ifndef __mips_n32
	KASSERT(pcb->pcb_context.val[_L_SR] & MIPS_SR_KX);
#endif
	KASSERT(pcb->pcb_context.val[_L_SR] & MIPS_SR_INT_IE);
}

/*
 * Routine to copy MD stuff from proc to proc on a fork.
 * For mips, this is the ABI and "32 bit process on a 64 bit kernel" flag.
 */
void
cpu_proc_fork(struct proc *p1, struct proc *p2)
{
	p2->p_md.md_abi = p1->p_md.md_abi;
}

static struct evcnt uarea_remapped = 
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "uarea", "remapped");
static struct evcnt uarea_reallocated = 
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "uarea", "reallocated");
EVCNT_ATTACH_STATIC(uarea_remapped);
EVCNT_ATTACH_STATIC(uarea_reallocated);

void
cpu_uarea_remap(struct lwp *l)
{
	bool uarea_ok;
	vaddr_t va;
	paddr_t pa;
	struct pcb *pcb = lwp_getpcb(l);

	/*
	 * Grab the starting physical address of the uarea.
	 */
	va = (vaddr_t)l->l_addr;

	/*
	 * If already direct mapped, we're done!
	 */
	if (mm_md_direct_mapped_phys(va, NULL, NULL))
		return;

	if (!pmap_extract(pmap_kernel(), va, &pa))
		panic("%s: pmap_extract(%#"PRIxVADDR") failed", __func__, va);

	/*
	 * Check to see if the existing uarea is physically contiguous.
	 */
	uarea_ok = true;
	for (vaddr_t i = PAGE_SIZE; uarea_ok && i < USPACE; i += PAGE_SIZE) {
		paddr_t pa0;
		if (!pmap_extract(pmap_kernel(), va + i, &pa0))
			panic("%s: pmap_extract(%#"PRIxVADDR") failed",
			    __func__, va+1);
		uarea_ok = (pa0 - pa == i);
	}

#ifndef _LP64
	/*
	 * If this is a 32bit kernel, it needs to be mappedable via KSEG0
	 */
	uarea_ok = uarea_ok && (pa + USPACE - 1 <= MIPS_PHYS_MASK);
#endif
	KASSERTMSG(pcb->pcb_context.val[_L_SP] == (intptr_t)l->l_md.md_utf,
	    "%s: %s (%#"PRIxREGISTER") != %s (%p)",
	     __func__,
	    "pcb->pcb_context.val[_L_SP]", pcb->pcb_context.val[_L_SP],
	    "(intptr_t)l->l_md.md_utf", l->l_md.md_utf);

	if (!uarea_ok) {
		struct pglist pglist;
#ifdef _LP64
		const paddr_t high = mips_avail_end;
#else
		const paddr_t high = MIPS_KSEG1_START - MIPS_KSEG0_START;
#endif
		int error;

		/*
		 * Allocate a new physically contiguou uarea which can be
		 * direct-mapped.
		 */
		error = uvm_pglistalloc(USPACE, mips_avail_start, high,
		    USPACE_ALIGN, 0, &pglist, 1, 1);
		if (error)
			panic("%s: uvm_pglistalloc failed: %d", __func__,
			    error);

		/*
		 * Get the physical address from the first page.
		 */
		pa = VM_PAGE_TO_PHYS(TAILQ_FIRST(&pglist));
	}

	/*
	 * Now set the new uarea (if it's different). If l->l_addr was already
	 * direct mapped address then this routine really won't change anything
	 * but that's not probable so don't micro optimize for it.
	 */
#ifdef _LP64
	va = MIPS_PHYS_TO_XKPHYS_CACHED(pa);
#else
	va = MIPS_PHYS_TO_KSEG0(pa);
#endif
	if (!uarea_ok) {
		/*
		 * Copy the trapframe and pcb from the old uarea to the new.
		 */
		((struct trapframe *)(va + USPACE))[-1] = *l->l_md.md_utf;
		*(struct pcb *)va = *pcb;
		/*
		 * Discard the old uarea.
		 */
		uvm_uarea_free(USER_TO_UAREA(l->l_addr), curcpu());
		uarea_reallocated.ev_count++;
	}

	l->l_addr = (struct user *)va;
	l->l_md.md_utf = (struct trapframe *)(va + USPACE) - 1;
	pcb = lwp_getpcb(l);
	pcb->pcb_context.val[_L_SP] = (vaddr_t)l->l_md.md_utf;
	uarea_remapped.ev_count++;
}

/*
 * Finish a swapin operation.
 * We neded to update the cached PTEs for the user area in the
 * machine dependent part of the proc structure.
 */
void
cpu_swapin(struct lwp *l)
{
	vaddr_t kva = (vaddr_t) lwp_getpcb(l);

	if (mm_md_direct_mapped_virt(kva, NULL, NULL))
		return;

	/*
	 * Cache the PTEs for the user area in the machine dependent
	 * part of the proc struct so cpu_switchto() can quickly map
	 * in the user struct and kernel stack.
	 */
	uint32_t x = (MIPS_HAS_R4K_MMU) ?
	    (MIPS3_PG_G | MIPS3_PG_RO | MIPS3_PG_WIRED) :
	    MIPS1_PG_G;
	pt_entry_t * const pte = pmap_pte_lookup(pmap_kernel(), kva);
	for (size_t i = 0; i < UPAGES; i++)
		l->l_md.md_upte[i] = pte[i].pt_entry &~ x;
}

void
cpu_lwp_free(struct lwp *l, int proc)
{
	KASSERT(l == curlwp);

#ifndef NOFPU
	fpu_discard();

	KASSERT(l->l_fpcpu == NULL);
	KASSERT(curcpu()->ci_fpcurlwp != l);
#endif
}

vaddr_t
cpu_lwp_pc(struct lwp *l)
{
	return l->l_md.md_utf->tf_regs[_R_PC];
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
		struct trapframe frame;
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

	fpu_save_lwp(l);

	struct pcb * const pcb = lwp_getpcb(l);
	cpustate.frame = *l->l_md.md_utf;
	cpustate.fpregs = pcb->pcb_fpregs;

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
	kva = uvm_km_alloc(phys_map, len, atop(uva) & uvmexp.colormask,
	    UVM_FLAG_COLORMATCH | UVM_KMF_VAONLY | UVM_KMF_WAITVA);
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
	paddr_t phys;

#if defined(MIPS3_PLUS) && !defined(_LP64)
	/*
	 * When we dumping memory in a crash dump, we try to use a large
	 * TLB entry to reduce the TLB trashing.
	 */
	if (__predict_false(mips_kcore_window_vtophys(kva, &phys)))
		return phys;
#endif

	/*
	 * If the KVA is direct mapped, we're done!
	 */
	if (mm_md_direct_mapped_virt(kva, &phys, NULL))
		return phys;

	if (VM_MIN_KERNEL_ADDRESS <= kva && kva < VM_MAX_KERNEL_ADDRESS) {
		pt_entry_t *pte = pmap_pte_lookup(pmap_kernel(), kva);
		if (pte != NULL) {
			if (!mips_pg_v(pte->pt_entry)) {
				printf("%s: pte not valid for %#"PRIxVADDR"\n",
				    __func__, kva);
			}
			phys = mips_tlbpfn_to_paddr(pte->pt_entry)
			    | (kva & PGOFSET);
			return phys;
		}
	}

	panic("%s: Virtual address %#"PRIxVADDR": cannot map to physical\n",
	    __func__, kva);
}
