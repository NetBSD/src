/*	$NetBSD: pmap.c,v 1.190 2007/05/23 09:36:22 martin Exp $	*/
/*
 *
 * Copyright (C) 1996-1999 Eduardo Horvath.
 * All rights reserved.
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR  ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR  BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.190 2007/05/23 09:36:22 martin Exp $");

#undef	NO_VCACHE /* Don't forget the locked TLB in dostart */
#define	HWREF

#include "opt_ddb.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/systm.h>
#include <sys/msgbuf.h>
#include <sys/lock.h>
#include <sys/pool.h>
#include <sys/exec.h>
#include <sys/core.h>
#include <sys/kcore.h>
#include <sys/proc.h>

#include <uvm/uvm.h>

#include <machine/pcb.h>
#include <machine/sparc64.h>
#include <machine/ctlreg.h>
#include <machine/promlib.h>
#include <machine/kcore.h>
#include <machine/cpu.h>
#include <machine/bootinfo.h>

#include "cache.h"

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_command.h>
#include <ddb/db_sym.h>
#include <ddb/db_variables.h>
#include <ddb/db_extern.h>
#include <ddb/db_access.h>
#include <ddb/db_output.h>
#else
#define Debugger()
#define db_printf	printf
#endif

#define	MEG		(1<<20) /* 1MB */
#define	KB		(1<<10)	/* 1KB */

paddr_t cpu0paddr;		/* contigious phys memory preallocated for cpus */

/* These routines are in assembly to allow access thru physical mappings */
extern int64_t pseg_get(struct pmap *, vaddr_t);
extern int pseg_set(struct pmap *, vaddr_t, int64_t, paddr_t);

/*
 * Diatribe on ref/mod counting:
 *
 * First of all, ref/mod info must be non-volatile.  Hence we need to keep it
 * in the pv_entry structure for each page.  (We could bypass this for the
 * vm_page, but that's a long story....)
 *
 * This architecture has nice, fast traps with lots of space for software bits
 * in the TTE.  To accelerate ref/mod counts we make use of these features.
 *
 * When we map a page initially, we place a TTE in the page table.  It's
 * inserted with the TLB_W and TLB_ACCESS bits cleared.  If a page is really
 * writable we set the TLB_REAL_W bit for the trap handler.
 *
 * Whenever we take a TLB miss trap, the trap handler will set the TLB_ACCESS
 * bit in the approprate TTE in the page table.  Whenever we take a protection
 * fault, if the TLB_REAL_W bit is set then we flip both the TLB_W and TLB_MOD
 * bits to enable writing and mark the page as modified.
 *
 * This means that we may have ref/mod information all over the place.  The
 * pmap routines must traverse the page tables of all pmaps with a given page
 * and collect/clear all the ref/mod information and copy it into the pv_entry.
 */

#ifdef	NO_VCACHE
#define	FORCE_ALIAS	1
#else
#define FORCE_ALIAS	0
#endif

#define	PV_ALIAS	0x1LL
#define PV_REF		0x2LL
#define PV_MOD		0x4LL
#define PV_NVC		0x8LL
#define PV_NC		0x10LL
#define PV_WE		0x20LL	/* Debug -- this page was writable somtime */
#define PV_MASK		(0x03fLL)
#define PV_VAMASK	(~(PAGE_SIZE - 1))
#define PV_MATCH(pv,va)	(!(((pv)->pv_va ^ (va)) & PV_VAMASK))
#define PV_SETVA(pv,va) ((pv)->pv_va = (((va) & PV_VAMASK) | \
					(((pv)->pv_va) & PV_MASK)))

struct pool pmap_pmap_pool;
struct pool pmap_pv_pool;

pv_entry_t	pmap_remove_pv(struct pmap *, vaddr_t, struct vm_page *);
void	pmap_enter_pv(struct pmap *, vaddr_t, paddr_t, struct vm_page *,
			   pv_entry_t);
void	pmap_page_cache(struct pmap *, paddr_t, int);

/*
 * First and last managed physical addresses.
 * XXX only used for dumping the system.
 */
paddr_t	vm_first_phys, vm_num_phys;

/*
 * Here's the CPU TSB stuff.  It's allocated in pmap_bootstrap.
 */
pte_t *tsb_dmmu;
pte_t *tsb_immu;
int tsbsize;		/* tsbents = 512 * 2^^tsbsize */
#define TSBENTS (512<<tsbsize)
#define	TSBSIZE	(TSBENTS * 16)

struct pmap kernel_pmap_;

int physmem;
/*
 * Virtual and physical addresses of the start and end of kernel text
 * and data segments.
 */
vaddr_t ktext;
paddr_t ktextp;
vaddr_t ektext;
paddr_t ektextp;
vaddr_t kdata;
paddr_t kdatap;
vaddr_t ekdata;
paddr_t ekdatap;

/*
 * Kernel 4MB pages.
 */
extern struct tlb_entry *kernel_tlbs;
extern int kernel_tlb_slots;

static int npgs;
static u_int nextavail;

vaddr_t	vmmap;			/* one reserved MI vpage for /dev/mem */

int phys_installed_size;		/* Installed physical memory */
struct mem_region *phys_installed;

paddr_t avail_start, avail_end;	/* These are used by ps & family */

static int ptelookup_va(vaddr_t va);

static inline void
clrx(void *addr)
{
	__asm volatile("clrx [%0]" : : "r" (addr) : "memory");
}

static inline void
tsb_invalidate(int ctx, vaddr_t va)
{
	int i;
	int64_t tag;

	i = ptelookup_va(va);
	tag = TSB_TAG(0, ctx, va);
	if (tsb_dmmu[i].tag == tag) {
		clrx(&tsb_dmmu[i].data);
	}
	if (tsb_immu[i].tag == tag) {
		clrx(&tsb_immu[i].data);
	}
}

struct prom_map *prom_map;
int prom_map_size;

#ifdef DEBUG
struct {
	int kernel;	/* entering kernel mapping */
	int user;	/* entering user mapping */
	int ptpneeded;	/* needed to allocate a PT page */
	int pwchange;	/* no mapping change, just wiring or protection */
	int wchange;	/* no mapping change, just wiring */
	int mchange;	/* was mapped but mapping to different page */
	int managed;	/* a managed page */
	int firstpv;	/* first mapping for this PA */
	int secondpv;	/* second mapping for this PA */
	int ci;		/* cache inhibited */
	int unmanaged;	/* not a managed page */
	int flushes;	/* cache flushes */
	int cachehit;	/* new entry forced valid entry out */
} enter_stats;
struct {
	int calls;
	int removes;
	int flushes;
	int tflushes;	/* TLB flushes */
	int pidflushes;	/* HW pid stolen */
	int pvfirst;
	int pvsearch;
} remove_stats;
#define	ENTER_STAT(x)	enter_stats.x ++
#define	REMOVE_STAT(x)	remove_stats.x ++

#define	PDB_CREATE	0x0001
#define	PDB_DESTROY	0x0002
#define	PDB_REMOVE	0x0004
#define	PDB_CHANGEPROT	0x0008
#define	PDB_ENTER	0x0010
#define PDB_DEMAP	0x0020
#define	PDB_REF		0x0040
#define PDB_COPY	0x0080

#define	PDB_MMU_ALLOC	0x0100
#define	PDB_MMU_STEAL	0x0200
#define	PDB_CTX_ALLOC	0x0400
#define	PDB_CTX_STEAL	0x0800
#define	PDB_MMUREG_ALLOC	0x1000
#define	PDB_MMUREG_STEAL	0x2000
#define	PDB_CACHESTUFF	0x4000
#define	PDB_ALIAS	0x8000
#define PDB_EXTRACT	0x10000
#define	PDB_BOOT	0x20000
#define	PDB_BOOT1	0x40000
#define	PDB_GROW	0x80000
int	pmapdebug = 0;
/* Number of H/W pages stolen for page tables */
int	pmap_pages_stolen = 0;

#define	BDPRINTF(n, f)	if (pmapdebug & (n)) prom_printf f
#define	DPRINTF(n, f)	if (pmapdebug & (n)) printf f
#else
#define	ENTER_STAT(x)
#define	REMOVE_STAT(x)
#define	BDPRINTF(n, f)
#define	DPRINTF(n, f)
#endif

#define pv_check()

/*
 *
 * A context is simply a small number that differentiates multiple mappings
 * of the same address.  Contexts on the spitfire are 13 bits, but could
 * be as large as 17 bits.
 *
 * Each context is either free or attached to a pmap.
 *
 * The context table is an array of pointers to psegs.  Just dereference
 * the right pointer and you get to the pmap segment tables.  These are
 * physical addresses, of course.
 *
 */

int pmap_next_ctx = 1;
paddr_t *ctxbusy;
LIST_HEAD(, pmap) pmap_ctxlist;
int numctx;
#define CTXENTRY	(sizeof(paddr_t))
#define CTXSIZE		(numctx*CTXENTRY)

static int pmap_get_page(paddr_t *p);
static void pmap_free_page(paddr_t pa);


/*
 * Support for big page sizes.  This maps the page size to the
 * page bits.  That is: these are the bits between 8K pages and
 * larger page sizes that cause aliasing.
 */
#define PSMAP_ENTRY(MASK, CODE)	{ .mask = MASK, .code = CODE }
struct page_size_map page_size_map[] = {
#ifdef DEBUG
	PSMAP_ENTRY(0, PGSZ_8K & 0),	/* Disable large pages */
#endif
	PSMAP_ENTRY((4 * 1024 * 1024 - 1) & ~(8 * 1024 - 1), PGSZ_4M),
	PSMAP_ENTRY((512 * 1024 - 1) & ~(8 * 1024 - 1), PGSZ_512K),
	PSMAP_ENTRY((64 * 1024 - 1) & ~(8 * 1024 - 1), PGSZ_64K),
	PSMAP_ENTRY((8 * 1024 - 1) & ~(8 * 1024 - 1), PGSZ_8K),
	PSMAP_ENTRY(0, 0),
};

/*
 * Enter a TTE into the kernel pmap only.  Don't do anything else.
 *
 * Use only during bootstrapping since it does no locking and
 * can lose ref/mod info!!!!
 *
 */
static void pmap_enter_kpage(vaddr_t va, int64_t data)
{
	paddr_t newp;

	newp = 0UL;
	while (pseg_set(pmap_kernel(), va, data, newp) & 1) {
		if (!pmap_get_page(&newp)) {
			prom_printf("pmap_enter_kpage: out of pages\n");
			panic("pmap_enter_kpage");
		}

		ENTER_STAT(ptpneeded);
		BDPRINTF(PDB_BOOT1,
			 ("pseg_set: pm=%p va=%p data=%lx newp %lx\n",
			  pmap_kernel(), va, (long)data, (long)newp));
#ifdef DEBUG
		if (pmapdebug & PDB_BOOT1)
		{int i; for (i=0; i<140000000; i++) ;}
#endif
	}
}

/*
 * Check the bootargs to see if we need to enable bootdebug.
 */
#ifdef DEBUG
static void pmap_bootdebug(void)
{
	const char *cp = prom_getbootargs();

	for (;;)
		switch (*++cp) {
		case '\0':
			return;
		case 'V':
			pmapdebug |= PDB_BOOT|PDB_BOOT1;
			break;
		case 'D':
			pmapdebug |= PDB_BOOT1;
			break;
		}
}
#endif


/*
 * Calculate the correct number of page colors to use.  This should be the
 * size of the E$/PAGE_SIZE.  However, different CPUs can have different sized
 * E$, so we need to take the GCM of the E$ size.
 */
static int pmap_calculate_colors(void)
{
	int node;
	int size, assoc, color, maxcolor = 1;

	for (node = prom_firstchild(prom_findroot()); node != 0;
	     node = prom_nextsibling(node)) {
		char *name = prom_getpropstring(node, "device_type");
		if (strcmp("cpu", name) != 0)
			continue;

		/* Found a CPU, get the E$ info. */
		size = prom_getpropint(node, "ecache-size", -1);
		if (size == -1) {
			prom_printf("pmap_calculate_colors: node %x has "
				"no ecache-size\n", node);
			/* If we can't get the E$ size, skip the node */
			continue;
		}

		assoc = prom_getpropint(node, "ecache-associativity", 1);
		color = size/assoc/PAGE_SIZE;
		if (color > maxcolor)
			maxcolor = color;
	}
	return (maxcolor);
}

static void pmap_alloc_bootargs(void)
{
/*	extern struct cpu_bootargs *cpu_args; */
	char *v;

	v = OF_claim(NULL, 2*PAGE_SIZE, PAGE_SIZE);
	if ((v == NULL) || (v == (void*)-1))
		panic("Can't claim a page of memory.");

	memset(v, 0, 2*PAGE_SIZE);

	cpu_args = (struct cpu_bootargs*)v;

	cpu_args->cb_initstack = v + 2*PAGE_SIZE;
}

#if defined(MULTIPROCESSOR)
static void pmap_mp_init(void);

static void
pmap_mp_init(void)
{
	pte_t *tp;
	char *v;
	int i;

	extern void cpu_mp_startup(void);

	if ((v = OF_claim(NULL, PAGE_SIZE, PAGE_SIZE)) == NULL) {
		panic("pmap_mp_init: Cannot claim a page.");
	}

	bcopy(mp_tramp_code, v, mp_tramp_code_len);
	*(u_long *)(v + mp_tramp_tlb_slots) = kernel_tlb_slots;
	*(u_long *)(v + mp_tramp_func) = (u_long)cpu_mp_startup;
	*(u_long *)(v + mp_tramp_ci) = (u_long)cpu_args;
	tp = (pte_t *)(v + mp_tramp_code_len);
	for (i = 0; i < kernel_tlb_slots; i++) {
		tp[i].tag  = kernel_tlbs[i].te_va;
		tp[i].data = TSB_DATA(0,		/* g */
				PGSZ_4M,		/* sz */
				kernel_tlbs[i].te_pa,	/* pa */
				1, /* priv */
				1, /* write */
				1, /* cache */
				1, /* aliased */
				1, /* valid */
				0 /* ie */);
		tp[i].data |= TLB_L | TLB_CV;
		printf("xtlb[%d]: Tag: %" PRIx64 " Data: %" PRIx64 "\n",
				i, tp[i].tag,
				   tp[i].data);
	}

	for (i = 0; i < PAGE_SIZE; i += sizeof(long))
		flush(v + i);

	cpu_spinup_trampoline = (vaddr_t)v;
}
#else
#define pmap_mp_init()	((void)0)
#endif

paddr_t pmap_kextract(vaddr_t va);

paddr_t
pmap_kextract(vaddr_t va)
{
	int i;
	paddr_t paddr = (paddr_t)-1;

	for (i = 0; i < kernel_tlb_slots; i++) {
		if ((va & ~PAGE_MASK_4M) == kernel_tlbs[i].te_va) {
			paddr = kernel_tlbs[i].te_pa +
				(paddr_t)(va & PAGE_MASK_4M);
			break;
		}
	}

	if (i == kernel_tlb_slots) {
		panic("pmap_kextract: Address %p is not from kernel space.\n"
				"Data segment is too small?\n", (void*)va);
	}

	return (paddr);
}

/*
 * Bootstrap kernel allocator, allocates from unused space in 4MB kernel
 * data segment meaning that
 *
 * - Access to allocated memory will never generate a trap
 * - Allocated chunks are never reclaimed or freed
 * - Allocation calls do not change PROM memlists
 */
static struct mem_region kdata_mem_pool;

static void
kdata_alloc_init(vaddr_t va_start, vaddr_t va_end)
{
	vsize_t va_size = va_end - va_start;

	kdata_mem_pool.start = va_start;
	kdata_mem_pool.size  = va_size;

	BDPRINTF(PDB_BOOT, ("kdata_alloc_init(): %d bytes @%p.\n", va_size,
				va_start));
}

static vaddr_t
kdata_alloc(vsize_t size, vsize_t align)
{
	vaddr_t va;
	vsize_t asize;

	asize = roundup(kdata_mem_pool.start, align) - kdata_mem_pool.start;

	kdata_mem_pool.start += asize;
	kdata_mem_pool.size  -= asize;

	if (kdata_mem_pool.size < size) {
		panic("kdata_alloc(): Data segment is too small.\n");
	}

	va = kdata_mem_pool.start;
	kdata_mem_pool.start += size;
	kdata_mem_pool.size  -= size;

	BDPRINTF(PDB_BOOT, ("kdata_alloc(): Allocated %d@%p, %d free.\n",
				size, (void*)va, kdata_mem_pool.size));

	return (va);
}

/*
 * Unified routine for reading PROM properties.
 */
static void
pmap_read_memlist(const char *device, const char *property, void **ml,
		  int *ml_size, vaddr_t (* ml_alloc)(vsize_t, vsize_t))
{
	void *va;
	int size, handle;

	if ( (handle = prom_finddevice(device)) == 0) {
		prom_printf("pmap_read_memlist(): No %s device found.\n",
				device);
		prom_halt();
	}
	if ( (size = OF_getproplen(handle, property)) < 0) {
		prom_printf("pmap_read_memlist(): %s/%s has no length.\n",
				device, property);
		prom_halt();
	}
	if ( (va = (void*)(* ml_alloc)(size, sizeof(uint64_t))) == NULL) {
		prom_printf("pmap_read_memlist(): Cannot allocate memlist.\n");
		prom_halt();
	}
	if (OF_getprop(handle, property, va, size) <= 0) {
		prom_printf("pmap_read_memlist(): Cannot read %s/%s.\n",
				device, property);
		prom_halt();
	}

	*ml = va;
	*ml_size = size;
}

/*
 * This is called during bootstrap, before the system is really initialized.
 *
 * It's called with the start and end virtual addresses of the kernel.  We
 * bootstrap the pmap allocator now.  We will allocate the basic structures we
 * need to bootstrap the VM system here: the page frame tables, the TSB, and
 * the free memory lists.
 *
 * Now all this is becoming a bit obsolete.  maxctx is still important, but by
 * separating the kernel text and data segments we really would need to
 * provide the start and end of each segment.  But we can't.  The rodata
 * segment is attached to the end of the kernel segment and has nothing to
 * delimit its end.  We could still pass in the beginning of the kernel and
 * the beginning and end of the data segment but we could also just as easily
 * calculate that all in here.
 *
 * To handle the kernel text, we need to do a reverse mapping of the start of
 * the kernel, then traverse the free memory lists to find out how big it is.
 */

void
pmap_bootstrap(u_long kernelstart, u_long kernelend)
{
	extern int etext, data_start[];	/* start of data segment */
	extern int msgbufmapped;
	struct mem_region *mp, *mp1, *avail, *orig;
	int i, j, pcnt, msgbufsiz;
	size_t s, sz;
	int64_t data;
	vaddr_t va, intstk;
	uint64_t phys_msgbuf;
	paddr_t newp = 0;

	void *prom_memlist;
	int prom_memlist_size;

	extern int	get_maxctx(void);

	BDPRINTF(PDB_BOOT, ("Entered pmap_bootstrap.\n"));

	/*
	 * Calculate kernel size.
	 */
	ktext   = kernelstart;
	ktextp  = pmap_kextract(ktext);
	ektext  = roundup((vaddr_t)&etext, PAGE_SIZE_4M);
	ektextp = roundup(pmap_kextract((vaddr_t)&etext), PAGE_SIZE_4M);

	kdata   = (vaddr_t)data_start;
	kdatap  = pmap_kextract(kdata);
	ekdata  = roundup(kernelend, PAGE_SIZE_4M);
	ekdatap = roundup(pmap_kextract(kernelend), PAGE_SIZE_4M);

	BDPRINTF(PDB_BOOT, ("Virtual layout: text %lx-%lx, data %lx-%lx.\n",
				ktext, ektext, kdata, ekdata));
	BDPRINTF(PDB_BOOT, ("Physical layout: text %lx-%lx, data %lx-%lx.\n",
				ktextp, ektextp, kdatap, ekdatap));

	/* Initialize bootstrap allocator. */
	kdata_alloc_init(kernelend + 1 * 1024 * 1024, ekdata);

#ifdef DEBUG
	pmap_bootdebug();
#endif

	pmap_alloc_bootargs();
	pmap_mp_init();

	/*
	 * set machine page size
	 */
	uvmexp.pagesize = NBPG;
	uvmexp.ncolors = pmap_calculate_colors();
	uvm_setpagesize();

	/*
	 * Get hold or the message buffer.
	 */
	msgbufp = (struct kern_msgbuf *)(vaddr_t)MSGBUF_VA;
/* XXXXX -- increase msgbufsiz for uvmhist printing */
	msgbufsiz = 4*PAGE_SIZE /* round_page(sizeof(struct msgbuf)) */;
	BDPRINTF(PDB_BOOT, ("Trying to allocate msgbuf at %lx, size %lx\n",
			    (long)msgbufp, (long)msgbufsiz));
	if ((long)msgbufp !=
	    (long)(phys_msgbuf = prom_claim_virt((vaddr_t)msgbufp, msgbufsiz)))
		prom_printf(
		    "cannot get msgbuf VA, msgbufp=%p, phys_msgbuf=%lx\n",
		    (void *)msgbufp, (long)phys_msgbuf);
	phys_msgbuf = prom_get_msgbuf(msgbufsiz, MMU_PAGE_ALIGN);
	BDPRINTF(PDB_BOOT,
		("We should have the memory at %lx, let's map it in\n",
			phys_msgbuf));
	if (prom_map_phys(phys_msgbuf, msgbufsiz, (vaddr_t)msgbufp,
			  -1/* sunos does this */) == -1) {
		prom_printf("Failed to map msgbuf\n");
	} else {
		BDPRINTF(PDB_BOOT, ("msgbuf mapped at %p\n",
			(void *)msgbufp));
	}
	msgbufmapped = 1;	/* enable message buffer */
	initmsgbuf((void *)msgbufp, msgbufsiz);

	/*
	 * Find out how much RAM we have installed.
	 */
	BDPRINTF(PDB_BOOT, ("pmap_bootstrap: getting phys installed\n"));
	pmap_read_memlist("/memory", "reg", &prom_memlist, &prom_memlist_size,
			kdata_alloc);
	phys_installed = prom_memlist;
	phys_installed_size = prom_memlist_size / sizeof(*phys_installed);

#ifdef DEBUG
	if (pmapdebug & PDB_BOOT1) {
		/* print out mem list */
		prom_printf("Installed physical memory:\n");
		for (i = 0; i < phys_installed_size; i++) {
			prom_printf("memlist start %lx size %lx\n",
					(u_long)phys_installed[i].start,
					(u_long)phys_installed[i].size);
		}
	}
#endif

	BDPRINTF(PDB_BOOT1, ("Calculating physmem:"));
	for (i = 0; i < phys_installed_size; i++)
		physmem += btoc(phys_installed[i].size);
	BDPRINTF(PDB_BOOT1, (" result %x or %d pages\n",
			     (int)physmem, (int)physmem));

	/*
	 * Calculate approx TSB size.  This probably needs tweaking.
	 */
	if (physmem < btoc(64 * 1024 * 1024))
		tsbsize = 0;
	else if (physmem < btoc(512 * 1024 * 1024))
		tsbsize = 1;
	else
		tsbsize = 2;

	/*
	 * Save the prom translations
	 */
	pmap_read_memlist("/virtual-memory", "translations", &prom_memlist,
			&prom_memlist_size, kdata_alloc);
	prom_map = prom_memlist;
	prom_map_size = prom_memlist_size / sizeof(struct prom_map);

#ifdef DEBUG
	if (pmapdebug & PDB_BOOT) {
		/* print out mem list */
		prom_printf("Prom xlations:\n");
		for (i = 0; i < prom_map_size; i++) {
			prom_printf("start %016lx size %016lx tte %016lx\n",
				    (u_long)prom_map[i].vstart,
				    (u_long)prom_map[i].vsize,
				    (u_long)prom_map[i].tte);
		}
		prom_printf("End of prom xlations\n");
	}
#endif

	/*
	 * Here's a quick in-lined reverse bubble sort.  It gets rid of
	 * any translations inside the kernel data VA range.
	 */
	for (i = 0; i < prom_map_size; i++) {
		for (j = i; j < prom_map_size; j++) {
			if (prom_map[j].vstart > prom_map[i].vstart) {
				struct prom_map tmp;

				tmp = prom_map[i];
				prom_map[i] = prom_map[j];
				prom_map[j] = tmp;
			}
		}
	}
#ifdef DEBUG
	if (pmapdebug & PDB_BOOT) {
		/* print out mem list */
		prom_printf("Prom xlations:\n");
		for (i = 0; i < prom_map_size; i++) {
			prom_printf("start %016lx size %016lx tte %016lx\n",
				    (u_long)prom_map[i].vstart,
				    (u_long)prom_map[i].vsize,
				    (u_long)prom_map[i].tte);
		}
		prom_printf("End of prom xlations\n");
	}
#endif

	/*
	 * Allocate a ncpu*128KB page for the cpu_info & stack structure now.
	 */
	cpu0paddr = prom_alloc_phys(16 * PAGE_SIZE * sparc_ncpus, 8 * PAGE_SIZE);
	if (cpu0paddr == 0) {
		prom_printf("Cannot allocate cpu_infos\n");
		prom_halt();
	}

	/*
	 * Now the kernel text segment is in its final location we can try to
	 * find out how much memory really is free.
	 */
	pmap_read_memlist("/memory", "available", &prom_memlist,
			&prom_memlist_size, kdata_alloc);
	orig = prom_memlist;
	sz  = prom_memlist_size;
	pcnt = prom_memlist_size / sizeof(*orig);

	BDPRINTF(PDB_BOOT1, ("Available physical memory:\n"));
	avail = (struct mem_region*)kdata_alloc(sz, sizeof(uint64_t));
	for (i = 0; i < pcnt; i++) {
		avail[i] = orig[i];
		BDPRINTF(PDB_BOOT1, ("memlist start %lx size %lx\n",
					(u_long)orig[i].start,
					(u_long)orig[i].size));
	}
	BDPRINTF(PDB_BOOT1, ("End of available physical memory\n"));

	/*
	 * Allocate and initialize a context table
	 */
	numctx = get_maxctx();
	ctxbusy = (paddr_t *)kdata_alloc(CTXSIZE, sizeof(uint64_t));
	memset(ctxbusy, 0, CTXSIZE);
	LIST_INIT(&pmap_ctxlist);

	/*
	 * Allocate our TSB.
	 *
	 * We will use the left over space to flesh out the kernel pmap.
	 */
	tsb_dmmu = (pte_t *)kdata_alloc(TSBSIZE, TSBSIZE);
	memset(tsb_dmmu, 0, TSBSIZE);
	tsb_immu = (pte_t *)kdata_alloc(TSBSIZE, TSBSIZE);
	memset(tsb_immu, 0, TSBSIZE);

	BDPRINTF(PDB_BOOT1, ("TSB allocated at %p/%p size %08x\n",
	    tsb_dmmu, tsb_immu, TSBSIZE));

	BDPRINTF(PDB_BOOT, ("ktext %08lx[%08lx] - %08lx[%08lx] : "
				"kdata %08lx[%08lx] - %08lx[%08lx]\n",
				(u_long)ktext, (u_long)ktextp,
				(u_long)ektext, (u_long)ektextp,
				(u_long)kdata, (u_long)kdatap,
				(u_long)ekdata, (u_long)ekdatap));
#ifdef DEBUG
	if (pmapdebug & PDB_BOOT1) {
		/* print out mem list */
		prom_printf("Available %lx physical memory before cleanup:\n",
			    (u_long)avail);
		for (i = 0; i < pcnt; i++) {
			prom_printf("memlist start %lx size %lx\n",
				    (u_long)avail[i].start,
				    (u_long)avail[i].size);
		}
		prom_printf("End of available physical memory before cleanup\n");
		prom_printf("kernel physical text size %08lx - %08lx\n",
			    (u_long)ktextp, (u_long)ektextp);
		prom_printf("kernel physical data size %08lx - %08lx\n",
			    (u_long)kdatap, (u_long)ekdatap);
	}
#endif
	/*
	 * Here's a another quick in-lined bubble sort.
	 */
	for (i = 0; i < pcnt; i++) {
		for (j = i; j < pcnt; j++) {
			if (avail[j].start < avail[i].start) {
				struct mem_region tmp;
				tmp = avail[i];
				avail[i] = avail[j];
				avail[j] = tmp;
			}
		}
	}

	/* Throw away page zero if we have it. */
	if (avail->start == 0) {
		avail->start += PAGE_SIZE;
		avail->size -= PAGE_SIZE;
	}

	/*
	 * Now we need to remove the area we valloc'ed from the available
	 * memory lists.  (NB: we may have already alloc'ed the entire space).
	 */
	npgs = 0;
	for (mp = avail, i = 0; i < pcnt; i++, mp = &avail[i]) {
		/*
		 * Now page align the start of the region.
		 */
		s = mp->start % PAGE_SIZE;
		if (mp->size >= s) {
			mp->size -= s;
			mp->start += s;
		}
		/*
		 * And now align the size of the region.
		 */
		mp->size -= mp->size % PAGE_SIZE;
		/*
		 * Check whether some memory is left here.
		 */
		if (mp->size == 0) {
			memcpy(mp, mp + 1,
			      (pcnt - (mp - avail)) * sizeof *mp);
			pcnt--;
			mp--;
			continue;
		}
		s = mp->start;
		sz = mp->size;
		npgs += btoc(sz);
		for (mp1 = avail; mp1 < mp; mp1++)
			if (s < mp1->start)
				break;
		if (mp1 < mp) {
			memcpy(mp1 + 1, mp1, (char *)mp - (char *)mp1);
			mp1->start = s;
			mp1->size = sz;
		}
#ifdef DEBUG
/* Clear all memory we give to the VM system.  I want to make sure
 * the PROM isn't using it for something, so this should break the PROM.
 */

/* Calling pmap_zero_page() at this point also hangs some machines
 * so don't do it at all. -- pk 26/02/2002
 */
#if 0
		{
			paddr_t p;
			for (p = mp->start; p < mp->start+mp->size;
			     p += PAGE_SIZE)
				pmap_zero_page(p);
		}
#endif
#endif /* DEBUG */
		/*
		 * In future we should be able to specify both allocated
		 * and free.
		 */
		BDPRINTF(PDB_BOOT1, ("uvm_page_physload(%lx, %lx)\n",
					(long)mp->start,
					(long)(mp->start + mp->size)));
		uvm_page_physload(
			atop(mp->start),
			atop(mp->start+mp->size),
			atop(mp->start),
			atop(mp->start+mp->size),
			VM_FREELIST_DEFAULT);
	}

#ifdef DEBUG
	if (pmapdebug & PDB_BOOT) {
		/* print out mem list */
		prom_printf("Available physical memory after cleanup:\n");
		for (i = 0; i < pcnt; i++) {
			prom_printf("avail start %lx size %lx\n",
				    (long)avail[i].start, (long)avail[i].size);
		}
		prom_printf("End of available physical memory after cleanup\n");
	}
#endif
	/*
	 * Allocate and clear out pmap_kernel()->pm_segs[]
	 */
	simple_lock_init(&pmap_kernel()->pm_lock);
	pmap_kernel()->pm_refs = 1;
	pmap_kernel()->pm_ctx = 0;

	/* Throw away page zero */
	do {
		pmap_get_page(&newp);
	} while (!newp);
	pmap_kernel()->pm_segs=(paddr_t *)(u_long)newp;
	pmap_kernel()->pm_physaddr = newp;
	/* mark kernel context as busy */
	ctxbusy[0] = pmap_kernel()->pm_physaddr;

	/*
	 * finish filling out kernel pmap.
	 */

	BDPRINTF(PDB_BOOT, ("pmap_kernel()->pm_physaddr = %lx\n",
	    (long)pmap_kernel()->pm_physaddr));
	/*
	 * Tell pmap about our mesgbuf -- Hope this works already
	 */
#ifdef DEBUG
	BDPRINTF(PDB_BOOT1, ("Calling consinit()\n"));
	if (pmapdebug & PDB_BOOT1)
		consinit();
	BDPRINTF(PDB_BOOT1, ("Inserting mesgbuf into pmap_kernel()\n"));
#endif
	/* it's not safe to call pmap_enter so we need to do this ourselves */
	va = (vaddr_t)msgbufp;
	prom_map_phys(phys_msgbuf, msgbufsiz, (vaddr_t)msgbufp, -1);
	while (msgbufsiz) {
		data = TSB_DATA(0 /* global */,
			PGSZ_8K,
			phys_msgbuf,
			1 /* priv */,
			1 /* Write */,
			1 /* Cacheable */,
			FORCE_ALIAS /* ALIAS -- Disable D$ */,
			1 /* valid */,
			0 /* IE */);
		pmap_enter_kpage(va, data);
		va += PAGE_SIZE;
		msgbufsiz -= PAGE_SIZE;
		phys_msgbuf += PAGE_SIZE;
	}
	BDPRINTF(PDB_BOOT1, ("Done inserting mesgbuf into pmap_kernel()\n"));

	BDPRINTF(PDB_BOOT1, ("Inserting PROM mappings into pmap_kernel()\n"));
	for (i = 0; i < prom_map_size; i++)
		if (prom_map[i].vstart && ((prom_map[i].vstart >> 32) == 0))
			for (j = 0; j < prom_map[i].vsize; j += PAGE_SIZE) {
				int k;

				for (k = 0; page_size_map[k].mask; k++) {
					if (((prom_map[i].vstart |
					      prom_map[i].tte) &
					      page_size_map[k].mask) == 0 &&
					      page_size_map[k].mask <
					      prom_map[i].vsize)
						break;
				}
#ifdef DEBUG
				page_size_map[k].use++;
#endif
				/* Enter PROM map into pmap_kernel() */
				pmap_enter_kpage(prom_map[i].vstart + j,
					(prom_map[i].tte + j) | TLB_EXEC |
					page_size_map[k].code);
			}
	BDPRINTF(PDB_BOOT1, ("Done inserting PROM mappings into pmap_kernel()\n"));

	/*
	 * Fix up start of kernel heap.
	 */
	vmmap = (vaddr_t)roundup(ekdata, 4*MEG);
	/* Let's keep 1 page of redzone after the kernel */
	vmmap += PAGE_SIZE;
	{
		extern struct pcb *proc0paddr;
		extern void main(void);
		vaddr_t u0va;
		paddr_t pa;

		u0va = vmmap;

		BDPRINTF(PDB_BOOT1,
			("Inserting proc0 USPACE into pmap_kernel() at %p\n",
				vmmap));

		while (vmmap < u0va + 2*USPACE) {
			int64_t data1;

			if (!pmap_get_page(&pa))
				panic("pmap_bootstrap: no pages");
			prom_map_phys(pa, PAGE_SIZE, vmmap, -1);
			data1 = TSB_DATA(0 /* global */,
				PGSZ_8K,
				pa,
				1 /* priv */,
				1 /* Write */,
				1 /* Cacheable */,
				FORCE_ALIAS /* ALIAS -- Disable D$ */,
				1 /* valid */,
				0 /* IE */);
			pmap_enter_kpage(vmmap, data1);
			vmmap += PAGE_SIZE;
		}
		BDPRINTF(PDB_BOOT1,
			 ("Done inserting stack 0 into pmap_kernel()\n"));

		/* Now map in and initialize our cpu_info structure */
#ifdef DIAGNOSTIC
		vmmap += PAGE_SIZE; /* redzone -- XXXX do we need one? */
#endif
		if ((vmmap ^ INTSTACK) & VA_ALIAS_MASK)
			vmmap += PAGE_SIZE; /* Matchup virtual color for D$ */
		intstk = vmmap;
		cpus = (struct cpu_info *)(intstk + CPUINFO_VA - INTSTACK);

		BDPRINTF(PDB_BOOT1,
			("Inserting cpu_info into pmap_kernel() at %p\n",
				 cpus));
		/* Now map in all 16 pages of interrupt stack/cpu_info */
		pa = cpu0paddr;
		prom_map_phys(pa, 128*KB, vmmap, -1);

		/*
		 * Also map it in as the interrupt stack.
		 * This lets the PROM see this if needed.
		 *
		 * XXXX locore.s does not flush these mappings
		 * before installing the locked TTE.
		 */
		prom_map_phys(pa, 64*KB, INTSTACK, -1);
		prom_map_phys(pa + 64*KB, 64*KB, KSTACK_VA, -1);
		for (i = 0; i < 16; i++) {
			int64_t data1;

			data1 = TSB_DATA(0 /* global */,
				PGSZ_8K,
				pa,
				1 /* priv */,
				1 /* Write */,
				1 /* Cacheable */,
				FORCE_ALIAS /* ALIAS -- Disable D$ */,
				1 /* valid */,
				0 /* IE */);
			pmap_enter_kpage(vmmap, data1);
			vmmap += PAGE_SIZE;
			pa += PAGE_SIZE;
		}
		BDPRINTF(PDB_BOOT1, ("Initializing cpu_info\n"));

		/* Initialize our cpu_info structure */
		memset((void *)intstk, 0, 128 * KB);
		cpus->ci_self = cpus;
		cpus->ci_next = NULL;
		cpus->ci_curlwp = &lwp0;
		cpus->ci_flags = CPUF_PRIMARY;
		cpus->ci_upaid = CPU_UPAID;
		cpus->ci_number = 0;
		cpus->ci_cpuid = cpus->ci_upaid;
		cpus->ci_fplwp = NULL;
		cpus->ci_spinup = main; /* Call main when we're running. */
		cpus->ci_paddr = cpu0paddr;
		cpus->ci_cpcb = (struct pcb *)u0va;
		cpus->ci_initstack = (void *)INITSTACK_VA;
		proc0paddr = cpus->ci_cpcb;

		cpu0paddr += 128 * KB;

		CPUSET_CLEAR(cpus_active);
		CPUSET_ADD(cpus_active, 0);

		/* The rest will be done at CPU attach time. */
		BDPRINTF(PDB_BOOT1,
			 ("Done inserting cpu_info into pmap_kernel()\n"));
	}

	vmmap = (vaddr_t)reserve_dumppages((void *)(u_long)vmmap);

	/*
	 * Set up bounds of allocatable memory for vmstat et al.
	 */
	nextavail = avail->start;
	avail_start = nextavail;
	for (mp = avail; mp->size; mp++)
		avail_end = mp->start+mp->size;
	BDPRINTF(PDB_BOOT1, ("Finished pmap_bootstrap()\n"));

	BDPRINTF(PDB_BOOT, ("left kdata: %" PRId64 " @%" PRIx64 ".\n",
				kdata_mem_pool.size, kdata_mem_pool.start));
}

/*
 * Initialize anything else for pmap handling.
 * Called during vm_init().
 */
void
pmap_init()
{
	struct vm_page *pg;
	struct pglist pglist;
	uint64_t data;
	paddr_t pa;
	psize_t size;
	vaddr_t va;

	BDPRINTF(PDB_BOOT1, ("pmap_init()\n"));

	size = sizeof(struct pv_entry) * physmem;
	if (uvm_pglistalloc((psize_t)size, (paddr_t)0, (paddr_t)-1,
		(paddr_t)PAGE_SIZE, (paddr_t)0, &pglist, 1, 0) != 0)
		panic("pmap_init: no memory");

	va = uvm_km_alloc(kernel_map, size, 0, UVM_KMF_VAONLY);
	if (va == 0)
		panic("pmap_init: no memory");

	/* Map the pages */
	TAILQ_FOREACH(pg, &pglist, pageq) {
		pa = VM_PAGE_TO_PHYS(pg);
		pmap_zero_page(pa);
		data = TSB_DATA(0 /* global */,
			PGSZ_8K,
			pa,
			1 /* priv */,
			1 /* Write */,
			1 /* Cacheable */,
			FORCE_ALIAS /* ALIAS -- Disable D$ */,
			1 /* valid */,
			0 /* IE */);
		pmap_enter_kpage(va, data);
		va += PAGE_SIZE;
	}

	/*
	 * initialize the pmap pools.
	 */
	pool_init(&pmap_pmap_pool, sizeof(struct pmap), 0, 0, 0, "pmappl",
	    &pool_allocator_nointr, IPL_NONE);
	pool_init(&pmap_pv_pool, sizeof(struct pv_entry), 0, 0, 0, "pv_entry",
	    &pool_allocator_nointr, IPL_NONE);

	vm_first_phys = avail_start;
	vm_num_phys = avail_end - avail_start;
}

/*
 * How much virtual space is available to the kernel?
 */
static vaddr_t kbreak; /* End of kernel VA */
void
pmap_virtual_space(start, end)
	vaddr_t *start, *end;
{

	/*
	 * Reserve one segment for kernel virtual memory
	 */
	/* Reserve two pages for pmap_copy_page && /dev/mem */
	*start = kbreak = (vaddr_t)(vmmap + 2*PAGE_SIZE);
	*end = VM_MAX_KERNEL_ADDRESS;
	BDPRINTF(PDB_BOOT1, ("pmap_virtual_space: %x-%x\n", *start, *end));
}

/*
 * Preallocate kernel page tables to a specified VA.
 * This simply loops through the first TTE for each
 * page table from the beginning of the kernel pmap,
 * reads the entry, and if the result is
 * zero (either invalid entry or no page table) it stores
 * a zero there, populating page tables in the process.
 * This is not the most efficient technique but i don't
 * expect it to be called that often.
 */
vaddr_t
pmap_growkernel(maxkvaddr)
        vaddr_t maxkvaddr;
{
	struct pmap *pm = pmap_kernel();
	paddr_t pa;

	if (maxkvaddr >= KERNEND) {
		printf("WARNING: cannot extend kernel pmap beyond %p to %p\n",
		       (void *)KERNEND, (void *)maxkvaddr);
		return (kbreak);
	}
	simple_lock(&pm->pm_lock);
	DPRINTF(PDB_GROW, ("pmap_growkernel(%lx...%lx)\n", kbreak, maxkvaddr));
	/* Align with the start of a page table */
	for (kbreak &= (-1 << PDSHIFT); kbreak < maxkvaddr;
	     kbreak += (1 << PDSHIFT)) {
		if (pseg_get(pm, kbreak))
			continue;

		pa = 0;
		while (pseg_set(pm, kbreak, 0, pa) & 1) {
			DPRINTF(PDB_GROW,
			    ("pmap_growkernel: extending %lx\n", kbreak));
			pa = 0;
			if (!pmap_get_page(&pa))
				panic("pmap_growkernel: no pages");
			ENTER_STAT(ptpneeded);
		}
	}
	simple_unlock(&pm->pm_lock);
	return (kbreak);
}

/*
 * Create and return a physical map.
 */
struct pmap *
pmap_create()
{
	struct pmap *pm;

	DPRINTF(PDB_CREATE, ("pmap_create()\n"));

	pm = pool_get(&pmap_pmap_pool, PR_WAITOK);
	memset(pm, 0, sizeof *pm);
	DPRINTF(PDB_CREATE, ("pmap_create(): created %p\n", pm));

	simple_lock_init(&pm->pm_lock);
	pm->pm_refs = 1;
	TAILQ_INIT(&pm->pm_obj.memq);
	if (pm != pmap_kernel()) {
		while (!pmap_get_page(&pm->pm_physaddr)) {
			uvm_wait("pmap_create");
		}
		pm->pm_segs = (paddr_t *)(u_long)pm->pm_physaddr;
	}
	DPRINTF(PDB_CREATE, ("pmap_create(%p): ctx %d\n", pm, pm->pm_ctx));
	return pm;
}

/*
 * Add a reference to the given pmap.
 */
void
pmap_reference(pm)
	struct pmap *pm;
{

	simple_lock(&pm->pm_lock);
	pm->pm_refs++;
	simple_unlock(&pm->pm_lock);
}

/*
 * Retire the given pmap from service.
 * Should only be called if the map contains no valid mappings.
 */
void
pmap_destroy(pm)
	struct pmap *pm;
{
	struct vm_page *pg, *nextpg;
	int refs;

	simple_lock(&pm->pm_lock);
	refs = --pm->pm_refs;
	simple_unlock(&pm->pm_lock);
	if (refs > 0) {
		return;
	}
	DPRINTF(PDB_DESTROY, ("pmap_destroy: freeing pmap %p\n", pm));
	ctx_free(pm);

	/* we could be a little smarter and leave pages zeroed */
	for (pg = TAILQ_FIRST(&pm->pm_obj.memq); pg != NULL; pg = nextpg) {
		nextpg = TAILQ_NEXT(pg, listq);
		TAILQ_REMOVE(&pm->pm_obj.memq, pg, listq);
		KASSERT(pg->mdpage.mdpg_pvh.pv_pmap == NULL);
		uvm_pagefree(pg);
	}
	pmap_free_page((paddr_t)(u_long)pm->pm_segs);
	pool_put(&pmap_pmap_pool, pm);
}

/*
 * Copy the range specified by src_addr/len
 * from the source map to the range dst_addr/len
 * in the destination map.
 *
 * This routine is only advisory and need not do anything.
 */
void
pmap_copy(dst_pmap, src_pmap, dst_addr, len, src_addr)
	struct pmap *dst_pmap, *src_pmap;
	vaddr_t dst_addr, src_addr;
	vsize_t len;
{

	DPRINTF(PDB_CREATE, ("pmap_copy(%p, %p, %p, %lx, %p)\n",
			     dst_pmap, src_pmap, (void *)(u_long)dst_addr,
			     (u_long)len, (void *)(u_long)src_addr));
}

/*
 * Garbage collects the physical map system for
 * pages which are no longer used.
 * Success need not be guaranteed -- that is, there
 * may well be pages which are not referenced, but
 * others may be collected.
 * Called by the pageout daemon when pages are scarce.
 */
void
pmap_collect(pm)
	struct pmap *pm;
{
	int64_t data;
	paddr_t pa, *pdir, *ptbl;
	struct vm_page *pg;
	int i, j, k, n, m;

	/*
	 * This is a good place to scan the pmaps for page tables with
	 * no valid mappings in them and free them.
	 */

	/* NEVER GARBAGE COLLECT THE KERNEL PMAP */
	if (pm == pmap_kernel())
		return;

	simple_lock(&pm->pm_lock);
	for (i = 0; i < STSZ; i++) {
		pdir = (paddr_t *)(u_long)ldxa((vaddr_t)&pm->pm_segs[i],
					       ASI_PHYS_CACHED);
		if (pdir == NULL) {
			continue;
		}
		m = 0;
		for (k = 0; k < PDSZ; k++) {
			ptbl = (paddr_t *)(u_long)ldxa((vaddr_t)&pdir[k],
						       ASI_PHYS_CACHED);
			if (ptbl == NULL) {
				continue;
			}
			m++;
			n = 0;
			for (j = 0; j < PTSZ; j++) {
				data = ldxa((vaddr_t)&ptbl[j], ASI_PHYS_CACHED);
				if (data & TLB_V)
					n++;
			}
			if (!n) {
				stxa((paddr_t)(u_long)&pdir[k],
				     ASI_PHYS_CACHED, 0);
				pa = (paddr_t)(u_long)ptbl;
				pg = PHYS_TO_VM_PAGE(pa);
				TAILQ_REMOVE(&pm->pm_obj.memq, pg, listq);
				pmap_free_page(pa);
			}
		}
		if (!m) {
			stxa((paddr_t)(u_long)&pm->pm_segs[i],
			     ASI_PHYS_CACHED, 0);
			pa = (paddr_t)(u_long)pdir;
			pg = PHYS_TO_VM_PAGE(pa);
			TAILQ_REMOVE(&pm->pm_obj.memq, pg, listq);
			pmap_free_page(pa);
		}
	}
	simple_unlock(&pm->pm_lock);
}

/*
 * Activate the address space for the specified process.  If the
 * process is the current process, load the new MMU context.
 */
void
pmap_activate(l)
	struct lwp *l;
{
	struct pmap *pmap = l->l_proc->p_vmspace->vm_map.pmap;

	if (pmap == pmap_kernel()) {
		return;
	}

	/*
	 * This is essentially the same thing that happens in cpu_switch()
	 * when the newly selected process is about to run, except that we
	 * have to make sure to clean the register windows before we set
	 * the new context.
	 */

	if (l != curlwp) {
		return;
	}
	write_user_windows();
	pmap_activate_pmap(pmap);
}

void
pmap_activate_pmap(struct pmap *pmap)
{

	if (pmap->pm_ctx == 0) {
		(void) ctx_alloc(pmap);
	}
	dmmu_set_secondary_context(pmap->pm_ctx);
}

/*
 * Deactivate the address space of the specified process.
 */
void
pmap_deactivate(l)
	struct lwp *l;
{
}

/*
 * pmap_kenter_pa:		[ INTERFACE ]
 *
 *	Enter a va -> pa mapping into the kernel pmap without any
 *	physical->virtual tracking.
 *
 *	Note: no locking is necessary in this function.
 */
void
pmap_kenter_pa(va, pa, prot)
	vaddr_t va;
	paddr_t pa;
	vm_prot_t prot;
{
	pte_t tte;
	paddr_t ptp;
	struct pmap *pm = pmap_kernel();
	int i;

	KASSERT(va < INTSTACK || va > EINTSTACK);
	KASSERT(va < kdata || va > ekdata);

	/*
	 * Construct the TTE.
	 */

	ENTER_STAT(unmanaged);
	if (pa & (PMAP_NVC|PMAP_NC)) {
		ENTER_STAT(ci);
	}

	tte.data = TSB_DATA(0, PGSZ_8K, pa, 1 /* Privileged */,
			    (VM_PROT_WRITE & prot),
			    !(pa & PMAP_NC), pa & (PMAP_NVC), 1, 0);
	/* We don't track mod/ref here. */
	if (prot & VM_PROT_WRITE)
		tte.data |= TLB_REAL_W|TLB_W;
	if (prot & VM_PROT_EXECUTE)
		tte.data |= TLB_EXEC;
	tte.data |= TLB_TSB_LOCK;	/* wired */
	ptp = 0;

 retry:
	i = pseg_set(pm, va, tte.data, ptp);
	if (i & 1) {
		KASSERT((i & 4) == 0);
		ptp = 0;
		if (!pmap_get_page(&ptp))
			panic("pmap_kenter_pa: no pages");
		ENTER_STAT(ptpneeded);
		goto retry;
	}
	if (ptp && i == 0) {
		/* We allocated a spare page but didn't use it.  Free it. */
		printf("pmap_kenter_pa: freeing unused page %llx\n",
		       (long long)ptp);
		pmap_free_page(ptp);
	}
#ifdef DEBUG
	i = ptelookup_va(va);
	if (pmapdebug & PDB_ENTER)
		prom_printf("pmap_kenter_pa: va=%08x data=%08x:%08x "
			"tsb_dmmu[%d]=%08x\n", va, (int)(tte.data>>32),
			(int)tte.data, i, &tsb_dmmu[i]);
	if (pmapdebug & PDB_MMU_STEAL && tsb_dmmu[i].data) {
		prom_printf("pmap_kenter_pa: evicting entry tag=%x:%08x "
			"data=%08x:%08x tsb_dmmu[%d]=%08x\n",
			(int)(tsb_dmmu[i].tag>>32), (int)tsb_dmmu[i].tag,
			(int)(tsb_dmmu[i].data>>32), (int)tsb_dmmu[i].data,
			i, &tsb_dmmu[i]);
		prom_printf("with va=%08x data=%08x:%08x tsb_dmmu[%d]=%08x\n",
			va, (int)(tte.data>>32), (int)tte.data,	i,
			&tsb_dmmu[i]);
	}
#endif
}

/*
 * pmap_kremove:		[ INTERFACE ]
 *
 *	Remove a mapping entered with pmap_kenter_pa() starting at va,
 *	for size bytes (assumed to be page rounded).
 */
void
pmap_kremove(va, size)
	vaddr_t va;
	vsize_t size;
{
	struct pmap *pm = pmap_kernel();
	int64_t data;
	paddr_t pa;
	int rv;
	bool flush = FALSE;

	KASSERT(va < INTSTACK || va > EINTSTACK);
	KASSERT(va < kdata || va > ekdata);

	DPRINTF(PDB_DEMAP, ("pmap_kremove: start 0x%lx size %lx\n", va, size));
	for (; size >= PAGE_SIZE; va += PAGE_SIZE, size -= PAGE_SIZE) {

#ifdef DIAGNOSTIC
		/*
		 * Is this part of the permanent 4MB mapping?
		 */
		if (va >= ktext && va < roundup(ekdata, 4*MEG))
			panic("pmap_kremove: va=%08x in locked TLB", (u_int)va);
#endif

		data = pseg_get(pm, va);
		if (data == 0) {
			continue;
		}

		flush = TRUE;
		pa = data & TLB_PA_MASK;

		/*
		 * We need to flip the valid bit and
		 * clear the access statistics.
		 */

		rv = pseg_set(pm, va, 0, 0);
		if (rv & 1)
			panic("pmap_kremove: pseg_set needs spare, rv=%d\n",
			    rv);
		DPRINTF(PDB_DEMAP, ("pmap_kremove: seg %x pdir %x pte %x\n",
		    (int)va_to_seg(va), (int)va_to_dir(va),
		    (int)va_to_pte(va)));
		REMOVE_STAT(removes);

		tsb_invalidate(pm->pm_ctx, va);
		REMOVE_STAT(tflushes);

		/*
		 * Here we assume nothing can get into the TLB
		 * unless it has a PTE.
		 */

		tlb_flush_pte(va, pm->pm_ctx);
	}
	if (flush) {
		REMOVE_STAT(flushes);
		blast_dcache();
	}
}

/*
 * Insert physical page at pa into the given pmap at virtual address va.
 * Supports 64-bit pa so we can map I/O space.
 */

int
pmap_enter(pm, va, pa, prot, flags)
	struct pmap *pm;
	vaddr_t va;
	paddr_t pa;
	vm_prot_t prot;
	int flags;
{
	pte_t tte;
	int64_t data;
	paddr_t opa = 0, ptp; /* XXX: gcc */
	pv_entry_t pvh, npv = NULL;
	struct vm_page *pg, *opg, *ptpg;
	int s, i, uncached = 0;
	int size = PGSZ_8K; /* PMAP_SZ_TO_TTE(pa); */
	bool wired = (flags & PMAP_WIRED) != 0;
	bool wasmapped = FALSE;
	bool dopv = TRUE;

	/*
	 * Is this part of the permanent mappings?
	 */
	KASSERT(pm != pmap_kernel() || va < INTSTACK || va > EINTSTACK);
	KASSERT(pm != pmap_kernel() || va < kdata || va > ekdata);

	/*
	 * If a mapping at this address already exists, check if we're
	 * entering the same PA again.  if it's different remove it.
	 */

	simple_lock(&pm->pm_lock);
	data = pseg_get(pm, va);
	if (data & TLB_V) {
		wasmapped = TRUE;
		opa = data & TLB_PA_MASK;
		if (opa != pa) {
			opg = PHYS_TO_VM_PAGE(opa);
			if (opg != NULL) {
				npv = pmap_remove_pv(pm, va, opg);
			}
		}
	}

	/*
	 * Construct the TTE.
	 */
	pg = PHYS_TO_VM_PAGE(pa);
	if (pg) {
		pvh = &pg->mdpage.mdpg_pvh;
		uncached = (pvh->pv_va & (PV_ALIAS|PV_NVC));
#ifdef DIAGNOSTIC
		if ((flags & VM_PROT_ALL) & ~prot)
			panic("pmap_enter: access_type exceeds prot");
#endif
		/*
		 * If we don't have the traphandler do it,
		 * set the ref/mod bits now.
		 */
		if (flags & VM_PROT_ALL)
			pvh->pv_va |= PV_REF;
		if (flags & VM_PROT_WRITE)
			pvh->pv_va |= PV_MOD;

		/*
		 * make sure we have a pv entry ready if we need one.
		 */
		if (pvh->pv_pmap == NULL || (wasmapped && opa == pa)) {
			if (npv != NULL) {
				pool_put(&pmap_pv_pool, npv);
				npv = NULL;
			}
			if (wasmapped && opa == pa) {
				dopv = FALSE;
			}
		} else if (npv == NULL) {
			npv = pool_get(&pmap_pv_pool, PR_NOWAIT);
			if (npv == NULL) {
				if (flags & PMAP_CANFAIL) {
					simple_unlock(&pm->pm_lock);
					return (ENOMEM);
				}
				panic("pmap_enter: no pv entries available");
			}
		}
		ENTER_STAT(managed);
	} else {
		ENTER_STAT(unmanaged);
		dopv = FALSE;
		if (npv != NULL) {
			pool_put(&pmap_pv_pool, npv);
			npv = NULL;
		}
	}

#ifndef NO_VCACHE
	if (pa & PMAP_NVC)
#endif
		uncached = 1;
	if (uncached) {
		ENTER_STAT(ci);
	}
	tte.data = TSB_DATA(0, size, pa, pm == pmap_kernel(),
		flags & VM_PROT_WRITE, !(pa & PMAP_NC),
		uncached, 1, pa & PMAP_LITTLE);
#ifdef HWREF
	if (prot & VM_PROT_WRITE)
		tte.data |= TLB_REAL_W;
	if (prot & VM_PROT_EXECUTE)
		tte.data |= TLB_EXEC;
#else
	/* If it needs ref accounting do nothing. */
	if (!(flags & VM_PROT_READ)) {
		simple_unlock(&pm->pm_lock);
		return 0;
	}
#endif
	if (flags & VM_PROT_EXECUTE) {
		if ((flags & (VM_PROT_READ|VM_PROT_WRITE)) == 0)
			tte.data |= TLB_EXEC_ONLY|TLB_EXEC;
		else
			tte.data |= TLB_EXEC;
	}
	if (wired)
		tte.data |= TLB_TSB_LOCK;
	ptp = 0;

 retry:
	i = pseg_set(pm, va, tte.data, ptp);
	if (i & 4) {
		/* ptp used as L3 */
		KASSERT(ptp != 0);
		KASSERT((i & 3) == 0);
		ptpg = PHYS_TO_VM_PAGE(ptp);
		if (ptpg) {
			ptpg->offset = (uint64_t)va & (0xfffffLL << 23);
			TAILQ_INSERT_TAIL(&pm->pm_obj.memq, ptpg, listq);
		} else {
			KASSERT(pm == pmap_kernel());
		}
	}
	if (i & 2) {
		/* ptp used as L2 */
		KASSERT(ptp != 0);
		KASSERT((i & 4) == 0);
		ptpg = PHYS_TO_VM_PAGE(ptp);
		if (ptpg) {
			ptpg->offset = (((uint64_t)va >> 43) & 0x3ffLL) << 13;
			TAILQ_INSERT_TAIL(&pm->pm_obj.memq, ptpg, listq);
		} else {
			KASSERT(pm == pmap_kernel());
		}
	}
	if (i & 1) {
		KASSERT((i & 4) == 0);
		ptp = 0;
		if (!pmap_get_page(&ptp)) {
			if (flags & PMAP_CANFAIL) {
				simple_unlock(&pm->pm_lock);
				if (npv != NULL) {
					pool_put(&pmap_pv_pool, npv);
				}
				return (ENOMEM);
			} else {
				panic("pmap_enter: no pages");
			}
		}
		ENTER_STAT(ptpneeded);
		goto retry;
	}
	if (ptp && i == 0) {
		/* We allocated a spare page but didn't use it.  Free it. */
		printf("pmap_enter: freeing unused page %llx\n",
		       (long long)ptp);
		pmap_free_page(ptp);
	}
	if (dopv) {
		pmap_enter_pv(pm, va, pa, pg, npv);
	}

	simple_unlock(&pm->pm_lock);
#ifdef DEBUG
	i = ptelookup_va(va);
	if (pmapdebug & PDB_ENTER)
		prom_printf("pmap_enter: va=%08x data=%08x:%08x "
			"tsb_dmmu[%d]=%08x\n", va, (int)(tte.data>>32),
			(int)tte.data, i, &tsb_dmmu[i]);
	if (pmapdebug & PDB_MMU_STEAL && tsb_dmmu[i].data) {
		prom_printf("pmap_enter: evicting entry tag=%x:%08x "
			"data=%08x:%08x tsb_dmmu[%d]=%08x\n",
			(int)(tsb_dmmu[i].tag>>32), (int)tsb_dmmu[i].tag,
			(int)(tsb_dmmu[i].data>>32), (int)tsb_dmmu[i].data, i,
			&tsb_dmmu[i]);
		prom_printf("with va=%08x data=%08x:%08x tsb_dmmu[%d]=%08x\n",
			va, (int)(tte.data>>32), (int)tte.data, i,
			&tsb_dmmu[i]);
	}
#endif

	if (flags & (VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE)) {

		/*
		 * preload the TSB with the new entry,
		 * since we're going to need it immediately anyway.
		 */

		i = ptelookup_va(va);
		tte.tag = TSB_TAG(0, pm->pm_ctx, va);
		s = splhigh();
		if (wasmapped && (pm->pm_ctx || pm == pmap_kernel())) {
			tsb_invalidate(pm->pm_ctx, va);
		}
		if (flags & (VM_PROT_READ | VM_PROT_WRITE)) {
			tsb_dmmu[i].tag = tte.tag;
			__asm volatile("" : : : "memory");
			tsb_dmmu[i].data = tte.data;
		}
		if (flags & VM_PROT_EXECUTE) {
			tsb_immu[i].tag = tte.tag;
			__asm volatile("" : : : "memory");
			tsb_immu[i].data = tte.data;
		}

		/*
		 * it's only necessary to flush the TLB if this page was
		 * previously mapped, but for some reason it's a lot faster
		 * for the fork+exit microbenchmark if we always do it.
		 */

		tlb_flush_pte(va, pm->pm_ctx);
		splx(s);
	} else if (wasmapped && (pm->pm_ctx || pm == pmap_kernel())) {
		/* Force reload -- protections may be changed */
		tsb_invalidate(pm->pm_ctx, va);
		tlb_flush_pte(va, pm->pm_ctx);
	}

	/* We will let the fast mmu miss interrupt load the new translation */
	pv_check();
	return 0;
}

void
pmap_remove_all(pm)
	struct pmap *pm;
{

	if (pm == pmap_kernel()) {
		return;
	}
	write_user_windows();
	pm->pm_refs = 0;
	ctx_free(pm);
	REMOVE_STAT(flushes);
	blast_dcache();
}

/*
 * Remove the given range of mapping entries.
 */
void
pmap_remove(pm, va, endva)
	struct pmap *pm;
	vaddr_t va, endva;
{
	int64_t data;
	paddr_t pa;
	struct vm_page *pg;
	pv_entry_t pv;
	int rv;
	bool flush = FALSE;

	/*
	 * In here we should check each pseg and if there are no more entries,
	 * free it.  It's just that linear scans of 8K pages gets expensive.
	 */

	KASSERT(pm != pmap_kernel() || endva < INTSTACK || va > EINTSTACK);
	KASSERT(pm != pmap_kernel() || endva < kdata || va > ekdata);

	simple_lock(&pm->pm_lock);
	DPRINTF(PDB_REMOVE, ("pmap_remove(pm=%p, va=%p, endva=%p):", pm,
			     (void *)(u_long)va, (void *)(u_long)endva));
	REMOVE_STAT(calls);

	/* Now do the real work */
	for (; va < endva; va += PAGE_SIZE) {
#ifdef DIAGNOSTIC
		/*
		 * Is this part of the permanent 4MB mapping?
		 */
		if (pm == pmap_kernel() && va >= ktext &&
			va < roundup(ekdata, 4*MEG))
			panic("pmap_remove: va=%08llx in locked TLB",
			      (long long)va);
#endif

		data = pseg_get(pm, va);
		if (data == 0) {
			continue;
		}

		flush = TRUE;
		/* First remove the pv entry, if there is one */
		pa = data & TLB_PA_MASK;
		pg = PHYS_TO_VM_PAGE(pa);
		if (pg) {
			pv = pmap_remove_pv(pm, va, pg);
			if (pv != NULL) {
				pool_put(&pmap_pv_pool, pv);
			}
		}

		/*
		 * We need to flip the valid bit and
		 * clear the access statistics.
		 */

		rv = pseg_set(pm, va, 0, 0);
		if (rv & 1)
			panic("pmap_remove: pseg_set needed spare, rv=%d!\n",
			    rv);

		DPRINTF(PDB_REMOVE, (" clearing seg %x pte %x\n",
				     (int)va_to_seg(va), (int)va_to_pte(va)));
		REMOVE_STAT(removes);

		if (!pm->pm_ctx && pm != pmap_kernel())
			continue;

		/*
		 * if the pmap is being torn down, don't bother flushing.
		 */

		if (!pm->pm_refs)
			continue;

		/*
		 * Here we assume nothing can get into the TLB
		 * unless it has a PTE.
		 */

		tsb_invalidate(pm->pm_ctx, va);
		REMOVE_STAT(tflushes);
		tlb_flush_pte(va, pm->pm_ctx);
	}
	simple_unlock(&pm->pm_lock);
	if (flush && pm->pm_refs) {
		REMOVE_STAT(flushes);
		blast_dcache();
	}
	DPRINTF(PDB_REMOVE, ("\n"));
	pv_check();
}

/*
 * Change the protection on the specified range of this pmap.
 */
void
pmap_protect(pm, sva, eva, prot)
	struct pmap *pm;
	vaddr_t sva, eva;
	vm_prot_t prot;
{
	paddr_t pa;
	int64_t data;
	struct vm_page *pg;
	pv_entry_t pv;
	int rv;

	KASSERT(pm != pmap_kernel() || eva < INTSTACK || sva > EINTSTACK);
	KASSERT(pm != pmap_kernel() || eva < kdata || sva > ekdata);

	if (prot == VM_PROT_NONE) {
		pmap_remove(pm, sva, eva);
		return;
	}

	simple_lock(&pm->pm_lock);
	sva = sva & ~PGOFSET;
	for (; sva < eva; sva += PAGE_SIZE) {
#ifdef DEBUG
		/*
		 * Is this part of the permanent 4MB mapping?
		 */
		if (pm == pmap_kernel() && sva >= ktext &&
		    sva < roundup(ekdata, 4 * MEG)) {
			prom_printf("pmap_protect: va=%08x in locked TLB\n",
			    sva);
			prom_abort();
			return;
		}
#endif
		DPRINTF(PDB_CHANGEPROT, ("pmap_protect: va %p\n",
		    (void *)(u_long)sva));
		data = pseg_get(pm, sva);
		if ((data & TLB_V) == 0) {
			continue;
		}

		pa = data & TLB_PA_MASK;
		DPRINTF(PDB_CHANGEPROT|PDB_REF,
			("pmap_protect: va=%08x data=%08llx "
			 "seg=%08x pte=%08x\n",
			 (u_int)sva, (long long)pa, (int)va_to_seg(sva),
			 (int)va_to_pte(sva)));

		pg = PHYS_TO_VM_PAGE(pa);
		if (pg) {
			/* Save REF/MOD info */
			pv = &pg->mdpage.mdpg_pvh;
			if (data & TLB_ACCESS)
				pv->pv_va |= PV_REF;
			if (data & TLB_MODIFY)
				pv->pv_va |= PV_MOD;
		}

		/* Just do the pmap and TSB, not the pv_list */
		if ((prot & VM_PROT_WRITE) == 0)
			data &= ~(TLB_W|TLB_REAL_W);
		if ((prot & VM_PROT_EXECUTE) == 0)
			data &= ~(TLB_EXEC);

		rv = pseg_set(pm, sva, data, 0);
		if (rv & 1)
			panic("pmap_protect: pseg_set needs spare! rv=%d\n",
			    rv);

		if (!pm->pm_ctx && pm != pmap_kernel())
			continue;

		tsb_invalidate(pm->pm_ctx, sva);
		tlb_flush_pte(sva, pm->pm_ctx);
	}
	simple_unlock(&pm->pm_lock);
	pv_check();
}

/*
 * Extract the physical page address associated
 * with the given map/virtual_address pair.
 */
bool
pmap_extract(pm, va, pap)
	struct pmap *pm;
	vaddr_t va;
	paddr_t *pap;
{
	paddr_t pa;

	if (pm == pmap_kernel() && va >= kdata && va < roundup(ekdata, 4*MEG)) {
		/* Need to deal w/locked TLB entry specially. */
		pa = pmap_kextract(va);
		DPRINTF(PDB_EXTRACT, ("pmap_extract: va=%lx pa=%llx\n",
				      (u_long)va, (unsigned long long)pa));
	} else if (pm == pmap_kernel() && va >= ktext && va < ektext) {
		/* Need to deal w/locked TLB entry specially. */
		pa = pmap_kextract(va);
		DPRINTF(PDB_EXTRACT, ("pmap_extract: va=%lx pa=%llx\n",
		    (u_long)va, (unsigned long long)pa));
	} else if (pm == pmap_kernel() && va >= INTSTACK && va < (INTSTACK + 64*KB)) {
		pa = (paddr_t)(curcpu()->ci_paddr - INTSTACK + va);
		DPRINTF(PDB_EXTRACT, ("pmap_extract (intstack): va=%lx pa=%llx\n",
		    (u_long)va, (unsigned long long)pa));
		if (pap != NULL)
			*pap = pa;
		return TRUE;
	} else if (pm == pmap_kernel() && va >= KSTACK_VA && va < (KSTACK_VA + 64*KB)) {
		pa = (paddr_t)(curcpu()->ci_paddr - KSTACK_VA + va);
		DPRINTF(PDB_EXTRACT, ("pmap_extract (kstack): va=%lx pa=%llx\n",
		    (u_long)va, (unsigned long long)pa));
		if (pap != NULL)
			*pap = pa;
		return TRUE;
	} else {
		if (pm != pmap_kernel()) {
			simple_lock(&pm->pm_lock);
		}
		pa = pseg_get(pm, va) & TLB_PA_MASK;
#ifdef DEBUG
		if (pmapdebug & PDB_EXTRACT) {
			paddr_t npa = ldxa((vaddr_t)&pm->pm_segs[va_to_seg(va)],
					   ASI_PHYS_CACHED);
			printf("pmap_extract: va=%p segs[%ld]=%llx",
			       (void *)(u_long)va, (long)va_to_seg(va),
			       (unsigned long long)npa);
			if (npa) {
				npa = (paddr_t)
					ldxa((vaddr_t)&((paddr_t *)(u_long)npa)
					     [va_to_dir(va)],
					     ASI_PHYS_CACHED);
				printf(" segs[%ld][%ld]=%lx",
				       (long)va_to_seg(va),
				       (long)va_to_dir(va), (long)npa);
			}
			if (npa)	{
				npa = (paddr_t)
					ldxa((vaddr_t)&((paddr_t *)(u_long)npa)
					     [va_to_pte(va)],
					     ASI_PHYS_CACHED);
				printf(" segs[%ld][%ld][%ld]=%lx",
				       (long)va_to_seg(va),
				       (long)va_to_dir(va),
				       (long)va_to_pte(va), (long)npa);
			}
			printf(" pseg_get: %lx\n", (long)pa);
		}
#endif
		if (pm != pmap_kernel()) {
			simple_unlock(&pm->pm_lock);
		}
	}
	if (pa == 0)
		return (FALSE);
	if (pap != NULL)
		*pap = pa + (va & PGOFSET);
	return (TRUE);
}

/*
 * Change protection on a kernel address.
 * This should only be called from MD code.
 */
void
pmap_kprotect(va, prot)
	vaddr_t va;
	vm_prot_t prot;
{
	struct pmap *pm = pmap_kernel();
	int64_t data;
	int rv;

	simple_lock(&pm->pm_lock);
	data = pseg_get(pm, va);
	KASSERT(data & TLB_V);
	if (prot & VM_PROT_WRITE) {
		data |= (TLB_W|TLB_REAL_W);
	} else {
		data &= ~(TLB_W|TLB_REAL_W);
	}
	rv = pseg_set(pm, va, data, 0);
	if (rv & 1)
		panic("pmap_kprotect: pseg_set needs spare! rv=%d", rv);
	tsb_invalidate(pm->pm_ctx, va);
	tlb_flush_pte(va, pm->pm_ctx);
	simple_unlock(&pm->pm_lock);
}

/*
 * Return the number bytes that pmap_dumpmmu() will dump.
 */
int
pmap_dumpsize()
{
	int	sz;

	sz = ALIGN(sizeof(kcore_seg_t)) + ALIGN(sizeof(cpu_kcore_hdr_t));
	sz += phys_installed_size * sizeof(phys_ram_seg_t);

	return btodb(sz + DEV_BSIZE - 1);
}

/*
 * Write the mmu contents to the dump device.
 * This gets appended to the end of a crash dump since
 * there is no in-core copy of kernel memory mappings on a 4/4c machine.
 *
 * Write the core dump headers and MD data to the dump device.
 * We dump the following items:
 *
 *	kcore_seg_t		 MI header defined in <sys/kcore.h>)
 *	cpu_kcore_hdr_t		 MD header defined in <machine/kcore.h>)
 *	phys_ram_seg_t[phys_installed_size]  physical memory segments
 */
int
pmap_dumpmmu(int (*dump)(dev_t, daddr_t, void *, size_t), daddr_t blkno)
{
	kcore_seg_t	*kseg;
	cpu_kcore_hdr_t	*kcpu;
	phys_ram_seg_t	memseg;
	int	error = 0;
	int	i, memsegoffset;
	int	buffer[dbtob(1) / sizeof(int)];
	int	*bp, *ep;

#define EXPEDITE(p,n) do {						\
	int *sp = (void *)(p);						\
	int sz = (n);							\
	while (sz > 0) {						\
		*bp++ = *sp++;						\
		if (bp >= ep) {						\
			error = (*dump)(dumpdev, blkno,			\
					(void *)buffer, dbtob(1));	\
			if (error != 0)					\
				return (error);				\
			++blkno;					\
			bp = buffer;					\
		}							\
		sz -= 4;						\
	}								\
} while (0)

	/* Setup bookkeeping pointers */
	bp = buffer;
	ep = &buffer[sizeof(buffer) / sizeof(buffer[0])];

	/* Fill in MI segment header */
	kseg = (kcore_seg_t *)bp;
	CORE_SETMAGIC(*kseg, KCORE_MAGIC, MID_MACHINE, CORE_CPU);
	kseg->c_size = dbtob(pmap_dumpsize()) - ALIGN(sizeof(kcore_seg_t));

	/* Fill in MD segment header (interpreted by MD part of libkvm) */
	kcpu = (cpu_kcore_hdr_t *)((long)bp + ALIGN(sizeof(kcore_seg_t)));
	kcpu->cputype = CPU_SUN4U;
	kcpu->kernbase = (uint64_t)KERNBASE;
	kcpu->cpubase = (uint64_t)CPUINFO_VA;

	/* Describe the locked text segment */
	kcpu->ktextbase = (uint64_t)ktext;
	kcpu->ktextp = (uint64_t)ktextp;
	kcpu->ktextsz = (uint64_t)ektextp - ktextp;

	/* Describe locked data segment */
	kcpu->kdatabase = (uint64_t)kdata;
	kcpu->kdatap = (uint64_t)kdatap;
	kcpu->kdatasz = (uint64_t)ekdatap - kdatap;

	/* Now the memsegs */
	kcpu->nmemseg = phys_installed_size;
	kcpu->memsegoffset = memsegoffset = ALIGN(sizeof(cpu_kcore_hdr_t));

	/* Now we need to point this at our kernel pmap. */
	kcpu->nsegmap = STSZ;
	kcpu->segmapoffset = (uint64_t)pmap_kernel()->pm_physaddr;

	/* Note: we have assumed everything fits in buffer[] so far... */
	bp = (int *)((long)kcpu + ALIGN(sizeof(cpu_kcore_hdr_t)));

	for (i = 0; i < phys_installed_size; i++) {
		memseg.start = phys_installed[i].start;
		memseg.size = phys_installed[i].size;
		EXPEDITE(&memseg, sizeof(phys_ram_seg_t));
	}

	if (bp != buffer)
		error = (*dump)(dumpdev, blkno++, (void *)buffer, dbtob(1));

	return (error);
}

/*
 * Determine (non)existence of physical page
 */
int
pmap_pa_exists(paddr_t pa)
{
	int i;

	/* Just go through physical memory list & see if we're there */
	for (i = 0; i < phys_installed_size; i++) {
		if ((phys_installed[i].start <= pa) &&
				(phys_installed[i].start +
				 phys_installed[i].size >= pa))
			return 1;
	}
	return 0;
}

/*
 * Lookup the appropriate TSB entry.
 *
 * Here is the full official pseudo code:
 *
 */

#ifdef NOTYET
int64 GenerateTSBPointer(
 	int64 va,		/* Missing VA			*/
 	PointerType type,	/* 8K_POINTER or 16K_POINTER	*/
 	int64 TSBBase,		/* TSB Register[63:13] << 13	*/
 	Boolean split,		/* TSB Register[12]		*/
 	int TSBSize)		/* TSB Register[2:0]		*/
{
 	int64 vaPortion;
 	int64 TSBBaseMask;
 	int64 splitMask;

	/* TSBBaseMask marks the bits from TSB Base Reg		*/
	TSBBaseMask = 0xffffffffffffe000 <<
		(split? (TSBsize + 1) : TSBsize);

	/* Shift va towards lsb appropriately and		*/
	/* zero out the original va page offset			*/
	vaPortion = (va >> ((type == 8K_POINTER)? 9: 12)) &
		0xfffffffffffffff0;

	if (split) {
		/* There's only one bit in question for split	*/
		splitMask = 1 << (13 + TSBsize);
		if (type == 8K_POINTER)
			/* Make sure we're in the lower half	*/
			vaPortion &= ~splitMask;
		else
			/* Make sure we're in the upper half	*/
			vaPortion |= splitMask;
	}
	return (TSBBase & TSBBaseMask) | (vaPortion & ~TSBBaseMask);
}
#endif
/*
 * Of course, since we are not using a split TSB or variable page sizes,
 * we can optimize this a bit.
 *
 * The following only works for a unified 8K TSB.  It will find the slot
 * for that particular va and return it.  IT MAY BE FOR ANOTHER MAPPING!
 */
int
ptelookup_va(vaddr_t va)
{
	long tsbptr;
#define TSBBASEMASK	(0xffffffffffffe000LL << tsbsize)

	tsbptr = (((va >> 9) & 0xfffffffffffffff0LL) & ~TSBBASEMASK);
	return (tsbptr / sizeof(pte_t));
}

/*
 * Do whatever is needed to sync the MOD/REF flags
 */

bool
pmap_clear_modify(pg)
	struct vm_page *pg;
{
	pv_entry_t pv;
	int rv;
	int changed = 0;
#ifdef DEBUG
	int modified = 0;

	DPRINTF(PDB_CHANGEPROT|PDB_REF, ("pmap_clear_modify(%p)\n", pg));
#endif

#if defined(DEBUG)
	modified = pmap_is_modified(pg);
#endif
	/* Clear all mappings */
	pv = &pg->mdpage.mdpg_pvh;
#ifdef DEBUG
	if (pv->pv_va & PV_MOD)
		pv->pv_va |= PV_WE;	/* Remember this was modified */
#endif
	if (pv->pv_va & PV_MOD)
		changed |= 1;
	pv->pv_va &= ~(PV_MOD);
#ifdef DEBUG
	if (pv->pv_next && !pv->pv_pmap) {
		printf("pmap_clear_modify: npv but no pmap for pv %p\n", pv);
		Debugger();
	}
#endif
	if (pv->pv_pmap != NULL) {
		for (; pv; pv = pv->pv_next) {
			int64_t data;
			struct pmap *pmap = pv->pv_pmap;
			vaddr_t va = pv->pv_va & PV_VAMASK;

			simple_lock(&pmap->pm_lock);
			/* First clear the mod bit in the PTE and make it R/O */
			data = pseg_get(pmap, va);
			KASSERT(data & TLB_V);
			/* Need to both clear the modify and write bits */
			if (data & TLB_MODIFY)
				changed |= 1;
#ifdef HWREF
			data &= ~(TLB_MODIFY|TLB_W);
#else
			data &= ~(TLB_MODIFY|TLB_W|TLB_REAL_W);
#endif
			rv = pseg_set(pmap, va, data, 0);
			if (rv & 1)
				printf("pmap_clear_modify: pseg_set needs"
				    " spare! rv=%d\n", rv);
			if (pmap->pm_ctx || pmap == pmap_kernel()) {
				tsb_invalidate(pmap->pm_ctx, va);
				tlb_flush_pte(va, pmap->pm_ctx);
			}
			/* Then clear the mod bit in the pv */
			if (pv->pv_va & PV_MOD)
				changed |= 1;
			pv->pv_va &= ~(PV_MOD);
			simple_unlock(&pmap->pm_lock);
		}
	}
	pv_check();
#ifdef DEBUG
	if (pmap_is_modified(pg)) {
		printf("pmap_clear_modify(): %p still modified!\n", pg);
		Debugger();
	}
	DPRINTF(PDB_CHANGEPROT|PDB_REF, ("pmap_clear_modify: pg %p %s\n", pg,
	    (changed ? "was modified" : "was not modified")));
	if (modified != changed) {
		printf("pmap_clear_modify: modified %d changed %d\n",
		       modified, changed);
		Debugger();
	} else return (modified);
#endif
	return (changed);
}

bool
pmap_clear_reference(pg)
	struct vm_page *pg;
{
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	pv_entry_t pv;
	int rv;
	int changed = 0;
#ifdef DEBUG
	int referenced = 0;
#endif

#ifdef DEBUG
	DPRINTF(PDB_CHANGEPROT|PDB_REF, ("pmap_clear_reference(%p)\n", pg));
	referenced = pmap_is_referenced(pg);
#endif
	/* Clear all references */
	pv = &pg->mdpage.mdpg_pvh;
	if (pv->pv_va & PV_REF)
		changed |= 1;
	pv->pv_va &= ~(PV_REF);
#ifdef DEBUG
	if (pv->pv_next && !pv->pv_pmap) {
		printf("pmap_clear_reference: npv but no pmap for pv %p\n", pv);
		Debugger();
	}
#endif
	if (pv->pv_pmap != NULL) {
		for (; pv; pv = pv->pv_next) {
			int64_t data;
			struct pmap *pmap = pv->pv_pmap;
			vaddr_t va = pv->pv_va & PV_VAMASK;

			simple_lock(&pmap->pm_lock);
			data = pseg_get(pmap, va);
			KASSERT(data & TLB_V);
			DPRINTF(PDB_CHANGEPROT,
			    ("clearing ref pm:%p va:%p ctx:%lx data:%llx\n",
			     pmap, (void *)(u_long)va, (u_long)pmap->pm_ctx,
			     (long long)data));
#ifdef HWREF
			if (data & TLB_ACCESS)
				changed |= 1;
			data &= ~TLB_ACCESS;
#else
			if (data < 0)
				changed |= 1;
			data = 0;
#endif
			rv = pseg_set(pmap, va, data, 0);
			if (rv & 1)
				panic("pmap_clear_reference: pseg_set needs"
				    " spare! rv=%d\n", rv);
			if (pmap->pm_ctx || pmap == pmap_kernel()) {
				tsb_invalidate(pmap->pm_ctx, va);
				tlb_flush_pte(va, pmap->pm_ctx);
			}
			if (pv->pv_va & PV_REF)
				changed |= 1;
			pv->pv_va &= ~(PV_REF);
			simple_unlock(&pmap->pm_lock);
		}
	}
	dcache_flush_page(pa);
	pv_check();
#ifdef DEBUG
	if (pmap_is_referenced(pg)) {
		printf("pmap_clear_reference(): %p still referenced!\n", pg);
		Debugger();
	}
	DPRINTF(PDB_CHANGEPROT|PDB_REF,
	    ("pmap_clear_reference: pg %p %s\n", pg,
	     (changed ? "was referenced" : "was not referenced")));
	if (referenced != changed) {
		printf("pmap_clear_reference: referenced %d changed %d\n",
		       referenced, changed);
		Debugger();
	} else return (referenced);
#endif
	return (changed);
}

bool
pmap_is_modified(pg)
	struct vm_page *pg;
{
	pv_entry_t pv, npv;
	int i = 0;

	/* Check if any mapping has been modified */
	pv = &pg->mdpage.mdpg_pvh;
	if (pv->pv_va & PV_MOD)
		i = 1;
#ifdef HWREF
#ifdef DEBUG
	if (pv->pv_next && !pv->pv_pmap) {
		printf("pmap_is_modified: npv but no pmap for pv %p\n", pv);
		Debugger();
	}
#endif
	if (!i && pv->pv_pmap != NULL)
		for (npv = pv; i == 0 && npv && npv->pv_pmap;
		     npv = npv->pv_next) {
			int64_t data;

			data = pseg_get(npv->pv_pmap, npv->pv_va & PV_VAMASK);
			KASSERT(data & TLB_V);
			if (data & TLB_MODIFY)
				i = 1;

			/* Migrate modify info to head pv */
			if (npv->pv_va & PV_MOD)
				i = 1;
			npv->pv_va &= ~PV_MOD;
		}
	/* Save modify info */
	if (i)
		pv->pv_va |= PV_MOD;
#ifdef DEBUG
	if (i)
		pv->pv_va |= PV_WE;
#endif
#endif

	DPRINTF(PDB_CHANGEPROT|PDB_REF, ("pmap_is_modified(%p) = %d\n", pg, i));
	pv_check();
	return (i);
}

bool
pmap_is_referenced(pg)
	struct vm_page *pg;
{
	pv_entry_t pv, npv;
	int i = 0;

	/* Check if any mapping has been referenced */
	pv = &pg->mdpage.mdpg_pvh;
	if (pv->pv_va & PV_REF)
		i = 1;
#ifdef HWREF
#ifdef DEBUG
	if (pv->pv_next && !pv->pv_pmap) {
		printf("pmap_is_referenced: npv but no pmap for pv %p\n", pv);
		Debugger();
	}
#endif
	if (!i && (pv->pv_pmap != NULL))
		for (npv = pv; npv; npv = npv->pv_next) {
			int64_t data;

			data = pseg_get(npv->pv_pmap, npv->pv_va & PV_VAMASK);
			KASSERT(data & TLB_V);
			if (data & TLB_ACCESS)
				i = 1;

			/* Migrate ref info to head pv */
			if (npv->pv_va & PV_REF)
				i = 1;
			npv->pv_va &= ~PV_REF;
		}
	/* Save ref info */
	if (i)
		pv->pv_va |= PV_REF;
#endif

	DPRINTF(PDB_CHANGEPROT|PDB_REF,
		("pmap_is_referenced(%p) = %d\n", pg, i));
	pv_check();
	return i;
}



/*
 *	Routine:	pmap_unwire
 *	Function:	Clear the wired attribute for a map/virtual-address
 *			pair.
 *	In/out conditions:
 *			The mapping must already exist in the pmap.
 */
void
pmap_unwire(pmap, va)
	pmap_t	pmap;
	vaddr_t va;
{
	int64_t data;
	int rv;

	DPRINTF(PDB_MMU_STEAL, ("pmap_unwire(%p, %lx)\n", pmap, va));

#ifdef DEBUG
	/*
	 * Is this part of the permanent 4MB mapping?
	 */
	if (pmap == pmap_kernel() && va >= ktext &&
		va < roundup(ekdata, 4*MEG)) {
		prom_printf("pmap_unwire: va=%08x in locked TLB\n", va);
		prom_abort();
		return;
	}
#endif
	simple_lock(&pmap->pm_lock);
	data = pseg_get(pmap, va & PV_VAMASK);
	KASSERT(data & TLB_V);
	data &= ~TLB_TSB_LOCK;
	rv = pseg_set(pmap, va & PV_VAMASK, data, 0);
	if (rv & 1)
		panic("pmap_unwire: pseg_set needs spare! rv=%d\n", rv);
	simple_unlock(&pmap->pm_lock);
	pv_check();
}

/*
 * Lower the protection on the specified physical page.
 *
 * Never enable writing as it will break COW
 */

void
pmap_page_protect(pg, prot)
	struct vm_page *pg;
	vm_prot_t prot;
{
	int64_t clear, set;
	int64_t data = 0;
	int rv;
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	pv_entry_t pv, npv, firstpv;
	struct pmap *pmap;
	vaddr_t va;
	bool needflush = FALSE;

	DPRINTF(PDB_CHANGEPROT,
	    ("pmap_page_protect: pg %p prot %x\n", pg, prot));

	pv = &pg->mdpage.mdpg_pvh;
	if (prot & (VM_PROT_READ|VM_PROT_EXECUTE)) {
		/* copy_on_write */

		set = TLB_V;
		clear = TLB_REAL_W|TLB_W;
		if (VM_PROT_EXECUTE & prot)
			set |= TLB_EXEC;
		else
			clear |= TLB_EXEC;
		if (VM_PROT_EXECUTE == prot)
			set |= TLB_EXEC_ONLY;

#ifdef DEBUG
		if (pv->pv_next && !pv->pv_pmap) {
			printf("pmap_page_protect: no pmap for pv %p\n", pv);
			Debugger();
		}
#endif
		if (pv->pv_pmap != NULL) {
			for (; pv; pv = pv->pv_next) {
				pmap = pv->pv_pmap;
				va = pv->pv_va & PV_VAMASK;

				simple_lock(&pmap->pm_lock);
				DPRINTF(PDB_CHANGEPROT | PDB_REF,
					("pmap_page_protect: "
					 "RO va %p of pg %p...\n",
					 (void *)(u_long)pv->pv_va, pg));
				data = pseg_get(pmap, va);
				KASSERT(data & TLB_V);

				/* Save REF/MOD info */
				if (data & TLB_ACCESS)
					pv->pv_va |= PV_REF;
				if (data & TLB_MODIFY)
					pv->pv_va |= PV_MOD;

				data &= ~clear;
				data |= set;
				rv = pseg_set(pmap, va, data, 0);
				if (rv & 1)
					panic("pmap_page_protect: "
					       "pseg_set needs spare! rv=%d\n",
					       rv);
				if (pmap->pm_ctx || pmap == pmap_kernel()) {
					tsb_invalidate(pmap->pm_ctx, va);
					tlb_flush_pte(va, pmap->pm_ctx);
				}
				simple_unlock(&pmap->pm_lock);
			}
		}
	} else {
		/* remove mappings */
		DPRINTF(PDB_REMOVE,
			("pmap_page_protect: demapping pg %p\n", pg));

		firstpv = pv;

		/* First remove the entire list of continuation pv's */
		for (npv = pv->pv_next; npv; npv = pv->pv_next) {
			pmap = npv->pv_pmap;
			va = npv->pv_va & PV_VAMASK;

			/* We're removing npv from pv->pv_next */
			simple_lock(&pmap->pm_lock);
			DPRINTF(PDB_CHANGEPROT|PDB_REF|PDB_REMOVE,
				("pmap_page_protect: "
				 "demap va %p of pg %p in pmap %p...\n",
				 (void *)(u_long)va, pg, pmap));

			/* clear the entry in the page table */
			data = pseg_get(pmap, va);
			KASSERT(data & TLB_V);

			/* Save ref/mod info */
			if (data & TLB_ACCESS)
				firstpv->pv_va |= PV_REF;
			if (data & TLB_MODIFY)
				firstpv->pv_va |= PV_MOD;
			/* Clear mapping */
			rv = pseg_set(pmap, va, 0, 0);
			if (rv & 1)
				panic("pmap_page_protect: pseg_set needs"
				     " spare! rv=%d\n", rv);
			if (pmap->pm_ctx || pmap == pmap_kernel()) {
				tsb_invalidate(pmap->pm_ctx, va);
				tlb_flush_pte(va, pmap->pm_ctx);
			}
			if (pmap->pm_refs > 0) {
				needflush = TRUE;
			}
			simple_unlock(&pmap->pm_lock);

			/* free the pv */
			pv->pv_next = npv->pv_next;
			pool_put(&pmap_pv_pool, npv);
		}

		pv = firstpv;

		/* Then remove the primary pv */
#ifdef DEBUG
		if (pv->pv_next && !pv->pv_pmap) {
			printf("pmap_page_protect: no pmap for pv %p\n", pv);
			Debugger();
		}
#endif
		if (pv->pv_pmap != NULL) {
			pmap = pv->pv_pmap;
			va = pv->pv_va & PV_VAMASK;

			simple_lock(&pmap->pm_lock);
			DPRINTF(PDB_CHANGEPROT|PDB_REF|PDB_REMOVE,
				("pmap_page_protect: "
				 "demap va %p of pg %p from pm %p...\n",
				 (void *)(u_long)va, pg, pmap));

			data = pseg_get(pmap, va);
			KASSERT(data & TLB_V);
			/* Save ref/mod info */
			if (data & TLB_ACCESS)
				pv->pv_va |= PV_REF;
			if (data & TLB_MODIFY)
				pv->pv_va |= PV_MOD;
			rv = pseg_set(pmap, va, 0, 0);
			if (rv & 1)
				panic("pmap_page_protect: pseg_set needs"
				    " spare! rv=%d\n", rv);
			if (pv->pv_pmap->pm_ctx ||
			    pv->pv_pmap == pmap_kernel()) {
				tsb_invalidate(pmap->pm_ctx, va);
				tlb_flush_pte(va, pmap->pm_ctx);
			}
			if (pmap->pm_refs > 0) {
				needflush = TRUE;
			}
			simple_unlock(&pmap->pm_lock);
			npv = pv->pv_next;
			/* dump the first pv */
			if (npv) {
				/* First save mod/ref bits */
				pv->pv_pmap = npv->pv_pmap;
				pv->pv_va |= npv->pv_va & PV_MASK;
				pv->pv_next = npv->pv_next;
				pool_put(&pmap_pv_pool, npv);
			} else {
				pv->pv_pmap = NULL;
				pv->pv_next = NULL;
			}
		}
		if (needflush) {
			dcache_flush_page(pa);
		}
	}
	/* We should really only flush the pages we demapped. */
	pv_check();
}

#ifdef PMAP_COUNT_DEBUG
/*
 * count pages in pmap -- this can be slow.
 */
int
pmap_count_res(struct pmap *pm)
{
	int64_t data;
	paddr_t *pdir, *ptbl;
	int i, j, k, n;

	/* Almost the same as pmap_collect() */
	/* Don't want one of these pages reused while we're reading it. */
	simple_lock(&pm->pm_lock);
	n = 0;
	for (i = 0; i < STSZ; i++) {
		pdir = (paddr_t *)(u_long)ldxa((vaddr_t)&pm->pm_segs[i],
					       ASI_PHYS_CACHED);
		if (pdir == NULL) {
			continue;
		}
		for (k = 0; k < PDSZ; k++) {
			ptbl = (paddr_t *)(u_long)ldxa((vaddr_t)&pdir[k],
						       ASI_PHYS_CACHED);
			if (ptbl == NULL) {
				continue;
			}
			for (j = 0; j < PTSZ; j++) {
				data = (int64_t)ldxa((vaddr_t)&ptbl[j],
						     ASI_PHYS_CACHED);
				if (data & TLB_V)
					n++;
			}
		}
	}
	simple_unlock(&pm->pm_lock);

	if (pm->pm_stats.resident_count != n)
		printf("pmap_count_resident: pm_stats = %ld, counted: %d\n",
		    pm->pm_stats.resident_count, n);

	return n;
}

/*
 * count wired pages in pmap -- this can be slow.
 */
int
pmap_count_wired(struct pmap *pm)
{
	int64_t data;
	paddr_t *pdir, *ptbl;
	int i, j, k, n;

	/* Almost the same as pmap_collect() */
	/* Don't want one of these pages reused while we're reading it. */
	simple_lock(&pm->pm_lock);
	n = 0;
	for (i = 0; i < STSZ; i++) {
		pdir = (paddr_t *)(u_long)ldxa((vaddr_t)&pm->pm_segs[i],
					       ASI_PHYS_CACHED);
		if (pdir == NULL) {
			continue;
		}
		for (k = 0; k < PDSZ; k++) {
			ptbl = (paddr_t *)(u_long)ldxa((vaddr_t)&pdir[k],
						       ASI_PHYS_CACHED);
			if (ptbl == NULL) {
				continue;
			}
			for (j = 0; j < PTSZ; j++) {
				data = (int64_t)ldxa((vaddr_t)&ptbl[j],
						     ASI_PHYS_CACHED);
				if (data & TLB_TSB_LOCK)
					n++;
			}
		}
	}
	simple_unlock(&pm->pm_lock);

	if (pm->pm_stats.wired_count != n)
		printf("pmap_count_wired: pm_stats = %ld, counted: %d\n",
		    pm->pm_stats.wired_count, n);


	return n;
}
#endif	/* PMAP_COUNT_DEBUG */

void
pmap_procwr(struct proc *p, vaddr_t va, size_t len)
{

	blast_icache();
}

/*
 * Allocate a hardware context to the given pmap.
 */
int
ctx_alloc(struct pmap *pm)
{
	int i, ctx;

	KASSERT(pm != pmap_kernel());
	KASSERT(pm == curproc->p_vmspace->vm_map.pmap);
	simple_lock(&pm->pm_lock);
	ctx = pmap_next_ctx++;

	/*
	 * if we have run out of contexts, remove all user entries from
	 * the TSB, TLB and dcache and start over with context 1 again.
	 */

	if (ctx == numctx) {
		write_user_windows();
		while (!LIST_EMPTY(&pmap_ctxlist)) {
			ctx_free(LIST_FIRST(&pmap_ctxlist));
		}
		for (i = TSBENTS - 1; i >= 0; i--) {
			if (TSB_TAG_CTX(tsb_dmmu[i].tag) != 0) {
				clrx(&tsb_dmmu[i].data);
			}
			if (TSB_TAG_CTX(tsb_immu[i].tag) != 0) {
				clrx(&tsb_immu[i].data);
			}
		}
		tlb_flush_all();
		ctx = 1;
		pmap_next_ctx = 2;
	}
	ctxbusy[ctx] = pm->pm_physaddr;
	LIST_INSERT_HEAD(&pmap_ctxlist, pm, pm_list);
	pm->pm_ctx = ctx;
	simple_unlock(&pm->pm_lock);
	DPRINTF(PDB_CTX_ALLOC, ("ctx_alloc: allocated ctx %d\n", ctx));
	return ctx;
}

/*
 * Give away a context.
 */
void
ctx_free(struct pmap *pm)
{
	int oldctx;

	oldctx = pm->pm_ctx;
	if (oldctx == 0) {
		return;
	}

#ifdef DIAGNOSTIC
	if (pm == pmap_kernel())
		panic("ctx_free: freeing kernel context");
	if (ctxbusy[oldctx] == 0)
		printf("ctx_free: freeing free context %d\n", oldctx);
	if (ctxbusy[oldctx] != pm->pm_physaddr) {
		printf("ctx_free: freeing someone else's context\n "
		       "ctxbusy[%d] = %p, pm(%p)->pm_ctx = %p\n",
		       oldctx, (void *)(u_long)ctxbusy[oldctx], pm,
		       (void *)(u_long)pm->pm_physaddr);
		Debugger();
	}
#endif
	/* We should verify it has not been stolen and reallocated... */
	DPRINTF(PDB_CTX_ALLOC, ("ctx_free: freeing ctx %d\n", oldctx));
	ctxbusy[oldctx] = 0UL;
	pm->pm_ctx = 0;
	LIST_REMOVE(pm, pm_list);
}

/*
 * Enter the pmap and virtual address into the
 * physical to virtual map table.
 *
 * We enter here with the pmap locked.
 */

void
pmap_enter_pv(struct pmap *pmap, vaddr_t va, paddr_t pa, struct vm_page *pg,
	      pv_entry_t npv)
{
	pv_entry_t pvh;

	pvh = &pg->mdpage.mdpg_pvh;
	DPRINTF(PDB_ENTER, ("pmap_enter: pvh %p: was %lx/%p/%p\n",
	    pvh, pvh->pv_va, pvh->pv_pmap, pvh->pv_next));
	if (pvh->pv_pmap == NULL) {

		/*
		 * No entries yet, use header as the first entry
		 */
		DPRINTF(PDB_ENTER, ("pmap_enter: first pv: pmap %p va %lx\n",
		    pmap, va));
		ENTER_STAT(firstpv);
		PV_SETVA(pvh, va);
		pvh->pv_pmap = pmap;
		pvh->pv_next = NULL;
		KASSERT(npv == NULL);
	} else {
		if (pg->loan_count == 0 && !(pvh->pv_va & PV_ALIAS)) {

			/*
			 * There is at least one other VA mapping this page.
			 * Check if they are cache index compatible. If not
			 * remove all mappings, flush the cache and set page
			 * to be mapped uncached. Caching will be restored
			 * when pages are mapped compatible again.
			 */
			if ((pvh->pv_va ^ va) & VA_ALIAS_MASK) {
				pvh->pv_va |= PV_ALIAS;
				pmap_page_cache(pmap, pa, 0);
				ENTER_STAT(ci);
			}
		}

		/*
		 * There is at least one other VA mapping this page.
		 * Place this entry after the header.
		 */

		DPRINTF(PDB_ENTER, ("pmap_enter: new pv: pmap %p va %lx\n",
		    pmap, va));
		npv->pv_pmap = pmap;
		npv->pv_va = va & PV_VAMASK;
		npv->pv_next = pvh->pv_next;
		pvh->pv_next = npv;

		if (!npv->pv_next) {
			ENTER_STAT(secondpv);
		}
	}
}

/*
 * Remove a physical to virtual address translation.
 */

pv_entry_t
pmap_remove_pv(struct pmap *pmap, vaddr_t va, struct vm_page *pg)
{
	pv_entry_t pvh, npv, pv;
	int64_t data = 0;

	pvh = &pg->mdpage.mdpg_pvh;

	DPRINTF(PDB_REMOVE, ("pmap_remove_pv(pm=%p, va=%p, pg=%p)\n", pmap,
	    (void *)(u_long)va, pg));
	pv_check();

	/*
	 * Remove page from the PV table.
	 * If it is the first entry on the list, it is actually
	 * in the header and we must copy the following entry up
	 * to the header.  Otherwise we must search the list for
	 * the entry.  In either case we free the now unused entry.
	 */
	if (pmap == pvh->pv_pmap && PV_MATCH(pvh, va)) {
		data = pseg_get(pvh->pv_pmap, pvh->pv_va & PV_VAMASK);
		KASSERT(data & TLB_V);
		npv = pvh->pv_next;
		if (npv) {
			/* First save mod/ref bits */
			pvh->pv_va = (pvh->pv_va & PV_MASK) | npv->pv_va;
			pvh->pv_next = npv->pv_next;
			pvh->pv_pmap = npv->pv_pmap;
		} else {
			pvh->pv_pmap = NULL;
			pvh->pv_next = NULL;
			pvh->pv_va &= (PV_REF|PV_MOD);
		}
		REMOVE_STAT(pvfirst);
	} else {
		for (pv = pvh, npv = pvh->pv_next; npv;
		     pv = npv, npv = npv->pv_next) {
			REMOVE_STAT(pvsearch);
			if (pmap == npv->pv_pmap && PV_MATCH(npv, va))
				break;
		}
		pv->pv_next = npv->pv_next;
		data = pseg_get(npv->pv_pmap, npv->pv_va & PV_VAMASK);
		KASSERT(data & TLB_V);
	}

	/* Save ref/mod info */
	if (data & TLB_ACCESS)
		pvh->pv_va |= PV_REF;
	if (data & TLB_MODIFY)
		pvh->pv_va |= PV_MOD;

	/* Check to see if the alias went away */
	if (pvh->pv_va & PV_ALIAS) {
		pvh->pv_va &= ~PV_ALIAS;
		for (pv = pvh; pv; pv = pv->pv_next) {
			if ((pv->pv_va ^ pvh->pv_va) & VA_ALIAS_MASK) {
				pvh->pv_va |= PV_ALIAS;
				break;
			}
		}
		if (!(pvh->pv_va & PV_ALIAS))
			pmap_page_cache(pmap, VM_PAGE_TO_PHYS(pg), 1);
	}
	pv_check();
	return npv;
}

/*
 *	pmap_page_cache:
 *
 *	Change all mappings of a page to cached/uncached.
 */
void
pmap_page_cache(struct pmap *pm, paddr_t pa, int mode)
{
	struct vm_page *pg;
	pv_entry_t pv;
	vaddr_t va;
	int rv;

	DPRINTF(PDB_ENTER, ("pmap_page_uncache(%llx)\n",
	    (unsigned long long)pa));
	pg = PHYS_TO_VM_PAGE(pa);
	pv = &pg->mdpage.mdpg_pvh;
	while (pv) {
		va = pv->pv_va & PV_VAMASK;
		if (pv->pv_pmap != pm)
			simple_lock(&pv->pv_pmap->pm_lock);
		if (pv->pv_va & PV_NC) {
			int64_t data;

			/* Non-cached -- I/O mapping */
			data = pseg_get(pv->pv_pmap, va);
			KASSERT(data & TLB_V);
			rv = pseg_set(pv->pv_pmap, va,
				     data & ~(TLB_CV|TLB_CP), 0);
			if (rv & 1)
				panic("pmap_page_cache: pseg_set needs"
				     " spare! rv=%d\n", rv);
		} else if (mode && (!(pv->pv_va & PV_NVC))) {
			int64_t data;

			/* Enable caching */
			data = pseg_get(pv->pv_pmap, va);
			KASSERT(data & TLB_V);
			rv = pseg_set(pv->pv_pmap, va, data | TLB_CV, 0);
			if (rv & 1)
				panic("pmap_page_cache: pseg_set needs"
				    " spare! rv=%d\n", rv);
		} else {
			int64_t data;

			/* Disable caching */
			data = pseg_get(pv->pv_pmap, va);
			rv = pseg_set(pv->pv_pmap, va, data & ~TLB_CV, 0);
			if (rv & 1)
				panic("pmap_page_cache: pseg_set needs"
				    " spare! rv=%d\n", rv);
		}
		if (pv->pv_pmap != pm)
			simple_unlock(&pv->pv_pmap->pm_lock);
		if (pv->pv_pmap->pm_ctx || pv->pv_pmap == pmap_kernel()) {
			/* Force reload -- cache bits have changed */
			tsb_invalidate(pv->pv_pmap->pm_ctx, va);
			tlb_flush_pte(va, pv->pv_pmap->pm_ctx);
		}
		pv = pv->pv_next;
	}
}


static int
pmap_get_page(paddr_t *p)
{
	struct vm_page *pg;
	paddr_t pa;

	if (uvm.page_init_done) {
		pg = uvm_pagealloc(NULL, 0, NULL,
		    UVM_PGA_ZERO | UVM_PGA_USERESERVE);
		if (pg == NULL)
			return (0);
		pa = VM_PAGE_TO_PHYS(pg);
	} else {
		if (!uvm_page_physget(&pa))
			return (0);
		pmap_zero_page(pa);
	}
	*p = pa;
	return (1);
}

static void
pmap_free_page(paddr_t pa)
{
	struct vm_page *pg = PHYS_TO_VM_PAGE(pa);

	uvm_pagefree(pg);
}


#ifdef DDB

void db_dump_pv(db_expr_t, int, db_expr_t, const char *);
void
db_dump_pv(db_expr_t addr, int have_addr, db_expr_t count, const char *modif)
{
	struct vm_page *pg;
	struct pv_entry *pv;

	if (!have_addr) {
		db_printf("Need addr for pv\n");
		return;
	}

	pg = PHYS_TO_VM_PAGE((paddr_t)addr);
	if (pg == NULL) {
		db_printf("page is not managed\n");
		return;
	}
	for (pv = &pg->mdpage.mdpg_pvh; pv; pv = pv->pv_next)
		db_printf("pv@%p: next=%p pmap=%p va=0x%llx\n",
			  pv, pv->pv_next, pv->pv_pmap,
			  (unsigned long long)pv->pv_va);
}

#endif

#ifdef DEBUG
/*
 * Test ref/modify handling.  */
void pmap_testout(void);
void
pmap_testout()
{
	vaddr_t va;
	volatile int *loc;
	int val = 0;
	paddr_t pa;
	struct vm_page *pg;
	int ref, mod;

	/* Allocate a page */
	va = (vaddr_t)(vmmap - PAGE_SIZE);
	KASSERT(va != 0);
	loc = (int*)va;

	pmap_get_page(&pa);
	pg = PHYS_TO_VM_PAGE(pa);
	pmap_enter(pmap_kernel(), va, pa, VM_PROT_ALL, VM_PROT_ALL);
	pmap_update(pmap_kernel());

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa,
	       ref, mod);

	/* Check it's properly cleared */
	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Checking cleared page: ref %d, mod %d\n",
	       ref, mod);

	/* Reference page */
	val = *loc;

	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Referenced page: ref %d, mod %d val %x\n",
	       ref, mod, val);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa,
	       ref, mod);

	/* Modify page */
	*loc = 1;

	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Modified page: ref %d, mod %d\n",
	       ref, mod);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa,
	       ref, mod);

	/* Check it's properly cleared */
	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Checking cleared page: ref %d, mod %d\n",
	       ref, mod);

	/* Modify page */
	*loc = 1;

	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Modified page: ref %d, mod %d\n",
	       ref, mod);

	/* Check pmap_protect() */
	pmap_protect(pmap_kernel(), va, va+1, VM_PROT_READ);
	pmap_update(pmap_kernel());
	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("pmap_protect(VM_PROT_READ): ref %d, mod %d\n",
	       ref, mod);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa,
	       ref, mod);

	/* Modify page */
	pmap_enter(pmap_kernel(), va, pa, VM_PROT_ALL, VM_PROT_ALL);
	pmap_update(pmap_kernel());
	*loc = 1;

	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Modified page: ref %d, mod %d\n",
	       ref, mod);

	/* Check pmap_protect() */
	pmap_protect(pmap_kernel(), va, va+1, VM_PROT_NONE);
	pmap_update(pmap_kernel());
	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("pmap_protect(VM_PROT_READ): ref %d, mod %d\n",
	       ref, mod);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa,
	       ref, mod);

	/* Modify page */
	pmap_enter(pmap_kernel(), va, pa, VM_PROT_ALL, VM_PROT_ALL);
	pmap_update(pmap_kernel());
	*loc = 1;

	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Modified page: ref %d, mod %d\n",
	       ref, mod);

	/* Check pmap_pag_protect() */
	pmap_page_protect(pg, VM_PROT_READ);
	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("pmap_protect(): ref %d, mod %d\n",
	       ref, mod);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa,
	       ref, mod);


	/* Modify page */
	pmap_enter(pmap_kernel(), va, pa, VM_PROT_ALL, VM_PROT_ALL);
	pmap_update(pmap_kernel());
	*loc = 1;

	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Modified page: ref %d, mod %d\n",
	       ref, mod);

	/* Check pmap_pag_protect() */
	pmap_page_protect(pg, VM_PROT_NONE);
	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("pmap_protect(): ref %d, mod %d\n",
	       ref, mod);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa,
	       ref, mod);

	/* Unmap page */
	pmap_remove(pmap_kernel(), va, va+1);
	pmap_update(pmap_kernel());
	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Unmapped page: ref %d, mod %d\n", ref, mod);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa, ref, mod);

	/* Check it's properly cleared */
	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Checking cleared page: ref %d, mod %d\n",
	       ref, mod);

	pmap_remove(pmap_kernel(), va, va+1);
	pmap_update(pmap_kernel());
	pmap_free_page(pa);
}
#endif

void
pmap_update(struct pmap *pmap)
{

#ifdef MULTIPROCESSOR
	smp_tlb_flush_all();	/* XXX */
#endif

	if (pmap->pm_refs > 0) {
		return;
	}
	pmap->pm_refs = 1;
	pmap_activate_pmap(pmap);
}
