/*	$NetBSD: at91bus.c,v 1.11.2.3 2013/01/16 05:32:44 yamt Exp $	*/

/*
 * Copyright (c) 2007 Embedtronics Oy
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
__KERNEL_RCSID(0, "$NetBSD: at91bus.c,v 1.11.2.3 2013/01/16 05:32:44 yamt Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_pmap_debug.h"

/* Define various stack sizes in pages */
#define IRQ_STACK_SIZE	8
#define ABT_STACK_SIZE	8
#ifdef IPKDB
#define UND_STACK_SIZE	16
#else
#define UND_STACK_SIZE	8
#endif

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/exec.h>
#include <sys/proc.h>
#include <sys/msgbuf.h>
#include <sys/reboot.h>
#include <sys/termios.h>
#include <sys/ksyms.h>

#include <machine/bootconfig.h>
#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <sys/bus.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <arm/undefined.h>

#include <arm/arm32/machdep.h>
#include <arm/cpufunc.h>

#include <arm/at91/at91var.h>
#include <arm/at91/at91busvar.h>
#include <arm/at91/at91dbgureg.h>

//#include <dev/cons.h>
#include <sys/termios.h>

#include "locators.h"

/* console stuff: */
#ifndef	CONSPEED
#define	CONSPEED B115200
#endif

#ifndef	CONMODE
#define	CONMODE	((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

int cnspeed = CONSPEED;
int cnmode = CONMODE;


/* kernel mapping: */
#define	KERNEL_BASE_PHYS	0x20200000
#define	KERNEL_TEXT_BASE	(KERNEL_BASE + 0x00200000)
#define	KERNEL_VM_BASE		(KERNEL_BASE + 0x01000000)
#define	KERNEL_VM_SIZE		0x0C000000



/* boot configuration: */
vm_offset_t physical_start;
vm_offset_t physical_freestart;
vm_offset_t physical_freeend;
vm_offset_t physical_freeend_low;
vm_offset_t physical_end;
u_int free_pages;

vm_offset_t msgbufphys;

//static struct arm32_dma_range dma_ranges[4];

#ifdef PMAP_DEBUG
extern int pmap_debug_level;
#endif

#define KERNEL_PT_SYS		0	/* L2 table for mapping vectors page */

#define KERNEL_PT_KERNEL	1	/* L2 table for mapping kernel */
#define	KERNEL_PT_KERNEL_NUM	4
					/* L2 tables for mapping kernel VM */ 
#define KERNEL_PT_VMDATA	(KERNEL_PT_KERNEL + KERNEL_PT_KERNEL_NUM)

#define	KERNEL_PT_VMDATA_NUM	4	/* start with 16MB of KVM */
#define NUM_KERNEL_PTS		(KERNEL_PT_VMDATA + KERNEL_PT_VMDATA_NUM)

pv_addr_t kernel_pt_table[NUM_KERNEL_PTS];

/* prototypes: */
void		consinit(void);
static int	at91bus_match(device_t, cfdata_t, void *);
static void	at91bus_attach(device_t, device_t, void *);
static int	at91bus_search(device_t, cfdata_t,
			       const int *, void *);
static int	at91bus_print(void *, const char *);
static int	at91bus_submatch(device_t, cfdata_t,
				 const int *, void *);


CFATTACH_DECL_NEW(at91bus, sizeof(struct at91bus_softc),
	at91bus_match, at91bus_attach, NULL, NULL);

struct at91bus_clocks at91bus_clocks = {0};
struct at91bus_softc *at91bus_sc = NULL;

#include "opt_at91types.h"

#ifdef	AT91RM9200
#include <arm/at91/at91rm9200busvar.h>
#endif

#ifdef	AT91SAM9260
#include <arm/at91/at91sam9260busvar.h>
#endif

#ifdef	AT91SAM9261
#include <arm/at91/at91sam9261busvar.h>
#endif

static const struct {
	uint32_t	cidr;
	const char *	name;
	const struct at91bus_machdep *machdep;
} at91_types[] = {
	{
		DBGU_CIDR_AT91RM9200,
		"AT91RM9200"
#ifdef	AT91RM9200
		, &at91rm9200bus
#endif
	},
	{
		DBGU_CIDR_AT91SAM9260,
		"AT91SAM9260"
#ifdef	AT91SAM9260
		, &at91sam9260bus
#endif
	},
	{
		DBGU_CIDR_AT91SAM9260,
		"AT91SAM9261"
#ifdef	AT91SAM9261
		, &at91sam9261bus
#endif
	},
	{
		DBGU_CIDR_AT91SAM9263,
		"AT91SAM9263"
	},
	{
		0,
		0,
		0
	}
};

uint32_t at91_chip_id;
static int at91_chip_ndx = -1;
struct at91bus_machdep at91bus_machdep = { 0 };
at91bus_tag_t at91bus_tag = 0;

static int
match_cid(void)
{
	uint32_t		cidr;
	int			i;

	/* get chip id */
	cidr = DBGUREG(DBGU_CIDR);
	at91_chip_id = cidr;

	/* do we know it? */
	for (i = 0; at91_types[i].name; i++) {
		if (cidr == at91_types[i].cidr)
			return i;
	}

	return -1;
}

int
at91bus_init(void)
{
	int i = at91_chip_ndx = match_cid();

	if (i < 0)
		panic("%s: unknown chip", __FUNCTION__);

	if (!at91_types[i].machdep)
		panic("%s: %s is not supported", __FUNCTION__, at91_types[i].name);

	memcpy(&at91bus_machdep, at91_types[i].machdep, sizeof(at91bus_machdep));
	at91bus_tag = &at91bus_machdep;

	return 0;
}

u_int
at91bus_setup(BootConfig *mem)
{
	int loop;
	int loop1;
	u_int l1pagetable;

	consinit();

#ifdef	VERBOSE_INIT_ARM
	printf("\nNetBSD/AT91 booting ...\n");
#endif

	// setup the CPU / MMU / TLB functions:
	if (set_cpufuncs())
		panic("%s: cpu not recognized", __FUNCTION__);

#ifdef	VERBOSE_INIT_ARM
	printf("%s: configuring system...\n", __FUNCTION__);
#endif

	/*
	 * Setup the variables that define the availability of
	 * physical memory.
	 */
	physical_start = mem->dram[0].address;
	physical_end = mem->dram[0].address + mem->dram[0].pages * PAGE_SIZE;

	physical_freestart = mem->dram[0].address + 0x9000ULL;
	physical_freeend = KERNEL_BASE_PHYS;
	physmem = (physical_end - physical_start) / PAGE_SIZE;

#ifdef	VERBOSE_INIT_ARM
	printf("physmemory: %d pages at 0x%08lx -> 0x%08lx\n", physmem,
	       physical_start, physical_end - 1);
#endif

	free_pages = (physical_freeend - physical_freestart) / PAGE_SIZE;

#ifdef	VERBOSE_INIT_ARM
	printf("freestart = 0x%08lx, free_pages = %d (0x%08x)\n",
	       physical_freestart, free_pages, free_pages);
#endif
	/* Define a macro to simplify memory allocation */
#define	valloc_pages(var, np)				\
	alloc_pages((var).pv_pa, (np));			\
	(var).pv_va = KERNEL_BASE + (var).pv_pa - physical_start;

#define alloc_pages(var, np)				\
	physical_freeend -= ((np) * PAGE_SIZE);		\
	if (physical_freeend < physical_freestart)	\
		panic("initarm: out of memory");	\
	(var) = physical_freeend;			\
	free_pages -= (np);				\
	memset((char *)(var), 0, ((np) * PAGE_SIZE));

	loop1 = 0;
	for (loop = 0; loop <= NUM_KERNEL_PTS; ++loop) {
		/* Are we 16KB aligned for an L1 ? */
		if (((physical_freeend - L1_TABLE_SIZE) & (L1_TABLE_SIZE - 1)) == 0
		    && kernel_l1pt.pv_pa == 0) {
			valloc_pages(kernel_l1pt, L1_TABLE_SIZE / PAGE_SIZE);
		} else {
			valloc_pages(kernel_pt_table[loop1],
			    L2_TABLE_SIZE / PAGE_SIZE);
			++loop1;
		}
	}

	/* This should never be able to happen but better confirm that. */
	if (!kernel_l1pt.pv_pa || (kernel_l1pt.pv_pa & (L1_TABLE_SIZE-1)) != 0)
		panic("initarm: Failed to align the kernel page directory");

	/*
	 * Allocate a page for the system vectors page
	 */
	valloc_pages(systempage, 1);
	systempage.pv_va = 0x00000000;

	/* Allocate stacks for all modes */
	valloc_pages(irqstack, IRQ_STACK_SIZE);
	valloc_pages(abtstack, ABT_STACK_SIZE);
	valloc_pages(undstack, UND_STACK_SIZE);
	valloc_pages(kernelstack, UPAGES);

#ifdef VERBOSE_INIT_ARM
	printf("IRQ stack: p0x%08lx v0x%08lx\n", irqstack.pv_pa,
	    irqstack.pv_va); 
	printf("ABT stack: p0x%08lx v0x%08lx\n", abtstack.pv_pa,
	    abtstack.pv_va); 
	printf("UND stack: p0x%08lx v0x%08lx\n", undstack.pv_pa,
	    undstack.pv_va); 
	printf("SVC stack: p0x%08lx v0x%08lx\n", kernelstack.pv_pa,
	    kernelstack.pv_va); 
#endif

	alloc_pages(msgbufphys, round_page(MSGBUFSIZE) / PAGE_SIZE);

	/*
	 * Ok we have allocated physical pages for the primary kernel
	 * page tables.  Save physical_freeend for when we give whats left 
	 * of memory below 2Mbyte to UVM.
	 */

	physical_freeend_low = physical_freeend;

#ifdef VERBOSE_INIT_ARM
	printf("Creating L1 page table at 0x%08lx\n", kernel_l1pt.pv_pa);
#endif

	/*
	 * Now we start construction of the L1 page table
	 * We start by mapping the L2 page tables into the L1.
	 * This means that we can replace L1 mappings later on if necessary
	 */
	l1pagetable = kernel_l1pt.pv_pa;

	/* Map the L2 pages tables in the L1 page table */
	pmap_link_l2pt(l1pagetable, 0x00000000, &kernel_pt_table[KERNEL_PT_SYS]);
	for (loop = 0; loop < KERNEL_PT_KERNEL_NUM; loop++)
		pmap_link_l2pt(l1pagetable, KERNEL_BASE + loop * 0x00400000,
		    &kernel_pt_table[KERNEL_PT_KERNEL + loop]);
	for (loop = 0; loop < KERNEL_PT_VMDATA_NUM; loop++)
		pmap_link_l2pt(l1pagetable, KERNEL_VM_BASE + loop * 0x00400000,
		    &kernel_pt_table[KERNEL_PT_VMDATA + loop]);

	/* update the top of the kernel VM */
	pmap_curmaxkvaddr =
	    KERNEL_VM_BASE + (KERNEL_PT_VMDATA_NUM * 0x00400000);

#ifdef VERBOSE_INIT_ARM
	printf("Mapping kernel\n");
#endif

	/* Now we fill in the L2 pagetable for the kernel static code/data */
	{
		extern char etext[], _end[];
		size_t textsize = (uintptr_t) etext - KERNEL_TEXT_BASE;
		size_t totalsize = (uintptr_t) _end - KERNEL_TEXT_BASE;
		u_int logical;

		textsize = (textsize + PGOFSET) & ~PGOFSET;
		totalsize = (totalsize + PGOFSET) & ~PGOFSET;

		logical = KERNEL_BASE_PHYS - mem->dram[0].address;	/* offset of kernel in RAM */
		logical += pmap_map_chunk(l1pagetable, KERNEL_BASE + logical,
		    physical_start + logical, textsize,
		    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
		logical += pmap_map_chunk(l1pagetable, KERNEL_BASE + logical,
		    physical_start + logical, totalsize - textsize,
		    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	}

#ifdef VERBOSE_INIT_ARM
	printf("Constructing L2 page tables\n");
#endif

	/* Map the stack pages */
	pmap_map_chunk(l1pagetable, irqstack.pv_va, irqstack.pv_pa,
	    IRQ_STACK_SIZE * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	pmap_map_chunk(l1pagetable, abtstack.pv_va, abtstack.pv_pa,
	    ABT_STACK_SIZE * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	pmap_map_chunk(l1pagetable, undstack.pv_va, undstack.pv_pa,
	    UND_STACK_SIZE * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	pmap_map_chunk(l1pagetable, kernelstack.pv_va, kernelstack.pv_pa,
	    UPAGES * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

	pmap_map_chunk(l1pagetable, kernel_l1pt.pv_va, kernel_l1pt.pv_pa,
	    L1_TABLE_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_PAGETABLE);

	for (loop = 0; loop < NUM_KERNEL_PTS; ++loop) {
		pmap_map_chunk(l1pagetable, kernel_pt_table[loop].pv_va,
		    kernel_pt_table[loop].pv_pa, L2_TABLE_SIZE,
		    VM_PROT_READ|VM_PROT_WRITE, PTE_PAGETABLE);
	}

	/* Map the vector page. */
	pmap_map_entry(l1pagetable, ARM_VECTORS_LOW, systempage.pv_pa,
		       VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

	/* Map the statically mapped devices. */
	pmap_devmap_bootstrap(l1pagetable, at91_devmap());

	/*
	 * Update the physical_freestart/physical_freeend/free_pages
	 * variables.
	 */
	{
		extern char _end[];

		physical_freestart = physical_start +
		    (((((uintptr_t) _end) + PGOFSET) & ~PGOFSET) -
		     KERNEL_BASE);
		physical_freeend = physical_end;
		free_pages =
		    (physical_freeend - physical_freestart) / PAGE_SIZE;
	}

	/*
	 * Now we have the real page tables in place so we can switch to them.
	 * Once this is done we will be running with the REAL kernel page
	 * tables.
	 */

	/* Switch tables */
#ifdef VERBOSE_INIT_ARM
	printf("freestart = 0x%08lx, free_pages = %d (0x%x)\n",
	       physical_freestart, free_pages, free_pages);
	printf("switching to new L1 page table  @%#lx...", kernel_l1pt.pv_pa);
#endif
	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);
	cpu_setttb(kernel_l1pt.pv_pa, true);
	cpu_tlb_flushID();
	cpu_domains(DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2));

	/*
	 * Moved from cpu_startup() as data_abort_handler() references
	 * this during uvm init
	 */
	uvm_lwp_setuarea(&lwp0, kernelstack.pv_va);

#ifdef VERBOSE_INIT_ARM
	printf("done!\n");
#endif

#ifdef VERBOSE_INIT_ARM
	printf("bootstrap done.\n");
#endif

	/* @@@@ check this out: @@@ */
	arm32_vector_init(ARM_VECTORS_LOW, ARM_VEC_ALL);

	/*
	 * Pages were allocated during the secondary bootstrap for the
	 * stacks for different CPU modes.
	 * We must now set the r13 registers in the different CPU modes to
	 * point to these stacks.
	 * Since the ARM stacks use STMFD etc. we must set r13 to the top end
	 * of the stack memory.
	 */
#ifdef VERBOSE_INIT_ARM
	printf("init subsystems: stacks ");
#endif

	set_stackptr(PSR_IRQ32_MODE,
	    irqstack.pv_va + IRQ_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_ABT32_MODE,
	    abtstack.pv_va + ABT_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_UND32_MODE,
	    undstack.pv_va + UND_STACK_SIZE * PAGE_SIZE);

	/*
	 * Well we should set a data abort handler.
	 * Once things get going this will change as we will need a proper
	 * handler.
	 * Until then we will use a handler that just panics but tells us
	 * why.
	 * Initialisation of the vectors will just panic on a data abort.
	 * This just fills in a slightly better one.
	 */
#ifdef VERBOSE_INIT_ARM
	printf("vectors ");
#endif
	data_abort_handler_address = (u_int)data_abort_handler;
	prefetch_abort_handler_address = (u_int)prefetch_abort_handler;
	undefined_handler_address = (u_int)undefinedinstruction_bounce;

	/* Initialise the undefined instruction handlers */
#ifdef VERBOSE_INIT_ARM
	printf("undefined ");
#endif
	undefined_init();

	/* Load memory into UVM. */
#ifdef VERBOSE_INIT_ARM
	printf("page ");
#endif
	uvm_setpagesize();	/* initialize PAGE_SIZE-dependent variables */
	uvm_page_physload(atop(physical_freestart), atop(physical_freeend),
	    atop(physical_freestart), atop(physical_freeend),
	    VM_FREELIST_DEFAULT);
	uvm_page_physload(atop(physical_start), atop(physical_freeend_low),
	    atop(physical_start), atop(physical_freeend_low),
	    VM_FREELIST_DEFAULT);

	/* Boot strap pmap telling it where the kernel page table is */
#ifdef VERBOSE_INIT_ARM
	printf("pmap ");
#endif
	pmap_bootstrap(KERNEL_VM_BASE, KERNEL_VM_BASE + KERNEL_VM_SIZE);

	/* Setup the IRQ system */
#ifdef VERBOSE_INIT_ARM
	printf("irq ");
#endif
	at91_intr_init();

#ifdef VERBOSE_INIT_ARM
	printf("done.\n");
#endif

#ifdef BOOTHOWTO
	boothowto = BOOTHOWTO;
#endif
	boothowto = AB_VERBOSE | AB_DEBUG; // @@@@

#ifdef IPKDB
	/* Initialise ipkdb */
	ipkdb_init();
	if (boothowto & RB_KDB)
		ipkdb_connect(0);
#endif

#ifdef DDB
	db_machine_init();
	if (boothowto & RB_KDB)
		Debugger();
#endif
#if 0
	printf("test data abort...\n");
	*((volatile uint32_t*)(0x1234567F)) = 0xdeadbeef;
#endif

#ifdef VERBOSE_INIT_ARM
  	printf("%s: returning new stack pointer 0x%lX\n", __FUNCTION__, (kernelstack.pv_va + USPACE_SVC_STACK_TOP));
#endif

	/* We return the new stack pointer address */
	return(kernelstack.pv_va + USPACE_SVC_STACK_TOP);
}

static int
at91bus_match(device_t parent, cfdata_t match, void *aux)
{
	// we could detect the device here...
	if (strcmp(match->cf_name, "at91bus") == 0)
		return 1;
	return 0;
}

static device_t
at91bus_found(device_t self, bus_addr_t addr, int pid)
{
	int locs[AT91BUSCF_NLOCS];
	struct at91bus_attach_args sa;
	struct at91bus_softc *sc;

	memset(&locs, 0, sizeof(locs));
	memset(&sa, 0, sizeof(sa));

	locs[AT91BUSCF_ADDR] = addr;
	locs[AT91BUSCF_PID]  = pid;

	sc = device_private(self);
	sa.sa_iot = sc->sc_iot;
	sa.sa_dmat = sc->sc_dmat;
	sa.sa_addr = addr;
	sa.sa_size = 1;
	sa.sa_pid = pid;

	return config_found_sm_loc(self, "at91bus", locs, &sa,
				   at91bus_print, at91bus_submatch);
}

static void
at91bus_attach(device_t parent, device_t self, void *aux)
{
	struct at91bus_softc	*sc;

	if (at91_chip_ndx < 0)
		panic("%s: at91bus_init() has not been called!", __FUNCTION__);

	sc = device_private(self);

        /* initialize bus space and bus dma things... */
	sc->sc_iot = &at91_bs_tag;
        sc->sc_dmat = at91_bus_dma_init(&at91_bd_tag);

	if (at91bus_sc == NULL)
		at91bus_sc = sc;

	printf(": %s, sclk %u.%03u kHz, mclk %u.%03u MHz, pclk %u.%03u MHz, mstclk %u.%03u, plla %u.%03u, pllb %u.%03u MHz\n",
	       at91_types[at91_chip_ndx].name,
	       AT91_SCLK / 1000U, AT91_SCLK % 1000U,
	       AT91_MCLK / 1000000U, (AT91_MCLK / 1000U) % 1000U,
	       AT91_PCLK / 1000000U, (AT91_PCLK / 1000U) % 1000U,
	       AT91_MSTCLK / 1000000U, (AT91_MSTCLK / 1000U) % 1000U,
	       AT91_PLLACLK / 1000000U, (AT91_PLLACLK / 1000U) % 1000U,
	       AT91_PLLBCLK / 1000000U, (AT91_PLLBCLK / 1000U) % 1000U);

	/*
	 *  Attach devices 
	 */
	at91_search_peripherals(self, at91bus_found);

	
	struct at91bus_attach_args sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_iot = sc->sc_iot;
	sa.sa_dmat = sc->sc_dmat;
	config_search_ia(at91bus_search, self, "at91bus", &sa);
}

int
at91bus_submatch(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct at91bus_attach_args *sa = aux;

	if (cf->cf_loc[AT91BUSCF_ADDR] == ldesc[AT91BUSCF_ADDR]
	    && cf->cf_loc[AT91BUSCF_PID] == ldesc[AT91BUSCF_PID]) {
		sa->sa_addr = cf->cf_loc[AT91BUSCF_ADDR];
		sa->sa_size = cf->cf_loc[AT91BUSCF_SIZE];
		sa->sa_pid  = cf->cf_loc[AT91BUSCF_PID];
		return (config_match(parent, cf, aux));
	} else
		return (0);
}

int
at91bus_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct at91bus_attach_args *sa = aux;

	sa->sa_addr = cf->cf_loc[AT91BUSCF_ADDR];
	sa->sa_size = cf->cf_loc[AT91BUSCF_SIZE];
	sa->sa_pid  = cf->cf_loc[AT91BUSCF_PID];

	if (config_match(parent, cf, aux) > 0)
		config_attach(parent, cf, aux, at91bus_print);

	return (0);
}

static int
at91bus_print(void *aux, const char *name)
{
        struct at91bus_attach_args *sa = (struct at91bus_attach_args*)aux;

	if (name)
		aprint_normal("%s at %s", sa->sa_pid >= 0 ? at91_peripheral_name(sa->sa_pid) : "device", name);

	if (sa->sa_size)
		aprint_normal(" at addr 0x%lx", sa->sa_addr);
	if (sa->sa_size > 1)
		aprint_normal("-0x%lx", sa->sa_addr + sa->sa_size - 1);
	if (sa->sa_pid >= 0)
		aprint_normal(" pid %d", sa->sa_pid);

	return (UNCONF);
}

void	consinit(void)
{
	static int consinit_called;

	if (consinit_called != 0)
		return;

	consinit_called = 1;

	if (at91_chip_ndx < 0)
		panic("%s: at91_init() has not been called!", __FUNCTION__);

	// call machine specific bus initialization code
	(*at91bus_tag->init)(&at91bus_clocks);

	// attach console
	(*at91bus_tag->attach_cn)(&at91_bs_tag, cnspeed, cnmode);
}
