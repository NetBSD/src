/*	$NetBSD: pmap.c,v 1.64 1999/05/23 23:03:44 ragge Exp $	   */
/*
 * Copyright (c) 1994, 1998 Ludd, University of Lule}, Sweden.
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
 *     This product includes software developed at Ludd, University of Lule}.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <vm/vm.h>
#include <vm/vm_page.h>
#include <vm/vm_kern.h>

#ifdef PMAPDEBUG
#include <dev/cons.h>
#endif

#include <uvm/uvm.h>

#include <machine/pte.h>
#include <machine/pcb.h>
#include <machine/mtpr.h>
#include <machine/macros.h>
#include <machine/sid.h>
#include <machine/cpu.h>
#include <machine/scb.h>

#include "opt_pmap_new.h"

/* QDSS console mapping hack */
#include "qd.h"
void	qdearly(void);

#define ISTACK_SIZE NBPG
vaddr_t	istack;
/* 
 * This code uses bitfield operators for most page table entries.  
 */
#define PROTSHIFT	27
#define PROT_KW		(PG_KW >> PROTSHIFT)
#define PROT_KR		(PG_KR >> PROTSHIFT) 
#define PROT_RW		(PG_RW >> PROTSHIFT)
#define PROT_RO		(PG_RO >> PROTSHIFT)
#define PROT_URKW	(PG_URKW >> PROTSHIFT)

struct pmap kernel_pmap_store;

struct	pte *Sysmap;		/* System page table */
struct	pv_entry *pv_table;	/* array of entries, one per LOGICAL page */
void	*scratch;
vaddr_t	iospace;

vaddr_t ptemapstart, ptemapend;
vm_map_t pte_map;
struct	vm_map	pte_map_store;

extern	caddr_t msgbufaddr;

#ifdef PMAPDEBUG
int	startpmapdebug = 0;
#endif

#ifndef PMAPDEBUG
static inline
#endif
void rensa __P((int, struct pte *));

vaddr_t   avail_start, avail_end;
vaddr_t   virtual_avail, virtual_end; /* Available virtual memory	*/

void pmap_pinit __P((pmap_t));
void pmap_release __P((pmap_t));

/*
 * pmap_bootstrap().
 * Called as part of vm bootstrap, allocates internal pmap structures.
 * Assumes that nothing is mapped, and that kernel stack is located
 * immediately after end.
 */
void
pmap_bootstrap()
{
	unsigned int sysptsize, i;
	extern	unsigned int etext, proc0paddr;
	struct pcb *pcb = (struct pcb *)proc0paddr;
	pmap_t pmap = pmap_kernel();

	/*
	 * Calculation of the System Page Table is somewhat a pain,
	 * because it must be in contiguous physical memory and all
	 * size calculations must be done now.
	 * Remember: sysptsize is in PTEs and nothing else!
	 */

#define USRPTSIZE ((MAXTSIZ + MAXDSIZ + MAXSSIZ + MMAPSPACE) / VAX_NBPG)
#ifdef notyet
#define	vax_btoc(x)	(((unsigned)(x) + VAX_PGOFSET) >> VAX_PGSHIFT)
	/* all physical memory */
	sysptsize = vax_btoc(avail_end);
	/* reverse mapping struct is in phys memory already */
	/* user page table */
	sysptsize += vax_btoc(USRPTSIZE * sizeof(struct pte) * maxproc);
	/* kernel malloc area */
	sysptsize += vax_btoc(NKMEMCLUSTERS * CLSIZE);
	/* Different table etc... called again in machdep */
	physmem = btoc(avail_end); /* XXX in machdep also */
	sysptsize += vax_btoc((int) allocsys((caddr_t) 0));
	/* buffer pool (buffer_map) */
	sysptsize += vax_btoc(MAXBSIZE * nbuf);
	/* exec argument submap */
	sysptsize += vax_btoc(16 * NCARGS);
	/* phys_map - XXX This submap should be nuked */
	sysptsize += vax_btoc(VM_PHYS_SIZE);
#else
	/* Kernel alloc area */
	sysptsize = (((0x100000 * maxproc) >> VAX_PGSHIFT) / 4);
	/* reverse mapping struct */
	sysptsize += (avail_end >> VAX_PGSHIFT) * 2;
	/* User Page table area. This may grow big */
	sysptsize += ((USRPTSIZE * 4) / VAX_NBPG) * maxproc;
	/* Kernel stacks per process */
	sysptsize += UPAGES * maxproc;
	/* IO device register space */
	sysptsize += IOSPSZ;
#endif

	/*
	 * Virtual_* and avail_* is used for mapping of system page table.
	 * The need for kernel virtual memory is linear dependent of the
	 * amount of physical memory also, therefore sysptsize is 
	 * a variable here that is changed dependent of the physical
	 * memory size.
	 */
	virtual_avail = avail_end + KERNBASE;
	virtual_end = KERNBASE + sysptsize * VAX_NBPG;
	memset(Sysmap, 0, sysptsize * 4); /* clear SPT before using it */

	/*
	 * The first part of Kernel Virtual memory is the physical
	 * memory mapped in. This makes some mm routines both simpler
	 * and faster, but takes ~0.75% more memory.
	 */
	pmap_map(KERNBASE, 0, avail_end, VM_PROT_READ|VM_PROT_WRITE);
	/*
	 * Kernel code is always readable for user, it must be because
	 * of the emulation code that is somewhere in there.
	 * And it doesn't hurt, /netbsd is also public readable.
	 * There are also a couple of other things that must be in
	 * physical memory and that isn't managed by the vm system.
	 */
	for (i = 0; i < ((unsigned)&etext - KERNBASE) >> VAX_PGSHIFT; i++)
		Sysmap[i].pg_prot = PROT_URKW;

	/* Map System Page Table and zero it,  Sysmap already set. */
	mtpr((unsigned)Sysmap - KERNBASE, PR_SBR);

	/* Map Interrupt stack and set red zone */
	istack = (unsigned)Sysmap + ROUND_PAGE(sysptsize * 4);
	mtpr(istack + ISTACK_SIZE, PR_ISP);
	kvtopte(istack)->pg_v = 0;

	/* Some scratch pages */
	scratch = (void *)((u_int)istack + ISTACK_SIZE);

	/* Physical-to-virtual translation table */
	(unsigned)pv_table = (u_int)scratch + 4 * VAX_NBPG;

	avail_start = (unsigned)pv_table + (ROUND_PAGE(avail_end >> PGSHIFT)) *
	    sizeof(struct pv_entry) - KERNBASE;

	/* Kernel message buffer */
	avail_end -= MSGBUFSIZE;
	msgbufaddr = (void *)(avail_end + KERNBASE);

	/* zero all mapped physical memory from Sysmap to here */
	memset((void *)istack, 0, (avail_start + KERNBASE) - istack);

	/* Set logical page size */
	uvmexp.pagesize = NBPG;
	uvm_setpagesize();

        /* QDSS console mapping hack */
#if NQD > 0
	qdearly();
#endif

	MAPVIRT(iospace, IOSPSZ); /* Device iospace mapping area */

	/* Init SCB and set up stray vectors. */
	avail_start = scb_init(avail_start);
	bzero(0, VAX_NBPG >> 1);

	if (dep_call->cpu_steal_pages)
		(*dep_call->cpu_steal_pages)();

	avail_start = ROUND_PAGE(avail_start);
	virtual_avail = ROUND_PAGE(virtual_avail);
	virtual_end = TRUNC_PAGE(virtual_end);


#if defined(PMAPDEBUG)
	cninit();
	printf("Sysmap %p, istack %lx, scratch %p\n",Sysmap,istack,scratch);
	printf("etext %p\n", &etext);
	printf("SYSPTSIZE %x\n",sysptsize);
	printf("pv_table %p, \n", pv_table);
	printf("avail_start %lx, avail_end %lx\n",avail_start,avail_end);
	printf("virtual_avail %lx,virtual_end %lx\n",virtual_avail,virtual_end);
	printf("startpmapdebug %p\n",&startpmapdebug);
#endif


	/* Init kernel pmap */
	pmap->pm_p1br = (void *)0x80000000;
	pmap->pm_p0br = (void *)0x80000000;
	pmap->pm_p1lr = 0x200000;
	pmap->pm_p0lr = AST_PCB;
	pmap->pm_stats.wired_count = pmap->pm_stats.resident_count = 0;
	    /* btop(virtual_avail - KERNBASE); */

	pmap->ref_count = 1;

	/* Activate the kernel pmap. */
	mtpr(pcb->P1BR = pmap->pm_p1br, PR_P1BR);
	mtpr(pcb->P0BR = pmap->pm_p0br, PR_P0BR);
	mtpr(pcb->P1LR = pmap->pm_p1lr, PR_P1LR);
	mtpr(pcb->P0LR = pmap->pm_p0lr, PR_P0LR);

	/*
	 * Now everything should be complete, start virtual memory.
	 */
	uvm_page_physload(avail_start >> PGSHIFT, avail_end >> PGSHIFT,
	    avail_start >> PGSHIFT, avail_end >> PGSHIFT,
	    VM_FREELIST_DEFAULT);
	mtpr(sysptsize, PR_SLR);
	mtpr(1, PR_MAPEN);
}

/*
 * How much virtual space does this kernel have?
 * (After mapping kernel text, data, etc.)
 */
void
pmap_virtual_space(v_start, v_end)
	vaddr_t *v_start;
	vaddr_t *v_end;
{
	*v_start = virtual_avail;
	*v_end	 = virtual_end;
}

/*
 * pmap_init() is called as part of vm init after memory management
 * is enabled. It is meant to do machine-specific allocations.
 * Here we allocate virtual memory for user page tables.
 */
void 
pmap_init() 
{
	/* reserve place on SPT for UPT */
	pte_map = uvm_km_suballoc(kernel_map, &ptemapstart, &ptemapend, 
	    USRPTSIZE * 4 * maxproc, TRUE, FALSE, &pte_map_store);
}


/*
 * pmap_create() creates a pmap for a new task.
 * If not already allocated, malloc space for one.
 */
#ifdef PMAP_NEW
struct pmap * 
pmap_create()
{
	struct pmap *pmap;

	MALLOC(pmap, struct pmap *, sizeof(*pmap), M_VMPMAP, M_WAITOK);
	pmap_pinit(pmap);
	return(pmap);
}
#else
pmap_t 
pmap_create(phys_size)
	psize_t phys_size;
{
	pmap_t	 pmap;

#ifdef PMAPDEBUG
if(startpmapdebug)printf("pmap_create: phys_size %x\n",(int)phys_size);
#endif
	if (phys_size)
		return NULL;

	pmap = (pmap_t) malloc(sizeof(struct pmap), M_VMPMAP, M_WAITOK);
	bzero(pmap, sizeof(struct pmap));
	pmap_pinit(pmap); 
	return (pmap);
}
#endif

/*
 * Initialize a preallocated an zeroed pmap structure,
 */
void
pmap_pinit(pmap)
	pmap_t pmap;
{
	int bytesiz;

	/*
	 * Allocate PTEs and stash them away in the pmap.
	 * XXX Ok to use kmem_alloc_wait() here?
	 */
	bytesiz = USRPTSIZE * sizeof(struct pte);
	pmap->pm_p0br = (void *)uvm_km_valloc_wait(pte_map, bytesiz);
	pmap->pm_p0lr = vax_btoc(MAXTSIZ + MAXDSIZ + MMAPSPACE) | AST_PCB;
	(vaddr_t)pmap->pm_p1br = (vaddr_t)pmap->pm_p0br + bytesiz - 0x800000;
	pmap->pm_p1lr = (0x200000 - vax_btoc(MAXSSIZ));
	pmap->pm_stack = USRSTACK;

#ifdef PMAPDEBUG
if (startpmapdebug)
	printf("pmap_pinit(%p): p0br=%p p0lr=0x%lx p1br=%p p1lr=0x%lx\n",
	pmap, pmap->pm_p0br, pmap->pm_p0lr, pmap->pm_p1br, pmap->pm_p1lr);
#endif

	pmap->ref_count = 1;
	pmap->pm_stats.resident_count = pmap->pm_stats.wired_count = 0;
}

/*
 * Release any resources held by the given physical map.
 * Called when a pmap initialized by pmap_pinit is being released.
 * Should only be called if the map contains no valid mappings.
 */
void
pmap_release(pmap)
	struct pmap *pmap;
{
#ifdef PMAPDEBUG
if(startpmapdebug)printf("pmap_release: pmap %p\n",pmap);
#endif

	if (pmap->pm_p0br)
		uvm_km_free_wakeup(pte_map, (vaddr_t)pmap->pm_p0br, 
		    USRPTSIZE * sizeof(struct pte));
}


/*
 * pmap_destroy(pmap): Remove a reference from the pmap. 
 * If the pmap is NULL then just return else decrese pm_count.
 * If this was the last reference we call's pmap_relaese to release this pmap.
 * OBS! remember to set pm_lock
 */

void
pmap_destroy(pmap)
	pmap_t pmap;
{
	int count;
  
#ifdef PMAPDEBUG
if(startpmapdebug)printf("pmap_destroy: pmap %p\n",pmap);
#endif
	if (pmap == NULL)
		return;

	simple_lock(&pmap->pm_lock);
	count = --pmap->ref_count;
	simple_unlock(&pmap->pm_lock);
  
	if (count == 0) {
		pmap_release(pmap);
		free((caddr_t)pmap, M_VMPMAP);
	}
}

/*
 * Rensa is a help routine to remove a pv_entry from the pv list.
 * Arguments are physical clustering page and page table entry pointer.
 */
void
rensa(clp, ptp)
	int clp;
	struct pte *ptp;
{
	struct	pv_entry *pf, *pl, *pv = pv_table + clp;
	int	s, *g;

#ifdef PMAPDEBUG
if (startpmapdebug)
	printf("rensa: pv %p clp 0x%x ptp %p\n", pv, clp, ptp);
#endif
	s = splimp();
	if (pv->pv_pte == ptp) {
		g = (int *)pv->pv_pte;
		if ((pv->pv_attr & (PG_V|PG_M)) == 0)
			pv->pv_attr |= g[0]|g[1]|g[2]|g[3]|g[4]|g[5]|g[6]|g[7];
		pv->pv_pte = 0;
		pv->pv_pmap->pm_stats.resident_count--;
		pv->pv_pmap = 0;
		splx(s);
		return;
	}
	for (pl = pv; pl->pv_next; pl = pl->pv_next) {
		if (pl->pv_next->pv_pte == ptp) {
			pf = pl->pv_next;
			pl->pv_next = pl->pv_next->pv_next;
			g = (int *)pf->pv_pte;
			if ((pv->pv_attr & (PG_V|PG_M)) == 0)
				pv->pv_attr |=
				    g[0]|g[1]|g[2]|g[3]|g[4]|g[5]|g[6]|g[7];
			pf->pv_pmap->pm_stats.resident_count--;
			FREE(pf, M_VMPVENT);
			splx(s);
			return;
		}
	}
	panic("rensa");
}

#ifdef PMAP_NEW
/*
 * New (real nice!) function that allocates memory in kernel space
 * without tracking it in the MD code.
 */
void
pmap_kenter_pa(va, pa, prot)
	vaddr_t va;
	paddr_t	pa;
	vm_prot_t prot;
{
	int *ptp;

	ptp = (int *)kvtopte(va);
#ifdef PMAPDEBUG
if(startpmapdebug)
printf("pmap_kenter_pa: va: %lx, pa %lx, prot %x ptp %p\n", va, pa, prot, ptp);
#endif
	ptp[0] = PG_V | ((prot & VM_PROT_WRITE)? PG_KW : PG_KR) |
	    PG_PFNUM(pa) | PG_SREF;
	ptp[1] = ptp[0] + 1;
	ptp[2] = ptp[0] + 2;
	ptp[3] = ptp[0] + 3;
	ptp[4] = ptp[0] + 4;
	ptp[5] = ptp[0] + 5;
	ptp[6] = ptp[0] + 6;
	ptp[7] = ptp[0] + 7;
	mtpr(0, PR_TBIA);
}

void
pmap_kremove(va, len)
	vaddr_t va;
	vsize_t len;
{
	struct pte *pte;
	int i;

#ifdef PMAPDEBUG
if(startpmapdebug)
printf("pmap_kremove: va: %lx, len %lx, ptp %p\n", va, len, kvtopte(va));
#endif

	/*
	 * Unfortunately we must check if any page may be on the pv list. 
	 */
	pte = kvtopte(va);
	len >>= PGSHIFT;

	for (i = 0; i < len; i++) {
		if (pte->pg_pfn == 0)
			continue;
		if (pte->pg_sref == 0)
			rensa(pte->pg_pfn >> LTOHPS, pte);
		bzero(pte, LTOHPN * sizeof(struct pte));
		pte += LTOHPN;
	}
	mtpr(0, PR_TBIA);
}

void
pmap_kenter_pgs(va, pgs, npgs)
	vaddr_t va;
	struct vm_page **pgs;
	int npgs;
{
	int i;
	int *ptp;

#ifdef PMAPDEBUG
if(startpmapdebug)
printf("pmap_kenter_pgs: va: %lx, pgs %p, npgs %x\n", va, pgs, npgs);
#endif

	/*
	 * May this routine affect page tables? 
	 * We assume that, and uses TBIA.
	 */
	ptp = (int *)kvtopte(va);
	for (i = 0 ; i < npgs ; i++) {
		ptp[0] = PG_V | PG_KW |
		    PG_PFNUM(VM_PAGE_TO_PHYS(pgs[i])) | PG_SREF;
		ptp[1] = ptp[0] + 1;
		ptp[2] = ptp[0] + 2;
		ptp[3] = ptp[0] + 3;
		ptp[4] = ptp[0] + 4;
		ptp[5] = ptp[0] + 5;
		ptp[6] = ptp[0] + 6;
		ptp[7] = ptp[0] + 7;
		ptp += LTOHPN;
	}
	mtpr(0, PR_TBIA);
}
#endif

/*
 * pmap_enter() is the main routine that puts in mappings for pages, or
 * upgrades mappings to more "rights". Note that:
 * - "wired" isn't used. We don't loose mappings unless asked for.
 * - "access_type" is set if the entering was caused by a fault.
 */
void
pmap_enter(pmap, v, p, prot, wired, access_type)
	pmap_t	pmap;
	vaddr_t	v;
	paddr_t	p;
	vm_prot_t prot, access_type;
	boolean_t wired;
{
	struct	pv_entry *pv, *tmp;
	int	i, s, newpte, oldpte, *patch;

	/* Can this happen with UVM??? */
	if (pmap == 0)
		return;

	/* Find addess of correct pte */
	if (v & KERNBASE) {
		patch = (int *)Sysmap;
		i = (v - KERNBASE) >> VAX_PGSHIFT;
		newpte = (p>>VAX_PGSHIFT)|(prot&VM_PROT_WRITE?PG_KW:PG_KR);
	} else if (v < 0x40000000) {
		patch = (int *)pmap->pm_p0br;
		i = (v >> VAX_PGSHIFT);
		if (i >= (pmap->pm_p0lr & ~AST_MASK))
			panic("P0 too small in pmap_enter");
		patch = (int *)pmap->pm_p0br;
		newpte = (p>>VAX_PGSHIFT)|(prot&VM_PROT_WRITE?PG_RW:PG_RO);
	} else {
		patch = (int *)pmap->pm_p1br;
		i = (v - 0x40000000) >> VAX_PGSHIFT;
		if (i < pmap->pm_p1lr)
			panic("pmap_enter: must expand P1");
		if (v < pmap->pm_stack)
			pmap->pm_stack = v;
		newpte = (p>>VAX_PGSHIFT)|(prot&VM_PROT_WRITE?PG_RW:PG_RO);
	}

	oldpte = patch[i] & ~(PG_V|PG_M);

	/* No mapping change. Can this happen??? */
	if (newpte == oldpte)
		return;

	pv = pv_table + (p >> PGSHIFT);

	/* Changing mapping? */
	oldpte &= PG_FRAME;
	if ((newpte & PG_FRAME) != oldpte) {

		/* Mapped before? Remove it then. */
		if (oldpte)
			pmap_page_protect(PHYS_TO_VM_PAGE((oldpte
			    << VAX_PGSHIFT)), 0);

		s = splimp();
		if (pv->pv_pte == 0) {
			pv->pv_pte = (struct pte *) & patch[i];
			pv->pv_pmap = pmap;
		} else {
			MALLOC(tmp, struct pv_entry *, sizeof(struct pv_entry),
			    M_VMPVENT, M_NOWAIT);
#ifdef DIAGNOSTIC
			if (tmp == 0) 
				panic("pmap_enter: cannot alloc pv_entry");
#endif
			tmp->pv_pte = (struct pte *)&patch[i];
			tmp->pv_pmap = pmap;
			tmp->pv_next = pv->pv_next;
			pv->pv_next = tmp;
		}
		splx(s);
	} else {
		/* No mapping change, just flush the TLB */
		mtpr(0, PR_TBIA);
	}
	pmap->pm_stats.resident_count++;

	if (access_type & VM_PROT_READ) {
		pv->pv_attr |= PG_V;
		newpte |= PG_V;
	}
	if (access_type & VM_PROT_WRITE)
		pv->pv_attr |= PG_M;

	patch[i] = newpte;
	patch[i+1] = newpte+1;
	patch[i+2] = newpte+2;
	patch[i+3] = newpte+3;
	patch[i+4] = newpte+4;
	patch[i+5] = newpte+5;
	patch[i+6] = newpte+6;
	patch[i+7] = newpte+7;
}

void *
pmap_bootstrap_alloc(size)
	int size;
{
	void *mem;

#ifdef PMAPDEBUG
if(startpmapdebug)
printf("pmap_bootstrap_alloc: size 0x %x\n",size);
#endif
	size = round_page(size);
	mem = (caddr_t)avail_start + KERNBASE;
	avail_start += size;
	memset(mem, 0, size);
	return (mem);
}

vaddr_t
pmap_map(virtuell, pstart, pend, prot)
	vaddr_t virtuell;
	paddr_t	pstart, pend;
	int prot;
{
	vaddr_t count;
	int *pentry;

#ifdef PMAPDEBUG
if(startpmapdebug)
	printf("pmap_map: virt %lx, pstart %lx, pend %lx, Sysmap %p\n",
	    virtuell, pstart, pend, Sysmap);
#endif

	pstart=(uint)pstart &0x7fffffff;
	pend=(uint)pend &0x7fffffff;
	virtuell=(uint)virtuell &0x7fffffff;
	(uint)pentry= (((uint)(virtuell)>>VAX_PGSHIFT)*4)+(uint)Sysmap;
	for(count=pstart;count<pend;count+=VAX_NBPG){
		*pentry++ = (count>>VAX_PGSHIFT)|PG_V|
		    (prot & VM_PROT_WRITE ? PG_KW : PG_KR);
	}
	mtpr(0,PR_TBIA);
	return(virtuell+(count-pstart)+0x80000000);
}

paddr_t 
pmap_extract(pmap, va)
	pmap_t pmap;
	vaddr_t va;
{
	int	*pte, sva = (va & 0x3fffffff) >> VAX_PGSHIFT;

#ifdef PMAPDEBUG
if(startpmapdebug)printf("pmap_extract: pmap %p, va %lx\n",pmap, va);
#endif
#ifdef DIAGNOSTIC
	if (va & PGOFSET)
		printf("Warning, pmap_extract va not aligned\n");
#endif

	if (va < 0x40000000) {
		if (sva > (pmap->pm_p0lr & ~AST_MASK))
			return 0;
		pte = (int *)pmap->pm_p0br;
	} else if (va & KERNBASE) {
		pte = (int *)Sysmap;
	} else {
		if (sva < pmap->pm_p1lr)
			return 0;
		pte = (int *)pmap->pm_p1br;
	}

	return (pte[sva] & PG_FRAME) << VAX_PGSHIFT;
}

/*
 * Sets protection for a given region to prot. If prot == none then
 * unmap region. pmap_remove is implemented as pmap_protect with
 * protection none.
 */
void
pmap_protect(pmap, start, end, prot)
	pmap_t	pmap;
	vaddr_t	start, end;
	vm_prot_t prot;
{
	struct	pte *pt, *pts, *ptd;
	int	pr;

#ifdef PMAPDEBUG
if(startpmapdebug) printf("pmap_protect: pmap %p, start %lx, end %lx, prot %x\n",
	pmap, start, end,prot);
#endif

	if (pmap == 0)
		return;

	if (start & KERNBASE) { /* System space */
		pt = Sysmap;
#ifdef DIAGNOSTIC
		if (((end & 0x3fffffff) >> VAX_PGSHIFT) > mfpr(PR_SLR))
			panic("pmap_protect: outside SLR: %lx", end);
#endif
		start &= ~KERNBASE;
		end &= ~KERNBASE;
		pr = (prot & VM_PROT_WRITE ? PROT_KW : PROT_KR);
	} else {
		if (start & 0x40000000) { /* P1 space */
			if (end <= pmap->pm_stack)
				return;
			if (start < pmap->pm_stack)
				start = pmap->pm_stack;
			pt = pmap->pm_p1br;
#ifdef DIAGNOSTIC
			if (((start & 0x3fffffff) >> VAX_PGSHIFT) < pmap->pm_p1lr)
				panic("pmap_protect: outside P1LR");
#endif
			start &= 0x3fffffff;
			end = (end == KERNBASE ? end >> 1 : end & 0x3fffffff);
		} else { /* P0 space */
			pt = pmap->pm_p0br;
#ifdef DIAGNOSTIC
			if ((end >> VAX_PGSHIFT) > (pmap->pm_p0lr & ~AST_MASK))
				panic("pmap_protect: outside P0LR");
#endif
		}
		pr = (prot & VM_PROT_WRITE ? PROT_RW : PROT_RO);
	}
	pts = &pt[start >> VAX_PGSHIFT];
	ptd = &pt[end >> VAX_PGSHIFT];
#ifdef DEBUG
	if (((int)pts - (int)pt) & 7)
		panic("pmap_remove: pts not even");
	if (((int)ptd - (int)pt) & 7)
		panic("pmap_remove: ptd not even");
#endif

	if (prot == VM_PROT_NONE) {
		while (pts < ptd) {
			if (*(int *)pts) {
				if ((*(int *)pts & PG_SREF) == 0)
					rensa(pts->pg_pfn >> LTOHPS, pts);
				bzero(pts, sizeof(struct pte) * LTOHPN);
			}
			pts += LTOHPN;
		}
	} else {
		while (pts < ptd) {
			if (*(int *)pts) {
				pts[0].pg_prot = pr;
				pts[1].pg_prot = pr;
				pts[2].pg_prot = pr;
				pts[3].pg_prot = pr;
				pts[4].pg_prot = pr;
				pts[5].pg_prot = pr;
				pts[6].pg_prot = pr;
				pts[7].pg_prot = pr;
			}
			pts += LTOHPN;
		}
	}
	mtpr(0,PR_TBIA);
}

int pmap_simulref(int bits, int addr);
/*
 * Called from interrupt vector routines if we get a page invalid fault.
 * Note: the save mask must be or'ed with 0x3f for this function.
 * Returns 0 if normal call, 1 if CVAX bug detected.
 */
int
pmap_simulref(int bits, int addr)
{
	u_int	*pte;
	struct  pv_entry *pv;
	paddr_t	pa;

#ifdef PMAPDEBUG
if (startpmapdebug) 
	printf("pmap_simulref: bits %x addr %x\n", bits, addr);
#endif
#ifdef DEBUG
	if (bits & 1)
		panic("pte trans len");
#endif
	/* Set addess on logical page boundary */
	addr &= ~PGOFSET;
	/* First decode userspace addr */
	if (addr >= 0) {
		if ((addr << 1) < 0)
			pte = (u_int *)mfpr(PR_P1BR);
		else
			pte = (u_int *)mfpr(PR_P0BR);
		pte += PG_PFNUM(addr);
		if (bits & 2) { /* PTE reference */
			pte = (u_int *)TRUNC_PAGE(pte);
			pte = (u_int *)kvtopte(pte);
			if (pte[0] == 0) /* Check for CVAX bug */
				return 1;	
			pa = (u_int)pte & ~KERNBASE;
		} else
			pa = Sysmap[PG_PFNUM(pte)].pg_pfn << VAX_PGSHIFT;
	} else {
		pte = (u_int *)kvtopte(addr);
		pa = (u_int)pte & ~KERNBASE;
	}
	pte[0] |= PG_V;
	pte[1] |= PG_V;
	pte[2] |= PG_V;
	pte[3] |= PG_V;
	pte[4] |= PG_V;
	pte[5] |= PG_V;
	pte[6] |= PG_V;
	pte[7] |= PG_V;
	pv = pv_table + (pa >> PGSHIFT);
	pv->pv_attr |= PG_V; /* Referenced */
	if (bits & 4)
		pv->pv_attr |= PG_M; /* (will be) modified. XXX page tables  */
	return 0;
}

/*
 * Checks if page is referenced; returns true or false depending on result.
 */
#ifdef PMAP_NEW
boolean_t
pmap_is_referenced(pg)
	struct vm_page *pg;
{
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
#else
boolean_t
pmap_is_referenced(pa)
	paddr_t	pa;
{
#endif
	struct	pv_entry *pv;

	pv = pv_table + (pa >> PGSHIFT);
#ifdef PMAPDEBUG
	if (startpmapdebug)
		printf("pmap_is_referenced: pa %lx pv_entry %p ", pa, pv);
#endif

	if (pv->pv_attr & PG_V)
		return 1;

	return 0;
}

/*
 * Clears valid bit in all ptes referenced to this physical page.
 */
#ifdef PMAP_NEW
boolean_t
pmap_clear_reference(pg)
	struct vm_page *pg;
{
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
#else
void 
pmap_clear_reference(pa)
	paddr_t	pa;
{
#endif
	struct	pv_entry *pv;

	pv = pv_table + (pa >> PGSHIFT);
#ifdef PMAPDEBUG
	if (startpmapdebug)
		printf("pmap_clear_reference: pa %lx pv_entry %p\n", pa, pv);
#endif

	pv->pv_attr &= ~PG_V;

	if (pv->pv_pte)
		pv->pv_pte[0].pg_v = pv->pv_pte[1].pg_v = 
		    pv->pv_pte[2].pg_v = pv->pv_pte[3].pg_v = 
		    pv->pv_pte[4].pg_v = pv->pv_pte[5].pg_v = 
		    pv->pv_pte[6].pg_v = pv->pv_pte[7].pg_v = 0;

	while ((pv = pv->pv_next))
		pv->pv_pte[0].pg_v = pv->pv_pte[1].pg_v =
		    pv->pv_pte[2].pg_v = pv->pv_pte[3].pg_v = 
		    pv->pv_pte[4].pg_v = pv->pv_pte[5].pg_v = 
		    pv->pv_pte[6].pg_v = pv->pv_pte[7].pg_v = 0;
#ifdef PMAP_NEW
	return TRUE; /* XXX */
#endif
}

/*
 * Checks if page is modified; returns true or false depending on result.
 */
#ifdef PMAP_NEW
boolean_t
pmap_is_modified(pg)
	struct vm_page *pg;
{
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
#else
boolean_t
pmap_is_modified(pa)
     paddr_t     pa;
{
#endif
	struct	pv_entry *pv;

	pv = pv_table + (pa >> PGSHIFT);
#ifdef PMAPDEBUG
	if (startpmapdebug)
		printf("pmap_is_modified: pa %lx pv_entry %p ", pa, pv);
#endif

	if (pv->pv_attr & PG_M)
		return 1;

	if (pv->pv_pte)
		if ((pv->pv_pte[0].pg_m | pv->pv_pte[1].pg_m
		    | pv->pv_pte[2].pg_m | pv->pv_pte[3].pg_m
		    | pv->pv_pte[4].pg_m | pv->pv_pte[5].pg_m
		    | pv->pv_pte[6].pg_m | pv->pv_pte[7].pg_m)) {
#ifdef PMAPDEBUG
			if (startpmapdebug) printf("Yes: (1)\n");
#endif
			return 1;
		}

	while ((pv = pv->pv_next)) {
		if ((pv->pv_pte[0].pg_m | pv->pv_pte[1].pg_m
		    | pv->pv_pte[2].pg_m | pv->pv_pte[3].pg_m
		    | pv->pv_pte[4].pg_m | pv->pv_pte[5].pg_m
		    | pv->pv_pte[6].pg_m | pv->pv_pte[7].pg_m)) {
#ifdef PMAPDEBUG
			if (startpmapdebug) printf("Yes: (2)\n");
#endif
			return 1;
		}
	}
#ifdef PMAPDEBUG
	if (startpmapdebug) printf("No\n");
#endif
	return 0;
}

/*
 * Clears modify bit in all ptes referenced to this physical page.
 */
#ifdef PMAP_NEW
boolean_t
pmap_clear_modify(pg)
	struct vm_page *pg;
{
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
#else
void 
pmap_clear_modify(pa)
	paddr_t	pa;
{
#endif
	struct	pv_entry *pv;

	pv = pv_table + (pa >> PGSHIFT);

#ifdef PMAPDEBUG
	if (startpmapdebug)
		printf("pmap_clear_modify: pa %lx pv_entry %p\n", pa, pv);
#endif
	pv->pv_attr &= ~PG_M;

	if (pv->pv_pte)
		pv->pv_pte[0].pg_m = pv->pv_pte[1].pg_m =
		    pv->pv_pte[2].pg_m = pv->pv_pte[3].pg_m = 
		    pv->pv_pte[4].pg_m = pv->pv_pte[5].pg_m = 
		    pv->pv_pte[6].pg_m = pv->pv_pte[7].pg_m = 0;

	while ((pv = pv->pv_next))
		pv->pv_pte[0].pg_m = pv->pv_pte[1].pg_m =
		    pv->pv_pte[2].pg_m = pv->pv_pte[3].pg_m = 
		    pv->pv_pte[4].pg_m = pv->pv_pte[5].pg_m = 
		    pv->pv_pte[6].pg_m = pv->pv_pte[7].pg_m = 0;
#ifdef PMAP_NEW
	return TRUE; /* XXX */
#endif
}

/*
 * Lower the permission for all mappings to a given page.
 * Lower permission can only mean setting protection to either read-only
 * or none; where none is unmapping of the page.
 */
#ifdef PMAP_NEW
void
pmap_page_protect(pg, prot)
	struct vm_page *pg;
	vm_prot_t       prot;
{
	struct	pte *pt;
	struct	pv_entry *pv, *opv, *pl;
	int	s, *g;
	paddr_t	pa;
#ifdef PMAPDEBUG
if(startpmapdebug) printf("pmap_page_protect: pg %p, prot %x, ",pg, prot);
#endif
	pa = VM_PAGE_TO_PHYS(pg);
#ifdef PMAPDEBUG
if(startpmapdebug) printf("pa %lx\n",pa);
#endif


#else
void
pmap_page_protect(pa, prot)
	paddr_t	pa;
	vm_prot_t	prot;
{
	struct	pte *pt;
	struct	pv_entry *pv, *opv, *pl;
	int	s, *g;
#endif
  
	pv = pv_table + (pa >> PGSHIFT);
	if (pv->pv_pte == 0 && pv->pv_next == 0)
		return;

	if (prot == VM_PROT_ALL) /* 'cannot happen' */
		return;

	if (prot == VM_PROT_NONE) {
		g = (int *)pv->pv_pte;
		s = splimp();
		if (g) {
			if ((pv->pv_attr & (PG_V|PG_M)) == 0)
				pv->pv_attr |= 
				    g[0]|g[1]|g[2]|g[3]|g[4]|g[5]|g[6]|g[7];
			bzero(g, sizeof(struct pte) * LTOHPN);
			pv->pv_pte = 0;
			pv->pv_pmap->pm_stats.resident_count--;
		}
		pl = pv->pv_next;
		pv->pv_pmap = 0;
		pv->pv_next = 0;
		while (pl) {
			g = (int *)pl->pv_pte;
			if ((pv->pv_attr & (PG_V|PG_M)) == 0)
				pv->pv_attr |=
				    g[0]|g[1]|g[2]|g[3]|g[4]|g[5]|g[6]|g[7];
			bzero(g, sizeof(struct pte) * LTOHPN);
			pl->pv_pmap->pm_stats.resident_count--;
			opv = pl;
			pl = pl->pv_next;
			FREE(opv, M_VMPVENT);
		} 
		splx(s);
	} else { /* read-only */
		do {
			pt = pv->pv_pte;
			if (pt == 0)
				continue;
			pt[0].pg_prot = pt[1].pg_prot = 
			    pt[2].pg_prot = pt[3].pg_prot = 
			    pt[4].pg_prot = pt[5].pg_prot = 
			    pt[6].pg_prot = pt[7].pg_prot = 
			    ((vaddr_t)pv->pv_pte < ptemapstart ? 
			    PROT_KR : PROT_RO);
		} while ((pv = pv->pv_next));
	}
	mtpr(0, PR_TBIA);
}

/*
 * Activate the address space for the specified process.
 * Note that if the process to activate is the current process, then
 * the processor internal registers must also be loaded; otherwise
 * the current process will have wrong pagetables.
 */
void
pmap_activate(p)
	struct proc *p;
{
	pmap_t pmap;
	struct pcb *pcb;

#ifdef PMAPDEBUG
if(startpmapdebug) printf("pmap_activate: p %p\n", p);
#endif

	pmap = p->p_vmspace->vm_map.pmap;
	pcb = &p->p_addr->u_pcb;

	pcb->P0BR = pmap->pm_p0br;
	pcb->P0LR = pmap->pm_p0lr;
	pcb->P1BR = pmap->pm_p1br;
	pcb->P1LR = pmap->pm_p1lr;

	if (p == curproc) {
		mtpr(pmap->pm_p0br, PR_P0BR);
		mtpr(pmap->pm_p0lr, PR_P0LR);
		mtpr(pmap->pm_p1br, PR_P1BR);
		mtpr(pmap->pm_p1lr, PR_P1LR);
	}
	mtpr(0, PR_TBIA);
}
