/*	$NetBSD: pmap.c,v 1.275.2.2 2012/04/17 00:06:56 yamt Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.275.2.2 2012/04/17 00:06:56 yamt Exp $");

#undef	NO_VCACHE /* Don't forget the locked TLB in dostart */
#define	HWREF

#include "opt_ddb.h"
#include "opt_multiprocessor.h"
#include "opt_modular.h"

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/systm.h>
#include <sys/msgbuf.h>
#include <sys/pool.h>
#include <sys/exec.h>
#include <sys/core.h>
#include <sys/kcore.h>
#include <sys/proc.h>
#include <sys/atomic.h>
#include <sys/cpu.h>

#include <sys/exec_aout.h>	/* for MID_* */

#include <uvm/uvm.h>

#include <machine/pcb.h>
#include <machine/sparc64.h>
#include <machine/ctlreg.h>
#include <machine/promlib.h>
#include <machine/kcore.h>
#include <machine/bootinfo.h>

#include <sparc64/sparc64/cache.h>

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
extern int64_t pseg_get_real(struct pmap *, vaddr_t);
extern int pseg_set_real(struct pmap *, vaddr_t, int64_t, paddr_t);

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

struct pool_cache pmap_cache;
struct pool_cache pmap_pv_cache;

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
int tsbsize;		/* tsbents = 512 * 2^^tsbsize */
#define TSBENTS (512<<tsbsize)
#define	TSBSIZE	(TSBENTS * 16)

static struct pmap kernel_pmap_;
struct pmap *const kernel_pmap_ptr = &kernel_pmap_;

static int ctx_alloc(struct pmap *);
static bool pmap_is_referenced_locked(struct vm_page *);

static void ctx_free(struct pmap *, struct cpu_info *);

/*
 * Check if any MMU has a non-zero context
 */
static inline bool
pmap_has_ctx(struct pmap *p)
{
	int i;

	/* any context on any cpu? */
	for (i = 0; i < sparc_ncpus; i++)
		if (p->pm_ctx[i] > 0)
			return true;

	return false;	
}

#ifdef MULTIPROCESSOR
#define pmap_ctx(PM)	((PM)->pm_ctx[cpu_number()])
#else
#define pmap_ctx(PM)	((PM)->pm_ctx[0])
#endif

/*
 * Check if this pmap has a live mapping on some MMU.
 */
static inline bool
pmap_is_on_mmu(struct pmap *p)
{
	/* The kernel pmap is always on all MMUs */
	if (p == pmap_kernel())
		return true;

	return pmap_has_ctx(p);
}

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

static void
tsb_invalidate(vaddr_t va, pmap_t pm)
{
	struct cpu_info *ci;
	int ctx;
	bool kpm = (pm == pmap_kernel());
	int i;
	int64_t tag;

	i = ptelookup_va(va);
#ifdef MULTIPROCESSOR
	for (ci = cpus; ci != NULL; ci = ci->ci_next) {
		if (!CPUSET_HAS(cpus_active, ci->ci_index))
			continue;
#else
		ci = curcpu();
#endif
		ctx = pm->pm_ctx[ci->ci_index];
		if (kpm || ctx > 0) {
			tag = TSB_TAG(0, ctx, va);
			if (ci->ci_tsb_dmmu[i].tag == tag) {
				clrx(&ci->ci_tsb_dmmu[i].data);
			}
			if (ci->ci_tsb_immu[i].tag == tag) {
				clrx(&ci->ci_tsb_immu[i].data);
			}
		}
#ifdef MULTIPROCESSOR
	}
#endif
}

struct prom_map *prom_map;
int prom_map_size;

#define	PDB_CREATE		0x000001
#define	PDB_DESTROY		0x000002
#define	PDB_REMOVE		0x000004
#define	PDB_CHANGEPROT		0x000008
#define	PDB_ENTER		0x000010
#define	PDB_DEMAP		0x000020	/* used in locore */
#define	PDB_REF			0x000040
#define	PDB_COPY		0x000080
#define	PDB_MMU_ALLOC		0x000100
#define	PDB_MMU_STEAL		0x000200
#define	PDB_CTX_ALLOC		0x000400
#define	PDB_CTX_STEAL		0x000800
#define	PDB_MMUREG_ALLOC	0x001000
#define	PDB_MMUREG_STEAL	0x002000
#define	PDB_CACHESTUFF		0x004000
#define	PDB_ALIAS		0x008000
#define PDB_EXTRACT		0x010000
#define	PDB_BOOT		0x020000
#define	PDB_BOOT1		0x040000
#define	PDB_GROW		0x080000
#define	PDB_CTX_FLUSHALL	0x100000
#define	PDB_ACTIVATE		0x200000

#if defined(DEBUG) && !defined(PMAP_DEBUG)
#define PMAP_DEBUG
#endif

#ifdef PMAP_DEBUG
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
#define	ENTER_STAT(x)	do { enter_stats.x ++; } while (0)
#define	REMOVE_STAT(x)	do { remove_stats.x ++; } while (0)

int	pmapdebug = 0;
//int	pmapdebug = 0 | PDB_CTX_ALLOC | PDB_ACTIVATE;
/* Number of H/W pages stolen for page tables */
int	pmap_pages_stolen = 0;

#define	BDPRINTF(n, f)	if (pmapdebug & (n)) prom_printf f
#define	DPRINTF(n, f)	if (pmapdebug & (n)) printf f
#else
#define	ENTER_STAT(x)	do { /* nothing */ } while (0)
#define	REMOVE_STAT(x)	do { /* nothing */ } while (0)
#define	BDPRINTF(n, f)
#define	DPRINTF(n, f)
#define pmapdebug 0
#endif

#define pv_check()

static int pmap_get_page(paddr_t *);
static void pmap_free_page(paddr_t, sparc64_cpuset_t);
static void pmap_free_page_noflush(paddr_t);

/*
 * Global pmap locks.
 */
static kmutex_t pmap_lock;
static bool lock_available = false;

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
 * This probably shouldn't be necessary, but it stops USIII machines from
 * breaking in general, and not just for MULTIPROCESSOR.
 */
#define USE_LOCKSAFE_PSEG_GETSET
#if defined(USE_LOCKSAFE_PSEG_GETSET)

static kmutex_t pseg_lock;

static __inline__ int64_t
pseg_get_locksafe(struct pmap *pm, vaddr_t va)
{
	int64_t rv;
	bool took_lock = lock_available /*&& pm == pmap_kernel()*/;

	if (__predict_true(took_lock))
		mutex_enter(&pseg_lock);
	rv = pseg_get_real(pm, va);
	if (__predict_true(took_lock))
		mutex_exit(&pseg_lock);
	return rv;
}

static __inline__ int
pseg_set_locksafe(struct pmap *pm, vaddr_t va, int64_t data, paddr_t ptp)
{
	int rv;
	bool took_lock = lock_available /*&& pm == pmap_kernel()*/;

	if (__predict_true(took_lock))
		mutex_enter(&pseg_lock);
	rv = pseg_set_real(pm, va, data, ptp);
	if (__predict_true(took_lock))
		mutex_exit(&pseg_lock);
	return rv;
}

#define pseg_get(pm, va)		pseg_get_locksafe(pm, va)
#define pseg_set(pm, va, data, ptp)	pseg_set_locksafe(pm, va, data, ptp)

#else /* USE_LOCKSAFE_PSEG_GETSET */

#define pseg_get(pm, va)		pseg_get_real(pm, va)
#define pseg_set(pm, va, data, ptp)	pseg_set_real(pm, va, data, ptp)

#endif /* USE_LOCKSAFE_PSEG_GETSET */

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
		if (pmapdebug & PDB_BOOT1)
		{int i; for (i=0; i<140000000; i++) ;}
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
#else
#define pmap_bootdebug()	/* nothing */
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
	char *v;

	v = OF_claim(NULL, 2*PAGE_SIZE, PAGE_SIZE);
	if ((v == NULL) || (v == (void*)-1))
		panic("Can't claim two pages of memory.");

	memset(v, 0, 2*PAGE_SIZE);

	cpu_args = (struct cpu_bootargs*)v;
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

	memcpy(v, mp_tramp_code, mp_tramp_code_len);
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
		DPRINTF(PDB_BOOT1, ("xtlb[%d]: Tag: %" PRIx64 " Data: %"
				PRIx64 "\n", i, tp[i].tag, tp[i].data));
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
#ifdef MODULAR
	extern vaddr_t module_start, module_end;
#endif
	extern char etext[], data_start[];	/* start of data segment */
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

	BDPRINTF(PDB_BOOT, ("Entered pmap_bootstrap.\n"));

	cache_setup_funcs();

	/*
	 * Calculate kernel size.
	 */
	ktext   = kernelstart;
	ktextp  = pmap_kextract(ktext);
	ektext  = roundup((vaddr_t)etext, PAGE_SIZE_4M);
	ektextp = roundup(pmap_kextract((vaddr_t)etext), PAGE_SIZE_4M);

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

	pmap_bootdebug();
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

	if (pmapdebug & PDB_BOOT1) {
		/* print out mem list */
		prom_printf("Installed physical memory:\n");
		for (i = 0; i < phys_installed_size; i++) {
			prom_printf("memlist start %lx size %lx\n",
					(u_long)phys_installed[i].start,
					(u_long)phys_installed[i].size);
		}
	}

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

	/*
	 * Allocate a ncpu*64KB page for the cpu_info & stack structure now.
	 */
	cpu0paddr = prom_alloc_phys(8 * PAGE_SIZE * sparc_ncpus, 8 * PAGE_SIZE);
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

	BDPRINTF(PDB_BOOT, ("ktext %08lx[%08lx] - %08lx[%08lx] : "
				"kdata %08lx[%08lx] - %08lx[%08lx]\n",
				(u_long)ktext, (u_long)ktextp,
				(u_long)ektext, (u_long)ektextp,
				(u_long)kdata, (u_long)kdatap,
				(u_long)ekdata, (u_long)ekdatap));
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

	if (pmapdebug & PDB_BOOT) {
		/* print out mem list */
		prom_printf("Available physical memory after cleanup:\n");
		for (i = 0; i < pcnt; i++) {
			prom_printf("avail start %lx size %lx\n",
				    (long)avail[i].start, (long)avail[i].size);
		}
		prom_printf("End of available physical memory after cleanup\n");
	}

	/*
	 * Allocate and clear out pmap_kernel()->pm_segs[]
	 */
	pmap_kernel()->pm_refs = 1;
	memset(&pmap_kernel()->pm_ctx, 0, sizeof(pmap_kernel()->pm_ctx));

	/* Throw away page zero */
	do {
		pmap_get_page(&newp);
	} while (!newp);
	pmap_kernel()->pm_segs=(paddr_t *)(u_long)newp;
	pmap_kernel()->pm_physaddr = newp;

	/*
	 * finish filling out kernel pmap.
	 */

	BDPRINTF(PDB_BOOT, ("pmap_kernel()->pm_physaddr = %lx\n",
	    (long)pmap_kernel()->pm_physaddr));
	/*
	 * Tell pmap about our mesgbuf -- Hope this works already
	 */
	BDPRINTF(PDB_BOOT1, ("Calling consinit()\n"));
	if (pmapdebug & PDB_BOOT1)
		consinit();
	BDPRINTF(PDB_BOOT1, ("Inserting mesgbuf into pmap_kernel()\n"));
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
				page_size_map[k].use++;
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
		extern void main(void);
		vaddr_t u0va;
		paddr_t pa;

		u0va = vmmap;

		BDPRINTF(PDB_BOOT1,
			("Inserting lwp0 USPACE into pmap_kernel() at %p\n",
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
		/* Now map in all 8 pages of interrupt stack/cpu_info */
		pa = cpu0paddr;
		prom_map_phys(pa, 64*KB, vmmap, -1);

		/*
		 * Also map it in as the interrupt stack.
		 * This lets the PROM see this if needed.
		 *
		 * XXXX locore.s does not flush these mappings
		 * before installing the locked TTE.
		 */
		prom_map_phys(pa, 64*KB, INTSTACK, -1);
		for (i = 0; i < 8; i++) {
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
		memset((void *)intstk, 0, 64 * KB);
		cpus->ci_self = cpus;
		cpus->ci_next = NULL;
		cpus->ci_curlwp = &lwp0;
		cpus->ci_flags = CPUF_PRIMARY;
		cpus->ci_cpuid = CPU_UPAID;
		cpus->ci_fplwp = NULL;
		cpus->ci_eintstack = NULL;
		cpus->ci_spinup = main; /* Call main when we're running. */
		cpus->ci_paddr = cpu0paddr;
		cpus->ci_cpcb = (struct pcb *)u0va;
		cpus->ci_idepth = -1;
		memset(cpus->ci_intrpending, -1, sizeof(cpus->ci_intrpending));

		uvm_lwp_setuarea(&lwp0, u0va);
		lwp0.l_md.md_tf = (struct trapframe64*)(u0va + USPACE
		    - sizeof(struct trapframe64));

		cpu0paddr += 64 * KB;

		CPUSET_CLEAR(cpus_active);
		CPUSET_ADD(cpus_active, 0);

		cpu_pmap_prepare(cpus, true);
		cpu_pmap_init(cpus);

		/* The rest will be done at CPU attach time. */
		BDPRINTF(PDB_BOOT1,
			 ("Done inserting cpu_info into pmap_kernel()\n"));
	}

	vmmap = (vaddr_t)reserve_dumppages((void *)(u_long)vmmap);

#ifdef MODULAR
	/*
	 * Reserve 16 MB of VA for module loading. Right now our full
	 * GENERIC kernel is about 13 MB, so this looks good enough.
	 * If we make this bigger, we should adjust the KERNEND and
	 * associated defines in param.h.
	 */
	module_start = vmmap;
	vmmap += 16 * 1024*1024;
	module_end = vmmap;
#endif

	/*
	 * Set up bounds of allocatable memory for vmstat et al.
	 */
	avail_start = avail->start;
	for (mp = avail; mp->size; mp++)
		avail_end = mp->start+mp->size;

	BDPRINTF(PDB_BOOT1, ("Finished pmap_bootstrap()\n"));

	BDPRINTF(PDB_BOOT, ("left kdata: %" PRId64 " @%" PRIx64 ".\n",
				kdata_mem_pool.size, kdata_mem_pool.start));
}

/*
 * Allocate TSBs for both mmus from the locked kernel data segment page.
 * This is run before the cpu itself is activated (or by the first cpu
 * itself)
 */
void
cpu_pmap_prepare(struct cpu_info *ci, bool initial)
{
	/* allocate our TSBs */
	ci->ci_tsb_dmmu = (pte_t *)kdata_alloc(TSBSIZE, TSBSIZE);
	ci->ci_tsb_immu = (pte_t *)kdata_alloc(TSBSIZE, TSBSIZE);
	memset(ci->ci_tsb_dmmu, 0, TSBSIZE);
	memset(ci->ci_tsb_immu, 0, TSBSIZE);
	if (!initial) {
		KASSERT(ci != curcpu());
		/*
		 * Initially share ctxbusy with the boot cpu, the
		 * cpu will replace it as soon as it runs (and can
		 * probe the number of available contexts itself).
		 * Untill then only context 0 (aka kernel) will be
		 * referenced anyway.
		 */
		ci->ci_numctx = curcpu()->ci_numctx;
		ci->ci_ctxbusy = curcpu()->ci_ctxbusy;
	}

	BDPRINTF(PDB_BOOT1, ("cpu %d: TSB allocated at %p/%p size %08x\n",
	    ci->ci_index, ci->ci_tsb_dmmu, ci->ci_tsb_immu, TSBSIZE));
}

/*
 * Initialize the per CPU parts for the cpu running this code.
 */
void
cpu_pmap_init(struct cpu_info *ci)
{
	size_t ctxsize;

	/*
	 * We delay initialising ci_ctx_lock here as LOCKDEBUG isn't
	 * running for cpu0 yet..
	 */
	ci->ci_pmap_next_ctx = 1;
#ifdef SUN4V
#error find out if we have 16 or 13 bit context ids
#else
	ci->ci_numctx = 0x2000; /* all SUN4U use 13 bit contexts */
#endif
	ctxsize = sizeof(paddr_t)*ci->ci_numctx;
	ci->ci_ctxbusy = (paddr_t *)kdata_alloc(ctxsize, sizeof(uint64_t));
	memset(ci->ci_ctxbusy, 0, ctxsize);
	LIST_INIT(&ci->ci_pmap_ctxlist);

	/* mark kernel context as busy */
	ci->ci_ctxbusy[0] = pmap_kernel()->pm_physaddr;
}

/*
 * Initialize anything else for pmap handling.
 * Called during vm_init().
 */
void
pmap_init(void)
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
	TAILQ_FOREACH(pg, &pglist, pageq.queue) {
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
	pool_cache_bootstrap(&pmap_cache, sizeof(struct pmap),
	    SPARC64_BLOCK_SIZE, 0, 0, "pmappl", NULL, IPL_NONE, NULL, NULL,
	    NULL);
	pool_cache_bootstrap(&pmap_pv_cache, sizeof(struct pv_entry), 0, 0,
	    PR_LARGECACHE, "pv_entry", NULL, IPL_NONE, NULL, NULL, NULL);

	vm_first_phys = avail_start;
	vm_num_phys = avail_end - avail_start;

	mutex_init(&pmap_lock, MUTEX_DEFAULT, IPL_NONE);
#if defined(USE_LOCKSAFE_PSEG_GETSET)
	mutex_init(&pseg_lock, MUTEX_SPIN, IPL_VM);
#endif
	lock_available = true;
}

/*
 * How much virtual space is available to the kernel?
 */
static vaddr_t kbreak; /* End of kernel VA */
void
pmap_virtual_space(vaddr_t *start, vaddr_t *end)
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
pmap_growkernel(vaddr_t maxkvaddr)
{
	struct pmap *pm = pmap_kernel();
	paddr_t pa;

	if (maxkvaddr >= KERNEND) {
		printf("WARNING: cannot extend kernel pmap beyond %p to %p\n",
		       (void *)KERNEND, (void *)maxkvaddr);
		return (kbreak);
	}
	DPRINTF(PDB_GROW, ("pmap_growkernel(%lx...%lx)\n", kbreak, maxkvaddr));
	/* Align with the start of a page table */
	for (kbreak &= (-1 << PDSHIFT); kbreak < maxkvaddr;
	     kbreak += (1 << PDSHIFT)) {
		if (pseg_get(pm, kbreak) & TLB_V)
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
	return (kbreak);
}

/*
 * Create and return a physical map.
 */
struct pmap *
pmap_create(void)
{
	struct pmap *pm;

	DPRINTF(PDB_CREATE, ("pmap_create()\n"));

	pm = pool_cache_get(&pmap_cache, PR_WAITOK);
	memset(pm, 0, sizeof *pm);
	DPRINTF(PDB_CREATE, ("pmap_create(): created %p\n", pm));

	pm->pm_refs = 1;
	TAILQ_INIT(&pm->pm_ptps);
	if (pm != pmap_kernel()) {
		while (!pmap_get_page(&pm->pm_physaddr)) {
			uvm_wait("pmap_create");
		}
		pm->pm_segs = (paddr_t *)(u_long)pm->pm_physaddr;
	}
	DPRINTF(PDB_CREATE, ("pmap_create(%p): ctx %d\n", pm, pmap_ctx(pm)));
	return pm;
}

/*
 * Add a reference to the given pmap.
 */
void
pmap_reference(struct pmap *pm)
{

	atomic_inc_uint(&pm->pm_refs);
}

/*
 * Retire the given pmap from service.
 * Should only be called if the map contains no valid mappings.
 */
void
pmap_destroy(struct pmap *pm)
{
#ifdef MULTIPROCESSOR
	struct cpu_info *ci;
	sparc64_cpuset_t pmap_cpus_active;
#else
#define pmap_cpus_active 0
#endif
	struct vm_page *pg;

	if ((int)atomic_dec_uint_nv(&pm->pm_refs) > 0) {
		return;
	}
	DPRINTF(PDB_DESTROY, ("pmap_destroy: freeing pmap %p\n", pm));
#ifdef MULTIPROCESSOR
	CPUSET_CLEAR(pmap_cpus_active);
	for (ci = cpus; ci != NULL; ci = ci->ci_next) {
		/* XXXMRG: Move the lock inside one or both tests? */
		mutex_enter(&ci->ci_ctx_lock);
		if (CPUSET_HAS(cpus_active, ci->ci_index)) {
			if (pm->pm_ctx[ci->ci_index] > 0) {
				CPUSET_ADD(pmap_cpus_active, ci->ci_index);
				ctx_free(pm, ci);
			}
		}
		mutex_exit(&ci->ci_ctx_lock);
	}
#else
	if (pmap_ctx(pm)) {
		mutex_enter(&curcpu()->ci_ctx_lock);
		ctx_free(pm, curcpu());
		mutex_exit(&curcpu()->ci_ctx_lock);
	}
#endif

	/* we could be a little smarter and leave pages zeroed */
	while ((pg = TAILQ_FIRST(&pm->pm_ptps)) != NULL) {
#ifdef DIAGNOSTIC
		struct vm_page_md *md = VM_PAGE_TO_MD(pg);
#endif

		TAILQ_REMOVE(&pm->pm_ptps, pg, pageq.queue);
		KASSERT(md->mdpg_pvh.pv_pmap == NULL);
		dcache_flush_page_cpuset(VM_PAGE_TO_PHYS(pg), pmap_cpus_active);
		uvm_pagefree(pg);
	}
	pmap_free_page((paddr_t)(u_long)pm->pm_segs, pmap_cpus_active);

	pool_cache_put(&pmap_cache, pm);
}

/*
 * Copy the range specified by src_addr/len
 * from the source map to the range dst_addr/len
 * in the destination map.
 *
 * This routine is only advisory and need not do anything.
 */
void
pmap_copy(struct pmap *dst_pmap, struct pmap *src_pmap, vaddr_t dst_addr, vsize_t len, vaddr_t src_addr)
{

	DPRINTF(PDB_CREATE, ("pmap_copy(%p, %p, %p, %lx, %p)\n",
			     dst_pmap, src_pmap, (void *)(u_long)dst_addr,
			     (u_long)len, (void *)(u_long)src_addr));
}

/*
 * Activate the address space for the specified process.  If the
 * process is the current process, load the new MMU context.
 */
void
pmap_activate(struct lwp *l)
{
	struct pmap *pmap = l->l_proc->p_vmspace->vm_map.pmap;

	if (pmap == pmap_kernel()) {
		return;
	}

	/*
	 * This is essentially the same thing that happens in cpu_switchto()
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

	if (pmap_ctx(pmap) == 0) {
		(void) ctx_alloc(pmap);
	}
	DPRINTF(PDB_ACTIVATE,
		("%s: cpu%d activating ctx %d\n", __func__,
		 cpu_number(), pmap_ctx(pmap)));
	dmmu_set_secondary_context(pmap_ctx(pmap));
}

/*
 * Deactivate the address space of the specified process.
 */
void
pmap_deactivate(struct lwp *l)
{

	DPRINTF(PDB_ACTIVATE,
		("%s: cpu%d deactivating ctx %d\n", __func__,
		 cpu_number(), pmap_ctx(l->l_proc->p_vmspace->vm_map.pmap)));
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
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
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
		pmap_free_page_noflush(ptp);
	}
#ifdef PMAP_DEBUG
	i = ptelookup_va(va);
	if (pmapdebug & PDB_ENTER)
		prom_printf("pmap_kenter_pa: va=%08x data=%08x:%08x "
			"tsb_dmmu[%d]=%08x\n", va, (int)(tte.data>>32),
			(int)tte.data, i, &curcpu()->ci_tsb_dmmu[i]);
	if (pmapdebug & PDB_MMU_STEAL && curcpu()->ci_tsb_dmmu[i].data) {
		prom_printf("pmap_kenter_pa: evicting entry tag=%x:%08x "
			"data=%08x:%08x tsb_dmmu[%d]=%08x\n",
			(int)(curcpu()->ci_tsb_dmmu[i].tag>>32), (int)curcpu()->ci_tsb_dmmu[i].tag,
			(int)(curcpu()->ci_tsb_dmmu[i].data>>32), (int)curcpu()->ci_tsb_dmmu[i].data,
			i, &curcpu()->ci_tsb_dmmu[i]);
		prom_printf("with va=%08x data=%08x:%08x tsb_dmmu[%d]=%08x\n",
			va, (int)(tte.data>>32), (int)tte.data,	i,
			&curcpu()->ci_tsb_dmmu[i]);
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
pmap_kremove(vaddr_t va, vsize_t size)
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
		if ((data & TLB_V) == 0) {
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

		tsb_invalidate(va, pm);
		REMOVE_STAT(tflushes);

		/*
		 * Here we assume nothing can get into the TLB
		 * unless it has a PTE.
		 */

		tlb_flush_pte(va, pm);
		dcache_flush_page_all(pa);
	}
	if (flush)
		REMOVE_STAT(flushes);
}

/*
 * Insert physical page at pa into the given pmap at virtual address va.
 * Supports 64-bit pa so we can map I/O space.
 */

int
pmap_enter(struct pmap *pm, vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	pte_t tte;
	int64_t data;
	paddr_t opa = 0, ptp; /* XXX: gcc */
	pv_entry_t pvh, npv = NULL, freepv;
	struct vm_page *pg, *opg, *ptpg;
	int s, i, uncached = 0, error = 0;
	int size = PGSZ_8K; /* PMAP_SZ_TO_TTE(pa); */
	bool wired = (flags & PMAP_WIRED) != 0;
	bool wasmapped = FALSE;
	bool dopv = TRUE;

	/*
	 * Is this part of the permanent mappings?
	 */
	KASSERT(pm != pmap_kernel() || va < INTSTACK || va > EINTSTACK);
	KASSERT(pm != pmap_kernel() || va < kdata || va > ekdata);

	/* Grab a spare PV. */
	freepv = pool_cache_get(&pmap_pv_cache, PR_NOWAIT);
	if (__predict_false(freepv == NULL)) {
		if (flags & PMAP_CANFAIL)
			return (ENOMEM);
		panic("pmap_enter: no pv entries available");
	}
	freepv->pv_next = NULL;

	/*
	 * If a mapping at this address already exists, check if we're
	 * entering the same PA again.  if it's different remove it.
	 */

	mutex_enter(&pmap_lock);
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
		struct vm_page_md * const md = VM_PAGE_TO_MD(pg);

		pvh = &md->mdpg_pvh;
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
				/* free it */
				npv->pv_next = freepv;
				freepv = npv;
				npv = NULL;
			}
			if (wasmapped && opa == pa) {
				dopv = FALSE;
			}
		} else if (npv == NULL) {
			/* use the pre-allocated pv */
			npv = freepv;
			freepv = freepv->pv_next;
		}
		ENTER_STAT(managed);
	} else {
		ENTER_STAT(unmanaged);
		dopv = FALSE;
		if (npv != NULL) {
			/* free it */
			npv->pv_next = freepv;
			freepv = npv;
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
		mutex_exit(&pmap_lock);
		goto out;
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
			TAILQ_INSERT_TAIL(&pm->pm_ptps, ptpg, pageq.queue);
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
			TAILQ_INSERT_TAIL(&pm->pm_ptps, ptpg, pageq.queue);
		} else {
			KASSERT(pm == pmap_kernel());
		}
	}
	if (i & 1) {
		KASSERT((i & 4) == 0);
		ptp = 0;
		if (!pmap_get_page(&ptp)) {
			mutex_exit(&pmap_lock);
			if (flags & PMAP_CANFAIL) {
				if (npv != NULL) {
					/* free it */
					npv->pv_next = freepv;
					freepv = npv;
				}
				error = ENOMEM;
				goto out;
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
		pmap_free_page_noflush(ptp);
	}
	if (dopv) {
		pmap_enter_pv(pm, va, pa, pg, npv);
	}

	mutex_exit(&pmap_lock);
#ifdef PMAP_DEBUG
	i = ptelookup_va(va);
	if (pmapdebug & PDB_ENTER)
		prom_printf("pmap_enter: va=%08x data=%08x:%08x "
			"tsb_dmmu[%d]=%08x\n", va, (int)(tte.data>>32),
			(int)tte.data, i, &curcpu()->ci_tsb_dmmu[i]);
	if (pmapdebug & PDB_MMU_STEAL && curcpu()->ci_tsb_dmmu[i].data) {
		prom_printf("pmap_enter: evicting entry tag=%x:%08x "
			"data=%08x:%08x tsb_dmmu[%d]=%08x\n",
			(int)(curcpu()->ci_tsb_dmmu[i].tag>>32), (int)curcpu()->ci_tsb_dmmu[i].tag,
			(int)(curcpu()->ci_tsb_dmmu[i].data>>32), (int)curcpu()->ci_tsb_dmmu[i].data, i,
			&curcpu()->ci_tsb_dmmu[i]);
		prom_printf("with va=%08x data=%08x:%08x tsb_dmmu[%d]=%08x\n",
			va, (int)(tte.data>>32), (int)tte.data, i,
			&curcpu()->ci_tsb_dmmu[i]);
	}
#endif

	if (flags & (VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE)) {

		/*
		 * preload the TSB with the new entry,
		 * since we're going to need it immediately anyway.
		 */

		KASSERT(pmap_ctx(pm)>=0);
		i = ptelookup_va(va);
		tte.tag = TSB_TAG(0, pmap_ctx(pm), va);
		s = splhigh();
		if (wasmapped && pmap_is_on_mmu(pm)) {
			tsb_invalidate(va, pm);
		}
		if (flags & (VM_PROT_READ | VM_PROT_WRITE)) {
			curcpu()->ci_tsb_dmmu[i].tag = tte.tag;
			__asm volatile("" : : : "memory");
			curcpu()->ci_tsb_dmmu[i].data = tte.data;
		}
		if (flags & VM_PROT_EXECUTE) {
			curcpu()->ci_tsb_immu[i].tag = tte.tag;
			__asm volatile("" : : : "memory");
			curcpu()->ci_tsb_immu[i].data = tte.data;
		}

		/*
		 * it's only necessary to flush the TLB if this page was
		 * previously mapped, but for some reason it's a lot faster
		 * for the fork+exit microbenchmark if we always do it.
		 */

		KASSERT(pmap_ctx(pm)>=0);
#ifdef MULTIPROCESSOR
		if (wasmapped && pmap_is_on_mmu(pm))
			tlb_flush_pte(va, pm);
		else
			sp_tlb_flush_pte(va, pmap_ctx(pm));
#else
		tlb_flush_pte(va, pm);
#endif
		splx(s);
	} else if (wasmapped && pmap_is_on_mmu(pm)) {
		/* Force reload -- protections may be changed */
		KASSERT(pmap_ctx(pm)>=0);
		tsb_invalidate(va, pm);
		tlb_flush_pte(va, pm);
	}

	/* We will let the fast mmu miss interrupt load the new translation */
	pv_check();
 out:
	/* Catch up on deferred frees. */
	for (; freepv != NULL; freepv = npv) {
		npv = freepv->pv_next;
		pool_cache_put(&pmap_pv_cache, freepv);
	}
	return error;
}

void
pmap_remove_all(struct pmap *pm)
{
#ifdef MULTIPROCESSOR
	struct cpu_info *ci;
	sparc64_cpuset_t pmap_cpus_active;
#endif

	if (pm == pmap_kernel()) {
		return;
	}
	write_user_windows();
	pm->pm_refs = 0;

	/*
	 * XXXMRG: pmap_destroy() does exactly the same dance here.
	 * surely one of them isn't necessary?
	 */
#ifdef MULTIPROCESSOR
	CPUSET_CLEAR(pmap_cpus_active);
	for (ci = cpus; ci != NULL; ci = ci->ci_next) {
		/* XXXMRG: Move the lock inside one or both tests? */
		mutex_enter(&ci->ci_ctx_lock);
		if (CPUSET_HAS(cpus_active, ci->ci_index)) {
			if (pm->pm_ctx[ci->ci_index] > 0) {
				CPUSET_ADD(pmap_cpus_active, ci->ci_index);
				ctx_free(pm, ci);
			}
		}
		mutex_exit(&ci->ci_ctx_lock);
	}
#else
	if (pmap_ctx(pm)) {
		mutex_enter(&curcpu()->ci_ctx_lock);
		ctx_free(pm, curcpu());
		mutex_exit(&curcpu()->ci_ctx_lock);
	}
#endif

	REMOVE_STAT(flushes);
	/*
	 * XXXMRG: couldn't we do something less severe here, and
	 * only flush the right context on each CPU?
	 */
	blast_dcache();
}

/*
 * Remove the given range of mapping entries.
 */
void
pmap_remove(struct pmap *pm, vaddr_t va, vaddr_t endva)
{
	int64_t data;
	paddr_t pa;
	struct vm_page *pg;
	pv_entry_t pv, freepv = NULL;
	int rv;
	bool flush = FALSE;

	/*
	 * In here we should check each pseg and if there are no more entries,
	 * free it.  It's just that linear scans of 8K pages gets expensive.
	 */

	KASSERT(pm != pmap_kernel() || endva < INTSTACK || va > EINTSTACK);
	KASSERT(pm != pmap_kernel() || endva < kdata || va > ekdata);

	mutex_enter(&pmap_lock);
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
		if ((data & TLB_V) == 0) {
			continue;
		}

		flush = TRUE;
		/* First remove the pv entry, if there is one */
		pa = data & TLB_PA_MASK;
		pg = PHYS_TO_VM_PAGE(pa);
		if (pg) {
			pv = pmap_remove_pv(pm, va, pg);
			if (pv != NULL) {
				/* free it */
				pv->pv_next = freepv;
				freepv = pv;
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

		if (pm != pmap_kernel() && !pmap_has_ctx(pm))
			continue;

		/*
		 * if the pmap is being torn down, don't bother flushing,
		 * we already have done so.
		 */

		if (!pm->pm_refs)
			continue;

		/*
		 * Here we assume nothing can get into the TLB
		 * unless it has a PTE.
		 */

		KASSERT(pmap_ctx(pm)>=0);
		tsb_invalidate(va, pm);
		REMOVE_STAT(tflushes);
		tlb_flush_pte(va, pm);
		dcache_flush_page_all(pa);
	}
	if (flush && pm->pm_refs)
		REMOVE_STAT(flushes);
	DPRINTF(PDB_REMOVE, ("\n"));
	pv_check();
	mutex_exit(&pmap_lock);

	/* Catch up on deferred frees. */
	for (; freepv != NULL; freepv = pv) {
		pv = freepv->pv_next;
		pool_cache_put(&pmap_pv_cache, freepv);
	}
}

/*
 * Change the protection on the specified range of this pmap.
 */
void
pmap_protect(struct pmap *pm, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
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

	sva = trunc_page(sva);
	mutex_enter(&pmap_lock);
	for (; sva < eva; sva += PAGE_SIZE) {
#ifdef PMAP_DEBUG
		/*
		 * Is this part of the permanent 4MB mapping?
		 */
		if (pm == pmap_kernel() && sva >= ktext &&
		    sva < roundup(ekdata, 4 * MEG)) {
			mutex_exit(&pmap_lock);
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
			struct vm_page_md * const md = VM_PAGE_TO_MD(pg);

			/* Save REF/MOD info */
			pv = &md->mdpg_pvh;
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

		if (pm != pmap_kernel() && !pmap_has_ctx(pm))
			continue;

		KASSERT(pmap_ctx(pm)>=0);
		tsb_invalidate(sva, pm);
		tlb_flush_pte(sva, pm);
	}
	pv_check();
	mutex_exit(&pmap_lock);
}

/*
 * Extract the physical page address associated
 * with the given map/virtual_address pair.
 */
bool
pmap_extract(struct pmap *pm, vaddr_t va, paddr_t *pap)
{
	paddr_t pa;
	int64_t data = 0;

	if (pm == pmap_kernel() && va >= kdata && va < roundup(ekdata, 4*MEG)) {
		/* Need to deal w/locked TLB entry specially. */
		pa = pmap_kextract(va);
		DPRINTF(PDB_EXTRACT, ("pmap_extract: va=%lx pa=%llx\n",
				      (u_long)va, (unsigned long long)pa));
		if (pap != NULL)
			*pap = pa;
		return TRUE;
	} else if (pm == pmap_kernel() && va >= ktext && va < ektext) {
		/* Need to deal w/locked TLB entry specially. */
		pa = pmap_kextract(va);
		DPRINTF(PDB_EXTRACT, ("pmap_extract: va=%lx pa=%llx\n",
		    (u_long)va, (unsigned long long)pa));
		if (pap != NULL)
			*pap = pa;
		return TRUE;
	} else if (pm == pmap_kernel() && va >= INTSTACK && va < (INTSTACK + 64*KB)) {
		pa = (paddr_t)(curcpu()->ci_paddr - INTSTACK + va);
		DPRINTF(PDB_EXTRACT, ("pmap_extract (intstack): va=%lx pa=%llx\n",
		    (u_long)va, (unsigned long long)pa));
		if (pap != NULL)
			*pap = pa;
		return TRUE;
	} else {
		data = pseg_get(pm, va);
		pa = data & TLB_PA_MASK;
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
	}
	if ((data & TLB_V) == 0)
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
pmap_kprotect(vaddr_t va, vm_prot_t prot)
{
	struct pmap *pm = pmap_kernel();
	int64_t data;
	int rv;

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
	KASSERT(pmap_ctx(pm)>=0);
	tsb_invalidate(va, pm);
	tlb_flush_pte(va, pm);
}

/*
 * Return the number bytes that pmap_dumpmmu() will dump.
 */
int
pmap_dumpsize(void)
{
	int	sz;

	sz = ALIGN(sizeof(kcore_seg_t)) + ALIGN(sizeof(cpu_kcore_hdr_t));
	sz += kernel_tlb_slots * sizeof(struct cpu_kcore_4mbseg);
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
	struct cpu_kcore_4mbseg ktlb;
	int	error = 0;
	int	i;
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
	kcpu->cputype = cputyp;
	kcpu->kernbase = (uint64_t)KERNBASE;
	kcpu->cpubase = (uint64_t)CPUINFO_VA;

	/* Describe the locked text segment */
	kcpu->ktextbase = (uint64_t)ktext;
	kcpu->ktextp = (uint64_t)ktextp;
	kcpu->ktextsz = (uint64_t)ektext - ktext;
	if (kcpu->ktextsz > 4*MEG)
		kcpu->ktextsz = 0;	/* old version can not work */

	/* Describe locked data segment */
	kcpu->kdatabase = (uint64_t)kdata;
	kcpu->kdatap = (uint64_t)kdatap;
	kcpu->kdatasz = (uint64_t)ekdatap - kdatap;

	/* new version of locked segments description */
	kcpu->newmagic = SPARC64_KCORE_NEWMAGIC;
	kcpu->num4mbsegs = kernel_tlb_slots;
	kcpu->off4mbsegs = ALIGN(sizeof(cpu_kcore_hdr_t));

	/* description of per-cpu mappings */
	kcpu->numcpuinfos = sparc_ncpus;
	kcpu->percpusz = 64 * 1024;	/* used to be 128k for some time */
	kcpu->thiscpu = cpu_number();	/* which cpu is doing this dump */
	kcpu->cpusp = cpu0paddr - 64 * 1024 * sparc_ncpus;

	/* Now the memsegs */
	kcpu->nmemseg = phys_installed_size;
	kcpu->memsegoffset = kcpu->off4mbsegs
		+ kernel_tlb_slots * sizeof(struct cpu_kcore_4mbseg);

	/* Now we need to point this at our kernel pmap. */
	kcpu->nsegmap = STSZ;
	kcpu->segmapoffset = (uint64_t)pmap_kernel()->pm_physaddr;

	/* Note: we have assumed everything fits in buffer[] so far... */
	bp = (int *)((long)kcpu + ALIGN(sizeof(cpu_kcore_hdr_t)));

	/* write locked kernel 4MB TLBs */
	for (i = 0; i < kernel_tlb_slots; i++) {
		ktlb.va = kernel_tlbs[i].te_va;
		ktlb.pa = kernel_tlbs[i].te_pa;
		EXPEDITE(&ktlb, sizeof(ktlb));
	}

	/* write memsegs */
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
pmap_clear_modify(struct vm_page *pg)
{
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	pv_entry_t pv;
	int rv;
	int changed = 0;
#ifdef DEBUG
	int modified = 0;

	DPRINTF(PDB_CHANGEPROT|PDB_REF, ("pmap_clear_modify(%p)\n", pg));

	modified = pmap_is_modified(pg);
#endif
	mutex_enter(&pmap_lock);
	/* Clear all mappings */
	pv = &md->mdpg_pvh;
#ifdef DEBUG
	if (pv->pv_va & PV_MOD)
		pv->pv_va |= PV_WE;	/* Remember this was modified */
#endif
	if (pv->pv_va & PV_MOD) {
		changed |= 1;
		pv->pv_va &= ~PV_MOD;
	}
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
			if (pmap_is_on_mmu(pmap)) {
				KASSERT(pmap_ctx(pmap)>=0);
				tsb_invalidate(va, pmap);
				tlb_flush_pte(va, pmap);
			}
			/* Then clear the mod bit in the pv */
			if (pv->pv_va & PV_MOD) {
				changed |= 1;
				pv->pv_va &= ~PV_MOD;
			}
		}
	}
	pv_check();
	mutex_exit(&pmap_lock);
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
pmap_clear_reference(struct vm_page *pg)
{
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	pv_entry_t pv;
	int rv;
	int changed = 0;
#ifdef DEBUG
	int referenced = 0;
#endif

	mutex_enter(&pmap_lock);
#ifdef DEBUG
	DPRINTF(PDB_CHANGEPROT|PDB_REF, ("pmap_clear_reference(%p)\n", pg));
	referenced = pmap_is_referenced_locked(pg);
#endif
	/* Clear all references */
	pv = &md->mdpg_pvh;
	if (pv->pv_va & PV_REF) {
		changed |= 1;
		pv->pv_va &= ~PV_REF;
	}
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

			data = pseg_get(pmap, va);
			KASSERT(data & TLB_V);
			DPRINTF(PDB_CHANGEPROT,
			    ("clearing ref pm:%p va:%p ctx:%lx data:%llx\n",
			     pmap, (void *)(u_long)va,
			     (u_long)pmap_ctx(pmap),
			     (long long)data));
#ifdef HWREF
			if (data & TLB_ACCESS) {
				changed |= 1;
				data &= ~TLB_ACCESS;
			}
#else
			if (data < 0)
				changed |= 1;
			data = 0;
#endif
			rv = pseg_set(pmap, va, data, 0);
			if (rv & 1)
				panic("pmap_clear_reference: pseg_set needs"
				    " spare! rv=%d\n", rv);
			if (pmap_is_on_mmu(pmap)) {
				KASSERT(pmap_ctx(pmap)>=0);
				tsb_invalidate(va, pmap);
				tlb_flush_pte(va, pmap);
			}
			if (pv->pv_va & PV_REF) {
				changed |= 1;
				pv->pv_va &= ~PV_REF;
			}
		}
	}
	dcache_flush_page_all(VM_PAGE_TO_PHYS(pg));
	pv_check();
#ifdef DEBUG
	if (pmap_is_referenced_locked(pg)) {
		pv = &md->mdpg_pvh;
		printf("pmap_clear_reference(): %p still referenced "
			"(pmap = %p, ctx = %d)\n", pg, pv->pv_pmap,
			pv->pv_pmap ? pmap_ctx(pv->pv_pmap) : 0);
		Debugger();
	}
	DPRINTF(PDB_CHANGEPROT|PDB_REF,
	    ("pmap_clear_reference: pg %p %s\n", pg,
	     (changed ? "was referenced" : "was not referenced")));
	if (referenced != changed) {
		printf("pmap_clear_reference: referenced %d changed %d\n",
		       referenced, changed);
		Debugger();
	} else {
		mutex_exit(&pmap_lock);
		return (referenced);
	}
#endif
	mutex_exit(&pmap_lock);
	return (changed);
}

bool
pmap_is_modified(struct vm_page *pg)
{
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	pv_entry_t pv, npv;
	bool res = false;

	/* Check if any mapping has been modified */
	pv = &md->mdpg_pvh;
	if (pv->pv_va & PV_MOD)
		res = true;
#ifdef HWREF
#ifdef DEBUG
	if (pv->pv_next && !pv->pv_pmap) {
		printf("pmap_is_modified: npv but no pmap for pv %p\n", pv);
		Debugger();
	}
#endif
	if (!res && pv->pv_pmap != NULL) {
		mutex_enter(&pmap_lock);
		for (npv = pv; !res && npv && npv->pv_pmap;
		     npv = npv->pv_next) {
			int64_t data;

			data = pseg_get(npv->pv_pmap, npv->pv_va & PV_VAMASK);
			KASSERT(data & TLB_V);
			if (data & TLB_MODIFY)
				res = true;

			/* Migrate modify info to head pv */
			if (npv->pv_va & PV_MOD) {
				res = true;
				npv->pv_va &= ~PV_MOD;
			}
		}
		/* Save modify info */
		if (res)
			pv->pv_va |= PV_MOD;
#ifdef DEBUG
		if (res)
			pv->pv_va |= PV_WE;
#endif
		mutex_exit(&pmap_lock);
	}
#endif

	DPRINTF(PDB_CHANGEPROT|PDB_REF, ("pmap_is_modified(%p) = %d\n", pg,
	    res));
	pv_check();
	return res;
}

/*
 * Variant of pmap_is_reference() where caller already holds pmap_lock
 */
static bool
pmap_is_referenced_locked(struct vm_page *pg)
{
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	pv_entry_t pv, npv;
	bool res = false;

	KASSERT(mutex_owned(&pmap_lock));

	/* Check if any mapping has been referenced */
	pv = &md->mdpg_pvh;
	if (pv->pv_va & PV_REF)
		return true;

#ifdef HWREF
#ifdef DEBUG
	if (pv->pv_next && !pv->pv_pmap) {
		printf("pmap_is_referenced: npv but no pmap for pv %p\n", pv);
		Debugger();
	}
#endif
	if (pv->pv_pmap == NULL)
		return false;

	for (npv = pv; npv; npv = npv->pv_next) {
		int64_t data;

		data = pseg_get(npv->pv_pmap, npv->pv_va & PV_VAMASK);
		KASSERT(data & TLB_V);
		if (data & TLB_ACCESS)
			res = true;

		/* Migrate ref info to head pv */
		if (npv->pv_va & PV_REF) {
			res = true;
			npv->pv_va &= ~PV_REF;
		}
	}
	/* Save ref info */
	if (res)
		pv->pv_va |= PV_REF;
#endif

	DPRINTF(PDB_CHANGEPROT|PDB_REF,
		("pmap_is_referenced(%p) = %d\n", pg, res));
	pv_check();
	return res;
}

bool
pmap_is_referenced(struct vm_page *pg)
{
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	pv_entry_t pv;
	bool res = false;

	/* Check if any mapping has been referenced */
	pv = &md->mdpg_pvh;
	if (pv->pv_va & PV_REF)
		return true;

#ifdef HWREF
#ifdef DEBUG
	if (pv->pv_next && !pv->pv_pmap) {
		printf("pmap_is_referenced: npv but no pmap for pv %p\n", pv);
		Debugger();
	}
#endif
	if (pv->pv_pmap != NULL) {
		mutex_enter(&pmap_lock);
		res = pmap_is_referenced_locked(pg);
		mutex_exit(&pmap_lock);
	}
#endif

	DPRINTF(PDB_CHANGEPROT|PDB_REF,
		("pmap_is_referenced(%p) = %d\n", pg, res));
	pv_check();
	return res;
}



/*
 *	Routine:	pmap_unwire
 *	Function:	Clear the wired attribute for a map/virtual-address
 *			pair.
 *	In/out conditions:
 *			The mapping must already exist in the pmap.
 */
void
pmap_unwire(pmap_t pmap, vaddr_t va)
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
	data = pseg_get(pmap, va & PV_VAMASK);
	KASSERT(data & TLB_V);
	data &= ~TLB_TSB_LOCK;
	rv = pseg_set(pmap, va & PV_VAMASK, data, 0);
	if (rv & 1)
		panic("pmap_unwire: pseg_set needs spare! rv=%d\n", rv);
	pv_check();
}

/*
 * Lower the protection on the specified physical page.
 *
 * Never enable writing as it will break COW
 */

void
pmap_page_protect(struct vm_page *pg, vm_prot_t prot)
{
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	int64_t clear, set;
	int64_t data = 0;
	int rv;
	pv_entry_t pv, npv, freepv = NULL;
	struct pmap *pmap;
	vaddr_t va;
	bool needflush = FALSE;

	DPRINTF(PDB_CHANGEPROT,
	    ("pmap_page_protect: pg %p prot %x\n", pg, prot));

	mutex_enter(&pmap_lock);
	pv = &md->mdpg_pvh;
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
				if (pmap_is_on_mmu(pmap)) {
					KASSERT(pmap_ctx(pmap)>=0);
					tsb_invalidate(va, pmap);
					tlb_flush_pte(va, pmap);
				}
			}
		}
	} else {
		/* remove mappings */
		DPRINTF(PDB_REMOVE,
			("pmap_page_protect: demapping pg %p\n", pg));

		/* First remove the entire list of continuation pv's */
		for (npv = pv->pv_next; npv; npv = pv->pv_next) {
			pmap = npv->pv_pmap;
			va = npv->pv_va & PV_VAMASK;

			/* We're removing npv from pv->pv_next */
			DPRINTF(PDB_CHANGEPROT|PDB_REF|PDB_REMOVE,
				("pmap_page_protect: "
				 "demap va %p of pg %p in pmap %p...\n",
				 (void *)(u_long)va, pg, pmap));

			/* clear the entry in the page table */
			data = pseg_get(pmap, va);
			KASSERT(data & TLB_V);

			/* Save ref/mod info */
			if (data & TLB_ACCESS)
				pv->pv_va |= PV_REF;
			if (data & TLB_MODIFY)
				pv->pv_va |= PV_MOD;
			/* Clear mapping */
			rv = pseg_set(pmap, va, 0, 0);
			if (rv & 1)
				panic("pmap_page_protect: pseg_set needs"
				     " spare! rv=%d\n", rv);
			if (pmap_is_on_mmu(pmap)) {
				KASSERT(pmap_ctx(pmap)>=0);
				tsb_invalidate(va, pmap);
				tlb_flush_pte(va, pmap);
			}
			if (pmap->pm_refs > 0) {
				needflush = TRUE;
			}

			/* free the pv */
			pv->pv_next = npv->pv_next;
			npv->pv_next = freepv;
			freepv = npv;
		}

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
			if (pmap_is_on_mmu(pmap)) {
			    	KASSERT(pmap_ctx(pmap)>=0);
				tsb_invalidate(va, pmap);
				tlb_flush_pte(va, pmap);
			}
			if (pmap->pm_refs > 0) {
				needflush = TRUE;
			}
			npv = pv->pv_next;
			/* dump the first pv */
			if (npv) {
				/* First save mod/ref bits */
				pv->pv_pmap = npv->pv_pmap;
				pv->pv_va = (pv->pv_va & PV_MASK) | npv->pv_va;
				pv->pv_next = npv->pv_next;
				npv->pv_next = freepv;
				freepv = npv;
			} else {
				pv->pv_pmap = NULL;
				pv->pv_next = NULL;
			}
		}
		if (needflush)
			dcache_flush_page_all(VM_PAGE_TO_PHYS(pg));
	}
	/* We should really only flush the pages we demapped. */
	pv_check();
	mutex_exit(&pmap_lock);

	/* Catch up on deferred frees. */
	for (; freepv != NULL; freepv = npv) {
		npv = freepv->pv_next;
		pool_cache_put(&pmap_pv_cache, freepv);
	}
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

	/* Don't want one of these pages reused while we're reading it. */
	mutex_enter(&pmap_lock);
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
	mutex_exit(&pmap_lock);

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

	/* Don't want one of these pages reused while we're reading it. */
	mutex_enter(&pmap_lock);	/* XXX uvmplock */
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
	mutex_exit(&pmap_lock);	/* XXX uvmplock */

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
static int
ctx_alloc(struct pmap *pm)
{
	int i, ctx;

	KASSERT(pm != pmap_kernel());
	KASSERT(pm == curproc->p_vmspace->vm_map.pmap);
	mutex_enter(&curcpu()->ci_ctx_lock);
	ctx = curcpu()->ci_pmap_next_ctx++;

	/*
	 * if we have run out of contexts, remove all user entries from
	 * the TSB, TLB and dcache and start over with context 1 again.
	 */

	if (ctx == curcpu()->ci_numctx) {
		DPRINTF(PDB_CTX_ALLOC|PDB_CTX_FLUSHALL,
			("ctx_alloc: cpu%d run out of contexts %d\n",
			 cpu_number(), curcpu()->ci_numctx));
		write_user_windows();
		while (!LIST_EMPTY(&curcpu()->ci_pmap_ctxlist)) {
#ifdef MULTIPROCESSOR
			KASSERT(pmap_ctx(LIST_FIRST(&curcpu()->ci_pmap_ctxlist)) != 0);
#endif
			ctx_free(LIST_FIRST(&curcpu()->ci_pmap_ctxlist),
				 curcpu());
		}
		for (i = TSBENTS - 1; i >= 0; i--) {
			if (TSB_TAG_CTX(curcpu()->ci_tsb_dmmu[i].tag) != 0) {
				clrx(&curcpu()->ci_tsb_dmmu[i].data);
			}
			if (TSB_TAG_CTX(curcpu()->ci_tsb_immu[i].tag) != 0) {
				clrx(&curcpu()->ci_tsb_immu[i].data);
			}
		}
		sp_tlb_flush_all();
		ctx = 1;
		curcpu()->ci_pmap_next_ctx = 2;
	}
	curcpu()->ci_ctxbusy[ctx] = pm->pm_physaddr;
	LIST_INSERT_HEAD(&curcpu()->ci_pmap_ctxlist, pm, pm_list[cpu_number()]);
	pmap_ctx(pm) = ctx;
	mutex_exit(&curcpu()->ci_ctx_lock);
	DPRINTF(PDB_CTX_ALLOC, ("ctx_alloc: cpu%d allocated ctx %d\n",
		cpu_number(), ctx));
	return ctx;
}

/*
 * Give away a context.
 */
static void
ctx_free(struct pmap *pm, struct cpu_info *ci)
{
	int oldctx;
	int cpunum;

	KASSERT(mutex_owned(&ci->ci_ctx_lock));

#ifdef MULTIPROCESSOR
	cpunum = ci->ci_index;
#else
	/* Give the compiler a hint.. */
	cpunum = 0;
#endif

	oldctx = pm->pm_ctx[cpunum];
	if (oldctx == 0)
		return;

#ifdef DIAGNOSTIC
	if (pm == pmap_kernel())
		panic("ctx_free: freeing kernel context");
	if (ci->ci_ctxbusy[oldctx] == 0)
		printf("ctx_free: freeing free context %d\n", oldctx);
	if (ci->ci_ctxbusy[oldctx] != pm->pm_physaddr) {
		printf("ctx_free: freeing someone else's context\n "
		       "ctxbusy[%d] = %p, pm(%p)->pm_ctx = %p\n",
		       oldctx, (void *)(u_long)ci->ci_ctxbusy[oldctx], pm,
		       (void *)(u_long)pm->pm_physaddr);
		Debugger();
	}
#endif
	/* We should verify it has not been stolen and reallocated... */
	DPRINTF(PDB_CTX_ALLOC, ("ctx_free: cpu%d freeing ctx %d\n",
		cpu_number(), oldctx));
	ci->ci_ctxbusy[oldctx] = 0UL;
	pm->pm_ctx[cpunum] = 0;
	LIST_REMOVE(pm, pm_list[cpunum]);
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
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	pv_entry_t pvh;

	KASSERT(mutex_owned(&pmap_lock));

	pvh = &md->mdpg_pvh;
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
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	pv_entry_t pvh, npv, pv;
	int64_t data = 0;

	KASSERT(mutex_owned(&pmap_lock));

	pvh = &md->mdpg_pvh;

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
	struct vm_page_md *md;
	pv_entry_t pv;
	vaddr_t va;
	int rv;

#if 0
	/*
	 * Why is this?
	 */
	if (CPU_ISSUN4US || CPU_ISSUN4V)
		return;
#endif

	KASSERT(mutex_owned(&pmap_lock));

	DPRINTF(PDB_ENTER, ("pmap_page_uncache(%llx)\n",
	    (unsigned long long)pa));
	pg = PHYS_TO_VM_PAGE(pa);
	md = VM_PAGE_TO_MD(pg);
	pv = &md->mdpg_pvh;
	while (pv) {
		va = pv->pv_va & PV_VAMASK;
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
			KASSERT(data & TLB_V);
			rv = pseg_set(pv->pv_pmap, va, data & ~TLB_CV, 0);
			if (rv & 1)
				panic("pmap_page_cache: pseg_set needs"
				    " spare! rv=%d\n", rv);
		}
		if (pmap_is_on_mmu(pv->pv_pmap)) {
			/* Force reload -- cache bits have changed */
			KASSERT(pmap_ctx(pv->pv_pmap)>=0);
			tsb_invalidate(va, pv->pv_pmap);
			tlb_flush_pte(va, pv->pv_pmap);
		}
		pv = pv->pv_next;
	}
}

/*
 * Some routines to allocate and free PTPs.
 */
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
pmap_free_page(paddr_t pa, sparc64_cpuset_t cs)
{
	struct vm_page *pg = PHYS_TO_VM_PAGE(pa);

	dcache_flush_page_cpuset(pa, cs);
	uvm_pagefree(pg);
}

static void
pmap_free_page_noflush(paddr_t pa)
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
	struct vm_page_md *md;
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
	md = VM_PAGE_TO_MD(pg);
	for (pv = &md->mdpg_pvh; pv; pv = pv->pv_next)
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
pmap_testout(void)
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
	pmap_free_page(pa, cpus_active);
}
#endif

void
pmap_update(struct pmap *pmap)
{

	if (pmap->pm_refs > 0) {
		return;
	}
	pmap->pm_refs = 1;
	pmap_activate_pmap(pmap);
}

/*
 * pmap_copy_page()/pmap_zero_page()
 *
 * we make sure that the destination page is flushed from all D$'s
 * before we perform the copy/zero.
 */
extern int cold;
void
pmap_copy_page(paddr_t src, paddr_t dst)
{

	if (!cold)
		dcache_flush_page_all(dst);
	pmap_copy_page_phys(src, dst);
}

void
pmap_zero_page(paddr_t pa)
{

	if (!cold)
		dcache_flush_page_all(pa);
	pmap_zero_page_phys(pa);
}
