/* $NetBSD: pmap.c,v 1.53 2011/09/03 18:42:13 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.53 2011/09/03 18:42:13 jmcneill Exp $");

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
	struct 		pv_entry *pv_next;
	pmap_t		pv_pmap;
	uintptr_t	pv_ppn;		/* physical page number */
	uintptr_t	pv_lpn;		/* logical page number  */
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

static pmap_t active_pmap = NULL;

static char  mem_name[20] = "";
static int   mem_fh;
static void *mem_uvm;	/* keeps all memory managed by UVM */

static int phys_npages = 0;
static int pm_nentries = 0;
static uint64_t pm_entries_size = 0;

static struct pool pmap_pool;

/* forwards */
void		pmap_bootstrap(void);
static void	pmap_page_activate(struct pv_entry *pv);
static void	pmap_page_deactivate(struct pv_entry *pv);
static void	pv_update(struct pv_entry *pv);
static void	pmap_update_page(uintptr_t ppn);
bool		pmap_fault(pmap_t pmap, vaddr_t va, vm_prot_t *atype);

static struct 	pv_entry *pv_get(pmap_t pmap, uintptr_t ppn, uintptr_t lpn);
static struct 	pv_entry *pv_alloc(void);
static void	pv_free(struct pv_entry *pv);
static void	pmap_deferred_init(void);

extern void	setup_signal_handlers(void);

/* exposed to signal handler */
vaddr_t kmem_k_start, kmem_k_end;
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
	ssize_t kmem_len, user_len, barrier_len, uvm_len;
	ssize_t pv_table_size;
	vaddr_t free_start, free_end;
	vaddr_t mpos;
	paddr_t pa;
	vaddr_t va;
	uintptr_t pg;
	void *addr;
	char dummy;
	int err;

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

	/* calculate memory lengths */
	kmem_len    = (physmem/2) * PAGE_SIZE;
	user_len    = MEMSIZE * 1024;
	barrier_len = 2 * 1024 * 1024;

	/* calculate kernel section (R-X) */
	kmem_k_start = (vaddr_t) PAGE_SIZE * (atop(_init)   );
	kmem_k_end   = (vaddr_t) PAGE_SIZE * (atop(&etext) + 1);

	/* claim memory with 2 pages more */
	uvm_len = kmem_len + barrier_len + user_len + barrier_len + 2*PAGE_SIZE;
	err = thunk_posix_memalign(&mem_uvm, PAGE_SIZE, uvm_len);
	if (err)
		panic("pmap_bootstrap: couldn't allocate uvm memory");
	/* make page aligned */
	mpos = round_page((vaddr_t) mem_uvm);// + PAGE_SIZE;
	if (!((void *) mpos >= mem_uvm))
		panic("pmap_bootstrap: mpos miscalculation");

	/* low barrier (---) */
	mpos += barrier_len;

	/* claim an area for userland (---/R--/RW-/RWX) */
	kmem_user_start = mpos;
	mpos += user_len;
	kmem_user_end   = mpos;
	/* TODO make a better user space size estimate */

	/* upper barrier (---) */
	mpos += barrier_len;

	/* calculate KVM section (RW-) */
	kmem_ext_start = mpos;
	mpos += kmem_len;
	kmem_ext_end   = mpos;

#if 1
	/* protect complete UVM area (---) */
	addr = thunk_mmap((void*) mem_uvm,
		uvm_len,
		THUNK_PROT_NONE,
		THUNK_MAP_ANON | THUNK_MAP_FIXED | THUNK_MAP_PRIVATE,
		-1, 0);
	if (addr != (void *) mem_uvm)
		panic("pmap_bootstrap: uvm space protection barrier failed (%p)\n", (void *)addr);
#endif

	aprint_debug("\nMemory summary\n");
	aprint_debug("\tkmem_k_start\t%p\n",    (void *) kmem_k_start);
	aprint_debug("\tkmem_k_end\t%p\n",      (void *) kmem_k_end);
	aprint_debug("\tkmem_ext_start\t%p\n",  (void *) kmem_ext_start);
	aprint_debug("\tkmem_ext_end\t%p\n",    (void *) kmem_ext_end);
	aprint_debug("\tkmem_user_start\t%p\n", (void *) kmem_user_start);
	aprint_debug("\tkmem_user_end\t%p\n",   (void *) kmem_user_end);
	aprint_debug("\n\n\n");

	aprint_debug("Creating memory mapped backend\n");

	/* create memory file since mmap/maccess only can be on files */
	strlcpy(mem_name, "/tmp/netbsd.XXXXXX", sizeof(mem_name));
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

	/* protect the current kernel section */
	/* XXX kernel stack? */
	err = thunk_mprotect((void *) kmem_k_start, kmem_k_end - kmem_k_start,
		THUNK_PROT_READ | THUNK_PROT_EXEC);
	assert(err == 0);

	/* set up pv_table; bootstrap problem! */
	fpos = 0;
	free_start = fpos;     /* in physical space ! */
	free_end   = file_len; /* in physical space ! */

	phys_npages = (free_end - free_start) / PAGE_SIZE;
	pv_table_size = round_page(phys_npages * sizeof(struct pv_entry));

	aprint_debug("claiming %"PRIu64" KB of pv_table for "
		"%"PRIdPTR" pages of physical memory\n",
		(uint64_t) pv_table_size/1024, (uintptr_t) phys_npages);

	kmem_ext_cur_start = kmem_ext_start;
	pv_fpos = fpos;
	pv_table = (struct pv_entry *) kmem_ext_cur_start;
	addr = thunk_mmap(pv_table, pv_table_size,
		THUNK_PROT_READ | THUNK_PROT_WRITE,
		THUNK_MAP_FILE | THUNK_MAP_FIXED | THUNK_MAP_SHARED,
		mem_fh, pv_fpos);
	if (addr != (void *) kmem_ext_start)
		panic("pmap_bootstrap: can't map in pv table\n");

	memset(pv_table, 0, pv_table_size);	/* test and clear */

	aprint_debug("pv_table initialiased correctly, mmap works\n");

	/* advance */
	kmem_ext_cur_start += pv_table_size;
	fpos += pv_table_size;

	/* set up kernel pmap */
	pm_nentries = (VM_MAX_ADDRESS - VM_MIN_ADDRESS) / PAGE_SIZE;
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
		THUNK_PROT_READ | THUNK_PROT_WRITE,
		THUNK_MAP_FILE | THUNK_MAP_FIXED | THUNK_MAP_SHARED,
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

	setup_signal_handlers();
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
	aprint_debug("\tpmap %p\n", pmap);

	return pmap;
}

void
pmap_destroy(pmap_t pmap)
{
	int i;

	/* if multiple references exist just remove a reference */
	aprint_debug("pmap_destroy %p\n", pmap);
	if (--pmap->pm_count > 0)
		return;

	/* safe guard against silly errors */
	KASSERT((pmap->pm_flags & PM_ACTIVE) == 0);
	KASSERT(pmap->pm_stats.resident_count == 0);
	KASSERT(pmap->pm_stats.wired_count == 0);
#ifdef DIAGNOSTIC
	for (i = 0; i < pm_nentries; i++)
		if (pmap->pm_entries[i] != NULL)
			panic("pmap_destroy: pmap isn't empty");
#endif
	free((void *)pmap->pm_entries, M_VMPMAP);
	pool_put(&pmap_pool, pmap);
}

void
pmap_reference(pmap_t pmap)
{
	aprint_debug("pmap_reference %p\n", (void *) pmap);
	pmap->pm_count++;
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
pv_get(pmap_t pmap, uintptr_t ppn, uintptr_t lpn)
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
	printf("pv_get: multiple mapped page ppn %"PRIdPTR", lpn %"PRIdPTR"\n",
		 ppn, lpn);
assert(ppn < phys_npages);
assert(ppn >= 0);
	pv = pv_alloc();
	if (pv == NULL)
		return NULL;

	pv->pv_next = pv_table[ppn].pv_next;
	pv_table[ppn].pv_next = pv;
	pmap->pm_stats.resident_count++;

	return pv;
}

/*
 * Check if the given page fault was our reference / modified emulation fault;
 * if so return true otherwise return false and let uvm handle it
 */
bool
pmap_fault(pmap_t pmap, vaddr_t va, vm_prot_t *atype)
{
	struct pv_entry *pv, *ppv;
	uintptr_t lpn, ppn;
	int prot, cur_prot, diff;

	/* get current protection settings */

	aprint_debug("pmap_fault pmap %p, va %p\n", pmap, (void *) va);

	/* get logical page from vaddr */
	lpn = atop(va - VM_MIN_ADDRESS);	/* V->L */
	pv  = pmap->pm_entries[lpn];

	/* not known! then it must be UVM's work */
	if (pv == NULL) {
aprint_debug("no mapping yet\n");
		*atype = VM_PROT_READ;		/* assume it was a read */
		return false;
	}

	/* determine physical address and lookup 'root' pv_entry */
	ppn = pv->pv_ppn;
	ppv = &pv_table[ppn];

	/* if unmanaged we just make sure it is there! */
	if (ppv->pv_vflags & PV_UNMANAGED) {
		printf("%s: oops warning unmanaged page %"PRIiPTR" faulted\n",
			__func__, ppn);
		/* atype not set */
		pmap_page_activate(pv);
		return true;
	}

	/* determine pmap access type (mmap doesnt need to be 1:1 on VM_PROT_) */
	prot = pv->pv_prot;
	cur_prot = VM_PROT_NONE;
	if (pv->pv_mmap_ppl & THUNK_PROT_READ)
		cur_prot |= VM_PROT_READ;
	if (pv->pv_mmap_ppl & THUNK_PROT_WRITE)
		cur_prot |= VM_PROT_WRITE;
	if (pv->pv_mmap_ppl & THUNK_PROT_EXEC)
		cur_prot |= VM_PROT_EXECUTE;

	diff = prot & (prot ^ cur_prot);

aprint_debug("prot = %d, cur_prot = %d, diff = %d\n", prot, cur_prot, diff);
	*atype = VM_PROT_READ;  /* assume its a read error */
	if (diff & VM_PROT_READ) {
		if ((ppv->pv_pflags & PV_REFERENCED) == 0) {
			ppv->pv_pflags |= PV_REFERENCED;
			pmap_update_page(ppn);
			return true;
		}
		return false;
	}

#if 0
	/* this might be questionable */
	if (diff & VM_PROT_EXECUTE) {
		*atype = VM_PROT_EXECUTE; /* assume it was executing */
		if (prot & VM_PROT_EXECUTE) {
			if ((ppv->pv_pflags & PV_REFERENCED) == 0) {
				ppv->pv_pflags |= PV_REFERENCED;
				pmap_update_page(ppn);
				return true;
			}
		}
		return false;
	}
#endif

	*atype = VM_PROT_WRITE; /* assume its a write error */
	if (diff & VM_PROT_WRITE) {
		if (prot & VM_PROT_WRITE) {
aprint_debug("should be allowed to write\n");
			if ((ppv->pv_pflags & PV_MODIFIED) == 0) {
aprint_debug("was marked unmodified\n");
				ppv->pv_pflags |= PV_MODIFIED;
				pmap_update_page(ppn);
				return true;
			}
		}
		return false;
	}

	/* not due to our r/m handling, let uvm handle it ! */
	return false;
}


static void
pmap_page_activate(struct pv_entry *pv)
{
	paddr_t pa = pv->pv_ppn * PAGE_SIZE;
	vaddr_t va = pv->pv_lpn * PAGE_SIZE + VM_MIN_ADDRESS; /* L->V */
	void *addr;

	addr = thunk_mmap((void *) va, PAGE_SIZE, pv->pv_mmap_ppl,
		THUNK_MAP_FILE | THUNK_MAP_FIXED | THUNK_MAP_SHARED,
		mem_fh, pa);
	aprint_debug("page_activate: (va %p, pa %p, ppl %d) -> %p\n",
		(void *) va, (void *) pa, pv->pv_mmap_ppl, (void *) addr);
	if (addr != (void *) va)
		panic("pmap_page_activate: mmap failed");
}

static void
pmap_page_deactivate(struct pv_entry *pv)
{
	paddr_t pa = pv->pv_ppn * PAGE_SIZE;
	vaddr_t va = pv->pv_lpn * PAGE_SIZE + VM_MIN_ADDRESS; /* L->V */
	void *addr;

	addr = thunk_mmap((void *) va, PAGE_SIZE, THUNK_PROT_NONE,
		THUNK_MAP_FILE | THUNK_MAP_FIXED | THUNK_MAP_SHARED,
		mem_fh, pa);
	aprint_debug("page_deactivate: (va %p, pa %p, ppl %d) -> %p\n",
		(void *) va, (void *) pa, pv->pv_mmap_ppl, (void *) addr);
	if (addr != (void *) va)
		panic("pmap_page_deactivate: mmap failed");
}

static void
pv_update(struct pv_entry *pv)
{
	int pflags, vflags;
	int mmap_ppl;

	/* get our per-physical-page flags */
	pflags = pv_table[pv->pv_ppn].pv_pflags;
	vflags = pv_table[pv->pv_ppn].pv_vflags;

	KASSERT(THUNK_PROT_READ == VM_PROT_READ);
	KASSERT(THUNK_PROT_WRITE == VM_PROT_WRITE);
	KASSERT(THUNK_PROT_EXEC == VM_PROT_EXECUTE);

	/* create referenced/modified emulation */
	if ((pv->pv_prot & VM_PROT_WRITE) &&
	    (pflags & PV_REFERENCED) && (pflags & PV_MODIFIED))
		mmap_ppl = THUNK_PROT_READ | THUNK_PROT_WRITE;
	else if ((pv->pv_prot & (VM_PROT_READ | VM_PROT_EXECUTE)) &&
		 (pflags & PV_REFERENCED))
		mmap_ppl = THUNK_PROT_READ;
	else
		mmap_ppl = THUNK_PROT_NONE;

	/* unmanaged pages are special; they dont track r/m */
	if (vflags & PV_UNMANAGED)
		mmap_ppl = THUNK_PROT_READ | THUNK_PROT_WRITE;

	pv->pv_mmap_ppl = mmap_ppl;
}

/* update mapping of a physical page */
static void
pmap_update_page(uintptr_t ppn)
{
	struct pv_entry *pv;

	for (pv = &pv_table[ppn]; pv != NULL; pv = pv->pv_next) {
		aprint_debug("pmap_update_page: ppn %"PRIdPTR", pv->pv_map = %p\n",
			ppn, pv->pv_pmap);
		if (pv->pv_pmap != NULL) {
			pv_update(pv);
			pmap_page_activate(pv);
		}
	}
}

static int
pmap_do_enter(pmap_t pmap, vaddr_t va, paddr_t pa, vm_prot_t prot, uint flags, int unmanaged)
{
	struct pv_entry *pv, *ppv;
	uintptr_t ppn, lpn;
	int s;

	/* to page numbers */
	ppn = atop(pa);
	lpn = atop(va - VM_MIN_ADDRESS);	/* V->L */
#ifdef DIAGNOSTIC
	if ((va < VM_MIN_ADDRESS) || (va > VM_MAX_ADDRESS))
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
	} else {
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
pv_release(pmap_t pmap, uintptr_t ppn, uintptr_t lpn)
{
	struct pv_entry *pv, *npv;

	aprint_debug("pv_release ppn %"PRIdPTR", lpn %"PRIdPTR"\n", ppn, lpn);
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
	uintptr_t slpn, elpn, lpn;
	struct pv_entry *pv;
	int s;

	slpn = atop(sva - VM_MIN_ADDRESS);	/* V->L */
 	elpn = atop(eva - VM_MIN_ADDRESS);	/* V->L */

	aprint_debug("pmap_remove() called from "
		"lpn %"PRIdPTR" to lpn %"PRIdPTR"\n", slpn, elpn);

	s = splvm();
	for (lpn = slpn; lpn < elpn; lpn++) {
		pv = pmap->pm_entries[lpn];
		if (pv != NULL) {
			if (pmap->pm_flags & PM_ACTIVE) {
				pmap_page_deactivate(pv);
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
aprint_debug("pmap_remove_all() called\n");
}

void
pmap_protect(pmap_t pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
aprint_debug("pmap_protect not implemented sva %p, eva %p, prot %d\n", (void *) sva, (void *) eva, prot);
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
#ifdef DIAGNOSTIC
	if ((va < VM_MIN_ADDRESS) || (va > VM_MAX_ADDRESS))
		panic("pmap_extract: invalid va isued\n");
#endif
	pv = pmap->pm_entries[atop(va - VM_MIN_ADDRESS)]; /* V->L */

	if (pv == NULL)
		return false;

	if (pap)
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
	pmap_remove(pmap_kernel(), va, va + size);
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
	aprint_debug("pmap_update (dummy)\n");
}

void
pmap_activate(struct lwp *l)
{
	struct proc *p = l->l_proc;
	pmap_t pmap;

	pmap = p->p_vmspace->vm_map.pmap;
	aprint_debug("pmap_activate for lwp %p, pmap = %p\n", l, pmap);

	if (pmap == pmap_kernel())
		return; /* kernel pmap is always active */

	KASSERT(active_pmap == NULL);
	KASSERT((pmap->pm_flags & PM_ACTIVE) == 0);

	active_pmap = pmap;
	pmap->pm_flags |= PM_ACTIVE;
}

void
pmap_deactivate(struct lwp *l)
{
	struct proc *p = l->l_proc;
	pmap_t pmap;
	int i;

	pmap = p->p_vmspace->vm_map.pmap;
	aprint_debug("pmap_DEactivate for lwp %p, pmap = %p\n", l, pmap);

	if (pmap == pmap_kernel())
		return; /* kernel pmap is always active */

	KASSERT(pmap == active_pmap);
	KASSERT(pmap->pm_flags & PM_ACTIVE);

	active_pmap = NULL;
	pmap->pm_flags &=~ PM_ACTIVE;
	for (i = 0; i < 1024; i++) {
		if (pmap->pm_entries[i] != NULL) {
			pmap_page_deactivate(pmap->pm_entries[i]);
//			MEMC_WRITE(pmap->pm_entries[i]->pv_deactivate);
		}
	}
	/* dummy */
//	cpu_cache_flush();
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

/* change access permissions on a given physical page */
void
pmap_page_protect(struct vm_page *page, vm_prot_t prot)
{
	intptr_t ppn;
	struct pv_entry *pv, *npv;

	ppn = atop(VM_PAGE_TO_PHYS(page));
	aprint_debug("pmap_page_protect page %"PRIiPTR" to prot %d\n", ppn, prot);

	if (prot == VM_PROT_NONE) {
		/* visit all mappings */
		npv = pv = &pv_table[ppn];
		while (pv != NULL && pv->pv_pmap != NULL) {
			/* skip unmanaged entries */
			if (pv->pv_vflags & PV_UNMANAGED) {
				pv = pv->pv_next;
				continue;
			}

			/* if in an active pmap deactivate */
			if (pv->pv_pmap->pm_flags & PM_ACTIVE)
				pmap_page_deactivate(pv);

			/* if not on the head, remember our next */
			if (pv != &pv_table[ppn])
				npv = pv->pv_next;

			/* remove from pmap */
			pv->pv_pmap->pm_entries[pv->pv_lpn] = NULL;
			if (pv->pv_vflags & PV_WIRED)
				pv->pv_pmap->pm_stats.wired_count--;
			pv_release(pv->pv_pmap, ppn, pv->pv_lpn);

			pv = npv;
		}
	} else if (prot != VM_PROT_ALL) {
		/* visit all mappings */
		for (pv = &pv_table[ppn]; pv != NULL; pv = pv->pv_next) {
			/* if managed and in a pmap restrict access */
			if ((pv->pv_pmap != NULL) &&
			    ((pv->pv_vflags & PV_UNMANAGED) == 0)) {
				pv->pv_prot &= prot;
				pv_update(pv);
				/* if in active pmap (re)activate page */
				if (pv->pv_pmap->pm_flags & PM_ACTIVE)
					pmap_page_activate(pv);
			}
		}
	}
}

bool
pmap_clear_modify(struct vm_page *page)
{
	struct pv_entry *pv;
	uintptr_t ppn;
	bool rv;

	ppn = atop(VM_PAGE_TO_PHYS(page));
	rv = pmap_is_modified(page);

	aprint_debug("pmap_clear_modify page %"PRIiPTR"\n", ppn);

	/* if marked modified, clear it in all the pmap's referencing it */
	if (rv) {
		/* if its marked modified in a kernel mapping, don't clear it */
		for (pv = &pv_table[ppn]; pv != NULL; pv = pv->pv_next)
			if (pv->pv_pmap == pmap_kernel() &&
			    (pv->pv_prot & VM_PROT_WRITE))
				return rv;
		/* clear it */
		pv_table[ppn].pv_pflags &= ~PV_MODIFIED;
		pmap_update_page(ppn);
	}
	return rv;
}

bool
pmap_clear_reference(struct vm_page *page)
{
	uintptr_t ppn;
	bool rv;

	ppn = atop(VM_PAGE_TO_PHYS(page));
	rv = pmap_is_referenced(page);

	aprint_debug("pmap_clear_reference page %"PRIiPTR"\n", ppn);

	if (rv) {
		pv_table[ppn].pv_pflags &= ~PV_REFERENCED;
		pmap_update_page(ppn);
	}
	return rv;
}

bool
pmap_is_modified(struct vm_page *page)
{
	intptr_t ppn;
	bool rv;

	ppn = atop(VM_PAGE_TO_PHYS(page));
	rv = (pv_table[ppn].pv_pflags & PV_MODIFIED) != 0;

	aprint_debug("pmap_is_modified page %"PRIiPTR" : %s\n", ppn, rv?"yes":"no");

	return rv;
}

bool
pmap_is_referenced(struct vm_page *page)
{
	intptr_t ppn;

	ppn = atop(VM_PAGE_TO_PHYS(page));
	aprint_debug("pmap_is_referenced page %"PRIiPTR"\n", ppn);

	return (pv_table[ppn].pv_pflags & PV_REFERENCED) != 0;
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

