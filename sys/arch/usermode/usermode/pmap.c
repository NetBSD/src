/* $NetBSD: pmap.c,v 1.112 2018/08/03 11:18:22 reinoud Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.112 2018/08/03 11:18:22 reinoud Exp $");

#include "opt_memsize.h"
#include "opt_kmempages.h"
#include "opt_misc.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/mutex.h>
#include <sys/buf.h>
#include <sys/kmem.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <machine/thunk.h>
#include <machine/machdep.h>
#include <machine/pcb.h>

#include <uvm/uvm.h>

struct pv_entry {
	struct 		pv_entry *pv_next;
	pmap_t		pv_pmap;
	uintptr_t	pv_ppn;		/* physical page number */
	uintptr_t	pv_lpn;		/* logical page number  */
	vm_prot_t	pv_prot;	/* logical protection */
	uint8_t		pv_mmap_ppl;	/* programmed protection */
	uint8_t		pv_vflags;	/* per mapping flags */
#define PV_WIRED	0x01		/* wired mapping */
#define PV_UNMANAGED	0x02		/* entered by pmap_kenter_ */
	uint8_t		pv_pflags;	/* per phys page flags */
#define PV_REFERENCED	0x01
#define PV_MODIFIED	0x02
};

#define PMAP_L2_SIZE	 PAGE_SIZE
#define PMAP_L2_NENTRY	(PMAP_L2_SIZE / sizeof(struct pv_entry *))

struct pmap_l2 {
	struct pv_entry *pm_l2[PMAP_L2_NENTRY];
};

struct pmap {
	int	pm_count;
	int	pm_flags;
#define PM_ACTIVE 0x01
	struct	pmap_statistics pm_stats;
	struct	pmap_l2 **pm_l1;
};

/*
 * pv_table is list of pv_entry structs completely spanning the total memory.
 * It is indexed on physical page number. Each entry will be daisy chained
 * with pv_entry records for each usage in all the pmaps.
 *
 * kernel_pm_entries contains all kernel L2 pages for its complete map.
 *
 */

static struct pv_entry **kernel_pm_entries;
static struct pv_entry  *pv_table;	/* physical pages info (direct mapped) */
static struct pv_entry **tlb;		/* current tlb mappings (direct mapped) */
static struct pmap	 pmap_kernel_store;
struct pmap * const	 kernel_pmap_ptr = &pmap_kernel_store;

static pmap_t active_pmap = NULL;

static char  mem_name[20] = "";
static int   mem_fh;

static int phys_npages = 0;
static int pm_nentries = 0;
static int pm_nl1 = 0;
static int pm_l1_size = 0;
static uint64_t pm_entries_size = 0;

static struct pool pmap_pool;
static struct pool pmap_pventry_pool;

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

/* exposed (to signal handler f.e.) */
vaddr_t kmem_k_start, kmem_k_end;
vaddr_t kmem_kvm_start, kmem_kvm_end;
vaddr_t kmem_user_start, kmem_user_end;
vaddr_t kmem_kvm_cur_start, kmem_kvm_cur_end;

/* amount of physical memory */
int	num_pv_entries = 0;
int	num_pmaps = 0;

#define SPARSE_MEMFILE


void
pmap_bootstrap(void)
{
	struct pmap *pmap;
	paddr_t DRAM_cfg;
	paddr_t fpos, file_len;
	paddr_t kernel_fpos, pv_fpos, tlb_fpos, pm_l1_fpos, pm_fpos;
	paddr_t wlen;
	paddr_t barrier_len;
	paddr_t pv_table_size;
	vaddr_t free_start, free_end;
	paddr_t pa;
	vaddr_t va;
	size_t  kmem_k_length, written;
	uintptr_t pg, l1;
	void *addr;
	int err;

	extern void _start(void);	/* start of kernel		 */
	extern int etext;		/* end of the kernel             */
	extern int edata;		/* end of the init. data segment */
	extern int end;			/* end of bss                    */
	vaddr_t vm_min_addr;

	vm_min_addr = thunk_get_vm_min_address();
	vm_min_addr = vm_min_addr < PAGE_SIZE ? PAGE_SIZE : vm_min_addr;

	thunk_printf_debug("Information retrieved from system and elf image\n");
	thunk_printf_debug("min VM address      at %p\n", (void *) vm_min_addr);
	thunk_printf_debug("start kernel        at %p\n", _start);
	thunk_printf_debug("  end kernel        at %p\n", &etext);
	thunk_printf_debug("  end of init. data at %p\n", &edata);
	thunk_printf_debug("1st end of data     at %p\n", &end);
	thunk_printf_debug("CUR end data        at %p\n", thunk_sbrk(0));

	barrier_len = 2 * 1024 * 1024;

	/* calculate kernel section (R-X) */
	kmem_k_start = (vaddr_t) PAGE_SIZE * (atop(_start)    );
	kmem_k_end   = (vaddr_t) PAGE_SIZE * (atop(&etext) + 1);
	kmem_k_length = kmem_k_end - kmem_k_start;

	/* calculate total available memory space & available pages */
	DRAM_cfg = (vaddr_t) TEXTADDR;
	physmem  = DRAM_cfg / PAGE_SIZE;

	/* kvm at the top */
	kmem_kvm_end    = kmem_k_start - barrier_len;
	kmem_kvm_start  = kmem_kvm_end - KVMSIZE;

	/* claim an area for userland (---/R--/RW-/RWX) */
	kmem_user_start = vm_min_addr;
	kmem_user_end   = kmem_kvm_start - barrier_len;

	/* print summary */
	aprint_verbose("\nMemory summary\n");
	aprint_verbose("\tkmem_user_start\t%p\n", (void *) kmem_user_start);
	aprint_verbose("\tkmem_user_end\t%p\n",   (void *) kmem_user_end);
	aprint_verbose("\tkmem_k_start\t%p\n",    (void *) kmem_k_start);
	aprint_verbose("\tkmem_k_end\t%p\n",      (void *) kmem_k_end);
	aprint_verbose("\tkmem_kvm_start\t%p\n",  (void *) kmem_kvm_start);
	aprint_verbose("\tkmem_kvm_end\t%p\n",    (void *) kmem_kvm_end);

	aprint_verbose("\tDRAM_cfg\t%10d\n", (int) DRAM_cfg);
	aprint_verbose("\tkvmsize\t\t%10d\n", (int) KVMSIZE);
	aprint_verbose("\tuser_len\t%10d\n",
		(int) (kmem_user_end - kmem_user_start));

	aprint_verbose("\n\n");

	/* make critical assertions before modifying anything */
	if (sizeof(struct pcb) > USPACE) {
		panic("sizeof(struct pcb) is %d bytes too big for USPACE. "
		   "Please adjust TRAPSTACKSIZE calculation",
		   (int) (USPACE - sizeof(struct pcb)));
	}
	if (TRAPSTACKSIZE < 4*PAGE_SIZE) {
		panic("TRAPSTACKSIZE is too small, please increase UPAGES");
	}
	if (sizeof(struct pmap_l2) > PAGE_SIZE) {
		panic("struct pmap_l2 bigger than one page?\n");
	}

	/* protect user memory UVM area (---) */
	err = thunk_munmap((void *) kmem_user_start,
			kmem_k_start - kmem_user_start);
	if (err)
		panic("pmap_bootstrap: userland uvm space protection "
			"failed (%d)\n", thunk_geterrno());

#if 0
	/* protect kvm UVM area if separate (---) */
	err = thunk_munmap((void *) kmem_kvm_start,
			kmem_kvm_end - kmem_kvm_start);
	if (err)
		panic("pmap_bootstrap: kvm uvm space protection "
			"failed (%d)\n", thunk_geterrno());
#endif

	thunk_printf_debug("Creating memory mapped backend\n");

	/* create memory file since mmap/maccess only can be on files */
	strlcpy(mem_name, "/tmp/netbsd.XXXXXX", sizeof(mem_name));
	mem_fh = thunk_mkstemp(mem_name);
	if (mem_fh < 0)
		panic("pmap_bootstrap: can't create memory file\n");

	/* unlink the file so space is freed when we quit */
	if (thunk_unlink(mem_name) == -1)
		panic("pmap_bootstrap: can't unlink %s", mem_name);

	/* file_len is the backing store length, nothing to do with placement */
	file_len = DRAM_cfg;

#ifdef SPARSE_MEMFILE
	{
		char dummy;

		wlen = thunk_pwrite(mem_fh, &dummy, 1, file_len - 1);
		if (wlen != 1)
			panic("pmap_bootstrap: can't grow file\n");
	}
#else
	{
		char block[PAGE_SIZE];

		printf("Creating memory file\r");
		for (pg = 0; pg < file_len; pg += PAGE_SIZE) {
			wlen = thunk_pwrite(mem_fh, block, PAGE_SIZE, pg);
			if (wlen != PAGE_SIZE)
				panic("pmap_bootstrap: write fails, disc full?");
		}
	}
#endif

	/* protect the current kernel section */
	err = thunk_mprotect((void *) kmem_k_start, kmem_k_length,
		THUNK_PROT_READ | THUNK_PROT_EXEC);
	assert(err == 0);

	/* madvise the host kernel about our intentions with the memory */
	/* no measured effect, but might make a difference on high load */
	err = thunk_madvise((void *) kmem_user_start,
		kmem_k_start - kmem_user_start,
		THUNK_MADV_WILLNEED | THUNK_MADV_RANDOM);
	assert(err == 0);

	/* map the kernel at the start of the 'memory' file */
	kernel_fpos = 0;
	written = thunk_pwrite(mem_fh, (void *) kmem_k_start, kmem_k_length,
			kernel_fpos);
	assert(written == kmem_k_length);
	fpos = kernel_fpos + kmem_k_length;

	/* initialize counters */
	free_start = fpos;     /* in physical space ! */
	free_end   = file_len; /* in physical space ! */
	kmem_kvm_cur_start = kmem_kvm_start;

	/* calculate pv table size */
	phys_npages = file_len / PAGE_SIZE;
	pv_table_size = round_page(phys_npages * sizeof(struct pv_entry));
	thunk_printf_debug("claiming %"PRIu64" KB of pv_table for "
		"%"PRIdPTR" pages of physical memory\n",
		(uint64_t) pv_table_size/1024, (uintptr_t) phys_npages);

	/* calculate number of pmap entries needed for a complete map */
	pm_nentries = (kmem_k_end - VM_MIN_ADDRESS) / PAGE_SIZE;
	pm_entries_size = round_page(pm_nentries * sizeof(struct pv_entry *));
	thunk_printf_debug("tlb va->pa lookup table is %"PRIu64" KB for "
		"%d logical pages\n", pm_entries_size/1024, pm_nentries);

	/* calculate how big the l1 tables are going to be */
	pm_nl1 = pm_nentries / PMAP_L2_NENTRY;
	pm_l1_size = round_page(pm_nl1 * sizeof(struct pmap_l1 *));

	/* claim pv table */
	pv_fpos = fpos;
	pv_table = (struct pv_entry *) kmem_kvm_cur_start;
	addr = thunk_mmap(pv_table, pv_table_size,
		THUNK_PROT_READ | THUNK_PROT_WRITE,
		THUNK_MAP_FILE | THUNK_MAP_FIXED | THUNK_MAP_SHARED,
		mem_fh, pv_fpos);
	if (addr != (void *) pv_table)
		panic("pmap_bootstrap: can't map in pv table\n");

	memset(pv_table, 0, pv_table_size);	/* test and clear */

	thunk_printf_debug("pv_table initialiased correctly, mmap works\n");

	/* advance */
	kmem_kvm_cur_start += pv_table_size;
	fpos += pv_table_size;

	/* set up tlb space */
	tlb = (struct pv_entry **) kmem_kvm_cur_start;
	tlb_fpos = fpos;
	addr = thunk_mmap(tlb, pm_entries_size,
		THUNK_PROT_READ | THUNK_PROT_WRITE,
		THUNK_MAP_FILE | THUNK_MAP_FIXED | THUNK_MAP_SHARED,
		mem_fh, tlb_fpos);
	if (addr != (void *) tlb)
		panic("pmap_bootstrap: can't map in tlb entries\n");

	memset(tlb, 0, pm_entries_size);	/* test and clear */

	thunk_printf_debug("kernel tlb entries initialized correctly\n");

	/* advance */
	kmem_kvm_cur_start += pm_entries_size;
	fpos += pm_entries_size;

	/* set up kernel pmap and add a l1 map */
        pmap = pmap_kernel();
        memset(pmap, 0, sizeof(*pmap));
	pmap->pm_count = 1;		/* reference */
	pmap->pm_flags = PM_ACTIVE;	/* kernel pmap is allways active */
	pmap->pm_l1 = (struct pmap_l2 **) kmem_kvm_cur_start;

	pm_l1_fpos = fpos;
	addr = thunk_mmap(pmap->pm_l1, pm_l1_size,
		THUNK_PROT_READ | THUNK_PROT_WRITE,
		THUNK_MAP_FILE | THUNK_MAP_FIXED | THUNK_MAP_SHARED,
		mem_fh, pm_l1_fpos);
	if (addr != (void *) pmap->pm_l1)
		panic("pmap_bootstrap: can't map in pmap l1 entries\n");

	memset(pmap->pm_l1, 0, pm_l1_size);	/* test and clear */

	thunk_printf_debug("kernel pmap l1 table initialiased correctly\n");

	/* advance for l1 tables */
	kmem_kvm_cur_start += round_page(pm_l1_size);
	fpos += round_page(pm_l1_size);

	/* followed by the pm entries */
	pm_fpos = fpos;
	kernel_pm_entries = (struct pv_entry **) kmem_kvm_cur_start;
	addr = thunk_mmap(kernel_pm_entries, pm_entries_size,
		THUNK_PROT_READ | THUNK_PROT_WRITE,
		THUNK_MAP_FILE | THUNK_MAP_FIXED | THUNK_MAP_SHARED,
		mem_fh, pm_fpos);
	if (addr != (void *) kernel_pm_entries)
		panic("pmap_bootstrap: can't map in kernel pmap entries\n");

	memset(kernel_pm_entries, 0, pm_entries_size);	/* test and clear */

	/* advance for the statically allocated pm_entries */
	kmem_kvm_cur_start += pm_entries_size;
	fpos += pm_entries_size;

	/* put pointers in the l1 to point to the pv_entry space */
	for (l1 = 0; l1 < pm_nl1; l1++) {
		pmap = pmap_kernel();
		pmap->pm_l1[l1] = (struct pmap_l2 *)
			((vaddr_t) kernel_pm_entries + l1 * PMAP_L2_SIZE);
	}

	/* kmem used [kmem_kvm_start - kmem_kvm_cur_start] */
	kmem_kvm_cur_end = kmem_kvm_cur_start;

	/* manually enter the mappings into the kernel map */
	for (pg = 0; pg < pv_table_size; pg += PAGE_SIZE) {
		pa = pv_fpos + pg;
		va = (vaddr_t) pv_table + pg;
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE, 0);
	}
	thunk_printf_debug("pv_table mem added to the kernel pmap\n");
	for (pg = 0; pg < pm_entries_size; pg += PAGE_SIZE) {
		pa = tlb_fpos + pg;
		va = (vaddr_t) tlb + pg;
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE, 0);
	}
	thunk_printf_debug("kernel tlb entries mem added to the kernel pmap\n");
	for (pg = 0; pg < pm_l1_size; pg += PAGE_SIZE) {
		pa = pm_l1_fpos + pg;
		va = (vaddr_t) pmap->pm_l1 + pg;
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE, 0);
	}
	thunk_printf_debug("kernel pmap l1 mem added to the kernel pmap\n");
	for (pg = 0; pg < pm_entries_size; pg += PAGE_SIZE) {
		pa = pm_fpos + pg;
		va = (vaddr_t) kernel_pm_entries + pg;
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE, 0);
	}
	thunk_printf_debug("kernel pmap entries mem added to the kernel pmap\n");
#if 0
	/* not yet, or not needed */
	for (pg = 0; pg < kmem_k_length; pg += PAGE_SIZE) {
		pa = kernel_fpos + pg;
		va = (vaddr_t) kmem_k_start + pg;
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE, 0);
	}
	thunk_printf_debug("kernel mem added to the kernel pmap\n");
#endif

	/* add file space to uvm's FREELIST */
	uvm_page_physload(atop(0),
	    atop(free_end),
	    atop(free_start + fpos), /* mark used till fpos */
	    atop(free_end), 
	    VM_FREELIST_DEFAULT);

	/* setup syscall emulation */
	if (thunk_syscallemu_init((void *)VM_MIN_ADDRESS,
	    (void *)VM_MAXUSER_ADDRESS) != 0)
		panic("couldn't enable syscall emulation");

	aprint_verbose("leaving pmap_bootstrap:\n");
	aprint_verbose("\t%"PRIu64" MB of physical pages left\n",
		(uint64_t) (free_end - (free_start + fpos))/1024/1024);
	aprint_verbose("\t%"PRIu64" MB of kmem left\n",
		(uint64_t) (kmem_kvm_end - kmem_kvm_cur_end)/1024/1024);

	setup_signal_handlers();
}

void
pmap_init(void)
{
}

/* return kernel space start and end (including growth) */
void
pmap_virtual_space(vaddr_t *vstartp, vaddr_t *vendp)
{
	if (vstartp)
		*vstartp = kmem_kvm_cur_start;		/* min to map in */
	if (vendp)
		*vendp   = kmem_kvm_end - PAGE_SIZE;	/* max available */
}

static void
pmap_deferred_init(void)
{
	/* XXX we COULD realloc our pv_table etc with malloc() but for what? */

	/* create pmap pool */
	pool_init(&pmap_pool, sizeof(struct pmap), 0, 0, 0,
	    "pmappool", NULL, IPL_NONE);
	pool_init(&pmap_pventry_pool, sizeof(struct pv_entry), 0, 0, 0,
	    "pventry", NULL, IPL_HIGH);
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

	thunk_printf_debug("pmap_create\n");
	num_pmaps++;
#if 0
	printf("%s: pre alloc: num_pmaps %"PRIu64" (%"PRIu64" kb), "
		   "num_pv_entries %"PRIu64" (%"PRIu64" kb)\n",
		__func__,
		(uint64_t) num_pmaps,
		(uint64_t) num_pmaps * (sizeof(*pmap) + pm_l1_size)   / 1024,
		(uint64_t) num_pv_entries,
		(uint64_t) num_pv_entries * (sizeof(struct pv_entry)) / 1024);
#endif

	pmap = pool_get(&pmap_pool, PR_WAITOK);
	memset(pmap, 0, sizeof(*pmap));
	pmap->pm_count = 1;
	pmap->pm_flags = 0;

	/* claim l1 table */
	pmap->pm_l1 = kmem_zalloc(pm_l1_size, KM_SLEEP);
	assert(pmap->pm_l1);

	thunk_printf_debug("\tpmap %p\n", pmap);

	return pmap;
}

void
pmap_destroy(pmap_t pmap)
{
	struct pmap_l2 *l2tbl;
	int l1;

	/* if multiple references exist just remove a reference */
	thunk_printf_debug("pmap_destroy %p\n", pmap);
	if (--pmap->pm_count > 0)
		return;
	num_pmaps--;

	/* safe guard against silly errors */
	KASSERT((pmap->pm_flags & PM_ACTIVE) == 0);
	KASSERT(pmap->pm_stats.resident_count == 0);
	KASSERT(pmap->pm_stats.wired_count == 0);
#ifdef DIAGNOSTIC
	for (l1 = 0; l1 < pm_nl1; l1++) {
		int l2;

		l2tbl = pmap->pm_l1[l1];
		if (!l2tbl)
			continue;
		for (l2 = 0; l2 < PMAP_L2_NENTRY; l2++) {
			if (l2tbl->pm_l2[l2])
				panic("pmap_destroy: pmap isn't empty");
		}
	}
#endif
	for (l1 = 0; l1 < pm_nl1; l1++) {
		l2tbl = pmap->pm_l1[l1];
		if (!l2tbl)
			continue;
		kmem_free(l2tbl, PMAP_L2_SIZE);
	}
	kmem_free(pmap->pm_l1, pm_l1_size);
	pool_put(&pmap_pool, pmap);
}

void
pmap_reference(pmap_t pmap)
{
	thunk_printf_debug("pmap_reference %p\n", (void *) pmap);
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
	struct pv_entry *pv;

	num_pv_entries++;
	pv = pool_get(&pmap_pventry_pool, PR_WAITOK);
	memset(pv, 0, sizeof(struct pv_entry));

	return pv;
}

static void
pv_free(struct pv_entry *pv)
{
	num_pv_entries--;
	pool_put(&pmap_pventry_pool, pv);
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
		if ((pv->pv_pmap == pmap) && (pv->pv_lpn == lpn)) {
			return pv;
		}
	}
	/* Otherwise, allocate a new entry and link it in after the head. */
	thunk_printf_debug("pv_get: multiple mapped page ppn %"PRIdPTR", "
		"lpn %"PRIdPTR"\n", ppn, lpn);

	/* extra sanity */
	assert(ppn < phys_npages);

	pv = pv_alloc();
	if (pv == NULL)
		return NULL;

	pv->pv_next = pv_table[ppn].pv_next;
	pv_table[ppn].pv_next = pv;
	pmap->pm_stats.resident_count++;

	return pv;
}

static void
pmap_set_pv(pmap_t pmap, uintptr_t lpn, struct pv_entry *pv)
{
	struct pmap_l2 *l2tbl;
	int l1 = lpn / PMAP_L2_NENTRY;
	int l2 = lpn % PMAP_L2_NENTRY;

#ifdef DIAGNOSTIC
	if (lpn >= pm_nentries)
		panic("peeing outside box : addr in page around %"PRIx64"\n",
			(uint64_t) lpn*PAGE_SIZE);
#endif

	l2tbl = pmap->pm_l1[l1];
	if (!l2tbl) {
		l2tbl = pmap->pm_l1[l1] = kmem_zalloc(PMAP_L2_SIZE, KM_SLEEP);
		/* should be zero filled */
	}
	l2tbl->pm_l2[l2] = pv;
}

static struct pv_entry *
pmap_lookup_pv(pmap_t pmap, uintptr_t lpn)
{
	struct pmap_l2 *l2tbl;
	int l1 = lpn / PMAP_L2_NENTRY;
	int l2 = lpn % PMAP_L2_NENTRY;

	if (lpn >= pm_nentries)
		return NULL;

	l2tbl = pmap->pm_l1[l1];
	if (l2tbl)
		return l2tbl->pm_l2[l2];
	return NULL;
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

	thunk_printf_debug("pmap_fault pmap %p, va %p\n", pmap, (void *) va);

	/* get logical page from vaddr */
	lpn = atop(va - VM_MIN_ADDRESS);	/* V->L */
	pv = pmap_lookup_pv(pmap, lpn);

	/* not known! then it must be UVM's work */
	if (pv == NULL) {
		//thunk_printf("%s: no mapping yet for %p\n",
		//	__func__, (void *) va);
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

	/* check the TLB, if NULL we have a TLB fault */
	if (tlb[pv->pv_lpn] == NULL) {
		if (pv->pv_mmap_ppl != THUNK_PROT_NONE) {
			thunk_printf_debug("%s: tlb fault page lpn %"PRIiPTR"\n",
				__func__, pv->pv_lpn);
			pmap_page_activate(pv);
			return true;
		}
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

	thunk_printf_debug("%s: prot = %d, cur_prot = %d, diff = %d\n",
		__func__, prot, cur_prot, diff);
	*atype = VM_PROT_READ;  /* assume its a read error */
	if (diff & VM_PROT_READ) {
		if ((ppv->pv_pflags & PV_REFERENCED) == 0) {
			ppv->pv_pflags |= PV_REFERENCED;
			pmap_update_page(ppn);
			return true;
		}
		panic("pmap: page not readable but marked referenced?");
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
			/* should be allowed to write */
			if ((ppv->pv_pflags & PV_MODIFIED) == 0) {
				/* was marked unmodified */
				ppv->pv_pflags |= PV_MODIFIED;
				pmap_update_page(ppn);
				return true;
			}
		}
		panic("pmap: page not writable but marked modified?");
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
	uint32_t map_flags;
	void *addr;

	map_flags = THUNK_MAP_FILE | THUNK_MAP_FIXED | THUNK_MAP_SHARED;

	addr = thunk_mmap((void *) va, PAGE_SIZE, pv->pv_mmap_ppl,
		map_flags, mem_fh, pa);
	thunk_printf_debug("page_activate: (va %p, pa %p, prot %d, ppl %d) -> %p\n",
		(void *) va, (void *) pa, pv->pv_prot, pv->pv_mmap_ppl,
		(void *) addr);
	if (addr != (void *) va)
		panic("pmap_page_activate: mmap failed (expected %p got %p): %d",
		    (void *)va, addr, thunk_geterrno());

	tlb[pv->pv_lpn] = NULL;
	if (pv->pv_mmap_ppl != THUNK_PROT_NONE)
		tlb[pv->pv_lpn] = pv;
}

static void
pmap_page_deactivate(struct pv_entry *pv)
{
	paddr_t pa = pv->pv_ppn * PAGE_SIZE;
	vaddr_t va = pv->pv_lpn * PAGE_SIZE + VM_MIN_ADDRESS; /* L->V */
	uint32_t map_flags;
	void *addr;

	/* don't try to unmap pv entries that are already unmapped */
	if (!tlb[pv->pv_lpn])
		return;

	if (tlb[pv->pv_lpn]->pv_mmap_ppl == THUNK_PROT_NONE)
		goto deactivate;

	map_flags = THUNK_MAP_FILE | THUNK_MAP_FIXED | THUNK_MAP_SHARED;
	addr = thunk_mmap((void *) va, PAGE_SIZE, THUNK_PROT_NONE,
		map_flags, mem_fh, pa);
	thunk_printf_debug("page_deactivate: (va %p, pa %p, ppl %d) -> %p\n",
		(void *) va, (void *) pa, pv->pv_mmap_ppl, (void *) addr);
	if (addr != (void *) va)
		panic("pmap_page_deactivate: mmap failed");

deactivate:
	tlb[pv->pv_lpn] = NULL;
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
	    (pflags & PV_REFERENCED) && (pflags & PV_MODIFIED)) {
		mmap_ppl = THUNK_PROT_READ | THUNK_PROT_WRITE;
	} else if ((pv->pv_prot & (VM_PROT_READ | VM_PROT_EXECUTE)) &&
		 (pflags & PV_REFERENCED)) {
		mmap_ppl = THUNK_PROT_READ;
		if (pv->pv_prot & VM_PROT_EXECUTE)
			mmap_ppl |= THUNK_PROT_EXEC;
	} else {
		mmap_ppl = THUNK_PROT_NONE;
	}

	/* unmanaged or wired pages are special; they dont track r/m */
	if (vflags & (PV_UNMANAGED | PV_WIRED))
		mmap_ppl = THUNK_PROT_READ | THUNK_PROT_WRITE;

	pv->pv_mmap_ppl = mmap_ppl;
}

/* update mapping of a physical page */
static void
pmap_update_page(uintptr_t ppn)
{
	struct pv_entry *pv;

	for (pv = &pv_table[ppn]; pv != NULL; pv = pv->pv_next) {
		thunk_printf_debug("pmap_update_page: ppn %"PRIdPTR", pv->pv_map = %p\n",
			ppn, pv->pv_pmap);
		if (pv->pv_pmap != NULL) {
			pv_update(pv);
			if (pv->pv_pmap->pm_flags & PM_ACTIVE)
				pmap_page_activate(pv);
			else
				pmap_page_deactivate(pv)
			;
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
	if ((va < VM_MIN_ADDRESS) || (va > VM_MAX_KERNEL_ADDRESS))
		panic("pmap_do_enter: invalid va isued\n");
#endif

	/* raise interupt level */
	s = splvm();

	/* remove existing mapping at this lpn */
	pv = pmap_lookup_pv(pmap, lpn);
	if (pv && pv->pv_ppn != ppn)
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
	/* pv->pv_next   = NULL; */	/* might confuse linked list? */
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
	pmap_set_pv(pmap, lpn, pv);

	/* adjust stats */
	if (pv->pv_vflags & PV_WIRED)
		pmap->pm_stats.wired_count++;

	splx(s);

	/* activate page directly when on active pmap */
	if (pmap->pm_flags & PM_ACTIVE)
		pmap_page_activate(pv);

	return 0;
}

int
pmap_enter(pmap_t pmap, vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	thunk_printf_debug("pmap_enter %p : v %p, p %p, prot %d, flags %d\n",
		(void *) pmap, (void *) va, (void *) pa, (int) prot, (int) flags);
	return pmap_do_enter(pmap, va, pa, prot, flags, 0);
}

/* release the pv_entry for a mapping.  Code derived also from hp300 pmap */
static void
pv_release(pmap_t pmap, uintptr_t ppn, uintptr_t lpn)
{
	struct pv_entry *pv, *npv;

	thunk_printf_debug("pv_release ppn %"PRIdPTR", lpn %"PRIdPTR"\n", ppn, lpn);
	pv = &pv_table[ppn];
	/*
	 * If it is the first entry on the list, it is actually
	 * in the header and we must copy the following entry up
	 * to the header.  Otherwise we must search the list for
	 * the entry.  In either case we free the now unused entry.
	 */
	if ((pmap == pv->pv_pmap) && (lpn == pv->pv_lpn)) {
		npv = pv->pv_next;
		if (npv) {
			/* pull up first entry from chain. */
			memcpy(pv, npv, offsetof(struct pv_entry, pv_pflags));
			pmap_set_pv(pv->pv_pmap, pv->pv_lpn, pv);
			pv_free(npv);
		} else {
			memset(pv, 0, offsetof(struct pv_entry, pv_pflags));
		}
	} else {
		for (npv = pv->pv_next; npv; npv = npv->pv_next) {
			if ((pmap == npv->pv_pmap) && (lpn == npv->pv_lpn))
				break;
			pv = npv;
		}
		KASSERT(npv != NULL);
		pv->pv_next = npv->pv_next;
		pv_free(npv);
	}
	pmap_set_pv(pmap, lpn, NULL);
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

	thunk_printf_debug("pmap_remove() called from "
		"lpn %"PRIdPTR" to lpn %"PRIdPTR"\n", slpn, elpn);

	s = splvm();
	for (lpn = slpn; lpn < elpn; lpn++) {
		pv = pmap_lookup_pv(pmap, lpn);
		if (pv != NULL) {
			if (pmap->pm_flags & PM_ACTIVE) {
				pmap_page_deactivate(pv);
//				MEMC_WRITE(pv->pv_deactivate);
//				cpu_cache_flush();
			}
			pmap_set_pv(pmap, lpn, NULL);
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
	/* just a hint that all the entries are to be removed */
	thunk_printf_debug("pmap_remove_all() dummy called\n");

	/* we dont do anything with the kernel pmap */
	if (pmap == pmap_kernel())
		return;

#if 0
	/* remove all mappings in one-go; not needed */
	pmap_remove(pmap, VM_MIN_ADDRESS, VM_MAXUSER_ADDRESS);
	thunk_munmap((void *) VM_MIN_ADDRESS, VM_MAXUSER_ADDRESS - VM_MIN_ADDRESS);
#endif
#if 0
	/* remove all cached info from the pages */
	thunk_msync(VM_MIN_ADDRESS, VM_MAXUSER_ADDRESS - VM_MIN_ADDRESS,
		THUNK_MS_SYNC | THUNK_MS_INVALIDATE);
#endif
}

void
pmap_protect(pmap_t pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
	struct pv_entry *pv;
	intptr_t slpn, elpn, lpn;
	int s;

	if (prot == VM_PROT_NONE) {
		pmap_remove(pmap, sva, eva);
		return;
	}
	if (prot & VM_PROT_WRITE)
		return; /* apparently we're meant to */
	if (pmap == pmap_kernel())
		return; /* can't restrict kernel w/o unmapping. */

	slpn = atop(sva - VM_MIN_ADDRESS);	/* V->L */
 	elpn = atop(eva - VM_MIN_ADDRESS);	/* V->L */

	thunk_printf_debug("pmap_protect() called from "
		"lpn %"PRIdPTR" to lpn %"PRIdPTR"\n", slpn, elpn);

	s = splvm();
	for (lpn = slpn; lpn < elpn; lpn++) {
		pv = pmap_lookup_pv(pmap, lpn);
		if (pv != NULL) {
			pv->pv_prot &= prot;
			pv_update(pv);
			if (pv->pv_pmap->pm_flags & PM_ACTIVE)
				pmap_page_activate(pv);
		}
	}
	splx(s);
}

void
pmap_unwire(pmap_t pmap, vaddr_t va)
{
	struct pv_entry *pv;
	intptr_t lpn;

	thunk_printf_debug("pmap_unwire called va = %p\n", (void *) va);
	if (pmap == NULL)
		return;

	lpn = atop(va - VM_MIN_ADDRESS);	/* V->L */
	pv = pmap_lookup_pv(pmap, lpn);
	if (pv == NULL)
		return;
	/* but is it wired? */
	if ((pv->pv_vflags & PV_WIRED) == 0)
		return;
	pmap->pm_stats.wired_count--;
	pv->pv_vflags &= ~PV_WIRED;

	/* XXX needed? */
	pmap_update_page(pv->pv_ppn);
}

bool
pmap_extract(pmap_t pmap, vaddr_t va, paddr_t *ppa)
{
	struct pv_entry *pv;
	intptr_t lpn;

	thunk_printf_debug("pmap_extract: extracting va %p\n", (void *) va);
#ifdef DIAGNOSTIC
	if ((va < VM_MIN_ADDRESS) || (va > VM_MAX_KERNEL_ADDRESS)) {
		thunk_printf_debug("pmap_extract: invalid va isued\n");
		thunk_printf("%p not in [%p, %p]\n", (void *) va,
		    (void *) VM_MIN_ADDRESS, (void *) VM_MAX_KERNEL_ADDRESS);
		return false;
	}
#endif
	lpn = atop(va - VM_MIN_ADDRESS);	/* V->L */
	pv = pmap_lookup_pv(pmap, lpn);

	if (pv == NULL)
		return false;
	if (ppa)
		*ppa = ptoa(pv->pv_ppn);
	return true;
}

/*
 * Enter an unmanaged, `wired' kernel mapping.
 * Only to be removed by pmap_kremove()
 */
void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	thunk_printf_debug("pmap_kenter_pa : v %p, p %p, prot %d, flags %d\n",
		(void *) va, (void *) pa, (int) prot, (int) flags);
	pmap_do_enter(pmap_kernel(), va, pa, prot, prot | PMAP_WIRED, 1);
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
	thunk_printf_debug("pmap_copy (dummy)\n");
}

void
pmap_update(pmap_t pmap)
{
	thunk_printf_debug("pmap_update (dummy)\n");
}

void
pmap_activate(struct lwp *l)
{
	struct proc *p = l->l_proc;
	pmap_t pmap;

	pmap = p->p_vmspace->vm_map.pmap;
	thunk_printf_debug("pmap_activate for lwp %p, pmap = %p\n", l, pmap);

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
	struct pv_entry *pv;
	struct pmap_l2 *l2tbl;
	pmap_t pmap;
	int l1, l2;

	pmap = p->p_vmspace->vm_map.pmap;
	thunk_printf_debug("pmap_DEactivate for lwp %p, pmap = %p\n", l, pmap);

	if (pmap == pmap_kernel())
		return; /* kernel pmap is always active */

	KASSERT(pmap == active_pmap);
	KASSERT(pmap->pm_flags & PM_ACTIVE);

	active_pmap = NULL;
	pmap->pm_flags &=~ PM_ACTIVE;

	for (l1 = 0; l1 < pm_nl1; l1++) {
		l2tbl = pmap->pm_l1[l1];
		if (!l2tbl)
			continue;
		for (l2 = 0; l2 < PMAP_L2_NENTRY; l2++) {
			pv = l2tbl->pm_l2[l2];
			if (pv) {
				pmap_page_deactivate(pv);
	//			MEMC_WRITE(pmap->pm_entries[i]->pv_deactivate);
			}
		}
	}

	/* dummy */
//	cpu_cache_flush();
}

void
pmap_zero_page(paddr_t pa)
{
	char *blob;

	thunk_printf_debug("pmap_zero_page: pa %p\n", (void *) pa);

	if (pa & (PAGE_SIZE-1))
		panic("%s: unaligned address passed : %p\n", __func__, (void *) pa);

	blob = thunk_mmap(NULL, PAGE_SIZE,
		THUNK_PROT_READ | THUNK_PROT_WRITE,
		THUNK_MAP_FILE | THUNK_MAP_SHARED,
		mem_fh, pa);
	if (!blob)
		panic("%s: couldn't get mapping", __func__);
	if (blob < (char *) kmem_k_end)
		panic("%s: mmap in illegal memory range", __func__);

	memset(blob, 0, PAGE_SIZE);

	thunk_munmap(blob, PAGE_SIZE);
}

void
pmap_copy_page(paddr_t src_pa, paddr_t dst_pa)
{
	char *sblob, *dblob;

	if (src_pa & (PAGE_SIZE-1))
		panic("%s: unaligned address passed : %p\n", __func__, (void *) src_pa);
	if (dst_pa & (PAGE_SIZE-1))
		panic("%s: unaligned address passed : %p\n", __func__, (void *) dst_pa);

	thunk_printf_debug("pmap_copy_page: pa src %p, pa dst %p\n",
		(void *) src_pa, (void *) dst_pa);

	/* XXX bug alart: can we allow the kernel to make a decision on this? */
	sblob = thunk_mmap(NULL, PAGE_SIZE,
		THUNK_PROT_READ,
		THUNK_MAP_FILE | THUNK_MAP_SHARED,
		mem_fh, src_pa);
	if (!sblob)
		panic("%s: couldn't get src mapping", __func__);
	if (sblob < (char *) kmem_k_end)
		panic("%s: mmap in illegal memory range", __func__);

	/* XXX bug alart: can we allow the kernel to make a decision on this? */
	dblob = thunk_mmap(NULL, PAGE_SIZE,
		THUNK_PROT_READ | THUNK_PROT_WRITE,
		THUNK_MAP_FILE | THUNK_MAP_SHARED,
		mem_fh, dst_pa);
	if (!dblob)
		panic("%s: couldn't get dst mapping", __func__);
	if (dblob < (char *) kmem_k_end)
		panic("%s: mmap in illegal memory range", __func__);

	memcpy(dblob, sblob, PAGE_SIZE);

	thunk_munmap(sblob, PAGE_SIZE);
	thunk_munmap(dblob, PAGE_SIZE);
}

/* change access permissions on a given physical page */
void
pmap_page_protect(struct vm_page *page, vm_prot_t prot)
{
	intptr_t ppn;
	struct pv_entry *pv, *npv;

	ppn = atop(VM_PAGE_TO_PHYS(page));
	thunk_printf_debug("pmap_page_protect page %"PRIiPTR" to prot %d\n", ppn, prot);

	if (prot == VM_PROT_NONE) {
		/* visit all mappings */
		npv = pv = &pv_table[ppn];
		while ((pv != NULL) && (pv->pv_pmap != NULL)) {
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
			pmap_set_pv(pv->pv_pmap, pv->pv_lpn, NULL);
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

	thunk_printf_debug("pmap_clear_modify page %"PRIiPTR"\n", ppn);

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

	thunk_printf_debug("pmap_clear_reference page %"PRIiPTR"\n", ppn);

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

	thunk_printf_debug("pmap_is_modified page %"PRIiPTR" : %s\n", ppn, rv?"yes":"no");

	return rv;
}

bool
pmap_is_referenced(struct vm_page *page)
{
	intptr_t ppn;

	ppn = atop(VM_PAGE_TO_PHYS(page));
	thunk_printf_debug("pmap_is_referenced page %"PRIiPTR"\n", ppn);

	return (pv_table[ppn].pv_pflags & PV_REFERENCED) != 0;
}

paddr_t
pmap_phys_address(paddr_t cookie)
{
	return ptoa(cookie);
}

vaddr_t
pmap_growkernel(vaddr_t maxkvaddr)
{
	thunk_printf_debug("pmap_growkernel: till %p (adding %"PRIu64" KB)\n",
		(void *) maxkvaddr,
		(uint64_t) (maxkvaddr - kmem_kvm_cur_end)/1024);
	if (maxkvaddr > kmem_kvm_end)
		return kmem_kvm_end;
	kmem_kvm_cur_end = maxkvaddr;
	return kmem_kvm_cur_end;
}

