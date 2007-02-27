/*	$NetBSD: pmap.c,v 1.2.26.1 2007/02/27 16:52:32 yamt Exp $	*/
/*
 * Copyright (c) 2003 Anders Magnusson (ragge@ludd.luth.se).
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
 * 3. The name of the author may not be used to endorse or promote products
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm.h>

#include <dev/cons.h>

#include <machine/pcb.h>
#include <machine/io.h>

#define	PGSH	9		/* Page shift in words */
#define	PGSZ	(1 << PGSH)	/* Page size in words */
#define	SECTSZ	(PGSZ << PGSH)	/* Size of a section */
#define	NSECT	32		/* 32 sections */
#define	PTOV_COMP	2	/* Shift to compensate for word addresses */

#define	SEGNO(addr) 	((addr) >> 18)	/* Get segment */
#define	PGINSEG(page)	(((page) & 0777000) >> PGSH)
#define	PGNO(page)	((page) >> PGSH)

#define	mk2word(x)	((x) /= 4)
#define	word2byte(x)	((x) * 4)

#define PMAPDEBUG
#ifdef PMAPDEBUG
int startpmapdebug = 0;
#define PMDEBUG(x) if (startpmapdebug)printf x
#else
#define PMDEBUG(x)
#endif

#ifdef __GNUC__
#define	CLRPT(x) \
	asm("move 6,%0 \n .long 0701106000000" : : "r"(x) : "6");
#endif
#ifdef __PCC__
#define CLRPT(x)	clrpt(x)
void clrpt(int);
#endif

void pmap_bootstrap(void);
int badaddr(int addr);

struct pmap kernel_pmap_store;
char *vmmap;
static int *mapaddr, *mappte;	/* Used for pmap_{copy,zero}_page */
extern int avail_start, avail_end; /* These are in words, not bytes */
extern struct ept *ept;
extern int proc0paddr;

void
pmap_bootstrap()
{
	int i, guardpage;
	extern caddr_t msgbufaddr;
	TUNION uu;

	/*
	 * Setup executive and user process table but do not enable paging.
	 */
	DATAO(PAG,proc0paddr >> PGSH | PAG_DATA_LUBA | PAG_DATA_DNUA);
	CONO(PAG, TPTOINT(ept) >> PGSH);

#ifdef notyet
	/* Count up memory */
	for (i = 1*1024*1024; ; i += PGSZ)
		if (badaddr(i))
			break;
#else
	i = 4*1024*1024;
#endif
	avail_end = i;

#ifdef DEBUG
	{
		extern int end[];
		consinit();
		printf("Kernel end %o kstack end %o\n", end, avail_start);
		printf("EPT %o UPT %o\n", ept, proc0paddr);
	}
#endif

	/*
	 * Enter the following 32 pages as kernel page table pages.
	 * Skip section 0 page, and let it act as a guard page.
	 */
	guardpage = avail_start;
	for (i = 0; i < NSECT; i++) {
		ept->ept_section[i] = PG_IMM|PG_WRITE | PGNO(avail_start);
		avail_start += PGSZ;
	}
	/* Map kernel memory 1:1 */
	for (i = KERNBASE; i < avail_start; i += PGSZ)
		pmap_kenter_pa(i*4, i*4, VM_PROT_READ|VM_PROT_WRITE);

	/* Map in page 0 at page 0. It is used for interrupts */
	pmap_kenter_pa(0, 0, VM_PROT_READ|VM_PROT_WRITE);

	/* Remove section page for section 0. Used as a guard page */
	{ /* XXX - use pmap_kremove */
		int *ptep = TINTTOP((ept->ept_section[1] & 07777777) << PGSH);
		mappte = &ptep[PGINSEG(guardpage)];
		mapaddr = TINTTOP(guardpage);
		ptep[PGINSEG(guardpage)] = 0;
	}

	/* Set logical page size */
	uvmexp.pagesize = NBPG;
	uvm_setpagesize();
	physmem = avail_end/PGSZ;

	/* Kernel message buffer */
	avail_end -= MSGBUFSIZE/4;
	msgbufaddr = TINTTOCP(avail_start | 0700000000000);
	for (i = 0; i < MSGBUFSIZE/4; i += PGSZ)
		pmap_kenter_pa(word2byte(avail_start+i),
		    word2byte(avail_end+i), VM_PROT_READ|VM_PROT_WRITE);
	avail_start += MSGBUFSIZE/4;

	/*
	 * Give away memory to UVM.
	 */
	uvm_page_physload(avail_start >> PGSH, avail_end >> PGSH,
	    avail_start >> PGSH, avail_end >> PGSH, VM_FREELIST_DEFAULT);
#ifdef notyet
	/* Lost section zero pages */
	uvm_page_physload(0, PGSZ. 0, PGSZ, VM_FREELIST_DEFAULT);
#endif
#if 0
	kernel_pmap_store.pm_count = 1;
	simple_lock_init(&kernel_pmap_store.pm_lock);
#endif

	/*
	 * Ready! Turn on paging.
	 */
	CONO(PAG,TPTOINT(ept) >> PGSH | PAG_CON_ENABLE | PAG_CON_T20);
	DATAO(PAG,proc0paddr >> PGSH | PAG_DATA_LUBA | PAG_DATA_DNUA);

#ifdef DEBUG
	printf("section pages at %o\n", guardpage);
	printf("\n");
#endif
}

void
pmap_virtual_space(vaddr_t *v1, vaddr_t *v2)
{
	*v1 = avail_start * sizeof(int);
	*v2 = 0200000000; /* Max virtual memory address in bytes */
}

long
pmap_resident_count(pmap_t pmap)
{
	panic("pmap_resident_count");
}

void
pmap_update(pmap_t pmap)
{
}

void
pmap_kenter_pa(vaddr_t v, paddr_t p, vm_prot_t prot)
{
	int seg, off, pte, *ptep;
	TUNION uu;

	seg = (v >> 20) & 0777;
	off = (v >> 11) & 0777;

	PMDEBUG(("pmap_kenter_pa: va: %lo, pa %lo, prot %o\n", v/4, p/4, prot));

	if (ept->ept_section[seg] == 0)
		panic("pmap_kenter_pa: need to add section, va %o", v);
	ptep = TINTTOP(((ept->ept_section[seg] & 07777777) << PGSH));
	pte = PG_IMM | (p >> 11);
	if (prot & VM_PROT_WRITE)
		pte |= PG_WRITE;
	ptep[off] = pte;
}

void
pmap_kremove(vaddr_t v, vsize_t size)
{
	panic("pmap_kremove");
}

bool
pmap_clear_modify(struct vm_page *vm)
{
	panic("pmap_clear_modify");
}

void
pmap_page_protect(struct vm_page *vm, vm_prot_t prot)
{
	panic("pmap_page_protect");
}

bool
pmap_clear_reference(struct vm_page *vm)
{
	panic("pmap_clear_reference");
}

void
pmap_remove(pmap_t pmap, vaddr_t v1, vaddr_t v2)
{
	panic("pmap_remove");
}

/*
 * Get the physical page address for the virtual address v.
 * Return false if no mapping exists.
 */
bool
pmap_extract(pmap_t pmap, vaddr_t v, paddr_t *pp)
{
	int nv = v >> PTOV_COMP;
	int seg, pga, pg, *pgp, paddr;

	if (pmap != pmap_kernel())
		panic("pmap_extract");
	seg = SEGNO(nv);
	if (((pga = ept->ept_section[seg]) & PG_IMM) == 0)
		return false;
	pg = PGINSEG(nv);
	pgp = (int *)(((pga << PGSH) & 017777777) << PTOV_COMP);
	if (((paddr = pgp[pg]) & PG_IMM) == 0)
		return false;
	*pp = (paddr << PGSH) & 037777777;
	*pp <<= PTOV_COMP;
	return true;
}

int
pmap_enter(pmap_t pmap, vaddr_t v, paddr_t p, vm_prot_t prot, int flags)
{
	PMDEBUG(("pmap_enter: pmap %o v %o p %o prot %o access %o\n",
	    pmap, v/4, p/4, prot, flags & VM_PROT_ALL));

	if (pmap != pmap_kernel())
		panic("user mapping");
	pmap_kenter_pa(v, p, prot);	/* XXX */
	return 0;
}

paddr_t
pmap_phys_address(int a)
{
	panic("pmap_phys_address");
}

void
pmap_unwire(pmap_t pmap, vaddr_t v)
{
	panic("pmap_unwire");
}

void
pmap_collect(pmap_t pmap)
{
	panic("pmap_collect");
}

void
cpu_swapout(struct lwp *p)
{
	panic("cpu_swapout");
}

void
cpu_swapin(struct lwp *p)
{
	panic("cpu_swapin");
}

/*
 * Increment the reference count.
 */
void
pmap_reference(pmap_t pmap)
{
#ifdef notyet
	pmap->pm_count++;
#endif
}

void
pmap_protect(pmap_t pmap, vaddr_t v1, vaddr_t v2, vm_prot_t prot)
{
	panic("pmap_protect");
}

void
pmap_destroy(pmap_t pmap)
{
	panic("pmap_destroy");
}

void
pmap_activate(struct lwp *p)
{
	panic("pmap_activate");
}

void
pmap_deactivate(struct lwp *p)
{
	panic("pmap_deactivate");
}

struct pmap *
pmap_create()
{
	struct pmap *pmap;

	MALLOC(pmap, struct pmap *, sizeof(*pmap), M_VMPMAP, M_WAITOK);
	memset(pmap, 0, sizeof(struct pmap));
	return (pmap);
}

void
pmap_copy(pmap_t pm1, pmap_t pm2, vaddr_t v1, vsize_t ax, vaddr_t v2)
{
	panic("pmap_copy");
}

void
pmap_zero_page(paddr_t p)
{
	PMDEBUG(("pmap_zero_page: paddr %o\n", p >> 2));
	mk2word(p);

	*mappte = PG_IMM | PG_WRITE | PGNO(p);
	memset(mapaddr, 0, PGSZ);
	*mappte = 0;
	CLRPT((int)mapaddr);
}

void
pmap_copy_page(paddr_t p1, paddr_t p2)
{
	panic("pmap_copy_page");
}

/*
 * Init VM pmap system. Nothing special to do.
 */
void
pmap_init()
{
}

/*
 * Remove all mappings.
 */
void
pmap_remove_all(struct pmap *p)
{
}
