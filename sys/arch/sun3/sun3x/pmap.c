/*	$NetBSD: pmap.c,v 1.18 1997/03/26 23:27:12 gwr Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jeremy Cooper.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * XXX These comments aren't quite accurate.  Need to change.
 * The sun3x uses the MC68851 Memory Management Unit, which is built
 * into the CPU.  The 68851 maps virtual to physical addresses using
 * a multi-level table lookup, which is stored in the very memory that
 * it maps.  The number of levels of lookup is configurable from one
 * to four.  In this implementation, we use three, named 'A' through 'C'.
 *
 * The MMU translates virtual addresses into physical addresses by 
 * traversing these tables in a proccess called a 'table walk'.  The most 
 * significant 7 bits of the Virtual Address ('VA') being translated are 
 * used as an index into the level A table, whose base in physical memory 
 * is stored in a special MMU register, the 'CPU Root Pointer' or CRP.  The 
 * address found at that index in the A table is used as the base
 * address for the next table, the B table.  The next six bits of the VA are 
 * used as an index into the B table, which in turn gives the base address 
 * of the third and final C table.
 *
 * The next six bits of the VA are used as an index into the C table to
 * locate a Page Table Entry (PTE).  The PTE is a physical address in memory
 * to which the remaining 13 bits of the VA are added, producing the
 * mapped physical address.
 *
 * To map the entire memory space in this manner would require 2114296 bytes 
 * of page tables per process - quite expensive.  Instead we will 
 * allocate a fixed but considerably smaller space for the page tables at 
 * the time the VM system is initialized.  When the pmap code is asked by
 * the kernel to map a VA to a PA, it allocates tables as needed from this
 * pool.  When there are no more tables in the pool, tables are stolen
 * from the oldest mapped entries in the tree.  This is only possible 
 * because all memory mappings are stored in the kernel memory map
 * structures, independent of the pmap structures.  A VA which references
 * one of these invalidated maps will cause a page fault.  The kernel
 * will determine that the page fault was caused by a task using a valid 
 * VA, but for some reason (which does not concern it), that address was
 * not mapped.  It will ask the pmap code to re-map the entry and then
 * it will resume executing the faulting task.
 *
 * In this manner the most efficient use of the page table space is
 * achieved.  Tasks which do not execute often will have their tables 
 * stolen and reused by tasks which execute more frequently.  The best
 * size for the page table pool will probably be determined by 
 * experimentation.
 *
 * You read all of the comments so far.  Good for you.
 * Now go play!
 */

/*** A Note About the 68851 Address Translation Cache
 * The MC68851 has a 64 entry cache, called the Address Translation Cache
 * or 'ATC'.  This cache stores the most recently used page descriptors
 * accessed by the MMU when it does translations.  Using a marker called a
 * 'task alias' the MMU can store the descriptors from 8 different table
 * spaces concurrently.  The task alias is associated with the base
 * address of the level A table of that address space.  When an address
 * space is currently active (the CRP currently points to its A table)
 * the only cached descriptors that will be obeyed are ones which have a
 * matching task alias of the current space associated with them.
 *
 * Since the cache is always consulted before any table lookups are done,
 * it is important that it accurately reflect the state of the MMU tables.
 * Whenever a change has been made to a table that has been loaded into
 * the MMU, the code must be sure to flush any cached entries that are
 * affected by the change.  These instances are documented in the code at
 * various points.
 */
/*** A Note About the Note About the 68851 Address Translation Cache
 * 4 months into this code I discovered that the sun3x does not have
 * a MC68851 chip. Instead, it has a version of this MMU that is part of the
 * the 68030 CPU.
 * All though it behaves very similarly to the 68851, it only has 1 task
 * alias and a 22 entry cache.  So sadly (or happily), the first paragraph
 * of the previous note does not apply to the sun3x pmap.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/user.h>
#include <sys/queue.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>

#include <machine/cpu.h>
#include <machine/kcore.h>
#include <machine/pmap.h>
#include <machine/pte.h>
#include <machine/machdep.h>
#include <machine/mon.h>

#include "pmap_pvt.h"

/* XXX - What headers declare these? */
extern struct pcb *curpcb;
extern int physmem;

extern void copypage __P((const void*, void*));
extern void zeropage __P((void*));

/* Defined in locore.s */
extern char kernel_text[];

/* Defined by the linker */
extern char etext[], edata[], end[];
extern char *esym;	/* DDB */

/*************************** DEBUGGING DEFINITIONS ***********************
 * Macros, preprocessor defines and variables used in debugging can make *
 * code hard to read.  Anything used exclusively for debugging purposes  *
 * is defined here to avoid having such mess scattered around the file.  *
 *************************************************************************/
#ifdef	PMAP_DEBUG
/*
 * To aid the debugging process, macros should be expanded into smaller steps
 * that accomplish the same goal, yet provide convenient places for placing
 * breakpoints.  When this code is compiled with PMAP_DEBUG mode defined, the
 * 'INLINE' keyword is defined to an empty string.  This way, any function
 * defined to be a 'static INLINE' will become 'outlined' and compiled as
 * a separate function, which is much easier to debug.
 */
#define	INLINE	/* nothing */

/*
 * It is sometimes convenient to watch the activity of a particular table
 * in the system.  The following variables are used for that purpose.
 */
a_tmgr_t *pmap_watch_atbl = 0;
b_tmgr_t *pmap_watch_btbl = 0;
c_tmgr_t *pmap_watch_ctbl = 0;

int pmap_debug = 0;
#define DPRINT(args) if (pmap_debug) printf args

#else	/********** Stuff below is defined if NOT debugging **************/

#define	INLINE	inline
#define DPRINT(args)  /* nada */

#endif	/* PMAP_DEBUG */
/*********************** END OF DEBUGGING DEFINITIONS ********************/

/*** Management Structure - Memory Layout
 * For every MMU table in the sun3x pmap system there must be a way to
 * manage it; we must know which process is using it, what other tables
 * depend on it, and whether or not it contains any locked pages.  This
 * is solved by the creation of 'table management'  or 'tmgr'
 * structures.  One for each MMU table in the system.
 *
 *                        MAP OF MEMORY USED BY THE PMAP SYSTEM
 *
 *      towards lower memory
 * kernAbase -> +-------------------------------------------------------+
 *              | Kernel     MMU A level table                          |
 * kernBbase -> +-------------------------------------------------------+
 *              | Kernel     MMU B level tables                         |
 * kernCbase -> +-------------------------------------------------------+
 *              |                                                       |
 *              | Kernel     MMU C level tables                         |
 *              |                                                       |
 * mmuCbase  -> +-------------------------------------------------------+
 *              | User       MMU C level tables                         |
 * mmuAbase  -> +-------------------------------------------------------+
 *              |                                                       |
 *              | User       MMU A level tables                         |
 *              |                                                       |
 * mmuBbase  -> +-------------------------------------------------------+
 *              | User       MMU B level tables                         |
 * tmgrAbase -> +-------------------------------------------------------+
 *              |  TMGR A level table structures                        |
 * tmgrBbase -> +-------------------------------------------------------+
 *              |  TMGR B level table structures                        |
 * tmgrCbase -> +-------------------------------------------------------+
 *              |  TMGR C level table structures                        |
 * pvbase    -> +-------------------------------------------------------+
 *              |  Physical to Virtual mapping table (list heads)       |
 * pvebase   -> +-------------------------------------------------------+
 *              |  Physical to Virtual mapping table (list elements)    |
 *              |                                                       |
 *              +-------------------------------------------------------+
 *      towards higher memory
 *
 * For every A table in the MMU A area, there will be a corresponding
 * a_tmgr structure in the TMGR A area.  The same will be true for
 * the B and C tables.  This arrangement will make it easy to find the
 * controling tmgr structure for any table in the system by use of
 * (relatively) simple macros.
 */

/*
 * Global variables for storing the base addresses for the areas
 * labeled above.
 */
static vm_offset_t  	kernAphys;
static mmu_long_dte_t	*kernAbase;
static mmu_short_dte_t	*kernBbase;
static mmu_short_pte_t	*kernCbase;
static mmu_short_pte_t	*mmuCbase;
static mmu_short_dte_t	*mmuBbase;
static mmu_long_dte_t	*mmuAbase;
static a_tmgr_t		*Atmgrbase;
static b_tmgr_t		*Btmgrbase;
static c_tmgr_t		*Ctmgrbase;
static pv_t 		*pvbase;
static pv_elem_t	*pvebase;
struct pmap 		kernel_pmap;

/*
 * This holds the CRP currently loaded into the MMU.
 */
struct mmu_rootptr kernel_crp;

/*
 * Just all around global variables.
 */
static TAILQ_HEAD(a_pool_head_struct, a_tmgr_struct) a_pool;
static TAILQ_HEAD(b_pool_head_struct, b_tmgr_struct) b_pool;
static TAILQ_HEAD(c_pool_head_struct, c_tmgr_struct) c_pool;


/*
 * Flags used to mark the safety/availability of certain operations or
 * resources.
 */
static boolean_t
    pv_initialized = FALSE,          /* PV system has been initialized. */
    tmp_vpages_inuse = FALSE,        /*
                                      * Temp. virtual pages are in use.
                                      * (see pmap_copy_page, et. al.)
                                      */
    bootstrap_alloc_enabled = FALSE; /* Safe to use pmap_bootstrap_alloc(). */

/*
 * XXX:  For now, retain the traditional variables that were
 * used in the old pmap/vm interface (without NONCONTIG).
 */
/* Kernel virtual address space available: */
vm_offset_t	virtual_avail, virtual_end;
/* Physical address space available: */
vm_offset_t	avail_start, avail_end;

/* This keep track of the end of the contiguously mapped range. */
vm_offset_t virtual_contig_end;

/* Physical address used by pmap_next_page() */
vm_offset_t avail_next;

/* These are used by pmap_copy_page(), etc. */
vm_offset_t tmp_vpages[2];

/*
 * The 3/80 is the only member of the sun3x family that has non-contiguous
 * physical memory.  Memory is divided into 4 banks which are physically
 * locatable on the system board.  Although the size of these banks varies
 * with the size of memory they contain, their base addresses are
 * permenently fixed.  The following structure, which describes these
 * banks, is initialized by pmap_bootstrap() after it reads from a similar
 * structure provided by the ROM Monitor.
 *
 * For the other machines in the sun3x architecture which do have contiguous
 * RAM, this list will have only one entry, which will describe the entire
 * range of available memory.
 */
struct pmap_physmem_struct avail_mem[NPHYS_RAM_SEGS];
u_int total_phys_mem;

/*************************************************************************/

/*
 * XXX - Should "tune" these based on statistics.
 *
 * My first guess about the relative numbers of these needed is
 * based on the fact that a "typical" process will have several
 * pages mapped at low virtual addresses (text, data, bss), then
 * some mapped shared libraries, and then some stack pages mapped
 * near the high end of the VA space.  Each process can use only
 * one A table, and most will use only two B tables (maybe three)
 * and probably about four C tables.  Therefore, the first guess
 * at the relative numbers of these needed is 1:2:4 -gwr
 *
 * The number of C tables needed is closely related to the amount
 * of physical memory available plus a certain amount attributable
 * to the use of double mappings.  With a few simulation statistics
 * we can find a reasonably good estimation of this unknown value.
 * Armed with that and the above ratios, we have a good idea of what
 * is needed at each level. -j
 *
 * Note: It is not physical memory memory size, but the total mapped
 * virtual space required by the combined working sets of all the
 * currently _runnable_ processes.  (Sleeping ones don't count.)
 * The amount of physical memory should be irrelevant. -gwr
 */
#define NUM_A_TABLES	16
#define NUM_B_TABLES	32
#define NUM_C_TABLES	64

/*
 * This determines our total virtual mapping capacity.
 * Yes, it is a FIXED value so we can pre-allocate.
 */
#define NUM_USER_PTES	(NUM_C_TABLES * MMU_C_TBL_SIZE)

/*
 * The size of the Kernel Virtual Address Space (KVAS)
 * for purposes of MMU table allocation is -KERNBASE
 * (length from KERNBASE to 0xFFFFffff)
 */
#define	KVAS_SIZE		(-KERNBASE)

/* Numbers of kernel MMU tables to support KVAS_SIZE. */
#define KERN_B_TABLES	(KVAS_SIZE >> MMU_TIA_SHIFT)
#define KERN_C_TABLES	(KVAS_SIZE >> MMU_TIB_SHIFT)
#define	NUM_KERN_PTES	(KVAS_SIZE >> MMU_TIC_SHIFT)

/*************************** MISCELANEOUS MACROS *************************/
#define PMAP_LOCK()	;	/* Nothing, for now */
#define PMAP_UNLOCK()	;	/* same. */
#define	NULL 0

static INLINE void *      mmu_ptov __P((vm_offset_t pa));
static INLINE vm_offset_t mmu_vtop __P((void * va));

#if	0
static INLINE a_tmgr_t * mmuA2tmgr __P((mmu_long_dte_t *));
#endif
static INLINE b_tmgr_t * mmuB2tmgr __P((mmu_short_dte_t *));
static INLINE c_tmgr_t * mmuC2tmgr __P((mmu_short_pte_t *));

static INLINE pv_t *pa2pv __P((vm_offset_t pa));
static INLINE int   pteidx __P((mmu_short_pte_t *));
static INLINE pmap_t current_pmap __P((void));

/*
 * We can always convert between virtual and physical addresses
 * for anything in the range [KERNBASE ... avail_start] because
 * that range is GUARANTEED to be mapped linearly.
 * We rely heavily upon this feature!
 */
static INLINE void *
mmu_ptov(pa)
	vm_offset_t pa;
{
	register vm_offset_t va;

	va = (pa + KERNBASE);
#ifdef	PMAP_DEBUG
	if ((va < KERNBASE) || (va >= virtual_contig_end))
		panic("mmu_ptov");
#endif
	return ((void*)va);
}
static INLINE vm_offset_t
mmu_vtop(vva)
	void *vva;
{
	register vm_offset_t va;

	va = (vm_offset_t)vva;
#ifdef	PMAP_DEBUG
	if ((va < KERNBASE) || (va >= virtual_contig_end))
		panic("mmu_ptov");
#endif
	return (va - KERNBASE);
}

/*
 * These macros map MMU tables to their corresponding manager structures.
 * They are needed quite often because many of the pointers in the pmap
 * system reference MMU tables and not the structures that control them.
 * There needs to be a way to find one when given the other and these
 * macros do so by taking advantage of the memory layout described above.
 * Here's a quick step through the first macro, mmuA2tmgr():
 *
 * 1) find the offset of the given MMU A table from the base of its table
 *    pool (table - mmuAbase).
 * 2) convert this offset into a table index by dividing it by the
 *    size of one MMU 'A' table. (sizeof(mmu_long_dte_t) * MMU_A_TBL_SIZE)
 * 3) use this index to select the corresponding 'A' table manager
 *    structure from the 'A' table manager pool (Atmgrbase[index]).
 */
/*  This function is not currently used. */
#if	0
static INLINE a_tmgr_t *
mmuA2tmgr(mmuAtbl)
	mmu_long_dte_t *mmuAtbl;
{
	register int idx;

	/* Which table is this in? */
	idx = (mmuAtbl - mmuAbase) / MMU_A_TBL_SIZE;
#ifdef	PMAP_DEBUG
	if ((idx < 0) || (idx >= NUM_A_TABLES))
		panic("mmuA2tmgr");
#endif
	return (&Atmgrbase[idx]);
}
#endif	/* 0 */

static INLINE b_tmgr_t *
mmuB2tmgr(mmuBtbl)
	mmu_short_dte_t *mmuBtbl;
{
	register int idx;

	/* Which table is this in? */
	idx = (mmuBtbl - mmuBbase) / MMU_B_TBL_SIZE;
#ifdef	PMAP_DEBUG
	if ((idx < 0) || (idx >= NUM_B_TABLES))
		panic("mmuB2tmgr");
#endif
	return (&Btmgrbase[idx]);
}

/* mmuC2tmgr			INTERNAL
 **
 * Given a pte known to belong to a C table, return the address of
 * that table's management structure.
 */
static INLINE c_tmgr_t *
mmuC2tmgr(mmuCtbl)
	mmu_short_pte_t *mmuCtbl;
{
	register int idx;

	/* Which table is this in? */
	idx = (mmuCtbl - mmuCbase) / MMU_C_TBL_SIZE;
#ifdef	PMAP_DEBUG
	if ((idx < 0) || (idx >= NUM_C_TABLES))
		panic("mmuC2tmgr");
#endif
	return (&Ctmgrbase[idx]);
}

/* This is now a function call below.
 * #define pa2pv(pa) \
 *	(&pvbase[(unsigned long)\
 *		_btop(pa)\
 *	])
 */

/* pa2pv			INTERNAL
 **
 * Return the pv_list_head element which manages the given physical
 * address.
 */
static INLINE pv_t *
pa2pv(pa)
	vm_offset_t pa;
{
	register struct pmap_physmem_struct *bank;
	register int idx;

	bank = &avail_mem[0];
	while (pa >= bank->pmem_end)
		bank = bank->pmem_next;

	pa -= bank->pmem_start;
	idx = bank->pmem_pvbase + _btop(pa);
#ifdef	PMAP_DEBUG
	if ((idx < 0) || (idx >= physmem))
		panic("pa2pv");
#endif
	return &pvbase[idx];
}

/* pteidx			INTERNAL
 **
 * Return the index of the given PTE within the entire fixed table of
 * PTEs.
 */
static INLINE int
pteidx(pte)
	mmu_short_pte_t *pte;
{
	return (pte - kernCbase);
}

/*
 * This just offers a place to put some debugging checks,
 * and reduces the number of places "curproc" appears...
 */
static INLINE pmap_t
current_pmap()
{
	struct proc *p;
	struct vmspace *vm;
	vm_map_t	map;
	pmap_t	pmap;

	p = curproc;	/* XXX */
	if (p == NULL)
		pmap = &kernel_pmap;
	else {
		vm = p->p_vmspace;
		map = &vm->vm_map;
		pmap = vm_map_pmap(map);
	}

	return (pmap);
}


/*************************** FUNCTION DEFINITIONS ************************
 * These appear here merely for the compiler to enforce type checking on *
 * all function calls.                                                   *
 *************************************************************************/

/** External functions
 ** - functions used within this module but written elsewhere.
 **   both of these functions are in locore.s
 ** XXX - These functions were later replaced with their more cryptic
 **       hp300 counterparts.  They may be removed now.
 **/
#if	0	/* deprecated mmu */
void   mmu_seturp __P((vm_offset_t));
void   mmu_flush __P((int, vm_offset_t));
void   mmu_flusha __P((void));
#endif	/* 0 */

/** Internal functions
 ** - all functions used only within this module are defined in
 **   pmap_pvt.h
 **/

/** Interface functions
 ** - functions required by the Mach VM Pmap interface, with MACHINE_CONTIG
 **   defined.
 **/
#ifdef INCLUDED_IN_PMAP_H
void   pmap_bootstrap __P((void));
void  *pmap_bootstrap_alloc __P((int));
void   pmap_enter __P((pmap_t, vm_offset_t, vm_offset_t, vm_prot_t, boolean_t));
pmap_t pmap_create __P((vm_size_t));
void   pmap_destroy __P((pmap_t));
void   pmap_reference __P((pmap_t));
boolean_t   pmap_is_referenced __P((vm_offset_t));
boolean_t   pmap_is_modified __P((vm_offset_t));
void   pmap_clear_modify __P((vm_offset_t));
vm_offset_t pmap_extract __P((pmap_t, vm_offset_t));
void   pmap_activate __P((pmap_t));
int    pmap_page_index __P((vm_offset_t));
u_int  pmap_free_pages __P((void));
#endif /* INCLUDED_IN_PMAP_H */

/********************************** CODE ********************************
 * Functions that are called from other parts of the kernel are labeled *
 * as 'INTERFACE' functions.  Functions that are only called from       *
 * within the pmap module are labeled as 'INTERNAL' functions.          *
 * Functions that are internal, but are not (currently) used at all are *
 * labeled 'INTERNAL_X'.                                                *
 ************************************************************************/ 

/* pmap_bootstrap			INTERNAL
 **
 * Initializes the pmap system.  Called at boot time from _vm_init()
 * in _startup.c.
 *
 * Reminder: having a pmap_bootstrap_alloc() and also having the VM
 *           system implement pmap_steal_memory() is redundant.
 *           Don't release this code without removing one or the other!
 */
void
pmap_bootstrap(nextva)
	vm_offset_t nextva;
{
	struct physmemory *membank;
	struct pmap_physmem_struct *pmap_membank;
	vm_offset_t va, pa, eva;
	int b, c, i, j;	/* running table counts */
	int size;

	/*
	 * This function is called by __bootstrap after it has
	 * determined the type of machine and made the appropriate
	 * patches to the ROM vectors (XXX- I don't quite know what I meant
	 * by that.)  It allocates and sets up enough of the pmap system
	 * to manage the kernel's address space.
	 */

	/*
	 * Determine the range of kernel virtual and physical
	 * space available. Note that we ABSOLUTELY DEPEND on
	 * the fact that the first bank of memory (4MB) is
	 * mapped linearly to KERNBASE (which we guaranteed in
	 * the first instructions of locore.s).
	 * That is plenty for our bootstrap work.
	 */
	virtual_avail = _round_page(nextva);
	virtual_contig_end = KERNBASE + 0x400000; /* +4MB */
	virtual_end = VM_MAX_KERNEL_ADDRESS;
	/* Don't need avail_start til later. */

	/* We may now call pmap_bootstrap_alloc(). */
	bootstrap_alloc_enabled = TRUE;

	/*
	 * This is a somewhat unwrapped loop to deal with
	 * copying the PROM's 'phsymem' banks into the pmap's
	 * banks.  The following is always assumed:
	 * 1. There is always at least one bank of memory.
	 * 2. There is always a last bank of memory, and its
	 *    pmem_next member must be set to NULL.
	 * XXX - Use: do { ... } while (membank->next) instead?
	 * XXX - Why copy this stuff at all? -gwr
	 *     - It is needed in pa2pv().
	 */
	membank = romVectorPtr->v_physmemory;
	pmap_membank = avail_mem;
	total_phys_mem = 0;

	while (membank->next) {
		pmap_membank->pmem_start = membank->address;
		pmap_membank->pmem_end = membank->address + membank->size;
		total_phys_mem += membank->size;
		/* This silly syntax arises because pmap_membank
		 * is really a pre-allocated array, but it is put into
		 * use as a linked list.
		 */
		pmap_membank->pmem_next = pmap_membank + 1;
		pmap_membank = pmap_membank->pmem_next;
		membank = membank->next;
	}

	/*
	 * XXX The last bank of memory should be reduced to exclude the
	 * physical pages needed by the PROM monitor from being used
	 * in the VM system.  XXX - See below - Fix!
	 */
	pmap_membank->pmem_start = membank->address;
	pmap_membank->pmem_end = membank->address + membank->size;
	pmap_membank->pmem_next = NULL;

#if 0	/* XXX - Need to integrate this! */
	/*
	 * The last few pages of physical memory are "owned" by
	 * the PROM.  The total amount of memory we are allowed
	 * to use is given by the romvec pointer. -gwr
	 *
	 * We should dedicate different variables for 'useable'
	 * and 'physically available'.  Most users are used to the
	 * kernel reporting the amount of memory 'physically available'
	 * as opposed to 'useable by the kernel' at boot time. -j
	 */
	total_phys_mem = *romVectorPtr->memoryAvail;
#endif	/* XXX */

	total_phys_mem += membank->size;	/* XXX see above */
	physmem = btoc(total_phys_mem);

	/*
	 * Avail_end is set to the first byte of physical memory
	 * after the end of the last bank.  We use this only to
	 * determine if a physical address is "managed" memory.
	 *
	 * XXX - The setting of avail_end is a temporary ROM saving hack.
	 */
	avail_end = pmap_membank->pmem_end -
		(total_phys_mem - *romVectorPtr->memoryAvail);
	avail_end = _trunc_page(avail_end);

	/*
	 * First allocate enough kernel MMU tables to map all
	 * of kernel virtual space from KERNBASE to 0xFFFFFFFF.
	 * Note: All must be aligned on 256 byte boundaries.
	 * Start with the level-A table (one of those).
	 */
	size = sizeof(mmu_long_dte_t)  * MMU_A_TBL_SIZE;
	kernAbase = pmap_bootstrap_alloc(size);
	bzero(kernAbase, size);

	/* Now the level-B kernel tables... */
	size = sizeof(mmu_short_dte_t) * MMU_B_TBL_SIZE * KERN_B_TABLES;
	kernBbase = pmap_bootstrap_alloc(size);
	bzero(kernBbase, size);

	/* Now the level-C kernel tables... */
	size = sizeof(mmu_short_pte_t) * MMU_C_TBL_SIZE * KERN_C_TABLES;
	kernCbase = pmap_bootstrap_alloc(size);
	bzero(kernCbase, size);
	/*
	 * Note: In order for the PV system to work correctly, the kernel
	 * and user-level C tables must be allocated contiguously.
	 * Nothing should be allocated between here and the allocation of
	 * mmuCbase below.  XXX: Should do this as one allocation, and
	 * then compute a pointer for mmuCbase instead of this...
	 *
	 * Allocate user MMU tables. 
	 * These must be contiguous with the preceeding.
	 */
	size = sizeof(mmu_short_pte_t) * MMU_C_TBL_SIZE	* NUM_C_TABLES;
	mmuCbase = pmap_bootstrap_alloc(size);

	size = sizeof(mmu_short_dte_t) * MMU_B_TBL_SIZE	* NUM_B_TABLES;
	mmuBbase = pmap_bootstrap_alloc(size);

	size = sizeof(mmu_long_dte_t)  * MMU_A_TBL_SIZE * NUM_A_TABLES;
	mmuAbase = pmap_bootstrap_alloc(size);

	/*
	 * Fill in the never-changing part of the kernel tables.
	 * For simplicity, the kernel's mappings will be editable as a
	 * flat array of page table entries at kernCbase.  The
	 * higher level 'A' and 'B' tables must be initialized to point
	 * to this lower one. 
	 */
	b = c = 0;

	/*
	 * Invalidate all mappings below KERNBASE in the A table.
	 * This area has already been zeroed out, but it is good
	 * practice to explicitly show that we are interpreting
	 * it as a list of A table descriptors.
	 */
	for (i = 0; i < MMU_TIA(KERNBASE); i++) {
		kernAbase[i].addr.raw = 0;
	}

	/*
	 * Set up the kernel A and B tables so that they will reference the
	 * correct spots in the contiguous table of PTEs allocated for the
	 * kernel's virtual memory space.
	 */
	for (i = MMU_TIA(KERNBASE); i < MMU_A_TBL_SIZE; i++) {
		kernAbase[i].attr.raw =
			MMU_LONG_DTE_LU | MMU_LONG_DTE_SUPV | MMU_DT_SHORT;
		kernAbase[i].addr.raw = mmu_vtop(&kernBbase[b]);

		for (j=0; j < MMU_B_TBL_SIZE; j++) {
			kernBbase[b + j].attr.raw = mmu_vtop(&kernCbase[c])
				| MMU_DT_SHORT;
			c += MMU_C_TBL_SIZE;
		}
		b += MMU_B_TBL_SIZE;
	}

	/* XXX - Doing kernel_pmap a little further down. */

	pmap_alloc_usermmu();	/* Allocate user MMU tables.        */
	pmap_alloc_usertmgr();	/* Allocate user MMU table managers.*/
	pmap_alloc_pv();	/* Allocate physical->virtual map.  */

	/*
	 * We are now done with pmap_bootstrap_alloc().  Round up
	 * `virtual_avail' to the nearest page, and set the flag
	 * to prevent use of pmap_bootstrap_alloc() hereafter.
	 */
	pmap_bootstrap_aalign(NBPG);
	bootstrap_alloc_enabled = FALSE;

	/*
	 * Now that we are done with pmap_bootstrap_alloc(), we
	 * must save the virtual and physical addresses of the
	 * end of the linearly mapped range, which are stored in
	 * virtual_contig_end and avail_start, respectively.
	 * These variables will never change after this point.
	 */
	virtual_contig_end = virtual_avail;
	avail_start = virtual_avail - KERNBASE;

	/*
	 * `avail_next' is a running pointer used by pmap_next_page() to
	 * keep track of the next available physical page to be handed
	 * to the VM system during its initialization, in which it
	 * asks for physical pages, one at a time.
	 */
	avail_next = avail_start;

	/*
	 * Now allocate some virtual addresses, but not the physical pages
	 * behind them.  Note that virtual_avail is already page-aligned.
	 *
	 * tmp_vpages[] is an array of two virtual pages used for temporary
	 * kernel mappings in the pmap module to facilitate various physical
	 * address-oritented operations.
	 */
	tmp_vpages[0] = virtual_avail;
	virtual_avail += NBPG;
	tmp_vpages[1] = virtual_avail;
	virtual_avail += NBPG;

	/** Initialize the PV system **/
	pmap_init_pv();

	/*
	 * Fill in the kernel_pmap structure and kernel_crp.
	 */
	kernAphys = mmu_vtop(kernAbase);
	kernel_pmap.pm_a_tmgr = NULL;
	kernel_pmap.pm_a_phys = kernAphys;
	kernel_pmap.pm_refcount = 1; /* always in use */

	kernel_crp.rp_attr = MMU_LONG_DTE_LU | MMU_DT_LONG;
	kernel_crp.rp_addr = kernAphys;

	/*
	 * Now pmap_enter_kernel() may be used safely and will be
	 * the main interface used hereafter to modify the kernel's
	 * virtual address space.  Note that since we are still running
	 * under the PROM's address table, none of these table modifications
	 * actually take effect until pmap_takeover_mmu() is called.
	 *
	 * Note: Our tables do NOT have the PROM linear mappings!
	 * Only the mappings created here exist in our tables, so
	 * remember to map anything we expect to use.
	 */
	va = (vm_offset_t) KERNBASE;
	pa = 0;

	/*
	 * The first page of the kernel virtual address space is the msgbuf
	 * page.  The page attributes (data, non-cached) are set here, while
	 * the address is assigned to this global pointer in cpu_startup().
	 * XXX - Make it non-cached?
	 */
	pmap_enter_kernel(va, pa|PMAP_NC, VM_PROT_ALL);
	va += NBPG; pa += NBPG;

	/* Next page is used as the temporary stack. */
	pmap_enter_kernel(va, pa, VM_PROT_ALL);
	va += NBPG; pa += NBPG;

	/*
	 * Map all of the kernel's text segment as read-only and cacheable.
	 * (Cacheable is implied by default).  Unfortunately, the last bytes
	 * of kernel text and the first bytes of kernel data will often be
	 * sharing the same page.  Therefore, the last page of kernel text
	 * has to be mapped as read/write, to accomodate the data.
	 */
	eva = _trunc_page((vm_offset_t)etext);
	for (; va < eva; va += NBPG, pa += NBPG)
		pmap_enter_kernel(va, pa, VM_PROT_READ|VM_PROT_EXECUTE);

	/*
	 * Map all of the kernel's data as read/write and cacheable.
	 * This includes: data, BSS, symbols, and everything in the
	 * contiguous memory used by pmap_bootstrap_alloc()
	 */
	for (; pa < avail_start; va += NBPG, pa += NBPG)
		pmap_enter_kernel(va, pa, VM_PROT_READ|VM_PROT_WRITE);

	/*
	 * At this point we are almost ready to take over the MMU.  But first
	 * we must save the PROM's address space in our map, as we call its
	 * routines and make references to its data later in the kernel.
	 */
	pmap_bootstrap_copyprom();
	pmap_takeover_mmu();
	pmap_bootstrap_setprom();

	/* Notify the VM system of our page size. */
	PAGE_SIZE = NBPG;
	vm_set_page_size();
}


/* pmap_alloc_usermmu			INTERNAL
 **
 * Called from pmap_bootstrap() to allocate MMU tables that will
 * eventually be used for user mappings.
 */
void
pmap_alloc_usermmu()
{
	/* XXX: Moved into caller. */
}

/* pmap_alloc_pv			INTERNAL
 **
 * Called from pmap_bootstrap() to allocate the physical
 * to virtual mapping list.  Each physical page of memory
 * in the system has a corresponding element in this list.
 */
void
pmap_alloc_pv()
{
	int	i;
	unsigned int	total_mem;

	/*
	 * Allocate a pv_head structure for every page of physical
	 * memory that will be managed by the system.  Since memory on
	 * the 3/80 is non-contiguous, we cannot arrive at a total page
	 * count by subtraction of the lowest available address from the
	 * highest, but rather we have to step through each memory
	 * bank and add the number of pages in each to the total.
	 *
	 * At this time we also initialize the offset of each bank's
	 * starting pv_head within the pv_head list so that the physical
	 * memory state routines (pmap_is_referenced(),
	 * pmap_is_modified(), et al.) can quickly find coresponding
	 * pv_heads in spite of the non-contiguity.
	 */
	total_mem = 0;
	for (i = 0; i < NPHYS_RAM_SEGS; i++) {
		avail_mem[i].pmem_pvbase = _btop(total_mem);
		total_mem += avail_mem[i].pmem_end -
			avail_mem[i].pmem_start;
		if (avail_mem[i].pmem_next == NULL)
			break;
	}
#ifdef	PMAP_DEBUG
	if (total_mem != total_phys_mem)
		panic("pmap_alloc_pv did not arrive at correct page count");
#endif
	
	pvbase = (pv_t *) pmap_bootstrap_alloc(sizeof(pv_t) *
		_btop(total_phys_mem));
}

/* pmap_alloc_usertmgr			INTERNAL
 **
 * Called from pmap_bootstrap() to allocate the structures which
 * facilitate management of user MMU tables.  Each user MMU table
 * in the system has one such structure associated with it.
 */
void
pmap_alloc_usertmgr()
{
	/* Allocate user MMU table managers */
	/* It would be a lot simpler to just make these BSS, but */
	/* we may want to change their size at boot time... -j */
	Atmgrbase = (a_tmgr_t *) pmap_bootstrap_alloc(sizeof(a_tmgr_t)
		* NUM_A_TABLES);
	Btmgrbase = (b_tmgr_t *) pmap_bootstrap_alloc(sizeof(b_tmgr_t)
		* NUM_B_TABLES);
	Ctmgrbase = (c_tmgr_t *) pmap_bootstrap_alloc(sizeof(c_tmgr_t)
		* NUM_C_TABLES);

	/*
	 * Allocate PV list elements for the physical to virtual
	 * mapping system.
	 */
	pvebase = (pv_elem_t *) pmap_bootstrap_alloc(
		sizeof(pv_elem_t) * (NUM_USER_PTES + NUM_KERN_PTES));
}

/* pmap_bootstrap_copyprom()			INTERNAL
 **
 * Copy the PROM mappings into our own tables.  Note, we
 * can use physical addresses until __bootstrap returns.
 */
void
pmap_bootstrap_copyprom()
{
	MachMonRomVector *romp;
	int *mon_ctbl;
	mmu_short_pte_t *kpte;
	int i, len;

	romp = romVectorPtr;

	/*
	 * Copy the mappings in MON_KDB_START...MONEND
	 * Note: mon_ctbl[0] maps MON_KDB_START
	 */
	mon_ctbl = *romp->monptaddr;
	i = _btop(MON_KDB_START - KERNBASE);
	kpte = &kernCbase[i];
	len = _btop(MONEND - MON_KDB_START);

	for (i = 0; i < len; i++) {
		kpte[i].attr.raw = mon_ctbl[i];
	}

	/*
	 * Copy the mappings at MON_DVMA_BASE (to the end).
	 * Note, in here, mon_ctbl[0] maps MON_DVMA_BASE.
	 * XXX - This does not appear to be necessary, but
	 * I'm not sure yet if it is or not. -gwr
	 */
	mon_ctbl = *romp->shadowpteaddr;
	i = _btop(MON_DVMA_BASE - KERNBASE);
	kpte = &kernCbase[i];
	len = _btop(MON_DVMA_SIZE);

	for (i = 0; i < len; i++) {
		kpte[i].attr.raw = mon_ctbl[i];
	}
}
		
/* pmap_takeover_mmu			INTERNAL
 **
 * Called from pmap_bootstrap() after it has copied enough of the
 * PROM mappings into the kernel map so that we can use our own
 * MMU table.
 */
void
pmap_takeover_mmu()
{

	loadcrp(&kernel_crp);
}

/* pmap_bootstrap_setprom()			INTERNAL
 **
 * Set the PROM mappings so it can see kernel space.
 * Note that physical addresses are used here, which
 * we can get away with because this runs with the
 * low 1GB set for transparent translation.
 */
void
pmap_bootstrap_setprom()
{
	mmu_long_dte_t *mon_dte;
	extern struct mmu_rootptr mon_crp;
	int i;

	mon_dte = (mmu_long_dte_t *) mon_crp.rp_addr;
	for (i = MMU_TIA(KERNBASE); i < MMU_TIA(KERN_END); i++) {
		mon_dte[i].attr.raw = kernAbase[i].attr.raw;
		mon_dte[i].addr.raw = kernAbase[i].addr.raw;
	}
}


/* pmap_init			INTERFACE
 **
 * Called at the end of vm_init() to set up the pmap system to go
 * into full time operation.  All initialization of kernel_pmap
 * should be already done by now, so this should just do things
 * needed for user-level pmaps to work.
 */
void
pmap_init()
{
	/** Initialize the manager pools **/
	TAILQ_INIT(&a_pool);
	TAILQ_INIT(&b_pool);
	TAILQ_INIT(&c_pool);

	/**************************************************************
	 * Initialize all tmgr structures and MMU tables they manage. *
	 **************************************************************/
	/** Initialize A tables **/
	pmap_init_a_tables();
	/** Initialize B tables **/
	pmap_init_b_tables();
	/** Initialize C tables **/
	pmap_init_c_tables();
}

/* pmap_init_a_tables()			INTERNAL
 **
 * Initializes all A managers, their MMU A tables, and inserts
 * them into the A manager pool for use by the system.
 */
void
pmap_init_a_tables()
{
	int i;
	a_tmgr_t *a_tbl;

	for (i=0; i < NUM_A_TABLES; i++) {
		/* Select the next available A manager from the pool */
		a_tbl = &Atmgrbase[i];

		/*
		 * Clear its parent entry.  Set its wired and valid
		 * entry count to zero.
		 */
		a_tbl->at_parent = NULL;
		a_tbl->at_wcnt = a_tbl->at_ecnt = 0;

		/* Assign it the next available MMU A table from the pool */
		a_tbl->at_dtbl = &mmuAbase[i * MMU_A_TBL_SIZE];

		/*
		 * Initialize the MMU A table with the table in the `proc0',
		 * or kernel, mapping.  This ensures that every process has
		 * the kernel mapped in the top part of its address space.
		 */
		bcopy(kernAbase, a_tbl->at_dtbl, MMU_A_TBL_SIZE * 
			sizeof(mmu_long_dte_t));

		/*
		 * Finally, insert the manager into the A pool,
		 * making it ready to be used by the system.
		 */
		TAILQ_INSERT_TAIL(&a_pool, a_tbl, at_link);
    }
}

/* pmap_init_b_tables()			INTERNAL
 **
 * Initializes all B table managers, their MMU B tables, and
 * inserts them into the B manager pool for use by the system.
 */
void
pmap_init_b_tables()
{
	int i,j;
	b_tmgr_t *b_tbl;

	for (i=0; i < NUM_B_TABLES; i++) {
		/* Select the next available B manager from the pool */
		b_tbl = &Btmgrbase[i];

		b_tbl->bt_parent = NULL;	/* clear its parent,  */
		b_tbl->bt_pidx = 0;		/* parent index,      */
		b_tbl->bt_wcnt = 0;		/* wired entry count, */
		b_tbl->bt_ecnt = 0;		/* valid entry count. */

		/* Assign it the next available MMU B table from the pool */
		b_tbl->bt_dtbl = &mmuBbase[i * MMU_B_TBL_SIZE];

		/* Invalidate every descriptor in the table */
		for (j=0; j < MMU_B_TBL_SIZE; j++)
			b_tbl->bt_dtbl[j].attr.raw = MMU_DT_INVALID;

		/* Insert the manager into the B pool */
		TAILQ_INSERT_TAIL(&b_pool, b_tbl, bt_link);
	}
}

/* pmap_init_c_tables()			INTERNAL
 **
 * Initializes all C table managers, their MMU C tables, and
 * inserts them into the C manager pool for use by the system.
 */
void
pmap_init_c_tables()
{
	int i,j;
	c_tmgr_t *c_tbl;

	for (i=0; i < NUM_C_TABLES; i++) {
		/* Select the next available C manager from the pool */
		c_tbl = &Ctmgrbase[i];

		c_tbl->ct_parent = NULL;	/* clear its parent,  */
		c_tbl->ct_pidx = 0;		/* parent index,      */
		c_tbl->ct_wcnt = 0;		/* wired entry count, */
		c_tbl->ct_ecnt = 0;		/* valid entry count. */

		/* Assign it the next available MMU C table from the pool */ 
		c_tbl->ct_dtbl = &mmuCbase[i * MMU_C_TBL_SIZE];

		for (j=0; j < MMU_C_TBL_SIZE; j++)
			c_tbl->ct_dtbl[j].attr.raw = MMU_DT_INVALID;

		TAILQ_INSERT_TAIL(&c_pool, c_tbl, ct_link);
	}
}

/* pmap_init_pv()			INTERNAL
 **
 * Initializes the Physical to Virtual mapping system.
 */
void
pmap_init_pv()
{
	int	i;

	/* Initialize every PV head. */
	for (i = 0; i < _btop(total_phys_mem); i++) {
		pvbase[i].pv_idx = PVE_EOL;	/* Indicate no mappings */
		pvbase[i].pv_flags = 0;		/* Zero out page flags  */
	}

	pv_initialized = TRUE;
}

/* get_a_table			INTERNAL
 **
 * Retrieve and return a level A table for use in a user map.
 */
a_tmgr_t *
get_a_table()
{
	a_tmgr_t *tbl;
	pmap_t pmap;

	/* Get the top A table in the pool */
	tbl = a_pool.tqh_first;
	if (tbl == NULL) {
		/*
		 * XXX - Instead of panicing here and in other get_x_table
		 * functions, we do have the option of sleeping on the head of
		 * the table pool.  Any function which updates the table pool
		 * would then issue a wakeup() on the head, thus waking up any
		 * processes waiting for a table.
		 *
		 * Actually, the place to sleep would be when some process
		 * asks for a "wired" mapping that would run us short of
		 * mapping resources.  This design DEPENDS on always having
		 * some mapping resources in the pool for stealing, so we
		 * must make sure we NEVER let the pool become empty. -gwr
		 */
		panic("get_a_table: out of A tables.");
	}

	TAILQ_REMOVE(&a_pool, tbl, at_link);
	/*
	 * If the table has a non-null parent pointer then it is in use.
	 * Forcibly abduct it from its parent and clear its entries.
	 * No re-entrancy worries here.  This table would not be in the
	 * table pool unless it was available for use.
	 *
	 * Note that the second argument to free_a_table() is FALSE.  This
	 * indicates that the table should not be relinked into the A table
	 * pool.  That is a job for the function that called us.
	 */
	if (tbl->at_parent) {
		pmap = tbl->at_parent;
		free_a_table(tbl, FALSE);
		pmap->pm_a_tmgr = NULL;
		pmap->pm_a_phys = kernAphys;
	}
#ifdef  NON_REENTRANT
	/*
	 * If the table isn't to be wired down, re-insert it at the
	 * end of the pool.
	 */
	if (!wired)
		/*
		 * Quandary - XXX
		 * Would it be better to let the calling function insert this
		 * table into the queue?  By inserting it here, we are allowing
		 * it to be stolen immediately.  The calling function is
		 * probably not expecting to use a table that it is not
		 * assured full control of.
		 * Answer - In the intrest of re-entrancy, it is best to let
		 * the calling function determine when a table is available
		 * for use.  Therefore this code block is not used.
		 */
		TAILQ_INSERT_TAIL(&a_pool, tbl, at_link);
#endif	/* NON_REENTRANT */
	return tbl;
}

/* get_b_table			INTERNAL
 **
 * Return a level B table for use.
 */
b_tmgr_t *
get_b_table()
{
	b_tmgr_t *tbl;

	/* See 'get_a_table' for comments. */
	tbl = b_pool.tqh_first;
	if (tbl == NULL)
		panic("get_b_table: out of B tables.");
	TAILQ_REMOVE(&b_pool, tbl, bt_link);
	if (tbl->bt_parent) {
		tbl->bt_parent->at_dtbl[tbl->bt_pidx].attr.raw = MMU_DT_INVALID;
		tbl->bt_parent->at_ecnt--;
		free_b_table(tbl, FALSE);
	}
#ifdef	NON_REENTRANT
	if (!wired)
		/* XXX see quandary in get_b_table */
		/* XXX start lock */
		TAILQ_INSERT_TAIL(&b_pool, tbl, bt_link);
		/* XXX end lock */
#endif	/* NON_REENTRANT */
	return tbl;
}

/* get_c_table			INTERNAL
 **
 * Return a level C table for use.
 */
c_tmgr_t *
get_c_table()
{
	c_tmgr_t *tbl;

	/* See 'get_a_table' for comments */
	tbl = c_pool.tqh_first;
	if (tbl == NULL)
		panic("get_c_table: out of C tables.");
	TAILQ_REMOVE(&c_pool, tbl, ct_link);
	if (tbl->ct_parent) {
		tbl->ct_parent->bt_dtbl[tbl->ct_pidx].attr.raw = MMU_DT_INVALID;
		tbl->ct_parent->bt_ecnt--;
		free_c_table(tbl, FALSE);
	}
#ifdef	NON_REENTRANT
	if (!wired)
		/* XXX See quandary in get_a_table */
		/* XXX start lock */
		TAILQ_INSERT_TAIL(&c_pool, tbl, c_link);
		/* XXX end lock */
#endif	/* NON_REENTRANT */

	return tbl;
}

/*
 * The following 'free_table' and 'steal_table' functions are called to
 * detach tables from their current obligations (parents and children) and
 * prepare them for reuse in another mapping.
 *
 * Free_table is used when the calling function will handle the fate
 * of the parent table, such as returning it to the free pool when it has
 * no valid entries.  Functions that do not want to handle this should
 * call steal_table, in which the parent table's descriptors and entry
 * count are automatically modified when this table is removed.
 */

/* free_a_table			INTERNAL
 **
 * Unmaps the given A table and all child tables from their current
 * mappings.  Returns the number of pages that were invalidated.
 * If 'relink' is true, the function will return the table to the head
 * of the available table pool.
 *
 * Cache note: The MC68851 will automatically flush all
 * descriptors derived from a given A table from its
 * Automatic Translation Cache (ATC) if we issue a
 * 'PFLUSHR' instruction with the base address of the
 * table.  This function should do, and does so.
 * Note note: We are using an MC68030 - there is no
 * PFLUSHR.
 */
int
free_a_table(a_tbl, relink)
	a_tmgr_t *a_tbl;
	boolean_t relink;
{
	int i, removed_cnt;
	mmu_long_dte_t	*dte;
	mmu_short_dte_t *dtbl;
	b_tmgr_t	*tmgr;

	/*
	 * Flush the ATC cache of all cached descriptors derived
	 * from this table.
	 * XXX - Sun3x does not use 68851's cached table feature
	 * flush_atc_crp(mmu_vtop(a_tbl->dte));
	 */

	/*
	 * Remove any pending cache flushes that were designated
	 * for the pmap this A table belongs to.
	 * a_tbl->parent->atc_flushq[0] = 0;
	 * XXX - Not implemented in sun3x.
	 */

	/*
	 * All A tables in the system should retain a map for the
	 * kernel. If the table contains any valid descriptors
	 * (other than those for the kernel area), invalidate them all,
	 * stopping short of the kernel's entries.
	 */
	removed_cnt = 0;
	if (a_tbl->at_ecnt) {
		dte = a_tbl->at_dtbl;
		for (i=0; i < MMU_TIA(KERNBASE); i++) {
			/*
			 * If a table entry points to a valid B table, free
			 * it and its children.
			 */
			if (MMU_VALID_DT(dte[i])) {
				/*
				 * The following block does several things,
				 * from innermost expression to the
				 * outermost:
				 * 1) It extracts the base (cc 1996)
				 *    address of the B table pointed
				 *    to in the A table entry dte[i].
				 * 2) It converts this base address into
				 *    the virtual address it can be
				 *    accessed with. (all MMU tables point
				 *    to physical addresses.)
				 * 3) It finds the corresponding manager
				 *    structure which manages this MMU table.
				 * 4) It frees the manager structure.
				 *    (This frees the MMU table and all
				 *    child tables. See 'free_b_table' for
				 *    details.)
				 */
				dtbl = mmu_ptov(dte[i].addr.raw);
				tmgr = mmuB2tmgr(dtbl);
				removed_cnt += free_b_table(tmgr, TRUE);
				dte[i].attr.raw = MMU_DT_INVALID;
			}
		}
		a_tbl->at_ecnt = 0;
	}
	if (relink) {
		a_tbl->at_parent = NULL;
		TAILQ_REMOVE(&a_pool, a_tbl, at_link);
		TAILQ_INSERT_HEAD(&a_pool, a_tbl, at_link);
	}
	return removed_cnt;
}

/* free_b_table			INTERNAL
 **
 * Unmaps the given B table and all its children from their current
 * mappings.  Returns the number of pages that were invalidated.
 * (For comments, see 'free_a_table()').
 */
int
free_b_table(b_tbl, relink)
	b_tmgr_t *b_tbl;
	boolean_t relink;
{
	int i, removed_cnt;
	mmu_short_dte_t *dte;
	mmu_short_pte_t	*dtbl;
	c_tmgr_t	*tmgr;

	removed_cnt = 0;
	if (b_tbl->bt_ecnt) {
		dte = b_tbl->bt_dtbl;
		for (i=0; i < MMU_B_TBL_SIZE; i++) {
			if (MMU_VALID_DT(dte[i])) {
				dtbl = mmu_ptov(MMU_DTE_PA(dte[i]));
				tmgr = mmuC2tmgr(dtbl);
				removed_cnt += free_c_table(tmgr, TRUE);
				dte[i].attr.raw = MMU_DT_INVALID;
			}
		}
		b_tbl->bt_ecnt = 0;
	}

	if (relink) {
		b_tbl->bt_parent = NULL;
		TAILQ_REMOVE(&b_pool, b_tbl, bt_link);
		TAILQ_INSERT_HEAD(&b_pool, b_tbl, bt_link);
	}
	return removed_cnt;
}

/* free_c_table			INTERNAL
 **
 * Unmaps the given C table from use and returns it to the pool for
 * re-use.  Returns the number of pages that were invalidated.
 *
 * This function preserves any physical page modification information 
 * contained in the page descriptors within the C table by calling
 * 'pmap_remove_pte().'
 */
int
free_c_table(c_tbl, relink)
	c_tmgr_t *c_tbl;
	boolean_t relink;
{
	int i, removed_cnt;

	removed_cnt = 0;
	if (c_tbl->ct_ecnt) {
		for (i=0; i < MMU_C_TBL_SIZE; i++) {
			if (MMU_VALID_DT(c_tbl->ct_dtbl[i])) {
				pmap_remove_pte(&c_tbl->ct_dtbl[i]);
				removed_cnt++;
			}
		}
		c_tbl->ct_ecnt = 0;
	}

	if (relink) {
		c_tbl->ct_parent = NULL;
		TAILQ_REMOVE(&c_pool, c_tbl, ct_link);
		TAILQ_INSERT_HEAD(&c_pool, c_tbl, ct_link);
	}
	return removed_cnt;
}

#if 0
/* free_c_table_novalid			INTERNAL
 **
 * Frees the given C table manager without checking to see whether
 * or not it contains any valid page descriptors as it is assumed
 * that it does not.
 */
void
free_c_table_novalid(c_tbl)
	c_tmgr_t *c_tbl;
{
	TAILQ_REMOVE(&c_pool, c_tbl, ct_link);
	TAILQ_INSERT_HEAD(&c_pool, c_tbl, ct_link);
	c_tbl->ct_parent->bt_dtbl[c_tbl->ct_pidx].attr.raw = MMU_DT_INVALID;
	c_tbl->ct_parent->bt_ecnt--;
	/*
	 * XXX - Should call equiv. of 'free_b_table_novalid' here if
	 * we just removed the last entry of the parent B table.
	 * But I want to insure that this will not endanger pmap_enter()
	 * with sudden removal of tables it is working with.
	 *
	 * We should probably add another field to each table, indicating
	 * whether or not it is 'locked', ie. in the process of being
	 * modified.
	 */
	c_tbl->ct_parent = NULL;
}
#endif

/* pmap_remove_pte			INTERNAL
 **
 * Unmap the given pte and preserve any page modification
 * information by transfering it to the pv head of the
 * physical page it maps to.  This function does not update
 * any reference counts because it is assumed that the calling
 * function will do so.
 */
void
pmap_remove_pte(pte)
	mmu_short_pte_t *pte;
{
	u_short     pv_idx, targ_idx;
	int         s;
	vm_offset_t pa;
	pv_t       *pv;

	pa = MMU_PTE_PA(*pte);
	if (is_managed(pa)) {
		pv = pa2pv(pa);
		targ_idx = pteidx(pte);	/* Index of PTE being removed    */

		/*
		 * If the PTE being removed is the first (or only) PTE in
		 * the list of PTEs currently mapped to this page, remove the
		 * PTE by changing the index found on the PV head.  Otherwise
		 * a linear search through the list will have to be executed 
		 * in order to find the PVE which points to the PTE being
		 * removed, so that it may be modified to point to its new
		 * neighbor.
		 */
		s = splimp();
		pv_idx = pv->pv_idx;	/* Index of first PTE in PV list */
		if (pv_idx == targ_idx) {
			pv->pv_idx = pvebase[targ_idx].pve_next;
		} else {
			/*
			 * Find the PV element which points to the target
			 * element.
			 */
			while (pvebase[pv_idx].pve_next != targ_idx) {
				pv_idx = pvebase[pv_idx].pve_next;
#ifdef	DIAGNOSTIC
				if (pv_idx == PVE_EOL)
					panic("pmap_remove_pte: pv list end!");
#endif
			}

			/*
			 * At this point, pv_idx is the index of the PV
			 * element just before the target element in the list.
			 * Unlink the target.
			 */
			pvebase[pv_idx].pve_next = pvebase[targ_idx].pve_next;
		}
		/*
		 * Save the mod/ref bits of the pte by simply
		 * ORing the entire pte onto the pv_flags member
		 * of the pv structure.
		 * There is no need to use a separate bit pattern
		 * for usage information on the pv head than that
		 * which is used on the MMU ptes.
		 */
		pv->pv_flags |= (u_short) pte->attr.raw;
		splx(s);
	}

	pte->attr.raw = MMU_DT_INVALID;
}

#if	0	/* XXX - I am eliminating this function. -j */
/* pmap_dereference_pte			INTERNAL
 **
 * Update the necessary reference counts in any tables and pmaps to
 * reflect the removal of the given pte.  Only called when no knowledge of
 * the pte's associated pmap is unknown.  This only occurs in the PV call
 * 'pmap_page_protect()' with a protection of VM_PROT_NONE, which means
 * that all references to a given physical page must be removed.
 */
void
pmap_dereference_pte(pte)
	mmu_short_pte_t *pte;
{
	vm_offset_t va;
	c_tmgr_t *c_tbl;
	pmap_t pmap;

	va = pmap_get_pteinfo(pte, &pmap, &c_tbl);
	/*
	 * Flush the translation cache of the page mapped by the PTE, should
	 * it prove to be in the current pmap.  Kernel mappings appear in
	 * all address spaces, so they always should be flushed 
	 */
	if (pmap == pmap_kernel() || pmap == current_pmap())
		TBIS(va);
	
	/*
	 * If the mapping belongs to a user map, update the necessary
	 * reference counts in the table manager.  XXX - It would be
	 * much easier to keep the resident count in the c_tmgr_t -gwr
	 */
	if (pmap != pmap_kernel()) {
		/*
		 * Most of the situations in which pmap_dereference_pte() is
		 * called are usually temporary removals of a mapping.  Often
		 * the mapping is reinserted shortly afterwards. If the parent
		 * C table's valid entry count reaches zero as a result of
		 * removing this mapping, we could return it to the free pool,
		 * but we leave it alone because it is likely to be used as
		 * stated above.
		 */
		c_tbl->ct_ecnt--;
		pmap->pm_stats.resident_count--;
	}
}
#endif	0	/* function elimination */

/* pmap_stroll			INTERNAL
 **
 * Retrieve the addresses of all table managers involved in the mapping of
 * the given virtual address.  If the table walk completed sucessfully,
 * return TRUE.  If it was only partially sucessful, return FALSE.
 * The table walk performed by this function is important to many other
 * functions in this module.
 *
 * Note: This function ought to be easier to read.
 */
boolean_t
pmap_stroll(pmap, va, a_tbl, b_tbl, c_tbl, pte, a_idx, b_idx, pte_idx)
	pmap_t pmap;
	vm_offset_t va;
	a_tmgr_t **a_tbl;
	b_tmgr_t **b_tbl;
	c_tmgr_t **c_tbl;
	mmu_short_pte_t **pte;
	int *a_idx, *b_idx, *pte_idx;
{
	mmu_long_dte_t *a_dte;   /* A: long descriptor table          */
	mmu_short_dte_t *b_dte;  /* B: short descriptor table         */

	if (pmap == pmap_kernel())
		return FALSE;

	/* Does the given pmap have its own A table? */
	*a_tbl = pmap->pm_a_tmgr;
	if (*a_tbl == NULL)
		return FALSE; /* No.  Return unknown. */
	/* Does the A table have a valid B table
	 * under the corresponding table entry?
	 */
	*a_idx = MMU_TIA(va);
	a_dte = &((*a_tbl)->at_dtbl[*a_idx]);
	if (!MMU_VALID_DT(*a_dte))
		return FALSE; /* No. Return unknown. */
	/* Yes. Extract B table from the A table. */
	*b_tbl = mmuB2tmgr(mmu_ptov(a_dte->addr.raw));
	/* Does the B table have a valid C table
	 * under the corresponding table entry?
	 */
	*b_idx = MMU_TIB(va);
	b_dte = &((*b_tbl)->bt_dtbl[*b_idx]);
	if (!MMU_VALID_DT(*b_dte))
		return FALSE; /* No. Return unknown. */
	/* Yes. Extract C table from the B table. */
	*c_tbl = mmuC2tmgr(mmu_ptov(MMU_DTE_PA(*b_dte)));
	*pte_idx = MMU_TIC(va);
	*pte = &((*c_tbl)->ct_dtbl[*pte_idx]);
	
	return	TRUE;
}
	
/* pmap_enter			INTERFACE
 **
 * Called by the kernel to map a virtual address
 * to a physical address in the given process map. 
 *
 * Note: this function should apply an exclusive lock
 * on the pmap system for its duration.  (it certainly
 * would save my hair!!)
 * This function ought to be easier to read.
 */
void
pmap_enter(pmap, va, pa, prot, wired)
	pmap_t	pmap;
	vm_offset_t va;
	vm_offset_t pa;
	vm_prot_t prot;
	boolean_t wired;
{
	boolean_t insert, managed; /* Marks the need for PV insertion.*/
	u_short nidx;            /* PV list index                     */
	int s;                   /* Used for splimp()/splx()          */
	int flags;               /* Mapping flags. eg. Cache inhibit  */
	u_int a_idx, b_idx, pte_idx; /* table indices                 */
	a_tmgr_t *a_tbl;         /* A: long descriptor table manager  */
	b_tmgr_t *b_tbl;         /* B: short descriptor table manager */
	c_tmgr_t *c_tbl;         /* C: short page table manager       */
	mmu_long_dte_t *a_dte;   /* A: long descriptor table          */
	mmu_short_dte_t *b_dte;  /* B: short descriptor table         */
	mmu_short_pte_t *c_pte;  /* C: short page descriptor table    */
	pv_t      *pv;           /* pv list head                      */
	enum {NONE, NEWA, NEWB, NEWC} llevel; /* used at end   */

	if (pmap == NULL)
		return;
	if (pmap == pmap_kernel()) {
		pmap_enter_kernel(va, pa, prot);
		return;
	}

	flags  = (pa & ~MMU_PAGE_MASK);
	pa    &= MMU_PAGE_MASK;

	/*
	 * Determine if the physical address being mapped is managed.
	 * If it isn't, the mapping should be cache inhibited.  (This is
	 * applied later in the function.)   XXX - Why non-cached? -gwr
	 */
	if ((managed = is_managed(pa)) == FALSE)
		flags |= PMAP_NC;

	/*
	 * For user mappings we walk along the MMU tables of the given
	 * pmap, reaching a PTE which describes the virtual page being
	 * mapped or changed.  If any level of the walk ends in an invalid
	 * entry, a table must be allocated and the entry must be updated
	 * to point to it.
	 * There is a bit of confusion as to whether this code must be
	 * re-entrant.  For now we will assume it is.  To support
	 * re-entrancy we must unlink tables from the table pool before
	 * we assume we may use them.  Tables are re-linked into the pool
	 * when we are finished with them at the end of the function.
	 * But I don't feel like doing that until we have proof that this
	 * needs to be re-entrant.
	 * 'llevel' records which tables need to be relinked.
	 */
	llevel = NONE;

	/*
	 * Step 1 - Retrieve the A table from the pmap.  If it has no
	 * A table, allocate a new one from the available pool.
	 */

	a_tbl = pmap->pm_a_tmgr;
	if (a_tbl == NULL) {
		/*
		 * This pmap does not currently have an A table.  Allocate
		 * a new one.
		 */
		a_tbl = get_a_table();
		a_tbl->at_parent = pmap;

		/*
		 * Assign this new A table to the pmap, and calculate its
		 * physical address so that loadcrp() can be used to make
		 * the table active.
		 */
		pmap->pm_a_tmgr = a_tbl;
		pmap->pm_a_phys = mmu_vtop(a_tbl->at_dtbl);

		/*
		 * If the process receiving a new A table is the current
		 * process, we are responsible for setting the MMU so that
		 * it becomes the current address space.  This only adds
		 * new mappings, so no need to flush anything.
		 */
		if (pmap == current_pmap()) {
			kernel_crp.rp_addr = pmap->pm_a_phys;
			loadcrp(&kernel_crp);
		}

		if (!wired)
			llevel = NEWA;
	} else {
		/*
		 * Use the A table already allocated for this pmap.
		 * Unlink it from the A table pool if necessary.
		 */
		if (wired && !a_tbl->at_wcnt)
			TAILQ_REMOVE(&a_pool, a_tbl, at_link);
	}

	/*
	 * Step 2 - Walk into the B table.  If there is no valid B table,
	 * allocate one.
	 */

	a_idx = MMU_TIA(va);            /* Calculate the TIA of the VA. */
	a_dte = &a_tbl->at_dtbl[a_idx]; /* Retrieve descriptor from table */
	if (MMU_VALID_DT(*a_dte)) {     /* Is the descriptor valid? */
		/* The descriptor is valid.  Use the B table it points to. */
		/*************************************
		 *               a_idx               *
		 *                 v                 *
		 * a_tbl -> +-+-+-+-+-+-+-+-+-+-+-+- *
		 *          | | | | | | | | | | | |  *
		 *          +-+-+-+-+-+-+-+-+-+-+-+- *
		 *                 |                 *
		 *                 \- b_tbl -> +-+-  *
		 *                             | |   *
		 *                             +-+-  *
		 *************************************/
		b_dte = mmu_ptov(a_dte->addr.raw);
		b_tbl = mmuB2tmgr(b_dte);

		/*
		 * If the requested mapping must be wired, but this table
		 * being used to map it is not, the table must be removed
		 * from the available pool and its wired entry count
		 * incremented.
		 */
		if (wired && !b_tbl->bt_wcnt) {
			TAILQ_REMOVE(&b_pool, b_tbl, bt_link);
			a_tbl->at_wcnt++;
		}
	} else {
		/* The descriptor is invalid.  Allocate a new B table. */
		b_tbl = get_b_table();

		/* Point the parent A table descriptor to this new B table. */
		a_dte->addr.raw = mmu_vtop(b_tbl->bt_dtbl);
		a_dte->attr.raw = MMU_LONG_DTE_LU | MMU_DT_SHORT;
		a_tbl->at_ecnt++; /* Update parent's valid entry count */

		/* Create the necessary back references to the parent table */
		b_tbl->bt_parent = a_tbl;
		b_tbl->bt_pidx = a_idx;

		/*
		 * If this table is to be wired, make sure the parent A table
		 * wired count is updated to reflect that it has another wired
		 * entry.
		 */
		if (wired)
			a_tbl->at_wcnt++;
		else if (llevel == NONE)
			llevel = NEWB;
	}

	/*
	 * Step 3 - Walk into the C table, if there is no valid C table,
	 * allocate one.
	 */

	b_idx = MMU_TIB(va);            /* Calculate the TIB of the VA */
	b_dte = &b_tbl->bt_dtbl[b_idx]; /* Retrieve descriptor from table */
	if (MMU_VALID_DT(*b_dte)) {     /* Is the descriptor valid? */
		/* The descriptor is valid.  Use the C table it points to. */
		/**************************************
		 *               c_idx                *
		 * |                v                 *
		 * \- b_tbl -> +-+-+-+-+-+-+-+-+-+-+- *
		 *             | | | | | | | | | | |  *
		 *             +-+-+-+-+-+-+-+-+-+-+- *
		 *                  |                 *
		 *                  \- c_tbl -> +-+-- *
		 *                              | | | *
		 *                              +-+-- *
		 **************************************/
		c_pte = mmu_ptov(MMU_PTE_PA(*b_dte));
		c_tbl = mmuC2tmgr(c_pte);

		/* If mapping is wired and table is not */
		if (wired && !c_tbl->ct_wcnt) {
			TAILQ_REMOVE(&c_pool, c_tbl, ct_link);
			b_tbl->bt_wcnt++;
		}
	} else {
		/* The descriptor is invalid.  Allocate a new C table. */
		c_tbl = get_c_table();

		/* Point the parent B table descriptor to this new C table. */
		b_dte->attr.raw = mmu_vtop(c_tbl->ct_dtbl);
		b_dte->attr.raw |= MMU_DT_SHORT;
		b_tbl->bt_ecnt++; /* Update parent's valid entry count */

		/* Create the necessary back references to the parent table */
		c_tbl->ct_parent = b_tbl;
		c_tbl->ct_pidx = b_idx;

		/*
		 * If this table is to be wired, make sure the parent B table
		 * wired count is updated to reflect that it has another wired
		 * entry.
		 */
		if (wired)
			b_tbl->bt_wcnt++;
		else if (llevel == NONE)
			llevel = NEWC;
	}

	/*
	 * Step 4 - Deposit a page descriptor (PTE) into the appropriate
	 * slot of the C table, describing the PA to which the VA is mapped.
	 */

	pte_idx = MMU_TIC(va);
	c_pte = &c_tbl->ct_dtbl[pte_idx];
	if (MMU_VALID_DT(*c_pte)) { /* Is the entry currently valid? */
		/*
		 * The PTE is currently valid.  This particular call
		 * is just a synonym for one (or more) of the following
		 * operations:
		 *     change protection of a page
		 *     change wiring status of a page
		 *     remove the mapping of a page
		 *
		 * XXX - Semi critical: This code should unwire the PTE
		 * and, possibly, associated parent tables if this is a
		 * change wiring operation.  Currently it does not.
		 *
		 * This may be ok if pmap_change_wiring() is the only
		 * interface used to UNWIRE a page.
		 */

		/* First check if this is a wiring operation. */
		if (wired && (c_pte->attr.raw & MMU_SHORT_PTE_WIRED)) {
			/*
			 * The PTE is already wired.  To prevent it from being
			 * counted as a new wiring operation, reset the 'wired'
			 * variable.
			 */
			wired = FALSE;
		}

		/* Is the new address the same as the old? */
		if (MMU_PTE_PA(*c_pte) == pa) {
			/*
			 * Yes, mark that it does not need to be reinserted
			 * into the PV list.
			 */
			insert = FALSE;

			/*
			 * Clear all but the modified, referenced and wired
			 * bits on the PTE.
			 */
			c_pte->attr.raw &= (MMU_SHORT_PTE_M
				| MMU_SHORT_PTE_USED | MMU_SHORT_PTE_WIRED);
		} else {
			/* No, remove the old entry */
			pmap_remove_pte(c_pte);
			insert = TRUE;
		}

		/*
		 * TLB flush is only necessary if modifying current map.
		 * However, in pmap_enter(), the pmap almost always IS
		 * the current pmap, so don't even bother to check.
		 */
		TBIS(va);
	} else {
		/*
		 * The PTE is invalid.  Increment the valid entry count in
		 * the C table manager to reflect the addition of a new entry.
		 */
		c_tbl->ct_ecnt++;

		/* XXX - temporarily make sure the PTE is cleared. */
		c_pte->attr.raw = 0;

		/* It will also need to be inserted into the PV list. */
		insert = TRUE;
	}

	/*
	 * If page is changing from unwired to wired status, set an unused bit
	 * within the PTE to indicate that it is wired.  Also increment the
	 * wired entry count in the C table manager.
	 */
	if (wired) {
		c_pte->attr.raw |= MMU_SHORT_PTE_WIRED;
		c_tbl->ct_wcnt++;
	}

	/*
	 * Map the page, being careful to preserve modify/reference/wired
	 * bits.  At this point it is assumed that the PTE either has no bits
	 * set, or if there are set bits, they are only modified, reference or
	 * wired bits.  If not, the following statement will cause erratic
	 * behavior.
	 */
#ifdef	PMAP_DEBUG
	if (c_pte->attr.raw & ~(MMU_SHORT_PTE_M |
		MMU_SHORT_PTE_USED | MMU_SHORT_PTE_WIRED)) {
		printf("pmap_enter: junk left in PTE at %p\n", c_pte);
		Debugger();
	}
#endif
	c_pte->attr.raw |= ((u_long) pa | MMU_DT_PAGE);

	/*
	 * If the mapping should be read-only, set the write protect
	 * bit in the PTE.
	 */
	if (!(prot & VM_PROT_WRITE))
		c_pte->attr.raw |= MMU_SHORT_PTE_WP;

	/*
	 * If the mapping should be cache inhibited (indicated by the flag
	 * bits found on the lower order of the physical address.)
	 * mark the PTE as a cache inhibited page.
	 */
	if (flags & PMAP_NC)
		c_pte->attr.raw |= MMU_SHORT_PTE_CI;

	/*
	 * If the physical address being mapped is managed by the PV
	 * system then link the pte into the list of pages mapped to that
	 * address.
	 */
	if (insert && managed) {
		pv = pa2pv(pa);
		nidx = pteidx(c_pte);

		s = splimp();
		pvebase[nidx].pve_next = pv->pv_idx;
		pv->pv_idx = nidx;
		splx(s);
	}

	/* Move any allocated tables back into the active pool. */
	
	switch (llevel) {
		case NEWA:
			TAILQ_INSERT_TAIL(&a_pool, a_tbl, at_link);
			/* FALLTHROUGH */
		case NEWB:
			TAILQ_INSERT_TAIL(&b_pool, b_tbl, bt_link);
			/* FALLTHROUGH */
		case NEWC:
			TAILQ_INSERT_TAIL(&c_pool, c_tbl, ct_link);
			/* FALLTHROUGH */
		default:
			break;
	}
}

/* pmap_enter_kernel			INTERNAL
 **
 * Map the given virtual address to the given physical address within the
 * kernel address space.  This function exists because the kernel map does
 * not do dynamic table allocation.  It consists of a contiguous array of ptes
 * and can be edited directly without the need to walk through any tables.
 * 
 * XXX: "Danger, Will Robinson!"
 * Note that the kernel should never take a fault on any page
 * between [ KERNBASE .. virtual_avail ] and this is checked in
 * trap.c for kernel-mode MMU faults.  This means that mappings
 * created in that range must be implicily wired. -gwr
 */
void
pmap_enter_kernel(va, pa, prot)
	vm_offset_t va;
	vm_offset_t pa;
	vm_prot_t   prot;
{
	boolean_t       was_valid, insert;
	u_short         pte_idx, pv_idx;
	int             s, flags;
	mmu_short_pte_t *pte;
	pv_t            *pv;
	vm_offset_t     old_pa;

	flags  = (pa & ~MMU_PAGE_MASK);
	pa    &= MMU_PAGE_MASK;

	/*
	 * Calculate the index of the PTE being modified.
	 */
	pte_idx = (u_long) _btop(va - KERNBASE);

	/* XXX - This array is traditionally named "Sysmap" */
	pte = &kernCbase[pte_idx];

	s = splimp();
	if (MMU_VALID_DT(*pte)) {
		was_valid = TRUE;
		/*
		 * If the PTE is already mapped to an address and it differs
		 * from the address requested, unlink it from the PV list.
		 *
		 * This only applies to mappings within virtual_avail
		 * and VM_MAX_KERNEL_ADDRESS.  All others are not requests
		 * from the VM system and should not be part of the PV system.
		 */
		if ((va >= virtual_avail) && (va < VM_MAX_KERNEL_ADDRESS)) {
		    old_pa = MMU_PTE_PA(*pte);
		    if (pa != old_pa) {
		        if (is_managed(old_pa)) {
		            /* XXX - Make this into a function call? */
		            pv = pa2pv(old_pa);
		            pv_idx = pv->pv_idx;
		            if (pv_idx == pte_idx) {
		                pv->pv_idx = pvebase[pte_idx].pve_next;
		            } else {
		                while (pvebase[pv_idx].pve_next != pte_idx)
		                    pv_idx = pvebase[pv_idx].pve_next;
		                pvebase[pv_idx].pve_next =
		                    pvebase[pte_idx].pve_next;
		            }
		            /* Save modified/reference bits */
		            pv->pv_flags |= (u_short) pte->attr.raw;
		        }
		        if (is_managed(pa))
		            insert = TRUE;
		        else
		            insert = FALSE;
		        /*
		         * Clear out any old bits in the PTE.
		         */
		        pte->attr.raw = MMU_DT_INVALID;
		    } else {
		        /*
		         * Old PA and new PA are the same.  No need to relink
		         * the mapping within the PV list.
		         */
		        insert = FALSE;

		        /*
		         * Save any mod/ref bits on the PTE.
		         */
		        pte->attr.raw &= (MMU_SHORT_PTE_USED|MMU_SHORT_PTE_M);
		    }
		} else {
		    /*
		     * If the VA lies below virtual_avail or beyond 
		     * VM_MAX_KERNEL_ADDRESS, it is not a request by the VM
		     * system and hence does not need to be linked into the PV
		     * system.
		     */
		    insert = FALSE;
		    pte->attr.raw = MMU_DT_INVALID;
		}
	} else {
		pte->attr.raw = MMU_DT_INVALID;
		was_valid = FALSE;
		if ((va >= virtual_avail) && (va < VM_MAX_KERNEL_ADDRESS)) {
			if (is_managed(pa))
				insert = TRUE; 
			else
				insert = FALSE;
		} else
			insert = FALSE;
	}

	/*
	 * Map the page.  Being careful to preserve modified/referenced bits
	 * on the PTE.
	 */
	pte->attr.raw |= (pa | MMU_DT_PAGE);

	if (!(prot & VM_PROT_WRITE)) /* If access should be read-only */
		pte->attr.raw |= MMU_SHORT_PTE_WP;
	if (flags & PMAP_NC)
		pte->attr.raw |= MMU_SHORT_PTE_CI;
	if (was_valid)
		TBIS(va);

	/*
	 * Insert the PTE into the PV system, if need be.
	 */
	if (insert) {
		pv = pa2pv(pa);
		pvebase[pte_idx].pve_next = pv->pv_idx;
		pv->pv_idx = pte_idx;
	}
	splx(s);
	
}

/* pmap_protect			INTERFACE
 **
 * Apply the given protection to the given virtual address range within
 * the given map.
 *
 * It is ok for the protection applied to be stronger than what is
 * specified.  We use this to our advantage when the given map has no
 * mapping for the virtual address.  By skipping a page when this
 * is discovered, we are effectively applying a protection of VM_PROT_NONE,
 * and therefore do not need to map the page just to apply a protection
 * code.  Only pmap_enter() needs to create new mappings if they do not exist.
 *
 * XXX - This function could be speeded up by using pmap_stroll() for inital
 *       setup, and then manual scrolling in the for() loop.
 */
void
pmap_protect(pmap, startva, endva, prot)
	pmap_t pmap;
	vm_offset_t startva, endva;
	vm_prot_t prot;
{
	boolean_t iscurpmap;
	int a_idx, b_idx, c_idx;
	a_tmgr_t *a_tbl;
	b_tmgr_t *b_tbl;
	c_tmgr_t *c_tbl;
	mmu_short_pte_t *pte;

	if (pmap == NULL)
		return;
	if (pmap == pmap_kernel()) {
		pmap_protect_kernel(startva, endva, prot);
		return;
	}

	/*
	 * In this particular pmap implementation, there are only three
	 * types of memory protection: 'all' (read/write/execute),
	 * 'read-only' (read/execute) and 'none' (no mapping.)
	 * It is not possible for us to treat 'executable' as a separate
	 * protection type.  Therefore, protection requests that seek to
	 * remove execute permission while retaining read or write, and those
	 * that make little sense (write-only for example) are ignored.
	 */
	switch (prot) {
		case VM_PROT_NONE:
			/*
			 * A request to apply the protection code of
			 * 'VM_PROT_NONE' is a synonym for pmap_remove().
			 */
			pmap_remove(pmap, startva, endva);
			return;
		case	VM_PROT_EXECUTE:
		case	VM_PROT_READ:
		case	VM_PROT_READ|VM_PROT_EXECUTE:
			/* continue */
			break;
		case	VM_PROT_WRITE:
		case	VM_PROT_WRITE|VM_PROT_READ:
		case	VM_PROT_WRITE|VM_PROT_EXECUTE:
		case	VM_PROT_ALL:
			/* None of these should happen in a sane system. */
			return;
	}

	/*
	 * If the pmap has no A table, it has no mappings and therefore
	 * there is nothing to protect.
	 */
	if ((a_tbl = pmap->pm_a_tmgr) == NULL)
		return;

	a_idx = MMU_TIA(startva);
	b_idx = MMU_TIB(startva);
	c_idx = MMU_TIC(startva);
	b_tbl = (b_tmgr_t *) c_tbl = NULL;

	iscurpmap = (pmap == current_pmap());
	while (startva < endva) {
		if (b_tbl || MMU_VALID_DT(a_tbl->at_dtbl[a_idx])) {
		  if (b_tbl == NULL) {
		    b_tbl = (b_tmgr_t *) a_tbl->at_dtbl[a_idx].addr.raw;
		    b_tbl = mmu_ptov((vm_offset_t) b_tbl);
		    b_tbl = mmuB2tmgr((mmu_short_dte_t *) b_tbl);
		  }
		  if (c_tbl || MMU_VALID_DT(b_tbl->bt_dtbl[b_idx])) {
		    if (c_tbl == NULL) {
		      c_tbl = (c_tmgr_t *) MMU_DTE_PA(b_tbl->bt_dtbl[b_idx]);
		      c_tbl = mmu_ptov((vm_offset_t) c_tbl);
		      c_tbl = mmuC2tmgr((mmu_short_pte_t *) c_tbl);
		    }
		    if (MMU_VALID_DT(c_tbl->ct_dtbl[c_idx])) {
		      pte = &c_tbl->ct_dtbl[c_idx];
		      /* make the mapping read-only */
		      pte->attr.raw |= MMU_SHORT_PTE_WP;
		      /*
		       * If we just modified the current address space,
		       * flush any translations for the modified page from
		       * the translation cache and any data from it in the
		       * data cache.
		       */
		      if (iscurpmap)
		          TBIS(startva);
		    }
		    startva += NBPG;

		    if (++c_idx >= MMU_C_TBL_SIZE) { /* exceeded C table? */
		      c_tbl = NULL;
		      c_idx = 0;
		      if (++b_idx >= MMU_B_TBL_SIZE) { /* exceeded B table? */
		        b_tbl = NULL;
		        b_idx = 0;
		      }
		    }
		  } else { /* C table wasn't valid */
		    c_tbl = NULL;
		    c_idx = 0;
		    startva += MMU_TIB_RANGE;
		    if (++b_idx >= MMU_B_TBL_SIZE) { /* exceeded B table? */
		      b_tbl = NULL;
		      b_idx = 0;
		    }
		  } /* C table */
		} else { /* B table wasn't valid */
		  b_tbl = NULL;
		  b_idx = 0;
		  startva += MMU_TIA_RANGE;
		  a_idx++;
		} /* B table */
	}
}

/* pmap_protect_kernel			INTERNAL
 **
 * Apply the given protection code to a kernel address range.
 */
void
pmap_protect_kernel(startva, endva, prot)
	vm_offset_t startva, endva;
	vm_prot_t prot;
{
	vm_offset_t va;
	mmu_short_pte_t *pte;

	pte = &kernCbase[(unsigned long) _btop(startva - KERNBASE)];
	for (va = startva; va < endva; va += NBPG, pte++) {
		if (MMU_VALID_DT(*pte)) {
		    switch (prot) {
		        case VM_PROT_ALL:
		            break;
		        case VM_PROT_EXECUTE:
		        case VM_PROT_READ:
		        case VM_PROT_READ|VM_PROT_EXECUTE:
		            pte->attr.raw |= MMU_SHORT_PTE_WP;
		            break;
		        case VM_PROT_NONE:
		            /* this is an alias for 'pmap_remove_kernel' */
		            pmap_remove_pte(pte);
		            break;
		        default:
		            break;
		    }
		    /*
		     * since this is the kernel, immediately flush any cached
		     * descriptors for this address.
		     */
		    TBIS(va);
		}
	}
}

/* pmap_change_wiring			INTERFACE
 **
 * Changes the wiring of the specified page.
 *
 * This function is called from vm_fault.c to unwire
 * a mapping.  It really should be called 'pmap_unwire'
 * because it is never asked to do anything but remove
 * wirings.
 */
void
pmap_change_wiring(pmap, va, wire)
	pmap_t pmap;
	vm_offset_t va;
	boolean_t wire;
{
	int a_idx, b_idx, c_idx;
	a_tmgr_t *a_tbl;
	b_tmgr_t *b_tbl;
	c_tmgr_t *c_tbl;
	mmu_short_pte_t *pte;
	
	/* Kernel mappings always remain wired. */
	if (pmap == pmap_kernel())
		return;

#ifdef	PMAP_DEBUG
	if (wire == TRUE)
		panic("pmap_change_wiring: wire requested.");
#endif
	
	/*
	 * Walk through the tables.  If the walk terminates without
	 * a valid PTE then the address wasn't wired in the first place.
	 * Return immediately.
	 */
	if (pmap_stroll(pmap, va, &a_tbl, &b_tbl, &c_tbl, &pte, &a_idx,
		&b_idx, &c_idx) == FALSE)
		return;


	/* Is the PTE wired?  If not, return. */
	if (!(pte->attr.raw & MMU_SHORT_PTE_WIRED))
		return;

	/* Remove the wiring bit. */
	pte->attr.raw &= ~(MMU_SHORT_PTE_WIRED);

	/*
	 * Decrement the wired entry count in the C table.
	 * If it reaches zero the following things happen:
	 * 1. The table no longer has any wired entries and is considered 
	 *    unwired.
	 * 2. It is placed on the available queue.
	 * 3. The parent table's wired entry count is decremented.
	 * 4. If it reaches zero, this process repeats at step 1 and
	 *    stops at after reaching the A table.
	 */
	if (--c_tbl->ct_wcnt == 0) {
		TAILQ_INSERT_TAIL(&c_pool, c_tbl, ct_link);
		if (--b_tbl->bt_wcnt == 0) {
			TAILQ_INSERT_TAIL(&b_pool, b_tbl, bt_link);
			if (--a_tbl->at_wcnt == 0) {
				TAILQ_INSERT_TAIL(&a_pool, a_tbl, at_link);
			}
		}
	}
}

/* pmap_pageable			INTERFACE
 **
 * Make the specified range of addresses within the given pmap,
 * 'pageable' or 'not-pageable'.  A pageable page must not cause
 * any faults when referenced.  A non-pageable page may.
 *
 * This routine is only advisory.  The VM system will call pmap_enter()
 * to wire or unwire pages that are going to be made pageable before calling
 * this function.  By the time this routine is called, everything that needs
 * to be done has already been done.
 */
void
pmap_pageable(pmap, start, end, pageable)
	pmap_t pmap;
	vm_offset_t start, end;
	boolean_t pageable;
{
	/* not implemented. */
}

/* pmap_copy				INTERFACE
 **
 * Copy the mappings of a range of addresses in one pmap, into
 * the destination address of another.
 *
 * This routine is advisory.  Should we one day decide that MMU tables
 * may be shared by more than one pmap, this function should be used to
 * link them together.  Until that day however, we do nothing.
 */
void
pmap_copy(pmap_a, pmap_b, dst, len, src)
	pmap_t pmap_a, pmap_b;
	vm_offset_t dst;
	vm_size_t   len;
	vm_offset_t src;
{
	/* not implemented. */
}

/* pmap_copy_page			INTERFACE
 **
 * Copy the contents of one physical page into another.
 *
 * This function makes use of two virtual pages allocated in pmap_bootstrap()
 * to map the two specified physical pages into the kernel address space.  It
 * then uses bcopy() to copy one into the other.
 *
 * Note: We could use the transparent translation registers to make the
 * mappings.  If we do so, be sure to disable interrupts before using them.
 */
void
pmap_copy_page(src, dst)
	vm_offset_t src, dst;
{
	PMAP_LOCK();
	if (tmp_vpages_inuse)
		panic("pmap_copy_page: temporary vpages are in use.");
	tmp_vpages_inuse++;

	/* XXX - Use non-cached mappings to avoid cache polution? */
	pmap_enter_kernel(tmp_vpages[0], src, VM_PROT_READ);
	pmap_enter_kernel(tmp_vpages[1], dst, VM_PROT_READ|VM_PROT_WRITE);
	copypage((char *) tmp_vpages[0], (char *) tmp_vpages[1]);

	tmp_vpages_inuse--;
	PMAP_UNLOCK();
}

/* pmap_zero_page			INTERFACE
 **
 * Zero the contents of the specified physical page.
 *
 * Uses one of the virtual pages allocated in pmap_boostrap()
 * to map the specified page into the kernel address space.  Then uses
 * bzero() to zero out the page.
 */
void
pmap_zero_page(pa)
	vm_offset_t pa;
{
	PMAP_LOCK();
	if (tmp_vpages_inuse)
		panic("pmap_zero_page: temporary vpages are in use.");
	tmp_vpages_inuse++;

	pmap_enter_kernel(tmp_vpages[0], pa, VM_PROT_READ|VM_PROT_WRITE);
	zeropage((char *) tmp_vpages[0]);

	tmp_vpages_inuse--;
	PMAP_UNLOCK();
}

/* pmap_collect			INTERFACE
 **
 * Called from the VM system when we are about to swap out
 * the process using this pmap.  This should give up any
 * resources held here, including all its MMU tables.
 */
void
pmap_collect(pmap)
	pmap_t pmap;
{
	/* XXX - todo... */
}

/* pmap_create			INTERFACE
 **
 * Create and return a pmap structure.
 */
pmap_t
pmap_create(size)
	vm_size_t size;
{
	pmap_t	pmap;

	if (size)
		return NULL;

	pmap = (pmap_t) malloc(sizeof(struct pmap), M_VMPMAP, M_WAITOK);
	pmap_pinit(pmap);

	return pmap;
}

/* pmap_pinit			INTERNAL
 **
 * Initialize a pmap structure.
 */
void
pmap_pinit(pmap)
	pmap_t pmap;
{
	bzero(pmap, sizeof(struct pmap));
	pmap->pm_a_tmgr = NULL;
	pmap->pm_a_phys = kernAphys;
}

/* pmap_release				INTERFACE
 **
 * Release any resources held by the given pmap.
 *
 * This is the reverse analog to pmap_pinit.  It does not
 * necessarily mean for the pmap structure to be deallocated,
 * as in pmap_destroy.
 */
void
pmap_release(pmap)
	pmap_t pmap;
{
	/*
	 * As long as the pmap contains no mappings,
	 * which always should be the case whenever
	 * this function is called, there really should
	 * be nothing to do.
	 *
	 * XXX - This function is being called while there are
	 * still valid mappings, so I guess the above must not
	 * be true.
	 * XXX - Unless the mappings persist due to a bug here...
	 *     + That's what was happening.  The map had no mappings,
	 *       but it still had an A table.  pmap_remove() was not
	 *       releasing tables when they were empty.
	 */
#ifdef	PMAP_DEBUG
	if (pmap == NULL)
		return;
	if (pmap == pmap_kernel())
		panic("pmap_release: kernel pmap");
#endif
	/*
	 * XXX - If this pmap has an A table, give it back.
	 * The pmap SHOULD be empty by now, and pmap_remove
	 * should have already given back the A table...
	 * However, I see:  pmap->pm_a_tmgr->at_ecnt == 1
	 * at this point, which means some mapping was not
	 * removed when it should have been. -gwr
	 */
	if (pmap->pm_a_tmgr != NULL) {
		/* First make sure we are not using it! */
		if (kernel_crp.rp_addr == pmap->pm_a_phys) {
			kernel_crp.rp_addr = kernAphys;
			loadcrp(&kernel_crp);
		}
#ifdef	PMAP_DEBUG /* XXX - todo! */
		/* XXX - Now complain... */
		printf("pmap_release: still have table\n");
		Debugger();
#endif
		free_a_table(pmap->pm_a_tmgr, TRUE);
		pmap->pm_a_tmgr = NULL;
		pmap->pm_a_phys = kernAphys;
	}
}

/* pmap_reference			INTERFACE
 **
 * Increment the reference count of a pmap.
 */
void
pmap_reference(pmap)
	pmap_t pmap;
{
	if (pmap == NULL)
		return;

	/* pmap_lock(pmap); */
	pmap->pm_refcount++;
	/* pmap_unlock(pmap); */
}

/* pmap_dereference			INTERNAL
 **
 * Decrease the reference count on the given pmap
 * by one and return the current count.
 */
int
pmap_dereference(pmap)
	pmap_t pmap;
{
	int rtn;

	if (pmap == NULL)
		return 0;

	/* pmap_lock(pmap); */
	rtn = --pmap->pm_refcount;
	/* pmap_unlock(pmap); */

	return rtn;
}
	
/* pmap_destroy			INTERFACE
 **
 * Decrement a pmap's reference count and delete
 * the pmap if it becomes zero.  Will be called
 * only after all mappings have been removed.
 */
void
pmap_destroy(pmap)
	pmap_t pmap;
{
	if (pmap == NULL)
		return;
	if (pmap == &kernel_pmap)
		panic("pmap_destroy: kernel_pmap!");
	if (pmap_dereference(pmap) == 0) {
		pmap_release(pmap);
		free(pmap, M_VMPMAP);
	}
}

/* pmap_is_referenced			INTERFACE
 **
 * Determine if the given physical page has been
 * referenced (read from [or written to.])
 */
boolean_t
pmap_is_referenced(pa)
	vm_offset_t pa;
{
	pv_t      *pv;
	int       idx, s;

	if (!pv_initialized)
		return FALSE;
	/* XXX - this may be unecessary. */
	if (!is_managed(pa))
		return FALSE;

	pv = pa2pv(pa);
	/*
	 * Check the flags on the pv head.  If they are set,
	 * return immediately.  Otherwise a search must be done.
	 */
	if (pv->pv_flags & PV_FLAGS_USED)
		return TRUE;
	else {
		s = splimp();
		/*
		 * Search through all pv elements pointing
		 * to this page and query their reference bits
		 */
		for (idx = pv->pv_idx; idx != PVE_EOL; idx =
			pvebase[idx].pve_next)
			if (MMU_PTE_USED(kernCbase[idx])) {
				splx(s);
				return TRUE;
			}
		splx(s);
	}

	return FALSE;
}

/* pmap_is_modified			INTERFACE
 **
 * Determine if the given physical page has been
 * modified (written to.)
 */
boolean_t
pmap_is_modified(pa)
	vm_offset_t pa;
{
	pv_t      *pv;
	int       idx, s;

	if (!pv_initialized)
		return FALSE;
	/* XXX - this may be unecessary. */
	if (!is_managed(pa))
		return FALSE;

	/* see comments in pmap_is_referenced() */
	pv = pa2pv(pa);
	if (pv->pv_flags & PV_FLAGS_MDFY) {
		return TRUE;
	} else {
		s = splimp();
		for (idx = pv->pv_idx; idx != PVE_EOL; idx =
			pvebase[idx].pve_next)
			if (MMU_PTE_MODIFIED(kernCbase[idx])) {
				splx(s);
				return TRUE;
			}
		splx(s);
	}

	return FALSE;
}

/* pmap_page_protect			INTERFACE
 **
 * Applies the given protection to all mappings to the given
 * physical page.
 */
void
pmap_page_protect(pa, prot)
	vm_offset_t pa;
	vm_prot_t prot;
{
	pv_t      *pv;
	int       idx, s;
	vm_offset_t va;
	struct mmu_short_pte_struct *pte;
	c_tmgr_t  *c_tbl;
	pmap_t    pmap, curpmap;

	if (!is_managed(pa))
		return;
	
	curpmap = current_pmap();
	pv = pa2pv(pa);
	s = splimp();
	for (idx = pv->pv_idx; idx != PVE_EOL; idx = pvebase[idx].pve_next) {
		pte = &kernCbase[idx];
		switch (prot) {
			case VM_PROT_ALL:
				/* do nothing */
				break;
			case VM_PROT_EXECUTE:
			case VM_PROT_READ:
			case VM_PROT_READ|VM_PROT_EXECUTE:
				pte->attr.raw |= MMU_SHORT_PTE_WP;

				/*
				 * Determine the virtual address mapped by
				 * the PTE and flush ATC entries if necessary.
				 */
				va = pmap_get_pteinfo(idx, &pmap, &c_tbl);
				if (pmap == curpmap || pmap == pmap_kernel())
					TBIS(va);
				break;
			case VM_PROT_NONE:
				/* Save the mod/ref bits. */
				pv->pv_flags |= pte->attr.raw;
				/* Invalidate the PTE. */
				pte->attr.raw = MMU_DT_INVALID;

				/*
				 * Update table counts.  And flush ATC entries
				 * if necessary.
				 */
				va = pmap_get_pteinfo(idx, &pmap, &c_tbl);

				/*
				 * If the PTE belongs to the kernel map,
				 * be sure to flush the page it maps.
				 */
				if (pmap == pmap_kernel()) {
					TBIS(va);
				} else {
					/*
					 * The PTE belongs to a user map.
					 * update the entry count in the C
					 * table to which it belongs and flush
					 * the ATC if the mapping belongs to
					 * the current pmap.
					 */
					c_tbl->ct_ecnt--;
					if (pmap == curpmap)
						TBIS(va);
				}
				break;
			default:
				break;
		}
	}

	/*
	 * If the protection code indicates that all mappings to the page
	 * be removed, truncate the PV list to zero entries.
	 */
	if (prot == VM_PROT_NONE)
		pv->pv_idx = PVE_EOL;
	splx(s);
}

/* pmap_get_pteinfo		INTERNAL
 **
 * Called internally to find the pmap and virtual address within that
 * map to which the pte at the given index maps.  Also includes the PTE's C
 * table manager.
 *
 * Returns the pmap in the argument provided, and the virtual address
 * by return value.
 */
vm_offset_t
pmap_get_pteinfo(idx, pmap, tbl)
	u_int idx;
	pmap_t *pmap;
	c_tmgr_t **tbl;
{
	a_tmgr_t    *a_tbl;
	b_tmgr_t    *b_tbl;
	c_tmgr_t    *c_tbl;
	vm_offset_t     va = 0;

	/*
	 * Determine if the PTE is a kernel PTE or a user PTE.
	 */
	if (idx >= NUM_KERN_PTES) {
		/*
		 * The PTE belongs to a user mapping.
		 * Find the virtual address by decoding table indices.
		 * Each successive decode will reveal the address from
		 * least to most significant bit fashion.
		 *
		 * 31                              0
		 * +-------------------------------+
		 * |AAAAAAABBBBBBCCCCCC............|
		 * +-------------------------------+
		 */
		/* XXX: c_tbl = mmuC2tmgr(pte); */
		/* XXX: Would like an inline for this to validate idx... */
		c_tbl = &Ctmgrbase[(idx - NUM_KERN_PTES) / MMU_C_TBL_SIZE];
		b_tbl = c_tbl->ct_parent;
		a_tbl = b_tbl->bt_parent;
		*pmap = a_tbl->at_parent;
		*tbl = c_tbl;

		/* Start with the 'C' bits, then add B and A... */
		va |= ((idx % MMU_C_TBL_SIZE) << MMU_TIC_SHIFT);
		va |= (c_tbl->ct_pidx << MMU_TIB_SHIFT);
		va |= (b_tbl->bt_pidx << MMU_TIA_SHIFT);
	} else {
		/*
		 * The PTE belongs to the kernel map.
		 */
		*pmap = pmap_kernel();

		va = _ptob(idx);
		va += KERNBASE;
	}
		
	return va;
}

#if	0	/* XXX - I am eliminating this function. */
/* pmap_find_tic			INTERNAL
 **
 * Given the address of a pte, find the TIC (level 'C' table index) for
 * the pte within its C table.
 */
char
pmap_find_tic(pte)
	mmu_short_pte_t *pte;
{
	return ((pte - mmuCbase) % MMU_C_TBL_SIZE);
}
#endif	/* 0 */


/* pmap_clear_modify			INTERFACE
 **
 * Clear the modification bit on the page at the specified
 * physical address.
 *
 */
void
pmap_clear_modify(pa)
	vm_offset_t pa;
{
	pmap_clear_pv(pa, PV_FLAGS_MDFY);
}

/* pmap_clear_reference			INTERFACE
 **
 * Clear the referenced bit on the page at the specified
 * physical address.
 */
void
pmap_clear_reference(pa)
	vm_offset_t pa;
{
	pmap_clear_pv(pa, PV_FLAGS_USED);
}
	
/* pmap_clear_pv			INTERNAL
 **
 * Clears the specified flag from the specified physical address.
 * (Used by pmap_clear_modify() and pmap_clear_reference().)
 *
 * Flag is one of:
 *   PV_FLAGS_MDFY - Page modified bit.
 *   PV_FLAGS_USED - Page used (referenced) bit.
 *
 * This routine must not only clear the flag on the pv list
 * head.  It must also clear the bit on every pte in the pv
 * list associated with the address.
 */
void
pmap_clear_pv(pa, flag)
	vm_offset_t pa;
	int flag;
{
	pv_t      *pv;
	int       idx, s;
	vm_offset_t     va;
	pmap_t          pmap;
	mmu_short_pte_t *pte;
	c_tmgr_t        *c_tbl;

	pv = pa2pv(pa);

	s = splimp();
	pv->pv_flags &= ~(flag);
	for (idx = pv->pv_idx; idx != PVE_EOL; idx = pvebase[idx].pve_next) {
		pte = &kernCbase[idx];
		pte->attr.raw &= ~(flag);
		/*
		 * The MC68030 MMU will not set the modified or
		 * referenced bits on any MMU tables for which it has
		 * a cached descriptor with its modify bit set.  To insure
		 * that it will modify these bits on the PTE during the next
		 * time it is written to or read from, we must flush it from
		 * the ATC.
		 *
		 * Ordinarily it is only necessary to flush the descriptor
		 * if it is used in the current address space.  But since I
		 * am not sure that there will always be a notion of
		 * 'the current address space' when this function is called,
		 * I will skip the test and always flush the address.  It
		 * does no harm.
		 */
		va = pmap_get_pteinfo(idx, &pmap, &c_tbl);
		TBIS(va);
	}
	splx(s);
}

/* pmap_extract			INTERFACE
 **
 * Return the physical address mapped by the virtual address
 * in the specified pmap or 0 if it is not known.
 *
 * Note: this function should also apply an exclusive lock
 * on the pmap system during its duration.
 */
vm_offset_t
pmap_extract(pmap, va)
	pmap_t      pmap;
	vm_offset_t va;
{
	int a_idx, b_idx, pte_idx;
	a_tmgr_t	*a_tbl;
	b_tmgr_t	*b_tbl;
	c_tmgr_t	*c_tbl;
	mmu_short_pte_t	*c_pte;

	if (pmap == pmap_kernel())
		return pmap_extract_kernel(va);
	if (pmap == NULL)
		return 0;

	if (pmap_stroll(pmap, va, &a_tbl, &b_tbl, &c_tbl,
		&c_pte, &a_idx, &b_idx, &pte_idx) == FALSE)
		return 0;

	if (!MMU_VALID_DT(*c_pte))
		return 0;

	return (MMU_PTE_PA(*c_pte));
}

/* pmap_extract_kernel		INTERNAL
 **
 * Extract a translation from the kernel address space.
 */
vm_offset_t
pmap_extract_kernel(va)
	vm_offset_t va;
{
	mmu_short_pte_t *pte;

	pte = &kernCbase[(u_int) _btop(va - KERNBASE)];
	return MMU_PTE_PA(*pte);
}

/* pmap_remove_kernel		INTERNAL
 **
 * Remove the mapping of a range of virtual addresses from the kernel map.
 * The arguments are already page-aligned.
 */
void
pmap_remove_kernel(sva, eva)
	vm_offset_t sva;
	vm_offset_t eva;
{
	int idx, eidx;

#ifdef	PMAP_DEBUG
	if ((sva & PGOFSET) || (eva & PGOFSET))
		panic("pmap_remove_kernel: alignment");
#endif

	idx  = _btop(sva - KERNBASE);
	eidx = _btop(eva - KERNBASE);

	while (idx < eidx)
		pmap_remove_pte(&kernCbase[idx++]);
	/* Always flush the ATC when maniplating the kernel address space. */
	TBIAS();
}

/* pmap_remove			INTERFACE
 **
 * Remove the mapping of a range of virtual addresses from the given pmap.
 *
 * If the range contains any wired entries, this function will probably create
 * disaster.
 */
void
pmap_remove(pmap, start, end)
	pmap_t pmap;
	vm_offset_t start;
	vm_offset_t end;
{

	if (pmap == pmap_kernel()) {
		pmap_remove_kernel(start, end);
		return;
	}

	/*
	 * XXX - Temporary(?) statement to prevent panic caused
	 * by vm_alloc_with_pager() handing us a software map (ie NULL)
	 * to remove because it couldn't get backing store.
	 * (I guess.)
	 */
	if (pmap == NULL)
		return;

	/*
	 * If the pmap doesn't have an A table of its own, it has no mappings
	 * that can be removed.
	 */
	if (pmap->pm_a_tmgr == NULL)
		return;

	/*
	 * Remove the specified range from the pmap.  If the function
	 * returns true, the operation removed all the valid mappings
	 * in the pmap and freed its A table.  If this happened to the
	 * currently loaded pmap, the MMU root pointer must be reloaded
	 * with the default 'kernel' map.
	 */ 
	if (pmap_remove_a(pmap->pm_a_tmgr, start, end)) {
		if (kernel_crp.rp_addr == pmap->pm_a_phys) {
			kernel_crp.rp_addr = kernAphys;
			loadcrp(&kernel_crp);
			/* will do TLB flush below */
		}
		pmap->pm_a_tmgr = NULL;
		pmap->pm_a_phys = kernAphys;
	}

	/*
	 * If we just modified the current address space,
	 * make sure to flush the MMU cache.
	 *
	 * XXX - this could be an unecessarily large flush.
	 * XXX - Could decide, based on the size of the VA range
	 * to be removed, whether to flush "by pages" or "all".
	 */
	if (pmap == current_pmap())
		TBIAU();
}

/* pmap_remove_a			INTERNAL
 **
 * This is function number one in a set of three that removes a range
 * of memory in the most efficient manner by removing the highest possible
 * tables from the memory space.  This particular function attempts to remove
 * as many B tables as it can, delegating the remaining fragmented ranges to
 * pmap_remove_b().
 *
 * If the removal operation results in an empty A table, the function returns
 * TRUE.
 *
 * It's ugly but will do for now.
 */
boolean_t
pmap_remove_a(a_tbl, start, end)
	a_tmgr_t *a_tbl;
	vm_offset_t start;
	vm_offset_t end;
{
	boolean_t empty;
	int idx;
	vm_offset_t nstart, nend;
	b_tmgr_t *b_tbl;
	mmu_long_dte_t  *a_dte;
	mmu_short_dte_t *b_dte;

	/*
	 * The following code works with what I call a 'granularity
	 * reduction algorithim'.  A range of addresses will always have
	 * the following properties, which are classified according to
	 * how the range relates to the size of the current granularity
	 * - an A table entry:
	 *
	 *            1 2       3 4
	 * -+---+---+---+---+---+---+---+-
	 * -+---+---+---+---+---+---+---+-
	 *
	 * A range will always start on a granularity boundary, illustrated
	 * by '+' signs in the table above, or it will start at some point
	 * inbetween a granularity boundary, as illustrated by point 1.
	 * The first step in removing a range of addresses is to remove the
	 * range between 1 and 2, the nearest granularity boundary.  This
	 * job is handled by the section of code governed by the
	 * 'if (start < nstart)' statement.
	 * 
	 * A range will always encompass zero or more intergral granules,
	 * illustrated by points 2 and 3.  Integral granules are easy to
	 * remove.  The removal of these granules is the second step, and
	 * is handled by the code block 'if (nstart < nend)'.
	 *
	 * Lastly, a range will always end on a granularity boundary,
	 * ill. by point 3, or it will fall just beyond one, ill. by point
	 * 4.  The last step involves removing this range and is handled by
	 * the code block 'if (nend < end)'.
	 */
	nstart = MMU_ROUND_UP_A(start);
	nend = MMU_ROUND_A(end);

	if (start < nstart) {
		/*
		 * This block is executed if the range starts between
		 * a granularity boundary.
		 *
		 * First find the DTE which is responsible for mapping
		 * the start of the range.
		 */
		idx = MMU_TIA(start);
		a_dte = &a_tbl->at_dtbl[idx];

		/*
		 * If the DTE is valid then delegate the removal of the sub
		 * range to pmap_remove_b(), which can remove addresses at
		 * a finer granularity.
		 */
		if (MMU_VALID_DT(*a_dte)) {
			b_dte = mmu_ptov(a_dte->addr.raw);
			b_tbl = mmuB2tmgr(b_dte);

			/*
			 * The sub range to be removed starts at the start
			 * of the full range we were asked to remove, and ends
			 * at the greater of:
			 * 1. The end of the full range, -or-
			 * 2. The end of the full range, rounded down to the
			 *    nearest granularity boundary.
			 */
			if (end < nstart)
				empty = pmap_remove_b(b_tbl, start, end);
			else
				empty = pmap_remove_b(b_tbl, start, nstart);

			/*
			 * If the removal resulted in an empty B table,
			 * invalidate the DTE that points to it and decrement
			 * the valid entry count of the A table.
			 */
			if (empty) {
				a_dte->attr.raw = MMU_DT_INVALID;
				a_tbl->at_ecnt--;
			}
		}
		/*
		 * If the DTE is invalid, the address range is already non-
		 * existant and can simply be skipped.
		 */
	}
	if (nstart < nend) {
		/*
		 * This block is executed if the range spans a whole number
		 * multiple of granules (A table entries.)
		 *
		 * First find the DTE which is responsible for mapping
		 * the start of the first granule involved.
		 */
		idx = MMU_TIA(nstart);
		a_dte = &a_tbl->at_dtbl[idx];

		/*
		 * Remove entire sub-granules (B tables) one at a time,
		 * until reaching the end of the range.
		 */
		for (; nstart < nend; a_dte++, nstart += MMU_TIA_RANGE)
			if (MMU_VALID_DT(*a_dte)) {
				/*
				 * Find the B table manager for the
				 * entry and free it.
				 */
				b_dte = mmu_ptov(a_dte->addr.raw);
				b_tbl = mmuB2tmgr(b_dte);
				free_b_table(b_tbl, TRUE);

				/*
				 * Invalidate the DTE that points to the
				 * B table and decrement the valid entry
				 * count of the A table.
				 */
				a_dte->attr.raw = MMU_DT_INVALID;
				a_tbl->at_ecnt--;
			}
	}
	if (nend < end) {
		/*
		 * This block is executed if the range ends beyond a
		 * granularity boundary.
		 *
		 * First find the DTE which is responsible for mapping
		 * the start of the nearest (rounded down) granularity
		 * boundary.
		 */
		idx = MMU_TIA(nend);
		a_dte = &a_tbl->at_dtbl[idx];

		/*
		 * If the DTE is valid then delegate the removal of the sub
		 * range to pmap_remove_b(), which can remove addresses at
		 * a finer granularity.
		 */
		if (MMU_VALID_DT(*a_dte)) {
			/*
			 * Find the B table manager for the entry
			 * and hand it to pmap_remove_b() along with
			 * the sub range.
			 */
			b_dte = mmu_ptov(a_dte->addr.raw);
			b_tbl = mmuB2tmgr(b_dte);

			empty = pmap_remove_b(b_tbl, nend, end);

			/*
			 * If the removal resulted in an empty B table,
			 * invalidate the DTE that points to it and decrement
			 * the valid entry count of the A table.
			 */
			if (empty) {
				a_dte->attr.raw = MMU_DT_INVALID;
				a_tbl->at_ecnt--;
			}
		}
	}

	/*
	 * If there are no more entries in the A table, release it
	 * back to the available pool and return TRUE.
	 */
	if (a_tbl->at_ecnt == 0) {
		a_tbl->at_parent = NULL;
		TAILQ_REMOVE(&a_pool, a_tbl, at_link);
		TAILQ_INSERT_HEAD(&a_pool, a_tbl, at_link);
		empty = TRUE;
	} else {
		empty = FALSE;
	}

	return empty;
}

/* pmap_remove_b			INTERNAL
 **
 * Remove a range of addresses from an address space, trying to remove entire
 * C tables if possible.
 *
 * If the operation results in an empty B table, the function returns TRUE.
 */
boolean_t
pmap_remove_b(b_tbl, start, end)
	b_tmgr_t *b_tbl;
	vm_offset_t start;
	vm_offset_t end;
{
	boolean_t empty;
	int idx;
	vm_offset_t nstart, nend, rstart;
	c_tmgr_t *c_tbl;
	mmu_short_dte_t  *b_dte;
	mmu_short_pte_t  *c_dte;
	

	nstart = MMU_ROUND_UP_B(start);
	nend = MMU_ROUND_B(end);

	if (start < nstart) {
		idx = MMU_TIB(start);
		b_dte = &b_tbl->bt_dtbl[idx];
		if (MMU_VALID_DT(*b_dte)) {
			c_dte = mmu_ptov(MMU_DTE_PA(*b_dte));
			c_tbl = mmuC2tmgr(c_dte);
			if (end < nstart)
				empty = pmap_remove_c(c_tbl, start, end);
			else
				empty = pmap_remove_c(c_tbl, start, nstart);
			if (empty) {
				b_dte->attr.raw = MMU_DT_INVALID;
				b_tbl->bt_ecnt--;
			}
		}
	}
	if (nstart < nend) {
		idx = MMU_TIB(nstart);
		b_dte = &b_tbl->bt_dtbl[idx];
		rstart = nstart;
		while (rstart < nend) {
			if (MMU_VALID_DT(*b_dte)) {
				c_dte = mmu_ptov(MMU_DTE_PA(*b_dte));
				c_tbl = mmuC2tmgr(c_dte);
				free_c_table(c_tbl, TRUE);
				b_dte->attr.raw = MMU_DT_INVALID;
				b_tbl->bt_ecnt--;
			}
			b_dte++;
			rstart += MMU_TIB_RANGE;
		}
	}
	if (nend < end) {
		idx = MMU_TIB(nend);
		b_dte = &b_tbl->bt_dtbl[idx];
		if (MMU_VALID_DT(*b_dte)) {
			c_dte = mmu_ptov(MMU_DTE_PA(*b_dte));
			c_tbl = mmuC2tmgr(c_dte);
			empty = pmap_remove_c(c_tbl, nend, end);
			if (empty) {
				b_dte->attr.raw = MMU_DT_INVALID;
				b_tbl->bt_ecnt--;
			}
		}
	}

	if (b_tbl->bt_ecnt == 0) {
		b_tbl->bt_parent = NULL;
		TAILQ_REMOVE(&b_pool, b_tbl, bt_link);
		TAILQ_INSERT_HEAD(&b_pool, b_tbl, bt_link);
		empty = TRUE;
	} else {
		empty = FALSE;
	}

	return empty;
}

/* pmap_remove_c			INTERNAL
 **
 * Remove a range of addresses from the given C table.
 */
boolean_t
pmap_remove_c(c_tbl, start, end)
	c_tmgr_t *c_tbl;
	vm_offset_t start;
	vm_offset_t end;
{
	boolean_t empty;
	int idx;
	mmu_short_pte_t *c_pte;
	
	idx = MMU_TIC(start);
	c_pte = &c_tbl->ct_dtbl[idx];
	for (;start < end; start += MMU_PAGE_SIZE, c_pte++) {
		if (MMU_VALID_DT(*c_pte)) {
			pmap_remove_pte(c_pte);
			c_tbl->ct_ecnt--;
		}
	}

	if (c_tbl->ct_ecnt == 0) {
		c_tbl->ct_parent = NULL;
		TAILQ_REMOVE(&c_pool, c_tbl, ct_link);
		TAILQ_INSERT_HEAD(&c_pool, c_tbl, ct_link);
		empty = TRUE;
	} else {
		empty = FALSE;
	}

	return empty;
}

/* is_managed				INTERNAL
 **
 * Determine if the given physical address is managed by the PV system.
 * Note that this logic assumes that no one will ask for the status of
 * addresses which lie in-between the memory banks on the 3/80.  If they
 * do so, it will falsely report that it is managed.
 *
 * Note: A "managed" address is one that was reported to the VM system as 
 * a "usable page" during system startup.  As such, the VM system expects the
 * pmap module to keep an accurate track of the useage of those pages.
 * Any page not given to the VM system at startup does not exist (as far as 
 * the VM system is concerned) and is therefore "unmanaged."  Examples are
 * those pages which belong to the ROM monitor and the memory allocated before
 * the VM system was started.
 */
boolean_t
is_managed(pa)
	vm_offset_t pa;
{
	if (pa >= avail_start && pa < avail_end)
		return TRUE;
	else
		return FALSE;
}

/* pmap_bootstrap_alloc			INTERNAL
 **
 * Used internally for memory allocation at startup when malloc is not
 * available.  This code will fail once it crosses the first memory
 * bank boundary on the 3/80.  Hopefully by then however, the VM system
 * will be in charge of allocation.
 */
void *
pmap_bootstrap_alloc(size)
	int size;
{
	void *rtn;

#ifdef	PMAP_DEBUG
	if (bootstrap_alloc_enabled == FALSE) {
		mon_printf("pmap_bootstrap_alloc: disabled\n");
		sunmon_abort();
	}
#endif

	rtn = (void *) virtual_avail;
	virtual_avail += size;

#ifdef	PMAP_DEBUG
	if (virtual_avail > virtual_contig_end) {
		mon_printf("pmap_bootstrap_alloc: out of mem\n");
		sunmon_abort();
	}
#endif

	return rtn;
}

/* pmap_bootstap_aalign			INTERNAL
 **
 * Used to insure that the next call to pmap_bootstrap_alloc() will
 * return a chunk of memory aligned to the specified size.
 *
 * Note: This function will only support alignment sizes that are powers
 * of two.
 */
void
pmap_bootstrap_aalign(size)
	int size;
{
	int off;

	off = virtual_avail & (size - 1);
	if (off) {
		(void) pmap_bootstrap_alloc(size - off);
	}
}

/* pmap_pa_exists
 **
 * Used by the /dev/mem driver to see if a given PA is memory
 * that can be mapped.  (The PA is not in a hole.)
 */
int
pmap_pa_exists(pa)
	vm_offset_t pa;
{
	/* XXX - NOTYET */
	return (0);
}

/* pmap_activate			INTERFACE
 **
 * This is called by locore.s:cpu_switch when we are switching to a
 * new process.  This should load the MMU context for the new proc.
 * XXX - Later, this should be done directly in locore.s
 */
void
pmap_activate(pmap)
pmap_t	pmap;
{
	u_long rootpa;

	/* Only do reload/flush if we have to. */
	rootpa = pmap->pm_a_phys;
	if (kernel_crp.rp_addr != rootpa) {
		DPRINT(("pmap_activate(%p)\n", pmap));
		kernel_crp.rp_addr = rootpa;
		loadcrp(&kernel_crp);
		TBIAU();
	}
}


/* pmap_update
 **
 * Apply any delayed changes scheduled for all pmaps immediately.
 *
 * No delayed operations are currently done in this pmap.
 */
void
pmap_update()
{
	/* not implemented. */
}

/*
 * Fill in the cpu_kcore header for dumpsys()
 * (See machdep.c)
 */
void
pmap_set_kcore_hdr(chdr_p)
	cpu_kcore_hdr_t *chdr_p;
{
	u_long spa, len;
	int i;

	chdr_p->ckh_contig_end = virtual_contig_end;
	chdr_p->ckh_kernCbase = (u_long) kernCbase;
	for (i = 0; i < NPHYS_RAM_SEGS; i++) {
		spa = avail_mem[i].pmem_start;
		spa = _trunc_page(spa);
		len = avail_mem[i].pmem_end - spa;
		len = _round_page(len);
		chdr_p->ram_segs[i].start = spa;
		chdr_p->ram_segs[i].size  = len;
	}
}


/* pmap_virtual_space			INTERFACE
 **
 * Return the current available range of virtual addresses in the
 * arguuments provided.  Only really called once.
 */
void
pmap_virtual_space(vstart, vend)
	vm_offset_t *vstart, *vend;
{
	*vstart = virtual_avail;
	*vend = virtual_end;
}

/* pmap_free_pages			INTERFACE
 **
 * Return the number of physical pages still available.
 *
 * This is probably going to be a mess, but it's only called
 * once and it's the only function left that I have to implement!
 */
u_int
pmap_free_pages()
{
	int i;
	u_int left;
	vm_offset_t avail;

	avail = avail_next;
	left = 0;
	i = 0;
	while (avail >= avail_mem[i].pmem_end) {
		if (avail_mem[i].pmem_next == NULL)
			return 0;
		i++;
	}
	while (i < NPHYS_RAM_SEGS) {
		if (avail < avail_mem[i].pmem_start) {
			/* Avail is inside a hole, march it
			 * up to the next bank.
			 */
			avail = avail_mem[i].pmem_start;
		}
		left += _btop(avail_mem[i].pmem_end - avail);
		if (avail_mem[i].pmem_next == NULL)
			break;
		i++;
	}

	return left;
}

/* pmap_page_index			INTERFACE
 **
 * Return the index of the given physical page in a list of useable
 * physical pages in the system.  Holes in physical memory may be counted
 * if so desired.  As long as pmap_free_pages() and pmap_page_index()
 * agree as to whether holes in memory do or do not count as valid pages,
 * it really doesn't matter.  However, if you like to save a little
 * memory, don't count holes as valid pages.  This is even more true when
 * the holes are large.
 *
 * We will not count holes as valid pages.  We can generate page indices
 * that conform to this by using the memory bank structures initialized
 * in pmap_alloc_pv().
 */
int
pmap_page_index(pa)
	vm_offset_t pa;
{
	struct pmap_physmem_struct *bank = avail_mem;

	/* Search for the memory bank with this page. */
	/* XXX - What if it is not physical memory? */
	while (pa > bank->pmem_end)
		bank = bank->pmem_next;
	pa -= bank->pmem_start;

	return (bank->pmem_pvbase + _btop(pa));
}

/* pmap_next_page			INTERFACE
 **
 * Place the physical address of the next available page in the
 * argument given.  Returns FALSE if there are no more pages left.
 *
 * This function must jump over any holes in physical memory.
 * Once this function is used, any use of pmap_bootstrap_alloc()
 * is a sin.  Sinners will be punished with erratic behavior.
 */
boolean_t
pmap_next_page(pa)
	vm_offset_t *pa;
{
	static struct pmap_physmem_struct *curbank = avail_mem;

	/* XXX - temporary ROM saving hack. */
	if (avail_next >= avail_end)
		return FALSE;

	if (avail_next >= curbank->pmem_end)
		if (curbank->pmem_next == NULL)
			return FALSE;
		else {
			curbank = curbank->pmem_next;
			avail_next = curbank->pmem_start;
		}

	*pa = avail_next;
	avail_next += NBPG;
	return TRUE;
}

/* pmap_count			INTERFACE
 **
 * Return the number of resident (valid) pages in the given pmap.
 *
 * Note:  If this function is handed the kernel map, it will report
 * that it has no mappings.  Hopefully the VM system won't ask for kernel
 * map statistics.
 */
segsz_t
pmap_count(pmap, type)
	pmap_t pmap;
	int    type;
{
	u_int     count;
	int       a_idx, b_idx;
	a_tmgr_t *a_tbl;
	b_tmgr_t *b_tbl;
	c_tmgr_t *c_tbl;

	/*
	 * If the pmap does not have its own A table manager, it has no
	 * valid entires.
	 */
	if (pmap->pm_a_tmgr == NULL)
		return 0;

	a_tbl = pmap->pm_a_tmgr;

	count = 0;
	for (a_idx = 0; a_idx < MMU_TIA(KERNBASE); a_idx++) {
	    if (MMU_VALID_DT(a_tbl->at_dtbl[a_idx])) {
	        b_tbl = mmuB2tmgr(mmu_ptov(a_tbl->at_dtbl[a_idx].addr.raw));
	        for (b_idx = 0; b_idx < MMU_B_TBL_SIZE; b_idx++) {
	            if (MMU_VALID_DT(b_tbl->bt_dtbl[b_idx])) {
	                c_tbl = mmuC2tmgr(
	                    mmu_ptov(MMU_DTE_PA(b_tbl->bt_dtbl[b_idx])));
	                if (type == 0)
	                    /*
	                     * A resident entry count has been requested.
	                     */
	                    count += c_tbl->ct_ecnt;
	                else
	                    /*
	                     * A wired entry count has been requested.
	                     */
	                    count += c_tbl->ct_wcnt;
	            }
	        }
	    }
	}

	return count;
}

/************************ SUN3 COMPATIBILITY ROUTINES ********************
 * The following routines are only used by DDB for tricky kernel text    *
 * text operations in db_memrw.c.  They are provided for sun3            *
 * compatibility.                                                        *
 *************************************************************************/
/* get_pte			INTERNAL
 **
 * Return the page descriptor the describes the kernel mapping
 * of the given virtual address.
 */
extern u_long ptest_addr __P((u_long));	/* XXX: locore.s */
u_long
get_pte(va)
	vm_offset_t va;
{
	u_long pte_pa;
	mmu_short_pte_t *pte;

	/* Get the physical address of the PTE */
	pte_pa = ptest_addr(va & ~PGOFSET);

	/* Convert to a virtual address... */
	pte = (mmu_short_pte_t *) (KERNBASE + pte_pa);

	/* Make sure it is in our level-C tables... */
	if ((pte < kernCbase) ||
		(pte >= &mmuCbase[NUM_USER_PTES]))
		return 0;

	/* ... and just return its contents. */
	return (pte->attr.raw);
}


/* set_pte			INTERNAL
 **
 * Set the page descriptor that describes the kernel mapping
 * of the given virtual address.
 */
void
set_pte(va, pte)
	vm_offset_t va;
	vm_offset_t pte;
{
	u_long idx;

	if (va < KERNBASE)
		return;

	idx = (unsigned long) _btop(va - KERNBASE);
	kernCbase[idx].attr.raw = pte;
}

#ifdef	PMAP_DEBUG
/************************** DEBUGGING ROUTINES **************************
 * The following routines are meant to be an aid to debugging the pmap  *
 * system.  They are callable from the DDB command line and should be   *
 * prepared to be handed unstable or incomplete states of the system.   *
 ************************************************************************/

/* pv_list
 **
 * List all pages found on the pv list for the given physical page.
 * To avoid endless loops, the listing will stop at the end of the list
 * or after 'n' entries - whichever comes first.
 */
void
pv_list(pa, n)
	vm_offset_t pa;
	int n;
{
	int  idx;
	vm_offset_t va;
	pv_t *pv;
	c_tmgr_t *c_tbl;
	pmap_t pmap;
	
	pv = pa2pv(pa);
	idx = pv->pv_idx;

	for (;idx != PVE_EOL && n > 0; idx=pvebase[idx].pve_next, n--) {
		va = pmap_get_pteinfo(idx, &pmap, &c_tbl);
		printf("idx %d, pmap 0x%x, va 0x%x, c_tbl %x\n",
			idx, (u_int) pmap, (u_int) va, (u_int) c_tbl);
	}
}
#endif	/* PMAP_DEBUG */

#ifdef NOT_YET
/* and maybe not ever */
/************************** LOW-LEVEL ROUTINES **************************
 * These routines will eventualy be re-written into assembly and placed *
 * in locore.s.  They are here now as stubs so that the pmap module can *
 * be linked as a standalone user program for testing.                  *
 ************************************************************************/
/* flush_atc_crp			INTERNAL
 **
 * Flush all page descriptors derived from the given CPU Root Pointer
 * (CRP), or 'A' table as it is known here, from the 68851's automatic
 * cache.
 */
void
flush_atc_crp(a_tbl)
{
	mmu_long_rp_t rp;

	/* Create a temporary root table pointer that points to the
	 * given A table.
	 */
	rp.attr.raw = ~MMU_LONG_RP_LU;
	rp.addr.raw = (unsigned int) a_tbl;

	mmu_pflushr(&rp);
	/* mmu_pflushr:
	 * 	movel   sp(4)@,a0
	 * 	pflushr a0@
	 *	rts
	 */
}
#endif /* NOT_YET */
