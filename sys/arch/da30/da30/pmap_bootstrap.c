/* 
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	from: @(#)pmap_bootstrap.c	8.1 (Berkeley) 6/10/93
 *	$Id: pmap_bootstrap.c,v 1.1 1994/06/18 12:10:02 paulus Exp $
 */

#include <sys/param.h>
#include <sys/msgbuf.h>
#include <machine/pte.h>
#include <da30/da30/clockreg.h>
#include <machine/vmparam.h>
#include <machine/cpu.h>

#include <vm/vm.h>

extern char *etext;
extern int Sysptsize;
extern char *extiobase, *proc0paddr;
extern struct ste *Sysseg;
extern struct pte *Sysptmap, *Sysmap;
extern vm_offset_t Umap, RTCbase;

extern int maxmem, physmem;
extern vm_offset_t avail_start, avail_end, virtual_avail, virtual_end;
extern vm_size_t mem_size;
extern int protection_codes[];

/*
 * Special purpose kernel virtual addresses, used for mapping
 * physical pages for a variety of temporary or permanent purposes:
 *
 *	CADDR1, CADDR2:	pmap zero/copy operations
 *	vmmap:		/dev/mem, crash dumps, parity error checking
 *	msgbufp:	kernel message buffer
 */
caddr_t		CADDR1, CADDR2, vmmap;
struct msgbuf	*msgbufp;

/*
 * Bootstrap the VM system.
 *
 * `nextpa' is the first available physical
 * memory address.  Returns an updated first PA reflecting the memory we
 * have allocated.  MMU is still off when we return.
 *
 * XXX assumes sizeof(u_int) == sizeof(struct pte)
 */
void
pmap_bootstrap(nextpa)
	vm_offset_t nextpa;
{
	vm_offset_t kstpa, kptpa, iiopa, eiopa, kptmpa, lkptpa, p0upa;
	u_int nptpages, kstsize;
	register u_int protoste, protopte, *ste, *pte, *epte;

	/*
	 * Calculate important physical addresses:
	 *
	 *	kstpa		kernel segment table	1 page
	 *
	 *	kptpa		statically allocated
	 *			kernel PT pages		Sysptsize+ pages
	 *
	 *	iiopa		internal IO space
	 *			PT pages		IIOMAPSIZE pages
	 *
	 *	eiopa		external IO space
	 *			PT pages		EIOMAPSIZE pages
	 *
	 * [ Sysptsize is the number of pages of PT, IIOMAPSIZE and
	 *   EIOMAPSIZE are the number of PTEs, hence we need to round
	 *   the total to a page boundary with IO maps at the end. ]
	 *
	 *	kptmpa		kernel PT map		1 page
	 *
	 *	lkptpa		last kernel PT page	1 page
	 *
	 *	p0upa		proc 0 u-area		UPAGES pages
	 *
	 * The KVA corresponding to any of these PAs is:
	 *	(PA - firstpa + KERNBASE).
	 */
	kstpa = nextpa;
	nextpa += NBPG;
	kptpa = nextpa;
	nptpages = Sysptsize +
		(IIOMAPSIZE + EIOMAPSIZE + NPTEPG - 1) / NPTEPG;
	nextpa += nptpages * NBPG;
	eiopa = nextpa - EIOMAPSIZE * sizeof(struct pte);
	iiopa = eiopa - IIOMAPSIZE * sizeof(struct pte);
	kptmpa = nextpa;
	nextpa += NBPG;
	lkptpa = nextpa;
	nextpa += NBPG;
	p0upa = nextpa;
	nextpa += UPAGES * NBPG;

	/*
	 * Initialize segment table and kernel page table map.
	 *
	 * On 68030s and earlier MMUs the two are identical except for
	 * the valid bits so both are initialized with essentially the
	 * same values.
	 *
	 * Portions of the last segment of KVA space (0xFFF00000 -
	 * 0xFFFFFFFF) are mapped for a couple of purposes.  0xFFF00000
	 * for UPAGES is used for mapping the current process u-area
	 * (u + kernel stack).
	 */
	/*
	 * Map the page table pages in both the HW segment table
	 * and the software Sysptmap.  Note that Sysptmap is also
	 * considered a PT page hence the +1.
	 */
	ste = (u_int *)kstpa;
	pte = (u_int *)kptmpa;
	epte = &pte[nptpages+1];
	protoste = kptpa | SG_RW | SG_V;
	protopte = kptpa | PG_RW | PG_CI | PG_V;
	while (pte < epte) {
	    *ste++ = protoste;
	    *pte++ = protopte;
	    protoste += NBPG;
	    protopte += NBPG;
	}
	/*
	 * Invalidate all but the last remaining entries in both.
	 */
	epte = &((u_int *)kptmpa)[NPTEPG-1];
	while (pte < epte) {
	    *ste++ = SG_NV;
	    *pte++ = PG_NV;
	}
	/*
	 * Initialize the last to point to point to the page
	 * table page allocated earlier.
	 */
	*ste = lkptpa | SG_RW | SG_V;
	*pte = lkptpa | PG_RW | PG_CI | PG_V;

	/*
	 * Invalidate all but the final entry in the last kernel PT page
	 * (u-area PTEs will be validated later).  The final entry maps
	 * the last page of physical memory.
	 */
	pte = (u_int *)lkptpa;
	epte = &pte[NPTEPG-1];
	while (pte < epte)
		*pte++ = PG_NV;
	*pte = MAXADDR | PG_RW | PG_CI | PG_V;
	/*
	 * Initialize kernel page table.
	 * Start by invalidating the `nptpages' that we have allocated.
	 */
	pte = (u_int *)kptpa;
	epte = &pte[nptpages * NPTEPG];
	while (pte < epte)
		*pte++ = PG_NV;
	/*
	 * Validate PTEs for kernel text (RO)
	 */
	pte = &((u_int *)kptpa)[da30_btop(KERNBASE)];
	epte = &pte[da30_btop(da30_trunc_page(&etext))];
#if defined(KGDB) || defined(DDB)
	protopte = PG_RW | PG_V;	/* XXX RW for now */
#else
	protopte = PG_RO | PG_V;
#endif
	while (pte < epte) {
		*pte++ = protopte;
		protopte += NBPG;
	}
	/*
	 * Validate PTEs for kernel data/bss, dynamic data allocated
	 * by us so far (nextpa bytes), and pages for proc0
	 * u-area and page table allocated below (RW).
	 */
	epte = &((u_int *)kptpa)[da30_btop(nextpa)];
	protopte = (protopte & ~PG_PROT) | PG_RW;
	/*
	 * Enable copy-back caching of data pages
	 */
	while (pte < epte) {
		*pte++ = protopte;
		protopte += NBPG;
	}
	/*
	 * Finally, validate the internal IO space PTEs (RW+CI).
	 * We do this here since the 320/350 MMU registers (also
	 * used, but to a lesser extent, on other models) are mapped
	 * in this range and it would be nice to be able to access
	 * them after the MMU is turned on.
	 */
	pte = (u_int *)iiopa;
	epte = (u_int *)eiopa;
	protopte = INTIOBASE | PG_RW | PG_CI | PG_V;
	while (pte < epte) {
		*pte++ = protopte;
		protopte += NBPG;
	}

	/*
	 * Calculate important exported kernel virtual addresses
	 */
	/*
	 * Sysseg: base of kernel segment table
	 */
	Sysseg = (struct ste *) kstpa;
	/*
	 * Sysptmap: base of kernel page table map
	 */
	Sysptmap = (struct pte *) kptmpa;
	/*
	 * Sysmap: kernel page table (as mapped through Sysptmap)
	 * Immediately follows `nptpages' of static kernel page table.
	 */
	Sysmap = (struct pte *) da30_ptob(nptpages * NPTEPG);
	/*
	 * Umap: first of UPAGES PTEs (in Sysmap) for fixed-address u-area.
	 * HIGHPAGES PTEs from the end of Sysmap.
	 */
	Umap = (vm_offset_t) Sysmap +
		(DA_MAX_PTSIZE - HIGHPAGES * sizeof(struct pte));
	/*
	 * intiobase, intiolimit: base and end of internal (DIO) IO space.
	 * IIOMAPSIZE pages prior to external IO space at end of static
	 * kernel page table.
	 */
	intiobase = (char *) da30_ptob(nptpages*NPTEPG - (IIOMAPSIZE+EIOMAPSIZE));
	intiolimit = (char *) da30_ptob(nptpages*NPTEPG - EIOMAPSIZE);
	/*
	 * extiobase: base of external (DIO-II) IO space.
	 * EIOMAPSIZE pages at the end of the static kernel page table.
	 */
	extiobase = (char *) da30_ptob(nptpages*NPTEPG - EIOMAPSIZE);

	/*
	 * Setup u-area for process 0.
	 */
	/*
	 * Validate PTEs in Sysmap corresponding to the u-area (Umap)
	 * which are HIGHPAGES from the end of the last kernel PT page
	 * allocated earlier.
	 */
	pte = &((u_int *)lkptpa)[NPTEPG - HIGHPAGES];
	epte = &pte[UPAGES];
	protopte = p0upa | PG_RW | PG_V;
	while (pte < epte) {
		*pte++ = protopte;
		protopte += NBPG;
	}
	/*
	 * Zero the u-area.
	 * NOTE: `pte' and `epte' aren't PTEs here.
	 */
	pte = (u_int *)p0upa;
	epte = (u_int *)(p0upa + UPAGES*NBPG);
	while (pte < epte)
		*pte++ = 0;
	/*
	 * Remember the u-area address so it can be loaded in the
	 * proc struct p_addr field later.
	 */
	proc0paddr = (char *) p0upa;

	/*
	 * VM data structures are now initialized, set up data for
	 * the pmap module.
	 */
	avail_start = nextpa;
	avail_end = da30_ptob(maxmem)
		/* XXX allow for msgbuf */
		- da30_round_page(sizeof(struct msgbuf));
	mem_size = da30_ptob(physmem);
	virtual_avail = VM_MIN_KERNEL_ADDRESS + nextpa;
	virtual_end = VM_MAX_KERNEL_ADDRESS;

	/*
	 * Initialize protection array.
	 */
	{
		register int *kp;

		kp = protection_codes;
		kp[VM_PROT_NONE|VM_PROT_NONE|VM_PROT_NONE] = 0;
		kp[VM_PROT_READ|VM_PROT_NONE|VM_PROT_NONE] = PG_RO;
		kp[VM_PROT_READ|VM_PROT_NONE|VM_PROT_EXECUTE] = PG_RO;
		kp[VM_PROT_NONE|VM_PROT_NONE|VM_PROT_EXECUTE] = PG_RO;
		kp[VM_PROT_NONE|VM_PROT_WRITE|VM_PROT_NONE] = PG_RW;
		kp[VM_PROT_NONE|VM_PROT_WRITE|VM_PROT_EXECUTE] = PG_RW;
		kp[VM_PROT_READ|VM_PROT_WRITE|VM_PROT_NONE] = PG_RW;
		kp[VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE] = PG_RW;
	}

	/*
	 * Kernel page/segment table allocated in locore,
	 * just initialize pointers.
	 */
	{
		struct pmap *kpm = &kernel_pmap_store;

		kpm->pm_stab = Sysseg;
		kpm->pm_ptab = Sysmap;
		simple_lock_init(&kpm->pm_lock);
		kpm->pm_count = 1;
		kpm->pm_stpa = (struct ste *)kstpa;
	}

	/*
	 * Allocate some fixed, special purpose kernel virtual addresses
	 */
	{
		vm_offset_t va = virtual_avail;

		CADDR1 = (caddr_t) va;
		va += DA_PAGE_SIZE;
		CADDR2 = (caddr_t) va;
		va += DA_PAGE_SIZE;
		vmmap = (caddr_t) va;
		va += DA_PAGE_SIZE;
		msgbufp = (struct msgbuf *) va;
		va += DA_PAGE_SIZE;
		virtual_avail = va;
	}
}
