/*	$NetBSD: pmap.c,v 1.61 2000/06/26 19:45:54 pk Exp $	*/
#undef	NO_VCACHE /* Don't forget the locked TLB in dostart */
#define HWREF 1 
#undef	BOOT_DEBUG
#undef	BOOT1_DEBUG
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

#include "opt_ddb.h"

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

#include <vm/vm.h>

#include <uvm/uvm.h>

#include <machine/pcb.h>
#include <machine/sparc64.h>
#include <machine/ctlreg.h>
#include <machine/openfirm.h>
#include <machine/kcore.h>

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

paddr_t cpu0paddr;/* XXXXXXXXXXXXXXXX */

/*
 * Support for big page sizes.  This maps the page size to the
 * page bits.  That is: these are the bits between 8K pages and
 * larger page sizes that cause aliasing.
 */
struct page_size_map page_size_map[] = {
#ifdef DEBUG
	{ 0, TLB_8K&0  },	/* Disable large pages */
#endif
	{ (4*1024*1024-1) & ~(8*1024-1), TLB_4M&0 },
	{ (512*1024-1) & ~(8*1024-1), TLB_512K&0  },
	{ (64*1024-1) & ~(8*1024-1), TLB_64K&0  },
	{ (8*1024-1) & ~(8*1024-1), TLB_8K  },
	{ 0, TLB_8K&0  }
};

extern int64_t asmptechk __P((union sun4u_data* pseg[], int addr)); /* DEBUG XXXXX */

/* These routines are in assembly to allow access thru physical mappings */
#if 1
extern int64_t pseg_get __P((struct pmap*, vaddr_t addr));
extern int pseg_set __P((struct pmap*, vaddr_t addr, int64_t tte, paddr_t spare));
#else
static int64_t pseg_get __P((struct pmap*, vaddr_t addr));
static int pseg_set __P((struct pmap*, vaddr_t addr, int64_t tte, paddr_t spare));

static int64_t pseg_get(struct pmap* pm, vaddr_t addr) {
	paddr_t *pdir, *ptbl;
	
	if ((pdir = (paddr_t *)ldda(&pm->pm_segs[va_to_seg(addr)],
				    ASI_PHYS_CACHED)) &&
	    (ptbl = (paddr_t *)ldda(&pdir[va_to_dir(addr)], ASI_PHYS_CACHED)))
		return  (ldda(&pdir[va_to_dir(addr)], ASI_PHYS_CACHED));
	return (0);
}

static int pseg_set(struct pmap* pm, vaddr_t addr, int64_t tte, paddr_t spare) {
	int i, j, k, s;
	paddr_t *pdir, *ptbl;

	if (!(pdir = (paddr_t *)ldda(&pm->pm_segs[va_to_seg(addr)],
	    ASI_PHYS_CACHED))) {
		if (!spare) return (1);
		stda(&pm->pm_segs[va_to_seg(addr)], ASI_PHYS_CACHED, spare);
		pdir = spare;
		spare = NULL;
	}
	if (!(ptbl = (paddr_t *)ldda(&pdir[va_to_dir(addr)], ASI_PHYS_CACHED))) {
		if (!spare) return (1);
		stda(&pdir[va_to_dir(addr)], ASI_PHYS_CACHED, spare);
		ptbl = spare;
		spare = NULL;
	}
	stda(&ptbl[va_to_pte(addr)], ASI_PHYS_CACHED, tte);
	return (0);
}
#endif

extern vm_page_t vm_page_alloc1 __P((void));
extern void vm_page_free1 __P((vm_page_t));


#ifdef DEBUG
#ifdef __STDC__
#define	ASSERT(x)	\
	if (!(x)) panic("%s at line %d: assertion failed\n", #x, __LINE__);
#else
#define	ASSERT(x)	\
	if (!(x)) panic("%s at line %d: assertion failed\n", "x", __LINE__);
#endif
#else
#define ASSERT(x)
#endif

/*
 * For each vm_page_t, there is a list of all currently valid virtual
 * mappings of that page.  An entry is a pv_entry_t, the list is pv_table.
 * XXX really should do this as a part of the higher level code.
 */
typedef struct pv_entry {
	struct pv_entry	*pv_next;	/* next pv_entry */
	struct pmap	*pv_pmap;	/* pmap where mapping lies */
	vaddr_t	pv_va;		/* virtual address for mapping */
} *pv_entry_t;
/* PV flags encoded in the low bits of the VA of the first pv_entry */

/*
 * Diatribe on ref/mod counting:
 *
 * First of all, ref/mod info must be non-volatile.  Hence we need to keep it
 * in the pv_entry structure for each page.  (We could bypass this for the 
 * vm_page_t, but that's a long story....)
 * 
 * This architecture has nice, fast traps with lots of space for software bits
 * in the TTE.  To accellerate ref/mod counts we make use of these features.
 *
 * When we map a page initially, we place a TTE in the page table.  It's 
 * inserted with the TLB_W and TLB_ACCESS bits cleared.  If a page is really
 * writeable we set the TLB_REAL_W bit for the trap handler.
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
#define PV_WE		0x10LL		/* Debug -- track if this page was ever writable */
#define PV_MASK		(0x01fLL)
#define PV_VAMASK	(~(NBPG-1))
#define PV_MATCH(pv,va)	(!((((pv)->pv_va)^(va))&PV_VAMASK))
#define PV_SETVA(pv,va) ((pv)->pv_va = (((va)&PV_VAMASK)|(((pv)->pv_va)&PV_MASK)))

pv_entry_t	pv_table;	/* array of entries, one per page */
static struct pool pv_pool;
extern void	pmap_remove_pv __P((struct pmap *pm, vaddr_t va, paddr_t pa));
extern void	pmap_enter_pv __P((struct pmap *pm, vaddr_t va, paddr_t pa));
extern void	pmap_page_cache __P((paddr_t pa, int mode));

/*
 * First and last managed physical addresses.  XXX only used for dumping the system.
 */
paddr_t	vm_first_phys, vm_num_phys;

u_int64_t first_phys_addr;
#define pa_index(pa)		atop((pa) - first_phys_addr)
#define pa_to_pvh(pa)		(&pv_table[pa_index(pa)])



/*
 * Here's the CPU TSB stuff.  It's allocated in pmap_bootstrap.
 */
pte_t *tsb;
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

static int npgs;
static u_int nextavail;
static struct mem_region memlist[8]; /* Pick a random size here */

vaddr_t	vmmap;			/* one reserved MI vpage for /dev/mem */

struct mem_region *mem, *avail, *orig;
int memsize;

static int memh = 0, vmemh = 0;	/* Handles to OBP devices */

static int pmap_initialized;

int avail_start, avail_end;	/* These are used by ps & family */

static int ptelookup_va __P((vaddr_t va)); /* sun4u */
#if notyet
static void tsb_enter __P((int ctx, int64_t va, int64_t data));
#endif
static void pmap_pinit __P((struct pmap *));
static void pmap_release __P((pmap_t));

struct pmap_stats {
	int	ps_unlink_pvfirst;	/* # of pv_unlinks on head */
	int	ps_unlink_pvsearch;	/* # of pv_unlink searches */
	int	ps_changeprots;		/* # of calls to changeprot */
	int	ps_useless_changeprots;	/* # of changeprots for wiring */
	int	ps_enter_firstpv;	/* pv heads entered */
	int	ps_enter_secondpv;	/* pv nonheads entered */
	int	ps_useless_changewire;	/* useless wiring changes */
	int	ps_npg_prot_all;	/* # of active pages protected */
	int	ps_npg_prot_actual;	/* # pages actually affected */
} pmap_stats;

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
int	pmapdebug = 0/*PDB_ALIAS|PDB_CTX_ALLOC|PDB_CTX_STEAL|PDB_EXTRACT|PDB_CREATE|PDB_DESTROY|PDB_CHANGEPROT|PDB_ENTER|PDB_REMOVE|PDB_DEMAP|*/;
/* Number of H/W pages stolen for page tables */
int	pmap_pages_stolen = 0;
#endif

#ifdef NOTDEF_DEBUG
void pv_check __P((void));
void
pv_check()
{
	int i, j, s;
	
	s = splhigh();
	for (i = 0; i < physmem; i++) {
		struct pv_entry *pv;
		for (pv = &pv_table[i]; pv; pv = pv->pv_next) {
			if (pv->pv_pmap &&
			    !(pseg_get(pv->pv_pmap, pv->pv_va)&TLB_V)) {
		printf("pv_check(): unreferenced pv=%p pa=%p va=%p pm=%p\n",
		       i, ptoa(first_phys_addr+i), pv->pv_va, pv->pv_pmap);
				Debugger();
			}
		}
	}
	splx(s);
}
#else
#define pv_check()
#endif

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
paddr_t *ctxbusy;	
int numctx;
#define CTXENTRY	(sizeof(paddr_t))
#define CTXSIZE		(numctx*CTXENTRY)

#define	pmap_get_page(p)	uvm_page_physget((p));


/*
 * Enter a TTE into the kernel pmap only.  Don't do anything else.
 */
static void pmap_enter_kpage __P((vaddr_t, int64_t));
static void
pmap_enter_kpage(va, data)
	vaddr_t va;
	int64_t data;
{
	paddr_t newp;

	newp = NULL;
	while (pseg_set(pmap_kernel(), va, data, newp) != NULL) {
		pmap_get_page(&newp);
		pmap_zero_page(newp);
#ifdef DEBUG
		enter_stats.ptpneeded ++;
#endif
#ifdef BOOT1_DEBUG
		prom_printf(
			"pseg_set: pm=%p va=%p data=%lx newp %lx\r\n",
			pmap_kernel(), va, (long)data, (long)newp);
		{int i; for (i=0; i<140000000; i++) ;}
#endif
	}
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
pmap_bootstrap(kernelstart, kernelend, maxctx)
	u_long kernelstart, kernelend;
	u_int maxctx;
{
	extern int data_start[], end[];	/* start of data segment */
	extern int msgbufmapped;
	struct mem_region *mp, *mp1;
	int msgbufsiz;
	int pcnt;
	size_t s, sz;
	int i, j;
	int64_t data;
	vaddr_t va;
	u_int64_t phys_msgbuf;
	paddr_t newkp;
	vaddr_t newkv, firstaddr, intstk;
	vsize_t kdsize, ktsize;
#ifdef DEBUG
	int opmapdebug = pmapdebug;
	pmapdebug = 0;
#endif

#ifdef BOOT_DEBUG
	prom_printf("Entered pmap_bootstrap.\r\n");
#endif
	/*
	 * set machine page size
	 */
	uvmexp.pagesize = NBPG;
	uvm_setpagesize();
	/*
	 * Find out how big the kernel's virtual address
	 * space is.  The *$#@$ prom loses this info
	 */
	if ((vmemh = OF_finddevice("/virtual-memory")) == -1) {
		prom_printf("no virtual-memory?");
		OF_exit();
	}
	bzero((caddr_t)memlist, sizeof(memlist));
	if (OF_getprop(vmemh, "available", memlist, sizeof(memlist)) <= 0) {
		prom_printf("no vmemory avail?");
		OF_exit();
	}

#ifdef BOOT_DEBUG
	/* print out mem list */
	prom_printf("Available virutal memory:\r\n");
	for (mp = memlist; mp->size; mp++) {
		prom_printf("memlist start %p size %lx\r\n", (void *)(u_long)mp->start,
		    (u_long)mp->size);
	}
	prom_printf("End of available virutal memory\r\n");
#endif
	/* 
	 * Get hold or the message buffer.
	 */
	msgbufp = (struct kern_msgbuf *)MSGBUF_VA;
/* XXXXX -- increase msgbufsiz for uvmhist printing */
	msgbufsiz = 4*NBPG /* round_page(sizeof(struct msgbuf)) */;
#ifdef BOOT_DEBUG
	prom_printf("Trying to allocate msgbuf at %lx, size %lx\r\n", 
		    (long)msgbufp, (long)msgbufsiz);
#endif
	if ((long)msgbufp !=
	    (long)(phys_msgbuf = prom_claim_virt((vaddr_t)msgbufp, msgbufsiz)))
		prom_printf(
		    "cannot get msgbuf VA, msgbufp=%p, phys_msgbuf=%lx\r\n", 
		    (void *)msgbufp, (long)phys_msgbuf);
	phys_msgbuf = prom_get_msgbuf(msgbufsiz, MMU_PAGE_ALIGN);
#ifdef BOOT_DEBUG
	prom_printf("We should have the memory at %lx, let's map it in\r\n", 
		    phys_msgbuf);
#endif
	if (prom_map_phys(phys_msgbuf, msgbufsiz, (vaddr_t)msgbufp, 
			  -1/* sunos does this */) == -1)
		prom_printf("Failed to map msgbuf\r\n");
#ifdef BOOT_DEBUG
	else
		prom_printf("msgbuf mapped at %p\r\n", (void *)msgbufp);
#endif
	msgbufmapped = 1;	/* enable message buffer */
	initmsgbuf((caddr_t)msgbufp, msgbufsiz);

	/* 
	 * Record kernel mapping -- we will map these with a permanent 4MB
	 * TLB entry when we initialize the CPU later.
	 */
#ifdef BOOT_DEBUG
	prom_printf("translating kernelstart %p\r\n", (void *)kernelstart);
#endif
	ktext = kernelstart;
	ktextp = prom_vtop(kernelstart);

	kdata = (vaddr_t)data_start;
	kdatap = prom_vtop(kdata);
	ekdata = (vaddr_t)end;

	/*
	 * Find the real size of the kernel.  Locate the smallest starting
	 * address > kernelstart.
	 */
	for (mp1 = mp = memlist; mp->size; mp++) {
		/*
		 * Check whether this region is at the end of the kernel.
		 */
		if (mp->start >= ekdata && (mp1->start < ekdata || 
						mp1->start > mp->start))
			mp1 = mp;
	}
	if (mp1->start < kdata)
		prom_printf("Kernel at end of vmem???\r\n");

#ifdef BOOT1_DEBUG
	prom_printf("The kernel data is mapped at %lx, next free seg: %lx, %lx\r\n",
		    (long)kdata, (u_long)mp1->start, (u_long)mp1->size);
#endif	
	/* 
	 * This it bogus and will be changed when the kernel is rounded to 4MB.
	 */
	firstaddr = (ekdata + 07) & ~ 07;	/* Longword align */

#if 1
#define	valloc(name, type, num) (name) = (type *)firstaddr; firstaddr += (num)
#else
#define	valloc(name, type, num) (name) = (type *)firstaddr; firstaddr = \
	(vaddr_t)((name)+(num))
#endif
#define MEG		(1<<20) /* 1MB */

	/*
	 * Since we can't always give the loader the hint to align us on a 4MB
	 * boundary, we will need to do the alignment ourselves.  First
	 * allocate a new 4MB aligned segment for the kernel, then map it
	 * in, copy the kernel over, swap mappings, then finally, free the
	 * old kernel.  Then we can continue with this.
	 *
	 * We'll do the data segment up here since we know how big it is.
	 * We'll do the text segment after we've read in the PROM translations
	 * so we can figure out its size.
	 */
	kdsize = round_page(ekdata - kdata);

	if (!(kdatap & (4*MEG-1))) {
		/* We were at a 4MB boundary -- claim the rest */
		psize_t szdiff = 4*MEG - kdsize;

		/* Claim the rest of the physical page. */
		newkp = kdatap + kdsize;
		newkv = kdata + kdsize;
		if (newkp != prom_claim_phys(newkp, szdiff)) {
			prom_printf("pmap_bootstrap: could not claim physical "
				"dseg extention at %lx size %lx\r\n", newkp, szdiff);
			goto remap_data;
		}

		/* And the rest of the virtual page. */
		if (prom_claim_virt(newkv, szdiff) != newkv)
			prom_printf("pmap_bootstrap: could not claim virtual "
				"dseg extention at size %lx\r\n", newkv, szdiff);

		/* Make sure all 4MB are mapped */
		prom_map_phys(newkp, szdiff, newkv, -1);
	} else {
remap_data:
		/* 
		 * Either we're not at a 4MB boundary or we can't get the rest
		 * of the 4MB extension.  We need to move the data segment.
		 */

#ifdef BOOT1_DEBUG
		prom_printf("Allocating new %lx kernel data at 4MB boundary\r\n",
		    (u_long)kdsize);
#endif
		if ((newkp = prom_alloc_phys(4*MEG, 4*MEG)) == 0 ) {
			prom_printf("Cannot allocate new kernel\r\n");
			OF_exit();
		}
#ifdef BOOT1_DEBUG
		prom_printf("Allocating new va for buffer at %p\r\n",
		    (void *)newkp);
#endif
		if ((newkv = (vaddr_t)prom_alloc_virt(4*MEG, 8)) ==
		    (vaddr_t)-1) {
			prom_printf("Cannot allocate new kernel va\r\n");
			OF_exit();
		}
#ifdef BOOT1_DEBUG
		prom_printf("Mapping in buffer %lx at %lx\r\n",
		    (u_long)newkp, (u_long)newkv);
#endif
		prom_map_phys(newkp, 4*MEG, (vaddr_t)newkv, -1); 
#ifdef BOOT1_DEBUG
		prom_printf("Copying %ld bytes kernel data...", kdsize);
#endif
		bzero((void *)newkv, 4*MEG);
		bcopy((void *)kdata, (void *)newkv,
		    kdsize);
#ifdef BOOT1_DEBUG
		prom_printf("done.  Swapping maps..unmap new\r\n");
#endif
		prom_unmap_virt((vaddr_t)newkv, 4*MEG);
#ifdef BOOT_DEBUG
		prom_printf("remap old ");
#endif
#if 0
		/*
		 * calling the prom will probably require reading part of the
		 * data segment so we can't do this.  */
		prom_unmap_virt((vaddr_t)kdatap, kdsize);
#endif
		prom_map_phys(newkp, 4*MEG, kdata, -1); 
		/*
		 * we will map in 4MB, more than we allocated, to allow
		 * further allocation
		 */
#ifdef BOOT1_DEBUG
		prom_printf("free old\r\n");
#endif
		prom_free_phys(kdatap, kdsize);
		kdatap = newkp;
		
#ifdef BOOT1_DEBUG
		prom_printf("pmap_bootstrap: firstaddr is %lx virt (%lx phys)"
		    "avail for kernel\r\n", (u_long)firstaddr,
		    (u_long)prom_vtop(firstaddr));
#endif
	}

	/*
	 * Find out how much RAM we have installed.
	 */
#ifdef BOOT_DEBUG
	prom_printf("pmap_bootstrap: getting phys installed\r\n");
#endif
	if ((memh = OF_finddevice("/memory")) == -1) {
		prom_printf("no memory?");
		OF_exit();
	}
	memsize = OF_getproplen(memh, "reg") + 2 * sizeof(struct mem_region);
	valloc(mem, struct mem_region, memsize);
	bzero((caddr_t)mem, memsize);
	if (OF_getprop(memh, "reg", mem, memsize) <= 0) {
		prom_printf("no memory installed?");
		OF_exit();
	}

#ifdef BOOT1_DEBUG
	/* print out mem list */
	prom_printf("Installed physical memory:\r\n");
	for (mp = mem; mp->size; mp++) {
		prom_printf("memlist start %lx size %lx\r\n",
		    (u_long)mp->start, (u_long)mp->size);
	}

	prom_printf("Calculating physmem:");
#endif

	for (mp = mem; mp->size; mp++)
		physmem += btoc(mp->size);

#ifdef BOOT1_DEBUG
	prom_printf(" result %x or %d pages\r\n", (int)physmem, (int)physmem);
#endif
	/* 
	 * Calculate approx TSB size.  This probably needs tweaking.
	 */
	if (physmem < 64 * 1024 * 1024)
		tsbsize = 0;
	else if (physmem < 512 * 1024 * 1024)
		tsbsize = 1;
	else
		tsbsize = 2;

	/*
	 * Save the prom translations
	 */
	sz = OF_getproplen(vmemh, "translations");
	valloc(prom_map, struct prom_map, sz);
	if (OF_getprop(vmemh, "translations", (void*)prom_map, sz) <= 0) {
		prom_printf("no translations installed?");
		OF_exit();
	}
	prom_map_size = sz / sizeof(struct prom_map);
#ifdef BOOT_DEBUG
	/* print out mem list */
	prom_printf("Prom xlations:\r\n");
	for (i = 0; i < prom_map_size; i++) {
		prom_printf("start %016lx size %016lx tte %016lx\r\n", 
		    (u_long)prom_map[i].vstart, (u_long)prom_map[i].vsize,
		    (u_long)prom_map[i].tte);
	}
	prom_printf("End of prom xlations\r\n");
#endif
	/*
	 * Hunt for the kernel text segment and figure out it size and
	 * alignment.  
	 */
	for (i = 0; i < prom_map_size; i++) 
		if (prom_map[i].vstart == ktext)
			break;
	if (i == prom_map_size) 
		panic("No kernel text segment!\r\n");
	ktsize = prom_map[i].vsize;
	ektext = ktext + ktsize;

	if (ktextp & (4*MEG-1)) {
#ifdef BOOT1_DEBUG
		prom_printf("Allocating new %lx kernel text at 4MB boundary\r\n",
		    (u_long)ktsize);
#endif
		if ((newkp = prom_alloc_phys(ktsize, 4*MEG)) == 0 ) {
			prom_printf("Cannot allocate new kernel text\r\n");
			OF_exit();
		}
#ifdef BOOT1_DEBUG
		prom_printf("Allocating new va for buffer at %p\r\n",
		    (void *)newkp);
#endif
		if ((newkv = (vaddr_t)prom_alloc_virt(ktsize, 8)) ==
		    (vaddr_t)-1) {
			prom_printf("Cannot allocate new kernel text va\r\n");
			OF_exit();
		}
#ifdef BOOT1_DEBUG
		prom_printf("Mapping in buffer %lx at %lx\r\n",
		    (u_long)newkp, (u_long)newkv);
#endif
		prom_map_phys(newkp, ktsize, (vaddr_t)newkv, -1); 
#ifdef BOOT1_DEBUG
		prom_printf("Copying %ld bytes kernel text...", ktsize);
#endif
		bcopy((void *)ktext, (void *)newkv,
		    ktsize);
#ifdef BOOT1_DEBUG
		prom_printf("done.  Swapping maps..unmap new\r\n");
#endif
		prom_unmap_virt((vaddr_t)newkv, 4*MEG);
#ifdef BOOT_DEBUG
		prom_printf("remap old ");
#endif
#if 0
		/*
		 * calling the prom will probably require reading part of the
		 * text segment so we can't do this.  
		 */
		prom_unmap_virt((vaddr_t)ktextp, ktsize);
#endif
		prom_map_phys(newkp, ktsize, ktext, -1); 
		/*
		 * we will map in 4MB, more than we allocated, to allow
		 * further allocation
		 */
#ifdef BOOT1_DEBUG
		prom_printf("free old\r\n");
#endif
		prom_free_phys(ktextp, ktsize);
		ktextp = newkp;
		
#ifdef BOOT1_DEBUG
		prom_printf("pmap_bootstrap: firstaddr is %lx virt (%lx phys)"
		    "avail for kernel\r\n", (u_long)firstaddr,
		    (u_long)prom_vtop(firstaddr));
#endif

		/*
		 * Re-fetch translations -- they've certainly changed.
		 */
		if (OF_getprop(vmemh, "translations", (void*)prom_map, sz) <=
			0) {
			prom_printf("no translations installed?");
			OF_exit();
		}
#ifdef BOOT_DEBUG
		/* print out mem list */
		prom_printf("New prom xlations:\r\n");
		for (i = 0; i < prom_map_size; i++) {
			prom_printf("start %016lx size %016lx tte %016lx\r\n",
				(u_long)prom_map[i].vstart, 
				(u_long)prom_map[i].vsize,
				(u_long)prom_map[i].tte);
		}
		prom_printf("End of prom xlations\r\n");
#endif
	} 
	ektextp = ktextp + ktsize;

	/*
	 * Here's a quick in-lined reverse bubble sort.  It gets rid of
	 * any translations inside the kernel data VA range.
	 */
	for(i = 0; i < prom_map_size; i++) {
		if (prom_map[i].vstart >= kdata &&
		    prom_map[i].vstart <= firstaddr) {
			prom_map[i].vstart = 0;
			prom_map[i].vsize = 0;
		}
		if (prom_map[i].vstart >= ktext &&
		    prom_map[i].vstart <= ektext) {
			prom_map[i].vstart = 0;
			prom_map[i].vsize = 0;
		}
		for(j = i; j < prom_map_size; j++) {
			if (prom_map[j].vstart >= kdata &&
			    prom_map[j].vstart <= firstaddr)
				continue;	/* this is inside the kernel */
			if (prom_map[j].vstart >= ktext &&
			    prom_map[j].vstart <= ektext)
				continue;	/* this is inside the kernel */
			if (prom_map[j].vstart > prom_map[i].vstart) {
				struct prom_map tmp;
				tmp = prom_map[i];
				prom_map[i] = prom_map[j];
				prom_map[j] = tmp;
			}
		}
	}
#ifdef BOOT_DEBUG
	/* print out mem list */
	prom_printf("Prom xlations:\r\n");
	for (i = 0; i < prom_map_size; i++) {
		prom_printf("start %016lx size %016lx tte %016lx\r\n", 
		    (u_long)prom_map[i].vstart, (u_long)prom_map[i].vsize,
		    (u_long)prom_map[i].tte);
	}
	prom_printf("End of prom xlations\r\n");
#endif

	/*
	 * Allocate a 64MB page for the cpu_info structure now.
	 */
	if ((cpu0paddr = prom_alloc_phys(8*NBPG, 8*NBPG)) == 0 ) {
		prom_printf("Cannot allocate new cpu_info\r\n");
		OF_exit();
	}


	/*
	 * Now the kernel text segment is in its final location we can try to
	 * find out how much memory really is free.  
	 */
	sz = OF_getproplen(memh, "available") + sizeof(struct mem_region);
	valloc(orig, struct mem_region, sz);
	bzero((caddr_t)orig, sz);
	if (OF_getprop(memh, "available", orig, sz) <= 0) {
		prom_printf("no available RAM?");
		OF_exit();
	}
#ifdef BOOT1_DEBUG
	/* print out mem list */
	prom_printf("Available physical memory:\r\n");
	for (mp = orig; mp->size; mp++) {
		prom_printf("memlist start %lx size %lx\r\n",
		    (u_long)mp->start, (u_long)mp->size);
	}
	prom_printf("End of available physical memory\r\n");
#endif
	valloc(avail, struct mem_region, sz);
	bzero((caddr_t)avail, sz);
	for (pcnt = 0, mp = orig, mp1 = avail; (mp1->size = mp->size);
	    mp++, mp1++) {
		mp1->start = mp->start;
		pcnt++;
	}

	/*
	 * Allocate and initialize a context table
	 */
	numctx = maxctx;
	valloc(ctxbusy, paddr_t, CTXSIZE);
	bzero((caddr_t)ctxbusy, CTXSIZE);

	/*
	 * Allocate our TSB.
	 *
	 * We will use the left over space to flesh out the kernel pmap.
	 */
#ifdef BOOT1_DEBUG
	prom_printf("firstaddr before TSB=%lx\r\n", (u_long)firstaddr);
#endif
	firstaddr = ((firstaddr + TSBSIZE - 1) & ~(TSBSIZE-1)); 
#ifdef BOOT_DEBUG
	i = (firstaddr + (NBPG-1)) & ~(NBPG-1);	/* First, page align */
	if ((int)firstaddr < i) {
		prom_printf("TSB alloc fixup failed\r\n");
		prom_printf("frobbed i, firstaddr before TSB=%x, %lx\r\n",
		    (int)i, (u_long)firstaddr);
		panic("TSB alloc\n");
		OF_exit();
	}
	prom_printf("frobbed i, firstaddr before TSB=%x, %lx\r\n", (int)i,
	    (u_long)firstaddr);
#endif
	valloc(tsb, pte_t, TSBSIZE);
	bzero(tsb, TSBSIZE);

#ifdef BOOT1_DEBUG
	prom_printf("firstaddr after TSB=%lx\r\n", (u_long)firstaddr);
	prom_printf("TSB allocated at %p size %08x\r\n", (void*)tsb,
	    (int)TSBSIZE);
#endif

	/* initialize pv_list stuff */
	first_phys_addr = mem->start;
#if 0
	valloc(pv_table, struct pv_entry, sizeof(struct pv_entry)*physmem);
	bzero((caddr_t)pv_table, sizeof(struct pv_entry)*physmem);
#ifdef BOOT1_DEBUG
	prom_printf("Allocating pv_table at %lx,%lx\r\n", (u_long)pv_table, 
		    (u_long)sizeof(struct pv_entry)*physmem);
#endif
#endif

#ifdef BOOT1_DEBUG
	prom_printf("firstaddr after pmap=%08lx\r\n", (u_long)firstaddr);
#endif

	/*
	 * Page align all regions.  
	 * Non-page memory isn't very interesting to us.
	 * Also, sort the entries for ascending addresses.
	 * 
	 * And convert from virtual to physical addresses.
	 */
	
#ifdef BOOT_DEBUG
	prom_printf("kernel virtual size %08lx - %08lx\r\n",
	    (u_long)kernelstart, (u_long)firstaddr);
#endif
	kdata = kdata & ~PGOFSET;
	ekdata = firstaddr;
	ekdata = (ekdata + PGOFSET) & ~PGOFSET;
#ifdef BOOT1_DEBUG
	prom_printf("kernel virtual size %08lx - %08lx\r\n",
	    (u_long)kernelstart, (u_long)kernelend);
#endif
	ekdatap = ekdata - kdata + kdatap;
	/* Switch from vaddrs to paddrs */
	if(ekdatap > (kdatap + 4*MEG)) {
		prom_printf("Kernel size exceeds 4MB\r\n");
		panic("kernel segment size exceeded\n");
		OF_exit();
	}

#ifdef BOOT1_DEBUG
	/* print out mem list */
	prom_printf("Available %lx physical memory before cleanup:\r\n",
	    (u_long)avail);
	for (mp = avail; mp->size; mp++) {
		prom_printf("memlist start %lx size %lx\r\n", 
			    (u_long)mp->start, 
			    (u_long)mp->size);
	}
	prom_printf("End of available physical memory before cleanup\r\n");
	prom_printf("kernel physical text size %08lx - %08lx\r\n",
	    (u_long)ktextp, (u_long)ektextp);
	prom_printf("kernel physical data size %08lx - %08lx\r\n",
	    (u_long)kdatap, (u_long)ekdatap);
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
		avail->start += NBPG;
		avail->size -= NBPG;
	}
	/*
	 * Now we need to remove the area we valloc'ed from the available
	 * memory lists.  (NB: we may have already alloc'ed the entire space).
	 */
	npgs = 0;
	for (mp = avail; mp->size; mp++) {
		/*
		 * Check whether this region holds all of the kernel.
		 */
		s = mp->start + mp->size;
		if (mp->start < kdatap && s > (kdatap + 4*MEG)) {
			avail[pcnt].start = kdatap + 4*MEG;
			avail[pcnt++].size = s - kdatap;
			mp->size = kdatap - mp->start;
		}
		/*
		 * Look whether this regions starts within the kernel.
		 */
		if (mp->start >= kdatap && mp->start < (kdatap + 4*MEG)) {
			s = ekdatap - mp->start;
			if (mp->size > s)
				mp->size -= s;
			else
				mp->size = 0;
			mp->start = (kdatap + 4*MEG);
		}
		/*
		 * Now look whether this region ends within the kernel.
		 */
		s = mp->start + mp->size;
		if (s > kdatap && s < (kdatap + 4*MEG))
			mp->size -= s - kdatap;
		/*
		 * Now page align the start of the region.
		 */
		s = mp->start % NBPG;
		if (mp->size >= s) {
			mp->size -= s;
			mp->start += s;
		}
		/*
		 * And now align the size of the region.
		 */
		mp->size -= mp->size % NBPG;
		/*
		 * Check whether some memory is left here.
		 */
		if (mp->size == 0) {
			bcopy(mp + 1, mp,
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
			bcopy(mp1, mp1 + 1, (char *)mp - (char *)mp1);
			mp1->start = s;
			mp1->size = sz;
		}
		/* 
		 * In future we should be able to specify both allocated
		 * and free.
		 */
		uvm_page_physload(
			atop(mp->start),
			atop(mp->start+mp->size),
			atop(mp->start),
			atop(mp->start+mp->size),
			VM_FREELIST_DEFAULT);
	}

#if 0
	/* finally, free up any space that valloc did not use */
	prom_unmap_virt((vaddr_t)ekdatap, (kdatap + (4*MEG)) - ekdatap);
	if (ekdatap < (kdatap + (4*MEG))) {
		uvm_page_physload(atop(ekdatap), atop(kdatap + (4*MEG)),
			atop(ekdatap), atop(kdatap + (4*MEG)),
			VM_FREELIST_DEFAULT);
	}
#endif

#ifdef BOOT_DEBUG
	/* print out mem list */
	prom_printf("Available physical memory after cleanup:\r\n");
	for (mp = avail; mp->size; mp++) {
		prom_printf("avail start %lx size %lx\r\n", 
			    (long)mp->start, (long)mp->size);
	}
	prom_printf("End of available physical memory after cleanup\r\n");
#endif
	/*
	 * Allocate and clear out pmap_kernel()->pm_segs[]
	 */
	{
		paddr_t newp;

		do {
			pmap_get_page(&newp);
			pmap_zero_page(newp);
		} while (!newp); /* Throw away page zero */
		pmap_kernel()->pm_segs=(paddr_t *)(u_long)newp;
		pmap_kernel()->pm_physaddr = newp;
		/* mark kernel context as busy */
		((paddr_t*)ctxbusy)[0] = (int)pmap_kernel()->pm_physaddr;
	}
	/*
	 * finish filling out kernel pmap.
	 */

#ifdef BOOT_DEBUG
	prom_printf("pmap_kernel()->pm_physaddr = %lx\r\n",
	    (long)pmap_kernel()->pm_physaddr);
#endif
	/*
	 * Tell pmap about our mesgbuf -- Hope this works already
	 */
#ifdef BOOT_DEBUG
	prom_printf("Calling consinit()\r\n");
	consinit();
	prom_printf("Inserting mesgbuf into pmap_kernel()\r\n");
#endif
	/* it's not safe to call pmap_enter so we need to do this ourselves */
	va = (vaddr_t)msgbufp;
	while (msgbufsiz) {

		prom_map_phys(phys_msgbuf, NBPG, (vaddr_t)msgbufp, -1); 
		data = TSB_DATA(0 /* global */, 
			TLB_8K,
			phys_msgbuf,
			1 /* priv */,
			1 /* Write */,
			1 /* Cacheable */,
			FORCE_ALIAS /* ALIAS -- Disable D$ */,
			1 /* valid */,
			0 /* IE */);
		pmap_enter_kpage(va, data);
		va += NBPG;
		msgbufsiz -= NBPG;
		phys_msgbuf += NBPG;

	}
		
	/*
	 * Also add a global NFO mapping for page zero.
	 */
	data = TSB_DATA(0 /* global */,
		TLB_8K,
		0 /* Physaddr */,
		1 /* priv */,
		0 /* Write */,
		1 /* Cacheable */,
		0 /* No ALIAS */,
		1 /* valid */,
		0 /* IE */);
	data |= TLB_NFO;
	pmap_enter_kpage(NULL, data);
#ifdef BOOT1_DEBUG
	prom_printf("Done inserting mesgbuf into pmap_kernel()\r\n");
#endif
	
#ifdef BOOT1_DEBUG
	prom_printf("Inserting PROM mappings into pmap_kernel()\r\n");
#endif
	
	for (i = 0; i < prom_map_size; i++)
		if (prom_map[i].vstart && ((prom_map[i].vstart>>32) == 0))
			for (j = 0; j < prom_map[i].vsize; j += NBPG) {
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
					(prom_map[i].tte + j)|
					page_size_map[k].code);
			}
#ifdef BOOT1_DEBUG
	prom_printf("Done inserting PROM mappings into pmap_kernel()\r\n");
#endif

	/*
	 * Fix up start of kernel heap.
	 */
	vmmap = (vaddr_t)(kdata + 4*MEG); /* Start after our locked TLB entry */
	/* Let's keep 1 page of redzone after the kernel */
	vmmap += NBPG;
	{ 
		extern vaddr_t u0[2];
		extern struct pcb* proc0paddr;
		extern void main __P((void));
		paddr_t pa;

		/* Initialize all the pointers to u0 */
		cpcb = (struct pcb *)vmmap;
		proc0paddr = cpcb;
		u0[0] = vmmap;
		/* Allocate some VAs for u0 */
		u0[1] = vmmap + 2*USPACE;

#ifdef BOOT1_DEBUG
	prom_printf("Inserting stack 0 into pmap_kernel() at %p\r\n", vmmap);
#endif

		while (vmmap < u0[1]) {
			int64_t data;

			pmap_get_page(&pa);
			prom_map_phys(pa, NBPG, vmmap, -1);
			data = TSB_DATA(0 /* global */,
				TLB_8K,
				pa,
				1 /* priv */,
				1 /* Write */,
				1 /* Cacheable */,
				FORCE_ALIAS /* ALIAS -- Disable D$ */,
				1 /* valid */,
				0 /* IE */);
			pmap_enter_kpage(vmmap, data);
			vmmap += NBPG;
		}
#ifdef BOOT1_DEBUG
	prom_printf("Done inserting stack 0 into pmap_kernel()\r\n");
#endif

		/* Now map in and initialize our cpu_info structure */
#ifdef DIAGNOSTIC
		vmmap += NBPG; /* redzone -- XXXX do we need one? */
#endif
		if ((vmmap ^ INTSTACK) & VA_ALIAS_MASK) 
			vmmap += NBPG; /* Matchup virtual color for D$ */
		intstk = vmmap;
		cpus = (struct cpu_info *)(intstk+EINTSTACK-INTSTACK);

#ifdef BOOT1_DEBUG
	prom_printf("Inserting cpu_info into pmap_kernel() at %p\r\n", cpus);
#endif
		/* Now map in all 8 pages of cpu_info */
		pa = cpu0paddr;
		for (i=0; i<8; i++) {
			int64_t data;

			prom_map_phys(pa, NBPG, vmmap, -1);
			data = TSB_DATA(0 /* global */,
				TLB_8K,
				pa,
				1 /* priv */,
				1 /* Write */,
				1 /* Cacheable */,
				FORCE_ALIAS /* ALIAS -- Disable D$ */,
				1 /* valid */,
				0 /* IE */);
			pmap_enter_kpage(vmmap, data);
			vmmap += NBPG;
			pa += NBPG;
		}
#ifdef BOOT1_DEBUG
	prom_printf("Initializing cpu_info\r\n");
#endif

		/* Initialize our cpu_info structure */
		bzero((void *)intstk, 8*NBPG);
		cpus->ci_next = NULL; /* Redundant, I know. */
		cpus->ci_curproc = &proc0;
		cpus->ci_cpcb = (struct pcb *)u0[0]; /* Need better source */
		cpus->ci_upaid = CPU_UPAID;
		cpus->ci_number = cpus->ci_upaid; /* How do we figure this out? */
		cpus->ci_fpproc = NULL;
		cpus->ci_spinup = main; /* Call main when we're running. */
		cpus->ci_initstack = (void *)u0[1];
		cpus->ci_paddr = cpu0paddr;
		/* The rest will be done at CPU attach time. */
#ifdef BOOT1_DEBUG
	prom_printf("Done inserting cpu_info into pmap_kernel()\r\n");
#endif
	}
	/*
	 * Set up bounds of allocatable memory for vmstat et al.
	 */
	nextavail = avail->start;
	avail_start = nextavail;
	for (mp = avail; mp->size; mp++)
		avail_end = mp->start+mp->size;
#ifdef DEBUG
	pmapdebug = opmapdebug;
#endif
#ifdef BOOT1_DEBUG
	prom_printf("Finished pmap_bootstrap()\r\n");
#endif

}

/*
 * Initialize anything else for pmap handling.
 * Called during vm_init().
 */
void
pmap_init()
{
	vm_page_t m;
	paddr_t pa;
	psize_t size;
	vaddr_t va;
	struct pglist mlist;

#ifdef NOTDEF_DEBUG
	prom_printf("pmap_init()\r\n");
#endif
	if (PAGE_SIZE != NBPG)
		panic("pmap_init: CLSIZE!=1");

	size = sizeof(struct pv_entry) * physmem;
	TAILQ_INIT(&mlist);
	if (uvm_pglistalloc((psize_t)size, (paddr_t)0, (paddr_t)-1,
		(paddr_t)NBPG, (paddr_t)0, &mlist, 1, 0) != 0)
		panic("cpu_start: no memory");

	va = uvm_km_valloc(kernel_map, size);
	if (va == 0)
		panic("cpu_start: no memory");

	pv_table = (struct pv_entry *)va;
	m = TAILQ_FIRST(&mlist);

	/* Map the pages */
	for (; m != NULL; m = TAILQ_NEXT(m,pageq)) {
		u_int64_t data;

		pa = VM_PAGE_TO_PHYS(m);
		pmap_zero_page(pa);
		data = TSB_DATA(0 /* global */, 
			TLB_8K,
			pa,
			1 /* priv */,
			1 /* Write */,
			1 /* Cacheable */,
			FORCE_ALIAS /* ALIAS -- Disable D$ */,
			1 /* valid */,
			0 /* IE */);
		pmap_enter_kpage(va, data);
		va += NBPG;
	}
	pmap_initialized = 1;

	/* Setup a pool for additional pvlist structures */
	pool_init(&pv_pool, sizeof(struct pv_entry), 0, 0, 0, "pv_entry", 0,
		  NULL, NULL, 0);

	vm_first_phys = avail_start;
	vm_num_phys = avail_end - avail_start;
}

/*
 * How much virtual space is available to the kernel?
 */
void
pmap_virtual_space(start, end)
	vaddr_t *start, *end;
{
	/*
	 * Reserve one segment for kernel virtual memory
	 */
	/* Reserve two pages for pmap_copy_page && /dev/mem */
	*start = (vaddr_t)(vmmap + 2*NBPG);
	*end = VM_MAX_KERNEL_ADDRESS;
#ifdef NOTDEF_DEBUG
	prom_printf("pmap_virtual_space: %x-%x\r\n", *start, *end);
#endif
}

/*
 * Create and return a physical map.
 */
struct pmap *
pmap_create()
{
	struct pmap *pm;

#ifdef DEBUG
	if (pmapdebug & (PDB_CREATE))
		printf("pmap_create()\n");
#endif

	pm = (struct pmap *)malloc(sizeof *pm, M_VMPMAP, M_WAITOK);
	bzero((caddr_t)pm, sizeof *pm);
#ifdef DEBUG
	if (pmapdebug & PDB_CREATE)
		printf("pmap_create(): created %p\n", pm);
#endif
	pmap_pinit(pm);
	return pm;
}

/*
 * Initialize a preallocated and zeroed pmap structure.
 */
void
pmap_pinit(pm)
	struct pmap *pm;
{

	/*
	 * Allocate some segment registers for this pmap.
	 */
	pm->pm_refs = 1;
	if(pm != pmap_kernel()) {
		vm_page_t page;
#ifdef NOTDEF_DEBUG
		printf("pmap_pinit: need to alloc page\n");
#endif
		while ((page = vm_page_alloc1()) == NULL) {
			/*
			 * Let the pager run a bit--however this may deadlock
			 */
#ifdef NOTDEF_DEBUG
			printf("pmap_pinit: calling uvm_wait()\n");
#endif
			uvm_wait("pmap_pinit");
		}
		pm->pm_physaddr = (paddr_t)VM_PAGE_TO_PHYS(page);
		pmap_zero_page(pm->pm_physaddr);
		pm->pm_segs = (paddr_t *)(u_long)pm->pm_physaddr;
		if (!pm->pm_physaddr) panic("pmap_pinit");
#ifdef NOTDEF_DEBUG
		printf("pmap_pinit: segs %p == %p\n", pm->pm_segs, (void*)page->phys_addr);
#endif
		ctx_alloc(pm);
	}
#ifdef DEBUG
	if (pmapdebug & PDB_CREATE)
		printf("pmap_pinit(%x): ctx %d\n", pm, pm->pm_ctx);
#endif
}

/*
 * Add a reference to the given pmap.
 */
void
pmap_reference(pm)
	struct pmap *pm;
{
	pm->pm_refs++;
}

/*
 * Retire the given pmap from service.
 * Should only be called if the map contains no valid mappings.
 */
void
pmap_destroy(pm)
	struct pmap *pm;
{
	if (--pm->pm_refs == 0) {
#ifdef DEBUG
		if (pmapdebug & PDB_DESTROY)
			printf("pmap_destroy: freeing pmap %p\n", pm);
#endif
		pmap_release(pm);
		free((caddr_t)pm, M_VMPMAP);
	}
}

/*
 * Release any resources held by the given physical map.
 * Called when a pmap initialized by pmap_pinit is being released.
 */
void
pmap_release(pm)
	struct pmap *pm;
{
	int i, j, k, s;
	paddr_t *pdir, *ptbl;

#ifdef DIAGNOSTIC
	if(pm == pmap_kernel())
		panic("pmap_release: releasing pmap_kernel()");
#endif

	s=splimp();
	for(i=0; i<STSZ; i++)
		if((pdir = (paddr_t *)(u_long)ldxa((vaddr_t)&pm->pm_segs[i], ASI_PHYS_CACHED))) {
			for (k=0; k<PDSZ; k++) {
				if ((ptbl = (paddr_t *)(u_long)ldxa((vaddr_t)&pdir[k], ASI_PHYS_CACHED))) {
					for (j=0; j<PTSZ; j++) {
						int64_t data = ldxa((vaddr_t)&ptbl[j], ASI_PHYS_CACHED);
						if (data&TLB_V && 
						    IS_VM_PHYSADDR(data&TLB_PA_MASK)) {
#ifdef DEBUG
							printf("pmap_release: pm=%p page %llx still in use\n", pm, 
							       ((u_int64_t)i<<STSHIFT)|((u_int64_t)k<<PDSHIFT)|((u_int64_t)j<<PTSHIFT));
							Debugger();
#endif
							pmap_remove_pv(pm, 
								       (long)((u_int64_t)i<<STSHIFT)|((long)k<<PDSHIFT)|((long)j<<PTSHIFT), 
								       data&TLB_PA_MASK);
						}
					}
					vm_page_free1((vm_page_t)PHYS_TO_VM_PAGE((paddr_t)(u_long)ptbl));
					stxa((paddr_t)(long)&pdir[k], ASI_PHYS_CACHED, NULL);
				}
			}
			vm_page_free1((vm_page_t)PHYS_TO_VM_PAGE((paddr_t)(u_long)pdir));
			stxa((paddr_t)(long)&pm->pm_segs[i], ASI_PHYS_CACHED, NULL);
		}
	vm_page_free1((vm_page_t)PHYS_TO_VM_PAGE((paddr_t)(u_long)pm->pm_segs));
	pm->pm_segs = NULL;
#ifdef NOTDEF_DEBUG
	for (i=0; i<physmem; i++) {
		struct pv_entry *pv;
		for (pv = &pv_table[i]; pv; pv=pv->pv_next) {
			if (pv->pv_pmap == pm) {
				printf("pmap_release(): unreferenced pv=%p pa=%p va=%p pm=%p\n",
				       i, ptoa(first_phys_addr+i), pv->pv_va, pm);
				Debugger();
				pmap_remove_pv(pm, pv->pv_va, i);
				break;
			}
		}
	}
#endif
	splx(s);
	ctx_free(pm);
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
#ifdef DEBUG
	if (pmapdebug&PDB_CREATE)
		printf("pmap_copy(%p, %p, %p, %x, %p)\n",
		       dst_pmap, src_pmap, dst_addr, len, src_addr);
#endif
}

/*
 * Require that all active physical maps contain no
 * incorrect entries NOW.
 */
void
pmap_update()
{
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
#if 1
	int i, j, k, n, m, s;
	paddr_t *pdir, *ptbl;
	/* This is a good place to scan the pmaps for page tables with
	 * no valid mappings in them and free them. */
	
	s = splimp();
	for (i=0; i<STSZ; i++) {
		if ((pdir = (paddr_t *)(u_long)ldxa((vaddr_t)&pm->pm_segs[i], ASI_PHYS_CACHED))) {
			m = 0;
			for (k=0; k<PDSZ; k++) {
				if ((ptbl = (paddr_t *)(u_long)ldxa((vaddr_t)&pdir[k], ASI_PHYS_CACHED))) {
					m++;
					n = 0;
					for (j=0; j<PTSZ; j++) {
						int64_t data = ldxa((vaddr_t)&ptbl[j], ASI_PHYS_CACHED);
						if (data&TLB_V)
							n++;
					}
					if (!n) {
						/* Free the damn thing */
						vm_page_free1((vm_page_t)PHYS_TO_VM_PAGE((paddr_t)(u_long)ptbl));
						stxa((paddr_t)(long)&pdir[k], ASI_PHYS_CACHED, NULL);
					}
				}
			}
			if (!m) {
				/* Free the damn thing */
				vm_page_free1((vm_page_t)PHYS_TO_VM_PAGE((paddr_t)(u_long)pdir));
				stxa((paddr_t)(long)&pm->pm_segs[i], ASI_PHYS_CACHED, NULL);
			}
		}
	}
	splx(s);
#endif
}

#if 0
/*
 * The two following routines are now in locore.s so I can code them in assembly
 * They can bypass the MMU or use VIS bcopy extensions for speed.
 */
/*
 * Fill the given physical page with zeroes.
 */
void
pmap_zero_page(pa)
	paddr_t pa;
{
	/* 
	 * We don't need to worry about flushing caches
	 * since all our virtual caches are write-through.
	 * All we need to do is map the page in somewhere, bzero it,
	 * and unmap it.  However, we need to be sure we don't
	 * map it in anywhere near the kernel or we may lose, badly.
	 */
	bzero((caddr_t)pa, NBPG);
}

/*
 * Copy the given physical source page to its destination.
 *
 * I will code this in assembly RSN.
 */
void
pmap_copy_page(src, dst)
	paddr_t src, dst;
{
	bcopy((caddr_t)src, (caddr_t)dst, NBPG);
}
#endif

/*
 * Activate the address space for the specified process.  If the
 * process is the current process, load the new MMU context.
 */
void
pmap_activate(p)
	struct proc *p;
{
	pmap_t pmap = p->p_vmspace->vm_map.pmap;
	int s;

	/*
	 * This is essentially the same thing that happens in cpu_switch()
	 * when the newly selected process is about to run, except that we
	 * have to make sure to clean the register windows before we set
	 * the new context.
	 */

	s = splpmap();
	if (p == curproc) {
		write_user_windows();
		if (pmap->pm_ctx == NULL)
			ctx_alloc(pmap);
		stxa(CTX_SECONDARY, ASI_DMMU, pmap->pm_ctx);
	}
	splx(s);
}

/*
 * Deactivate the address space of the specified process.
 */
void
pmap_deactivate(p)
	struct proc *p;
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
#if 1
void
pmap_kenter_pa(va, pa, prot)
	vaddr_t va;
	paddr_t pa;
	vm_prot_t prot;
{
	pmap_enter(pmap_kernel(), va, pa, prot, prot|PMAP_WIRED);
}
#else
void
pmap_kenter_pa(va, pa, prot)
	vaddr_t va;
	paddr_t pa;
	vm_prot_t prot;
{
	pte_t tte;
	paddr_t pg;
	struct pmap *pm = pmap_kernel();
	int i;

	/*
	 * Construct the TTE.
	 */
#ifdef DEBUG	     
	enter_stats.unmanaged ++;
#endif
#ifdef DEBUG
	if (pa & (PMAP_NVC|PMAP_NC)) 
		enter_stats.ci ++;
#endif
	tte.tag.tag = TSB_TAG(0,pm->pm_ctx,va);
	tte.data.data = TSB_DATA(0, TLB_8K, pa, pm == pmap_kernel(),
				 (VM_PROT_WRITE & prot),
				 (!(pa & PMAP_NC)), pa & (PMAP_NVC), 1, 0);
	/* We don't track modification here. */
	if (VM_PROT_WRITE & prot) tte.data.data |= TLB_REAL_W|TLB_W; /* HWREF -- XXXX */
	tte.data.data |= TLB_TSB_LOCK;	/* wired */
	ASSERT((tte.data.data & TLB_NFO) == 0);
	pg = NULL;
	while (pseg_set(pm, va, tte.data.data, pg) != NULL) {
		if (pmap_initialized || !uvm_page_physget(&pg)) {
			vm_page_t page;
#ifdef NOTDEF_DEBUG
			printf("pmap_kenter_pa: need to alloc page\n");
#endif
			while ((page = vm_page_alloc1()) == NULL) {
				/*
				 * Let the pager run a bit--however this may deadlock
				 */
#ifdef NOTDEF_DEBUG
				printf("pmap_kenter_pa: calling uvm_wait()\n");
#endif
				uvm_wait("pmap_kenter_pa");
			}
			pg = (paddr_t)VM_PAGE_TO_PHYS(page);
		}
		pmap_zero_page((paddr_t)pg);
#ifdef DEBUG
		enter_stats.ptpneeded ++;
#endif
	}
	i = ptelookup_va(va);
#ifdef DEBUG
	if( pmapdebug & PDB_ENTER )
		prom_printf("pmap_kenter_pa: va=%08x tag=%x:%08x data=%08x:%08x tsb[%d]=%08x\r\n", va,
			    (int)(tte.tag.tag>>32), (int)tte.tag.tag, 
			    (int)(tte.data.data>>32), (int)tte.data.data, 
			    i, &tsb[i]);
	if( pmapdebug & PDB_MMU_STEAL && tsb[i].data.data ) {
		prom_printf("pmap_kenter_pa: evicting entry tag=%x:%08x data=%08x:%08x tsb[%d]=%08x\r\n",
			    (int)(tsb[i].tag.tag>>32), (int)tsb[i].tag.tag, 
			    (int)(tsb[i].data.data>>32), (int)tsb[i].data.data, 
			    i, &tsb[i]);
		prom_printf("with va=%08x tag=%x:%08x data=%08x:%08x tsb[%d]=%08x\r\n", va,
			    (int)(tte.tag.tag>>32), (int)tte.tag.tag, 
			    (int)(tte.data.data>>32), (int)tte.data.data, 
			    i, &tsb[i]);
	}
#endif
	tsb_enter(pm->pm_ctx, va, tte.data.data);
	ASSERT((tsb[i].data.data & TLB_NFO) == 0);
#if 1
	/* this is correct */
	dcache_flush_page(va);
#else
	/* Go totally crazy */
	blast_vcache();
#endif

}
#endif
/*
 * pmap_kenter_pgs:		[ INTERFACE ]
 *
 *	Enter a va -> pa mapping for the array of vm_page's into the
 *	kernel pmap without any physical->virtual tracking, starting
 *	at address va, for npgs pages.
 *
 *	Note: no locking is necessary in this function.
 */
void
pmap_kenter_pgs(va, pgs, npgs)
	vaddr_t va;
	struct vm_page **pgs;
	int npgs;
{
	register u_int64_t phys;
	int i;

	for (i = 0; i < npgs; i++) {
		phys = VM_PAGE_TO_PHYS(pgs[i]);
		
		/* Eventually we can try to optimize this w/large pages */
		pmap_kenter_pa(va, phys, VM_PROT_READ|VM_PROT_WRITE);
		va += NBPG;
	}
}

/*
 * pmap_kremove:		[ INTERFACE ]
 *
 *	Remove a mapping entered with pmap_kenter_pa() or pmap_kenter_pgs()
 *	starting at va, for size bytes (assumed to be page rounded).
 */
#if 1
void
pmap_kremove(va, size)
	vaddr_t va;
	vsize_t size;
{
	return pmap_remove(pmap_kernel(), va, va+size);
}
#else
void
pmap_kremove(va, size)
	vaddr_t va;
	vsize_t size;
{
	struct pmap *pm = pmap_kernel();
	int64_t data;
	int i, flush = 0;

#ifdef DEBUG
	if (pmapdebug & PDB_DEMAP) {
		printf("pmap_kremove: start %p size %lx\n",
		       va, size);
	}
#endif
	while (size >= NBPG) {
		/*
		 * Is this part of the permanent 4MB mapping?
		 */
#ifdef DIAGNOSTIC
		if (pm == pmap_kernel() && (va >= ktext && va < kdata+4*MEG))
			panic("pmap_kremove: va=%08x in locked TLB\r\n", va);
#endif
		/* Shouldn't need to do this if the entry's not valid. */
		if ((data = pseg_get(pm, va))) {
			paddr_t entry;
			
			flush |= 1;
			entry = (data&TLB_PA_MASK);
			/* We need to flip the valid bit and clear the access statistics. */
			if (pseg_set(pm, va, 0, 0)) {
				printf("pmap_kremove: gotten pseg empty!\n");
				Debugger();
				/* panic? */
			}
#ifdef DEBUG
			if (pmapdebug & PDB_DEMAP)
				printf("pmap_kremove: clearing seg %x pdir %x pte %x\n", 
				       (int)va_to_seg(va), (int)va_to_dir(va), 
				       (int)va_to_pte(va));
			remove_stats.removes ++;
#endif
			
			i = ptelookup_va(va);
			if (tsb[i].tag.tag > 0 
			    && tsb[i].tag.tag == TSB_TAG(0,pm->pm_ctx,va))
			{
				/* 
				 * Invalidate the TSB 
				 * 
				 * While we can invalidate it by clearing the
				 * valid bit:
				 *
				 * ptp->data.data_v = 0;
				 *
				 * it's faster to do store 1 doubleword.
				 */
#ifdef DEBUG
				if (pmapdebug & PDB_DEMAP)
					printf(" clearing TSB [%d]\n", i);
#endif
				tsb[i].data.data = 0LL; 
				ASSERT((tsb[i].data.data & TLB_NFO) == 0);
				/* Flush the TLB */
			}
#ifdef DEBUG
			remove_stats.tflushes ++;
#endif
			/* Here we assume nothing can get into the TLB unless it has a PTE */
			tlb_flush_pte(va, pm->pm_ctx);
		}
		va += NBPG;
		size -= NBPG;
	}
	if (flush) {
#ifdef DEBUG
		remove_stats.flushes ++;
#endif
		blast_vcache();
	}
}
#endif

/*
 * Insert physical page at pa into the given pmap at virtual address va.
 * Supports 64-bit pa so we can map I/O space.
 */
int
pmap_enter(pm, va, pa, prot, flags)
	struct pmap *pm;
	vaddr_t va;
	u_int64_t pa;
	vm_prot_t prot;
	int flags;
{
	pte_t tte;
	paddr_t pg;
	int i, aliased = 0;
	pv_entry_t pv = NULL;
	int size = 0; /* PMAP_SZ_TO_TTE(pa); */
	boolean_t wired = (flags & PMAP_WIRED) != 0;

	/*
	 * Is this part of the permanent 4MB mapping?
	 */
#ifdef DIAGNOSTIC
	if (pm == pmap_kernel() && va >= ktext && va < kdata+4*MEG) {
		prom_printf("pmap_enter: va=%08x pa=%x:%08x in locked TLB\r\n", 
			    va, (int)(pa>>32), (int)pa);
		OF_enter();
		return (KERN_SUCCESS);
	}
#endif

#ifdef DEBUG
	/* Trap mapping of page zero */
	if (va == NULL) {
		prom_printf("pmap_enter: NULL va=%08x pa=%x:%08x\r\n", 
			    va, (int)(pa>>32), (int)pa);
		OF_enter();
	}
#endif
#ifdef NOTDEF_DEBUG
	if (pa>>32)
		prom_printf("pmap_enter: va=%08x 64-bit pa=%x:%08x seg=%08x pte=%08x\r\n", 
			    va, (int)(pa>>32), (int)pa, 
			    (int)va_to_seg(va), (int)va_to_pte(va));
#endif
	/*
	 * XXXX If a mapping at this address already exists, remove it.
	 */
	if ((tte.data.data = pseg_get(pm, va))<0) {
		pmap_remove(pm, va, va+NBPG-1);
	}

	/*
	 * Construct the TTE.
	 */
	if (IS_VM_PHYSADDR(pa)) {
		pv = pa_to_pvh(pa);
		aliased = (pv->pv_va&(PV_ALIAS|PV_NVC));
#ifdef DIAGNOSTIC
		if ((flags & VM_PROT_ALL) & ~prot)
			panic("pmap_enter: access_type exceeds prot");
#endif
		/* If we don't have the traphandler do it, set the ref/mod bits now */
		if (flags & VM_PROT_ALL)
			pv->pv_va |= PV_REF;
		if (flags & VM_PROT_WRITE)
			pv->pv_va |= PV_MOD;
#ifdef DEBUG
		enter_stats.managed ++;
#endif
	} else {
#ifdef DEBUG	     
		enter_stats.unmanaged ++;
#endif
		aliased = 0;
	}
	if (pa & PMAP_NVC) aliased = 1;
#ifdef NO_VCACHE
	aliased = 1; /* Disable D$ */
#endif
#ifdef DEBUG
	enter_stats.ci ++;
#endif
	/*
	 * Not used any more.
	tte.tag.tag = TSB_TAG(0,pm->pm_ctx,va);
	 */
	tte.data.data = TSB_DATA(0, size, pa, pm == pmap_kernel(),
				 (flags & VM_PROT_WRITE),
				 (!(pa & PMAP_NC)),aliased,1,(pa & PMAP_LITTLE));
#ifdef HWREF
	if (prot & VM_PROT_WRITE) tte.data.data |= TLB_REAL_W;
#endif
	if (wired) tte.data.data |= TLB_TSB_LOCK;
	ASSERT((tte.data.data & TLB_NFO) == 0);
	pg = NULL;
#ifdef NOTDEF_DEBUG
	printf("pmap_enter: inserting %x:%x at %x\n", 
	       (int)(tte.data.data>>32), (int)tte.data.data, (int)va);
#endif
	while (pseg_set(pm, va, tte.data.data, pg) != NULL) {
		if (pmap_initialized || !uvm_page_physget(&pg)) {
			vm_page_t page;
#ifdef NOTDEF_DEBUG
			printf("pmap_enter: need to alloc page\n");
#endif
			while ((page = vm_page_alloc1()) == NULL) {
				/*
				 * Let the pager run a bit--however this may deadlock
				 */
#ifdef NOTDEF_DEBUG
				printf("pmap_enter: calling uvm_wait()\n");
#endif
				uvm_wait("pmap_enter");
			}
			pg = (paddr_t)VM_PAGE_TO_PHYS(page);
		} 
		pmap_zero_page((paddr_t)pg);
#ifdef DEBUG
		enter_stats.ptpneeded ++;
#endif
#ifdef NOTDEF_DEBUG
	printf("pmap_enter: inserting %x:%x at %x with %x\n", 
	       (int)(tte.data.data>>32), (int)tte.data.data, (int)va, (int)pg);
#endif
	}

#if 1
	if (pv)
		pmap_enter_pv(pm, va, pa);
#else
	if (pv) {
       		/*
		 * Enter the pmap and virtual address into the
		 * physical to virtual map table.
		 */
		int npv;
		int s = splimp();
#ifdef DEBUG
		if (pmapdebug & PDB_ENTER)
			printf("pmap_enter: pv %x: was %x/%x/%x ",
			       pv, pv->pv_va, pv->pv_pmap, pv->pv_next);
#endif
		if (pv->pv_pmap == NULL) {
			/*
			 * No entries yet, use header as the first entry
			 */
#ifdef DEBUG
			if (pmapdebug & PDB_ENTER)
				printf("pmap_enter: first pv: pmap %x va %x\n",
				       pm, va);
			enter_stats.firstpv++;
#endif
			PV_SETVA(pv,va);
			if (pa & PMAP_NVC)
				pv->pv_va |= PV_NVC;
			pv->pv_pmap = pm;
			pv->pv_next = NULL;
		} else {
#ifdef DEBUG
			enter_stats.secondpv++;
#endif

			/*
			 * There is at least one other VA mapping this page.
			 * Place this entry after the header.
			 *
			 * Note: the entry may already be in the table if
			 * we are only changing the protection bits.
			 */
			for (npv = pv; npv; npv = npv->pv_next) {
				aliased = (aliased || 
					   (pm ==  npv->pv_pmap && 
					    ((pv->pv_va^npv->pv_va)&VA_ALIAS_MASK)));
				if (pm == npv->pv_pmap && PV_MATCH(npv,va)) {
#ifdef PARANOIADIAG
					int64_t data;
					
					data = pseg_get(pm, va);
					if (data >= 0 ||
					    data&TLB_PA_MASK != pa)
						printf(
		"pmap_enter: found va %lx pa %lx in pv_table but != %lx\n",
						va, pa, (long)data);
#endif
					goto fnd;
				}
			}
#ifdef DEBUG
			if (pmapdebug & PDB_ENTER)
				printf("pmap_enter: new pv: pmap %x va %x\n",
				       pm, va);
#endif
			/* can this cause us to recurse forever? */
			npv = pool_get(&pv_pool, PR_WAITOK);
			PV_SETVA(npv,va);
			npv->pv_pmap = pm;
			npv->pv_next = pv->pv_next;
			pv->pv_next = npv;
#ifdef DEBUG
			if (!npv->pv_next)
				enter_stats.secondpv++;
#endif
			/* Fixup possible new aliasing */
			if (aliased && !(pv->pv_va&(PV_ALIAS|PV_NVC))) {
				pv->pv_va|=(pa & PMAP_NVC)?PV_NVC:PV_ALIAS;
#ifdef DEBUG
				if (pmapdebug & PDB_ALIAS) 
						printf("pmap_enter: aliased page %p:%p\n", 
						       (int)(pa>>32), (int)pa);
#endif
				for (npv = pv; npv; npv = npv->pv_next) 
					if (npv->pv_pmap == pm) {
#ifdef DEBUG
						if (pmapdebug & PDB_ALIAS) 
							printf("pmap_enter: dealiasing %p in ctx %d\n", 
							       npv->pv_va, npv->pv_pmap->pm_ctx);
#endif
						/* Turn off cacheing of this TTE */
						if (pseg_set(pm, (npv->pv_va&PV_VAMASK), 
							     pseg_get(pm, (npv->pv_va&PV_VAMASK)) 
							     & ~TLB_CV, 0)) {
							printf("pmap_enter: aliased pseg empty!\n");
							Debugger();
							/* panic? */
						}
						/* pmap unused? */
						if ((!pm->pm_ctx) && pm != pmap_kernel())
							continue;
						i = ptelookup_va((npv->pv_va&PV_VAMASK));
						if (tsb[i].tag.tag > 0 && tsb[i].tag.tag == 
						    TSB_TAG(0, pm->pm_ctx, (npv->pv_va&PV_VAMASK))) {
							/* 
							 * Invalidate the TSB 
							 * 
							 * While we can invalidate it by clearing the
							 * valid bit:
							 *
							 * ptp->data.data_v = 0;
							 *
							 * it's faster to do store 1 doubleword.
							 */
							tsb[i].data.data = 0LL; 
							ASSERT((tsb[i].data.data & TLB_NFO) == 0);
						}
						/* Force reload -- protections may be changed */
						tlb_flush_pte((npv->pv_va&PV_VAMASK), pm->pm_ctx);	
#if 1
						/* XXXXXX We should now flush the DCACHE to make sure */
						dcache_flush_page((npv->pv_va&PV_VAMASK));
#else
						blast_vcache();
#endif
					}
			}
		fnd:
			;
		}
		splx(s);
	}
#endif
	i = ptelookup_va(va);
#ifdef DEBUG
	if( pmapdebug & PDB_ENTER )
		prom_printf("pmap_enter: va=%08x tag=%x:%08x data=%08x:%08x tsb[%d]=%08x\r\n", va,
			    (int)(tte.tag.tag>>32), (int)tte.tag.tag, 
			    (int)(tte.data.data>>32), (int)tte.data.data, 
			    i, &tsb[i]);
	if( pmapdebug & PDB_MMU_STEAL && tsb[i].data.data ) {
		prom_printf("pmap_enter: evicting entry tag=%x:%08x data=%08x:%08x tsb[%d]=%08x\r\n",
			    (int)(tsb[i].tag.tag>>32), (int)tsb[i].tag.tag, 
			    (int)(tsb[i].data.data>>32), (int)tsb[i].data.data, 
			    i, &tsb[i]);
		prom_printf("with va=%08x tag=%x:%08x data=%08x:%08x tsb[%d]=%08x\r\n", va,
			    (int)(tte.tag.tag>>32), (int)tte.tag.tag, 
			    (int)(tte.data.data>>32), (int)tte.data.data, 
			    i, &tsb[i]);
	}
#endif
	if (pm->pm_ctx || pm == pmap_kernel()) {
		if (tsb[i].tag.tag > 0 && 
		    tsb[i].tag.tag == TSB_TAG(0,pm->pm_ctx,va)) {
			/* 
			 * Invalidate the TSB 
			 * 
			 * While we can invalidate it by clearing the
			 * valid bit:
			 *
			 * ptp->data.data_v = 0;
			 *
			 * it's faster to do store 1 doubleword.
			 */
			tsb[i].data.data = 0LL; 
			ASSERT((tsb[i].data.data & TLB_NFO) == 0);
		}
		/* Force reload -- protections may be changed */
		tlb_flush_pte(va, pm->pm_ctx);	
		ASSERT((tsb[i].data.data & TLB_NFO) == 0);
	}
#if 1
#if 1
	/* this is correct */
	dcache_flush_page(va);
#else
	/* Go totally crazy */
	blast_vcache();
#endif
#endif
	/* We will let the fast mmu miss interrupt load the new translation */
	pv_check();
	return (KERN_SUCCESS);
}

/*
 * Remove the given range of mapping entries.
 */
void
pmap_remove(pm, va, endva)
	struct pmap *pm;
	vaddr_t va, endva;
{
	int i, flush=0;
	int64_t data;

	/* 
	 * In here we should check each pseg and if there are no more entries,
	 * free it.  It's just that linear scans of 8K pages gets expensive.
	 */

#ifdef DEBUG
	if (pmapdebug & PDB_REMOVE)
		printf("pmap_remove(pm=%x, va=%p, endva=%p):", pm, va, endva);
	remove_stats.calls ++;
#endif

	/* Now do the real work */
	while (va < endva) {
		/*
		 * Is this part of the permanent 4MB mapping?
		 */
#ifdef DIAGNOSTIC
		if( pm == pmap_kernel() && va >= ktext && va < kdata+4*MEG ) 
			panic("pmap_remove: va=%08x in locked TLB\r\n", va);
#endif
		/* We don't really need to do this if the valid bit is not set... */
		if ((data = pseg_get(pm, va))) {
			paddr_t entry;
			
			flush |= 1;
			/* First remove it from the pv_table */
			entry = (data&TLB_PA_MASK);
			if (IS_VM_PHYSADDR(entry))
				pmap_remove_pv(pm, va, entry);
			
			/* We need to flip the valid bit and clear the access statistics. */
			if (pseg_set(pm, va, 0, 0)) {
				printf("pmap_remove: gotten pseg empty!\n");
				Debugger();
				/* panic? */
			}
#ifdef DEBUG
			if (pmapdebug & PDB_REMOVE)
				printf(" clearing seg %x pte %x\n", (int)va_to_seg(va), (int)va_to_pte(va));
			remove_stats.removes ++;
#endif
			if (!pm->pm_ctx && pm != pmap_kernel()) continue;
			i = ptelookup_va(va);
			if (tsb[i].tag.tag > 0 
			    && tsb[i].tag.tag == TSB_TAG(0,pm->pm_ctx,va))
			{
				/* 
				 * Invalidate the TSB 
				 * 
				 * While we can invalidate it by clearing the
				 * valid bit:
				 *
				 * ptp->data.data_v = 0;
				 *
				 * it's faster to do store 1 doubleword.
				 */
#ifdef DEBUG
				if (pmapdebug & PDB_REMOVE)
					printf(" clearing TSB [%d]\n", i);
#endif
				tsb[i].data.data = 0LL; 
				ASSERT((tsb[i].data.data & TLB_NFO) == 0);
				/* Flush the TLB */
			}
#ifdef NOTDEF_DEBUG
			else if (pmapdebug & PDB_REMOVE) {
				printf("TSB[%d] has ctx %d va %x: ",
				       i,
				       TSB_TAG_CTX(tsb[i].tag.tag),
				       (int)(TSB_TAG_VA(tsb[i].tag.tag)|(i<<13)));
				printf("%08x:%08x %08x:%08x\n",
				       (int)(tsb[i].tag.tag>>32), (int)tsb[i].tag.tag, 
				       (int)(tsb[i].data.data>>32), (int)tsb[i].data.data);			       
			}
#endif
#ifdef DEBUG
			remove_stats.tflushes ++;
#endif
			/* Here we assume nothing can get into the TLB unless it has a PTE */
			tlb_flush_pte(va, pm->pm_ctx);
		}
		va += NBPG;
	}
	if (flush) {
#ifdef DEBUG
		remove_stats.flushes ++;
#endif
		blast_vcache();
	}
#ifdef DEBUG
	if (pmapdebug & PDB_REMOVE)
		printf("\n");
#endif
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
	int i;
	paddr_t pa;
	int64_t data;
	
	if (prot & VM_PROT_WRITE) 
		return;

	if (prot == VM_PROT_NONE) {
		pmap_remove(pm, sva, eva);
		return;
	}
		
	sva = sva & ~PGOFSET;
	while (sva < eva) {
		/*
		 * Is this part of the permanent 4MB mapping?
		 */
		if( pm == pmap_kernel() && sva >= ktext && sva < kdata+4*MEG ) {
			prom_printf("pmap_protect: va=%08x in locked TLB\r\n", sva);
			OF_enter();
			return;
		}

#ifdef DEBUG
		if (pmapdebug & PDB_CHANGEPROT)
			printf("pmap_protect: va %p\n", sva);
#endif
		if (((data = pseg_get(pm, sva))&TLB_V) /*&& ((data&TLB_TSB_LOCK) == 0)*/) {
			pa = data&TLB_PA_MASK;
#ifdef DEBUG
			if (pmapdebug & (PDB_CHANGEPROT|PDB_REF))
				printf("pmap_protect: va=%08x data=%x:%08x seg=%08x pte=%08x\r\n", 
					    sva, (int)(pa>>32), (int)pa, (int)va_to_seg(sva), (int)va_to_pte(sva));
/* Catch this before the assertion */
			if (data & TLB_NFO) {
				printf("pmap_protect: pm=%x NFO mapping va=%x data=%x:%x\n",
				       pm, sva, (int)(data>>32), (int)data);
				Debugger();
			}
#endif
			/* Just do the pmap and TSB, not the pv_list */
			data &= ~(TLB_W|TLB_REAL_W);
			ASSERT((data & TLB_NFO) == 0);
			if (pseg_set(pm, sva, data, 0)) {
				printf("pmap_protect: gotten pseg empty!\n");
				Debugger();
				/* panic? */
			}
			
			if (!pm->pm_ctx && pm != pmap_kernel()) continue;
			i = ptelookup_va(sva);
			if (tsb[i].tag.tag > 0 
			    && tsb[i].tag.tag == TSB_TAG(0,pm->pm_ctx,sva)) {
				tsb[i].data.data = data;
				ASSERT((tsb[i].data.data & TLB_NFO) == 0);
				
			}
			tlb_flush_pte(sva, pm->pm_ctx);
		}
		sva += NBPG;
	}
	pv_check();
}

/*
 * Extract the physical page address associated
 * with the given map/virtual_address pair.
 * GRR, the vm code knows; we should not have to do this!
 *
 * XXX XXX XXX Need to deal with the case that the address is NOT MAPPED!
 */
boolean_t
pmap_extract(pm, va, pap)
	register struct pmap *pm;
	vaddr_t va;
	paddr_t *pap;
{
	paddr_t pa;

	if( pm == pmap_kernel() && va >= kdata && va < kdata+4*MEG ) {
		/* Need to deal w/locked TLB entry specially. */
		pa = (paddr_t) (kdata - kdata + va);
#ifdef DEBUG
		if (pmapdebug & PDB_EXTRACT) {
			printf("pmap_extract: va=%x pa=%lx\n", va, (long)pa);
		}
#endif
	} else if( pm == pmap_kernel() && va >= ktext && va < ektext ) {
		/* Need to deal w/locked TLB entry specially. */
		pa = (paddr_t) (ktextp - ktext + va);
#ifdef DEBUG
		if (pmapdebug & PDB_EXTRACT) {
			printf("pmap_extract: va=%x pa=%lx\n", va, (long)pa);
		}
#endif
	} else {
		pa = (pseg_get(pm, va)&TLB_PA_MASK)+(va&PGOFSET);
#ifdef DEBUG
		if (pmapdebug & PDB_EXTRACT) {
			pa = ldxa((vaddr_t)&pm->pm_segs[va_to_seg(va)], ASI_PHYS_CACHED);
			printf("pmap_extract: va=%p segs[%ld]=%lx", va, (long)va_to_seg(va), (long)pa);
			if (pa) {
				pa = (paddr_t)ldxa((vaddr_t)&((paddr_t*)(u_long)pa)[va_to_dir(va)], ASI_PHYS_CACHED);
				printf(" segs[%ld][%ld]=%lx", va_to_seg(va), (long)va_to_dir(va), (long)pa);
			}
			if (pa)	{
				pa = (paddr_t)ldxa((vaddr_t)&((paddr_t*)(u_long)pa)[va_to_pte(va)], ASI_PHYS_CACHED);
				printf(" segs[%ld][%ld][%ld]=%lx", (long)va_to_seg(va), 
				       (long)va_to_dir(va), (long)va_to_pte(va), (long)pa);
			}
			printf(" pseg_get: %lx\n", (long)pa);
		}
#endif
	}
	if (pap != NULL)
		*pap = pa;
	return (TRUE);
}

#if 0
/* This appears to be no longer used. */
/*
 * Map physical addresses into kernel VM. -- used by device drivers
 */
vaddr_t
pmap_map(va, pa, endpa, prot)
	register vaddr_t va;
	retister paddr_t pa, endpa;
	register int prot;
{
	register int pgsize = PAGE_SIZE;
	int i;
	
	while (pa < endpa) {
		for (i=0; page_size_map[i].mask; i++) {
			if (((pa | va) & page_size_map[i].mask) == 0
				&& pa + page_size_map[i].mask < endpa)
				break;
		}
		
		do {
#ifdef DEBUG
			page_size_map[i].use++;
#endif
			pmap_enter(pmap_kernel(), va, pa|page_size_map[i].code, 
				   prot,
				   VM_PROT_READ|VM_PROT_WRITE|PMAP_WIRED);
			va += pgsize;
			pa += pgsize;
		} while (pa & page_size_map[i].mask);
	}
	return (va);
}
#endif

#if 0
/*
 * Really change page protections -- used by device drivers
 */
void pmap_changeprot(pm, start, prot, size)
pmap_t pm; 
vaddr_t start;
vm_prot_t prot;
int size;
{
	int i, s;
	vaddr_t sva, eva;
	int64_t data, set, clr;
	
	if (prot == VM_PROT_NONE) {
		pmap_remove(pm, start, start+size);
		return;
	}
		
	if (prot & VM_PROT_WRITE) {
#ifdef HWREF
		set = TLB_REAL_W/*|TLB_W|TLB_MODIFY*/;
#else
		set = TLB_REAL_W|TLB_W|TLB_MODIFY;
#endif
		clr = 0LL;
	} else {
		set = 0LL;
		clr = TLB_REAL_W|TLB_W;
	}

	sva = start & ~PGOFSET;
	eva = start + size;
	while (sva < eva) {
		/*
		 * Is this part of the permanent 4MB mapping?
		 */
		if( pm == pmap_kernel() && sva >= ktext && sva < kdata+4*MEG ) {
			prom_printf("pmap_changeprot: va=%08x in locked TLB\r\n", sva);
			OF_enter();
			return;
		}

#ifdef DEBUG
		if (pmapdebug & (PDB_CHANGEPROT|PDB_REF))
			printf("pmap_changeprot: va %p prot %x\n", sva, prot);
#endif
		/* First flush the TSB */
		i = ptelookup_va(sva);
		/* Then update the page table */
		s = splimp();
		if ((data = pseg_get(pm, sva))) {
			data |= set;
			data &= ~clr;
			ASSERT((data & TLB_NFO) == 0);
			if (pseg_set(pm, sva, data, 0)) {
				printf("pmap_changeprot: gotten empty pseg!\n");
				Debugger();
				/* panic? */
			}
			if (pm->pm_ctx || pm == pmap_kernel()) {
				tlb_flush_pte(sva, pm->pm_ctx);
				if (tsb[i].tag.tag > 0 
				    && tsb[i].tag.tag == TSB_TAG(0,pm->pm_ctx,sva)) 
					tsb[i].tag.tag = tsb[i].data.data = data;
			}
		}
		splx(s);
		sva += NBPG;
	}
	pv_check();
}
#endif

/*
 * Return the number bytes that pmap_dumpmmu() will dump.
 */
int
pmap_dumpsize()
{
	int	sz;

	sz = ALIGN(sizeof(kcore_seg_t)) + ALIGN(sizeof(cpu_kcore_hdr_t));
	sz += memsize * sizeof(phys_ram_seg_t);

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
 *	phys_ram_seg_t[memsize]  physical memory segments
 *	segmap_t[NKREG*NSEGRG]	 the kernel's segment map (NB: needed?)
 */
int
pmap_dumpmmu(dump, blkno)
	register daddr_t blkno;
	register int (*dump)	__P((dev_t, daddr_t, caddr_t, size_t));
{
	kcore_seg_t	*kseg;
	cpu_kcore_hdr_t	*kcpu;
	phys_ram_seg_t	memseg;
	register int	error = 0;
	register int	i, memsegoffset, segmapoffset;
	int		buffer[dbtob(1) / sizeof(int)];
	int		*bp, *ep;

#define EXPEDITE(p,n) do {						\
	int *sp = (int *)(p);						\
	int sz = (n);							\
	while (sz > 0) {						\
		*bp++ = *sp++;						\
		if (bp >= ep) {						\
			error = (*dump)(dumpdev, blkno,			\
					(caddr_t)buffer, dbtob(1));	\
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
	kcpu->kernbase = KERNBASE;
	kcpu->kphys = (paddr_t)ktextp;
	kcpu->nmemseg = memsize;
	kcpu->memsegoffset = memsegoffset = ALIGN(sizeof(cpu_kcore_hdr_t));
	kcpu->nsegmap = STSZ;
	kcpu->segmapoffset = segmapoffset =
		memsegoffset + memsize * sizeof(phys_ram_seg_t);

	kcpu->npmeg = 0; 
	kcpu->pmegoffset = 0; /* We don't do this. */

	/* Note: we have assumed everything fits in buffer[] so far... */
	bp = (int *)((long)kcpu + ALIGN(sizeof(cpu_kcore_hdr_t)));

	for (i = 0; i < memsize; i++) {
		memseg.start = mem[i].start;
		memseg.size = mem[i].size;
		EXPEDITE(&memseg, sizeof(phys_ram_seg_t));
	}

#if 0
	/*
	 * Since we're not mapping this in we need to re-do some of this
	 * logic.
	 */
	EXPEDITE(&kernel_pmap_.pm_segs[0], sizeof(kernel_pmap_.pm_segs));
#endif

	if (bp != buffer)
		error = (*dump)(dumpdev, blkno++, (caddr_t)buffer, dbtob(1));

	return (error);
}

/*
 * Determine (non)existance of physical page
 */
int pmap_pa_exists(pa)
paddr_t pa;
{
	register struct mem_region *mp;

	/* Just go through physical memory list & see if we're there */
	for (mp = mem; mp->size && mp->start <= pa; mp++)
		if( mp->start <= pa && mp->start + mp->size >= pa )
			return 1;
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
ptelookup_va(va)
	vaddr_t va;
{
	long tsbptr;
#define TSBBASEMASK	(0xffffffffffffe000LL<<tsbsize)

	tsbptr = (((va >> 9) & 0xfffffffffffffff0LL) & ~TSBBASEMASK );
	return (tsbptr/sizeof(pte_t));
}

#if notyet
void
tsb_enter(ctx, va, data)
	int ctx;
	int64_t va;
	int64_t data;
{
	int i, s;
	int64_t pa;

	i = ptelookup_va(va);
	s = splimp();
	pa = tsb[i].data.data&TLB_PA_MASK;
	/* 
	 * If we use fast DMMU access fault handlers to track
	 * referenced and modified bits, we should save the 
	 * TSB entry's state here.  Since we don't, we don't.
	 */
	/* Do not use global entries */
	tsb[i].tag.tag = TSB_TAG(0,ctx,va);
	tsb[i].data.data = data;
	tlb_flush_pte(va, ctx);	/* Force reload -- protections may be changed */
	splx(s);
}
#endif

/*
 * Do whatever is needed to sync the MOD/REF flags
 */

boolean_t
pmap_clear_modify(pg)
	struct vm_page *pg;
{
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	int changed = 0;
#ifdef DEBUG
	int modified = 0;
#endif
	int i, s;
	register pv_entry_t pv;
	
#ifdef DEBUG
	if (pmapdebug & (PDB_CHANGEPROT|PDB_REF))
		printf("pmap_clear_modify(%p)\n", pa);
#endif

	if (!IS_VM_PHYSADDR(pa)) {
		pv_check();
#ifdef DEBUG
		printf("pmap_clear_modify(%p): page not managed\n", pa);
		Debugger();
#endif
		/* We always return 0 for I/O mappings */
		return (changed);
	}

#if defined(DEBUG)
	modified = pmap_is_modified(pg);
#endif
	/* Clear all mappings */
	s = splimp();
	pv = pa_to_pvh(pa);
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
	if (pv->pv_pmap != NULL)
		for (; pv; pv = pv->pv_next) {
			int64_t data;

			/* First clear the mod bit in the PTE and make it R/O */
			data = pseg_get(pv->pv_pmap, pv->pv_va&PV_VAMASK);
			/* Need to both clear the modify and write bits */
			if (data & (TLB_MODIFY|TLB_W))
				changed |= 1;
#ifdef HWREF
			data &= ~(TLB_MODIFY|TLB_W);
#else
			data &= ~(TLB_MODIFY|TLB_W|TLB_REAL_W);
#endif
			ASSERT((data & TLB_NFO) == 0);
			if (pseg_set(pv->pv_pmap, pv->pv_va&PV_VAMASK, data, 0)) {
				printf("pmap_clear_modify: gotten pseg empty!\n");
				Debugger();
				/* panic? */
			}
			if (pv->pv_pmap->pm_ctx || pv->pv_pmap == pmap_kernel()) {
				i = ptelookup_va(pv->pv_va&PV_VAMASK);
				if (tsb[i].tag.tag == TSB_TAG(0, pv->pv_pmap->pm_ctx, pv->pv_va&PV_VAMASK))
					tsb[i].data.data = /* data */ 0;
				tlb_flush_pte(pv->pv_va&PV_VAMASK, pv->pv_pmap->pm_ctx);
			}
			/* Then clear the mod bit in the pv */
			if (pv->pv_va & PV_MOD)
				changed |= 1;
			pv->pv_va &= ~(PV_MOD);
		}
	splx(s);
	pv_check();
#ifdef DEBUG
	if (pmap_is_modified(pg)) {
		printf("pmap_clear_modify(): %p still modified!\n", pg);
		Debugger();
	}
	if (pmapdebug & (PDB_CHANGEPROT|PDB_REF))
		printf("pmap_clear_modify: page %lx %s\n", (long)pa, 
		       (changed?"was modified":"was not modified"));
	if (modified != changed) {
		printf("pmap_clear_modify: modified %d changed %d\n", modified, changed);
		Debugger();
	} else return (modified);
#endif
	return (changed);
}


boolean_t
pmap_clear_reference(pg)
	struct vm_page* pg;
{
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	int changed = 0;
#ifdef DEBUG
	int referenced = 0;
#endif
	int i, s;
	register pv_entry_t pv;

#ifdef DEBUG
	if (pmapdebug & (PDB_CHANGEPROT|PDB_REF))
		printf("pmap_clear_reference(%p)\n", pa);
#endif
	if (!IS_VM_PHYSADDR(pa)) {
		pv_check();
#ifdef DEBUG
		printf("pmap_clear_reference(%p): page not managed\n", pa);
		Debugger();
#endif
		return (changed);
	}
#if defined(DEBUG)
	referenced = pmap_is_referenced(pg);
#endif
	/* Clear all references */
	s = splimp();
	pv = pa_to_pvh(pa);
#ifdef NOT_DEBUG
	if (pv->pv_va & PV_MOD)
		printf("pmap_clear_reference(): pv %p still modified\n", (long)pa);
#endif
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
			
			data = pseg_get(pv->pv_pmap, pv->pv_va&PV_VAMASK);
#ifdef DEBUG
			if (pmapdebug & PDB_CHANGEPROT)
				printf("clearing ref pm:%p va:%p ctx:%x data:%x:%x\n", pv->pv_pmap,
				       pv->pv_va, pv->pv_pmap->pm_ctx, (int)(data>>32), (int)data);
#endif
#ifdef HWREF
			if (data & TLB_ACCESS)
				changed |= 1;
			data &= ~TLB_ACCESS;
#else
			if (data < 0)
				changed |= 1;
			data = 0;
#endif
			ASSERT((data & TLB_NFO) == 0);
			if (pseg_set(pv->pv_pmap, pv->pv_va, data, 0)) {
				printf("pmap_clear_reference: gotten pseg empty!\n");
				Debugger();
				/* panic? */
			}
			if (pv->pv_pmap->pm_ctx || pv->pv_pmap == pmap_kernel()) {
				i = ptelookup_va(pv->pv_va&PV_VAMASK);
				/* Invalidate our TSB entry since ref info is in the PTE */
				if (tsb[i].tag.tag == TSB_TAG(0,pv->pv_pmap->pm_ctx,pv->pv_va&PV_VAMASK))
					tsb[i].data.data = 0;
				tlb_flush_pte(pv->pv_va&PV_VAMASK, pv->pv_pmap->pm_ctx);
			}
			if (pv->pv_va & PV_REF)
				changed |= 1;
			pv->pv_va &= ~(PV_REF);
		}
	}
	/* Stupid here will take a cache hit even on unmapped pages 8^( */
	blast_vcache();
	splx(s);
	pv_check();
#ifdef DEBUG
	if (pmap_is_referenced(pg)) {
		printf("pmap_clear_reference(): %p still referenced!\n", pg);
		Debugger();
	}
	if (pmapdebug & (PDB_CHANGEPROT|PDB_REF))
		printf("pmap_clear_reference: page %lx %s\n", (long)pa, 
		       (changed?"was referenced":"was not referenced"));
	if (referenced != changed) {
		printf("pmap_clear_reference: referenced %d changed %d\n", referenced, changed);
		Debugger();
	} else return (referenced);
#endif
	return (changed);
}

boolean_t
pmap_is_modified(pg)
	struct vm_page* pg;
{
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	int i=0, s;
	register pv_entry_t pv, npv;

	if (!IS_VM_PHYSADDR(pa)) {
		pv_check();
#ifdef DEBUG
		printf("pmap_is_modified(%p): page not managed\n", pa);
		Debugger();
#endif
		return 0;
	}
	/* Check if any mapping has been modified */
	s = splimp();
	pv = pa_to_pvh(pa);
#ifdef HWREF
	if (pv->pv_va&PV_MOD) i = 1;
#ifdef DEBUG	
	if (pv->pv_next && !pv->pv_pmap) {
		printf("pmap_is_modified: npv but no pmap for pv %p\n", pv);
		Debugger();
	}
#endif
	if (pv->pv_pmap != NULL)
		for (npv = pv; i == 0 && npv && npv->pv_pmap; npv = npv->pv_next) {
			int64_t data;
			
			data = pseg_get(npv->pv_pmap, npv->pv_va&PV_VAMASK);
			if (data & (TLB_MODIFY|TLB_W)) i = 1;
			/* Migrate modify info to head pv */
			if (npv->pv_va & PV_MOD) i = 1;
			npv->pv_va &= ~PV_MOD;
		}
	/* Save modify info */
	if (i) pv->pv_va |= PV_MOD;
#ifdef DEBUG
	if (i) pv->pv_va |= PV_WE;
#endif
#else
	if (pv->pv_va&PV_MOD) i = 1;
#endif
	splx(s);

#ifdef DEBUG
	if (pmapdebug & (PDB_CHANGEPROT|PDB_REF)) {
		printf("pmap_is_modified(%p) = %d\n", pa, i);
		/* if (i) Debugger(); */
	}
#endif
	pv_check();
	return (i);
}

boolean_t
pmap_is_referenced(pg)
	struct vm_page* pg;
{
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	int i=0, s;
	register pv_entry_t pv, npv;

	if (!IS_VM_PHYSADDR(pa)) {
#ifdef DEBUG
		printf("pmap_is_referenced(%p): page not managed\n", pa);
		Debugger();
#endif
		return 0;
	}
	/* Check if any mapping has been referenced */
	s = splimp();
	pv = pa_to_pvh(pa);
#ifdef HWREF 
	if (pv->pv_va&PV_REF) i = 1;
#ifdef DEBUG	
	if (pv->pv_next && !pv->pv_pmap) {
		printf("pmap_is_referenced: npv but no pmap for pv %p\n", pv);
		Debugger();
	}
#endif
	if (pv->pv_pmap != NULL)
		for (npv = pv; npv; npv = npv->pv_next) {
			int64_t data;
			
			data = pseg_get(npv->pv_pmap, npv->pv_va&PV_VAMASK);
			if (data & TLB_ACCESS) i = 1;
			/* Migrate modify info to head pv */
			if (npv->pv_va & PV_REF) i = 1;
			npv->pv_va &= ~PV_REF;
		}
	/* Save ref info */
	if (i) pv->pv_va |= PV_REF;
#else
	if (pv->pv_va&PV_REF) i = 1;
#endif
	splx(s);

#ifdef DEBUG
	if (pmapdebug & (PDB_CHANGEPROT|PDB_REF)) {
		printf("pmap_is_referenced(%p) = %d\n", pa, i);
		/* if (i) Debugger(); */
	}
#endif
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
	register pmap_t	pmap;
	vaddr_t va;
{
	int64_t data;
	int s;

#ifdef DEBUG
	if (pmapdebug & (PDB_MMU_STEAL)) /* XXXX Need another flag for this */
		printf("pmap_unwire(%p, %lx)\n", pmap, va);
#endif
	if (pmap == NULL) {
		pv_check();
		return;
	}

	/*
	 * Is this part of the permanent 4MB mapping?
	 */
	if( pmap == pmap_kernel() && va >= ktext && va < kdata+4*MEG ) {
		prom_printf("pmap_unwire: va=%08x in locked TLB\r\n", va);
		OF_enter();
		return;
	}
	s = splimp();
	data = pseg_get(pmap, va&PV_VAMASK);

	data &= ~TLB_TSB_LOCK;

	if (pseg_set(pmap, va&PV_VAMASK, data, 0)) {
		printf("pmap_unwire: gotten pseg empty!\n");
		Debugger();
		/* panic? */
	}
	splx(s);
	pv_check();
}

/*
 * Lower the protection on the specified physical page.
 *
 * Never enable writing as it will break COW
 */

void
pmap_page_protect(pg, prot)
	struct vm_page* pg;
	vm_prot_t prot;
{
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	register pv_entry_t pv;
	register int i, s;
	long long clear, set;
	int64_t data = 0LL;

#ifdef DEBUG
	if (pmapdebug & PDB_CHANGEPROT)
		printf("pmap_page_protect: pa %p prot %x\n", pa, prot);
#endif

	if (!IS_VM_PHYSADDR(pa)) {
#ifdef DEBUG
		printf("pmap_page_protect(%p): page unmanaged\n", pa);
		Debugger();
#endif
		pv_check();
		return;
	}
	if (prot & VM_PROT_WRITE) {
		pv_check();
		return;
	}

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

		pv = pa_to_pvh(pa);
		s = splimp();
#ifdef DEBUG	
		if (pv->pv_next && !pv->pv_pmap) {
			printf("pmap_page_protect: npv but no pmap for pv %p\n", pv);
			Debugger();
		}
#endif
		if (pv->pv_pmap != NULL) {
			for (; pv; pv = pv->pv_next) {
#ifdef DEBUG
				if (pmapdebug & (PDB_CHANGEPROT|PDB_REF)) {
					printf("pmap_page_protect: RO va %p of pa %p...\n",
					       pv->pv_va, pa);
				}
#if 0
				if (!pv->pv_pmap->pm_segs[va_to_seg(pv->pv_va&PV_VAMASK)]) {
					printf("pmap_page_protect(%x:%x,%x): pv %x va %x not in pmap %x\n",
					       (int)(pa>>32), (int)pa, prot, pv, pv->pv_va, pv->pv_pmap);
					Debugger();
					continue;
				}
#endif
#endif
				data = pseg_get(pv->pv_pmap, pv->pv_va&PV_VAMASK);
				data &= ~(clear);
				data |= (set);
				ASSERT((data & TLB_NFO) == 0);
				if (pseg_set(pv->pv_pmap, pv->pv_va&PV_VAMASK, data, 0)) {
					printf("pmap_page_protect: gotten pseg empty!\n");
					Debugger();
					/* panic? */
				}
				if (pv->pv_pmap->pm_ctx || pv->pv_pmap == pmap_kernel()) {
					i = ptelookup_va(pv->pv_va&PV_VAMASK);
					/* since we already know the va for each mapping we don't need to scan the entire TSB */
					if (tsb[i].tag.tag == TSB_TAG(0, pv->pv_pmap->pm_ctx, pv->pv_va&PV_VAMASK))
						tsb[i].data.data = /* data */ 0;
					tlb_flush_pte(pv->pv_va&PV_VAMASK, pv->pv_pmap->pm_ctx);
				}
			}
		}
		splx(s);
	} else {
		pv_entry_t npv, firstpv;
		/* remove mappings */
		
#ifdef DEBUG
		if (pmapdebug & PDB_REMOVE) 
			printf("pmap_page_protect: demapping pa %lx\n", (long)pa);
#endif
			       
		firstpv = pv = pa_to_pvh(pa);
		s = splimp();
		/* First remove the entire list of continuation pv's*/
		for (npv = pv->pv_next; npv; npv = pv->pv_next) {
			/* We're removing npv from pv->pv_next */
#ifdef DEBUG
			if (pmapdebug & (PDB_CHANGEPROT|PDB_REF|PDB_REMOVE)) {
				printf("pmap_page_protect: demap va %p of pa %p in pmap %p...\n",
				       npv->pv_va, (long)pa, npv->pv_pmap);
			}
#if 0
			if (!npv->pv_pmap->pm_segs[va_to_seg(npv->pv_va&PV_VAMASK)]) {
				printf("pmap_page_protect(%x:%x,%x): pv %x va %x not in pmap %x\n",
				       (int)(pa>>32), (int)pa, prot, npv, npv->pv_va, npv->pv_pmap);
				Debugger();
				continue;
			}
#endif
#endif
			/* clear the entry in the page table */
			data = pseg_get(npv->pv_pmap, npv->pv_va&PV_VAMASK);

			/* Save ref/mod info */
			if (data & TLB_ACCESS) 
				firstpv->pv_va |= PV_REF;
			if (data & (TLB_W|TLB_MODIFY))
				firstpv->pv_va |= PV_MOD;
			if (data & TLB_TSB_LOCK) {
#ifdef DIAGNOSTIC
				printf("pmap_page_protect: wired page pm %p va %p not removed\n",
				       npv->pv_pmap, npv->pv_va);
				printf("vm wire count %d\n", 
					PHYS_TO_VM_PAGE(pa)->wire_count);
				continue;
#endif			
			}
			/* Clear mapping */
			if (pseg_set(npv->pv_pmap, npv->pv_va&PV_VAMASK, 0, 0)) {
				printf("pmap_page_protect: gotten pseg empty!\n");
				Debugger();
				/* panic? */
			}
			if (npv->pv_pmap->pm_ctx || npv->pv_pmap == pmap_kernel()) {
				/* clear the entry in the TSB */
				i = ptelookup_va(npv->pv_va&PV_VAMASK);
				/* since we already know the va for each mapping we don't need to scan the entire TSB */
				if (tsb[i].tag.tag == TSB_TAG(0, npv->pv_pmap->pm_ctx, npv->pv_va&PV_VAMASK))
					tsb[i].data.data = 0LL;			
				tlb_flush_pte(npv->pv_va&PV_VAMASK, npv->pv_pmap->pm_ctx);
			}
			
			/* free the pv */
			pv->pv_next = npv->pv_next;
			pool_put(&pv_pool, npv);
		}

		pv = firstpv;

		/* Then remove the primary pv */
#ifdef DEBUG	
		if (pv->pv_next && !pv->pv_pmap) {
			printf("pmap_page_protect: npv but no pmap for pv %p\n", pv);
			Debugger();
		}
#endif
		if (pv->pv_pmap != NULL) {
#ifdef DEBUG
			if (pmapdebug & (PDB_CHANGEPROT|PDB_REF|PDB_REMOVE)) {
				printf("pmap_page_protect: demap va %p of pa %lx from pm %p...\n",
				       pv->pv_va, (long)pa, pv->pv_pmap);
			}
#if 0
			if (!pv->pv_pmap->pm_segs[va_to_seg(pv->pv_va&PV_VAMASK)]) {
				printf("pmap_page_protect(%x:%x,%x): pv %x va %x not in pmap %x\n",
				       (int)(pa>>32), (int)pa, prot, pv, pv->pv_va, pv->pv_pmap);
					Debugger();
					goto skipit;
			}
#endif
#endif
			data = pseg_get(pv->pv_pmap, pv->pv_va&PV_VAMASK);
			/* Save ref/mod info */
			if (data & TLB_ACCESS) 
				pv->pv_va |= PV_REF;
			if (data & (TLB_W|TLB_MODIFY))
				pv->pv_va |= PV_MOD;
			if (data & TLB_TSB_LOCK) {
#ifdef DIAGNOSTIC
				printf("pmap_page_protect: Removing wired page pm %p va %p\n",
				       pv->pv_pmap, pv->pv_va);
#endif			
			}
			if (pseg_set(pv->pv_pmap, pv->pv_va&PV_VAMASK, 0, 0)) {
				printf("pmap_page_protect: gotten pseg empty!\n");
				Debugger();
				/* panic? */
			}
			if (pv->pv_pmap->pm_ctx || pv->pv_pmap == pmap_kernel()) {
				i = ptelookup_va(pv->pv_va&PV_VAMASK);
				/* since we already know the va for each mapping we don't need to scan the entire TSB */
				if (tsb[i].tag.tag == TSB_TAG(0, pv->pv_pmap->pm_ctx, pv->pv_va&PV_VAMASK))
					tsb[i].data.data = 0LL;			
				tlb_flush_pte(pv->pv_va&PV_VAMASK, pv->pv_pmap->pm_ctx);
			}
			npv = pv->pv_next;
			/* dump the first pv */
			if (npv) {
				/* First save mod/ref bits */
				npv->pv_va |= (pv->pv_va&PV_MASK);
				*pv = *npv;
				pool_put(&pv_pool, npv);
			} else {
				pv->pv_pmap = NULL;
				pv->pv_next = NULL;
			}
#if 0
		skipit:
#endif
		}
		splx(s);
	}
	/* We should really only flush the pages we demapped. */
	blast_vcache();
	pv_check();
}

/*
 * count pages in pmap -- this can be slow.
 */
int
pmap_count_res(pm)
	pmap_t pm;
{
	int i, j, k, n, s;
	paddr_t *pdir, *ptbl;
	/* Almost the same as pmap_collect() */

	/* Don't want one of these pages reused while we're reading it. */
	s = splimp();
	n = 0;
	for (i=0; i<STSZ; i++) {
		if((pdir = (paddr_t *)(u_long)ldxa((vaddr_t)&pm->pm_segs[i], ASI_PHYS_CACHED))) {
			for (k=0; k<PDSZ; k++) {
				if ((ptbl = (paddr_t *)(u_long)ldxa((vaddr_t)&pdir[k], ASI_PHYS_CACHED))) {
					for (j=0; j<PTSZ; j++) {
						int64_t data = (int64_t)ldxa((vaddr_t)&ptbl[j], ASI_PHYS_CACHED);
						if (data&TLB_V)
							n++;
					}
				}
			}
		}
	}
	splx(s);
	return n;
}

/*
 * Allocate a context.  If necessary, steal one from someone else.
 * Changes hardware context number and loads segment map.
 *
 * This routine is only ever called from locore.s just after it has
 * saved away the previous process, so there are no active user windows.
 *
 * The new context is flushed from the TLB before returning.
 */
int
ctx_alloc(pm)
	struct pmap* pm;
{
	register int s, cnum;
	static int next = 0;

	if (pm == pmap_kernel()) {
#ifdef DIAGNOSTIC
		printf("ctx_alloc: kernel pmap!\n");
#endif
		return (0);
	}
	s = splpmap();
	cnum = next;
	do {
		if (cnum >= numctx-1) 
			cnum = 0;
	} while (ctxbusy[++cnum] != NULL && cnum != next);
	if (cnum==0) cnum++; /* Never steal ctx 0 */
	if (ctxbusy[cnum]) {
		int i;
#ifdef DEBUG
		/* We should identify this pmap and clear it */
		printf("Warning: stealing context %d\n", cnum);
		remove_stats.pidflushes ++;
#endif
		/* We gotta steal this context */
		for (i = 0; i < TSBENTS; i++) {
			if (TSB_TAG_CTX(tsb[i].tag.tag) == cnum)
				tsb[i].data.data = 0LL;
		}
		tlb_flush_ctx(cnum);
	}
	ctxbusy[cnum] = pm->pm_physaddr;
	next = cnum;
	splx(s);
	pm->pm_ctx = cnum;
#ifdef DEBUG
	if (pmapdebug & PDB_CTX_ALLOC)
		printf("ctx_alloc: allocated ctx %d\n", cnum);
#endif
	return cnum;
}

/*
 * Give away a context.
 */
void
ctx_free(pm)
	struct pmap* pm;
{
	int oldctx;
	
	oldctx = pm->pm_ctx;

	if (oldctx == 0)
		panic("ctx_free: freeing kernel context");
#ifdef DIAGNOSTIC
	if (ctxbusy[oldctx] == 0)
		printf("ctx_free: freeing free context %d\n", oldctx);
	if (ctxbusy[oldctx] != pm->pm_physaddr) {
		printf("ctx_free: freeing someone esle's context\n "
		       "ctxbusy[%d] = %p, pm(%p)->pm_ctx = %p\n", 
		       oldctx, ctxbusy[oldctx], pm, pm->pm_physaddr);
		Debugger();
	}
#endif
	/* We should verify it has not been stolen and reallocated... */
#ifdef DEBUG
	if (pmapdebug & PDB_CTX_ALLOC) {
		printf("ctx_free: freeing ctx %d\n", oldctx);
		Debugger();
	}
#endif
	ctxbusy[oldctx] = NULL;
}

/*
 * Enter the pmap and virtual address into the
 * physical to virtual map table.
 */
void
pmap_enter_pv(pmap, va, pa)
	pmap_t pmap;
	vaddr_t va;
	paddr_t pa;
{
	pv_entry_t pv, npv;
	int s;

	pv = pa_to_pvh(pa);
	s = splimp();
#ifdef DEBUG
	if (pmapdebug & PDB_ENTER)
		printf("pmap_enter: pv %p: was %lx/%p/%p\n",
		       pv, pv->pv_va, pv->pv_pmap, pv->pv_next);
#endif
	if (pv->pv_pmap == NULL) {
		/*
		 * No entries yet, use header as the first entry
		 */
#ifdef DEBUG
		if (pmapdebug & PDB_ENTER)
			printf("pmap_enter: first pv: pmap %p va %lx\n",
				pmap, va);
		enter_stats.firstpv++;
#endif
		PV_SETVA(pv, va);
		pv->pv_pmap = pmap;
		pv->pv_next = NULL;
	} else {
		if (!(pv->pv_va & PV_ALIAS)) {
			/*
			 * There is at least one other VA mapping this page.
			 * Check if they are cache index compatible. If not
			 * remove all mappings, flush the cache and set page
			 * to be mapped uncached. Caching will be restored
			 * when pages are mapped compatible again.
			 * XXX - caching is not currently being restored, but
			 * XXX - I haven't seen the pages uncached since
			 * XXX - using pmap_prefer().	mhitch
			 */
			for (npv = pv; npv; npv = npv->pv_next) {
				/*
				 * Check cache aliasing incompatibility
				 */
				if ((npv->pv_va^va)&VA_ALIAS_MASK) {
					pmap_page_cache(pa, 0);
#ifdef DEBUG
					enter_stats.ci++;
#endif
					break;
				}
			}
		}
		/*
		 * There is at least one other VA mapping this page.
		 * Place this entry after the header.
		 *
		 * Note: the entry may already be in the table if
		 * we are only changing the protection bits.
		 */
		for (npv = pv; npv; npv = npv->pv_next) {
			if (pmap == npv->pv_pmap && PV_MATCH(npv, va)) {
#ifdef PARANOIADIAG
				int64_t data;

				data = pseg_get(pm, va);
				if (data >= 0 ||
				    data&TLB_PA_MASK != pa)
					printf(
		"pmap_enter: found va %lx pa %lx in pv_table but != %lx\n",
						va, pa, (long)data);
#endif
				goto fnd;
			}
		}
#ifdef DEBUG
		if (pmapdebug & PDB_ENTER)
			printf("pmap_enter: new pv: pmap %p va %lx\n",
				pmap, va);
#endif
		/* can this cause us to recurse forever? */
		npv = pool_get(&pv_pool, PR_NOWAIT);
		if (npv == NULL)
			panic("pmap_enter: new pv malloc() failed");
		PV_SETVA(npv, va);
		npv->pv_pmap = pmap;
		npv->pv_next = pv->pv_next;
		pv->pv_next = npv;
#ifdef DEBUG
		if (!npv->pv_next)
			enter_stats.secondpv++;
#endif
	fnd:
		;
	}
	splx(s);
}

/*
 * Remove a physical to virtual address translation.
 */

void
pmap_remove_pv(pmap, va, pa)
	pmap_t pmap;
	vaddr_t va;
	paddr_t pa;
{
	register pv_entry_t pv, npv, opv;
	int64_t data = 0LL;
	int s;

#ifdef DEBUG
	if (pmapdebug & (PDB_REMOVE))
		printf("pmap_remove_pv(pm=%p, va=%p, pa=%lx)\n", pmap, va, (long)pa);
#endif
	/*
	 * Remove page from the PV table (raise IPL since we
	 * may be called at interrupt time).
	 */
	if (!IS_VM_PHYSADDR(pa)) {
		printf("pmap_remove_pv(): %p not managed\n", pa);
		pv_check();
		return;
	}
	pv_check();
	opv = pv = pa_to_pvh(pa);
	s = splimp();
	/*
	 * If it is the first entry on the list, it is actually
	 * in the header and we must copy the following entry up
	 * to the header.  Otherwise we must search the list for
	 * the entry.  In either case we free the now unused entry.
	 */
	if (pmap == pv->pv_pmap && PV_MATCH(pv,va)) {
		/* Save modified/ref bits */
		data = pseg_get(pv->pv_pmap, pv->pv_va&PV_VAMASK);
		npv = pv->pv_next;
		if (npv) {
			/* First save mod/ref bits */
			npv->pv_va |= (pv->pv_va&PV_MASK);
			*pv = *npv;
			pool_put(&pv_pool, npv);
		} else {
			pv->pv_pmap = NULL;
			pv->pv_next = NULL;
			pv->pv_va &= (PV_REF|PV_MOD); /* Only save ref/mod bits */
		}
#ifdef DEBUG
		remove_stats.pvfirst++;
#endif
	} else {
		for (npv = pv->pv_next; npv; pv = npv, npv = npv->pv_next) {
#ifdef DEBUG
			remove_stats.pvsearch++;
#endif
			if (pmap == npv->pv_pmap && PV_MATCH(npv,va))
				goto fnd;
		}

		/* 
		 * Sometimes UVM gets confused and calls pmap_remove() instead
		 * of pmap_kremove() 
		 */
		return; 
#ifdef DIAGNOSTIC
		printf("pmap_remove_pv(%x, %x, %x) not found\n", pmap, va, pa);
		
		Debugger();
		splx(s);
		return;
#endif
	fnd:
		pv->pv_next = npv->pv_next;
		/* 
		 * move any referenced/modified info to the base pv
		 */
		data = pseg_get(npv->pv_pmap, npv->pv_va&PV_VAMASK);
		/* 
		 * Here, if this page was aliased, we should try clear out any
		 * alias that may have occurred.  However, that's a complicated
		 * operation involving multiple scans of the pv list. 
		 */
		pool_put(&pv_pool, npv);
	}

	/* Save ref/mod info */
	if (data & TLB_ACCESS) 
		opv->pv_va |= PV_REF;
	if (data & (TLB_W|TLB_MODIFY))
		opv->pv_va |= PV_MOD;
	splx(s);
	pv_check();
}

/*
 *	pmap_page_cache:
 *
 *	Change all mappings of a page to cached/uncached.
 */
void
pmap_page_cache(pa, mode)
	paddr_t pa;
	int mode;
{
	pv_entry_t pv;
	int i, s;

#ifdef DEBUG
	if (pmapdebug & (PDB_ENTER))
		printf("pmap_page_uncache(%lx)\n", pa);
#endif
	if (!IS_VM_PHYSADDR(pa))
		return;

	pv = pa_to_pvh(pa);
	s = splimp();
	while (pv) {
		vaddr_t va;

		va = (pv->pv_va & PV_VAMASK);
		if (mode) {
			/* Enable caching */
			if (pseg_set(pv->pv_pmap, va, 
				     pseg_get(pv->pv_pmap, va) | TLB_CV, 0)) {
				printf("pmap_page_cache: aliased pseg empty!\n");
				Debugger();
				/* panic? */
			}
		} else {
			/* Disable caching */
			if (pseg_set(pv->pv_pmap, va, 
				     pseg_get(pv->pv_pmap, va) & ~TLB_CV, 0)) {
				printf("pmap_page_cache: aliased pseg empty!\n");
				Debugger();
				/* panic? */
			}
		}
		if (pv->pv_pmap->pm_ctx || pv->pv_pmap == pmap_kernel()) {
			i = ptelookup_va(va);
			if (tsb[i].tag.tag > 0 && tsb[i].tag.tag == 
			    TSB_TAG(0, pv->pv_pmap->pm_ctx, va)) {
				/* 
				 * Invalidate the TSB 
				 * 
				 * While we can invalidate it by clearing the
				 * valid bit:
				 *
				 * ptp->data.data_v = 0;
				 *
				 * it's faster to do store 1 doubleword.
				 */
				tsb[i].data.data = 0LL; 
				ASSERT((tsb[i].data.data & TLB_NFO) == 0);
			}
			/* Force reload -- protections may be changed */
			tlb_flush_pte(va, pv->pv_pmap->pm_ctx);	
		}
		
		pv = pv->pv_next;
	}

	splx(s);
}

/*
 *	vm_page_alloc1:
 *
 *	Allocate and return a memory cell with no associated object.
 */
vm_page_t
vm_page_alloc1()
{
	vm_page_t pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_USERESERVE);
	if (pg) {
		pg->flags &= ~PG_BUSY;	/* never busy */
		pg->wire_count = 1;	/* no mappings yet */
	}
	return pg;
}

/*
 *	vm_page_free1:
 *
 *	Returns the given page to the free list,
 *	disassociating it with any VM object.
 *
 *	Object and page must be locked prior to entry.
 */
void
vm_page_free1(mem)
	register vm_page_t	mem;
{
	if (mem->flags != (PG_CLEAN|PG_FAKE)) {
		printf("Freeing invalid page %p\n", mem);
		Debugger();
		return;
	}
	mem->flags |= PG_BUSY;
	mem->wire_count = 0;
	uvm_pagefree(mem);
}

#ifdef DDB

void db_dump_pv __P((db_expr_t, int, db_expr_t, char *));
void
db_dump_pv(addr, have_addr, count, modif)
	db_expr_t addr;
	int have_addr;
	db_expr_t count;
	char *modif;
{
	struct pv_entry *pv;

	if (!have_addr) {
		db_printf("Need addr for pv\n");
		return;
	}

	for (pv = pa_to_pvh(addr); pv; pv = pv->pv_next)
		db_printf("pv@%p: next=%p pmap=%p va=0x%x\n",
			  pv, pv->pv_next, pv->pv_pmap, pv->pv_va);
	
}

#endif

#ifdef DEBUG
/*
 * Test ref/modify handling.
 */
void pmap_testout __P((void));
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
	va = (vaddr_t)(vmmap - NBPG);
	ASSERT(va != NULL);
	loc = (int*)va;

	pg = vm_page_alloc1();
	pa = (paddr_t)VM_PAGE_TO_PHYS(pg);
	pmap_enter(pmap_kernel(), va, pa, VM_PROT_ALL, VM_PROT_ALL);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       va, (long)pa,
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
	       va, (long)pa,
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
	       va, (long)pa,
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

	/* Unmap page */
	pmap_remove(pmap_kernel(), va, va+1);
	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Unmapped page: ref %d, mod %d\n",
	       ref, mod);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       va, (long)pa,
	       ref, mod);

	/* Check it's properly cleared */
	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Checking cleared page: ref %d, mod %d\n",
	       ref, mod);

	pmap_remove(pmap_kernel(), va, va+1);
	vm_page_free1(pg);
}
#endif
