/* $NetBSD: pmap.c,v 1.25 2011/08/23 18:37:51 reinoud Exp $ */

/*-
 * Copyright (c) 2011 Reinoud Zandijk <reinoud@NetBSD.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.25 2011/08/23 18:37:51 reinoud Exp $");

#include "opt_memsize.h"
#include "opt_kmempages.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/mutex.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <machine/thunk.h>

#include <uvm/uvm.h>

struct pv_entry {
	struct 	pv_entry *pv_next;
	pmap_t	pv_pmap;
	int	pv_ppn;		/* physical page number */
	int	pv_lpn;		/* logical page number  */
	vm_prot_t	pv_prot;	/* logical protection */
	int		pv_mmap_ppl;	/* programmed protection */
	uint8_t		pv_vflags;	/* per mapping flags */
#define PV_WIRED	0x01		/* wired mapping */
#define PV_UNMANAGED	0x02		/* entered by pmap_kenter_ */
	uint8_t		pv_pflags;	/* per phys page flags */
#define PV_REFERENCED	0x01
#define PV_MODIFIED	0x02
};

/* this is a guesstimate of 16KB/process on amd64, or around 680+ mappings */
#define PM_MIN_NENTRIES ((int) (16*1024 / sizeof(pv_entry)))
struct pmap {
	int	pm_count;
	int	pm_flags;
#define PM_ACTIVE 0x01
	struct	pmap_statistics pm_stats;
	struct	pv_entry **pm_entries;
};

static struct pv_entry *pv_table;
static struct pmap	pmap_kernel_store;
struct pmap * const	kernel_pmap_ptr = &pmap_kernel_store;

static char mem_name[20] = "";
static int mem_fh;

static int phys_npages = 0;
static int pm_nentries = 0;
static uint64_t pm_entries_size = 0;

static struct pool pmap_pool;

/* forwards */
void		pmap_bootstrap(void);
static void	pmap_page_activate(struct pv_entry *pv);
static void	pv_update(struct pv_entry *pv);
static void	pmap_update_page(int ppn);

static struct 	pv_entry *pv_get(pmap_t pmap, int ppn, int lpn);
static struct 	pv_entry *pv_alloc(void);
static void	pv_free(struct pv_entry *pv);
//void	pmap_remove(pmap_t pmap, vaddr_t sva, vaddr_t eva);
static void	pmap_deferred_init(void);

/* exposed to signal handler */
vaddr_t kmem_k_start, kmem_k_end;
vaddr_t kmem_data_start, kmem_data_end;
vaddr_t kmem_ext_start, kmem_ext_end;
vaddr_t kmem_user_start, kmem_user_end;
vaddr_t kmem_ext_cur_start, kmem_ext_cur_end;

void
pmap_bootstrap(void)
{
	struct pmap *pmap;
	ssize_t fpos, file_len;
	ssize_t pv_fpos, pm_fpos;
	ssize_t wlen;
	uint64_t pv_table_size;
	paddr_t free_start, free_end;
	paddr_t pa;
	vaddr_t va;
	void *addr;
	char dummy;
	int pg, err;

	extern void _init(void);	/* start of kernel               */
	extern int etext;		/* end of the kernel             */
	extern int edata;		/* end of the init. data segment */
	extern int end;			/* end of bss                    */

	aprint_debug("Information retrieved from elf image\n");
	aprint_debug("start kernel        at %p\n", _init);
	aprint_debug("  end kernel        at %p\n", &etext);
	aprint_debug("  end of init. data at %p\n", &edata);
	aprint_debug("1st end of data     at %p\n", &end);
	aprint_debug("CUR end data        at %p\n", thunk_sbrk(0));

	/* calculate sections of the kernel (R+X) */
	kmem_k_start = (vaddr_t) PAGE_SIZE * (atop(_init)   );
	kmem_k_end   = (vaddr_t) PAGE_SIZE * (atop(&etext) + 1);

	/* read only section (for userland R, kernel RW) */
	kmem_data_start = (vaddr_t) PAGE_SIZE * (atop(&etext));
	kmem_data_end   = (vaddr_t) PAGE_SIZE * (atop(&end) + 1);
	if (kmem_data_end > (vaddr_t) thunk_sbrk(0)) {
		aprint_debug("sbrk() has advanced, catching up\n");
		kmem_data_end = (vaddr_t) PAGE_SIZE * (atop(thunk_sbrk(0))+1);
	}

#ifdef DIAGNOSTIC
	if (kmem_k_end >= kmem_data_start) {
		aprint_debug("end of kernel and kernel exec could clash\n");
		aprint_debug("   kmem_k_end = %p, kmem_data_start = %p\n",
			(void *) kmem_k_end, (void *) kmem_data_start);
		aprint_debug("fixing\n");
	}
#endif
	/* on clash move RO segment so that all code is executable */
	if (kmem_k_end >= kmem_data_start)
		kmem_data_start = kmem_k_end;

	/* claim an area for kernel data space growth (over dim) */
	kmem_ext_start   = kmem_data_end;
	kmem_ext_end     = kmem_ext_start + (physmem/2) * PAGE_SIZE;

	/* claim an area for userland */
	kmem_user_start = kmem_ext_end;
	kmem_user_end   = kmem_user_start + 1024 * MEMSIZE;
	/* TODO make a better user space size estimate */

	/* claim dummy space over all we need just to make fun of sbrk */
	addr = thunk_mmap((void*) kmem_data_end,
		kmem_user_end - kmem_data_end,
		PROT_NONE,
		MAP_ANON | MAP_FIXED,
		-1, 0);
	if (addr != (void *) kmem_data_end)
		panic("pmap_bootstrap: protection barrier failed\n");

	if (kmem_user_end < kmem_user_start)
		panic("pmap_bootstrap: to small memorysize specified");

	aprint_debug("\nMemory summary\n");
	aprint_debug("\tkmem_k_start\t%p\n",    (void *) kmem_k_start);
	aprint_debug("\tkmem_k_end\t%p\n",      (void *) kmem_k_end);
	aprint_debug("\tkmem_data_start\t%p\n", (void *) kmem_data_start);
	aprint_debug("\tkmem_data_end\t%p\n",   (void *) kmem_data_end);
	aprint_debug("\tkmem_ext_start\t%p\n",  (void *) kmem_ext_start);
	aprint_debug("\tkmem_ext_end\t%p\n",    (void *) kmem_ext_end);
	aprint_debug("\tkmem_user_start\t%p\n", (void *) kmem_user_start);
	aprint_debug("\tkmem_user_end\t%p\n",   (void *) kmem_user_end);
	aprint_debug("\n\n\n");

	aprint_debug("Creating memory mapped backend\n");

	/* create memory file since mmap/maccess only can be on files */
	strlcpy(mem_name, "/tmp/netbsd.XXXXX", sizeof(mem_name));
	mem_fh = thunk_mkstemp(mem_name);
	if (mem_fh < 0)
		panic("pmap_bootstrap: can't create memory file\n");
	/* unlink the file so space is freed when we quit */
	if (thunk_unlink(mem_name) == -1)
		panic("pmap_bootstrap: can't unlink %s", mem_name);

	/* file_len is the backing store length, nothing to do with placement */
	file_len = 1024 * MEMSIZE;
	wlen = thunk_pwrite(mem_fh, &dummy, 1, file_len - 1);
	if (wlen != 1)
		panic("pmap_bootstrap: can't grow file\n");

	/* (un)protect the current kernel and data sections */
	/* XXX kernel stack? */
	err = thunk_mprotect((void *) kmem_k_start, kmem_k_end - kmem_k_start,
		PROT_READ | PROT_EXEC);
	assert(err == 0);
	err = thunk_mprotect((void *) kmem_k_end, kmem_data_end - kmem_k_end,
		PROT_READ | PROT_WRITE);
	assert(err == 0);

	/* set up pv_table; bootstrap problem! */
	fpos = 0;
	free_start = fpos;     /* in physical space ! */
	free_end   = file_len; /* in physical space ! */

	phys_npages = (free_end - free_start) / PAGE_SIZE;
	pv_table_size = round_page(phys_npages * sizeof(struct pv_entry));
	aprint_debug("claiming %"PRIu64" KB of pv_table for "
		"%d pages of physical memory\n", pv_table_size/1024, phys_npages);

	kmem_ext_cur_start = kmem_ext_start;
	pv_fpos = fpos;
	pv_table = (struct pv_entry *) kmem_ext_cur_start;
	addr = thunk_mmap(pv_table, pv_table_size,
		PROT_READ | PROT_WRITE,
		MAP_FILE | MAP_FIXED,
		mem_fh, pv_fpos);
	if (addr != (void *) kmem_ext_start)
		panic("pmap_bootstrap: can't map in pv table\n");

	memset(pv_table, 0, pv_table_size);	/* test and clear */

	aprint_debug("pv_table initialiased correctly, mmap works\n");

	/* advance */
	kmem_ext_cur_start += pv_table_size;
	fpos += pv_table_size;

	/* set up kernel pmap */
	pm_nentries = (kmem_user_end - kmem_k_start) / PAGE_SIZE;
	pm_entries_size = round_page(pm_nentries * sizeof(struct pv_entry *));
	aprint_debug("pmap va->pa lookup table is %"PRIu64" KB for %d logical pages\n",
		pm_entries_size/1024, pm_nentries);

        pmap = pmap_kernel(); 
        memset(pmap, 0, sizeof(*pmap)); 
	pmap->pm_count = 1;		/* reference */
	pmap->pm_flags = PM_ACTIVE;	/* kernel pmap is allways active */
	pmap->pm_entries = (struct pv_entry **) kmem_ext_cur_start;

	pm_fpos = fpos;
	addr = thunk_mmap(pmap->pm_entries, pm_entries_size,
		PROT_READ | PROT_WRITE,
		MAP_FILE | MAP_FIXED,
		mem_fh, pm_fpos);
	if (addr != (void *) pmap->pm_entries)
		panic("pmap_bootstrap: can't map in pmap entries\n");

	memset(pmap->pm_entries, 0, pm_entries_size);	/* test and clear */

	aprint_debug("kernel pmap entries initialiased correctly\n");

	/* advance */
	kmem_ext_cur_start += pm_entries_size;
	fpos += pm_entries_size;

	/* kmem used [kmem_ext_start - kmem_ext_cur_start] */
	kmem_ext_cur_end = kmem_ext_cur_start;

	/* manually enter the mappings into the kernel map */
	for (pg = 0; pg < pv_table_size; pg += PAGE_SIZE) {
		pa = pv_fpos + pg;
		va = (vaddr_t) pv_table + pg;
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE, 0);
	}
	aprint_debug("pv_table mem added to the kernel pmap\n");
	for (pg = 0; pg < pm_entries_size; pg += PAGE_SIZE) {
		pa = pm_fpos + pg;
		va = (vaddr_t) pmap->pm_entries + pg;
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE, 0);
	}
	aprint_debug("kernel pmap entries mem added to the kernel pmap\n");

	/* add file space to uvm's FREELIST */
	/* XXX really from 0? or from fpos to have better stats */
	uvm_page_physload(atop(0),
	    atop(free_end),
	    atop(free_start + fpos), /* mark used till fpos */
	    atop(free_end), 
	    VM_FREELIST_DEFAULT);

	aprint_debug("leaving pmap_bootstrap:\n");
	aprint_debug("\t%"PRIu64" MB of physical pages left\n",
		(uint64_t) (free_end - (free_start + fpos))/1024/1024);
	aprint_debug("\t%"PRIu64" MB of kmem left\n",
		(uint64_t) (kmem_ext_end - kmem_ext_cur_end)/1024/1024);
}

void
pmap_init(void)
{
	/* All deferred to pmap_create, because malloc() is nice. */
}

/* return kernel space start and end (including growth) */
void
pmap_virtual_space(vaddr_t *vstartp, vaddr_t *vendp)
{
	*vstartp = kmem_ext_cur_start;	/* min to map in */
	*vendp   = kmem_ext_end;	/* max available */
}

static void
pmap_deferred_init(void)
{
	/* XXX we COULD realloc our pv_table etc with malloc() but for what? */

	/* create pmap pool */
	pool_init(&pmap_pool, sizeof(struct pmap), 0, 0, 0,
	    "pmappool", NULL, IPL_NONE);
}

pmap_t
pmap_create(void)
{
	static int pmap_initialised = 0;
	struct pmap *pmap;

	if (!pmap_initialised) {
		pmap_deferred_init();
		pmap_initialised = 1;
	}

	aprint_debug("pmap_create\n");
	pmap = pool_get(&pmap_pool, PR_WAITOK);
	memset(pmap, 0, sizeof(*pmap));
		
	pmap->pm_count = 1;
	pmap->pm_flags = 0;
	pmap->pm_entries = (struct pv_entry **) malloc(
		pm_entries_size, M_VMPMAP,
		M_WAITOK | M_ZERO);

	return pmap;
}

void
pmap_destroy(pmap_t pmap)
{
aprint_debug("pmap_destroy not implemented!\n");
}

void
pmap_reference(pmap_t pmap)
{
aprint_debug("pmap_reference %p\n", (void *) pmap);
}

long
pmap_resident_count(pmap_t pmap)
{
	return pmap->pm_stats.resident_count;
}

long
pmap_wired_count(pmap_t pmap)
{
	return pmap->pm_stats.wired_count;
}

static struct pv_entry *
pv_alloc(void)
{
	return malloc(sizeof(struct pv_entry), M_VMPMAP, M_NOWAIT | M_ZERO);
}

static void
pv_free(struct pv_entry *pv)
{
	free(pv, M_VMPMAP);
}

static struct pv_entry *
pv_get(pmap_t pmap, int ppn, int lpn)
{
	struct pv_entry *pv;

	/* If the head entry's free use that. */
	pv = &pv_table[ppn];
	if (pv->pv_pmap == NULL) {
		pmap->pm_stats.resident_count++;
		return pv;
	}
	/* If this mapping exists already, use that. */
	for (pv = pv; pv != NULL; pv = pv->pv_next) {
		if (pv->pv_pmap == pmap && pv->pv_lpn == lpn) {
			return pv;
		}
	}
	/* Otherwise, allocate a new entry and link it in after the head. */
	printf("pv_get: multiple mapped page ppn %d, lpn %d\n", ppn, lpn);
assert(ppn < phys_npages);
assert(ppn >= 0);
panic("pv_get: multiple\n");
	pv = pv_alloc();
	if (pv == NULL)
		return NULL;

	pv->pv_next = pv_table[ppn].pv_next;
	pv_table[ppn].pv_next = pv;
	pmap->pm_stats.resident_count++;

	return pv;
}

static void
pmap_page_activate(struct pv_entry *pv)
{
	paddr_t pa = pv->pv_ppn * PAGE_SIZE;
	vaddr_t va = pv->pv_lpn * PAGE_SIZE + kmem_k_start; /* XXX V->A make new var */

	void *addr;

	addr = thunk_mmap((void *) va, PAGE_SIZE, pv->pv_mmap_ppl,
		MAP_FILE | MAP_FIXED,
		mem_fh, pa);
aprint_debug("page_activate: (va %p, pa %p, ppl %d) -> %p\n", (void *) va, (void *) pa, pv->pv_mmap_ppl, (void *) addr);
	if (addr != (void *) va)
		panic("pmap_page_activate: mmap failed");
}


static void
pv_update(struct pv_entry *pv)
{
	int pflags;
	int mmap_ppl;

	/* get our per-physical-page flags */
	pflags = pv_table[pv->pv_ppn].pv_pflags;

	/* create referenced/modified emulation */
	if ((pv->pv_prot & VM_PROT_WRITE) &&
	    (pflags & PV_REFERENCED) && (pflags & PV_MODIFIED))
		mmap_ppl = PROT_READ | PROT_WRITE;
	else if ((pv->pv_prot & (VM_PROT_READ | VM_PROT_EXECUTE)) &&
		 (pflags & PV_REFERENCED))
		mmap_ppl = PROT_READ;
	else
		mmap_ppl = PROT_NONE;
	pv->pv_mmap_ppl = mmap_ppl;
}

/* update mapping of a physical page */
static void
pmap_update_page(int ppn)
{
	struct pv_entry *pv;

	for (pv = &pv_table[ppn]; pv != NULL; pv = pv->pv_next) {
		if (pv->pv_pmap != NULL) {
			pv_update(pv);
			pmap_page_activate(pv);
		}
	}
}

static int
pmap_do_enter(pmap_t pmap, vaddr_t va, paddr_t pa, vm_prot_t prot, uint flags, int unmanaged)
{
	int ppn, lpn, s;
	struct pv_entry *pv, *ppv;

	/* to page numbers */
	ppn = atop(pa);
	lpn = atop(va - kmem_k_start);	/* XXX V->A make new var */
#ifdef DIAGNOSTIC
	if ((va < kmem_k_start) || (va > kmem_user_end))
		panic("pmap_do_enter: invalid va isued\n");
#endif

	/* raise interupt level */
	s = splvm();

	/* remove existing mapping at this lpn */
	if (pmap->pm_entries[lpn] &&
	    pmap->pm_entries[lpn]->pv_ppn != ppn)
		pmap_remove(pmap, va, va + PAGE_SIZE);

	/* get our entry */
	ppv = &pv_table[ppn];
	pv = pv_get(pmap, ppn, lpn);	/* get our (copy) of pv entry */

	/* and adjust stats */
	if (pv == NULL)
		panic("pamp_do_enter: didn't find pv entry!");
	if (pv->pv_vflags & PV_WIRED)
		pmap->pm_stats.wired_count--;

	/* enter our details */
	pv->pv_pmap   = pmap;
	pv->pv_ppn    = ppn;
	pv->pv_lpn    = lpn;
	pv->pv_prot   = prot;
	pv->pv_vflags = 0;
	pv->pv_next   = NULL;
	if (flags & PMAP_WIRED)
		pv->pv_vflags |= PV_WIRED;

	if (unmanaged) {
		/* dont track r/m */
		pv->pv_vflags |= PV_UNMANAGED;
		ppv->pv_pflags |= PV_MODIFIED | PV_REFERENCED;	/* XXX */
	} else {
		/* XXX flag it dirty(?) if prot write? */
		if (flags & VM_PROT_WRITE)
			ppv->pv_pflags |= PV_REFERENCED | PV_MODIFIED;
		else if (flags & (VM_PROT_ALL))
			ppv->pv_pflags |= PV_REFERENCED;
	}

	/* map it in */
	pmap_update_page(ppn);
	pmap->pm_entries[lpn] = pv;

	/* adjust stats */
	if (pv->pv_vflags & PV_WIRED)
		pmap->pm_stats.wired_count++;

	splx(s);

	return 0;
}

int
pmap_enter(pmap_t pmap, vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	aprint_debug("pmap_enter %p : v %p, p %p, prot %d, flags %d\n",
		(void *) pmap, (void *) va, (void *) pa, (int) prot, (int) flags);
	return pmap_do_enter(pmap, va, pa, prot, flags, 0);
}

/* release the pv_entry for a mapping.  Code derived also from hp300 pmap */
static void
pv_release(pmap_t pmap, int ppn, int lpn)
{
	struct pv_entry *pv, *npv;

aprint_debug("pv_release ppn %d, lpn %d\n", ppn, lpn);
	pv = &pv_table[ppn];
	/*
	 * If it is the first entry on the list, it is actually
	 * in the header and we must copy the following entry up
	 * to the header.  Otherwise we must search the list for
	 * the entry.  In either case we free the now unused entry.
	 */
	if (pmap == pv->pv_pmap && lpn == pv->pv_lpn) {
		npv = pv->pv_next;
		if (npv) {
			/* Pull up first entry from chain. */
			memcpy(pv, npv, offsetof(struct pv_entry, pv_pflags));
			pv->pv_pmap->pm_entries[pv->pv_lpn] = pv;
			pv_free(npv);
		} else {
			memset(pv, 0, offsetof(struct pv_entry, pv_pflags));
		}
	} else {
		for (npv = pv->pv_next; npv; npv = npv->pv_next) {
			if (pmap == npv->pv_pmap && lpn == npv->pv_lpn)
				break;
			pv = npv;
		}
		KASSERT(npv != NULL);
		pv->pv_next = npv->pv_next;
		pv_free(npv);
	}
	pmap->pm_entries[lpn] = NULL;
	pmap->pm_stats.resident_count--;
}

void
pmap_remove(pmap_t pmap, vaddr_t sva, vaddr_t eva)
{
	int slpn, elpn, lpn, s;
	struct pv_entry *pv;

aprint_debug("pmap_remove() called\n");

	slpn = atop(sva); elpn = atop(eva);
	s = splvm();
	for (lpn = slpn; lpn < elpn; lpn++) {
		pv = pmap->pm_entries[lpn];
		if (pv != NULL) {
			if (pmap->pm_flags & PM_ACTIVE) {
aprint_debug("pmap_remove: haven't removed old mmap yet\n");
//				MEMC_WRITE(pv->pv_deactivate);
//				cpu_cache_flush();
			}
			pmap->pm_entries[lpn] = NULL;
			if (pv->pv_vflags & PV_WIRED)
				pmap->pm_stats.wired_count--;
			pv_release(pmap, pv->pv_ppn, lpn);
		}
	}
	splx(s);

}

void
pmap_remove_all(pmap_t pmap)
{
panic("pmap_remove_all() called\n");
}

void
pmap_protect(pmap_t pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
aprint_debug("pmap_protect not implemented\n");
}

void
pmap_unwire(pmap_t pmap, vaddr_t va)
{
aprint_debug("pmap_unwire called not implemented\n'");
}

bool
pmap_extract(pmap_t pmap, vaddr_t va, paddr_t *pap)
{
	struct pv_entry *pv;

	/* TODO protect against roque values */
	aprint_debug("pmap_extract: extracting va %p\n", (void *) va);
	pv = pmap->pm_entries[atop(va - kmem_k_start)];	/* XXX V->A make new var */

	if (pv == NULL)
		return false;

	*pap = ptoa(pv->pv_ppn);
	return true;
}

/*
 * Enter an unmanaged, `wired' kernel mapping.
 * Only to be removed by pmap_kremove()
 */
void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	aprint_debug("pmap_kenter_pa : v %p, p %p, prot %d, flags %d\n",
		(void *) va, (void *) pa, (int) prot, (int) flags);
	pmap_do_enter(pmap_kernel(),  va, pa, prot, prot | PMAP_WIRED, 1);
}

void
pmap_kremove(vaddr_t va, vsize_t size)
{
aprint_debug("pmap_kremove not implented\n'");
}

void
pmap_copy(pmap_t dst_map, pmap_t src_map, vaddr_t dst_addr, vsize_t len,
    vaddr_t src_addr)
{
aprint_debug("pmap_copy not implemented\n");
}

void
pmap_update(pmap_t pmap)
{
aprint_debug("pmap_update not implemented\n");
}

void
pmap_activate(struct lwp *l)
{
aprint_debug("pmap_activate not implemented\n");
}

void
pmap_deactivate(struct lwp *l)
{
aprint_debug("pmap_deactivate not implemented\n");
}

/* XXX braindead zero_page implementation but it works for now */
void
pmap_zero_page(paddr_t pa)
{
	char blob[PAGE_SIZE];
	int num;

	aprint_debug("pmap_zero_page: pa %p\n", (void *) pa);

	memset(blob, 0, PAGE_SIZE);
	num = thunk_pwrite(mem_fh, blob, PAGE_SIZE, pa);
	if (num != PAGE_SIZE)
		panic("pmap_zero_page couldn't write out\n");
}

void
pmap_copy_page(paddr_t src, paddr_t dst)
{
aprint_debug("pmap_copy_page not implemented\n");
}

void
pmap_page_protect(struct vm_page *pg, vm_prot_t prot)
{
aprint_debug("pmap_page_protect not implemented\n");
}

bool
pmap_clear_modify(struct vm_page *pg)
{
aprint_debug("pmap_clear_modify not implemented\n");
	return true;
}

bool
pmap_clear_reference(struct vm_page *pg)
{
aprint_debug("pmap_clear_reference not implemented\n");
	return true;
}

bool
pmap_is_modified(struct vm_page *pg)
{
aprint_debug("pmap_is_modified not implemented\n");
	return false;
}

bool
pmap_is_referenced(struct vm_page *pg)
{
aprint_debug("pmap_is_referenced not implemented\n");
	return false;
}

paddr_t
pmap_phys_address(paddr_t cookie)
{
	panic("pmap_phys_address not implemented\n");
	return ptoa(cookie);
}

vaddr_t
pmap_growkernel(vaddr_t maxkvaddr)
{
	aprint_debug("pmap_growkernel: till %p (adding %"PRIu64" KB)\n",
		(void *) maxkvaddr,
		(uint64_t) (maxkvaddr - kmem_ext_cur_end)/1024);
	if (maxkvaddr > kmem_ext_end)
		return kmem_ext_end;
	kmem_ext_cur_end = maxkvaddr;
	return kmem_ext_cur_end;
}

