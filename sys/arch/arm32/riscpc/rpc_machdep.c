/*	$NetBSD: rpc_machdep.c,v 1.34 2000/06/26 14:20:38 mrg Exp $	*/

/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * machdep.c
 *
 * Machine dependant functions for kernel setup
 *
 * This file needs a lot of work. 
 *
 * Created      : 17/09/94
 */

#include "opt_cputypes.h"
#include "opt_ddb.h"
#include "opt_pmap_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/reboot.h>
#include <sys/proc.h>
#include <sys/msgbuf.h>
#include <sys/exec.h>

#include <dev/cons.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <uvm/uvm.h>

#include <machine/signal.h>
#include <machine/frame.h>
#include <machine/bootconfig.h>
#include <machine/cpu.h>
#include <machine/io.h>
#include <machine/irqhandler.h>
#include <machine/katelib.h>
#include <machine/pte.h>
#include <machine/vidc.h>
#include <machine/vconsole.h>
#include <machine/undefined.h>
#include <machine/rtc.h>
#include <arm32/iomd/iomdreg.h>

#include "opt_ipkdb.h"
#ifdef HYDRA
#include "hydrabus.h"
#endif	/* HYDRA */

/*
 * Address to call from cpu_reset() to reset the machine.
 * This is machine architecture dependant as it varies depending
 * on where the ROM appears when you turn the MMU off.
 */

u_int cpu_reset_address = 0;

/* Define various stack sizes in pages */
#define IRQ_STACK_SIZE	1
#define ABT_STACK_SIZE	1
#ifdef IPKDB
#define UND_STACK_SIZE	2
#else
#define UND_STACK_SIZE	1
#endif

BootConfig bootconfig;		/* Boot config storage */
videomemory_t videomemory;	/* Video memory descriptor */

vm_offset_t physical_start;
vm_offset_t physical_freestart;
vm_offset_t physical_freeend;
vm_offset_t physical_end;
u_int free_pages;
int physmem = 0;

#ifndef PMAP_STATIC_L1S
int max_processes = 64;			/* Default number */
#endif	/* !PMAP_STATIC_L1S */

u_int videodram_size = 0;		/* Amount of DRAM to reserve for video */
vm_offset_t videodram_start;

/* Physical and virtual addresses for some global pages */
pv_addr_t systempage;
pv_addr_t irqstack;
pv_addr_t undstack;
pv_addr_t abtstack;
pv_addr_t kernelstack;
#if NHYDRABUS > 0
pv_addr_t hydrascratch;
#endif	/* NHYDRABUS */

char *boot_args = NULL;
char *boot_file = NULL;

vm_offset_t msgbufphys;

extern u_int data_abort_handler_address;
extern u_int prefetch_abort_handler_address;
extern u_int undefined_handler_address;

#ifdef PMAP_DEBUG
extern int pmap_debug_level;
#endif	/* PMAP_DEBUG */

#define	KERNEL_PT_VMEM		0	/* Page table for mapping video memory */
#define	KERNEL_PT_SYS		1	/* Page table for mapping proc0 zero page */
#define	KERNEL_PT_KERNEL	2	/* Page table for mapping kernel */
#define	KERNEL_PT_VMDATA	3	/* Page tables for mapping kernel VM */
#define	KERNEL_PT_VMDATA_NUM	(KERNEL_VM_SIZE >> (PDSHIFT + 2))
#define	NUM_KERNEL_PTS		(KERNEL_PT_VMDATA + KERNEL_PT_VMDATA_NUM)

pt_entry_t kernel_pt_table[NUM_KERNEL_PTS];

struct user *proc0paddr;

#ifdef CPU_SA110
#define CPU_SA110_CACHE_CLEAN_SIZE (0x4000 * 2)
static vaddr_t sa110_cc_base;
#endif	/* CPU_SA110 */

/* Prototypes */

void physcon_display_base	__P((u_int addr));
extern void consinit		__P((void));

void map_section	__P((vm_offset_t pt, vm_offset_t va, vm_offset_t pa,
			     int cacheable));
void map_pagetable	__P((vm_offset_t pt, vm_offset_t va, vm_offset_t pa));
void map_entry		__P((vm_offset_t pt, vm_offset_t va, vm_offset_t pa));
void map_entry_nc	__P((vm_offset_t pt, vm_offset_t va, vm_offset_t pa));
void map_entry_ro	__P((vm_offset_t pt, vm_offset_t va, vm_offset_t pa));
vm_size_t map_chunk	__P((vm_offset_t pd, vm_offset_t pt, vm_offset_t va,
			     vm_offset_t pa, vm_size_t size, u_int acc,
			     u_int flg));

void pmap_bootstrap		__P((vm_offset_t kernel_l1pt, pv_addr_t kernel_ptpt));
void data_abort_handler		__P((trapframe_t *frame));
void prefetch_abort_handler	__P((trapframe_t *frame));
void undefinedinstruction_bounce	__P((trapframe_t *frame));
void zero_page_readonly		__P((void));
void zero_page_readwrite	__P((void));

static void process_kernel_args	__P((void));

extern void dump_spl_masks	__P((void));
extern pt_entry_t *pmap_pte	__P((pmap_t pmap, vm_offset_t va));
extern void db_machine_init	__P((void));
extern void console_flush	__P((void));
extern void vidcconsole_reinit	__P((void));
extern int vidcconsole_blank	__P((struct vconsole *vc, int type));
void rpc_sa110_cc_setup		__P((void));

extern void parse_mi_bootargs	__P((char *args));
void parse_rpc_bootargs		__P((char *args));

extern void dumpsys	__P((void));
extern void hydrastop	__P((void));

/*
 * void cpu_reboot(int howto, char *bootstr)
 *
 * Reboots the system
 *
 * Deal with any syncing, unmounting, dumping and shutdown hooks,
 * then reset the CPU.
 */

/* NOTE: These variables will be removed, well some of them */

extern u_int spl_mask;
extern u_int current_mask;
extern u_int arm700bugcount;

void
cpu_reboot(howto, bootstr)
	int howto;
	char *bootstr;
{
#if NHYDRABUS > 0
	/*
	 * If we are halting the master then we should halt the slaves :-)
	 * otherwise it can get a bit disconcerting to have 4 other
	 * processors still tearing away doing things.
	 */

	hydrastop();
#endif	/* NHYDRABUS */

#ifdef DIAGNOSTIC
	printf("boot: howto=%08x curproc=%p\n", howto, curproc);

	printf("ipl_bio=%08x ipl_net=%08x ipl_tty=%08x ipl_imp=%08x\n",
	    irqmasks[IPL_BIO], irqmasks[IPL_NET], irqmasks[IPL_TTY],
	    irqmasks[IPL_IMP]);
	printf("ipl_audio=%08x ipl_clock=%08x ipl_none=%08x\n",
	    irqmasks[IPL_AUDIO], irqmasks[IPL_CLOCK], irqmasks[IPL_NONE]);

	dump_spl_masks();

	/* Did we encounter the ARM700 bug we discovered ? */
	if (arm700bugcount > 0)
		printf("ARM700 PREFETCH/SWI bug count = %d\n", arm700bugcount);
#endif	/* DIAGNOSTIC */

	/*
	 * If we are still cold then hit the air brakes
	 * and crash to earth fast
	 */
	if (cold) {
		doshutdownhooks();
		printf("Halted while still in the ICE age.\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
		printf("rebooting...\n");
		cpu_reset();
		/*NOTREACHED*/
	}

	/* Disable console buffering */
	cnpollc(1);

	/*
	 * If RB_NOSYNC was not specified sync the discs.
	 * Note: Unless cold is set to 1 here, syslogd will die during the unmount.
	 * It looks like syslogd is getting woken up only to find that it cannot
	 * page part of the binary in as the filesystem has been unmounted.
	 */
	if (!(howto & RB_NOSYNC))
		bootsync();

	/* Say NO to interrupts */
	splhigh();

	/* Do a dump if requested. */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		dumpsys();

	/*
	 * Auto reboot overload protection
	 *
	 * This code stops the kernel entering an endless loop of reboot
	 * - panic cycles. This will have the effect of stopping further
	 * reboots after it has rebooted 8 times after panics. A clean
	 * halt or reboot will reset the counter.
	 */

	/*
	 * Have we done 8 reboots in a row ? If so halt rather than reboot
	 * since 8 panics in a row without 1 clean halt means something is
	 * seriously wrong.
	 */
	if (cmos_read(RTC_ADDR_REBOOTCNT) > 8)
		howto |= RB_HALT;

	/*
	 * If we are rebooting on a panic then up the reboot count
	 * otherwise reset.
	 * This will thus be reset if the kernel changes the boot action from
	 * reboot to halt due to too any reboots.
	 */
	if (((howto & RB_HALT) == 0) && panicstr)
		cmos_write(RTC_ADDR_REBOOTCNT,
		   cmos_read(RTC_ADDR_REBOOTCNT) + 1);
	else
		cmos_write(RTC_ADDR_REBOOTCNT, 0);

	/*
	 * If we need a RiscBSD reboot, request it buy setting a bit in
	 * the CMOS RAM. This can be detected by the RiscBSD boot loader
	 * during a RISCOS boot. No other way to do this as RISCOS is in ROM.
	 */
	if ((howto & RB_HALT) == 0)
		cmos_write(RTC_ADDR_BOOTOPTS,
		    cmos_read(RTC_ADDR_BOOTOPTS) | 0x02);

	/* Run any shutdown hooks */
	doshutdownhooks();

	/* Make sure IRQ's are disabled */
	IRQdisable;

	if (howto & RB_HALT) {
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
	}

	printf("rebooting...\n");
	cpu_reset();
	/*NOTREACHED*/
}

/*
 * u_int initarm(BootConfig *bootconf)
 *
 * Initial entry point on startup. This gets called before main() is
 * entered.
 * It should be responcible for setting up everything that must be
 * in place when main is called.
 * This includes
 *   Taking a copy of the boot configuration structure.
 *   Initialising the physical console so characters can be printed.
 *   Setting up page tables for the kernel
 *   Relocating the kernel to the bottom of physical memory
 */

/* This routine is frightening mess ! This is what my mind looks like -mark */

/*
 * This code is looking even worse these days ...
 * This is the problem you get when you are booting from another Operating System
 * without a proper boot loader
 * Made even worse by the fact that if the machine does not have VRAM
 * the video memory tends to be physically sitting where we relocate the
 * kernel to.
 */

u_int
initarm(bootconf)
	BootConfig *bootconf;
{
	int loop;
	int loop1;
	u_int logical;
	u_int kerneldatasize;
	u_int l1pagetable;
	u_int l2pagetable;
	extern char page0[], page0_end[];
	struct exec *kernexec = (struct exec *)KERNEL_TEXT_BASE;
	int id;
	pv_addr_t kernel_l1pt;
	pv_addr_t kernel_ptpt;

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	set_cpufuncs();

	/* Copy the boot configuration structure */
	bootconfig = *bootconf;

	/*
	 * Initialise the video memory descriptor
	 *
	 * Note: all references to the video memory virtual/physical address
	 * should go via this structure.
	 */

	/*
	 * In the future ...
	 *
	 * All console output will be postponed until the primary bootstrap
	 * has been completed so that we have had a chance to reserve some
	 * memory for the video system if we do not have separate VRAM.
	 */

	/* Hardwire it in case we have an old boot loader */

	videomemory.vidm_vbase = bootconfig.display_start;
	videomemory.vidm_pbase = VRAM_BASE;
	videomemory.vidm_type = VIDEOMEM_TYPE_VRAM;
	videomemory.vidm_size = bootconfig.display_size;

	if (bootconfig.magic == BOOTCONFIG_MAGIC) {
		videomemory.vidm_vbase = bootconfig.display_start;
		videomemory.vidm_pbase = bootconfig.display_phys;
		videomemory.vidm_size = bootconfig.display_size;
		if (bootconfig.vram[0].pages)
			videomemory.vidm_type = VIDEOMEM_TYPE_VRAM;
		else
			videomemory.vidm_type = VIDEOMEM_TYPE_DRAM;
	}

	/*
	 * Initialise the physical console
	 * This is done in main() but for the moment we do it here so that
	 * we can use printf in initarm() before main() has been called.
	 */
	consinit();

	/* Talk to the user */
	printf("initarm...\n");

	/* Tell the user if his boot loader is too old */
	if (bootconfig.magic != BOOTCONFIG_MAGIC) {
		printf("\nNO MAGIC NUMBER IN BOOTCONFIG. PLEASE UPGRADE YOUR BOOT LOADER\n\n");
		delay(5000000);
	}

	printf("Kernel loaded from file %s\n", bootconfig.kernelname);
	printf("Kernel arg string %s\n", (char *)bootconfig.argvirtualbase);

	printf("\nBoot configuration structure reports the following memory\n");

	printf("  DRAM block 0a at %08x size %08x  DRAM block 0b at %08x size %08x\n\r",
	    bootconfig.dram[0].address,
	    bootconfig.dram[0].pages * bootconfig.pagesize,
	    bootconfig.dram[1].address,
	    bootconfig.dram[1].pages * bootconfig.pagesize);
	printf("  DRAM block 1a at %08x size %08x  DRAM block 1b at %08x size %08x\n\r",
	    bootconfig.dram[2].address,
	    bootconfig.dram[2].pages * bootconfig.pagesize,
	    bootconfig.dram[3].address,
	    bootconfig.dram[3].pages * bootconfig.pagesize);
	printf("  VRAM block 0  at %08x size %08x\n\r",
	    bootconfig.vram[0].address,
	    bootconfig.vram[0].pages * bootconfig.pagesize);

/*	printf("  videomem: VA=%08x PA=%08x\n", videomemory.vidm_vbase, videomemory.vidm_pbase);*/

	/* Check to make sure the page size is correct */
	if (NBPG != bootconfig.pagesize)
		panic("Page size is not %d bytes\n", NBPG);

	/*
	 * Ok now we have the hard bit.
	 * We have the kernel allocated up high. The rest of the memory map is
	 * available. We are still running on RISC OS page tables.
	 *
	 * We need to construct new page tables move the kernel in physical
	 * memory and switch to them.
	 *
	 * The booter will have left us 6 pages at the top of memory.
	 * Two of these are used as L2 page tables and the other 4 form the L1
	 * page table.
	 */

	/*
	 * Ok we must construct own own page table tables.
	 * Once we have these we can reorganise the memory as required
	 */

	/*
	 * We better check to make sure the booter has set up the scratch
	 * area for us correctly. We use this area to create temporary pagetables
	 * while we reorganise the memory map.
	 */

	if ((bootconfig.scratchphysicalbase & 0x3fff) != 0)
		panic("initarm: Scratch area not aligned on 16KB boundry\n");

	if ((bootconfig.scratchsize < 0xc000) != 0)
		panic("initarm: Scratch area too small (need >= 48KB)\n");

	/*
	 * Ok start the primary bootstrap.
	 * The primary bootstrap basically replaces the booter page tables with
	 * new ones that it creates in the boot scratch area. These page tables
	 * map the rest of the physical memory into the virtaul memory map.
	 * This allows low physical memory to be accessed to create the
	 * kernels page tables, relocate the kernel code from high physical
	 * memory to low physical memory etc.
	 */
	printf("initarm: Primary bootstrap ... ");

	kerneldatasize = bootconfig.kernsize + bootconfig.argsize;

	l2pagetable = bootconfig.scratchvirtualbase;
	l1pagetable = l2pagetable + 0x4000;

	if (bootconfig.vram[0].pages > 0) {
		/*
		 * Now we construct a L2 pagetables for the VRAM
 		 */
		for (logical = 0; logical < 0x200000; logical += NBPG) {
			map_entry(l2pagetable + 0x1000, logical,
			    bootconfig.vram[0].address + logical);
			map_entry(l2pagetable + 0x1000, logical + 0x200000,
			    bootconfig.vram[0].address + logical);
		}

		/*
		 * Update the videomemory structure to reflect the mapping
		 * changes
		 */
		videomemory.vidm_vbase = VMEM_VBASE;
		videomemory.vidm_pbase = VRAM_BASE;
		videomemory.vidm_type = VIDEOMEM_TYPE_VRAM;
		videomemory.vidm_size = bootconfig.vram[0].pages * NBPG;
	} else {
		if (bootconfig.display_phys != bootconfig.dram[0].address)
			panic("video DRAM is being unpredictable\n");

		/*
		 * Now we construct a L2 pagetables for the DRAM
 		 */
		for (logical = 0; logical < bootconfig.display_size;
		    logical += NBPG) {
			map_entry(l2pagetable + 0x1000, logical,
			    bootconfig.display_phys + logical);
		}

		/*
		 * Update the videomemory structure to reflect the mapping
		 * changes
		 */
		videomemory.vidm_vbase = VMEM_VBASE;
		videomemory.vidm_pbase = bootconfig.display_phys;
		videomemory.vidm_type = VIDEOMEM_TYPE_DRAM;
		videomemory.vidm_size = bootconfig.display_size;
	}

	/*
	 * Now map L2 page tables for the current kernel memory
	 * and the new kernel memory
	 */
	for (logical = 0; logical < kerneldatasize + bootconfig.scratchsize;
	    logical += NBPG) {
		map_entry(l2pagetable + 0x3000, logical,
		    bootconfig.kernphysicalbase + logical);
	}

#if NHYDRABUS > 0
	/*
	 * If we have the hydra nick the first physical page for hydra booting
	 * Needs to be 2MB aligned
	 */
	for (logical = 0; logical < 0x400000; logical += NBPG) {
		map_entry(l2pagetable + 0x2000, logical,
		    bootconfig.dram[0].address + logical + NBPG);
	}
#else	/* NHYDRABUS */
	for (logical = 0; logical < 0x400000; logical += NBPG) {
		map_entry(l2pagetable + 0x2000, logical,
		    bootconfig.dram[0].address + logical);
	}
#endif	/* NHYDRABUS */

	/*
	 * Now we construct the L1 pagetable. This only needs the minimum to
	 * keep us going until we can contruct the proper kernel L1 page table.
	 */
	map_section(l1pagetable, VIDC_BASE,  VIDC_HW_BASE, 0);
	map_section(l1pagetable, IOMD_BASE,  IOMD_HW_BASE, 0);

	map_pagetable(l1pagetable, 0x00000000,
	    bootconfig.scratchphysicalbase + 0x2000);
	map_pagetable(l1pagetable, KERNEL_BASE,
	    bootconfig.scratchphysicalbase + 0x3000);
	map_pagetable(l1pagetable, VMEM_VBASE,
	    bootconfig.scratchphysicalbase + 0x1000);

	/* Print some debugging info */
/*	printf("page tables look like this ...\n");
	printf("V0x00000000 - %08x\n", ReadWord(l1pagetable + 0x0000));
	printf("V0x00100000 - %08x\n", ReadWord(l1pagetable + 0x0004));
	printf("V0x00200000 - %08x\n", ReadWord(l1pagetable + 0x0008));
	printf("V0x00300000 - %08x\n", ReadWord(l1pagetable + 0x000C));
	printf("V0x03500000 - %08x\n", ReadWord(l1pagetable + 0x00d4));
	printf("V0x00200000 - %08x\n", ReadWord(l1pagetable + 0x0080));
	printf("V0xf0000000 - %08x\n", ReadWord(l1pagetable + 0x3c00));
	printf("V0xf0100000 - %08x\n", ReadWord(l1pagetable + 0x3c04));
	printf("V0xf0200000 - %08x\n", ReadWord(l1pagetable + 0x3c08));
	printf("V0xf0300000 - %08x\n", ReadWord(l1pagetable + 0x3c0C));
	printf("V0xf1000000 - %08x\n", ReadWord(l1pagetable + 0x3c40));
	printf("V0xf2000000 - %08x\n", ReadWord(l1pagetable + 0x3c80));
	printf("V0xf3000000 - %08x\n", ReadWord(l1pagetable + 0x3cc0));
	printf("V0xf4000000 - %08x\n", ReadWord(l1pagetable + 0x3d00));
	printf("V0xf5000000 - %08x\n", ReadWord(l1pagetable + 0x3d40));
	printf("V0xf6000000 - %08x\n", ReadWord(l1pagetable + 0x3d80));
	printf("V0xf7000000 - %08x\n", ReadWord(l1pagetable + 0x3dc0));
	printf("page dir = P%08x\n", bootconfig.scratchphysicalbase + 0x4000);
	printf("l1= V%08x\n", l1pagetable);
*/

	/* Grind to a halt if no VRAM */

/*	if (bootconfig.vram[0].pages == 0) {
		printf("Switching to bootstrap pagetables\n");
		printf("[Hit a key top continue]\n");
		cngetc();
	}*/

	/* If no VRAM kill the VIDC DAC's until the end of the bootstrap */
	if (bootconfig.vram[0].pages == 0)
		vidcconsole_blank(vconsole_current, BLANK_OFF);

	/* If we don't have VRAM ..
	 * Ahhhhhhhhhhhhhhhhhhhhhh
	 * We have just mapped the kernel across the video DRAM from RISCOS.
	 * Better block all printing until we complete the secondary
	 * bootstrap and have allocate new video DRAM.
	 */

	/*
	 * Pheww right we are ready to switch page tables !!!
	 * The L1 table is at bootconfig.scratchphysicalbase + 0x4000
	 */
 
	/* Switch tables */
	setttb(bootconfig.scratchphysicalbase + 0x4000);

	/*
	 * We must now clean the cache again....
	 * Cleaning may be done by reading new data to displace any
	 * dirty data in the cache. This will have happened in setttb()
	 * but since we are boot strapping the addresses used for the read
	 * may have just been remapped and thus the cache could be out
	 * of sync. A re-clean after the switch will cure this.
	 * After booting there are no gross reloations of the kernel thus
	 * this problem wil not occur after initarm().
	 */
	cpu_cache_cleanID();

	/*
	 * Since we have mapped the VRAM up into kernel space we must
	 * now update the bootconfig and display structures by hand.
	 */
	if (bootconfig.vram[0].pages != 0) {
		bootconfig.display_start = VMEM_VBASE;
		physcon_display_base(VMEM_VBASE);
	}

	if (bootconfig.vram[0].pages != 0)
		printf("done.\n");

	id = ReadByte(IOMD_BASE + (IOMD_ID0 << 2))
	  | (ReadByte(IOMD_BASE + (IOMD_ID1 << 2)) << 8);
	switch (id) {
	case ARM7500_IOC_ID:
#ifndef CPU_ARM7500
		panic("Encountered ARM7500 IOMD but no ARM7500 kernel support");
#endif	/* CPU_ARM7500 */
		break;
	case RPC600_IOMD_ID:
#ifdef CPU_ARM7500
		panic("Encountered ARM6/7 IOMD and ARM7500 kernel support");
#endif	/* CPU_ARM7500 */
		break;
	}

	/*
	 * Ok we have finished the primary boot strap. All this has done is to
	 * allow us to access all the physical memory from known virtual
	 * location. We also now know that all the used pages are at the top
	 * of the physical memory and where they are in the virtual memory map.
	 *
	 * This should be the stage we are at at the end of the bootstrap when
	 * we have a two stage booter.
	 *
	 * The secondary bootstrap has the responcibility to sort locating the
	 * kernel to the correct address and for creating the kernel page tables.
	 * It must also set up various memory pointers that are used by pmap etc.  
	 */
	process_kernel_args();

	if (bootconfig.vram[0].pages != 0)
		printf("initarm: Secondary bootstrap ... ");

	/* Zero down the memory we mapped in for the secondary bootstrap */
	memset(0x00000000, 0, 0x400000);	/* XXX */

	/*
	 * Set up the variables that define the availablilty of physcial
	 * memory
	 */
	physical_start = bootconfig.dram[0].address;
	physical_freestart = physical_start;
	physical_end = bootconfig.dram[bootconfig.dramblocks - 1].address
	    + bootconfig.dram[bootconfig.dramblocks - 1].pages * NBPG;
	physical_freeend = physical_end;
	free_pages = bootconfig.drampages;
    
	for (loop = 0; loop < bootconfig.dramblocks; ++loop)
		physmem += bootconfig.dram[loop].pages;
    
	/*
	 * Reserve some pages at the top of the memory for later use
	 *
	 * This area is not currently used but could be used for the allocation
	 * of L1 page tables for each process.
	 * The size of this memory would be determined by the maximum number of
	 * processes.
	 *
	 * For the moment we just reserve a few pages just to make sure the
	 * system copes.
	 */
	physical_freeend -= videodram_size;
	free_pages -= (videodram_size / NBPG);
	videodram_start = physical_freeend;

	if (videodram_size) {
		videomemory.vidm_vbase = VMEM_VBASE;
		videomemory.vidm_pbase = videodram_start;
		videomemory.vidm_type = VIDEOMEM_TYPE_DRAM;
		videomemory.vidm_size = videodram_size;
	}

	/*
	 * Right We have the bottom meg of memory mapped to 0x00000000
	 * so was can get at it. The kernel will ocupy the start of it.
	 * After the kernel/args we allocate some of the fixed page tables
	 * we need to get the system going.
	 * We allocate one page directory and 8 page tables and store the
	 * physical addresses in the kernel_pt_table array.	
	 * Must remember that neither the page L1 or L2 page tables are the
	 * same size as a page !
	 *
	 * Ok the next bit of physical allocate may look complex but it is
	 * simple really. I have done it like this so that no memory gets
	 * wasted during the allocate of various pages and tables that are
	 * all different sizes.
	 * The start address will be page aligned.
	 * We allocate the kernel page directory on the first free 16KB
	 * boundry we find.
	 * We allocate the kernel page tables on the first 1KB boundry we find.
	 * We allocate 9 PT's. This means that in the process we
	 * KNOW that we will encounter at least 1 16KB boundry.
	 *
	 * Eventually if the top end of the memory gets used for process L1
	 * page tables the kernel L1 page table may be moved up there.
	 */

#ifdef VERBOSE_INIT_ARM
	printf("Allocating page tables\n");
#endif

#if NHYDRABUS > 0
	/*
	 * The Simtec Hydra board needs a 2MB aligned page for bootstrapping.
	 * Simplest thing is to nick the bottom page of physical memory.
	 */

	hydrascratch.pv_pa = physical_start;
	physical_start += NBPG;
	--free_pages;
#endif	/* NHYDRABUS */

	/* Update the address of the first free page of physical memory */
	physical_freestart = physical_start + kerneldatasize;
	free_pages -= (physical_freestart - physical_start) / NBPG;

	/* Define a macro to simplify memory allocation */
#define	valloc_pages(var, np)			\
	alloc_pages((var).pv_pa, (np));	\
	(var).pv_va = KERNEL_BASE + (var).pv_pa - physical_start;

#define alloc_pages(var, np)			\
	(var) = physical_freestart;		\
	physical_freestart += ((np) * NBPG);	\
	free_pages -= (np);			\
	memset((char *)(var) - physical_start, 0, ((np) * NBPG));

	loop1 = 0;
	kernel_l1pt.pv_pa = 0;
	for (loop = 0; loop <= NUM_KERNEL_PTS; ++loop) {
		/* Are we 16KB aligned for an L1 ? */
		if ((physical_freestart & (PD_SIZE - 1)) == 0
		    && kernel_l1pt.pv_pa == 0) {
			valloc_pages(kernel_l1pt, PD_SIZE / NBPG);
		} else {
			alloc_pages(kernel_pt_table[loop1], PT_SIZE / NBPG);
			++loop1;
		}
	}

#ifdef DIAGNOSTIC
	/* This should never be able to happen but better confirm that. */
	if (!kernel_l1pt.pv_pa || (kernel_l1pt.pv_pa & (PD_SIZE-1)) != 0)
		panic("initarm: Failed to align the kernel page directory\n");
#endif

	/*
	 * Allocate a page for the system page mapped to V0x00000000
	 * This page will just contain the system vectors and can be
	 * shared by all processes.
	 */
	alloc_pages(systempage.pv_pa, 1);

	/* Allocate a page for the page table to map kernel page tables*/
	valloc_pages(kernel_ptpt, PT_SIZE / NBPG);

	/* Allocate stacks for all modes */
	valloc_pages(irqstack, IRQ_STACK_SIZE);
	valloc_pages(abtstack, ABT_STACK_SIZE);
	valloc_pages(undstack, UND_STACK_SIZE);
	valloc_pages(kernelstack, UPAGES);

#ifdef VERBOSE_INIT_ARM
	printf("IRQ stack: p0x%08lx v0x%08lx\n", irqstack.pv_pa, irqstack.pv_va); 
	printf("ABT stack: p0x%08lx v0x%08lx\n", abtstack.pv_pa, abtstack.pv_va); 
	printf("UND stack: p0x%08lx v0x%08lx\n", undstack.pv_pa, undstack.pv_va); 
	printf("SVC stack: p0x%08lx v0x%08lx\n", kernelstack.pv_pa, kernelstack.pv_va); 
#endif

	alloc_pages(msgbufphys, round_page(MSGBUFSIZE) / NBPG);

#ifdef CPU_SA110
	/*
	 * XXX totally stuffed hack to work round problems introduced
	 * in recent versions of the pmap code. Due to the calls used there
	 * we cannot allocate virtual memory during bootstrap.
	 */
	sa110_cc_base = (KERNEL_BASE + (physical_freestart - physical_start)
	    + (CPU_SA110_CACHE_CLEAN_SIZE - 1))
	    & ~(CPU_SA110_CACHE_CLEAN_SIZE - 1);
#endif	/* CPU_SA110 */

	/*
	 * Ok we have allocated physical pages for the primary kernel
	 * page tables
	 */

#ifdef VERBOSE_INIT_ARM
	printf("Creating L1 page table\n");
#endif

	/*
	 * Now we start consturction of the L1 page table
	 * We start by mapping the L2 page tables into the L1.
	 * This means that we can replace L1 mappings later on if necessary
	 */
	l1pagetable = kernel_l1pt.pv_pa - physical_start;

	/* Map the L2 pages tables in the L1 page table */
	map_pagetable(l1pagetable, 0x00000000,
	    kernel_pt_table[KERNEL_PT_SYS]);
	map_pagetable(l1pagetable, KERNEL_BASE,
	    kernel_pt_table[KERNEL_PT_KERNEL]);
	for (loop = 0; loop < KERNEL_PT_VMDATA_NUM; ++loop)
		map_pagetable(l1pagetable, KERNEL_VM_BASE + loop * 0x00400000,
		    kernel_pt_table[KERNEL_PT_VMDATA + loop]);
	map_pagetable(l1pagetable, PROCESS_PAGE_TBLS_BASE,
	    kernel_ptpt.pv_pa);
	map_pagetable(l1pagetable, VMEM_VBASE,
	    kernel_pt_table[KERNEL_PT_VMEM]);


#ifdef VERBOSE_INIT_ARM
	printf("Mapping kernel\n");
#endif

	/* Now we fill in the L2 pagetable for the kernel code/data */
	l2pagetable = kernel_pt_table[KERNEL_PT_KERNEL] - physical_start;

	if (N_GETMAGIC(kernexec[0]) == ZMAGIC) {
		/*
		 * This is a work around for a recent problem that occurred
		 * with ARM 610 processors and some ARM 710 processors
		 * Other ARM 710 and StrongARM processors don't have a problem.
		 */
#if defined(CPU_ARM6) || defined(CPU_ARM7)
		logical = map_chunk(0, l2pagetable, KERNEL_TEXT_BASE,
		    physical_start, kernexec->a_text,
		    AP_KRW, PT_CACHEABLE);
#else	/* CPU_ARM6 || CPU_ARM7 */
		logical = map_chunk(0, l2pagetable, KERNEL_TEXT_BASE,
		    physical_start, kernexec->a_text,
		    AP_KR, PT_CACHEABLE);
#endif	/* CPU_ARM6 || CPU_ARM7 */
		logical += map_chunk(0, l2pagetable, KERNEL_TEXT_BASE + logical,
		    physical_start + logical, kerneldatasize - kernexec->a_text,
		    AP_KRW, PT_CACHEABLE);
	} else
		map_chunk(0, l2pagetable, KERNEL_TEXT_BASE,
		    physical_start, kerneldatasize,
		    AP_KRW, PT_CACHEABLE);

#ifdef VERBOSE_INIT_ARM
	printf("Constructing L2 page tables\n");
#endif

	/* Map the stack pages */
	map_chunk(0, l2pagetable, irqstack.pv_va, irqstack.pv_pa,
	    IRQ_STACK_SIZE * NBPG, AP_KRW, PT_CACHEABLE);
	map_chunk(0, l2pagetable, abtstack.pv_va, abtstack.pv_pa,
	    ABT_STACK_SIZE * NBPG, AP_KRW, PT_CACHEABLE);
	map_chunk(0, l2pagetable, undstack.pv_va, undstack.pv_pa,
	    UND_STACK_SIZE * NBPG, AP_KRW, PT_CACHEABLE);
	map_chunk(0, l2pagetable, kernelstack.pv_va, kernelstack.pv_pa,
	    UPAGES * NBPG, AP_KRW, PT_CACHEABLE);
	map_chunk(0, l2pagetable, kernel_l1pt.pv_va, kernel_l1pt.pv_pa,
	    PD_SIZE, AP_KRW, 0);

	/* Map the page table that maps the kernel pages */
	map_entry_nc(l2pagetable, kernel_ptpt.pv_pa - physical_start,
	    kernel_ptpt.pv_pa);

	/* Now we fill in the L2 pagetable for the VRAM */

	/*
	 * Current architectures mean that the VRAM is always in 1 continuous
	 * bank.
	 * This means that we can just map the 2 meg that the VRAM would occupy.
	 * In theory we don't need a page table for VRAM, we could section map
	 * it but we would need the page tables if DRAM was in use.
	 */
	l2pagetable = kernel_pt_table[KERNEL_PT_VMEM] - physical_start;

	map_chunk(0, l2pagetable, VMEM_VBASE, videomemory.vidm_pbase,
	    videomemory.vidm_size, AP_KRW, PT_CACHEABLE);
	map_chunk(0, l2pagetable, VMEM_VBASE + videomemory.vidm_size,
	    videomemory.vidm_pbase, videomemory.vidm_size,
	    AP_KRW, PT_CACHEABLE);

	/*
	 * Map entries in the page table used to map PTE's
	 * Basically every kernel page table gets mapped here
	 */
	/* The -2 is slightly bogus, it should be -log2(sizeof(pt_entry_t)) */
	l2pagetable = kernel_ptpt.pv_pa - physical_start;
	map_entry_nc(l2pagetable, (KERNEL_BASE >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_KERNEL]);
	map_entry_nc(l2pagetable, (PROCESS_PAGE_TBLS_BASE >> (PGSHIFT-2)),
	    kernel_ptpt.pv_pa);
	map_entry_nc(l2pagetable, (VMEM_VBASE >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_VMEM]);
	map_entry_nc(l2pagetable, (0x00000000 >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_SYS]);
	for (loop = 0; loop < KERNEL_PT_VMDATA_NUM; ++loop) {
		map_entry_nc(l2pagetable, ((KERNEL_VM_BASE +
		    (loop * 0x00400000)) >> (PGSHIFT-2)),
		    kernel_pt_table[KERNEL_PT_VMDATA + loop]);
	}

	/*
	 * Map the system page in the kernel page table for the bottom 1Meg
	 * of the virtual memory map.
	 */
	l2pagetable = kernel_pt_table[KERNEL_PT_SYS] - physical_start;
	map_entry(l2pagetable, 0x0000000, systempage.pv_pa);

	/* Map the VIDC20, IOMD, COMBO and podules */

	/* Map the VIDC20 */
	map_section(l1pagetable, VIDC_BASE, VIDC_HW_BASE, 0);

	/* Map the IOMD (and SLOW and MEDIUM simple podules) */
	map_section(l1pagetable, IOMD_BASE, IOMD_HW_BASE, 0);

	/* Map the COMBO (and module space) */
	map_section(l1pagetable, IO_BASE, IO_HW_BASE, 0);

	/* Bit more debugging info */
/*	printf("page tables look like this ...\n");
	printf("V0x00000000 - %08x\n", ReadWord(l1pagetable + 0x0000));
	printf("V0x03200000 - %08x\n", ReadWord(l1pagetable + 0x00c8));
	printf("V0x03500000 - %08x\n", ReadWord(l1pagetable + 0x00d4));
	printf("V0xf0000000 - %08x\n", ReadWord(l1pagetable + 0x3c00));
	printf("V0xf0100000 - %08x\n", ReadWord(l1pagetable + 0x3c04));
	printf("V0xf1000000 - %08x\n", ReadWord(l1pagetable + 0x3c40));
	printf("V0xf2000000 - %08x\n", ReadWord(l1pagetable + 0x3c80));
	printf("V0xf3000000 - %08x\n", ReadWord(l1pagetable + 0x3cc0));
	printf("V0xf3300000 - %08x\n", ReadWord(l1pagetable + 0x3ccc));
	printf("V0xf4000000 - %08x\n", ReadWord(l1pagetable + 0x3d00));
	printf("V0xf5000000 - %08x\n", ReadWord(l1pagetable + 0x3d40));
	printf("V0xf6000000 - %08x\n", ReadWord(l1pagetable + 0x3d80));
	printf("V0xf7000000 - %08x\n", ReadWord(l1pagetable + 0x3dc0));
	printf("V0xefc00000 - %08x\n", ReadWord(l1pagetable + 0x3bf8));
	printf("V0xef800000 - %08x\n", ReadWord(l1pagetable + 0x3bfc));
*/

	/*
	 * Now we have the real page tables in place so we can switch to them.
	 * Once this is done we will be running with the REAL kernel page
	 * tables.
	 */

	/*
	 * The last thing we must do is copy the kernel down to the new memory.
	 * This copies all our kernel data structures and variables as well
	 * which is why it is left to the last moment.
	 */
	if (bootconfig.vram[0].pages != 0)
		printf("mapping ... ");

	memcpy((char *)0x00000000, (char *)KERNEL_TEXT_BASE, kerneldatasize);

	/* Switch tables */
#ifdef VERBOSE_INIT_ARM
	printf("switching to new L1 page table\n");
#endif

	setttb(kernel_l1pt.pv_pa);

	/*
	 * We must now clean the cache again....
	 * Cleaning may be done by reading new data to displace any
	 * dirty data in the cache. This will have happened in setttb()
	 * but since we are boot strapping the addresses used for the read
	 * may have just been remapped and thus the cache could be out
	 * of sync. A re-clean after the switch will cure this.
	 * After booting there are no gross reloations of the kernel thus
	 * this problem will not occur after initarm().
	 */
	cpu_cache_cleanID();

	if (videodram_size != 0) {
		bootconfig.display_start = VMEM_VBASE;
		physcon_display_base(VMEM_VBASE);
		vidcconsole_reinit();

		/* Turn the VIDC DAC's on again. */
		vidcconsole_blank(vconsole_current, BLANK_NONE);
		printf("\x0cSecondary bootstrap: ");
	}

	printf("done.\n");

	/* Right set up the vectors at the bottom of page 0 */
	memcpy((char *)0x00000000, page0, page0_end - page0);

	/* We have modified a text page so sync the icache */
	cpu_cache_syncI_rng(0, page0_end - page0);

	/*
	 * Pages were allocated during the secondary bootstrap for the
	 * stacks for different CPU modes.
	 * We must now set the r13 registers in the different CPU modes to
	 * point to these stacks.
	 * Since the ARM stacks use STMFD etc. we must set r13 to the top end
	 * of the stack memory.
	 */
	printf("init subsystems: stacks ");
	console_flush();

	set_stackptr(PSR_IRQ32_MODE, irqstack.pv_va + IRQ_STACK_SIZE * NBPG);
	set_stackptr(PSR_ABT32_MODE, abtstack.pv_va + ABT_STACK_SIZE * NBPG);
	set_stackptr(PSR_UND32_MODE, undstack.pv_va + UND_STACK_SIZE * NBPG);
#ifdef PMAP_DEBUG
	if (pmap_debug_level >= 0)
		printf("kstack V%08lx P%08lx\n", kernelstack.pv_va,
		    kernelstack.pv_pa);
#endif	/* PMAP_DEBUG */

	/*
	 * Well we should set a data abort handler.
	 * Once things get going this will change as we will need a proper
	 * handler. Until then we will use a handler that just panics but
	 * tells us why.
	 * Initialisation of the vectors will just panic on a data abort.
	 * This just fills in a slighly better one.
	 */
	printf("vectors ");
	data_abort_handler_address = (u_int)data_abort_handler;
	prefetch_abort_handler_address = (u_int)prefetch_abort_handler;
	undefined_handler_address = (u_int)undefinedinstruction_bounce;
	console_flush();

#if 0
	/* Diagnostic stuff. while writing the boot code */
	for (loop = 0x0; loop < 0x1000; ++loop) {
		if (ReadWord(PAGE_DIRS_BASE + loop * 4) != 0)
			printf("Pagetable for V%08x = %08x\n", loop << 20,
			    ReadWord(0xf2000000 + loop * 4));
	}
 
	for (loop = 0x0; loop < 0x400; ++loop) {
		if (ReadWord(kernel_pt_table[KERNEL_PT_PTE] + loop * 4) != 0)
			printf("Pagetable for V%08x P%08x = %08x\n",
			    loop << 22, kernel_pt_table[KERNEL_PT_PTE]+loop*4,
			    ReadWord(kernel_pt_table[KERNEL_PT_PTE]+loop * 4));
	}
#endif

	/* At last !
	 * We now have the kernel in physical memory from the bottom upwards.
	 * Kernel page tables are physically above this.
	 * The kernel is mapped to 0xf0000000
	 * The kernel data PTs will handle the mapping of 0xf1000000-0xf1ffffff
	 * 2Meg of VRAM is mapped to 0xf4000000
	 * The page tables are mapped to 0xefc00000
	 * The IOMD is mapped to 0xf6000000
	 * The VIDC is mapped to 0xf6100000
	 */

	/* Initialise the undefined instruction handlers */
	printf("undefined ");
	undefined_init();
	console_flush();

	/* Boot strap pmap telling it where the kernel page table is */
	printf("pmap ");
	pmap_bootstrap(kernel_l1pt.pv_va, kernel_ptpt);
	console_flush();

	/* Setup the IRQ system */
	printf("irq ");
	console_flush();
	irq_init();
	printf("done.\n");

	if (cmos_read(RTC_ADDR_REBOOTCNT) > 0)
		printf("Warning: REBOOTCNT = %d\n",
		    cmos_read(RTC_ADDR_REBOOTCNT));

#ifdef CPU_SA110
	if (cputype == ID_SA110)
		rpc_sa110_cc_setup();	
#endif	/* CPU_SA110 */

#ifdef IPKDB
	/* Initialise ipkdb */
	ipkdb_init();
	if (boothowto & RB_KDB)
		ipkdb_connect(0);
#endif	/* NIPKDB */

#ifdef DDB
	printf("ddb: ");
	db_machine_init();
	{
		extern int end;
		extern int *esym;

		ddb_init(*(int *)&end, ((int *)&end) + 1, esym);
	}

	if (boothowto & RB_KDB)
		Debugger();
#endif	/* DDB */

	/* We return the new stack pointer address */
	return(kernelstack.pv_va + USPACE_SVC_STACK_TOP);
}

static void
process_kernel_args(void)
{
	char *args;

	/* Ok now we will check the arguments for interesting parameters. */
	args = (char *)bootconfig.argvirtualbase;
	boothowto = 0;

	/* Skip the first parameter (the boot loader filename) */
	while (*args != ' ' && *args != 0)
		++args;

	while (*args == ' ')
		++args;

	/* Skip the kernel image filename */
	while (*args != ' ' && *args != 0)
		++args;

	while (*args == ' ')
		++args;

	boot_args = args;
	parse_mi_bootargs(boot_args);
	parse_rpc_bootargs(boot_args);
}


void
parse_rpc_bootargs(args)
	char *args;
{
	int integer;

	if (get_bootconf_option(args, "videodram", BOOTOPT_TYPE_INT, &integer)) {
		videodram_size = integer;
		/* Round to 4K page */
		videodram_size *= 1024;
		videodram_size = round_page(videodram_size);
		if (videodram_size > 1024*1024)
			videodram_size = 1024*1024;
	}
}


/*
 * Ok these are some development functions. They map blocks of memory
 * into the video ram virtual memory.
 * The idea is to follow this with a call to the vidc device to
 * reinitialise the vidc20 for the new video ram.
 * Only meaning full if was support VRAM.
 */

/* Map DRAM into the video memory */

int
vmem_mapdram()
{
	u_int l2pagetable;
	u_int logical;

	if (videodram_start == 0 || videodram_size == 0)
		return(ENOMEM);

	/* flush existing video data */
	cpu_cache_purgeD();

	/* Get the level 2 pagetable for the video memory */
	l2pagetable = (u_int)pmap_pte(kernel_pmap,
	    (vm_offset_t)videomemory.vidm_vbase);

	/* Map a block of DRAM into the video memory area */
	for (logical = 0; logical < 0x200000; logical += NBPG) {
		map_entry(l2pagetable, logical, videodram_start
		    + logical);
		map_entry(l2pagetable, logical + 0x200000,
		    videodram_start + logical);
	}

	/* Flush the TLB so we pick up the new mappings */
	cpu_tlb_flushD();

	/* Rebuild the video memory descriptor */
	videomemory.vidm_vbase = VMEM_VBASE;
	videomemory.vidm_pbase = videodram_start;
	videomemory.vidm_type = VIDEOMEM_TYPE_DRAM;
	videomemory.vidm_size = videodram_size;

	/* Reinitialise the video system */
/*	video_reinit();*/
	return(0);
}


/* Map VRAM into the video memory */

int
vmem_mapvram()
{
	u_int l2pagetable;
	u_int logical;

	if (bootconfig.vram[0].address == 0 || bootconfig.vram[0].pages == 0)
		return(ENOMEM);

	/* flush existing video data */
	cpu_cache_purgeD();

	/* Get the level 2 pagetable for the video memory */
	l2pagetable = (u_int)pmap_pte(kernel_pmap,
	    (vm_offset_t)videomemory.vidm_vbase);

	/* Map the VRAM into the video memory area */
	for (logical = 0; logical < 0x200000; logical += NBPG) {
		map_entry(l2pagetable, logical, bootconfig.vram[0].address
		    + logical);
		map_entry(l2pagetable, logical + 0x200000,
		    bootconfig.vram[0].address + logical);
	}

	/* Flush the TLB so we pick up the new mappings */
	cpu_tlb_flushD();

	/* Rebuild the video memory descriptor */
	videomemory.vidm_vbase = VMEM_VBASE;
	videomemory.vidm_pbase = VRAM_BASE;
	videomemory.vidm_type = VIDEOMEM_TYPE_VRAM;
	videomemory.vidm_size = bootconfig.vram[0].pages * NBPG;

	/* Reinitialise the video system */
/*	video_reinit();*/
	return(0);
}


/* Set the cache behaviour for the video memory */

int
vmem_cachectl(flag)
	int flag;
{
	u_int l2pagetable;
	u_int logical;
	u_int pa;

	if (bootconfig.vram[0].address == 0 || bootconfig.vram[0].pages == 0)
		return(ENOMEM);

	/* Get the level 2 pagetable for the video memory */
	l2pagetable = (u_int)pmap_pte(kernel_pmap,
	    (vm_offset_t)videomemory.vidm_vbase);

	/* Map the VRAM into the video memory area */
	if (flag == 0) {
		printf("Disabling caching and buffering of VRAM\n");
		for (logical = 0; logical < 0x200000; logical += NBPG) {
			map_entry_nc(l2pagetable, logical,
			    bootconfig.vram[0].address + logical);
			map_entry_nc(l2pagetable, logical + 0x200000,
			    bootconfig.vram[0].address + logical);
		}
	} else if (flag == 1) {
		printf("Disabling caching of VRAM\n");
		for (logical = 0; logical < 0x200000; logical += NBPG) {
			pa = bootconfig.vram[0].address + logical;
			WriteWord(l2pagetable + ((logical >> 10) & 0x00000ffc),
			    L2_PTE_NC((pa & PG_FRAME), AP_KRW));
			WriteWord(l2pagetable + (((logical+0x200000) >> 10) & 0x00000ffc),
			    L2_PTE_NC((pa & PG_FRAME), AP_KRW));
		}
	} else if (flag == 2) {
		printf("Disabling buffering of VRAM\n");
		for (logical = 0; logical < 0x200000; logical += NBPG) {
			pa = bootconfig.vram[0].address + logical;
			WriteWord(l2pagetable + ((logical >> 10) & 0x00000ffc),
			    L2_PTE_NC_NB((pa & PG_FRAME), AP_KRW)|PT_C);
			WriteWord(l2pagetable + (((logical+0x200000) >> 10) & 0x00000ffc),
			    L2_PTE_NC_NB((pa & PG_FRAME), AP_KRW)|PT_C);
		}
	} else {
		printf("Enabling caching and buffering of VRAM\n");
		for (logical = 0; logical < 0x200000; logical += NBPG) {
			map_entry(l2pagetable, logical,
			    bootconfig.vram[0].address + logical);
			map_entry(l2pagetable, logical + 0x200000,
			    bootconfig.vram[0].address + logical);
		}
	}

	/* clean out any existing cached video data */
	cpu_cache_purgeD();

	/* Flush the TLB so we pick up the new mappings */
	cpu_tlb_flushD();

	return(0);
}

#ifdef CPU_SA110

/*
 * For optimal cache cleaning we need two 16K banks of
 * virtual address space that NOTHING else will access
 * and then we alternate the cache cleaning between the
 * two banks.
 * The cache cleaning code requires requires 2 banks aligned
 * on total size boundry so the banks can be alternated by
 * eorring the size bit (assumes the bank size is a power of 2)
 */
extern unsigned int sa110_cache_clean_addr;
extern unsigned int sa110_cache_clean_size;
void
rpc_sa110_cc_setup(void)
{
	int loop;
	paddr_t kaddr;
	pt_entry_t *pte;

	(void) pmap_extract(kernel_pmap, KERNEL_TEXT_BASE, &kaddr);
	for (loop = 0; loop < CPU_SA110_CACHE_CLEAN_SIZE; loop += NBPG) {
		pte = pmap_pte(kernel_pmap, (sa110_cc_base + loop));
		*pte = L2_PTE(kaddr, AP_KR);
	}
	sa110_cache_clean_addr = sa110_cc_base;
	sa110_cache_clean_size = CPU_SA110_CACHE_CLEAN_SIZE / 2;
}
#endif	/* CPU_SA110 */

/* End of machdep.c */
