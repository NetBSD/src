/*	$NetBSD: pmap.c,v 1.10 1996/07/02 22:51:46 cgd Exp $	*/

/*
 * Copyright (c) 1992, 1996 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

/*
 *	File:	pmap.c
 *
 *	Author list
 *	vax:  Avadis Tevanian, Jr., Michael Wayne Young
 *	i386: Lance Berc, Mike Kupfer, Bob Baron, David Golub, Richard Draves
 *	alpha: Alessandro Forin
 *	NetBSD/Alpha: Chris Demetriou
 *
 *	Physical Map management code for DEC Alpha
 *
 *	Manages physical address maps.
 *
 *	This code was derived exclusively from information available in
 *	"Alpha Architecture Reference Manual", Richard L. Sites ed.
 *	Digital Press, Burlington, MA 01803
 *	ISBN 1-55558-098-X, Order no. EY-L520E-DP
 */

/*
 *	In addition to hardware address maps, this
 *	module is called upon to provide software-use-only
 *	maps which may or may not be stored in the same
 *	form as hardware maps.  These pseudo-maps are
 *	used to store intermediate results from copy
 *	operations to and from address spaces.
 *
 *	Since the information managed by this module is
 *	also stored by the logical address mapping module,
 *	this module may throw away valid virtual-to-physical
 *	mappings at almost any time.  However, invalidations
 *	of virtual-to-physical mappings must be done as
 *	requested.
 *
 *	In order to cope with hardware architectures which
 *	make virtual-to-physical map invalidates expensive,
 *	this module may delay invalidate or reduced protection
 *	operations until such time as they are actually
 *	necessary.  This module is given full information as
 *	to which processors are currently using which maps,
 *	and to when physical maps must be made correct.
 */

#include <cpus.h>
#include <mach_kdb.h>

#include <mach/std_types.h>

#include <kern/lock.h>

#include <kern/thread.h>
#include <kern/zalloc.h>
#include <machine/machspl.h>

#include <vm/pmap.h>
#include <vm/vm_map.h>
#include <vm/vm_kern.h>
#include <mach/vm_param.h>
#include <mach/vm_prot.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <vm/vm_user.h>
#include <mach/machine/vm_param.h>
#include <machine/thread.h>

#include <alpha/alpha_scb.h>

#define	roundup(x,s)	(((x) + ((s)-1)) & ~((s)-1))

/* For external use... */
vm_offset_t kvtophys(vm_offset_t virt)
{
	vm_offset_t pmap_resident_extract();
	return pmap_resident_extract(kernel_pmap, virt);
}

/* ..but for internal use... */
#define phystokv(a)	PHYS_TO_K0SEG(a)
#define	kvtophys(p)	K0SEG_TO_PHYS(p)


/*
 *	Private data structures.
 */
/*
 *	Map from MI protection codes to MD codes.
 *	Assume that there are three MI protection codes, all using low bits.
 */
unsigned int	user_protection_codes[8];
unsigned int	kernel_protection_codes[8];

alpha_protection_init()
{
	register unsigned int	*kp, *up, prot;

	kp = kernel_protection_codes;
	up = user_protection_codes;
	for (prot = 0; prot < 8; prot++) {
		switch (prot) {
		case VM_PROT_NONE | VM_PROT_NONE | VM_PROT_NONE:
			*kp++ = 0;
			*up++ = 0;
			break;
		case VM_PROT_READ | VM_PROT_NONE | VM_PROT_NONE:
		case VM_PROT_READ | VM_PROT_NONE | VM_PROT_EXECUTE:
		case VM_PROT_NONE | VM_PROT_NONE | VM_PROT_EXECUTE:
			*kp++ = ALPHA_PTE_KR;
			*up++ = ALPHA_PTE_UR|ALPHA_PTE_KR;
			break;
		case VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_NONE:
			*kp++ = ALPHA_PTE_KW;
			*up++ = ALPHA_PTE_UW|ALPHA_PTE_KW;
			break;
		case VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_EXECUTE:
		case VM_PROT_READ | VM_PROT_WRITE | VM_PROT_NONE:
		case VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE:
			*kp++ = ALPHA_PTE_KW|ALPHA_PTE_KR;
			*up++ = ALPHA_PTE_UW|ALPHA_PTE_UR|ALPHA_PTE_KW|ALPHA_PTE_KR;
			break;
		}
	}
}

/*
 *	Given a map and a machine independent protection code,
 *	convert to a alpha protection code.
 */

#define	alpha_protection(map, prot) \
	(((map) == kernel_pmap) ? kernel_protection_codes[prot] : \
				  user_protection_codes[prot])

/* Build the typical kernel pte */
#define	pte_ktemplate(t,pa,pr)						\
MACRO_BEGIN								\
	(t) = pa_to_pte(pa) | ALPHA_PTE_VALID | ALPHA_PTE_GLOBAL |	\
	  (alpha_protection(kernel_pmap,pr) << ALPHA_PTE_PROTOFF);	\
MACRO_END

/* build the typical pte */
#define	pte_template(m,t,pa,pr)						\
MACRO_BEGIN								\
	(t) = pa_to_pte(pa) | ALPHA_PTE_VALID |				\
	  (alpha_protection(m,pr) << ALPHA_PTE_PROTOFF);		\
MACRO_END

/*
 *	For each vm_page_t, there is a list of all currently
 *	valid virtual mappings of that page.  An entry is
 *	a pv_entry_t; the list is the pv_table.
 */

typedef struct pv_entry {
	struct pv_entry	*next;		/* next pv_entry */
	pmap_t		pmap;		/* pmap where mapping lies */
	vm_offset_t	va;		/* virtual address for mapping */
} *pv_entry_t;

#define PV_ENTRY_NULL	((pv_entry_t) 0)

pv_entry_t	pv_head_table;		/* array of entries, one per page */

/*
 *	pv_list entries are kept on a list that can only be accessed
 *	with the pmap system locked (at SPLVM, not in the cpus_active set).
 *	The list is refilled from the pv_list_zone if it becomes empty.
 */
pv_entry_t	pv_free_list;		/* free list at SPLVM */
decl_simple_lock_data(, pv_free_list_lock)

#define	PV_ALLOC(pv_e) { \
	simple_lock(&pv_free_list_lock); \
	if ((pv_e = pv_free_list) != 0) { \
	    pv_free_list = pv_e->next; \
	} \
	simple_unlock(&pv_free_list_lock); \
}

#define	PV_FREE(pv_e) { \
	simple_lock(&pv_free_list_lock); \
	pv_e->next = pv_free_list; \
	pv_free_list = pv_e; \
	simple_unlock(&pv_free_list_lock); \
}

zone_t		pv_list_zone;		/* zone of pv_entry structures */

/*
 *	Each entry in the pv_head_table is locked by a bit in the
 *	pv_lock_table.  The lock bits are accessed by the physical
 *	address of the page they lock.
 */

char	*pv_lock_table;		/* pointer to array of bits */
#define pv_lock_table_size(n)	(((n)+BYTE_SIZE-1)/BYTE_SIZE)

/*
 *	First and last physical addresses that we maintain any information
 *	for.  Initialized to zero so that pmap operations done before
 *	pmap_init won't touch any non-existent structures.
 */
vm_offset_t	vm_first_phys = (vm_offset_t) 0;
vm_offset_t	vm_last_phys  = (vm_offset_t) 0;
boolean_t	pmap_initialized = FALSE;/* Has pmap_init completed? */

/*
 *	Index into pv_head table, its lock bits, and the modify/reference
 *	bits starting at vm_first_phys.
 */

#define pa_index(pa)	(atop(pa - vm_first_phys))

#define pai_to_pvh(pai)		(&pv_head_table[pai])
#define lock_pvh_pai(pai)	(bit_lock(pai, pv_lock_table))
#define unlock_pvh_pai(pai)	(bit_unlock(pai, pv_lock_table))

/*
 *	Array of physical page attributes for managed pages.
 *	One byte per physical page.
 */
char	*pmap_phys_attributes;

/*
 *	Physical page attributes.  Copy bits from PTE.
 */
#define	PHYS_MODIFIED	(ALPHA_PTE_MOD>>16)	/* page modified */
#define	PHYS_REFERENCED	(ALPHA_PTE_REF>>16)	/* page referenced */

#define	pte_get_attributes(p)	((*p & (ALPHA_PTE_MOD|ALPHA_PTE_REF)) >> 16)

/*
 *	Amount of virtual memory mapped by one
 *	page-directory entry.
 */
#define	PDE_MAPPED_SIZE		(pdetova(1))
#define	PDE2_MAPPED_SIZE	(pde2tova(1))
#define	PDE3_MAPPED_SIZE	(pde3tova(1))

/*
 *	We allocate page table pages directly from the VM system
 *	through this object.  It maps physical memory.
 */
vm_object_t	pmap_object = VM_OBJECT_NULL;

/*
 *	Locking and TLB invalidation
 */

/*
 *	Locking Protocols:
 *
 *	There are two structures in the pmap module that need locking:
 *	the pmaps themselves, and the per-page pv_lists (which are locked
 *	by locking the pv_lock_table entry that corresponds to the pv_head
 *	for the list in question.)  Most routines want to lock a pmap and
 *	then do operations in it that require pv_list locking -- however
 *	pmap_remove_all and pmap_copy_on_write operate on a physical page
 *	basis and want to do the locking in the reverse order, i.e. lock
 *	a pv_list and then go through all the pmaps referenced by that list.
 *	To protect against deadlock between these two cases, the pmap_lock
 *	is used.  There are three different locking protocols as a result:
 *
 *  1.  pmap operations only (pmap_extract, pmap_access, ...)  Lock only
 *		the pmap.
 *
 *  2.  pmap-based operations (pmap_enter, pmap_remove, ...)  Get a read
 *		lock on the pmap_lock (shared read), then lock the pmap
 *		and finally the pv_lists as needed [i.e. pmap lock before
 *		pv_list lock.]
 *
 *  3.  pv_list-based operations (pmap_remove_all, pmap_copy_on_write, ...)
 *		Get a write lock on the pmap_lock (exclusive write); this
 *		also guaranteees exclusive access to the pv_lists.  Lock the
 *		pmaps as needed.
 *
 *	At no time may any routine hold more than one pmap lock or more than
 *	one pv_list lock.  Because interrupt level routines can allocate
 *	mbufs and cause pmap_enter's, the pmap_lock and the lock on the
 *	kernel_pmap can only be held at splvm.
 */

#if	NCPUS > 1
/*
 *	We raise the interrupt level to splvm, to block interprocessor
 *	interrupts during pmap operations.  We must take the CPU out of
 *	the cpus_active set while interrupts are blocked.
 */
#define SPLVM(spl)	{ \
	spl = splvm(); \
	i_bit_clear(cpu_number(), &cpus_active); \
}

#define SPLX(spl)	{ \
	i_bit_set(cpu_number(), &cpus_active); \
	splx(spl); \
}

/*
 *	Lock on pmap system
 */
lock_data_t	pmap_system_lock;

volatile boolean_t	cpu_update_needed[NCPUS];

#define PMAP_READ_LOCK(pmap, spl) { \
	SPLVM(spl); \
	lock_read(&pmap_system_lock); \
	simple_lock(&(pmap)->lock); \
}

#define PMAP_WRITE_LOCK(spl) { \
	SPLVM(spl); \
	lock_write(&pmap_system_lock); \
}

#define PMAP_READ_UNLOCK(pmap, spl) { \
	simple_unlock(&(pmap)->lock); \
	lock_read_done(&pmap_system_lock); \
	SPLX(spl); \
}

#define PMAP_WRITE_UNLOCK(spl) { \
	lock_write_done(&pmap_system_lock); \
	SPLX(spl); \
}

#define PMAP_WRITE_TO_READ_LOCK(pmap) { \
	simple_lock(&(pmap)->lock); \
	lock_write_to_read(&pmap_system_lock); \
}

#define LOCK_PVH(index)		(lock_pvh_pai(index))

#define UNLOCK_PVH(index)	(unlock_pvh_pai(index))

#define PMAP_UPDATE_TLBS(pmap, s, e) \
{ \
	cpu_set	cpu_mask = 1 << cpu_number(); \
	cpu_set	users; \
 \
	/* Since the pmap is locked, other updates are locked */ \
	/* out, and any pmap_activate has finished. */ \
 \
	/* find other cpus using the pmap */ \
	users = (pmap)->cpus_using & ~cpu_mask; \
	if (users) { \
	    /* signal them, and wait for them to finish */ \
	    /* using the pmap */ \
	    signal_cpus(users, (pmap), (s), (e)); \
	    while ((pmap)->cpus_using & cpus_active & ~cpu_mask) \
		continue; \
	} \
 \
	/* invalidate our own TLB if pmap is in use */ \
	if ((pmap)->cpus_using & cpu_mask) { \
	    INVALIDATE_TLB((s), (e)); \
	} \
}

#else	NCPUS > 1

#define SPLVM(spl)
#define SPLX(spl)

#define PMAP_READ_LOCK(pmap, spl)	SPLVM(spl)
#define PMAP_WRITE_LOCK(spl)		SPLVM(spl)
#define PMAP_READ_UNLOCK(pmap, spl)	SPLX(spl)
#define PMAP_WRITE_UNLOCK(spl)		SPLX(spl)
#define PMAP_WRITE_TO_READ_LOCK(pmap)

#define LOCK_PVH(index)
#define UNLOCK_PVH(index)

#if 0 /*fix bug later */
#define PMAP_UPDATE_TLBS(pmap, s, e) { \
	/* invalidate our own TLB if pmap is in use */ \
	if ((pmap)->cpus_using) { \
	    INVALIDATE_TLB((s), (e)); \
	} \
}
#else
#define PMAP_UPDATE_TLBS(pmap, s, e) { \
	    INVALIDATE_TLB((s), (e)); \
}
#endif

#endif	/* NCPUS > 1 */

#if 0
#define INVALIDATE_TLB(s, e) { \
	register vm_offset_t	v = s, ve = e; \
	while (v < ve) { \
	    tbis(v); v += ALPHA_PGBYTES; \
	} \
}
#else
#define INVALIDATE_TLB(s, e) { \
	tbia(); \
}
#endif


#if	NCPUS > 1

void pmap_update_interrupt();

/*
 *	Structures to keep track of pending TLB invalidations
 */

#define UPDATE_LIST_SIZE	4

struct pmap_update_item {
	pmap_t		pmap;		/* pmap to invalidate */
	vm_offset_t	start;		/* start address to invalidate */
	vm_offset_t	end;		/* end address to invalidate */
} ;

typedef	struct pmap_update_item	*pmap_update_item_t;

/*
 *	List of pmap updates.  If the list overflows,
 *	the last entry is changed to invalidate all.
 */
struct pmap_update_list {
	decl_simple_lock_data(,	lock)
	int			count;
	struct pmap_update_item	item[UPDATE_LIST_SIZE];
} ;
typedef	struct pmap_update_list	*pmap_update_list_t;

struct pmap_update_list	cpu_update_list[NCPUS];

#endif	/* NCPUS > 1 */

/*
 *	Other useful macros.
 */
#define current_pmap()		(vm_map_pmap(current_thread()->task->map))
#define pmap_in_use(pmap, cpu)	(((pmap)->cpus_using & (1 << (cpu))) != 0)

struct pmap	kernel_pmap_store;
pmap_t		kernel_pmap;

struct zone	*pmap_zone;		/* zone of pmap structures */

int		pmap_debug = 0;		/* flag for debugging prints */
int		ptes_per_vm_page;	/* number of hardware ptes needed
					   to map one VM page. */
unsigned int	inuse_ptepages_count = 0;	/* debugging */

extern char end;
/*
 * Page directory for kernel.
 */
extern pt_entry_t	root_kpdes[];	/* see start.s */

void pmap_remove_range();	/* forward */
#if	NCPUS > 1
void signal_cpus();		/* forward */
#endif	/* NCPUS > 1 */

/*
 *	Given an offset and a map, compute the address of the
 *	pte.  If the address is invalid with respect to the map
 *	then PT_ENTRY_NULL is returned (and the map may need to grow).
 *
 *	This is only used internally.
 */
#define	pmap_pde(pmap, addr) (&(pmap)->dirbase[pdenum(addr)])

pt_entry_t *pmap_pte(pmap, addr)
	register pmap_t		pmap;
	register vm_offset_t	addr;
{
	register pt_entry_t	*ptp;
	register pt_entry_t	pte;

	if (pmap->dirbase == 0)
		return(PT_ENTRY_NULL);
	/* seg1 */
	pte = *pmap_pde(pmap,addr);
	if ((pte & ALPHA_PTE_VALID) == 0)
		return(PT_ENTRY_NULL);
	/* seg2 */
	ptp = (pt_entry_t *)ptetokv(pte);
	pte = ptp[pte2num(addr)];
	if ((pte & ALPHA_PTE_VALID) == 0)
		return(PT_ENTRY_NULL);
	/* seg3 */
	ptp = (pt_entry_t *)ptetokv(pte);
	return(&ptp[pte3num(addr)]);

}

#define DEBUG_PTE_PAGE	1

extern	vm_offset_t	virtual_avail, virtual_end;
extern	vm_offset_t	avail_start, avail_end;

/*
 *	Bootstrap the system enough to run with virtual memory.
 *	Map the kernel's code and data, and allocate the system page table.
 *	Called with mapping OFF.  Page_size must already be set.
 *
 *	Parameters:
 *	avail_start	PA of first available physical page
 *	avail_end	PA of last available physical page
 *	virtual_avail	VA of first available page
 *	virtual_end	VA of last available page
 *
 */
vm_size_t	pmap_kernel_vm = 5;	/* each one 8 meg worth */

void pmap_bootstrap()
{
	vm_offset_t	pa;
	pt_entry_t	template;
	pt_entry_t	*pde, *pte;
	int		i;
	extern boolean_t vm_fault_dirty_handling;


	/*
	 *	Tell VM to do mod bits for us
	 */
	vm_fault_dirty_handling = TRUE;

	alpha_protection_init();

	/*
	 *	Set ptes_per_vm_page for general use.
	 */
	ptes_per_vm_page = page_size / ALPHA_PGBYTES;

	/*
	 *	The kernel's pmap is statically allocated so we don't
	 *	have to use pmap_create, which is unlikely to work
	 *	correctly at this part of the boot sequence.
	 */

	kernel_pmap = &kernel_pmap_store;

#if	NCPUS > 1
	lock_init(&pmap_system_lock, FALSE);	/* NOT a sleep lock */
#endif	/* NCPUS > 1 */

	simple_lock_init(&kernel_pmap->lock);

	kernel_pmap->ref_count = 1;

	/*
	 *	The kernel page directory has been allocated;
	 *	its virtual address is in root_kpdes.
	 *
	 *	No other physical memory has been allocated.
	 */

	kernel_pmap->dirbase = root_kpdes;

	/*
	 *	The distinguished tlbpid value of 0 is reserved for
	 *	the kernel pmap. Initialize the tlbpid allocator,
	 *	who knows about this.
	 */
	kernel_pmap->pid = 0;
	pmap_tlbpid_init();

#if 0
	/*
	 *	Rid of console's default mappings
	 */
	for (pde = pmap_pde(kernel_pmap,0);
	     pde < pmap_pde(kernel_pmap,VM_MIN_KERNEL_ADDRESS);)
		*pde++ = 0;

#endif
	/*
	 *	Allocate the seg2 kernel page table entries from the front
	 *	of available physical memory.  Take enough to cover all of
	 *	the K2SEG range. But of course one page is enough for 8Gb,
	 *	and more in future chips ...
	 */
#define	enough_kseg2()	(PAGE_SIZE)

	pte = (pt_entry_t *) pmap_steal_memory(enough_kseg2());	/* virtual */
	pa  = kvtophys(pte);					/* physical */
	bzero(pte, enough_kseg2());

#undef	enough_kseg2

	/*
	 *	Make a note of it in the seg1 table
	 */

	tbia();
	pte_ktemplate(template,pa,VM_PROT_READ|VM_PROT_WRITE);
	pde = pmap_pde(kernel_pmap,K2SEG_BASE);
	i = ptes_per_vm_page;
	do {
	    *pde++ = template;
	    pte_increment_pa(template);
	    i--;
	} while (i > 0);

	/*
	 *	The kernel runs unmapped and cached (k0seg),
	 *	only dynamic data are mapped in k2seg.
	 *	==> No need to map it.
	 */

	/*
	 *	But don't we need some seg2 pagetables to start with ?
	 */
	pde = &pte[pte2num(K2SEG_BASE)];
	for (i = pmap_kernel_vm; i > 0; i--) {
	    register int j;

	    pte = (pt_entry_t *) pmap_steal_memory(PAGE_SIZE);	/* virtual */
	    pa  = kvtophys(pte);				/* physical */
	    pte_ktemplate(template,pa,VM_PROT_READ|VM_PROT_WRITE);
	    bzero(pte, PAGE_SIZE);
	    j = ptes_per_vm_page;
	    do {
		*pde++ = template;
	        pte_increment_pa(template);
	    } while (--j > 0);
	}

	/*
	 *	Assert kernel limits (cuz pmap_expand)
	 */

	virtual_avail = round_page(K2SEG_BASE);
	virtual_end   = trunc_page(K2SEG_BASE + pde2tova(pmap_kernel_vm));

	/* no console yet, so no printfs in this function */
}

pmap_rid_of_console()
{
	pt_entry_t	*pde;
	/*
	 *	Rid of console's default mappings
	 */
	for (pde = pmap_pde(kernel_pmap,0L);
	     pde < pmap_pde(kernel_pmap,VM_MIN_KERNEL_ADDRESS);)
		*pde++ = 0;
}

/*
 * Map I/O space before pmap system fully working.
 * pmap_bootstrap() must have been called already.
 */
vm_offset_t
pmap_map_io(phys, size)
	vm_offset_t	phys;
	long		size;
{
	pt_entry_t	template;
	pt_entry_t	*pte;

	pte = pmap_pte(kernel_pmap, virtual_avail);
	if (pte == PT_ENTRY_NULL) halt();	/* extreme screwup */

	pte_ktemplate(template,phys,VM_PROT_READ|VM_PROT_WRITE);

	phys = virtual_avail;
	virtual_avail += round_page(size);	

	while (size > 0) {
		*pte++ = template;
		pte_increment_pa(template);
		size -= ALPHA_PGBYTES;
	}
	tbia();

	return phys;	/* misnomer */
}

unsigned int pmap_free_pages()
{
	return atop(avail_end - avail_start);
}

vm_offset_t pmap_steal_memory(size)
	vm_size_t size;
{
	vm_offset_t addr;

	/*
	 *	We round the size to a long integer multiple.
	 */

	size = roundup(size,sizeof(integer_t));
	addr = phystokv(avail_start);
	avail_start += size;
	return addr;
}

/*
 *	Allocate permanent data structures in the k0seg.
 */
void pmap_startup(startp, endp)
	vm_offset_t *startp, *endp;
{
	register long		npages;
	vm_offset_t		addr;
	register vm_size_t	size;
	int			i;
	vm_page_t 		pages;

	/*
	 *	Allocate memory for the pv_head_table and its lock bits,
	 *	the modify bit array, and the vm_page structures.
	 */

	npages = ((BYTE_SIZE * (avail_end - avail_start)) /
		  (BYTE_SIZE * (PAGE_SIZE + sizeof *pages +
				sizeof *pv_head_table) + 2));

	size = npages * sizeof *pages;
	pages = (vm_page_t) pmap_steal_memory(size);

	size = npages * sizeof *pv_head_table;
	pv_head_table = (pv_entry_t) pmap_steal_memory(size);
	bzero((char *) pv_head_table, size);

	size = pv_lock_table_size(npages);
	pv_lock_table = (char *) pmap_steal_memory(size);
	bzero((char *) pv_lock_table, size);

	size = (npages + BYTE_SIZE - 1) / BYTE_SIZE;
	pmap_phys_attributes = (char *) pmap_steal_memory(size);
	bzero((char *) pmap_phys_attributes, size);

	avail_start = round_page(avail_start);

	if (npages > pmap_free_pages())
		panic("pmap_startup");

	for (i = 0; i < npages; i++) {
		vm_page_init(&pages[i], avail_start + ptoa(i));
		vm_page_release(&pages[i]);
	}

	*startp = virtual_avail;
	*endp = virtual_end;
}

/*
 *	Initialize the pmap module.
 *	Called by vm_init, to initialize any structures that the pmap
 *	system needs to map virtual memory.
 */
void pmap_init()
{
	vm_size_t	s;
	int		i;

	/*
	 *	Create the zone of physical maps,
	 *	and of the physical-to-virtual entries.
	 */
	s = (vm_size_t) sizeof(struct pmap);
	pmap_zone = zinit(s, 400*s, 4096, FALSE, "pmap"); /* XXX */
	s = (vm_size_t) sizeof(struct pv_entry);
	pv_list_zone = zinit(s, 10000*s, 4096, FALSE, "pv_list"); /* XXX */

#if	NCPUS > 1
	/*
	 *	Set up the pmap request lists
	 */
	for (i = 0; i < NCPUS; i++) {
	    pmap_update_list_t	up = &cpu_update_list[i];

	    simple_lock_init(&up->lock);
	    up->count = 0;
	}

	alpha_set_scb_entry( SCB_INTERPROC, pmap_update_interrupt);

#endif	/* NCPUS > 1 */

	/*
	 *	Only now, when all of the data structures are allocated,
	 *	can we set vm_first_phys and vm_last_phys.  If we set them
	 *	too soon, the kmem_alloc_wired above will try to use these
	 *	data structures and blow up.
	 */

	vm_first_phys = avail_start;
	vm_last_phys = avail_end;
	pmap_initialized = TRUE;
}

#define pmap_valid_page(x) ((avail_start <= x) && (x < avail_end))
#define valid_page(x) (pmap_initialized && pmap_valid_page(x))

/*
 *	Routine:	pmap_page_table_page_alloc
 *
 *	Allocates a new physical page to be used as a page-table page.
 *
 *	Must be called with the pmap system and the pmap unlocked,
 *	since these must be unlocked to use vm_page_grab.
 */
vm_offset_t
pmap_page_table_page_alloc()
{
	register vm_page_t	m;
	register vm_offset_t	pa;

	check_simple_locks();

	/*
	 *	We cannot allocate the pmap_object in pmap_init,
	 *	because it is called before the zone package is up.
	 *	Allocate it now if it is missing.
	 */
	if (pmap_object == VM_OBJECT_NULL)
	    pmap_object = vm_object_allocate(mem_size);

	/*
	 *	Allocate a VM page
	 */
	while ((m = vm_page_grab()) == VM_PAGE_NULL)
		VM_PAGE_WAIT((void (*)()) 0);

	/*
	 *	Map the page to its physical address so that it
	 *	can be found later.
	 */
	pa = m->phys_addr;
	vm_object_lock(pmap_object);
	vm_page_insert(m, pmap_object, pa);
	vm_page_lock_queues();
	vm_page_wire(m);
	inuse_ptepages_count++;
	vm_page_unlock_queues();
	vm_object_unlock(pmap_object);

	/*
	 *	Zero the page.
	 */
	bzero(phystokv(pa), PAGE_SIZE);

	return pa;
}

/*
 *	Deallocate a page-table page.
 *	The page-table page must have all mappings removed,
 *	and be removed from its page directory.
 */
void
pmap_page_table_page_dealloc(pa)
	vm_offset_t	pa;
{
	vm_page_t	m;

	vm_object_lock(pmap_object);
	m = vm_page_lookup(pmap_object, pa);
	if (m == VM_PAGE_NULL)
	    panic("pmap_page_table_page_dealloc: page %#X not in object", pa);
	vm_page_lock_queues();
	vm_page_free(m);
	inuse_ptepages_count--;
	vm_page_unlock_queues();
	vm_object_unlock(pmap_object);
}

/*
 *	Create and return a physical map.
 *
 *	If the size specified for the map
 *	is zero, the map is an actual physical
 *	map, and may be referenced by the
 *	hardware.
 *
 *	If the size specified is non-zero,
 *	the map will be used in software only, and
 *	is bounded by that size.
 */
pmap_t pmap_create(size)
	vm_size_t	size;
{
	register pmap_t			p;
	register pmap_statistics_t	stats;

	/*
	 *	A software use-only map doesn't even need a map.
	 */

	if (size != 0) {
		return(PMAP_NULL);
	}

/*
 *	Allocate a pmap struct from the pmap_zone.  Then allocate
 *	the page descriptor table from the pd_zone.
 */

	p = (pmap_t) zalloc(pmap_zone);
	if (p == PMAP_NULL)
		panic("pmap_create");

	if (kmem_alloc_wired(kernel_map,
			     (vm_offset_t *)&p->dirbase, ALPHA_PGBYTES)
							!= KERN_SUCCESS)
		panic("pmap_create");

	aligned_block_copy(root_kpdes, p->dirbase, ALPHA_PGBYTES);
	p->ref_count = 1;
	p->pid = -1;

	simple_lock_init(&p->lock);
	p->cpus_using = 0;
	p->hacking = 0;

	/*
	 *	Initialize statistics.
	 */

	stats = &p->stats;
	stats->resident_count = 0;
	stats->wired_count = 0;

if (pmap_debug) db_printf("pmap_create(%x->%x)\n", p, p->dirbase);
	return(p);
}

/*
 *	Retire the given physical map from service.
 *	Should only be called if the map contains
 *	no valid mappings.
 */

void pmap_destroy(p)
	register pmap_t	p;
{
	register pt_entry_t	*pdep, *ptep, *eptep;
	register vm_offset_t	pa;
	register int		c;
	register spl_t		s;

	if (p == PMAP_NULL)
		return;

	SPLVM(s);
	simple_lock(&p->lock);
	c = --p->ref_count;
	simple_unlock(&p->lock);
	SPLX(s);

	if (c != 0) {
	    return;	/* still in use */
	}

if (pmap_debug) db_printf("pmap_destroy(%x->%x)\n", p, p->dirbase);
	/*
	 *	Free the memory maps, then the
	 *	pmap structure.
	 */
	for (pdep = p->dirbase;
	     pdep < pmap_pde(p,VM_MIN_KERNEL_ADDRESS);
	     pdep += ptes_per_vm_page) {
	    if (*pdep & ALPHA_PTE_VALID) {
		pa = pte_to_pa(*pdep);

		ptep = (pt_entry_t *)phystokv(pa);
		eptep = ptep + NPTES;
		for (; ptep < eptep; ptep += ptes_per_vm_page ) {
		    if (*ptep & ALPHA_PTE_VALID)
			pmap_page_table_page_dealloc(pte_to_pa(*ptep));
		}
		pmap_page_table_page_dealloc(pa);
	    }
	}
	pmap_destroy_tlbpid(p->pid, FALSE);
	kmem_free(kernel_map, p->dirbase, ALPHA_PGBYTES);
	zfree(pmap_zone, (vm_offset_t) p);
}

/*
 *	Add a reference to the specified pmap.
 */

void pmap_reference(p)
	register pmap_t	p;
{
	spl_t	s;
	if (p != PMAP_NULL) {
		SPLVM(s);
		simple_lock(&p->lock);
		p->ref_count++;
		simple_unlock(&p->lock);
		SPLX(s);
	}
}

/*
 *	Remove a range of hardware page-table entries.
 *	The entries given are the first (inclusive)
 *	and last (exclusive) entries for the VM pages.
 *	The virtual address is the va for the first pte.
 *
 *	The pmap must be locked.
 *	If the pmap is not the kernel pmap, the range must lie
 *	entirely within one pte-page.  This is NOT checked.
 *	Assumes that the pte-page exists.
 */

/* static */
void pmap_remove_range(pmap, va, spte, epte)
	pmap_t			pmap;
	vm_offset_t		va;
	pt_entry_t		*spte;
	pt_entry_t		*epte;
{
	register pt_entry_t	*cpte;
	int			num_removed, num_unwired;
	int			pai;
	vm_offset_t		pa;

	num_removed = 0;
	num_unwired = 0;

	for (cpte = spte; cpte < epte;
	     cpte += ptes_per_vm_page, va += PAGE_SIZE) {

	    if (*cpte == 0)
		continue;
	    pa = pte_to_pa(*cpte);

	    num_removed++;
	    if (*cpte & ALPHA_PTE_WIRED)
		num_unwired++;

	    if (!valid_page(pa)) {

		/*
		 *	Outside range of managed physical memory.
		 *	Just remove the mappings.
		 */
		register int	i = ptes_per_vm_page;
		register pt_entry_t	*lpte = cpte;
		do {
		    *lpte = 0;
		    lpte++;
		} while (--i > 0);
		continue;
	    }

	    pai = pa_index(pa);
	    LOCK_PVH(pai);

	    /*
	     *	Get the modify and reference bits.
	     */
	    {
		register int		i;
		register pt_entry_t	*lpte;

		i = ptes_per_vm_page;
		lpte = cpte;
		do {
		    pmap_phys_attributes[pai] |= pte_get_attributes(lpte);
		    *lpte = 0;
		    lpte++;
		} while (--i > 0);
	    }

	    /*
	     *	Remove the mapping from the pvlist for
	     *	this physical page.
	     */
	    {
		register pv_entry_t	pv_h, prev, cur;

		pv_h = pai_to_pvh(pai);
		if (pv_h->pmap == PMAP_NULL) {
		    panic("pmap_remove: null pv_list!");
		}
		if (pv_h->va == va && pv_h->pmap == pmap) {
		    /*
		     * Header is the pv_entry.  Copy the next one
		     * to header and free the next one (we cannot
		     * free the header)
		     */
		    cur = pv_h->next;
		    if (cur != PV_ENTRY_NULL) {
			*pv_h = *cur;
			PV_FREE(cur);
		    }
		    else {
			pv_h->pmap = PMAP_NULL;
		    }
		}
		else {
		    cur = pv_h;
		    do {
			prev = cur;
			if ((cur = prev->next) == PV_ENTRY_NULL) {
			    panic("pmap-remove: mapping not in pv_list!");
			}
		    } while (cur->va != va || cur->pmap != pmap);
		    prev->next = cur->next;
		    PV_FREE(cur);
		}
		UNLOCK_PVH(pai);
	    }
	}

	/*
	 *	Update the counts
	 */
	pmap->stats.resident_count -= num_removed;
	pmap->stats.wired_count -= num_unwired;
}

/*
 *	One level up, iterate an operation on the
 *	virtual range va..eva, mapped by the 1st
 *	level pte spte.
 */

/* static */
void pmap_iterate_lev2(pmap, s, e, spte, operation)
	pmap_t			pmap;
	vm_offset_t		s, e;
	pt_entry_t		*spte;
	void			(*operation)();
{
	vm_offset_t		l;
	pt_entry_t		*epte;
	pt_entry_t		*cpte;

if (pmap_debug > 1) db_printf("iterate2(%x,%x,%x)", s, e, spte);
	while (s <  e) {
	    /* at most 1 << 23 virtuals per iteration */
	    l = roundup(s+1,PDE2_MAPPED_SIZE);
	    if (l > e)
	    	l = e;
	    if (*spte & ALPHA_PTE_VALID) {
		register int	n;
		cpte = (pt_entry_t *) ptetokv(*spte);
		n = pte3num(l);
		if (n == 0) n = SEG_MASK + 1;/* l == next segment up */
		epte = &cpte[n];
		cpte = &cpte[pte3num(s)];
if (epte < cpte) gimmeabreak();
if (pmap_debug > 1) db_printf(" [%x %x, %x %x]", s, l, cpte, epte);
		operation(pmap, s, cpte, epte);
	    }
	    s = l;
	    spte++;
	}
if (pmap_debug > 1) db_printf("\n");
}

void
pmap_make_readonly(pmap, va, spte, epte)
	pmap_t			pmap;
	vm_offset_t		va;
	pt_entry_t		*spte;
	pt_entry_t		*epte;
{
	while (spte < epte) {
	    if (*spte & ALPHA_PTE_VALID)
		*spte &= ~ALPHA_PTE_WRITE;
	    spte++;
	}
}

/*
 *	Remove the given range of addresses
 *	from the specified map.
 *
 *	It is assumed that the start and end are properly
 *	rounded to the hardware page size.
 */
vm_offset_t pmap_suspect_vs, pmap_suspect_ve;


void pmap_remove(map, s, e)
	pmap_t		map;
	vm_offset_t	s, e;
{
	spl_t			spl;
	register pt_entry_t	*pde;
	register pt_entry_t	*spte;
	vm_offset_t		l;

	if (map == PMAP_NULL)
		return;

if (pmap_debug || ((s > pmap_suspect_vs) && (s < pmap_suspect_ve))) 
db_printf("[%d]pmap_remove(%x,%x,%x)\n", cpu_number(), map, s, e);
	PMAP_READ_LOCK(map, spl);

	/*
	 *	Invalidate the translation buffer first
	 */
	PMAP_UPDATE_TLBS(map, s, e);

	pde = pmap_pde(map, s);
	while (s < e) {
	    /* at most (1 << 33) virtuals per iteration */
	    l = roundup(s+1, PDE_MAPPED_SIZE);
	    if (l > e)
		l = e;
	    if (*pde & ALPHA_PTE_VALID) {
		spte = (pt_entry_t *)ptetokv(*pde);
		spte = &spte[pte2num(s)];
		pmap_iterate_lev2(map, s, l, spte, pmap_remove_range);
	    }
	    s = l;
	    pde++;
	}

	PMAP_READ_UNLOCK(map, spl);
}

/*
 *	Routine:	pmap_page_protect
 *
 *	Function:
 *		Lower the permission for all mappings to a given
 *		page.
 */
vm_offset_t pmap_suspect_phys;

void pmap_page_protect(phys, prot)
	vm_offset_t	phys;
	vm_prot_t	prot;
{
	pv_entry_t		pv_h, prev;
	register pv_entry_t	pv_e;
	register pt_entry_t	*pte;
	int			pai;
	register pmap_t		pmap;
	spl_t			spl;
	boolean_t		remove;

if (pmap_debug || (phys == pmap_suspect_phys)) db_printf("pmap_page_protect(%x,%x)\n", phys, prot);

	assert(phys != vm_page_fictitious_addr);
	if (!valid_page(phys)) {
	    /*
	     *	Not a managed page.
	     */
	    return;
	}

	/*
	 * Determine the new protection.
	 */
	switch (prot) {
	    case VM_PROT_READ:
	    case VM_PROT_READ|VM_PROT_EXECUTE:
		remove = FALSE;
		break;
	    case VM_PROT_ALL:
		return;	/* nothing to do */
	    default:
		remove = TRUE;
		break;
	}

	/*
	 *	Lock the pmap system first, since we will be changing
	 *	several pmaps.
	 */

	PMAP_WRITE_LOCK(spl);

	pai = pa_index(phys);
	pv_h = pai_to_pvh(pai);

	/*
	 * Walk down PV list, changing or removing all mappings.
	 * We do not have to lock the pv_list because we have
	 * the entire pmap system locked.
	 */
	if (pv_h->pmap != PMAP_NULL) {

	    prev = pv_e = pv_h;
	    do {
		pmap = pv_e->pmap;
		/*
		 * Lock the pmap to block pmap_extract and similar routines.
		 */
		simple_lock(&pmap->lock);

		{
		    register vm_offset_t va;

		    va = pv_e->va;
		    pte = pmap_pte(pmap, va);

		    /*
		     * Consistency checks.
		     */
		    /* assert(*pte & ALPHA_PTE_VALID); XXX */
		    /* assert(pte_to_phys(*pte) == phys); */

		    /*
		     * Invalidate TLBs for all CPUs using this mapping.
		     */
		    PMAP_UPDATE_TLBS(pmap, va, va + PAGE_SIZE);
		}

		/*
		 * Remove the mapping if new protection is NONE
		 * or if write-protecting a kernel mapping.
		 */
		if (remove || pmap == kernel_pmap) {
		    /*
		     * Remove the mapping, collecting any modify bits.
		     */
		    if (*pte & ALPHA_PTE_WIRED)
			panic("pmap_remove_all removing a wired page");

		    {
			register int	i = ptes_per_vm_page;

			do {
			    pmap_phys_attributes[pai] |= pte_get_attributes(pte);
			    *pte++ = 0;
			} while (--i > 0);
		    }

		    pmap->stats.resident_count--;

		    /*
		     * Remove the pv_entry.
		     */
		    if (pv_e == pv_h) {
			/*
			 * Fix up head later.
			 */
			pv_h->pmap = PMAP_NULL;
		    }
		    else {
			/*
			 * Delete this entry.
			 */
			prev->next = pv_e->next;
			PV_FREE(pv_e);
		    }
		}
		else {
		    /*
		     * Write-protect.
		     */
		    register int i = ptes_per_vm_page;

		    do {
			*pte &= ~ALPHA_PTE_WRITE;
			pte++;
		    } while (--i > 0);

		    /*
		     * Advance prev.
		     */
		    prev = pv_e;
		}

		simple_unlock(&pmap->lock);

	    } while ((pv_e = prev->next) != PV_ENTRY_NULL);

	    /*
	     * If pv_head mapping was removed, fix it up.
	     */
	    if (pv_h->pmap == PMAP_NULL) {
		pv_e = pv_h->next;
		if (pv_e != PV_ENTRY_NULL) {
		    *pv_h = *pv_e;
		    PV_FREE(pv_e);
		}
	    }
	}

	PMAP_WRITE_UNLOCK(spl);
}

/*
 *	Set the physical protection on the
 *	specified range of this map as requested.
 *	Will not increase permissions.
 */
void pmap_protect(map, s, e, prot)
	pmap_t		map;
	vm_offset_t	s, e;
	vm_prot_t	prot;
{
	register pt_entry_t	*pde;
	register pt_entry_t	*spte, *epte;
	vm_offset_t		l;
	spl_t			spl;

	if (map == PMAP_NULL)
		return;

if (pmap_debug || ((s > pmap_suspect_vs) && (s < pmap_suspect_ve))) 
db_printf("[%d]pmap_protect(%x,%x,%x,%x)\n", cpu_number(), map, s, e, prot);
	/*
	 * Determine the new protection.
	 */
	switch (prot) {
	    case VM_PROT_READ|VM_PROT_EXECUTE:
		alphacache_Iflush();
	    case VM_PROT_READ:
		break;
	    case VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE:
		alphacache_Iflush();
	    case VM_PROT_READ|VM_PROT_WRITE:
		return;	/* nothing to do */
	    default:
		pmap_remove(map, s, e);
		return;
	}

	SPLVM(spl);
	simple_lock(&map->lock);

	/*
	 *	Invalidate the translation buffer first
	 */
	PMAP_UPDATE_TLBS(map, s, e);

	pde = pmap_pde(map, s);
	while (s < e) {
	    /* at most (1 << 33) virtuals per iteration */
	    l = roundup(s+1, PDE_MAPPED_SIZE);
	    if (l > e)
		l = e;
	    if (*pde & ALPHA_PTE_VALID) {
		spte = (pt_entry_t *)ptetokv(*pde);
		spte = &spte[pte2num(s)];
		pmap_iterate_lev2(map, s, l, spte, pmap_make_readonly);
	    }
	    s = l;
	    pde++;
	}

	simple_unlock(&map->lock);
	SPLX(spl);
}

/*
 *	Insert the given physical page (p) at
 *	the specified virtual address (v) in the
 *	target physical map with the protection requested.
 *
 *	If specified, the page will be wired down, meaning
 *	that the related pte can not be reclaimed.
 *
 *	NB:  This is the only routine which MAY NOT lazy-evaluate
 *	or lose information.  That is, this routine must actually
 *	insert this page into the given map NOW.
 */
void pmap_enter(pmap, v, pa, prot, wired)
	register pmap_t		pmap;
	vm_offset_t		v;
	register vm_offset_t	pa;
	vm_prot_t		prot;
	boolean_t		wired;
{
	register pt_entry_t	*pte;
	register pv_entry_t	pv_h;
	register int		i, pai;
	pv_entry_t		pv_e;
	pt_entry_t		template;
	spl_t			spl;
	vm_offset_t		old_pa;

	assert(pa != vm_page_fictitious_addr);
if (pmap_debug || ((v > pmap_suspect_vs) && (v < pmap_suspect_ve))) 
db_printf("[%d]pmap_enter(%x(%d), %x, %x, %x, %x)\n", cpu_number(), pmap, pmap->pid, v, pa, prot, wired);
	if (pmap == PMAP_NULL)
		return;
if (pmap->pid < 0) gimmeabreak();

	/*
	 *	Must allocate a new pvlist entry while we're unlocked;
	 *	zalloc may cause pageout (which will lock the pmap system).
	 *	If we determine we need a pvlist entry, we will unlock
	 *	and allocate one.  Then we will retry, throwing away
	 *	the allocated entry later (if we no longer need it).
	 */
	pv_e = PV_ENTRY_NULL;
Retry:
	PMAP_READ_LOCK(pmap, spl);

	/*
	 *	Expand pmap to include this pte.  Assume that
	 *	pmap is always expanded to include enough hardware
	 *	pages to map one VM page.
	 */

	while ((pte = pmap_pte(pmap, v)) == PT_ENTRY_NULL) {
		/*
		 *	Must unlock to expand the pmap.
		 */
		PMAP_READ_UNLOCK(pmap, spl);

		pmap_expand(pmap, v);

		PMAP_READ_LOCK(pmap, spl);
	}

	/*
	 *	Special case if the physical page is already mapped
	 *	at this address.
	 */
	old_pa = pte_to_pa(*pte);
	if (*pte && old_pa == pa) {
	    /*
	     *	May be changing its wired attribute or protection
	     */
		
	    if (wired && !(*pte & ALPHA_PTE_WIRED))
		pmap->stats.wired_count++;
	    else if (!wired && (*pte & ALPHA_PTE_WIRED))
		pmap->stats.wired_count--;

	    pte_template(pmap,template,pa,prot);
	    if (pmap == kernel_pmap)
		template |= ALPHA_PTE_GLOBAL;
	    if (wired)
		template |= ALPHA_PTE_WIRED;
	    PMAP_UPDATE_TLBS(pmap, v, v + PAGE_SIZE);
	    i = ptes_per_vm_page;
	    do {
		template |= (*pte & ALPHA_PTE_MOD);
		*pte = template;
		pte++;
		pte_increment_pa(template);
	    } while (--i > 0);
	}
	else {

	    /*
	     *	Remove old mapping from the PV list if necessary.
	     */
	    if (*pte) {
		/*
		 *	Invalidate the translation buffer,
		 *	then remove the mapping.
		 */
		PMAP_UPDATE_TLBS(pmap, v, v + PAGE_SIZE);

		/*
		 *	Don't free the pte page if removing last
		 *	mapping - we will immediately replace it.
		 */
		pmap_remove_range(pmap, v, pte,
				  pte + ptes_per_vm_page);
	    }

	    if (valid_page(pa)) {

		/*
		 *	Enter the mapping in the PV list for this
		 *	physical page.
		 */

		pai = pa_index(pa);
		LOCK_PVH(pai);
		pv_h = pai_to_pvh(pai);

		if (pv_h->pmap == PMAP_NULL) {
		    /*
		     *	No mappings yet
		     */
		    pv_h->va = v;
		    pv_h->pmap = pmap;
		    pv_h->next = PV_ENTRY_NULL;
		    if (prot & VM_PROT_EXECUTE)
			alphacache_Iflush();
		}
		else {
#if	DEBUG
		    {
			/* check that this mapping is not already there */
			pv_entry_t	e = pv_h;
			while (e != PV_ENTRY_NULL) {
			    if (e->pmap == pmap && e->va == v)
				panic("pmap_enter: already in pv_list");
			    e = e->next;
			}
		    }
#endif	/* DEBUG */
		    
		    /*
		     *	Add new pv_entry after header.
		     */
		    if (pv_e == PV_ENTRY_NULL) {
			PV_ALLOC(pv_e);
			if (pv_e == PV_ENTRY_NULL) {
			    UNLOCK_PVH(pai);
			    PMAP_READ_UNLOCK(pmap, spl);

			    /*
			     * Refill from zone.
			     */
			    pv_e = (pv_entry_t) zalloc(pv_list_zone);
			    goto Retry;
			}
		    }
		    pv_e->va = v;
		    pv_e->pmap = pmap;
		    pv_e->next = pv_h->next;
		    pv_h->next = pv_e;
		    /*
		     *	Remember that we used the pvlist entry.
		     */
		    pv_e = PV_ENTRY_NULL;
		}
		UNLOCK_PVH(pai);
	    }

	    /*
	     *	And count the mapping.
	     */

	    pmap->stats.resident_count++;
	    if (wired)
		pmap->stats.wired_count++;

	    /*
	     *	Build a template to speed up entering -
	     *	only the pfn changes.
	     */
	    pte_template(pmap,template,pa,prot);
	    if (pmap == kernel_pmap)
		template |= ALPHA_PTE_GLOBAL;
	    if (wired)
		template |= ALPHA_PTE_WIRED;
	    i = ptes_per_vm_page;
	    do {
		*pte = template;
		pte++;
		pte_increment_pa(template);
	    } while (--i > 0);
	}

	if (pv_e != PV_ENTRY_NULL) {
	    PV_FREE(pv_e);
	}

	PMAP_READ_UNLOCK(pmap, spl);
}

/*
 *	Routine:	pmap_change_wiring
 *	Function:	Change the wiring attribute for a map/virtual-address
 *			pair.
 *	In/out conditions:
 *			The mapping must already exist in the pmap.
 */
void pmap_change_wiring(map, v, wired)
	register pmap_t	map;
	vm_offset_t	v;
	boolean_t	wired;
{
	register pt_entry_t	*pte;
	register int		i;
	spl_t			spl;

if (pmap_debug) db_printf("pmap_change_wiring(%x,%x,%x)\n", map, v, wired);
	/*
	 *	We must grab the pmap system lock because we may
	 *	change a pte_page queue.
	 */
	PMAP_READ_LOCK(map, spl);

	if ((pte = pmap_pte(map, v)) == PT_ENTRY_NULL)
		panic("pmap_change_wiring: pte missing");

	if (wired && !(*pte & ALPHA_PTE_WIRED)) {
	    /*
	     *	wiring down mapping
	     */
	    map->stats.wired_count++;
	    i = ptes_per_vm_page;
	    do {
		*pte++ |= ALPHA_PTE_WIRED;
	    } while (--i > 0);
	}
	else if (!wired && (*pte & ALPHA_PTE_WIRED)) {
	    /*
	     *	unwiring mapping
	     */
	    map->stats.wired_count--;
	    i = ptes_per_vm_page;
	    do {
		*pte &= ~ALPHA_PTE_WIRED;
	    } while (--i > 0);
	}

	PMAP_READ_UNLOCK(map, spl);
}

/*
 *	Routine:	pmap_extract
 *	Function:
 *		Extract the physical page address associated
 *		with the given map/virtual_address pair.
 */

vm_offset_t pmap_extract(pmap, va)
	register pmap_t	pmap;
	vm_offset_t	va;
{
	register pt_entry_t	*pte;
	register vm_offset_t	pa;
	spl_t			spl;

if (pmap_debug) db_printf("[%d]pmap_extract(%x,%x)\n", cpu_number(), pmap, va);
	/*
	 *	Special translation for kernel addresses
	 *	in K1 or K0 space (directly mapped to
	 *	physical addresses).
	 */
	if (ISA_K1SEG(va))
		return K1SEG_TO_PHYS(va);
	if (ISA_K0SEG(va))
		return K0SEG_TO_PHYS(va);

	SPLVM(spl);
	simple_lock(&pmap->lock);
	if ((pte = pmap_pte(pmap, va)) == PT_ENTRY_NULL)
	    pa = (vm_offset_t) 0;
	else if (!(*pte & ALPHA_PTE_VALID))
	    pa = (vm_offset_t) 0;
	else
	    pa = pte_to_pa(*pte) + (va & ALPHA_OFFMASK);
	simple_unlock(&pmap->lock);

	/*
	 * Beware: this puts back this thread in the cpus_active set
	 */
	SPLX(spl);
	return(pa);
}

vm_offset_t pmap_resident_extract(pmap, va)
	register pmap_t	pmap;
	vm_offset_t	va;
{
	register pt_entry_t	*pte;
	register vm_offset_t	pa;

	/*
	 *	Special translation for kernel addresses
	 *	in K1 or K0 space (directly mapped to
	 *	physical addresses).
	 */
	if (ISA_K1SEG(va))
		return K1SEG_TO_PHYS(va);
	if (ISA_K0SEG(va))
		return K0SEG_TO_PHYS(va);

	if ((pte = pmap_pte(pmap, va)) == PT_ENTRY_NULL)
	    pa = (vm_offset_t) 0;
	else if (!(*pte & ALPHA_PTE_VALID))
	    pa = (vm_offset_t) 0;
	else
	    pa = pte_to_pa(*pte) + (va & ALPHA_OFFMASK);
	return(pa);
}

/*
 *	Routine:	pmap_expand
 *
 *	Expands a pmap to be able to map the specified virtual address.
 *
 *	Must be called with the pmap system and the pmap unlocked,
 *	since these must be unlocked to use vm_page_grab.
 *	Thus it must be called in a loop that checks whether the map
 *	has been expanded enough.
 */
pmap_expand(map, v)
	register pmap_t		map;
	register vm_offset_t	v;
{
	pt_entry_t		*pdp;
	register vm_page_t	m;
	register vm_offset_t	pa;
	register int		i;
	spl_t			spl;

	/* Would have to go through all maps to add this page */
	if (map == kernel_pmap)
	    panic("pmap_expand");

	/*
	 *	Allocate a VM page for the level 2 page table entries,
	 *	if not already there.
	 */
	pdp = pmap_pde(map,v);
	if ((*pdp & ALPHA_PTE_VALID) == 0) {

	    pt_entry_t	*pte;

	    pa = pmap_page_table_page_alloc();

	    /*
	     * Re-lock the pmap and check that another thread has
	     * not already allocated the page-table page.  If it
	     * has, discard the new page-table page (and try
	     * again to make sure).
	     */
	    PMAP_READ_LOCK(map, spl);

	    if (*pdp & ALPHA_PTE_VALID) {
		/*
		 * Oops...
		 */
		PMAP_READ_UNLOCK(map, spl);
		pmap_page_table_page_dealloc(pa);
		return;
	    }
	    /*
	     *	Map the page.
	     */
	    i = ptes_per_vm_page;
	    pte = pdp;
	    do {
	        pte_ktemplate(*pte,pa,VM_PROT_READ|VM_PROT_WRITE);
	        pte++;
	        pa += ALPHA_PGBYTES;
	    } while (--i > 0);
	    PMAP_READ_UNLOCK(map, spl);
	}

	/*
	 *	Allocate a level 3 page table.
	 */

	pa = pmap_page_table_page_alloc();

	/*
	 * Re-lock the pmap and check that another thread has
	 * not already allocated the page-table page.  If it
	 * has, we are done.
	 */
	PMAP_READ_LOCK(map, spl);

	if (pmap_pte(map, v) != PT_ENTRY_NULL) {
		PMAP_READ_UNLOCK(map, spl);
		pmap_page_table_page_dealloc(pa);
		return;
	}

	/*
	 *	Set the page directory entry for this page table.
	 *	If we have allocated more than one hardware page,
	 *	set several page directory entries.
	 */
	i = ptes_per_vm_page;
	pdp = (pt_entry_t *)ptetokv(*pdp);
	pdp = &pdp[pte2num(v)];
	do {
	    pte_ktemplate(*pdp,pa,VM_PROT_READ|VM_PROT_WRITE);
	    pdp++;
	    pa += ALPHA_PGBYTES;
	} while (--i > 0);
	PMAP_READ_UNLOCK(map, spl);
	return;
}

/*
 *	Copy the range specified by src_addr/len
 *	from the source map to the range dst_addr/len
 *	in the destination map.
 *
 *	This routine is only advisory and need not do anything.
 */
#if	0
void pmap_copy(dst_pmap, src_pmap, dst_addr, len, src_addr)
	pmap_t		dst_pmap;
	pmap_t		src_pmap;
	vm_offset_t	dst_addr;
	vm_size_t	len;
	vm_offset_t	src_addr;
{
#ifdef	lint
	dst_pmap++; src_pmap++; dst_addr++; len++; src_addr++;
#endif	/* lint */
}
#endif

/*
 *	Routine:	pmap_collect
 *	Function:
 *		Garbage collects the physical map system for
 *		pages which are no longer used.
 *		Success need not be guaranteed -- that is, there
 *		may well be pages which are not referenced, but
 *		others may be collected.
 *	Usage:
 *		Called by the pageout daemon when pages are scarce.
 */
void pmap_collect(p)
	pmap_t 		p;
{
#if	notyet

	register pt_entry_t	*pdp, *ptp;
	pt_entry_t		*eptp;
	vm_offset_t		pa;
	spl_t			spl;
	int			wired;

	if (p == PMAP_NULL)
		return;

	if (p == kernel_pmap)
		return;

	/*
	 *	Garbage collect map.
	 */
	PMAP_READ_LOCK(p, spl);
	PMAP_UPDATE_TLBS(p, VM_MIN_ADDRESS, VM_MAX_ADDRESS);
	pmap_destroy_tlbpid(p->pid, FALSE);

	for (pdp = p->dirbase;
	     pdp < pmap_pde(p,VM_MIN_KERNEL_ADDRESS);
	     pdp += ptes_per_vm_page)
	{
	    if (*pdp & ALPHA_PTE_VALID) {

		pa = pte_to_pa(*pdp);
		ptp = (pt_entry_t *)phystokv(pa);
		eptp = ptp + NPTES*ptes_per_vm_page;

		/*
		 * If the pte page has any wired mappings, we cannot
		 * free it.
		 */
		wired = 0;
		{
		    register pt_entry_t *ptep;
		    for (ptep = ptp; ptep < eptp; ptep++) {
			if (*ptep & ALPHA_PTE_WIRED) {
			    wired = 1;
			    break;
			}
		    }
		}
		if (!wired) {
		    /*
		     * Remove the virtual addresses mapped by this pte page.
		     */
.....		    pmap_remove_range_2(p,
				pdetova(pdp - p->dirbase),
				ptp,
				eptp);

		    /*
		     * Invalidate the page directory pointer.
		     */
		    {
			register int i = ptes_per_vm_page;
			register pt_entry_t *pdep = pdp;
			do {
			    *pdep++ = 0;
			} while (--i > 0);
		    }

		    PMAP_READ_UNLOCK(p, spl);

		    /*
		     * And free the pte page itself.
		     */
		    {
			register vm_page_t m;

			vm_object_lock(pmap_object);
			m = vm_page_lookup(pmap_object, pa);
			if (m == VM_PAGE_NULL)
			    panic("pmap_collect: pte page not in object");
			vm_page_lock_queues();
			vm_page_free(m);
			inuse_ptepages_count--;
			vm_page_unlock_queues();
			vm_object_unlock(pmap_object);
		    }

		    PMAP_READ_LOCK(p, spl);
		}
	    }
	}
	PMAP_READ_UNLOCK(p, spl);
	return;
#endif
}

/*
 *	Routine:	pmap_activate
 *	Function:
 *		Binds the given physical map to the given
 *		processor, and returns a hardware map description.
 */
#if	0
void pmap_activate(my_pmap, th, my_cpu)
	register pmap_t	my_pmap;
	thread_t	th;
	int		my_cpu;
{
	PMAP_ACTIVATE(my_pmap, th, my_cpu);
}
#endif

/*
 *	Routine:	pmap_deactivate
 *	Function:
 *		Indicates that the given physical map is no longer
 *		in use on the specified processor.  (This is a macro
 *		in pmap.h)
 */
#if	0
void pmap_deactivate(pmap, th, which_cpu)
	pmap_t		pmap;
	thread_t	th;
	int		which_cpu;
{
#ifdef	lint
	pmap++; th++; which_cpu++;
#endif
	PMAP_DEACTIVATE(pmap, th, which_cpu);
}
#endif

/*
 *	Routine:	pmap_kernel
 *	Function:
 *		Returns the physical map handle for the kernel.
 */
#if	0
pmap_t pmap_kernel()
{
    	return (kernel_pmap);
}
#endif

/*
 *	pmap_zero_page zeros the specified (machine independent) page.
 *	See machine/phys.c or machine/phys.s for implementation.
 */
#if	1
pmap_zero_page(phys)
	register vm_offset_t	phys;
{

	assert(phys != vm_page_fictitious_addr);

if (pmap_debug || (phys == pmap_suspect_phys)) db_printf("pmap_zero_page(%x)\n", phys);
	bzero(phystokv(phys), PAGE_SIZE);
}
#endif

/*
 *	pmap_copy_page copies the specified (machine independent) page.
 *	See machine/phys.c or machine/phys.s for implementation.
 */
#if 1	/* fornow */
pmap_copy_page(src, dst)
	vm_offset_t	src, dst;
{
	assert(src != vm_page_fictitious_addr);
	assert(dst != vm_page_fictitious_addr);

if (pmap_debug || (src == pmap_suspect_phys) || (dst == pmap_suspect_phys)) db_printf("pmap_copy_page(%x,%x)\n", src, dst);
	aligned_block_copy(phystokv(src), phystokv(dst), PAGE_SIZE);

}
#endif

/*
 *	Routine:	pmap_pageable
 *	Function:
 *		Make the specified pages (by pmap, offset)
 *		pageable (or not) as requested.
 *
 *		A page which is not pageable may not take
 *		a fault; therefore, its page table entry
 *		must remain valid for the duration.
 *
 *		This routine is merely advisory; pmap_enter
 *		will specify that these pages are to be wired
 *		down (or not) as appropriate.
 */
pmap_pageable(pmap, start, end, pageable)
	pmap_t		pmap;
	vm_offset_t	start;
	vm_offset_t	end;
	boolean_t	pageable;
{
#ifdef	lint
	pmap++; start++; end++; pageable++;
#endif
}

/*
 *	Clear specified attribute bits.
 */
void
phys_attribute_clear(phys, bits)
	vm_offset_t	phys;
	int		bits;
{
	pv_entry_t		pv_h;
	register pv_entry_t	pv_e;
	register pt_entry_t	*pte;
	int			pai;
	register pmap_t		pmap;
	spl_t			spl;

	assert(phys != vm_page_fictitious_addr);
	if (!valid_page(phys)) {
	    /*
	     *	Not a managed page.
	     */
	    return;
	}

	/*
	 *	Lock the pmap system first, since we will be changing
	 *	several pmaps.
	 */

	PMAP_WRITE_LOCK(spl);

	pai = pa_index(phys);
	pv_h = pai_to_pvh(pai);

	/*
	 * Walk down PV list, clearing all modify or reference bits.
	 * We do not have to lock the pv_list because we have
	 * the entire pmap system locked.
	 */
	if (pv_h->pmap != PMAP_NULL) {
	    /*
	     * There are some mappings.
	     */
	    for (pv_e = pv_h; pv_e != PV_ENTRY_NULL; pv_e = pv_e->next) {

		pmap = pv_e->pmap;
		/*
		 * Lock the pmap to block pmap_extract and similar routines.
		 */
		simple_lock(&pmap->lock);

		{
		    register vm_offset_t va;

		    va = pv_e->va;
		    pte = pmap_pte(pmap, va);

#if	0
		    /*
		     * Consistency checks.
		     */
		    assert(*pte & ALPHA_PTE_VALID);
		    /* assert(pte_to_phys(*pte) == phys); */
#endif

		    /*
		     * Invalidate TLBs for all CPUs using this mapping.
		     */
		    PMAP_UPDATE_TLBS(pmap, va, va + PAGE_SIZE);
		}

		/*
		 * Clear modify or reference bits.
		 */
		{
		    register int	i = ptes_per_vm_page;
		    do {
			*pte &= ~bits;
		    } while (--i > 0);
		}
		simple_unlock(&pmap->lock);
	    }
	}

	pmap_phys_attributes[pai] &= ~ (bits >> 16);

	PMAP_WRITE_UNLOCK(spl);
}

/*
 *	Check specified attribute bits.
 */
boolean_t
phys_attribute_test(phys, bits)
	vm_offset_t	phys;
	int		bits;
{
	pv_entry_t		pv_h;
	register pv_entry_t	pv_e;
	register pt_entry_t	*pte;
	int			pai;
	register pmap_t		pmap;
	spl_t			spl;

	assert(phys != vm_page_fictitious_addr);
	if (!valid_page(phys)) {
	    /*
	     *	Not a managed page.
	     */
	    return (FALSE);
	}

	/*
	 *	Lock the pmap system first, since we will be checking
	 *	several pmaps.
	 */

	PMAP_WRITE_LOCK(spl);

	pai = pa_index(phys);
	pv_h = pai_to_pvh(pai);

	if (pmap_phys_attributes[pai] & (bits >> 16)) {
	    PMAP_WRITE_UNLOCK(spl);
	    return (TRUE);
	}

	/*
	 * Walk down PV list, checking all mappings.
	 * We do not have to lock the pv_list because we have
	 * the entire pmap system locked.
	 */
	if (pv_h->pmap != PMAP_NULL) {
	    /*
	     * There are some mappings.
	     */
	    for (pv_e = pv_h; pv_e != PV_ENTRY_NULL; pv_e = pv_e->next) {

		pmap = pv_e->pmap;
		/*
		 * Lock the pmap to block pmap_extract and similar routines.
		 */
		simple_lock(&pmap->lock);

		{
		    register vm_offset_t va;

		    va = pv_e->va;
		    pte = pmap_pte(pmap, va);

#if	0
		    /*
		     * Consistency checks.
		     */
		    assert(*pte & ALPHA_PTE_VALID);
		    /* assert(pte_to_phys(*pte) == phys); */
#endif
		}

		/*
		 * Check modify or reference bits.
		 */
		{
		    register int	i = ptes_per_vm_page;

		    do {
			if (*pte & bits) {
			    simple_unlock(&pmap->lock);
			    PMAP_WRITE_UNLOCK(spl);
			    return (TRUE);
			}
		    } while (--i > 0);
		}
		simple_unlock(&pmap->lock);
	    }
	}
	PMAP_WRITE_UNLOCK(spl);
	return (FALSE);
}

/*
 *	Set specified attribute bits.  <ugly>
 */
void
phys_attribute_set(phys, bits)
	vm_offset_t	phys;
	int		bits;
{
	int			pai;
	spl_t			spl;

	assert(phys != vm_page_fictitious_addr);
	if (!valid_page(phys)) {
	    /*
	     *	Not a managed page.
	     */
	    return;
	}

	/*
	 *	Lock the pmap system.
	 */

	PMAP_WRITE_LOCK(spl);

	pai = pa_index(phys);
	pmap_phys_attributes[pai]  |= (bits >> 16);

	PMAP_WRITE_UNLOCK(spl);
}

/*
 *	Clear the modify bits on the specified physical page.
 */

void pmap_clear_modify(phys)
	register vm_offset_t	phys;
{
if (pmap_debug) db_printf("pmap_clear_mod(%x)\n", phys);
	phys_attribute_clear(phys, ALPHA_PTE_MOD);
}

/*
 *	Set the modify bits on the specified physical page.
 */

void pmap_set_modify(phys)
	register vm_offset_t	phys;
{
if (pmap_debug) db_printf("pmap_set_mod(%x)\n", phys);
	phys_attribute_set(phys, ALPHA_PTE_MOD);
}

/*
 *	pmap_is_modified:
 *
 *	Return whether or not the specified physical page is modified
 *	by any physical maps.
 */

boolean_t pmap_is_modified(phys)
	register vm_offset_t	phys;
{
if (pmap_debug) db_printf("pmap_is_mod(%x)\n", phys);
	return (phys_attribute_test(phys, ALPHA_PTE_MOD));
}

/*
 *	pmap_clear_reference:
 *
 *	Clear the reference bit on the specified physical page.
 */

void pmap_clear_reference(phys)
	vm_offset_t	phys;
{
if (pmap_debug) db_printf("pmap_clear_ref(%x)\n", phys);
	phys_attribute_clear(phys, ALPHA_PTE_REF);
}

/*
 *	pmap_is_referenced:
 *
 *	Return whether or not the specified physical page is referenced
 *	by any physical maps.
 */

boolean_t pmap_is_referenced(phys)
	vm_offset_t	phys;
{
if (pmap_debug) db_printf("pmap_is_ref(%x)\n", phys);
	return (phys_attribute_test(phys, ALPHA_PTE_REF));
}

#if	NCPUS > 1
/*
*	    TLB Coherence Code (TLB "shootdown" code)
* 
* Threads that belong to the same task share the same address space and
* hence share a pmap.  However, they  may run on distinct cpus and thus
* have distinct TLBs that cache page table entries. In order to guarantee
* the TLBs are consistent, whenever a pmap is changed, all threads that
* are active in that pmap must have their TLB updated. To keep track of
* this information, the set of cpus that are currently using a pmap is
* maintained within each pmap structure (cpus_using). Pmap_activate() and
* pmap_deactivate add and remove, respectively, a cpu from this set.
* Since the TLBs are not addressable over the bus, each processor must
* flush its own TLB; a processor that needs to invalidate another TLB
* needs to interrupt the processor that owns that TLB to signal the
* update.
* 
* Whenever a pmap is updated, the lock on that pmap is locked, and all
* cpus using the pmap are signaled to invalidate. All threads that need
* to activate a pmap must wait for the lock to clear to await any updates
* in progress before using the pmap. They must ACQUIRE the lock to add
* their cpu to the cpus_using set. An implicit assumption made
* throughout the TLB code is that all kernel code that runs at or higher
* than splvm blocks out update interrupts, and that such code does not
* touch pageable pages.
* 
* A shootdown interrupt serves another function besides signaling a
* processor to invalidate. The interrupt routine (pmap_update_interrupt)
* waits for the both the pmap lock (and the kernel pmap lock) to clear,
* preventing user code from making implicit pmap updates while the
* sending processor is performing its update. (This could happen via a
* user data write reference that turns on the modify bit in the page
* table). It must wait for any kernel updates that may have started
* concurrently with a user pmap update because the IPC code
* changes mappings.
* Spinning on the VALUES of the locks is sufficient (rather than
* having to acquire the locks) because any updates that occur subsequent
* to finding the lock unlocked will be signaled via another interrupt.
* (This assumes the interrupt is cleared before the low level interrupt code 
* calls pmap_update_interrupt()). 
* 
* The signaling processor must wait for any implicit updates in progress
* to terminate before continuing with its update. Thus it must wait for an
* acknowledgement of the interrupt from each processor for which such
* references could be made. For maintaining this information, a set
* cpus_active is used. A cpu is in this set if and only if it can 
* use a pmap. When pmap_update_interrupt() is entered, a cpu is removed from
* this set; when all such cpus are removed, it is safe to update.
* 
* Before attempting to acquire the update lock on a pmap, a cpu (A) must
* be at least at the priority of the interprocessor interrupt
* (splip<=splvm). Otherwise, A could grab a lock and be interrupted by a
* kernel update; it would spin forever in pmap_update_interrupt() trying
* to acquire the user pmap lock it had already acquired. Furthermore A
* must remove itself from cpus_active.  Otherwise, another cpu holding
* the lock (B) could be in the process of sending an update signal to A,
* and thus be waiting for A to remove itself from cpus_active. If A is
* spinning on the lock at priority this will never happen and a deadlock
* will result.
*/

/*
 *	Signal another CPU that it must flush its TLB
 */
void    signal_cpus(use_list, pmap, start, end)
	cpu_set		use_list;
	pmap_t		pmap;
	vm_offset_t	start, end;
{
	register int		which_cpu, j;
	register pmap_update_list_t	update_list_p;

	while ((which_cpu = ffs(use_list)) != 0) {
	    which_cpu -= 1;	/* convert to 0 origin */

	    update_list_p = &cpu_update_list[which_cpu];
	    simple_lock(&update_list_p->lock);

	    j = update_list_p->count;
	    if (j >= UPDATE_LIST_SIZE) {
		/*
		 *	list overflowed.  Change last item to
		 *	indicate overflow.
		 */
		update_list_p->item[UPDATE_LIST_SIZE-1].pmap  = kernel_pmap;
		update_list_p->item[UPDATE_LIST_SIZE-1].start = VM_MIN_ADDRESS;
		update_list_p->item[UPDATE_LIST_SIZE-1].end   = VM_MAX_KERNEL_ADDRESS;
	    }
	    else {
		update_list_p->item[j].pmap  = pmap;
		update_list_p->item[j].start = start;
		update_list_p->item[j].end   = end;
		update_list_p->count = j+1;
	    }
	    cpu_update_needed[which_cpu] = TRUE;
	    simple_unlock(&update_list_p->lock);

	    if ((cpus_idle & (1 << which_cpu)) == 0)
		interrupt_processor(which_cpu);
	    use_list &= ~(1 << which_cpu);
	}
}

void process_pmap_updates(my_pmap)
	register pmap_t		my_pmap;
{
	register int		my_cpu = cpu_number();
	register pmap_update_list_t	update_list_p;
	register int		j;
	register pmap_t		pmap;

	update_list_p = &cpu_update_list[my_cpu];
	simple_lock(&update_list_p->lock);

	for (j = 0; j < update_list_p->count; j++) {
	    pmap = update_list_p->item[j].pmap;
	    if (pmap == my_pmap ||
		pmap == kernel_pmap) {

		INVALIDATE_TLB(update_list_p->item[j].start,
				update_list_p->item[j].end);
	    }
	}
	update_list_p->count = 0;
	cpu_update_needed[my_cpu] = FALSE;
	simple_unlock(&update_list_p->lock);
}

#if	MACH_KDB

static boolean_t db_interp_int[NCPUS];
int db_inside_pmap_update[NCPUS];
int suicide_cpu;

cpu_interrupt_to_db(i)
	int i;
{
	db_interp_int[i] = TRUE;
	interrupt_processor(i);
}
#endif

/*
 *	Interrupt routine for TBIA requested from other processor.
 */
void pmap_update_interrupt()
{
	register int		my_cpu;
	register pmap_t		my_pmap;
	spl_t			s;

	my_cpu = cpu_number();

	db_inside_pmap_update[my_cpu]++;
#if	MACH_KDB
	if (db_interp_int[my_cpu]) {
		db_interp_int[my_cpu] = FALSE;
		remote_db_enter();
		/* In case another processor modified text  */
		alphacache_Iflush();
if (cpu_number() == suicide_cpu) halt();
		goto out;	/* uhmmm, maybe should do updates just in case */
	}
#endif
	/*
	 *	Exit now if we're idle.  We'll pick up the update request
	 *	when we go active, and we must not put ourselves back in
	 *	the active set because we'll never process the interrupt
	 *	while we're idle (thus hanging the system).
	 */
	if (cpus_idle & (1 << my_cpu))
	    goto out;

	if (current_thread() == THREAD_NULL)
	    my_pmap = kernel_pmap;
	else {
	    my_pmap = current_pmap();
	    if (!pmap_in_use(my_pmap, my_cpu))
		my_pmap = kernel_pmap;
	}

	/*
	 *	Raise spl to splvm (above splip) to block out pmap_extract
	 *	from IO code (which would put this cpu back in the active
	 *	set).
	 */
	s = splvm();

	do {

	    /*
	     *	Indicate that we're not using either user or kernel
	     *	pmap.
	     */
	    i_bit_clear(my_cpu, &cpus_active);

	    /*
	     *	Wait for any pmap updates in progress, on either user
	     *	or kernel pmap.
	     */
	    while (*(volatile int *)&my_pmap->lock.lock_data ||
		   *(volatile int *)&kernel_pmap->lock.lock_data)
		continue;

	    process_pmap_updates(my_pmap);

	    i_bit_set(my_cpu, &cpus_active);

	} while (cpu_update_needed[my_cpu]);
	
	splx(s);
out:
	db_inside_pmap_update[my_cpu]--;
}
#else	NCPUS > 1
/*
 *	Dummy routine to satisfy external reference.
 */
void pmap_update_interrupt()
{
	/* should never be called. */
}
#endif	/* NCPUS > 1 */

void
set_ptbr(pmap_t map, pcb_t pcb, boolean_t switchit)
{
	/* optimize later */
	vm_offset_t     pa;

	pa = pmap_resident_extract(kernel_pmap, map->dirbase);
	if (pa == 0)
		panic("set_ptbr");
	pcb->mss.hw_pcb.ptbr = alpha_btop(pa);
	if (switchit) {
		pcb->mss.hw_pcb.asn = map->pid;
		swpctxt(kvtophys((vm_offset_t) pcb), &(pcb)->mss.hw_pcb.ksp);
	}
}

/***************************************************************************
 *
 *	TLBPID Management
 *
 *	This is basically a unique number generator, with the twist
 *	that numbers are in a given range (dynamically defined).
 *	All things considered, I did it right in the MIPS case.
 */

int	pmap_max_asn	= 63;	/* Default value at boot.  Should be 2^ */

decl_simple_lock_data(static, tlbpid_lock)
static struct pmap **pids_in_use;
static int pmap_next_pid;

pmap_tlbpid_init()
{
	simple_lock_init(&tlbpid_lock);

#define	MAX_PID_EVER	1023	/* change if necessary, this is one page */
	pids_in_use = (struct pmap **)
		pmap_steal_memory( (MAX_PID_EVER+1) * sizeof(struct pmap *));
	bzero(pids_in_use, (MAX_PID_EVER+1) * sizeof(struct pmap *));
#undef	MAX_PID_EVER

	pmap_next_pid = 1;
}

/*
 * Axioms:
 *	- pmap_next_pid always points to a free one, unless the table is full;
 *	  in that case it points to a likely candidate for recycling.
 *	- pmap.pid prevents from making duplicates: if -1 there is no
 *	  pid for it, otherwise there is one and only one entry at that index.
 *
 * pmap_assign_tlbpid	provides a tlbpid for the given pmap, creating
 *			a new one if necessary
 * pmap_destroy_tlbpid	returns a tlbpid to the pool of available ones
 */

pmap_assign_tlbpid(map)
	struct pmap *map;
{
	register int pid, next_pid;

	if (map->pid < 0) {

		simple_lock(&tlbpid_lock);

		next_pid = pmap_next_pid;
		if (pids_in_use[next_pid]) {
			/* are we _really_ sure it's full ? */
			for (pid = 1; pid < pmap_max_asn; pid++)
				if (pids_in_use[pid] == PMAP_NULL) {
					/* aha! */
					next_pid = pid;
					goto got_a_free_one;
				}
			/* Table full */
			while (pids_in_use[next_pid]->cpus_using) {
				if (++next_pid == pmap_max_asn)
					next_pid = 1;
			}
			pmap_destroy_tlbpid(next_pid, TRUE);
		}
got_a_free_one:
		pids_in_use[next_pid] = map;
		map->pid = next_pid;
		if (++next_pid == pmap_max_asn)
			next_pid = 1;
		pmap_next_pid = next_pid;

		simple_unlock(&tlbpid_lock);
	}
}

pmap_destroy_tlbpid(pid, locked)
	int 		pid;
	boolean_t	locked;
{
	struct pmap    *map;

	if (pid < 0)	/* no longer in use */
		return;

	if (!locked) simple_lock(&tlbpid_lock);

	/*
	 * Make the pid available, and the map unassigned.
	 */
	map = pids_in_use[pid];
	pids_in_use[pid] = PMAP_NULL;
	map->pid = -1;

	if (!locked) simple_unlock(&tlbpid_lock);
}

#if	1 /* DEBUG */

print_pv_list()
{
	pv_entry_t	p;
	vm_offset_t	phys;

	db_printf("phys pages %x < p < %x\n", vm_first_phys, vm_last_phys);
	for (phys = vm_first_phys; phys < vm_last_phys; phys += PAGE_SIZE) {
		p = pai_to_pvh(pa_index(phys));
		if (p->pmap != PMAP_NULL) {
			db_printf("%x: %x %x\n", phys, p->pmap, p->va);
			while (p = p->next)
				db_printf("\t\t%x %x\n", p->pmap, p->va);
		}
	}
}

#endif
