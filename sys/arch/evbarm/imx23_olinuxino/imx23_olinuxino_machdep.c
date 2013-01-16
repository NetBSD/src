/* $Id: imx23_olinuxino_machdep.c,v 1.1.2.2 2013/01/16 05:32:53 yamt Exp $ */

/*
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Petri Laakso.
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

#include <sys/bus.h>
#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/lwp.h>
#include <sys/mount.h>
#include <sys/mutex.h>
#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/rnd.h>
#include <sys/termios.h>
#include <sys/types.h>

#include <uvm/uvm.h>
#include <uvm/uvm_prot.h>
#include <uvm/uvm_pmap.h>

#include <machine/db_machdep.h>
#include <machine/bootconfig.h>
#include <machine/frame.h>
#include <machine/param.h>
#include <machine/pcb.h>
#include <machine/pmap.h>

#include <arm/undefined.h>
#include <arm/arm32/machdep.h>

#include <arm/imx/imx23_digctlreg.h>
#include <arm/imx/imx23_clkctrlreg.h>
#include <arm/imx/imx23_rtcreg.h>
#include <arm/imx/imx23_uartdbgreg.h>
#include <arm/imx/imx23var.h>

#include "plcom.h"
#if (NPLCOM > 0)
#include <evbarm/dev/plcomreg.h>
#include <evbarm/dev/plcomvar.h>
#endif

#include "opt_evbarm_boardtype.h"

static vaddr_t get_ttb(void);
static void setup_real_page_tables(void);
//static void entropy_init(void);

/*
 * Static device map for i.MX23 peripheral address space.
 */
#define _A(a)	((a) & ~L1_S_OFFSET)
#define _S(s)	(((s) + L1_S_SIZE - 1) & ~(L1_S_SIZE-1))
static const struct pmap_devmap imx23_devmap[] = {
	{
		_A(APBH_BASE),			/* Virtual address. */
		_A(APBH_BASE),			/* Physical address. */
		_S(APBH_SIZE + APBX_SIZE),	/* APBX located after APBH. */
		VM_PROT_READ|VM_PROT_WRITE,	/* Protection bits. */
		PTE_NOCACHE			/* Cache attributes. */
	},
	{ 0, 0, 0, 0, 0 }
};
#undef _A
#undef _S

static vm_offset_t physical_freestart;
static vm_offset_t physical_freeend;
static u_int free_pages;
//static rndsave_t imx23_boot_rsp;

BootConfig bootconfig;
vm_offset_t physical_start;
vm_offset_t physical_end;
char *boot_args;
paddr_t msgbufphys;

extern char KERNEL_BASE_phys;
extern char KERNEL_BASE_virt;
extern char _end[];
extern char __data_start[];
extern char _edata[];
extern char __bss_start[];
extern char __bss_end__[];
extern pv_addr_t kernelstack;

extern u_int data_abort_handler_address;
extern u_int prefetch_abort_handler_address;

/* Define various stack sizes in pages. */
#define FIQ_STACK_SIZE	1
#define IRQ_STACK_SIZE	1
#define ABT_STACK_SIZE	1
#define UND_STACK_SIZE	1

/* Macros to translate between physical and virtual addresses. */
#define KERNEL_BASE_PHYS ((paddr_t)&KERNEL_BASE_phys)
#define KERNEL_BASE_VIRT ((vaddr_t)&KERNEL_BASE_virt)
#define KERN_VTOPHYS(va)						\
	((paddr_t)((vaddr_t)va - KERNEL_BASE_VIRT + KERNEL_BASE_PHYS))
#define KERN_PHYSTOV(pa)						\
	((vaddr_t)((paddr_t)pa - KERNEL_BASE_PHYS + KERNEL_BASE_VIRT))

#define KERNEL_PT_SYS		0	/* L2 table for mapping vectors page. */
#define KERNEL_PT_KERNEL	1	/* L2 table for mapping kernel. */
#define KERNEL_PT_KERNEL_NUM	4

#define KERNEL_PT_VMDATA	(KERNEL_PT_KERNEL + KERNEL_PT_KERNEL_NUM)
/* Page tables for mapping kernel VM */
#define KERNEL_PT_VMDATA_NUM	4 	/* start with 16MB of KVM */
#define NUM_KERNEL_PTS	(KERNEL_PT_VMDATA + KERNEL_PT_VMDATA_NUM)

pv_addr_t kernel_pt_table[NUM_KERNEL_PTS];

#define	KERNEL_VM_BASE	(KERNEL_BASE + 0x01000000)
#define KERNEL_VM_SIZE 	(0xf0000000 - KERNEL_VM_BASE)

#define REG_RD(reg) *(volatile uint32_t *)(reg)
#define REG_WR(reg, val)						\
do {									\
	*(volatile uint32_t *)((reg)) = val;				\
} while (0)

/*
 * Initialize everything and return new svc stack pointer.
 */
u_int
initarm(void *arg)
{

	if (set_cpufuncs())
		panic("set_cpufuncs failed");

	pmap_devmap_bootstrap(get_ttb(), imx23_devmap);

	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);

	consinit();
	//entropy_init();

	/* Talk to the user. */
#define BDSTR(s)	_BDSTR(s)
#define _BDSTR(s)	#s
	printf("\nNetBSD/evbarm (" BDSTR(EVBARM_BOARDTYPE) ") booting ...\n");
#undef BDSTR
#undef _BDSTR

	boot_args[0] = '\0';

#ifdef VERBOSE_INIT_ARM
	printf("initarm: Configuring system ...\n");
#endif

	physical_start = DRAM_BASE;
	physical_end = DRAM_BASE + MEMSIZE * 1024 * 1024;
	physmem = (physical_end - physical_start) / PAGE_SIZE;

	/* bootconfig is used by cpu_dump() and cousins. */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = DRAM_BASE;
	bootconfig.dram[0].pages = physmem;

	/*
	 * Our kernel is at the beginning of the DRAM, so set our free space to
	 * all the memory after the kernel.
	 */
	physical_freestart = KERN_VTOPHYS(round_page((vaddr_t) _end));
	physical_freeend = physical_end;
	free_pages = (physical_freeend - physical_freestart) / PAGE_SIZE;

#ifdef VERBOSE_INIT_ARM
	/* Tell the user about the memory. */
	printf("physmemory: %d pages at 0x%08lx -> 0x%08lx\n", physmem,
	    physical_start, physical_end - 1);
#endif

	/*
	 * Set up first and second level page tables. Pages of memory will be
	 * allocated and mapped for structures required for system operation.
	 * kernel_l1pt, kernel_pt_table[], systempage, irqstack, abtstack,
	 * undstack, kernelstack, msgbufphys will be set to point to the memory
	 * that was allocated for them.
	 */
	setup_real_page_tables();

#ifdef VERBOSE_INIT_ARM
	printf("freestart = 0x%08lx, free_pages = %d (0x%08x)\n",
	    physical_freestart, free_pages, free_pages);
#endif

	uvm_lwp_setuarea(&lwp0, kernelstack.pv_va);

#ifdef VERBOSE_INIT_ARM
	printf("bootstrap done.\n");
#endif

	/* Copy vectors from page0 to vectors page. */
	arm32_vector_init(ARM_VECTORS_HIGH, ARM_VEC_ALL);
#ifdef VERBOSE_INIT_ARM
	printf("init subsystems: stacks ");
#endif
	set_stackptr(PSR_FIQ32_MODE,
	    fiqstack.pv_va + FIQ_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_IRQ32_MODE,
	    irqstack.pv_va + IRQ_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_ABT32_MODE,
	    abtstack.pv_va + ABT_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_UND32_MODE,
	    undstack.pv_va + UND_STACK_SIZE * PAGE_SIZE);
#ifdef VERBOSE_INIT_ARM
	printf("vectors ");
#endif
	data_abort_handler_address = (u_int)data_abort_handler;
	prefetch_abort_handler_address = (u_int)prefetch_abort_handler;
	undefined_handler_address = (u_int)undefinedinstruction_bounce;
#ifdef VERBOSE_INIT_ARM
	printf("undefined ");
#endif
	undefined_init();
	/* Load memory into UVM. */
#ifdef VERBOSE_INIT_ARM
	printf("page ");
#endif
	uvm_setpagesize();
	uvm_page_physload(atop(physical_freestart), atop(physical_freeend),
	    atop(physical_freestart), atop(physical_freeend),
	    VM_FREELIST_DEFAULT);

	/* Boot strap pmap telling it where the kernel page table is. */
#ifdef VERBOSE_INIT_ARM
	printf("pmap ");
#endif
	pmap_bootstrap(KERNEL_VM_BASE, KERNEL_VM_BASE + KERNEL_VM_SIZE);

#ifdef VERBOSE_INIT_ARM
	printf("done.\n");
#endif

#ifdef __HAVE_MEMORY_DISK__
	md_root_setconf(memory_disk, sizeof memory_disk);
#endif

#ifdef BOOTHOWTO
	boothowto |= BOOTHOWTO;
#endif

#ifdef KGDB
	if (boothowto & RB_KDB) {
		kgdb_debug_init = 1;
		kgdb_connect(1);
	}
#endif

#ifdef DDB
	db_machine_init();
	if (boothowto & RB_KDB)
		Debugger();
#endif
	
	return kernelstack.pv_va + USPACE_SVC_STACK_TOP;
}

/*
 * Return TTBR (Translation Table Base Register) value from coprocessor.
 */
static vaddr_t
get_ttb(void)
{
	vaddr_t ttb;

	__asm volatile("mrc p15, 0, %0, c2, c0, 0" : "=r" (ttb));

	return ttb;
}

/*
 * valloc_pages() is used to allocate free memory to be used for kernel pages.
 * Virtual and physical addresses of the allocated memory are saved for the
 * later use by the structures:
 *
 * - kernel_l1pt which holds the address of the kernel's L1 translaton table.
 * - kernel_pt_table[] holds the addresses of the kernel's L2 page tables.
 *
 * pmap_link_l2pt() is used to create link from L1 table entry to the L2 page
 * table. Link is a reference to coarse page table which has 256 entries,
 * splitting the 1MB that the table describes into 4kB blocks.
 *
 * pmap_map_entry() updates the PTE in L2 PT for an VA to point to single
 * physical page previously allocated.
 *
 * pmap_map_chunk() maps a chunk of memory using the most efficient
 * mapping possible (section, large page, small page) into the provided L1 and
 * L2 tables at the specified virtual address. pmap_map_chunk() excepts linking
 * to be done before it is called for chunks smaller than a section.
 */
static void
setup_real_page_tables(void)
{
	/*
	 * Define a macro to simplify memory allocation. As we allocate the
	 * memory, make sure that we don't walk over our temporary first level
	 * translation table.
	 */
#define valloc_pages(var, np)						\
	(var).pv_pa = physical_freestart;				\
	physical_freestart += ((np) * PAGE_SIZE);			\
	if (physical_freestart > (physical_freeend - L1_TABLE_SIZE))	\
		panic("%s: out of memory", __func__);			\
	free_pages -= (np);						\
	(var).pv_va = KERN_PHYSTOV((var).pv_pa);			\
	memset((char *)(var).pv_va, 0, ((np) * PAGE_SIZE));

	int loop, pt_index;

	pt_index = 0;
	kernel_l1pt.pv_pa = 0;
	kernel_l1pt.pv_va = 0;
	for (loop = 0; loop <= NUM_KERNEL_PTS; ++loop) {
		/* Are we 16kB aligned for an L1? */
		if ((physical_freestart & (L1_TABLE_SIZE - 1)) == 0 &&
		    kernel_l1pt.pv_pa == 0) {
			valloc_pages(kernel_l1pt, L1_TABLE_SIZE / PAGE_SIZE);
		} else {
			valloc_pages(kernel_pt_table[pt_index],
			    L2_TABLE_SIZE / PAGE_SIZE);
			++pt_index;
		}
	}
 
	/* Make sure L1 page table is aligned to 16kB. */
	if (!kernel_l1pt.pv_pa ||
	    (kernel_l1pt.pv_pa & (L1_TABLE_SIZE - 1)) != 0)
		panic("%s: Failed to align the kernel page directory",
		    __func__);

	/*
	 * Allocate a page for the system page mapped to ARM_VECTORS_HIGH.
	 * This page will just contain the system vectors and can be shared by
	 * all processes.
	 */
	valloc_pages(systempage, 1);
	systempage.pv_va = ARM_VECTORS_HIGH;

	/* Allocate stacks for all modes. */
	valloc_pages(fiqstack, FIQ_STACK_SIZE);
	valloc_pages(irqstack, IRQ_STACK_SIZE);
	valloc_pages(abtstack, ABT_STACK_SIZE);
	valloc_pages(undstack, UND_STACK_SIZE);
	valloc_pages(kernelstack, UPAGES);

	/* Allocate the message buffer. */
	pv_addr_t msgbuf;
	int msgbuf_pgs = round_page(MSGBUFSIZE) / PAGE_SIZE;
	valloc_pages(msgbuf, msgbuf_pgs);
	msgbufphys = msgbuf.pv_pa;

	vaddr_t l1_va = kernel_l1pt.pv_va;
	vaddr_t l1_pa = kernel_l1pt.pv_pa;

	/* Map the L2 pages tables in the L1 page table. */

	pmap_link_l2pt(l1_va, ARM_VECTORS_HIGH & ~(0x00400000 - 1),
	    &kernel_pt_table[KERNEL_PT_SYS]);

	for (loop = 0; loop < KERNEL_PT_KERNEL_NUM; loop++)
		pmap_link_l2pt(l1_va, KERNEL_BASE + loop * 0x00400000,
		    &kernel_pt_table[KERNEL_PT_KERNEL + loop]);

	for (loop = 0; loop < KERNEL_PT_VMDATA_NUM; loop++)
		pmap_link_l2pt(l1_va, KERNEL_VM_BASE + loop * 0x00400000,
		    &kernel_pt_table[KERNEL_PT_VMDATA + loop]);

	/* Update the top of the kernel VM. */
	pmap_curmaxkvaddr =
	    KERNEL_VM_BASE + (KERNEL_PT_VMDATA_NUM * 0x00400000);

	extern char etext[];
	size_t textsize = (uintptr_t)etext - KERNEL_BASE;
	size_t totalsize = (uintptr_t)_end - KERNEL_BASE;
	u_int logical;

	textsize = (textsize + PGOFSET) & ~PGOFSET;
	totalsize = (totalsize + PGOFSET) & ~PGOFSET;

	logical = 0x00000000; /* offset of kernel in RAM */

	logical += pmap_map_chunk(l1_va, KERNEL_BASE + logical,
	    physical_start + logical, textsize,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

	logical += pmap_map_chunk(l1_va, KERNEL_BASE + logical,
	    physical_start + logical, totalsize - textsize,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

	/* Map the stack pages. */
	pmap_map_chunk(l1_va, fiqstack.pv_va, fiqstack.pv_pa,
	    FIQ_STACK_SIZE * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

	pmap_map_chunk(l1_va, irqstack.pv_va, irqstack.pv_pa,
	    IRQ_STACK_SIZE * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

	pmap_map_chunk(l1_va, abtstack.pv_va, abtstack.pv_pa,
	    ABT_STACK_SIZE * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

	pmap_map_chunk(l1_va, undstack.pv_va, undstack.pv_pa,
	    UND_STACK_SIZE * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

	pmap_map_chunk(l1_va, kernelstack.pv_va, kernelstack.pv_pa,
	    UPAGES * PAGE_SIZE, VM_PROT_READ | VM_PROT_WRITE, PTE_CACHE);

	pmap_map_chunk(l1_va, kernel_l1pt.pv_va, kernel_l1pt.pv_pa,
	    L1_TABLE_SIZE, VM_PROT_READ | VM_PROT_WRITE, PTE_PAGETABLE);

	for (loop = 0; loop < NUM_KERNEL_PTS; ++loop)
		pmap_map_chunk(l1_va, kernel_pt_table[loop].pv_va,
		    kernel_pt_table[loop].pv_pa, L2_TABLE_SIZE,
		    VM_PROT_READ|VM_PROT_WRITE, PTE_PAGETABLE);

	/* Map the vector page. */
	pmap_map_entry(l1_va, ARM_VECTORS_HIGH, systempage.pv_pa,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

	pmap_devmap_bootstrap(l1_va, imx23_devmap);

#ifdef VERBOSE_INIT_ARM
	/* Tell the user about where all the bits and pieces live. */
	printf("%22s       Physical              Virtual        Num\n", " ");
	printf("%22s Starting    Ending    Starting    Ending   Pages\n", " ");

	static const char mem_fmt[] =
	    "%20s: 0x%08lx 0x%08lx 0x%08lx 0x%08lx %d\n";
	static const char mem_fmt_nov[] =
	    "%20s: 0x%08lx 0x%08lx                       %d\n";

	printf(mem_fmt, "SDRAM", physical_start, physical_end-1,
	    KERN_PHYSTOV(physical_start), KERN_PHYSTOV(physical_end-1),
	    physmem);
	printf(mem_fmt, "text section",
	    KERN_VTOPHYS(KERNEL_BASE), KERN_VTOPHYS(etext-1),
	    (vaddr_t)KERNEL_BASE, (vaddr_t)etext-1,
	    (int)(textsize / PAGE_SIZE));
	printf(mem_fmt, "data section",
	    KERN_VTOPHYS(__data_start), KERN_VTOPHYS(_edata),
	    (vaddr_t)__data_start, (vaddr_t)_edata,
	    (int)((round_page((vaddr_t)_edata)
	    - trunc_page((vaddr_t)__data_start)) / PAGE_SIZE));
	printf(mem_fmt, "bss section",
	    KERN_VTOPHYS(__bss_start), KERN_VTOPHYS(__bss_end__),
	    (vaddr_t)__bss_start, (vaddr_t)__bss_end__,
	    (int)((round_page((vaddr_t)__bss_end__)
	    - trunc_page((vaddr_t)__bss_start)) / PAGE_SIZE));
	printf(mem_fmt, "L1 page directory",
	    kernel_l1pt.pv_pa, kernel_l1pt.pv_pa + L1_TABLE_SIZE - 1,
	    kernel_l1pt.pv_va, kernel_l1pt.pv_va + L1_TABLE_SIZE - 1,
	    L1_TABLE_SIZE / PAGE_SIZE);
	printf(mem_fmt, "Exception Vectors",
	    systempage.pv_pa, systempage.pv_pa + PAGE_SIZE - 1,
	    (vaddr_t)ARM_VECTORS_HIGH,
	    (vaddr_t)ARM_VECTORS_HIGH + PAGE_SIZE - 1, 1);
	printf(mem_fmt, "FIQ stack",
	    fiqstack.pv_pa, fiqstack.pv_pa + (FIQ_STACK_SIZE * PAGE_SIZE) - 1,
	    fiqstack.pv_va, fiqstack.pv_va + (FIQ_STACK_SIZE * PAGE_SIZE) - 1,
	    FIQ_STACK_SIZE);
	printf(mem_fmt, "IRQ stack",
	    irqstack.pv_pa, irqstack.pv_pa + (IRQ_STACK_SIZE * PAGE_SIZE) - 1,
	    irqstack.pv_va, irqstack.pv_va + (IRQ_STACK_SIZE * PAGE_SIZE) - 1,
	    IRQ_STACK_SIZE);
	printf(mem_fmt, "ABT stack",
	    abtstack.pv_pa, abtstack.pv_pa + (ABT_STACK_SIZE * PAGE_SIZE) - 1,
	    abtstack.pv_va, abtstack.pv_va + (ABT_STACK_SIZE * PAGE_SIZE) - 1,
	    ABT_STACK_SIZE);
	printf(mem_fmt, "UND stack",
	    undstack.pv_pa, undstack.pv_pa + (UND_STACK_SIZE * PAGE_SIZE) - 1,
	    undstack.pv_va, undstack.pv_va + (UND_STACK_SIZE * PAGE_SIZE) - 1,
	    UND_STACK_SIZE);
	printf(mem_fmt, "SVC stack",
	    kernelstack.pv_pa, kernelstack.pv_pa + (UPAGES * PAGE_SIZE) - 1,
	    kernelstack.pv_va, kernelstack.pv_va + (UPAGES * PAGE_SIZE) - 1,
	    UPAGES);
	printf(mem_fmt_nov, "Message Buffer",
	    msgbufphys, msgbufphys + msgbuf_pgs * PAGE_SIZE - 1, msgbuf_pgs);
	printf(mem_fmt, "Free Memory", physical_freestart, physical_freeend-1,
	    KERN_PHYSTOV(physical_freestart), KERN_PHYSTOV(physical_freeend-1),
	    free_pages);
#endif

	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);
	cpu_setttb(l1_pa, FALSE);
	cpu_tlb_flushID();
	cpu_domains(DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2));

	return;
}

/*
 * Generate initial random bits for rnd_init().
 */
#ifdef notyet
static void
entropy_init(void)
{
	uint32_t tmp;
	int loop, index;

	/* Test if HW_DIGCTL_ENTROPY is feeding random numbers. */
	tmp = REG_RD(HW_DIGCTL_BASE + HW_DIGCTL_ENTROPY);
	if (tmp == REG_RD(HW_DIGCTL_BASE + HW_DIGCTL_ENTROPY))
		return;

	index = 0;
	for (loop = 0; loop < RND_SAVEWORDS; loop++) {
		imx23_boot_rsp.data[index++] = (uint8_t)(tmp);
		imx23_boot_rsp.data[index++] = (uint8_t)(tmp>>8);
		imx23_boot_rsp.data[index++] = (uint8_t)(tmp>>16);
		imx23_boot_rsp.data[index++] = (uint8_t)(tmp>>24);
		imx23_boot_rsp.entropy += 32;
		tmp = REG_RD(HW_DIGCTL_BASE + HW_DIGCTL_ENTROPY);
	}

	extern rndsave_t *boot_rsp;
	boot_rsp = &imx23_boot_rsp;

	return;
}
#endif

/*
 * Initialize console.
 */
static struct plcom_instance imx23_pi = {
	.pi_type = PLCOM_TYPE_PL011,
	.pi_iot = &imx23_bus_space,
	.pi_size = PL011COM_UART_SIZE,
	.pi_iobase = HW_UARTDBG_BASE
};

#define PLCONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#define PLCONSPEED 115200
void
consinit(void)
{
	/* consinit() is called from also from the main(). */
	static int consinit_called = 0;

	if (consinit_called)
		return;

	plcomcnattach(&imx23_pi, PLCONSPEED, IMX23_UART_CLK, PLCONMODE, 0);

	consinit_called = 1;

	return;
}

/*
 * Reboot or halt the system.
 */
void
cpu_reboot(int howto, char *bootstr)
{
	static int cpu_reboot_called = 0;

	boothowto |= howto;
	
	/*
	 * If this is the first invocation of cpu_reboot() and the RB_NOSYNC
	 * flag is not set in howto; sync and unmount the system disks by
	 * calling vfs_shutdown(9) and set the time of day clock by calling
	 * resettodr(9).
	 */
	if (!cpu_reboot_called && !(boothowto & RB_NOSYNC)) {
		vfs_shutdown();
		resettodr();
	}

	cpu_reboot_called = 1;

	IRQdisable;	/* FIQ's stays on because they are special. */

	/*
	 * If rebooting after a crash (i.e., if RB_DUMP is set in howto, but
	 * RB_HALT is not), save a system crash dump.
	 */
	if ((boothowto & RB_DUMP) && !(boothowto & RB_HALT))
		panic("please implement crash dump!"); // XXX

	/* Run any shutdown hooks by calling pmf_system_shutdown(9). */
	pmf_system_shutdown(boothowto);

	printf("system %s.\n", boothowto & RB_HALT ? "halted" : "rebooted");

	if (boothowto & RB_HALT) {
		/* Enable i.MX233 wait-for-interrupt mode. */
		REG_WR(HW_CLKCTRL_BASE + HW_CLKCTRL_CPU,
		    (REG_RD(HW_CLKCTRL_BASE + HW_CLKCTRL_CPU) |
		    HW_CLKCTRL_CPU_INTERRUPT_WAIT));

		/* Disable FIQ's and wait for interrupt (which never arrives) */
		__asm volatile(					\
		    "mrs r0, cpsr\n\t"			\
		    "orr r0, #0x40\n\t"			\
		    "msr cpsr_c, r0\n\t"			\
		    "mov r0, #0\n\t"			\
		    "mcr p15, 0, r0, c7, c0, 4\n\t"
		);

		for(;;);

		/* NOT REACHED */
	}

	/* Reboot the system. */
	REG_WR(HW_RTC_BASE + HW_RTC_WATCHDOG, 10000);
	REG_WR(HW_RTC_BASE + HW_RTC_CTRL_SET, HW_RTC_CTRL_WATCHDOGEN);
	REG_WR(HW_RTC_BASE + HW_RTC_WATCHDOG, 0);

	for(;;);

	/* NOT REACHED */
}

/*
 * Delay us microseconds.
 */
void
delay(unsigned int us)
{
	uint32_t start;
	uint32_t now;
	uint32_t elapsed;
	uint32_t total;
	uint32_t last;

	total = 0;
	last = 0;
	start = REG_RD(HW_DIGCTL_BASE + HW_DIGCTL_MICROSECONDS);

	do {
		now = REG_RD(HW_DIGCTL_BASE + HW_DIGCTL_MICROSECONDS);

		if (start <= now)
			elapsed = now - start;
		else	/* Take care of overflow. */
			elapsed = (UINT32_MAX - start) + 1 + now;

		total += elapsed - last;
		last = elapsed;

	} while (total < us);

	return;
}
