/*	$NetBSD: pmap.c,v 1.1.1.1 1997/01/14 20:57:08 gwr Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
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
 * alias and a 22 entry cache.  So sadly (or happily), the previous note
 * does not apply to the sun3x pmap.
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
#include <machine/pmap.h>
#include <machine/pte.h>
#include <machine/mon.h>

#include "machdep.h"
#include "pmap_pvt.h"

/* XXX - What headers declare these? */
extern struct pcb *curpcb;
extern int physmem;

/* Defined in locore.s */
extern char kernel_text[];

/* Defined by the linker */
extern char etext[], edata[], end[];
extern char *esym;	/* DDB */

/*
 * I think it might be cleaner to have one of these in each of
 * the a_tmgr_t structures, but it's late at night... -gwr
 *
 */
struct rootptr {
	u_long limit; /* and type */
	u_long paddr;
};
struct rootptr proc0crp;

/* This is set by locore.s with the monitor's root ptr. */
extern struct rootptr mon_crp;

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
 * mmuAbase  -> +-------------------------------------------------------+
 *              |                                                       |
 *              | User       MMU A level tables                         |
 *              |                                                       |
 * mmuBbase  -> +-------------------------------------------------------+
 *              | User       MMU B level tables                         |
 * mmuCbase  -> +-------------------------------------------------------+
 *              | User       MMU C level tables                         |
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
/* Global variables for storing the base addresses for the areas
 * labeled above.
 */
static mmu_long_dte_t	*kernAbase;
static mmu_short_dte_t	*kernBbase;
static mmu_short_pte_t	*kernCbase;
static mmu_long_dte_t	*mmuAbase;
static mmu_short_dte_t	*mmuBbase;
static mmu_short_pte_t	*mmuCbase;
static a_tmgr_t		*Atmgrbase;
static b_tmgr_t		*Btmgrbase;
static c_tmgr_t		*Ctmgrbase;
static pv_t		*pvbase;
static pv_elem_t	*pvebase;

/* Just all around global variables.
 */
static TAILQ_HEAD(a_pool_head_struct, a_tmgr_struct) a_pool;
static TAILQ_HEAD(b_pool_head_struct, b_tmgr_struct) b_pool;
static TAILQ_HEAD(c_pool_head_struct, c_tmgr_struct) c_pool;
       struct pmap	kernel_pmap;
static a_tmgr_t		*proc0Atmgr;
       a_tmgr_t		*curatbl;
static boolean_t	pv_initialized = 0;
static vm_offset_t	last_mapped = 0;
       int		tmp_vpages_inuse = 0;

/*
 * XXX:  For now, retain the traditional variables that were
 * used in the old pmap/vm interface (without NONCONTIG).
 */
/* Kernel virtual address space available: */
vm_offset_t	virtual_avail, virtual_end;
/* Physical address space available: */
vm_offset_t	avail_start, avail_end;

vm_offset_t tmp_vpages[2];


/* The 3/80 is the only member of the sun3x family that has non-contiguous
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
struct pmap_physmem_struct avail_mem[SUN3X_80_MEM_BANKS];
u_int total_phys_mem;

/* These macros map MMU tables to their corresponding manager structures.
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
#define mmuA2tmgr(table) \
	(&Atmgrbase[\
		((mmu_long_dte_t *)(table) - mmuAbase)\
		/ MMU_A_TBL_SIZE\
	])
#define mmuB2tmgr(table) \
	(&Btmgrbase[\
		((mmu_short_dte_t *)(table) - mmuBbase)\
		/ MMU_B_TBL_SIZE\
        ])
#define mmuC2tmgr(table) \
	(&Ctmgrbase[\
		((mmu_short_pte_t *)(table) - mmuCbase)\
		/ MMU_C_TBL_SIZE\
	])
#define pte2pve(pte) \
	(&pvebase[\
		((mmu_short_pte_t *)(pte) - mmuCbase)\
	])
/* I don't think this is actually used.
 * #define pte2pv(pte) \
 *	(pa2pv(\
 *		(pte)->attr.raw & MMU_SHORT_PTE_BASEADDR\
 *	))
 */
/* This is now a function call
 * #define pa2pv(pa) \
 *	(&pvbase[(unsigned long)\
 *		sun3x_btop(pa)\
 *	])
 */
#define pve2pte(pve) \
	(&mmuCbase[(unsigned long)\
		(((pv_elem_t *)(pve)) - pvebase)\
		/ sizeof(mmu_short_pte_t)\
	])

/*************************** TEMPORARY STATMENTS *************************
 * These statements will disappear once this code is integrated into the *
 * system.  They are here only to make the code `stand alone'.           *
 *************************************************************************/
#define mmu_ptov(pa) ((unsigned long) KERNBASE + (unsigned long) (pa))
#define mmu_vtop(va) ((unsigned long) (va) - (unsigned long) KERNBASE)
#define NULL 0

#define NUM_A_TABLES	20
#define NUM_B_TABLES	60
#define NUM_C_TABLES	60

/*************************** MISCELANEOUS MACROS *************************/
#define PMAP_LOCK()	;	/* Nothing, for now */
#define PMAP_UNLOCK()	;	/* same. */
/*************************** FUNCTION DEFINITIONS ************************
 * These appear here merely for the compiler to enforce type checking on *
 * all function calls.                                                   *
 *************************************************************************
 */

/** External functions
 ** - functions used within this module but written elsewhere.
 **   both of these functions are in locore.s
 */
void   mmu_seturp __P((vm_offset_t));
void   mmu_flush __P((int, vm_offset_t));
void   mmu_flusha __P((void));

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
void   pmap_activate __P((pmap_t, struct pcb *));
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
 * Initializes the pmap system.  Called at boot time from sun3x_vm_init()
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

	/* XXX - Attention: moved stuff. */

	/*
	 * Determine the range of kernel virtual space available.
	 */
	virtual_avail = sun3x_round_page(nextva);
	virtual_end = VM_MAX_KERNEL_ADDRESS;

	/*
	 * Determine the range of physical memory available and
	 * relay this information to the pmap via the avail_mem[]
	 * array of physical memory segment structures.
	 *
	 * Avail_end is set to the first byte of physical memory
	 * outside the last bank.
	 */
	avail_start = virtual_avail - KERNBASE;

	/*
	 * This is a somewhat unwrapped loop to deal with
	 * copying the PROM's 'phsymem' banks into the pmap's
	 * banks.  The following is always assumed:
	 * 1. There is always at least one bank of memory.
	 * 2. There is always a last bank of memory, and its
	 *    pmem_next member must be set to NULL.
	 * XXX - Use: do { ... } while (membank->next) instead?
	 * XXX - Why copy this stuff at all? -gwr
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
	avail_end = pmap_membank->pmem_end;
	avail_end = sun3x_trunc_page(avail_end);

	/* XXX - End moved stuff. */

	/*
	 * The first step is to allocate MMU tables.
	 * Note: All must be aligned on 256 byte boundaries.
	 *
	 * Start with the top level, or 'A' table.
	 */
	kernAbase = (mmu_long_dte_t *) virtual_avail;
	size = sizeof(mmu_long_dte_t) * MMU_A_TBL_SIZE;
	bzero(kernAbase, size);
	avail_start += size;
	virtual_avail += size;

	/* Allocate enough B tables to map from KERNBASE to
	 * the end of VM.
	 */
	kernBbase = (mmu_short_dte_t *) virtual_avail;
	size = sizeof(mmu_short_dte_t) *
		(MMU_A_TBL_SIZE - MMU_TIA(KERNBASE)) * MMU_B_TBL_SIZE;
	bzero(kernBbase, size);
	avail_start += size;
	virtual_avail += size;

	/* Allocate enough C tables. */
	kernCbase = (mmu_short_pte_t *) virtual_avail;
	size = sizeof (mmu_short_pte_t) *
		(MMU_A_TBL_SIZE - MMU_TIA(KERNBASE))
		* MMU_B_TBL_SIZE * MMU_C_TBL_SIZE;
	bzero(kernCbase, size);
	avail_start += size;
	virtual_avail += size;

	/* For simplicity, the kernel's mappings will be editable as a
	 * flat array of page table entries at kernCbase.  The
	 * higher level 'A' and 'B' tables must be initialized to point
	 * to this lower one. 
	 */
	b = c = 0;

	/* Invalidate all mappings below KERNBASE in the A table.
	 * This area has already been zeroed out, but it is good
	 * practice to explicitly show that we are interpreting
	 * it as a list of A table descriptors.
	 */
	for (i = 0; i < MMU_TIA(KERNBASE); i++) {
		kernAbase[i].addr.raw = 0;
	}

	/* Set up the kernel A and B tables so that they will reference the
	 * correct spots in the contiguous table of PTEs allocated for the
	 * kernel's virtual memory space.
	 */
	for (i = MMU_TIA(KERNBASE); i < MMU_A_TBL_SIZE; i++) {
		kernAbase[i].attr.raw =
			MMU_LONG_DTE_LU | MMU_LONG_DTE_SUPV | MMU_DT_SHORT;
		kernAbase[i].addr.raw = (unsigned long) mmu_vtop(&kernBbase[b]);

		for (j=0; j < MMU_B_TBL_SIZE; j++) {
			kernBbase[b + j].attr.raw =
				(unsigned long) mmu_vtop(&kernCbase[c])
				| MMU_DT_SHORT;
			c += MMU_C_TBL_SIZE;
		}
		b += MMU_B_TBL_SIZE;
	}

	/*
	 * Now pmap_enter_kernel() may be used safely and will be
	 * the main interface used by _startup.c and other various
	 * modules to modify kernel mappings.
	 *
	 * Note: Our tables will NOT have the default linear mappings!
	 */
	va = (vm_offset_t) KERNBASE;
	pa = mmu_vtop(KERNBASE);

	/*
	 * The first page is the msgbuf page (data, non-cached).
	 * Just fixup the mapping here; setup is in cpu_startup().
	 * XXX - Make it non-cached?
	 */
	pmap_enter_kernel(va, pa|PMAP_NC, VM_PROT_ALL);
	va += NBPG; pa += NBPG;

	/* The tmporary stack page. */
	pmap_enter_kernel(va, pa, VM_PROT_ALL);
	va += NBPG; pa += NBPG;

	/*
	 * Map all of the kernel's text segment as read-only and cacheable.
	 * (Cacheable is implied by default).  Unfortunately, the last bytes
	 * of kernel text and the first bytes of kernel data will often be
	 * sharing the same page.  Therefore, the last page of kernel text
	 * has to be mapped as read/write, to accomodate the data.
	 */
	eva = sun3x_trunc_page((vm_offset_t)etext);
	for (; va < eva; pa += NBPG, va += NBPG)
		pmap_enter_kernel(va, pa, VM_PROT_READ|VM_PROT_EXECUTE);

	/* Map all of the kernel's data (including BSS) segment as read/write
	 * and cacheable.
	 */
	for (; va < (vm_offset_t) esym; pa += NBPG, va += NBPG)
		pmap_enter_kernel(va, pa, VM_PROT_READ|VM_PROT_WRITE);

	/* Map all of the data we have allocated since the start of this
	 * function.
	 */
	for (; va < virtual_avail; va += NBPG, pa += NBPG)
		pmap_enter_kernel(va, pa, VM_PROT_READ|VM_PROT_WRITE);

	/* Set 'last_mapped' to the address of the last physical page
	 * that was mapped in the kernel.  This variable is used by
	 * pmap_bootstrap_alloc() to determine when it needs to map
	 * a new page.
	 *
	 * XXX - This can be a lot simpler.  We already know that the
	 * first 4MB of memory (at least) is mapped PA=VA-KERNBASE,
	 * so we should never need to creat any new mappings. -gwr
	 *
	 * True, but it only remains so as long as we are using the
	 * ROM's CRP.  Unless, of course, we copy these mappings into
	 * our table. -j
	 */
	last_mapped = sun3x_trunc_page(pa - (NBPG - 1));

	/* It is now safe to use pmap_bootstrap_alloc(). */

	pmap_alloc_usermmu();	/* Allocate user MMU tables.        */
	pmap_alloc_usertmgr();	/* Allocate user MMU table managers.*/
	pmap_alloc_pv();	/* Allocate physical->virtual map.  */
	pmap_alloc_etc();	/* Allocate miscelaneous things.    */

	/* Notify the VM system of our page size. */
	PAGE_SIZE = NBPG;
	vm_set_page_size();

	/* XXX - Attention: moved stuff. */

	/*
	 * XXX - Make sure avail_start is within the low 4M range
	 * that the Sun PROM guarantees will be mapped in?
	 * Make sure it is below avail_end as well?
	 */

	/*
	 * Now steal some virtual addresses, but
	 * not the physical pages behind them.
	 */

	/*
	 * vpages array:  just some virtual addresses for
	 * temporary mappings in the pmap module (two pages)
	 */
	pmap_bootstrap_aalign(NBPG);
	tmp_vpages[0] = virtual_avail;
	virtual_avail += NBPG;
	tmp_vpages[1] = virtual_avail;
	virtual_avail += NBPG;

	/* XXX - End moved stuff. */

	/* It should be noted that none of these mappings take
	 * effect until the MMU's root pointer is
	 * is changed from the PROM map, to our own. 
	 */
	pmap_bootstrap_copyprom();
	pmap_takeover_mmu();
}


/* pmap_alloc_usermmu			INTERNAL
 **
 * Called from pmap_bootstrap() to allocate MMU tables that will
 * eventually be used for user mappings.
 */
void
pmap_alloc_usermmu()
{
	/* Allocate user MMU tables. 
	 * These must be aligned on 256 byte boundaries.
	 */
	pmap_bootstrap_aalign(256);
	mmuAbase = (mmu_long_dte_t *)
		pmap_bootstrap_alloc(sizeof(mmu_long_dte_t)
		* MMU_A_TBL_SIZE
		* NUM_A_TABLES);
	mmuBbase = (mmu_short_dte_t *)
		pmap_bootstrap_alloc(sizeof(mmu_short_dte_t)
		* MMU_B_TBL_SIZE
		* NUM_B_TABLES);
	mmuCbase = (mmu_short_pte_t *)
		pmap_bootstrap_alloc(sizeof(mmu_short_pte_t)
		* MMU_C_TBL_SIZE
		* NUM_C_TABLES);
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

	/* Allocate a pv_head structure for every page of physical
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
	for (i = 0; i < SUN3X_80_MEM_BANKS; i++) {
		avail_mem[i].pmem_pvbase = sun3x_btop(total_mem);
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
		sun3x_btop(total_phys_mem));
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
	/* XXX - It would be a lot simpler to just make these BSS. -gwr */
	Atmgrbase = (a_tmgr_t *) pmap_bootstrap_alloc(sizeof(a_tmgr_t)
		* NUM_A_TABLES);
	Btmgrbase = (b_tmgr_t *) pmap_bootstrap_alloc(sizeof(b_tmgr_t)
		* NUM_B_TABLES);
	Ctmgrbase = (c_tmgr_t *) pmap_bootstrap_alloc(sizeof(c_tmgr_t)
		* NUM_C_TABLES);

	/* Allocate PV list elements for the physical to virtual
	 * mapping system.
	 */
	pvebase = (pv_elem_t *) pmap_bootstrap_alloc(
		sizeof(struct pv_elem_struct)
		* MMU_C_TBL_SIZE
		* NUM_C_TABLES );
}

/* pmap_alloc_etc			INTERNAL
 **
 * Called from pmap_bootstrap() to allocate any remaining pieces
 * that didn't fit neatly into any of the other pmap_alloc
 * functions.
 */
void
pmap_alloc_etc()
{
	/* Allocate an A table manager for the kernel_pmap */
	proc0Atmgr = (a_tmgr_t *) pmap_bootstrap_alloc(sizeof(a_tmgr_t));
}

/* pmap_bootstrap_copyprom()			INTERNAL
 **
 * Copy the PROM mappings into our own tables.  Note, we
 * can use physical addresses until __bootstrap returns.
 */
void
pmap_bootstrap_copyprom()
{
#if 0	/* XXX - Just history... */
	/*
	 * XXX - This method makes DVMA difficult, because
	 * the PROM only provides PTEs for 1M of DVMA space.
	 * That's OK for boot programs, but not a VM system.
	 */
	int a_idx;
	mmu_long_dte_t *mon_atbl;

	/*
	 * Copy the last entry (PROM monitor and DVMA mappings)
	 * so our level-A table will use the PROM level-B table.
	 */
	mon_atbl = (mmu_long_dte_t *) mon_crp.paddr;
	a_idx = MMU_TIA(VM_MAX_KERNEL_ADDRESS);
	kernAbase[a_idx].attr.raw = mon_atbl[a_idx].attr.raw;
	kernAbase[a_idx].addr.raw = mon_atbl[a_idx].addr.raw;

#endif
#if 0	/* XXX - More history... */
	/*
	 * XXX - This method is OK with our DVMA implementation,
	 * but causes pmap_extract() to be ignorant of the PROM
	 * mappings.  Maybe that's OK.  If not, we should just
	 * copy the PTEs (level-C) from the PROM. -gwr
	 */
	mmu_long_dte_t *mon_atbl;
	mmu_short_dte_t *mon_btbl;
	mmu_short_dte_t *our_btbl;
	int a_idx, b_idx;

	/*
	 * Copy parts of the level-B table from the PROM for
	 * mappings that the PROM cares about.
	 */
	mon_atbl = (mmu_long_dte_t *) mon_crp.paddr;
	a_idx = MMU_TIA(MON_KDB_START);
	mon_btbl = (mmu_short_dte_t *) mon_atbl[a_idx].addr.raw;

	/* Temporary use of b_idx to find our level-B table. */
	b_idx = MMU_B_TBL_SIZE * (a_idx - MMU_TIA(KERNBASE));
	our_btbl = &kernBbase[b_idx];

	/*
	 * Preserve both the kadb and monitor mappings (2MB).
	 * We could have started at MONSTART, but this costs
	 * us nothing, and might be useful someday...
	 */
	for (b_idx = MMU_TIB(MON_KDB_START);
		 b_idx < MMU_TIB(MONEND); b_idx++)
		our_btbl[b_idx].attr.raw = mon_btbl[b_idx].attr.raw;

	/*
	 * Preserve the monitor's DVMA map for now (1MB).
	 * Later, we might want to kill this mapping so we
	 * can have all of the DVMA space (16MB).
	 */
	for (b_idx = MMU_TIB(MON_DVMA_BASE);
		 b_idx < MMU_B_TBL_SIZE; b_idx++)
		our_btbl[b_idx].attr.raw = mon_btbl[b_idx].attr.raw;
#endif
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
	i = sun3x_btop(MON_KDB_START - KERNBASE);
	kpte = &kernCbase[i];
	len = sun3x_btop(MONEND - MON_KDB_START);

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
	i = sun3x_btop(MON_DVMA_BASE - KERNBASE);
	kpte = &kernCbase[i];
	len = sun3x_btop(MON_DVMA_SIZE);

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
	vm_offset_t tbladdr;

	tbladdr = mmu_vtop((vm_offset_t) kernAbase);
	mon_printf("pmap_takeover_mmu: tbladdr=0x%x\n", tbladdr);

	/* Initialize the CPU Root Pointer (CRP) for proc0. */
	/* XXX: I'd prefer per-process CRP storage. -gwr */
	proc0crp.limit = 0x80000003;	/* limit and type */
	proc0crp.paddr = tbladdr;	/* phys. addr. */
	curpcb->pcb_mmuctx = (int) &proc0crp;

	mon_printf("pmap_takeover_mmu: loadcrp...\n");
	loadcrp((vm_offset_t) &proc0crp);
	mon_printf("pmap_takeover_mmu: survived!\n");
}

/* pmap_init			INTERFACE
 **
 * Called at the end of vm_init() to set up the pmap system to go
 * into full time operation.
 */
void
pmap_init()
{
	/** Initialize the manager pools **/
	TAILQ_INIT(&a_pool);
	TAILQ_INIT(&b_pool);
	TAILQ_INIT(&c_pool);

	/** Initialize the PV system **/
	pmap_init_pv();

	/** Zero out the kernel's pmap **/
	bzero(&kernel_pmap, sizeof(struct pmap));

	/* Initialize the A table manager that is used in pmaps which
	 * do not have an A table of their own.  This table uses the
	 * kernel, or 'proc0' level A MMU table, which contains no valid
	 * user space mappings.  Any user process that attempts to execute
	 * using this A table will fault.  At which point the VM system will
	 * call pmap_enter, which will then allocate it an A table of its own
	 * from the pool.
	 */
	proc0Atmgr->at_dtbl = kernAbase;
	proc0Atmgr->at_parent = &kernel_pmap;
	kernel_pmap.pm_a_tbl = proc0Atmgr;

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

		/* Clear its parent entry.  Set its wired and valid
		 * entry count to zero.
		 */
		a_tbl->at_parent = NULL;
		a_tbl->at_wcnt = a_tbl->at_ecnt = 0;

		/* Assign it the next available MMU A table from the pool */
		a_tbl->at_dtbl = &mmuAbase[i * MMU_A_TBL_SIZE];

		/* Initialize the MMU A table with the table in the `proc0',
		 * or kernel, mapping.  This ensures that every process has
		 * the kernel mapped in the top part of its address space.
		 */
		bcopy(kernAbase, a_tbl->at_dtbl, MMU_A_TBL_SIZE * 
			sizeof(mmu_long_dte_t));

		/* Finally, insert the manager into the A pool,
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
	bzero(pvbase, sizeof(pv_t) * sun3x_btop(total_phys_mem));
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

	/* Get the top A table in the pool */
	tbl = a_pool.tqh_first;
	if (tbl == NULL)
		panic("get_a_table: out of A tables.");
	TAILQ_REMOVE(&a_pool, tbl, at_link);
	/* If the table has a non-null parent pointer then it is in use.
	 * Forcibly abduct it from its parent and clear its entries.
	 * No re-entrancy worries here.  This table would not be in the
	 * table pool unless it was available for use.
	 */
	if (tbl->at_parent) {
		tbl->at_parent->pm_stats.resident_count -= free_a_table(tbl);
		tbl->at_parent->pm_a_tbl = proc0Atmgr;
	}
#ifdef  NON_REENTRANT
	/* If the table isn't to be wired down, re-insert it at the
	 * end of the pool.
	 */
	if (!wired)
		/* Quandary - XXX
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
		tbl->bt_parent->at_parent->pm_stats.resident_count -=
		    free_b_table(tbl);
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
		tbl->ct_parent->bt_parent->at_parent->pm_stats.resident_count
		    -= free_c_table(tbl);
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

/* The following 'free_table' and 'steal_table' functions are called to
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
free_a_table(a_tbl)
	a_tmgr_t *a_tbl;
{
	int i, removed_cnt;
	mmu_long_dte_t	*dte;
	mmu_short_dte_t *dtbl;
	b_tmgr_t	*tmgr;

	/* Flush the ATC cache of all cached descriptors derived
	 * from this table.
	 * XXX - Sun3x does not use 68851's cached table feature
	 * flush_atc_crp(mmu_vtop(a_tbl->dte));
	 */

	/* Remove any pending cache flushes that were designated
	 * for the pmap this A table belongs to.
	 * a_tbl->parent->atc_flushq[0] = 0;
	 * XXX - Not implemented in sun3x.
	 */

	/* All A tables in the system should retain a map for the
	 * kernel. If the table contains any valid descriptors
	 * (other than those for the kernel area), invalidate them all,
	 * stopping short of the kernel's entries.
	 */
	removed_cnt = 0;
	if (a_tbl->at_ecnt) {
		dte = a_tbl->at_dtbl;
		for (i=0; i < MMU_TIA(KERNBASE); i++)
			/* If a table entry points to a valid B table, free
			 * it and its children.
			 */
			if (MMU_VALID_DT(dte[i])) {
				/* The following block does several things,
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
				dtbl = (mmu_short_dte_t *) MMU_DTE_PA(dte[i]);
				dtbl = (mmu_short_dte_t *) mmu_ptov(dtbl);
				tmgr = mmuB2tmgr(dtbl);
				removed_cnt += free_b_table(tmgr);
			}
	}
	a_tbl->at_ecnt = 0;
	return removed_cnt;
}

/* free_b_table			INTERNAL
 **
 * Unmaps the given B table and all its children from their current
 * mappings.  Returns the number of pages that were invalidated.
 * (For comments, see 'free_a_table()').
 */
int
free_b_table(b_tbl)
	b_tmgr_t *b_tbl;
{
	int i, removed_cnt;
	mmu_short_dte_t *dte;
	mmu_short_pte_t	*dtbl;
	c_tmgr_t	*tmgr;

	removed_cnt = 0;
	if (b_tbl->bt_ecnt) {
		dte = b_tbl->bt_dtbl;
		for (i=0; i < MMU_B_TBL_SIZE; i++)
			if (MMU_VALID_DT(dte[i])) {
				dtbl = (mmu_short_pte_t *) MMU_DTE_PA(dte[i]);
				dtbl = (mmu_short_pte_t *) mmu_ptov(dtbl);
				tmgr = mmuC2tmgr(dtbl);
				removed_cnt += free_c_table(tmgr);
			}
	}

	b_tbl->bt_ecnt = 0;
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
free_c_table(c_tbl)
	c_tmgr_t *c_tbl;
{
	int i, removed_cnt;

	removed_cnt = 0;
	if (c_tbl->ct_ecnt)
		for (i=0; i < MMU_C_TBL_SIZE; i++)
			if (MMU_VALID_DT(c_tbl->ct_dtbl[i])) {
				pmap_remove_pte(&c_tbl->ct_dtbl[i]);
				removed_cnt++;
			}
	c_tbl->ct_ecnt = 0;
	return removed_cnt;
}

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
}

/* pmap_remove_pte			INTERNAL
 **
 * Unmap the given pte and preserve any page modification
 * information by transfering it to the pv head of the
 * physical page it maps to.  This function does not update
 * any reference counts because it is assumed that the calling
 * function will do so.  If the calling function does not have the
 * ability to do so, the function pmap_dereference_pte() exists
 * for this purpose.
 */
void
pmap_remove_pte(pte)
	mmu_short_pte_t *pte;
{
	vm_offset_t pa;
	pv_t       *pv;
	pv_elem_t  *pve;

	pa = MMU_PTE_PA(*pte);
	if (is_managed(pa)) {
		pv = pa2pv(pa);
		/* Save the mod/ref bits of the pte by simply
		 * ORing the entire pte onto the pv_flags member
		 * of the pv structure.
		 * There is no need to use a separate bit pattern
		 * for usage information on the pv head than that
		 * which is used on the MMU ptes.
		 */
		pv->pv_flags |= pte->attr.raw;

		pve = pte2pve(pte);
		if (pve == pv->pv_head.lh_first)
			pv->pv_head.lh_first = pve->pve_link.le_next;
		LIST_REMOVE(pve, pve_link);
	}

	pte->attr.raw = MMU_DT_INVALID;
}

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
	c_tmgr_t *c_tbl;

	c_tbl = pmap_find_c_tmgr(pte);
	c_tbl->ct_parent->bt_parent->at_parent->pm_stats.resident_count--;
	if (--c_tbl->ct_ecnt == 0)
		free_c_table_novalid(c_tbl);
}

/* pmap_stroll			INTERNAL
 **
 * Retrieve the addresses of all table managers involved in the mapping of
 * the given virtual address.  If the table walk completed sucessfully,
 * return TRUE.  If it was only partial sucessful, return FALSE.
 * The table walk performed by this function is important to many other
 * functions in this module.
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

	/* Does the given pmap have an A table? */
	*a_tbl = pmap->pm_a_tbl;
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
	*b_tbl = pmap_find_b_tmgr(
		  (mmu_short_dte_t *) mmu_ptov(
		    MMU_DTE_PA(*a_dte)
		  )
		);
	/* Does the B table have a valid C table
	 * under the corresponding table entry?
	 */
	*b_idx = MMU_TIB(va);
	b_dte = &((*b_tbl)->bt_dtbl[*b_idx]);
	if (!MMU_VALID_DT(*b_dte))
		return FALSE; /* No. Return unknown. */
	/* Yes. Extract C table from the B table. */
	*c_tbl = pmap_find_c_tmgr(
		  (mmu_short_pte_t *) mmu_ptov(
		    MMU_DTE_PA(*b_dte)
		  )
		);
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
 */
void
pmap_enter(pmap, va, pa, prot, wired)
	pmap_t	pmap;
	vm_offset_t va;
	vm_offset_t pa;
	vm_prot_t prot;
	boolean_t wired;
{
	u_int a_idx, b_idx, pte_idx; /* table indexes (fix grammar) */
	a_tmgr_t *a_tbl;         /* A: long descriptor table manager  */
	b_tmgr_t *b_tbl;         /* B: short descriptor table manager */
	c_tmgr_t *c_tbl;         /* C: short page table manager       */
	mmu_long_dte_t *a_dte;   /* A: long descriptor table          */
	mmu_short_dte_t *b_dte;  /* B: short descriptor table         */
	mmu_short_pte_t *c_pte;  /* C: short page descriptor table    */
	pv_t      *pv;           /* pv list head                      */
	pv_elem_t *pve;          /* pv element                        */
	enum {NONE, NEWA, NEWB, NEWC} llevel; /* used at end   */

	if (pmap == NULL)
		return;
	if (pmap == pmap_kernel()) {
		pmap_enter_kernel(va, pa, prot);
		return;
	}
		
	/* For user mappings we walk along the MMU tables of the given
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

	/* Step 1 - Retrieve the A table from the pmap.  If it is the default
	 * A table (commonly known as the 'proc0' A table), allocate a new one.
	 */

	a_tbl = pmap->pm_a_tbl;
	if (a_tbl == proc0Atmgr) {
		pmap->pm_a_tbl = a_tbl = get_a_table();
		if (!wired)
			llevel = NEWA;
	} else {
		/* Use the A table already allocated for this pmap.
		 * Unlink it from the A table pool if necessary.
		 */
		if (wired && !a_tbl->at_wcnt)
			TAILQ_REMOVE(&a_pool, a_tbl, at_link);
	}

	/* Step 2 - Walk into the B table.  If there is no valid B table,
	 * allocate one.
	 */

	a_idx = MMU_TIA(va);            /* Calculate the TIA of the VA. */
	a_dte = &a_tbl->at_dtbl[a_idx]; /* Retrieve descriptor from table */
	if (MMU_VALID_DT(*a_dte)) {     /* Is the descriptor valid? */
		/* Yes, it points to a valid B table.  Use it. */
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
		b_dte = (mmu_short_dte_t *) mmu_ptov(a_dte->addr.raw);
		b_tbl = mmuB2tmgr(b_dte);
		if (wired && !b_tbl->bt_wcnt) {
			/* If mapping is wired and table is not */
			TAILQ_REMOVE(&b_pool, b_tbl, bt_link);
			a_tbl->at_wcnt++; /* Update parent table's wired
			                   * entry count. */
		}
	} else {
		b_tbl = get_b_table(); /* No, need to allocate a new B table */
		/* Point the parent A table descriptor to this new B table. */
		a_dte->addr.raw = (unsigned long) mmu_vtop(b_tbl->bt_dtbl);
		a_dte->attr.attr_struct.dt = MMU_DT_SHORT;
		/* Create the necessary back references to the parent table */
		b_tbl->bt_parent = a_tbl;
		b_tbl->bt_pidx = a_idx;
		/* If this table is to be wired, make sure the parent A table
		 * wired count is updated to reflect that it has another wired
		 * entry.
		 */
		a_tbl->at_ecnt++; /* Update parent's valid entry count */
		if (wired)
			a_tbl->at_wcnt++;
		else if (llevel == NONE)
			llevel = NEWB;
	}

	/* Step 3 - Walk into the C table, if there is no valid C table,
	 * allocate one.
	 */

	b_idx = MMU_TIB(va);            /* Calculate the TIB of the VA */
	b_dte = &b_tbl->bt_dtbl[b_idx]; /* Retrieve descriptor from table */
	if (MMU_VALID_DT(*b_dte)) {     /* Is the descriptor valid? */
		/* Yes, it points to a valid C table.  Use it. */
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
		c_pte = (mmu_short_pte_t *) MMU_PTE_PA(*b_dte);
		c_pte = (mmu_short_pte_t *) mmu_ptov(c_pte);
		c_tbl = mmuC2tmgr(c_pte);
		if (wired && !c_tbl->ct_wcnt) {
			/* If mapping is wired and table is not */
			TAILQ_REMOVE(&c_pool, c_tbl, ct_link);
			b_tbl->bt_wcnt++;
		}
	} else {
		c_tbl = get_c_table(); /* No, need to allocate a new C table */
		/* Point the parent B table descriptor to this new C table. */
		b_dte->attr.raw = (unsigned long) mmu_vtop(c_tbl->ct_dtbl);
		b_dte->attr.attr_struct.dt = MMU_DT_SHORT;
		/* Create the necessary back references to the parent table */
		c_tbl->ct_parent = b_tbl;
		c_tbl->ct_pidx = b_idx;
		/* If this table is to be wired, make sure the parent B table
		 * wired count is updated to reflect that it has another wired
		 * entry.
		 */
		b_tbl->bt_ecnt++; /* Update parent's valid entry count */
		if (wired)
			b_tbl->bt_wcnt++;
		else if (llevel == NONE)
			llevel = NEWC;
	}

	/* Step 4 - Deposit a page descriptor (PTE) into the appropriate
	 * slot of the C table, describing the PA to which the VA is mapped.
	 */

	pte_idx = MMU_TIC(va);
	c_pte = &c_tbl->ct_dtbl[pte_idx];
	if (MMU_VALID_DT(*c_pte)) { /* Is the entry currently valid? */
		/* If the PTE is currently valid, then this function call
		 * is just a synonym for one (or more) of the following
		 * operations:
		 *     change protections on a page
		 *     change wiring status of a page
		 *     remove the mapping of a page
		 */
		/* Is the new address the same as the old? */
		if (MMU_PTE_PA(*c_pte) == pa) {
			/* Yes, do nothing. */
		} else {
			/* No, remove the old entry */
			pmap_remove_pte(c_pte);
		}
	} else {
		/* No, update the valid entry count in the C table */
		c_tbl->ct_ecnt++;
		/* and in pmap */
		pmap->pm_stats.resident_count++;
        }
	/* Map the page. */
	c_pte->attr.raw = ((unsigned long) pa | MMU_DT_PAGE);

	if (wired) /* Does the entry need to be wired? */ {
		c_pte->attr.raw |= MMU_SHORT_PTE_WIRED;
	}

        /* If the physical address being mapped is managed by the PV
         * system then link the pte into the list of pages mapped to that
         * address.
         */
        if (is_managed(pa)) {
            pv = pa2pv(pa);
            pve = pte2pve(c_pte);
            LIST_INSERT_HEAD(&pv->pv_head, pve, pve_link);
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
	boolean_t was_valid = FALSE;
	mmu_short_pte_t *pte;

	/* XXX - This array is traditionally named "Sysmap" */
	pte = &kernCbase[(unsigned long) sun3x_btop(va - KERNBASE)];
	if (MMU_VALID_DT(*pte))
		was_valid = TRUE;

	pte->attr.raw = (pa | MMU_DT_PAGE);

	if (!(prot & VM_PROT_WRITE)) /* If access should be read-only */
		pte->attr.raw |= MMU_SHORT_PTE_WP;
	if (pa & PMAP_NC)
		pte->attr.raw |= MMU_SHORT_PTE_CI;
	if (was_valid) {
		/* mmu_flusha(FC_SUPERD, va); */
		/* mmu_flusha(); */
		TBIA();
	}

}

/* pmap_protect			INTERFACE
 **
 * Apply the given protection to the given virtual address within
 * the given map.
 *
 * It is ok for the protection applied to be stronger than what is
 * specified.  We use this to our advantage when the given map has no
 * mapping for the virtual address.  By returning immediately when this
 * is discovered, we are effectively applying a protection of VM_PROT_NONE,
 * and therefore do not need to map the page just to apply a protection
 * code.  Only pmap_enter() needs to create new mappings if they do not exist.
 */
void
pmap_protect(pmap, va, pa, prot)
	pmap_t pmap;
	vm_offset_t va, pa;
	vm_prot_t prot;
{
	int a_idx, b_idx, c_idx;
	a_tmgr_t *a_tbl;
	b_tmgr_t *b_tbl;
	c_tmgr_t *c_tbl;
	mmu_short_pte_t *pte;

	if (pmap == NULL)
		return;
	if (pmap == pmap_kernel()) {
		pmap_protect_kernel(va, pa, prot);
		return;
	}

	/* Retrieve the mapping from the given pmap.  If it does
	 * not exist then we need not do anything more.
	 */
	if (pmap_stroll(pmap, va, &a_tbl, &b_tbl, &c_tbl, &pte,
		&a_idx, &b_idx, &c_idx) == FALSE) {
		return;
	}

	switch (prot) {
		case VM_PROT_ALL:
			/* this should never happen in a sane system */
			break;
		case VM_PROT_READ:
		case VM_PROT_READ|VM_PROT_EXECUTE:
			/* make the mapping read-only */
			pte->attr.raw |= MMU_SHORT_PTE_WP;
			break;
		case VM_PROT_NONE:
			/* this is an alias for 'pmap_remove' */
			pmap_dereference_pte(pte);
			break;
		default:
			break;
	}
}

/* pmap_protect_kernel			INTERNAL
 **
 * Apply the given protection code to a kernel address mapping.
 */
void
pmap_protect_kernel(va, pa, prot)
	vm_offset_t va, pa;
	vm_prot_t prot;
{
	mmu_short_pte_t *pte;

	pte = &kernCbase[(unsigned long) sun3x_btop(va - KERNBASE)];
	if (MMU_VALID_DT(*pte)) {
		switch (prot) {
			case VM_PROT_ALL:
				break;
			case VM_PROT_READ:
			case VM_PROT_READ|VM_PROT_EXECUTE:
				pte->attr.raw |= MMU_SHORT_PTE_WP;
				break;
			case VM_PROT_NONE:
				/* this is an alias for 'pmap_remove_kernel' */
				pte->attr.raw = MMU_DT_INVALID;
				break;
			default:
				break;
		}
	}
	/* since this is the kernel, immediately flush any cached
	 * descriptors for this address.
	 */
	/* mmu_flush(FC_SUPERD, va); */
	TBIS(va);
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
	
	/* Walk through the tables.  If the walk terminates without
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

	/* Decrement the wired entry count in the C table.
	 * If it reaches zero the following things happen:
	 * 1. The table no longer has any wired entries and is considered 
	 *    unwired.
	 * 2. It is placed on the available queue.
	 * 3. The parent table's wired entry count is decremented.
	 * 4. If it reaches zero, this process repeats at step 1 and
	 *    stops at after reaching the A table.
	 */
	if (c_tbl->ct_wcnt-- == 0) {
		TAILQ_INSERT_TAIL(&c_pool, c_tbl, ct_link);
		if (b_tbl->bt_wcnt-- == 0) {
			TAILQ_INSERT_TAIL(&b_pool, b_tbl, bt_link);
			if (a_tbl->at_wcnt-- == 0) {
				TAILQ_INSERT_TAIL(&a_pool, a_tbl, at_link);
			}
		}
	}

	pmap->pm_stats.wired_count--;
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
 * This function makes use of two virtual pages allocated in sun3x_vm_init()
 * (found in _startup.c) to map the two specified physical pages into the
 * kernel address space.  It then uses bcopy() to copy one into the other.
 */
void
pmap_copy_page(src, dst)
	vm_offset_t src, dst;
{
	PMAP_LOCK();
	if (tmp_vpages_inuse)
		panic("pmap_copy_page: temporary vpages are in use.");
	tmp_vpages_inuse++;

	pmap_enter_kernel(tmp_vpages[0], src, VM_PROT_READ);
	pmap_enter_kernel(tmp_vpages[1], dst, VM_PROT_READ|VM_PROT_WRITE);
	bcopy((char *) tmp_vpages[1], (char *) tmp_vpages[0], NBPG);
	/* xxx - there's no real need to unmap the mappings is there? */

	tmp_vpages_inuse--;
	PMAP_UNLOCK();
}

/* pmap_zero_page			INTERFACE
 **
 * Zero the contents of the specified physical page.
 *
 * Uses one of the virtual pages allocated in sun3x_vm_init() (_startup.c)
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
	bzero((char *) tmp_vpages[0], NBPG);
	/* xxx - there's no real need to unmap the mapping is there? */

	tmp_vpages_inuse--;
	PMAP_UNLOCK();
}

/* pmap_collect			INTERFACE
 **
 * Called from the VM system to collect unused pages in the given
 * pmap.
 *
 * No one implements it, so I'm not even sure how it is supposed to
 * 'collect' anything anyways.  There's nothing to do but do what everyone
 * else does..
 */
void
pmap_collect(pmap)
	pmap_t pmap;
{
	/* not implemented. */
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
	pmap->pm_a_tbl = proc0Atmgr;
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
	/* As long as the pmap contains no mappings,
	 * which always should be the case whenever
	 * this function is called, there really should
	 * be nothing to do.
	 */
#ifdef	PMAP_DEBUG
	if (pmap == NULL)
		return;
	if (pmap == pmap_kernel())
		panic("pmap_release: kernel pmap release requested.");
	if (pmap->pm_a_tbl != proc0Atmgr)
		panic("pmap_release: pmap not empty.");
#endif
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
	pv_elem_t *pve;
	struct mmu_short_pte_struct *pte;

	if (!pv_initialized)
		return FALSE;
	if (!is_managed(pa))
		return FALSE;

	pv = pa2pv(pa);
	/* Check the flags on the pv head.  If they are set,
	 * return immediately.  Otherwise a search must be done.
         */
	if (pv->pv_flags & PV_FLAGS_USED)
		return TRUE;
	else
		/* Search through all pv elements pointing
		 * to this page and query their reference bits
		 */
		for (pve = pv->pv_head.lh_first;
                     pve != NULL;
                     pve = pve->pve_link.le_next) {
			pte = pve2pte(pve);
			if (MMU_PTE_USED(*pte))
				return TRUE;
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
	pv_elem_t *pve;

	if (!pv_initialized)
		return FALSE;
	if (!is_managed(pa))
		return FALSE;

	/* see comments in pmap_is_referenced() */
	pv = pa2pv(pa);
	if (pv->pv_flags & PV_FLAGS_MDFY)
		return TRUE;
	else
		for (pve = pv->pv_head.lh_first; pve != NULL;
                     pve = pve->pve_link.le_next) {
			struct mmu_short_pte_struct *pte;
			pte = pve2pte(pve);
			if (MMU_PTE_MODIFIED(*pte))
				return TRUE;
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
	pv_elem_t *pve;
	struct mmu_short_pte_struct *pte;

	if (!is_managed(pa))
		return;
	
	pv = pa2pv(pa);
	for (pve = pv->pv_head.lh_first; pve != NULL;
		pve = pve->pve_link.le_next) {
		pte = pve2pte(pve);
		switch (prot) {
			case VM_PROT_ALL:
				/* do nothing */
				break;
			case VM_PROT_READ:
			case VM_PROT_READ|VM_PROT_EXECUTE:
				pte->attr.raw |= MMU_SHORT_PTE_WP;
				break;
			case VM_PROT_NONE:
				pmap_dereference_pte(pte);
				break;
			default:
				break;
		}
	}
}

/* pmap_who_owns_pte			INTERNAL
 **
 * Called internally to find which pmap the given pte is
 * a member of.
 */
pmap_t
pmap_who_owns_pte(pte)
	mmu_short_pte_t *pte;
{
	c_tmgr_t *c_tbl;
	
	c_tbl = pmap_find_c_tmgr(pte);
	
	return c_tbl->ct_parent->bt_parent->at_parent;
}

/* pmap_find_va			INTERNAL_X
 **
 * Called internally to find the virtual address that the
 * given pte maps.
 *
 * Note: I don't know if this function will ever be used, but I've
 * implemented it just in case.
 */
vm_offset_t
pmap_find_va(pte)
	mmu_short_pte_t *pte;
{
	a_tmgr_t    *a_tbl;
	b_tmgr_t    *b_tbl;
	c_tmgr_t    *c_tbl;
	vm_offset_t     va = 0;

	/* Find the virtual address by decoding table indexes.
	 * Each successive decode will reveal the address from
	 * least to most significant bit fashion.
	 *
	 * 31                              0
         * +-------------------------------+
	 * |AAAAAAABBBBBBCCCCCCxxxxxxxxxxxx|
	 * +-------------------------------+
	 *
	 * Start with the 'C' bits.
	 */
	va |= (pmap_find_tic(pte) << MMU_TIC_SHIFT);
	c_tbl = pmap_find_c_tmgr(pte);
	b_tbl = c_tbl->ct_parent;

	/* Add the 'B' bits. */
	va |= (c_tbl->ct_pidx << MMU_TIB_SHIFT);
	a_tbl = b_tbl->bt_parent;

	/* Add the 'A' bits. */
	va |= (b_tbl->bt_pidx << MMU_TIA_SHIFT);

	return va;
}

/**** These functions should be removed.  Structures have changed, making ****
 **** them uneccessary.                                                   ****/

/* pmap_find_tic			INTERNAL
 **
 * Given the address of a pte, find the TIC (level 'C' table index) for
 * the pte within its C table.
 */
char
pmap_find_tic(pte)
	mmu_short_pte_t *pte;
{
	return ((mmuCbase - pte) % MMU_C_TBL_SIZE);
}

/* pmap_find_tib			INTERNAL
 **
 * Given the address of dte known to belong to a B table, find the TIB
 * (level 'B' table index) for the dte within its table.
 */
char
pmap_find_tib(dte)
	mmu_short_dte_t *dte;
{
	return ((mmuBbase - dte) % MMU_B_TBL_SIZE);
}

/* pmap_find_tia			INTERNAL
 **
 * Given the address of a dte known to belong to an A table, find the
 * TIA (level 'C' table index) for the dte withing its table.
 */
char
pmap_find_tia(dte)
	mmu_long_dte_t *dte;
{
	return ((mmuAbase - dte) % MMU_A_TBL_SIZE);
}

/**** This one should stay ****/

/* pmap_find_c_tmgr			INTERNAL
 **
 * Given a pte known to belong to a C table, return the address of that
 * table's management structure.
 */
c_tmgr_t *
pmap_find_c_tmgr(pte)
	mmu_short_pte_t *pte;
{
	return &Ctmgrbase[
		((mmuCbase - pte) / sizeof(*pte) / MMU_C_TBL_SIZE)
		];
}

/* pmap_find_b_tmgr			INTERNAL
 **
 * Given a dte known to belong to a B table, return the address of that
 * table's management structure.
 */
b_tmgr_t *
pmap_find_b_tmgr(dte)
	mmu_short_dte_t *dte;
{
	return &Btmgrbase[
		((mmuBbase - dte) / sizeof(*dte) / MMU_B_TBL_SIZE)
		];
}

/* pmap_find_a_tmgr			INTERNAL
 **
 * Given a dte known to belong to an A table, return the address of that
 * table's management structure.
 */
a_tmgr_t *
pmap_find_a_tmgr(dte)
	mmu_long_dte_t *dte;
{
	return &Atmgrbase[
		((mmuAbase - dte) / sizeof(*dte) / MMU_A_TBL_SIZE)
		];
}

/**** End of functions that should be removed.                          ****
 ****                                                                   ****/

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
	pv_elem_t *pve;
	mmu_short_pte_t *pte;

	pv = pa2pv(pa);
	pv->pv_flags &= ~(flag);
	for (pve = pv->pv_head.lh_first; pve != NULL; 
             pve = pve->pve_link.le_next) {
		pte = pve2pte(pve);
		pte->attr.raw &= ~(flag);
	}
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
		&c_pte, &a_idx, &b_idx, &pte_idx) == FALSE);
		return 0;

	if (MMU_VALID_DT(*c_pte))
		return MMU_PTE_PA(*c_pte);
	else
		return 0;
}

/* pmap_extract_kernel		INTERNAL
 **
 * Extract a traslation from the kernel address space.
 */
vm_offset_t
pmap_extract_kernel(va)
	vm_offset_t va;
{
	mmu_short_pte_t *pte;

	pte = &kernCbase[(unsigned long) sun3x_btop(va - KERNBASE)];
	return MMU_PTE_PA(*pte);
}

/* pmap_remove_kernel		INTERNAL
 **
 * Remove the mapping of a range of virtual addresses from the kernel map.
 */
void
pmap_remove_kernel(start, end)
	vm_offset_t start;
	vm_offset_t end;
{
	start -= KERNBASE;
	end   -= KERNBASE;
	start = sun3x_round_page(start); /* round down */
	start = sun3x_btop(start);
	end   += MMU_PAGE_SIZE - 1;    /* next round operation will be up */
	end   = sun3x_round_page(end); /* round */
	end   = sun3x_btop(end);

	while (start < end)
		kernCbase[start++].attr.raw = MMU_DT_INVALID;
}

/* pmap_remove			INTERFACE
 **
 * Remove the mapping of a range of virtual addresses from the given pmap.
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
	pmap_remove_a(pmap->pm_a_tbl, start, end);

	/* If we just modified the current address space,
	 * make sure to flush the MMU cache.
	 */
	if (curatbl == pmap->pm_a_tbl) {
		/* mmu_flusha(); */
		TBIA();
	}
}

/* pmap_remove_a			INTERNAL
 **
 * This is function number one in a set of three that removes a range
 * of memory in the most efficient manner by removing the highest possible
 * tables from the memory space.  This particular function attempts to remove
 * as many B tables as it can, delegating the remaining fragmented ranges to
 * pmap_remove_b().
 *
 * It's ugly but will do for now.
 */
void
pmap_remove_a(a_tbl, start, end)
	a_tmgr_t *a_tbl;
	vm_offset_t start;
	vm_offset_t end;
{
	int idx;
	vm_offset_t nstart, nend, rstart;
	b_tmgr_t *b_tbl;
	mmu_long_dte_t  *a_dte;
	mmu_short_dte_t *b_dte;
	

	if (a_tbl == proc0Atmgr) /* If the pmap has no A table, return */
		return;

	nstart = MMU_ROUND_UP_A(start);
	nend = MMU_ROUND_A(end);

	if (start < nstart) {
		idx = MMU_TIA(start);
		a_dte = &a_tbl->at_dtbl[idx];
		if (MMU_VALID_DT(*a_dte)) {
			b_dte = (mmu_short_dte_t *) MMU_DTE_PA(*a_dte);
			b_dte = (mmu_short_dte_t *) mmu_ptov(b_dte);
			b_tbl = mmuB2tmgr(b_dte);
			if (end < nstart) {
				pmap_remove_b(b_tbl, start, end);
				return;
			} else {
				pmap_remove_b(b_tbl, start, nstart);
			}
		} else if (end < nstart) {
			return;
		}
	}
	if (nstart < nend) {
		idx = MMU_TIA(nstart);
		a_dte = &a_tbl->at_dtbl[idx];
		rstart = nstart;
		while (rstart < nend) {
			if (MMU_VALID_DT(*a_dte)) {
				b_dte = (mmu_short_dte_t *) MMU_DTE_PA(*a_dte);
				b_dte = (mmu_short_dte_t *) mmu_ptov(b_dte);
				b_tbl = mmuB2tmgr(b_dte);
				a_dte->attr.raw = MMU_DT_INVALID;
				a_tbl->at_ecnt--;
				free_b_table(b_tbl);
				TAILQ_REMOVE(&b_pool, b_tbl, bt_link);
				TAILQ_INSERT_HEAD(&b_pool, b_tbl, bt_link);
			}
			a_dte++;
			rstart += MMU_TIA_RANGE;
		}
	}
	if (nend < end) {
		idx = MMU_TIA(nend);
		a_dte = &a_tbl->at_dtbl[idx];
		if (MMU_VALID_DT(*a_dte)) {
			b_dte = (mmu_short_dte_t *) MMU_DTE_PA(*a_dte);
			b_dte = (mmu_short_dte_t *) mmu_ptov(b_dte);
			b_tbl = mmuB2tmgr(b_dte);
			pmap_remove_b(b_tbl, nend, end);
		}
	}
}

/* pmap_remove_b			INTERNAL
 **
 * Remove a range of addresses from an address space, trying to remove entire
 * C tables if possible.
 */
void
pmap_remove_b(b_tbl, start, end)
	b_tmgr_t *b_tbl;
	vm_offset_t start;
	vm_offset_t end;
{
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
			c_dte = (mmu_short_pte_t *) MMU_DTE_PA(*b_dte);
			c_dte = (mmu_short_pte_t *) mmu_ptov(c_dte);
			c_tbl = mmuC2tmgr(c_dte);
			if (end < nstart) {
				pmap_remove_c(c_tbl, start, end);
				return;
			} else {
				pmap_remove_c(c_tbl, start, nstart);
			}
		} else if (end < nstart) {
			return;
		}
	}
	if (nstart < nend) {
		idx = MMU_TIB(nstart);
		b_dte = &b_tbl->bt_dtbl[idx];
		rstart = nstart;
		while (rstart < nend) {
			if (MMU_VALID_DT(*b_dte)) {
				c_dte = (mmu_short_pte_t *) MMU_DTE_PA(*b_dte);
				c_dte = (mmu_short_pte_t *) mmu_ptov(c_dte);
				c_tbl = mmuC2tmgr(c_dte);
				b_dte->attr.raw = MMU_DT_INVALID;
				b_tbl->bt_ecnt--;
				free_c_table(c_tbl);
				TAILQ_REMOVE(&c_pool, c_tbl, ct_link);
				TAILQ_INSERT_HEAD(&c_pool, c_tbl, ct_link);
			}
			b_dte++;
			rstart += MMU_TIB_RANGE;
		}
	}
	if (nend < end) {
		idx = MMU_TIB(nend);
		b_dte = &b_tbl->bt_dtbl[idx];
		if (MMU_VALID_DT(*b_dte)) {
			c_dte = (mmu_short_pte_t *) MMU_DTE_PA(*b_dte);
			c_dte = (mmu_short_pte_t *) mmu_ptov(c_dte);
			c_tbl = mmuC2tmgr(c_dte);
			pmap_remove_c(c_tbl, nend, end);
		}
	}
}

/* pmap_remove_c			INTERNAL
 **
 * Remove a range of addresses from the given C table.
 */
void
pmap_remove_c(c_tbl, start, end)
	c_tmgr_t *c_tbl;
	vm_offset_t start;
	vm_offset_t end;
{
	int idx;
	mmu_short_pte_t *c_pte;
	
	idx = MMU_TIC(start);
	c_pte = &c_tbl->ct_dtbl[idx];
	while (start < end) {
		if (MMU_VALID_DT(*c_pte))
			pmap_remove_pte(c_pte);
		c_tbl->ct_ecnt--;
		start += MMU_PAGE_SIZE;
		c_pte++;
	}
}

/* is_managed				INTERNAL
 **
 * Determine if the given physical address is managed by the PV system.
 * Note that this logic assumes that no one will ask for the status of
 * addresses which lie in-between the memory banks on the 3/80.  If they
 * do so, it will falsely report that it is managed.
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

/* pa2pv			INTERNAL
 **
 * Return the pv_list_head element which manages the given physical
 * address.
 */
pv_t *
pa2pv(pa)
	vm_offset_t pa;
{
	struct pmap_physmem_struct *bank = &avail_mem[0];

	while (pa >= bank->pmem_end)
		bank = bank->pmem_next;

	pa -= bank->pmem_start;
	return &pvbase[bank->pmem_pvbase + sun3x_btop(pa)];
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

	rtn = (void *) virtual_avail;

	/* While the size is greater than a page, map single pages,
	 * decreasing size until it is less than a page.
	 */
	while (size > NBPG) {
		(void) pmap_bootstrap_alloc(NBPG);

		/* If the above code is ok, let's keep it.
		 * It looks cooler than:
		 * virtual_avail += NBPG;
		 * avail_start += NBPG;
		 * last_mapped = sun3x_trunc_page(avail_start);
		 * pmap_enter_kernel(last_mapped, last_mapped + KERNBASE,
		 *    VM_PROT_READ|VM_PROT_WRITE);
		 */

		 size -= NBPG;
	}
	avail_start += size;
	virtual_avail += size;

	/* did the allocation cross a page boundary? */
	if (last_mapped != sun3x_trunc_page(avail_start)) {
		last_mapped = sun3x_trunc_page(avail_start);
		pmap_enter_kernel(last_mapped + KERNBASE, last_mapped,
		    VM_PROT_READ|VM_PROT_WRITE);
	}

	return rtn;
}

/* pmap_bootstap_aalign			INTERNAL
 **
 * Used to insure that the next call to pmap_bootstrap_alloc() will return
 * a chunk of memory aligned to the specified size.
 */
void
pmap_bootstrap_aalign(size)
	int size;
{
	if (((unsigned int) avail_start % size) != 0) {
		(void) pmap_bootstrap_alloc(size -
		    ((unsigned int) (avail_start % size)));
	}
}
		
#if 0
/* pmap_activate			INTERFACE
 **
 * Make the virtual to physical mappings contained in the given
 * pmap the current map used by the system.
 */
void
pmap_activate(pmap, pcbp)
pmap_t	pmap;
struct  pcb *pcbp;
{
	vm_offset_t	pa;
	/* Save the A table being loaded in 'curatbl'.
	 * pmap_remove() uses this variable to determine if a given A
	 * table is currently being used as the system map.  If so, it
	 * will issue an MMU cache flush whenever mappings are removed.
	 */
	curatbl = pmap->pm_a_tbl;
	/* call the locore routine to set the user root pointer table */
	pa = mmu_vtop(pmap->pm_a_tbl->at_dtbl);
	mmu_seturp(pa);
}
#endif

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

	avail = sun3x_round_up_page(avail_start);

	left = 0;
	i = 0;
	while (avail >= avail_mem[i].pmem_end) {
		if (avail_mem[i].pmem_next == NULL)
			return 0;
		i++;
	}
	while (i < SUN3X_80_MEM_BANKS) {
		if (avail < avail_mem[i].pmem_start) {
			/* Avail is inside a hole, march it
			 * up to the next bank.
			 */
			avail = avail_mem[i].pmem_start;
		}
		left += sun3x_btop(avail_mem[i].pmem_end - avail);
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
 * We will not count holes as valid pages.  We can generate page indexes
 * that conform to this by using the memory bank structures initialized
 * in pmap_alloc_pv().
 */
int
pmap_page_index(pa)
	vm_offset_t pa;
{
	struct pmap_physmem_struct *bank = avail_mem;

	while (pa > bank->pmem_end)
		bank = bank->pmem_next;
	pa -= bank->pmem_start;

	return (bank->pmem_pvbase + sun3x_btop(pa));
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
	static boolean_t initialized = FALSE;
	static struct pmap_physmem_struct *curbank = avail_mem;

	if (!initialized) {
		pmap_bootstrap_aalign(NBPG);
		initialized = TRUE;
	}

	if (avail_start >= curbank->pmem_end)
		if (curbank->pmem_next == NULL)
			return FALSE;
		else {
			curbank = curbank->pmem_next;
			avail_start = curbank->pmem_start;
		}

	*pa = avail_start;
	avail_start += NBPG;
	return TRUE;
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
 *
 * XXX - It might be nice if this worked outside of the MMU
 * structures we manage.  (Could do it with ptest). -gwr
 */
vm_offset_t
get_pte(va)
	vm_offset_t va;
{
	u_long idx;

	idx = (unsigned long) sun3x_btop(mmu_vtop(va));
	return (kernCbase[idx].attr.raw);
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

	idx = (unsigned long) sun3x_btop(mmu_vtop(va));
	kernCbase[idx].attr.raw = pte;
}

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
