/*	$NetBSD: pmap.c,v 1.22 1999/01/10 15:01:32 mrg Exp $	*/
/* #define NO_VCACHE */ /* Don't forget the locked TLB in dostart */
#define HWREF 
/* #define BOOT_DEBUG */
/* #define BOOT1_DEBUG */
/* #define printf	db_printf */
/*
 * 
 * Copyright (C) 1996, 1997 Eduardo Horvath.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "opt_uvm.h"
#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/systm.h>
#include <sys/msgbuf.h>
#include <sys/lock.h>
#include <sys/exec.h>
#include <sys/core.h>
#include <sys/kcore.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_pageout.h>

#if defined(UVM)
#include <uvm/uvm.h>
#endif

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
#endif

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
#define	PV_ALIAS	0x1LL
#define PV_REF		0x2LL
#define PV_MOD		0x4LL
#define PV_NVC		0x8LL
#define PV_WE		0x10LL		/* Debug -- track if this page was ever writable */
#define PV_MASK		(0x01fLL)
#define PV_VAMASK	(~(NBPG-1))
#if 0
#define PV_MATCH(pv,va)	(((pv)->pv_va) == (va))
#define PV_SETVA(pv,va) ((pv)->pv_va = (va))
#else
#define PV_MATCH(pv,va)	(!((((pv)->pv_va)^(va))&PV_VAMASK))
#define PV_SETVA(pv,va) ((pv)->pv_va = ((va)&PV_VAMASK)|(((pv)->pv_va)&PV_MASK))
#endif

pv_entry_t	pv_table;	/* array of entries, one per page */
extern void	pmap_remove_pv __P((struct pmap *pm, vaddr_t va, paddr_t pa));

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

/* 
 * And here's the IOMMU TSB stuff, also allocated in pmap_bootstrap.
 */
int64_t		*iotsb;
paddr_t		iotsbp;
int		iotsbsize; /* tsbents = 1024 * 2 ^^ tsbsize */
#define IOTSBENTS	(1024<<iotsbsize)
#define IOTSBSIZE	(IOTSBENTS * 8)

struct pmap kernel_pmap_;

int physmem;
u_long ksegv;				/* vaddr start of kernel */
u_int64_t ksegp;			/* paddr of start of kernel */
u_long ksegend;
u_int64_t ksegpend;
static int npgs;
static u_int nextavail;
static struct mem_region memlist[8]; /* Pick a random size here */

caddr_t	vmmap;			/* one reserved MI vpage for /dev/mem */

struct mem_region *mem, *avail, *orig;
int memsize;

static int memh = 0, vmemh = 0;	/* Handles to OBP devices */

static int pmap_initialized;

int avail_start, avail_end;	/* These are used by ps & family */

static int ptelookup_va __P((vaddr_t va)); /* sun4u */
static void tsb_enter __P((int ctx, int64_t va, int64_t data));
static void pmap_pinit __P((struct pmap *));
static void pmap_release __P((pmap_t));
#if 0
static int pv_syncflags __P((pv_entry_t));
#endif

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
 * of the same address.  Contextx on the spitfire are 13 bits, but could
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

#if defined(MACHINE_NEW_NONCONTIG)
#if defined(UVM)
#define	pmap_get_page(p)	uvm_page_physget((p));
#else
#define	pmap_get_page(p)	vm_page_physget((p));
#endif
#else
#define	pmap_get_page(p)		\
	do {				\
		*(p) = avail->start;	\
		avail->start += NBPG;	\
		avail->size -= NBPG;	\
		if (!avail->size)	\
			avail++;	\
	} while (0)
#endif
/*
 * This is called during initppc, before the system is really initialized.
 *
 * It's called with the start and end virtual addresses of the kernel.
 * We bootstrap the pmap allocator now.  We will allocate the basic
 * structures we need to bootstrap the VM system here: the page frame
 * tables, the TSB, and the free memory lists.
 */
void
pmap_bootstrap(kernelstart, kernelend, maxctx)
	u_long kernelstart, kernelend;
	u_int maxctx;
{
	extern int msgbufmapped;
	struct mem_region *mp, *mp1;
	int msgbufsiz;
	int pcnt;
	size_t s, sz;
	int i, j;
	u_int64_t phys_msgbuf;
	u_long firstaddr, newkp, ksize;
	u_long *newkv;
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
#if defined(UVM)
	uvmexp.pagesize = NBPG;
	uvm_setpagesize();
#else
	cnt.v_page_size = NBPG;
	vm_set_page_size();
#endif
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
		prom_printf("memlist start %p size %lx\r\n", (void *)mp->start,
		    (u_long)mp->size);
	}
	prom_printf("End of available virutal memory\r\n");
#endif
	/* 
	 * Get hold or the message buffer.
	 */
	msgbufp = (struct kern_msgbuf *)MSGBUF_VA;
	msgbufsiz = NBPG /* round_page(sizeof(struct msgbuf)) */;
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
			  -1/* sunos does this */) != -1)
		prom_printf("Failed to map msgbuf\r\n");
#ifdef BOOT_DEBUG
	else
		prom_printf("msgbuf mapped at %p\r\n", (void *)msgbufp);
#endif
	msgbufmapped = 1;	/* enable message buffer */
	initmsgbuf((caddr_t)msgbufp, msgbufsiz);

	/* 
	 * Record kernel mapping -- we will map this with a permanent 4MB
	 * TLB entry when we initialize the CPU later.
	 */
#ifdef BOOT_DEBUG
	prom_printf("translating kernelstart %p\r\n", (void *)kernelstart);
#endif
	ksegv = kernelstart;
	ksegp = prom_vtop(kernelstart);

	/*
	 * Find the real size of the kernel.  Locate the smallest starting
	 * address > kernelstart.
	 */
	for (mp1 = mp = memlist; mp->size; mp++) {
		/*
		 * Check whether this region is at the end of the kernel.
		 */
		if (mp->start > kernelstart && (mp1->start < kernelstart || 
						mp1->start > mp->start))
			mp1 = mp;
	}
	if (mp1->start < kernelstart)
		prom_printf("Kernel at end of vmem???\r\n");
#ifdef BOOT1_DEBUG
	prom_printf("The kernel is mapped at %lx, next free seg: %lx, %lx\r\n",
		    (long)ksegp, (u_long)mp1->start, (u_long)mp1->size);
#endif	
	/* 
	 * This it bogus and will be changed when the kernel is rounded to 4MB.
	 */
	firstaddr = (kernelend + 07) & ~ 07;	/* Longword align */

#if 1
#define	valloc(name, type, num) (name) = (type *)firstaddr; firstaddr += (num)
#else
#define	valloc(name, type, num) (name) = (type *)firstaddr; firstaddr = \
	(vaddr_t)((name)+(num))
#endif
#define MEG		(1<<20) /* 1MB */

	/*
	 * Since we can't give the loader the hint to align us on a 4MB
	 * boundary, we will need to do the alignment ourselves.  First
	 * allocate a new 4MB aligned segment for the kernel, then map it
	 * in, copy the kernel over, swap mappings, then finally, free the
	 * old kernel.  Then we can continue with this.
	 */
	ksize = round_page(mp1->start - kernelstart);

	if (ksegp & (4*MEG-1)) {
#ifdef BOOT1_DEBUG
		prom_printf("Allocating new %lx kernel at 4MB boundary\r\n",
		    (u_long)ksize);
#endif
		if ((newkp = prom_alloc_phys(ksize, 4*MEG)) == 0 ) {
			prom_printf("Cannot allocate new kernel\r\n");
			OF_exit();
		}
#ifdef BOOT1_DEBUG
		prom_printf("Allocating new va for buffer at %p\r\n",
		    (void *)newkp);
#endif
		if ((newkv = (u_long*)prom_alloc_virt(ksize, 8)) ==
		    (u_long*)-1) {
			prom_printf("Cannot allocate new kernel va\r\n");
			OF_exit();
		}
#ifdef BOOT1_DEBUG
		prom_printf("Mapping in buffer %lx at %lx\r\n",
		    (u_long)newkp, (u_long)newkv);
#endif
		prom_map_phys(newkp, 4*MEG, (vaddr_t)newkv, -1); 
#ifdef BOOT1_DEBUG
		prom_printf("Copying kernel...");
#endif
		bzero(newkv, 4*MEG);
		bcopy((void *)kernelstart, (void *)newkv,
		    kernelend - kernelstart);
#ifdef BOOT1_DEBUG
		prom_printf("done.  Swapping maps..unmap new\r\n");
#endif
		prom_unmap_virt((vaddr_t)newkv, 4*MEG);
#ifdef BOOT_DEBUG
		prom_printf("remap old ");
#endif
		prom_map_phys(newkp, 4*MEG, kernelstart, -1); 
		/*
		 * we will map in 4MB, more than we allocated, to allow
		 * further allocation
		 */
#ifdef BOOT1_DEBUG
		prom_printf("free old\r\n");
#endif
		prom_free_phys(ksegp, ksize);
		ksegp = newkp;
		
#ifdef BOOT1_DEBUG
		prom_printf("pmap_bootstrap: firstaddr is %lx virt (%lx phys)"
		    "avail for kernel\r\n", (u_long)firstaddr,
		    (u_long)prom_vtop(firstaddr));
#endif
	} else {
		/* We was at a 4MB boundary after all! */
		newkp = ksegp;

		/* Make sure all 4MB are mapped */
		prom_map_phys(ksegp, 4*MEG, kernelstart, -1); 
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
	 * Count the number of available entries.  And make an extra
	 * copy to fiddle with.
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
	 * Here's a quick in-lined reverse bubble sort.  It gets rid of
	 * any translations inside the kernel VA range.
	 */
	for(i = 0; i < prom_map_size; i++) {
		if (prom_map[i].vstart >= ksegv &&
		    prom_map[i].vstart <= firstaddr) {
			prom_map[i].vstart = 0;
			prom_map[i].vsize = 0;
		}
		for(j = i; j < prom_map_size; j++) {
			if (prom_map[j].vstart >= ksegv &&
			    prom_map[j].vstart <= firstaddr)
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
	/*
	 * Allocate a single IOMMU TSB so they're all mapped coherently.
	 */
	iotsbsize = 0; /* We will only allocate an 8K TSB now */
	valloc(iotsb, int64_t, IOTSBSIZE);
	iotsbp = ((vaddr_t)iotsb) - kernelstart + ksegp; 
	bzero(iotsb, IOTSBSIZE);	/* Invalidate all entries */	


	/* initialize pv_list stuff */
	first_phys_addr = mem->start;
	valloc(pv_table, struct pv_entry, sizeof(struct pv_entry)*physmem);
	bzero((caddr_t)pv_table, sizeof(struct pv_entry)*physmem);
#ifdef BOOT1_DEBUG
	prom_printf("Allocating pv_table at %lx,%lx\r\n", (u_long)pv_table, 
		    (u_long)sizeof(struct pv_entry)*physmem);
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
	kernelstart = kernelstart & ~PGOFSET;
	kernelend = firstaddr;
	kernelend = (kernelend + PGOFSET) & ~PGOFSET;
#ifdef BOOT1_DEBUG
	prom_printf("kernel virtual size %08lx - %08lx\r\n",
	    (u_long)kernelstart, (u_long)kernelend);
#endif
	ksegend = kernelend;
	ksegpend = kernelend - kernelstart + ksegp;
	/* Switch from vaddrs to paddrs */
	kernelstart = ksegp & ~PGOFSET;
	kernelend = ksegpend;
	if(kernelend > (kernelstart + 4*MEG)) {
		prom_printf("Kernel size exceeds 4MB\r\n");
		panic("kernel segment size exceeded\n");
		OF_exit();
	}

#if 0       
	/* DEBUG -- don't allow these pages to be used. */
	kernelend = (kernelstart + 4*MEG);
#endif
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
	prom_printf("kernel physical size %08lx - %08lx\r\n",
	    (u_long)kernelstart, (u_long)kernelend);
#endif
	/*
	 * Here's a another quick in-lined bubble sort.
	 */
	for (i = 0; i < pcnt; i++) {
		/* XXXMRG: why add up npgs when it is set to zero below? */
		npgs += btoc(avail[i].size);
		for (j = i; j < pcnt; j++) {
			if (avail[j].start < avail[i].start) {
				struct mem_region tmp;
				tmp = avail[i];
				avail[i] = avail[j];
				avail[j] = tmp;
			}
		}
	}

	npgs = 0;
	for (mp = avail; mp->size; mp++) {
		/*
		 * Check whether this region holds all of the kernel.
		 */
		s = mp->start + mp->size;
		if (mp->start < kernelstart && s > kernelend) {
			avail[pcnt].start = kernelend;
			avail[pcnt++].size = s - kernelend;
			mp->size = kernelstart - mp->start;
		}
		/*
		 * Look whether this regions starts within the kernel.
		 */
		if (mp->start >= kernelstart && mp->start < kernelend) {
			s = kernelend - mp->start;
			if (mp->size > s)
				mp->size -= s;
			else
				mp->size = 0;
			mp->start = kernelend;
		}
		/*
		 * Now look whether this region ends within the kernel.
		 */
		s = mp->start + mp->size;
		if (s > kernelstart && s < kernelend)
			mp->size -= s - kernelstart;
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
			bcopy(mp1, mp1 + 1, (void *)mp - (void *)mp1);
			mp1->start = s;
			mp1->size = sz;
		}
#if defined(MACHINE_NEW_NONCONTIG)
		/* 
		 * In future we should be able to specify both allocated
		 * and free.
		 */
#if defined(UVM)
		uvm_page_physload(
			atop(mp->start),
			atop(mp->start+mp->size),
			atop(mp->start),
			atop(mp->start+mp->size),
			VM_FREELIST_DEFAULT);
#else
		vm_page_physload(
			atop(mp->start),
			atop(mp->start+mp->size),
			atop(mp->start),
			atop(mp->start+mp->size));
#endif
#endif
	}

#ifdef BOOT_DEBUG
	/* Throw away page zero if we have it. */
	if (avail->start == 0) {
		avail->start += NBPG;
		avail->size -= NBPG;
	}
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
		pmap_kernel()->pm_segs=(paddr_t*)newp;
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
	{
		pte_t tte;
		vaddr_t va = (vaddr_t)msgbufp;
		paddr_t newp;

		prom_map_phys(phys_msgbuf, NBPG, (vaddr_t)msgbufp, -1); 
#ifdef NO_VCACHE
		tte.data.data = TSB_DATA(0 /* global */, 
					 TLB_8K,
					 phys_msgbuf,
					 1 /* priv */,
					 1 /* Write */,
					 1 /* Cacheable */,
					 1 /* ALIAS -- Disable D$ */, 
					 1 /* valid */);
#else
		tte.data.data = TSB_DATA(0 /* global */, 
					 TLB_8K,
					 phys_msgbuf,
					 1 /* priv */,
					 1 /* Write */,
					 1 /* Cacheable */,
					 0 /* No ALIAS */, 
					 1 /* valid */);
#endif
		newp = NULL;
		while(pseg_set(pmap_kernel(), va, tte.data.data, newp)
		      != NULL) {
			pmap_get_page(&newp);
			pmap_zero_page(newp);
#ifdef DEBUG
			enter_stats.ptpneeded ++;
#endif
#ifdef BOOT1_DEBUG
			prom_printf(
			    "pseg_set: pm=%p va=%p data=%lx newp %lx\r\n", 
			    pmap_kernel(), va, (long)tte.data.data, (long)newp);
			{int i; for (i=0; i<140000000; i++) ;}
#endif
		}
		
		/* 
		 * Also add a global NFO mapping for page zero.
		 */
		tte.data.data = TSB_DATA(0 /* global */, 
					 TLB_8K,
					 0 /* Physaddr */,
					 1 /* priv */,
					 0 /* Write */,
					 1 /* Cacheable */,
					 0 /* No ALIAS */, 
					 1 /* valid */);
		tte.data.data |= TLB_L|TLB_NFO;
		newp = NULL;
		while(pseg_set(pmap_kernel(), va, tte.data.data, newp)
		      != NULL) {
			pmap_get_page(&newp);
			pmap_zero_page(newp);
#ifdef DEBUG
			enter_stats.ptpneeded ++;
#endif
		}
	}
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
				paddr_t newp;
				
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
#if 0
				/* Enter prom map into TSB */
				int k = ptelookup_va(prom_map[i].vstart+j);
				tsb[k].tag.tag = TSB_TAG(0, 0,
				    prom_map[i].vstart+j);
				tsb[k].data.data = prom_map[i].tte + j;
#endif
#if 1
				/* And into pmap_kernel() */
				newp = NULL;
				while (pseg_set(pmap_kernel(),
				    prom_map[i].vstart + j, 
				    (prom_map[i].tte + j) |
				    page_size_map[k].code, newp) != NULL) {
					pmap_get_page(&newp);
					pmap_zero_page(newp);
#ifdef DEBUG
					enter_stats.ptpneeded++;
#endif
				}
#else
				prom_printf("i=%d j=%d\r\n", i, j);
				pmap_enter_phys(pmap_kernel(),
				    (vaddr_t)prom_map[i].vstart + j, 
				    (prom_map[i].tte & TLB_PA_MASK) + j,
				    TLB_8K, VM_PROT_WRITE, 1);
#endif
			}
#ifdef BOOT1_DEBUG
	prom_printf("Done inserting PROM mappings into pmap_kernel()\r\n");
#endif


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
}

/*
 * Initialize anything else for pmap handling.
 * Called during vm_init().
 */
void
pmap_init()
{
#ifdef NOTDEF_DEBUG
	prom_printf("pmap_init()\r\n");
#endif
	if (PAGE_SIZE != NBPG)
		panic("pmap_init: CLSIZE!=1");
	pmap_initialized = 1;

	vm_first_phys = avail_start;
	vm_num_phys = avail_end - avail_start;
}

#if !defined(MACHINE_NEW_NONCONTIG)
/*
 * Return the index of the given page in terms of pmap_next_page() calls.
 */
int
pmap_page_index(pa)
	paddr_t pa;
{
	struct mem_region *mp;
	psize_t pre;
	
	pa &= ~PGOFSET;
	for (pre = 0, mp = avail; mp->size; mp++) {
		if (pa >= mp->start
		    && pa < mp->start + mp->size)
			return btoc(pre + (pa - mp->start));
		pre += mp->size;
	}
	return -1;
}
#endif

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
	vmmap = (caddr_t)(ksegv + 4*MEG); /* Start after our locked TLB entry */
	*start = (vaddr_t)(vmmap + NBPG);
	*end = VM_MAX_KERNEL_ADDRESS;
#ifdef NOTDEF_DEBUG
	prom_printf("pmap_virtual_space: %x-%x\r\n", *start, *end);
#endif
}

#ifdef MACHINE_NONCONTIG
/*
 * Return the number of possible page indices returned
 * from pmap_page_index for any page provided by pmap_next_page.
 */
u_int
pmap_free_pages()
{
	return npgs;
}

/*
 * If there are still physical pages available, put the address of
 * the next available one at paddr and return TRUE.  Otherwise,
 * return FALSE to indicate that there are no more free pages.
 */
int
pmap_next_page(paddr)
	paddr_t *paddr;
{
	static int lastidx = -1;
	
	if (lastidx < 0
	    || nextavail >= avail[lastidx].start + avail[lastidx].size) {
		if (avail[++lastidx].size == 0) {
#ifdef NOT_DEBUG
			printf("pmap_next_page: failed lastidx=%d nextavail=%x "
			       "avail[lastidx]=(%x:%08x,%x:%08x)\n",
			       lastidx, nextavail,
			       (int)(avail[lastidx].start>>32), (int)(avail[lastidx].start),
			       (int)(avail[lastidx].size>>32), (int)(avail[lastidx].size));
#endif
			return FALSE;
		}
		nextavail = avail[lastidx].start;
	}
	*paddr = nextavail;
	nextavail += NBPG;
#ifdef NOTDEF_DEBUG
	printf("pmap_next_page: OK lastidx=%d nextavail=%x "
	       "avail[lastidx]=(%x:%08x,%x:%08x)\n",
	       lastidx, nextavail, 
	       (int)(avail[lastidx].start>>32), (int)(avail[lastidx].start),
	       (int)(avail[lastidx].size>>32), (int)(avail[lastidx].size));
#endif
	return TRUE;
}
#endif

/*
 * Create and return a physical map.
 */
#if defined(PMAP_NEW)
struct pmap *
pmap_create()
#else
struct pmap *
pmap_create(size)
	vsize_t size;
#endif
{
	struct pmap *pm;
	
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
#if defined(UVM)
			uvm_wait("pmap_pinit");
#else
			VM_WAIT;
#endif
		}
		pm->pm_physaddr = (paddr_t)VM_PAGE_TO_PHYS(page);
		pmap_zero_page(pm->pm_physaddr);
		pm->pm_segs = (paddr_t*)pm->pm_physaddr;
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
		if((pdir = (paddr_t *)ldxa(&pm->pm_segs[i], ASI_PHYS_CACHED))) {
			for (k=0; k<PDSZ; k++) {
				if ((ptbl = (paddr_t *)ldxa(&pdir[k], ASI_PHYS_CACHED))) {
					for (j=0; j<PTSZ; j++) {
						int64_t data = ldxa(&ptbl[j], ASI_PHYS_CACHED);
						if (data&TLB_V && 
						    IS_VM_PHYSADDR(data&TLB_PA_MASK))
							pmap_remove_pv(pm, 
								       ((long)i<<STSHIFT)|((long)k<<PDSHIFT)|((long)j<<PTSHIFT), 
								       data&TLB_PA_MASK);
					}
					vm_page_free1((vm_page_t)PHYS_TO_VM_PAGE((paddr_t)ptbl));
					stxa(&pdir[k], ASI_PHYS_CACHED, NULL);
				}
			}
			vm_page_free1((vm_page_t)PHYS_TO_VM_PAGE((paddr_t)pdir));
			stxa(&pm->pm_segs[i], ASI_PHYS_CACHED, NULL);
		}
	vm_page_free1((vm_page_t)PHYS_TO_VM_PAGE((paddr_t)pm->pm_segs));
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
		if ((pdir = (paddr_t *)ldxa(&pm->pm_segs[i], ASI_PHYS_CACHED))) {
			m = 0;
			for (k=0; k<PDSZ; k++) {
				if ((ptbl = (paddr_t *)ldxa(&pdir[k], ASI_PHYS_CACHED))) {
					m++;
					n = 0;
					for (j=0; j<PTSZ; j++) {
						int64_t data = ldxa(&ptbl[j], ASI_PHYS_CACHED);
						if (data&TLB_V)
							n++;
					}
					if (!n) {
						/* Free the damn thing */
						vm_page_free1((vm_page_t)PHYS_TO_VM_PAGE((paddr_t)ptbl));
						stxa(&pdir[k], ASI_PHYS_CACHED, NULL);
					}
				}
			}
			if (!m) {
				/* Free the damn thing */
				vm_page_free1((vm_page_t)PHYS_TO_VM_PAGE((paddr_t)pdir));
				stxa(&pm->pm_segs[i], ASI_PHYS_CACHED, NULL);
			}
		}
	}
	splx(s);
#endif
}

/*
 * Make the specified pages pageable or not as requested.
 *
 * This routine is merely advisory.
 */
void
pmap_pageable(pm, start, end, pageable)
	struct pmap *pm;
	vaddr_t start, end;
	int pageable;
{
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
 * Insert physical page at pa into the given pmap at virtual address va.
 */
void
pmap_enter(pm, va, pa, prot, wired)
	struct pmap *pm;
	vaddr_t va;
	paddr_t pa;
	vm_prot_t prot;
	int wired;
{
	register u_int64_t phys;

	phys = pa;
	/* Call 64-bit clean version of pmap_enter */
	pmap_enter_phys(pm, va, phys, TLB_8K, prot, wired);
}

#if defined(PMAP_NEW)
/* Different interfaces to pmap_enter_phys */

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
				 (!(pa & PMAP_NC)), pa & (PMAP_NVC), 1);
	if (VM_PROT_WRITE & prot) tte.data.data |= TLB_REAL_W; /* HWREF -- XXXX */
	tte.data.data |= TLB_TSB_LOCK;	/* wired */
	ASSERT((tte.data.data & TLB_NFO) == 0);
	pg = NULL;
	while (pseg_set(pm, va, tte.data.data, pg) != NULL) {
#if defined(UVM)
		if (pmap_initialized || !uvm_page_physget(&pg)) {
#else
		if (pmap_initialized || !pmap_next_page(&pg)) {
#endif
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
#if defined(UVM)
				uvm_wait("pmap_kenter_pa");
#else
				VM_WAIT;
#endif
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
		prom_printf("pmap_kenter: va=%08x tag=%x:%08x data=%08x:%08x tsb[%d]=%08x\r\n", va,
			    (int)(tte.tag.tag>>32), (int)tte.tag.tag, 
			    (int)(tte.data.data>>32), (int)tte.data.data, 
			    i, &tsb[i]);
	if( pmapdebug & PDB_MMU_STEAL && tsb[i].data.data ) {
		prom_printf("pmap_kenter: evicting entry tag=%x:%08x data=%08x:%08x tsb[%d]=%08x\r\n",
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
#if 0
	/* this is correct */
	dcache_flush_page(va);
#else
	/* Go totally crazy */
	blast_vcache();
#endif
#endif

}

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
void
pmap_kremove(va, size)
	vaddr_t va;
	vsize_t size;
{
	struct pmap *pm = pmap_kernel();
	int64_t data;
	int i, flush = 0;

	while (size >= NBPG) {
		/*
		 * Is this part of the permanent 4MB mapping?
		 */
#ifdef DIAGNOSTIC
		if( pm == pmap_kernel() && va >= ksegv && va < ksegv+4*MEG )
			panic("pmap_remove: va=%08x in locked TLB\r\n", va);
#endif
		if ((data = pseg_get(pm, va))<0) {
			paddr_t entry;
			
			flush |= 1;
			entry = (data&TLB_PA_MASK);
			/* We need to flip the valid bit and clear the access statistics. */
			if (pseg_set(pm, va, 0, 0)) {
				printf("pmap_remove: gotten pseg empty!\n");
				Debugger();
				/* panic? */
			}
#ifdef DEBUG
			if (pmapdebug & PDB_REMOVE)
				printf(" clearing seg %x pdir %x pte %x\n", 
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
				if (pmapdebug & PDB_REMOVE)
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
#endif /* PMAP_NEW */

/*
 * Insert physical page at pa into the given pmap at virtual address va.
 * Supports 64-bit pa so we can map I/O space.
 */
void
pmap_enter_phys(pm, va, pa, size, prot, wired)
	struct pmap *pm;
	vaddr_t va;
	u_int64_t pa;
	u_int64_t size;
	vm_prot_t prot;
	int wired;
{
	pte_t tte;
	int s, i, aliased = 0;
	register pv_entry_t pv=NULL, npv;
	paddr_t pg;

	/*
	 * Is this part of the permanent 4MB mapping?
	 */
#ifdef DIAGNOSTIC
	if (pm == pmap_kernel() && va >= ksegv && va < ksegv+4*MEG) {
		prom_printf("pmap_enter: va=%08x pa=%x:%08x in locked TLB\r\n", 
			    va, (int)(pa>>32), (int)pa);
		OF_enter();
		return;
	}
#endif

#ifdef DEBUG
	/* Trap mapping of page zero */
	if (va == NULL) {
		prom_printf("pmap_enter: NULL va=%08x pa=%x:%08x\r\b", 
			    va, (int)(pa>>32), (int)pa);
		OF_enter();
	}
#endif
	/* allocate new page table if needed */
#ifdef NOTDEF_DEBUG
	if (pa>>32)
		prom_printf("pmap_enter: va=%08x 64-bit pa=%x:%08x seg=%08x pte=%08x\r\n", 
			    va, (int)(pa>>32), (int)pa, (int)va_to_seg(va), (int)va_to_pte(va));
#endif
	/*
	 * Construct the TTE.
	 */
	if (IS_VM_PHYSADDR(pa)) {
		pv = pa_to_pvh(pa);
		aliased = (pv->pv_va&(PV_ALIAS|PV_NVC));
		if ((tte.data.data = pseg_get(pm, va))<0 &&
		    ((tte.data.data^pa)&TLB_PA_MASK)) {
			vaddr_t entry;
			
			/* different mapping for this page exists -- remove it. */
			entry = (tte.data.data&TLB_PA_MASK);
			pmap_remove_pv(pm, va, entry);
		}		
#ifndef HWREF
		/* If we don't have the traphandler do it set the ref/mod bits now */
		pv->pv_va |= PV_REF;
		if (VM_PROT_WRITE & prot)
			pv->pv_va |= PV_MOD;
#endif
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
	tte.tag.tag = TSB_TAG(0,pm->pm_ctx,va);
#ifndef HWREF
	tte.data.data = TSB_DATA(0, size, pa, pm == pmap_kernel(),
				 (VM_PROT_WRITE & prot),
				 (!(pa & PMAP_NC)),aliased,1);
	if (VM_PROT_WRITE & prot) tte.data.data |= TLB_REAL_W; /* HWREF -- XXXX */
#else
	/* Force dmmu_write_fault to be executed */
	tte.data.data = TSB_DATA(0, size, pa, pm == pmap_kernel(),
				 0/*(VM_PROT_WRITE & prot)*/,
				 (!(pa & PMAP_NC)),aliased,1);
	if (VM_PROT_WRITE & prot) tte.data.data |= TLB_REAL_W; /* HWREF -- XXXX */
#endif
	if (wired) tte.data.data |= TLB_TSB_LOCK;
	ASSERT((tte.data.data & TLB_NFO) == 0);
	pg = NULL;
#ifdef NOTDEF_DEBUG
	printf("pmap_enter_phys: inserting %x:%x at %x\n", (int)(tte.data.data>>32), (int)tte.data.data, (int)va);
#endif
	while (pseg_set(pm, va, tte.data.data, pg) != NULL) {
#if defined(UVM)
		if (pmap_initialized || !uvm_page_physget(&pg)) {
#else
		if (pmap_initialized || !pmap_next_page(&pg)) {
#endif
			vm_page_t page;
#ifdef NOTDEF_DEBUG
			printf("pmap_enter_phys: need to alloc page\n");
#endif
			while ((page = vm_page_alloc1()) == NULL) {
				/*
				 * Let the pager run a bit--however this may deadlock
				 */
#ifdef NOTDEF_DEBUG
				printf("pmap_enter_phys: calling uvm_wait()\n");
#endif
#if defined(UVM)
				uvm_wait("pmap_enter_phys");
#else
				VM_WAIT;
#endif
			}
			pg = (paddr_t)VM_PAGE_TO_PHYS(page);
		} 
		pmap_zero_page((paddr_t)pg);
#ifdef DEBUG
		enter_stats.ptpneeded ++;
#endif
#ifdef NOTDEF_DEBUG
	printf("pmap_enter_phys: inserting %x:%x at %x with %x\n", (int)(tte.data.data>>32), (int)tte.data.data, (int)va, (int)pg);
#endif
	}

	if (pv) {
       		/*
		 * Enter the pmap and virtual address into the
		 * physical to virtual map table.
		 */
		s = splimp();
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
				aliased = (aliased || (pm ==  npv->pv_pmap && ((pv->pv_va^npv->pv_va)&VA_ALIAS_MASK)));
				if (pm == npv->pv_pmap && PV_MATCH(npv,va)) {
#ifdef XXXX_DIAGNOSTIC
					unsigned entry;
					
					if (!pm->pm_segtab)
						entry = kvtopte(va)->pt_entry;
					else {
						pte = pmap_segmap(pm, va);
						if (pte) {
							pte += (va >> PGSHIFT) &
								(NPTEPG - 1);
							entry = pte->pt_entry;
						} else
							entry = 0;
					}
					if (!(entry & PG_V) ||
					    (entry & PG_FRAME) != pa)
						printf("pmap_enter: found va %x pa %x in pv_table but != %x\n",
						       va, pa, entry);
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
			npv = (pv_entry_t)
				malloc(sizeof *npv, M_VMPVENT, M_WAITOK);
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
						printf("pmap_enter_phys: aliased page %p:%p\n", 
						       (int)(pa>>32), (int)pa);
#endif
				for (npv = pv; npv; npv = npv->pv_next) 
					if (npv->pv_pmap == pm) {
#ifdef DEBUG
						if (pmapdebug & PDB_ALIAS) 
							printf("pmap_enter_phys: dealiasing %p in ctx %d\n", 
							       npv->pv_va, npv->pv_pmap->pm_ctx);
#endif
						/* Turn off cacheing of this TTE */
						if (pseg_set(npv->pv_pmap, va, pseg_get(npv->pv_pmap, va) & ~TLB_CV, 0)) {
							printf("pmap_enter_phys: aliased pseg empty!\n");
							Debugger();
							/* panic? */
						}
						/* This may cause us to enter the same mapping twice. */
						tsb_enter(npv->pv_pmap->pm_ctx,(npv->pv_va&PV_VAMASK),
							  pseg_get(npv->pv_pmap, va));
#if 0
						/* XXXXXX We should now flush the DCACHE to make sure */
						dcache_flush_page((pv->pv_va&PV_VAMASK));
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
	tsb_enter(pm->pm_ctx, va, tte.data.data);
	ASSERT((tsb[i].data.data & TLB_NFO) == 0);
#if 1
#if 0
	/* this is correct */
	dcache_flush_page(va);
#else
	/* Go totally crazy */
	blast_vcache();
#endif
#endif
	/* We will let the fast mmu miss interrupt load the new translation */
#if 0
	/* Tell prom about our mappings so we can debug w/OBP after a watchdog */
	if (pm->pm_ctx)	prom_map_phys(pa, NBPG, va, -1);
#endif
	pv_check();
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
		printf("pmap_remove(pm=%x, va=%x, pa=%x):", pm, va, endva);
	remove_stats.calls ++;
#endif

	/* Now do the real work */
	while (va < endva) {
		/*
		 * Is this part of the permanent 4MB mapping?
		 */
#ifdef DIAGNOSTIC
		if( pm == pmap_kernel() && va >= ksegv && va < ksegv+4*MEG ) 
			panic("pmap_remove: va=%08x in locked TLB\r\n", va);
#endif

		if ((data = pseg_get(pm, va))<0) {
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
		if( pm == pmap_kernel() && sva >= ksegv && sva < ksegv+4*MEG ) {
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
				prom_printf("pmap_protect: va=%08x data=%x:%08x seg=%08x pte=%08x\r\n", 
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
 */
paddr_t
pmap_extract(pm, va)
	register struct pmap *pm;
	vaddr_t va;
{
	paddr_t pa;

	if( pm == pmap_kernel() && va >= ksegv && va < ksegv+4*MEG ) {
		/* Need to deal w/locked TLB entry specially. */
		pa = (paddr_t) (ksegp - ksegv + va);
#ifdef DEBUG
		if (pmapdebug & PDB_EXTRACT) {
			db_printf("pmap_extract: va=%x pa=%lx\n", va, (long)pa);
		}
#endif
	} else {
		pa = (pseg_get(pm, va)&TLB_PA_MASK)+(va&PGOFSET);
#ifdef DEBUG
		if (pmapdebug & PDB_EXTRACT) {
			paddr_t pa;
			pa = ldxa(&pm->pm_segs[va_to_seg(va)], ASI_PHYS_CACHED);
			printf("pmap_extract: va=%p segs[%ld]=%lx", va, (long)va_to_seg(va), (long)pa);
			if (pa) {
				pa = ldxa(&((paddr_t*)pa)[va_to_dir(va)], ASI_PHYS_CACHED);
				printf(" segs[%ld][%ld]=%lx", va_to_seg(va), (long)va_to_dir(va), (long)pa);
			}
			if (pa)	{
				pa = ldxa(&((paddr_t*)pa)[va_to_pte(va)], ASI_PHYS_CACHED);
				printf(" segs[%ld][%ld][%ld]=%lx", (long)va_to_seg(va), 
				       (long)va_to_dir(va), (long)va_to_pte(va), (long)pa);
			}
			db_printf(" pseg_get: %lx\n", (long)pa);
		}
#endif
	}
	return pa;
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
			pmap_enter_phys(pmap_kernel(), va, pa, page_size_map[i].code, prot, 1);
			va += pgsize;
			pa += pgsize;
		} while (pa & page_size_map[i].mask);
	}
	return (va);
}
#endif

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
		if( pm == pmap_kernel() && sva >= ksegv && sva < ksegv+4*MEG ) {
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
			tlb_flush_pte(sva, pm->pm_ctx);
		}
		if (tsb[i].tag.tag > 0 
		    && tsb[i].tag.tag == TSB_TAG(0,pm->pm_ctx,sva)) 
			tsb[i].tag.tag = tsb[i].data.data = 0LL;
		splx(s);
		sva += NBPG;
	}
	pv_check();
}

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
	kcore_seg_t	*ksegp;
	cpu_kcore_hdr_t	*kcpup;
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
	ksegp = (kcore_seg_t *)bp;
	CORE_SETMAGIC(*ksegp, KCORE_MAGIC, MID_MACHINE, CORE_CPU);
	ksegp->c_size = dbtob(pmap_dumpsize()) - ALIGN(sizeof(kcore_seg_t));

	/* Fill in MD segment header (interpreted by MD part of libkvm) */
	kcpup = (cpu_kcore_hdr_t *)((long)bp + ALIGN(sizeof(kcore_seg_t)));
	kcpup->cputype = CPU_SUN4U;
	kcpup->kernbase = KERNBASE;
	kcpup->kphys = (paddr_t)ksegp;
	kcpup->nmemseg = memsize;
	kcpup->memsegoffset = memsegoffset = ALIGN(sizeof(cpu_kcore_hdr_t));
	kcpup->nsegmap = STSZ;
	kcpup->segmapoffset = segmapoffset =
		memsegoffset + memsize * sizeof(phys_ram_seg_t);

	kcpup->npmeg = 0; 
	kcpup->pmegoffset = 0; /* We don't do this. */

	/* Note: we have assumed everything fits in buffer[] so far... */
	bp = (int *)((long)kcpup + ALIGN(sizeof(cpu_kcore_hdr_t)));

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

#if 0
/* 
 * Lookup an entry in TSB -- returns NULL if not mapped. 
 *
 * At the moment it just looks up an entry in the TSB.
 * This will need to be changed to store ref and modified
 * info elsewhere or we will have severe data corruption.
 */
int 
ptelookup_pa(pa)
	paddr_t pa;
{
	register int i;

	/* Scan for PA in TSB */
	for( i=0; i<TSBENTS; i++ )
		if( (tsb[i].data.data&TLB_PA_MASK) == (pa&TLB_PA_MASK) )
			return i;
	return -1;
}
#endif

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

void tsb_enter(ctx, va, data)
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

/*
 * Do whatever is needed to sync the MOD/REF flags
 */
#if 0
int
pv_syncflags(pv)
	pv_entry_t pv;
{
	pv_entry_t npv;
	int s = splimp();
	int flags = pv->pv_va&PV_MASK;
	
#ifdef DEBUG	
	if (pv->pv_next && !pv->pv_pmap) {
		printf("pv_syncflags: npv but no pmap for pv %p\n", pv);
		Debugger();
	}
#endif
	if (pv->pv_pmap != NULL)
		for (npv = pv; npv; npv = npv->pv_next) {
			int64_t data;

			/* First clear the mod bit in the PTE and make it R/O */
			data = pseg_get(npv->pv_pmap, npv->pv_va&PV_VAMASK);
			/* Need to both clear the modify and write bits */
			if (data & (TLB_MODIFY|TLB_W))
				flags |= PV_MOD;
#ifdef HWREF
			if (data & (TLB_ACCESS))
				flags |= PV_REF;
#else
			if (data < 0)
				flags |= PV_REF;
#endif
			data &= ~(TLB_MODIFY|TLB_ACCESS);
			ASSERT((data & TLB_NFO) == 0);
			if (pseg_set(npv->pv_pmap, npv->pv_va&PV_VAMASK, data, 0)) {
				printf("pv_syncflags: gotten pseg empty!\n");
				Debugger();
				/* panic? */
			}
			/* Then clear the mod bit in the pv */
			flags |= npv->pv_va&PV_MASK;
			npv->pv_va &= ~(PV_MOD|PV_REF);
		}
	pv->pv_va |= flags&(PV_MOD|PV_REF);
#ifdef DEBUG
	if (pv->pv_va & PV_MOD)
		pv->pv_va |= PV_WE;	/* Remember this was modified */
#endif
	splx(s);
	return (flags);
}
#endif

#if defined(PMAP_NEW)
boolean_t
pmap_clear_modify(pg)
	struct vm_page *pg;
#else
void
pmap_clear_modify(pa)
	paddr_t pa;
#endif
{
#if defined(PMAP_NEW)
	paddr_t pa = PMAP_PGARG(pg);
	int changed = 0;
#endif
	int i, s;
	register pv_entry_t pv;
	
#ifdef DEBUG
	if (pmapdebug & (PDB_CHANGEPROT|PDB_REF))
		printf("pmap_clear_modify(%p)\n", pa);
#endif

	if (!IS_VM_PHYSADDR(pa)) {
		pv_check();
		goto out;
	}

	/* Clear all mappings */
	s = splimp();
	pv = pa_to_pvh(pa);
#ifdef DEBUG
	if (pv->pv_va & PV_MOD)
		pv->pv_va |= PV_WE;	/* Remember this was modified */
#endif
#if defined(PMAP_NEW)
	if (pv->pv_va & PV_MOD)
		changed |= 1;
#endif
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
#if defined(PMAP_NEW)
			if (data & (TLB_MODIFY|TLB_W))
				changed |= 1;
#endif
			data &= ~(TLB_MODIFY|TLB_W);
			ASSERT((data & TLB_NFO) == 0);
			if (pseg_set(pv->pv_pmap, pv->pv_va&PV_VAMASK, data, 0)) {
				printf("pmap_clear_modify: gotten pseg empty!\n");
				Debugger();
				/* panic? */
			}
			i = ptelookup_va(pv->pv_va&PV_VAMASK);
			if (tsb[i].tag.tag == TSB_TAG(0, pv->pv_pmap->pm_ctx, pv->pv_va&PV_VAMASK)) 
				tsb[i].data.data = 0LL;
			tlb_flush_pte(pv->pv_va&PV_VAMASK, pv->pv_pmap->pm_ctx);

			/* Then clear the mod bit in the pv */
#if defined(PMAP_NEW)
			if (pv->pv_va & PV_MOD)
				changed |= 1;
#endif
			pv->pv_va &= ~(PV_MOD);
		}
	splx(s);
	pv_check();
#ifdef DEBUG
#if defined(PMAP_NEW)
	if (pmap_is_modified(pg)) {
		printf("pmap_clear_modify(): %p still modified!\n", pg);
		Debugger();
	}
#else
	if (pmap_is_modified(pa)) {
		printf("pmap_clear_modify(): %p still modified!\n", (long)pa);
		Debugger();
	}
#endif
#endif
out:
#if defined(PMAP_NEW)
	return (changed);
#endif
}


#if defined(PMAP_NEW)
boolean_t
pmap_clear_reference(pg)
	struct vm_page* pg;
#else
void
pmap_clear_reference(pa)
	paddr_t pa;
#endif
{
#if defined(PMAP_NEW)
	paddr_t pa = PMAP_PGARG(pg);
	int changed = 0;
#endif
	int i, s;
	register pv_entry_t pv;

#ifdef DEBUG
	if (pmapdebug & (PDB_CHANGEPROT|PDB_REF))
		printf("pmap_clear_reference(%p)\n", pa);
#endif
	if (!IS_VM_PHYSADDR(pa)) {
		pv_check();
		goto out;
	}
	/* Clear all references */
	s = splimp();
	pv = pa_to_pvh(pa);
#ifdef NOT_DEBUG
	if (pv->pv_va & PV_MOD)
		printf("pmap_clear_reference(): pv %p still modified\n", (long)pa);
#endif
#if defined(PMAP_NEW)
	if (pv->pv_va & PV_REF)
		changed |= 1;
#endif
	pv->pv_va &= ~(PV_REF);
#ifdef DEBUG	
	if (pv->pv_next && !pv->pv_pmap) {
		printf("pmap_clear_reference: npv but no pmap for pv %p\n", pv);
		Debugger();
	}
#endif
	if (pv->pv_pmap != NULL)
		for (; pv; pv = pv->pv_next) {
			int64_t data;
			
			data = pseg_get(pv->pv_pmap, pv->pv_va&PV_VAMASK);
#if defined(PMAP_NEW)
			if (data & TLB_ACCESS)
				changed |= 1;
#endif
			data &= ~TLB_ACCESS;
#ifdef DEBUG
			if (pmapdebug & PDB_CHANGEPROT)
				printf("clearing ref pm:%p va:%p ctx:%x data:%x:%x\n", pv->pv_pmap,
				       pv->pv_va, pv->pv_pmap->pm_ctx, (int)(data>>32), (int)data);
#endif
			ASSERT((data & TLB_NFO) == 0);
			if (pseg_set(pv->pv_pmap, pv->pv_va, data, 0)) {
				printf("pmap_clear_reference: gotten pseg empty!\n");
				Debugger();
				/* panic? */
			}
			i = ptelookup_va(pv->pv_va&PV_VAMASK);
			if (tsb[i].tag.tag == TSB_TAG(0,pv->pv_pmap->pm_ctx,pv->pv_va&PV_VAMASK)) 
				tsb[i].data.data = 0LL;
#if defined(PMAP_NEW)
			if (pv->pv_va & PV_REF)
				changed |= 1;
#endif
			pv->pv_va &= ~(PV_REF);
			tlb_flush_pte(pv->pv_va&PV_VAMASK, pv->pv_pmap->pm_ctx);
			}
	/* Stupid here will take a cache hit even on unmapped pages 8^( */
	blast_vcache();
	splx(s);
	pv_check();
#ifdef DEBUG
#if defined(PMAP_NEW)
	if (pmap_is_referenced(pg)) {
		printf("pmap_clear_reference(): %p still referenced!\n", pg);
		Debugger();
	}
#else
	if (pmap_is_referenced(pa)) {
		printf("pmap_clear_reference(): %p still referenced!\n", (long)pa);
		Debugger();
	}
#endif
#endif
out:
#if defined(PMAP_NEW)
	return (changed);
#endif
}

#if defined(PMAP_NEW)
boolean_t
pmap_is_modified(pg)
	struct vm_page* pg;
#else
boolean_t
pmap_is_modified(pa)
	paddr_t pa;
#endif
{
#if defined(PMAP_NEW)
	paddr_t pa = PMAP_PGARG(pg);
#endif
	int i, s;
	register pv_entry_t pv, npv;

	if (!IS_VM_PHYSADDR(pa)) {
		pv_check();
		return 0;
	}
	/* Check if any mapping has been modified */
	s = splimp();
	pv = pa_to_pvh(pa);
#ifdef HWREF
	i = (pv->pv_va&PV_MOD);
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
			i = i || (data & (TLB_MODIFY|TLB_W));
		}
	/* Save modify info */
	if (i) pv->pv_va |= PV_MOD;
#ifdef DEBUG
	if (i) pv->pv_va |= PV_WE;
#endif
#else
	i = (pv->pv_va&PV_MOD);
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

#if defined(PMAP_NEW)
boolean_t
pmap_is_referenced(pg)
	struct vm_page* pg;
#else
boolean_t
pmap_is_referenced(pa)
	paddr_t pa;
#endif
{
#if defined(PMAP_NEW)
	paddr_t pa = PMAP_PGARG(pg);
#endif
	int i, s;
	register pv_entry_t pv, npv;

	if (!IS_VM_PHYSADDR(pa))
		return 0;

	/* Check if any mapping has been referenced */
	s = splimp();
	pv = pa_to_pvh(pa);
#ifdef HWREF 
	i = (pv->pv_va&PV_REF);
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
			i = i || (data & TLB_ACCESS);
		}
	if (i) pv->pv_va |= PV_REF;
#else
	i = (pv->pv_va&PV_REF);
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
 *	Routine:	pmap_change_wiring
 *	Function:	Change the wiring attribute for a map/virtual-address
 *			pair.
 *	In/out conditions:
 *			The mapping must already exist in the pmap.
 */
void
pmap_change_wiring(pmap, va, wired)
	register pmap_t	pmap;
	vaddr_t va;
	boolean_t wired;
{
	int64_t data;
	int s;

#ifdef DEBUG
	if (pmapdebug & (PDB_CHANGEPROT|PDB_REMOVE))
		printf("pmap_change_wiring(%p, %lx, %x)\n", pmap, va, wired);
#endif
	if (pmap == NULL) {
		pv_check();
		return;
	}

	/*
	 * Is this part of the permanent 4MB mapping?
	 */
	if( pmap == pmap_kernel() && va >= ksegv && va < ksegv+4*MEG ) {
		prom_printf("pmap_changeprot: va=%08x in locked TLB\r\n", va);
		OF_enter();
		return;
	}
	s = splimp();
	data = pseg_get(pmap, va&PV_VAMASK);

	if (wired) 
		data |= TLB_TSB_LOCK;
	else
		data &= ~TLB_TSB_LOCK;

	if (pseg_set(pmap, va&PV_VAMASK, data, 0)) {
		printf("pmap_change_wiring: gotten pseg empty!\n");
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

#if defined(PMAP_NEW)
void
pmap_page_protect(pg, prot)
	struct vm_page* pg;
	vm_prot_t prot;
#else
void
pmap_page_protect(pa, prot)
	paddr_t pa;
	vm_prot_t prot;
#endif
{
#if defined(PMAP_NEW)
	paddr_t pa = PMAP_PGARG(pg);
#endif
	register pv_entry_t pv;
	register int i, s;
	long long clear, set;
	int64_t data = 0LL;

#ifdef DEBUG
	if (pmapdebug & PDB_CHANGEPROT)
		printf("pmap_page_protect: pa %p prot %x\n", pa, prot);
#endif

	if (!IS_VM_PHYSADDR(pa)) {
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
		if(VM_PROT_EXECUTE & prot)
			set |= TLB_EXEC;
		else
			clear |= TLB_EXEC;
		if(VM_PROT_EXECUTE == prot)
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
				i = ptelookup_va(pv->pv_va&PV_VAMASK);
				/* since we already know the va for each mapping we don't need to scan the entire TSB */
				if (tsb[i].tag.tag == TSB_TAG(0, pv->pv_pmap->pm_ctx, pv->pv_va&PV_VAMASK)) 
					tsb[i].data.data = 0LL;			
				tlb_flush_pte(pv->pv_va&PV_VAMASK, pv->pv_pmap->pm_ctx);
			}
		}
		splx(s);
	} else {
		pv_entry_t npv, firstpv;
		/* remove mappings */
		
		firstpv = pv = pa_to_pvh(pa);
		s = splimp();
		/* First remove the entire list of continuation pv's*/
		for (npv = pv->pv_next; npv; npv = pv->pv_next) {
			/* We're removing npv from pv->pv_next */
#ifdef DEBUG
			if (pmapdebug & (PDB_CHANGEPROT|PDB_REF)) {
				printf("pmap_page_protect: demap va %p of pa %p...\n",
				       npv->pv_va, pa);
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
#ifdef DEBUG
				printf("pmap_page_protect: Removing wired page pm %p va %p\n",
				       npv->pv_pmap, npv->pv_va);
#endif			
				/* Skip this pv, it's wired */
				pv = npv;
				continue;
			}
			/* Clear mapping */
			if (pseg_set(npv->pv_pmap, npv->pv_va&PV_VAMASK, 0, 0)) {
				printf("pmap_page_protect: gotten pseg empty!\n");
				Debugger();
				/* panic? */
			}
			/* clear the entry in the TSB */
			i = ptelookup_va(npv->pv_va&PV_VAMASK);
			/* since we already know the va for each mapping we don't need to scan the entire TSB */
			if (tsb[i].tag.tag == TSB_TAG(0, npv->pv_pmap->pm_ctx, npv->pv_va&PV_VAMASK)) 
				tsb[i].data.data = 0LL;			
			tlb_flush_pte(npv->pv_va&PV_VAMASK, npv->pv_pmap->pm_ctx);
			
			/* free the pv */
			pv->pv_next = npv->pv_next;
			free((caddr_t)npv, M_VMPVENT);
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
			if (pmapdebug & (PDB_CHANGEPROT|PDB_REF)) {
				printf("pmap_page_protect: demap va %p of pa %p...\n",
				       pv->pv_va, pa);
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
#ifdef DEBUG
				printf("pmap_page_protect: Removing wired page pm %p va %p\n",
				       pv->pv_pmap, pv->pv_va);
#endif			
				/* It's wired, leave it */
				goto skipit;
			}
			if (pseg_set(pv->pv_pmap, pv->pv_va&PV_VAMASK, 0, 0)) {
				printf("pmap_page_protect: gotten pseg empty!\n");
				Debugger();
				/* panic? */
			}
			i = ptelookup_va(pv->pv_va&PV_VAMASK);
			/* since we already know the va for each mapping we don't need to scan the entire TSB */
			if (tsb[i].tag.tag == TSB_TAG(0, pv->pv_pmap->pm_ctx, pv->pv_va&PV_VAMASK)) 
				tsb[i].data.data = 0LL;			
			tlb_flush_pte(pv->pv_va&PV_VAMASK, pv->pv_pmap->pm_ctx);
			npv = pv->pv_next;
			/* dump the first pv */
			if (npv) {
				/* First save mod/ref bits */
				npv->pv_va |= (pv->pv_va&PV_MASK);
				*pv = *npv;
				free((caddr_t)npv, M_VMPVENT);
			} else {
				pv->pv_pmap = NULL;
				pv->pv_next = NULL;
			}
		skipit:
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
		if((pdir = (paddr_t *)ldxa(&pm->pm_segs[i], ASI_PHYS_CACHED))) {
			for (k=0; k<PDSZ; k++) {
				if ((ptbl = (paddr_t *)ldxa(&pdir[k], ASI_PHYS_CACHED))) {
					for (j=0; j<PTSZ; j++) {
						int64_t data = ldxa(&ptbl[j], ASI_PHYS_CACHED);
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

	s = splpmap();
	cnum = next;
	do {
		if (cnum >= numctx-1) 
			cnum = 0;
	} while (ctxbusy[++cnum] != NULL && cnum != next);
	if (cnum==0) cnum++; /* Never steal ctx 0 */
	if (ctxbusy[cnum]) {
#ifdef DEBUG
		/* We should identify this pmap and clear it */
		printf("Warning: stealing context %d\n", cnum);
		remove_stats.pidflushes ++;
#endif
		/* We gotta steal this context */
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
		printf("pmap_remove_pv(pm=%x, va=%x, pa=%x)\n", pmap, va, pa);
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
			free((caddr_t)npv, M_VMPVENT);
		} else {
			pv->pv_pmap = NULL;
			pv->pv_next = NULL;
			pv->pv_va &= PV_MASK; /* Only save ref/mod bits */
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
#if defined(PMAP_NEW)

		/* 
		 * Sometimes UVM gets confused and calls pmap_remove() instead
		 * of pmap_kremove() 
		 */
		return; 
#endif
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
		free((caddr_t)npv, M_VMPVENT);
	}

	/* Save ref/mod info */
	if (data & TLB_ACCESS) 
		opv->pv_va |= PV_REF;
	if (data & (TLB_W|TLB_MODIFY))
		opv->pv_va |= PV_MOD;
	splx(s);
	pv_check();
}

#if defined(UVM)
/*
 *	vm_page_alloc1:
 *
 *	Allocate and return a memory cell with no associated object.
 */
vm_page_t
vm_page_alloc1()
{
#if 1
	return uvm_pagealloc(NULL,0,NULL);
#else
	register vm_page_t	mem;
	int		spl;

	spl = splimp();				/* XXX */
	uvm_lock_fpageq();            /* lock free page queue */
	if (uvm.page_free.tqh_first == NULL) {
		uvm_unlock_fpageq();
		splx(spl);
		printf("vm_page_alloc1: free list empty\n");
		return (NULL);
	}

	mem = uvm.page_free.tqh_first;
	TAILQ_REMOVE(&uvm.page_free, mem, pageq);

	uvmexp.free--;
	uvm_unlock_fpageq();
	splx(spl);

	mem->flags =  PG_BUSY | PG_CLEAN | PG_FAKE;
	mem->pqflags = 0;
	mem->uobject = NULL;
	mem->uanon = NULL;
	mem->wire_count = 0;
	mem->loan_count = 0;

	/*
	 *	Decide if we should poke the pageout daemon.
	 *	We do this if the free count is less than the low
	 *	water mark, or if the free count is less than the high
	 *	water mark (but above the low water mark) and the inactive
	 *	count is less than its target.
	 *
	 *	We don't have the counts locked ... if they change a little,
	 *	it doesn't really matter.
	 */

	if (uvmexp.free < uvmexp.freemin ||
	    (uvmexp.free < uvmexp.freetarg &&
	    uvmexp.inactive < uvmexp.inactarg)) 
		thread_wakeup(&uvm.pagedaemon);

#ifdef DEBUG
	pmap_pages_stolen ++;
#endif
	return (mem);
#endif
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
#if 1
	if (mem->flags != (PG_BUSY|PG_CLEAN|PG_FAKE)) {
		printf("Freeing invalid page %p\n", mem);
		Debugger();
		return;
	}
	uvm_pagefree(mem);
#else
#ifdef DIAGNOSTIC
	if (mem->pqflags & (PQ_ACTIVE|PQ_INACTIVE|PG_FAKE))
		panic("vm_page_free1: active/inactive page!");
#endif
	
	if (!(mem->flags & PG_FAKE)) {
		int	spl;

		spl = splimp();
		uvm_lock_fpageq();
		mem->pqflags = PQ_FREE;
		TAILQ_INSERT_TAIL(&uvm.page_free, mem, pageq);
		TAILQ_INSERT_TAIL(&uvm.page_free, mem, pageq);
		uvmexp.free++;
		splx(spl);
	}
#ifdef DEBUG
	pmap_pages_stolen --;
#endif
#endif
}
#else
/*
 *	vm_page_alloc1:
 *
 *	Allocate and return a memory cell with no associated object.
 */
vm_page_t
vm_page_alloc1()
{
	register vm_page_t	mem;
	int		spl;

	spl = splimp();				/* XXX */
	simple_lock(&vm_page_queue_free_lock);
	if (vm_page_queue_free.tqh_first == NULL) {
		printf("vm_page_alloc1: free list empty\n");
		simple_unlock(&vm_page_queue_free_lock);
		splx(spl);
		return (NULL);
	}

	mem = vm_page_queue_free.tqh_first;
	TAILQ_REMOVE(&vm_page_queue_free, mem, pageq);

	cnt.v_free_count--;
	simple_unlock(&vm_page_queue_free_lock);
	splx(spl);

	mem->flags = PG_BUSY | PG_CLEAN | PG_FAKE;
	mem->wire_count = 0;

	/*
	 *	Decide if we should poke the pageout daemon.
	 *	We do this if the free count is less than the low
	 *	water mark, or if the free count is less than the high
	 *	water mark (but above the low water mark) and the inactive
	 *	count is less than its target.
	 *
	 *	We don't have the counts locked ... if they change a little,
	 *	it doesn't really matter.
	 */

	if (cnt.v_free_count < cnt.v_free_min ||
	    (cnt.v_free_count < cnt.v_free_target &&
	     cnt.v_inactive_count < cnt.v_inactive_target))
		thread_wakeup((void *)&vm_pages_needed);
#ifdef DEBUG
	pmap_pages_stolen ++;
#endif
	return (mem);
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

	if (mem->flags & PG_ACTIVE) {
		TAILQ_REMOVE(&vm_page_queue_active, mem, pageq);
		mem->flags &= ~PG_ACTIVE;
		cnt.v_active_count--;
	}

	if (mem->flags & PG_INACTIVE) {
		TAILQ_REMOVE(&vm_page_queue_inactive, mem, pageq);
		mem->flags &= ~PG_INACTIVE;
		cnt.v_inactive_count--;
	}

	if (!(mem->flags & PG_FICTITIOUS)) {
		int	spl;

		spl = splimp();
		simple_lock(&vm_page_queue_free_lock);
		TAILQ_INSERT_TAIL(&vm_page_queue_free, mem, pageq);

		cnt.v_free_count++;
		simple_unlock(&vm_page_queue_free_lock);
		splx(spl);
	}
#ifdef DEBUG
	pmap_pages_stolen --;
#endif
}
#endif

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

