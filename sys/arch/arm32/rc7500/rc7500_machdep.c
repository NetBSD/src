/*	$NetBSD: rc7500_machdep.c,v 1.5 1998/04/02 02:01:13 mark Exp $	*/

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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/callout.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/buf.h>
#include <sys/map.h>
#include <sys/exec.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/syscallargs.h>

#ifdef SYSVMSG
#include <sys/msg.h>
#endif
#ifdef SYSVSEM
#include <sys/sem.h>
#endif
#ifdef SYSVSHM
#include <sys/shm.h>
#endif

#include <dev/cons.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <vm/vm_kern.h>

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

#include "ipkdb.h"

#ifdef RC7500
#include <arm32/rc7500/rc7500_prom.h>
#endif

/*
 * Address to call from cpu_reset() to reset the machine.
 * This is machine architecture dependant as it varies depending
 * on where the ROM appears when you turn the MMU off.
 */

u_int cpu_reset_address = 0;

BootConfig bootconfig;		/* Boot config storage */
videomemory_t videomemory;	/* Video memory descriptor */

vm_offset_t physical_start;
vm_offset_t physical_freestart;
vm_offset_t physical_freeend;
vm_offset_t physical_end;
int physical_memoryblock;
u_int free_pages;
vm_offset_t pagetables_start;
int physmem = 0;

int debug_flags;
#ifndef PMAP_STATIC_L1S
int max_processes = 64;
#endif	/* !PMAP_STATIC_L1S */
int cpu_cache;
int cpu_ctrl;

u_int memory_disc_size;		/* Memory disc size */
u_int videodram_size;		/* Amount of DRAM to reserve for video */
vm_offset_t videodram_start;

vm_offset_t physical_pt_start;
vm_offset_t virtual_pt_end;

typedef struct {
	vm_offset_t physical;
	vm_offset_t virtual;
} pv_addr_t;

pv_addr_t systempage;
pv_addr_t irqstack;
pv_addr_t undstack;
pv_addr_t abtstack;
pv_addr_t kernelstack;
#ifdef RC7500
pv_addr_t fiqstack;
#endif

pt_entry_t kernel_pt_table[15];

char *boot_args;

vm_offset_t msgbufphys;

extern u_int data_abort_handler_address;
extern u_int prefetch_abort_handler_address;
extern u_int undefined_handler_address;

#ifdef PMAP_DEBUG
extern int pmap_debug_level;
#endif	/* PMAP_DEBUG */

#define KERNEL_PT_PAGEDIR   0	/* Page table for mapping proc0 pagetables */
#define KERNEL_PT_PDE       1	/* Page table for mapping L1 page dirs */
#define KERNEL_PT_PTE       2	/* */
#define KERNEL_PT_VMEM      3	/* Page table for mapping video memory */
#define KERNEL_PT_SYS       4	/* Page table for mapping proc0 zero page */
#define KERNEL_PT_KERNEL    5	/* Page table for mapping kernel */
#define KERNEL_PT_VMDATA0   6	/* Page tables for mapping kernel VM */
#define KERNEL_PT_VMDATA1   7
#define KERNEL_PT_VMDATA2   8
#define KERNEL_PT_VMDATA3   9
#define KERNEL_PT_VMDATA4  10
#define KERNEL_PT_VMDATA5  11
#define KERNEL_PT_VMDATA6  12
#define KERNEL_PT_VMDATA7  13
#define NUM_KERNEL_PTS	   14

struct user *proc0paddr;

extern int cold;

/* Prototypes */

void physconputchar		__P((char));
void physcon_display_base	__P((u_int addr));
extern void consinit			__P((void));

void map_section	__P((vm_offset_t pt, vm_offset_t va, vm_offset_t pa,
			     int cacheable));
void map_pagetable	__P((vm_offset_t pt, vm_offset_t va, vm_offset_t pa));
void map_entry		__P((vm_offset_t pt, vm_offset_t va, vm_offset_t pa));
void map_entry_ro	__P((vm_offset_t pt, vm_offset_t va, vm_offset_t pa));

void pmap_bootstrap		__P((vm_offset_t kernel_l1pt));
void process_kernel_args	__P((void));
caddr_t allocsys		__P((caddr_t v));
void data_abort_handler		__P((trapframe_t *frame));
void prefetch_abort_handler	__P((trapframe_t *frame));
void undefinedinstruction_bounce	__P((trapframe_t *frame));
void zero_page_readonly		__P((void));
void zero_page_readwrite	__P((void));
extern void dump_spl_masks	__P((void));
extern pt_entry_t *pmap_pte	__P((pmap_t pmap, vm_offset_t va));
extern void db_machine_init	__P((void));
extern void console_flush	__P((void));
extern void vidcconsole_reinit	__P((void));
extern int vidcconsole_blank	__P((struct vconsole *vc, int type));

extern void parse_mi_bootargs		__P((char *args));
void parse_rc7500_bootargs		__P((char *args));

extern void dumpsys	__P((void));

/*
 * void boot(int howto, char *bootstr)
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
#ifdef DIAGNOSTIC
	/* info */

	printf("boot: howto=%08x curproc=%p\n", howto, curproc);

	printf("current_mask=%08x spl_mask=%08x\n", current_mask, spl_mask);
	printf("ipl_bio=%08x ipl_net=%08x ipl_tty=%08x ipl_clock=%08x ipl_imp=%08x\n",
            irqmasks[IPL_BIO], irqmasks[IPL_NET], irqmasks[IPL_TTY],
            irqmasks[IPL_CLOCK], irqmasks[IPL_IMP]);

	dump_spl_masks();

	/* Did we encounter the ARM700 bug we discovered ? */
	if (arm700bugcount > 0)
		printf("ARM700 PREFETCH/SWI bug count = %d\n", arm700bugcount);
#endif

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

char bootstring[64];
char bootargs[32];
void setleds();

u_int
initarm(prom_id)
	struct prom_id *prom_id;
{
	int loop;
	int loop1;
	u_int logical;
	u_int physical;
	u_int kerneldatasize;
	u_int l1pagetable;
	u_int l2pagetable;
	u_int vdrambase;
	u_int reserv_mem;
	extern char page0[], page0_end[];
/*	struct exec *kernexec = (struct exec *)KERNEL_BASE;*/

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */

	set_cpufuncs();

	/*
	 * XXXX - FIX ME
	 */
	cpu_cache = 0x03;
	boothowto = 0;

#ifndef MEMORY_DISK_SIZE
#define MEMORY_DISK_SIZE	0
#endif
	memory_disc_size = MEMORY_DISK_SIZE * 1024;

#ifdef MEMORY_DISK_HOOKS
	boot_args = "root=/dev/md0a";
#else
	if (strcmp(prom_id->bootdev, "fd") == 0) {
		boot_args = "root=/dev/fd0a";
	} else {
		strcpy(bootstring, "root=/dev/");
		strcat(bootstring, prom_id->bootdev);
		if (((prom_id->bootdevnum >> B_UNITSHIFT) & B_UNITMASK) == 0)
			strcat(bootstring, "0a");
		else
			strcat(bootstring, "1a");
		boot_args = bootstring;
	}
#endif

	strcpy(bootargs, prom_id->bootargs);

	process_kernel_args();

	IRQdisable;

	/*
	 * The old version of ROM did not set kstart field which
	 * will be 0.  The ROM reserve 32K bytes of memory at
	 * low memory location.  I need to fix this!!!
	 */
	if (prom_id->kstart == 0 || !(prom_id->kstart & 0x10000000))
		reserv_mem = 0x8000;
	else
		reserv_mem = prom_id->kstart - prom_id->physmem_start;

	bootconfig.kernvirtualbase = KERNBASE;
	bootconfig.kernphysicalbase = 0x10000000 + reserv_mem;
	bootconfig.kernsize = (prom_id->ksize + NBPG - 1) & PG_FRAME;

	bootconfig.display_start = 0x10000000 + prom_id->video_start;
	bootconfig.display_size = prom_id->video_size;
	bootconfig.width = prom_id->display_width - 1;
	bootconfig.height = prom_id->display_height - 1;
	bootconfig.bitsperpixel = 3;	/* it's actually 8 */
	bootconfig.dram[0].address = prom_id->physmem_start;
	bootconfig.dram[0].pages = prom_id->ramsize / NBPG;

	bootconfig.dramblocks = 1;
	bootconfig.pagesize = 4096;
	bootconfig.drampages = prom_id->ramsize / NBPG;

	strcpy(&bootconfig.kernelname[0], prom_id->bootfile);

	bootconfig.framerate = 0;

/*
	videodram_size = 0x100000;
*/
	videomemory.vidm_pbase = prom_id->physmem_end - videodram_size;
	vdrambase = videomemory.vidm_pbase;
	bootconfig.display_start = VMEM_VBASE;

	/*
	 * Note: The video memory is not part of the managed memory.
	 * Exclude these memory off the available DRAM.
	 */
	bootconfig.dram[0].pages -= videodram_size / NBPG;
	bootconfig.drampages -= videodram_size / NBPG;

#ifdef PROM_DEBUG
	/*
	 * Initialise the prom console
	 */
	init_prom_interface();

	/* Talk to the user */

	printf("initarm...\n");

	printf("Kernel loaded from file %s\n", bootconfig.kernelname);
#endif

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

	/*
	 * Update the videomemory structure to reflect the mapping changes
	 */
 
	videomemory.vidm_vbase = VMEM_VBASE;
	videomemory.vidm_pbase = vdrambase;
	videomemory.vidm_type = VIDEOMEM_TYPE_DRAM;
	videomemory.vidm_size = videodram_size;

	kerneldatasize = bootconfig.kernsize + bootconfig.argsize;

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

#ifdef PROM_DEBUG
	printf("initarm: Secondary bootstrap ... ");
#endif

	/* Set up the variables that define the availablilty of physcial memory */

	physical_start = bootconfig.dram[0].address;
	physical_freestart = physical_start + reserv_mem;
	physical_end = bootconfig.dram[bootconfig.dramblocks - 1].address
	    + bootconfig.dram[bootconfig.dramblocks - 1].pages * NBPG;
	physical_freeend = physical_end;
	physical_memoryblock = 0;
	free_pages = bootconfig.drampages - reserv_mem / NBPG;
    
	bootconfig.dram[0].address += reserv_mem;
	bootconfig.dram[0].pages -= reserv_mem / NBPG;
	for (loop = 0; loop < bootconfig.dramblocks; ++loop)
		physmem += bootconfig.dram[loop].pages;

#ifdef PROM_DEBUG
	printf("physical_start=%x, physical_freestart=%x, physical_end=%x, physical_freeend=%x, free_pages=%x\n",
	    (u_int) physical_start, (u_int) physical_freestart,
	    (u_int) physical_end, (u_int) physical_freeend, free_pages);
#endif

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

#if 0
	/*
	 * Note:  The DRAM video memory is already excluded from
	 * the free physical memory.
	 */
	physical_freeend -= videodram_size;
	free_pages -= (videodram_size / NBPG);
	videodram_start = physical_freeend;
#endif

#ifdef PROM_DEBUG
	printf("physical_start=%x, physical_freestart=%x, physical_end=%x, physical_freeend=%x, free_pages=%x\n",
	    (u_int) physical_start, (u_int) physical_freestart,
	    (u_int) physical_end, (u_int) physical_freeend, free_pages);
#endif

	/* Right We have the bottom meg of memory mapped to 0x00000000
	 * so was can get at it. The kernel will ocupy the start of it.
	 * After the kernel/args we allocate some the the fixed page tables
	 * we need to get the system going.
	 * We allocate one page directory and 8 page tables and store the
	 * physical addresses in the kernel_pt_table array.
	 * Must remember that neither the page L1 or L2 page tables are the same
	 * size as a page !
	 *
	 * Ok the next bit of physical allocate may look complex but it is
	 * simple really. I have done it like this so that no memory gets wasted
	 * during the allocate of various pages and tables that are all different
	 * sizes.
	 * The start address will be page aligned.
	 * We allocate the kernel page directory on the first free 16KB boundry
	 * we find.
	 * We allocate the kernel page tables on the first 1KB boundry we find.
	 * We allocate 9 PT's. This means that in the process we
	 * KNOW that we will encounter at least 1 16KB boundry.
	 *
	 * Eventually if the top end of the memory gets used for process L1 page
	 * tables the kernel L1 page table may be moved up there.
	 */

	physical = physical_start + kerneldatasize + reserv_mem;

#ifdef PROM_DEBUG
	printf("physical=%08x next_phys=%08x\n", physical, pmap_next_phys_page(physical - NBPG));
#endif

	loop1 = 1;
	kernel_pt_table[0] = 0;
	for (loop = 0; loop < 15; ++loop) {
		if ((physical & (PD_SIZE-1)) == 0 && kernel_pt_table[0] == 0) {
			kernel_pt_table[KERNEL_PT_PAGEDIR] = physical;
			bzero((char *)(physical - physical_start), PD_SIZE);
			physical += PD_SIZE; 
		} else {
			kernel_pt_table[loop1] = physical;
			bzero((char *)(physical - physical_start), PT_SIZE);
			physical += PT_SIZE;
			++loop1;
		}
	}

#ifdef PROM_DEBUG
	/* A bit of debugging info */
	for (loop=0; loop < 10; ++loop)
		printf("%d - P%08x\n", loop, kernel_pt_table[loop]);
#endif

	/* This should never be able to happen but better confirm that. */

	if ((kernel_pt_table[0] & (PD_SIZE-1)) != 0)
		panic("initarm: Failed to align the kernel page directory\n");

	/* Update the address of the first free page of physical memory */

	physical_freestart = physical;

#ifdef PROM_DEBUG
	printf("physical_fs=%08x next_phys=%08x\n", (u_int)physical_freestart,
	    (u_int)pmap_next_phys_page(physical_freestart - NBPG));
#endif
	free_pages -= (physical - physical_start - reserv_mem) / NBPG;

	/* Allocate a page for the system page mapped to 0x00000000 */

	systempage.physical = physical_freestart;
	physical_freestart += NBPG;
#ifdef PROM_DEBUG
	printf("(0)physical_fs=%08x next_phys=%08x\n", (u_int)physical_freestart,
	    (u_int)pmap_next_phys_page(physical_freestart - NBPG));
#endif
	--free_pages;

	/*
	 * Allocate a page for fiq.
	 */
	fiqstack.physical = physical_freestart;
	physical_freestart += NBPG;
	fiqstack.virtual = KERNEL_BASE + fiqstack.physical-physical_start;
	free_pages--;

	/* Allocate another 3 pages for the stacks in different CPU modes. */

	irqstack.physical = physical_freestart;
	physical_freestart += NBPG;
	abtstack.physical = physical_freestart;
	physical_freestart += NBPG;
	undstack.physical = physical_freestart;
	physical_freestart += NBPG;
	bzero((char *)irqstack.physical - physical_start, 3*NBPG);
	free_pages -= 3;
	irqstack.virtual = KERNEL_BASE + irqstack.physical-physical_start;
	abtstack.virtual = KERNEL_BASE + abtstack.physical-physical_start;
	undstack.virtual = KERNEL_BASE + undstack.physical-physical_start;

#ifdef PROM_DEBUG
	printf("(1)physical_fs=%08x next_phys=%08x\n", (u_int)physical_freestart,
	    (u_int)pmap_next_phys_page(physical_freestart - NBPG));
#endif

	kernelstack.physical = physical_freestart;
	physical_freestart += UPAGES * NBPG;
	bzero((char *)kernelstack.physical - physical_start, UPAGES * NBPG);
	free_pages -= UPAGES;

#ifdef PROM_DEBUG
	printf("(2)physical_fs=%08x next_phys=%08x\n", (u_int)physical_freestart,
	    (u_int)pmap_next_phys_page(physical_freestart - NBPG));
#endif

	kernelstack.virtual = KERNEL_BASE + kernelstack.physical - physical_start;

	msgbufphys = physical_freestart;
	physical_freestart += round_page(MSGBUFSIZE);
	free_pages -= round_page(MSGBUFSIZE) / NBPG;

#ifdef PROM_DEBUG
	printf("physical_fs=%08x next_phys=%08x\n", (u_int)physical_freestart,
	    (u_int)pmap_next_phys_page(physical_freestart - NBPG));
#endif

	/*
	 * Ok we have allocated physical pages for the primary kernel page tables
	 * Now we fill in the L2 pagetable for the kernel code/data
	 */
	l2pagetable = kernel_pt_table[KERNEL_PT_KERNEL] - physical_start;

#if 0
	if (N_GETMAGIC(kernexec[0]) == ZMAGIC) {
#ifdef PROM_DEBUG
		printf("[ktext read-only] ");
		printf("[%08x %08x %08x] \n", (u_int)kerneldatasize, (u_int)kernexec->a_text,
		    (u_int)(kernexec->a_text+kernexec->a_data+kernexec->a_bss));
#if 0
		printf("physical start=%08x physical freestart=%08x\n", physical_start, physical_freestart);
#endif
#endif

		for (logical = 0; logical < 0x00/*kernexec->a_text*/;
		    logical += NBPG)
			map_entry_ro(l2pagetable, logical, physical_start + reserv_mem
			    + logical);
		for (; logical < kerneldatasize; logical += NBPG)
			map_entry(l2pagetable, logical, physical_start + reserv_mem
			    + logical);
	} else
#endif
		for (logical = 0; logical < kerneldatasize; logical += NBPG)
			map_entry(l2pagetable, logical, physical_start + reserv_mem
			    + logical);

	/* Map the stack pages */

	map_entry(l2pagetable, fiqstack.physical-physical_start,
	    fiqstack.physical);
	map_entry(l2pagetable, irqstack.physical-physical_start,
	    irqstack.physical);
	map_entry(l2pagetable, abtstack.physical-physical_start,
	    abtstack.physical); 
	map_entry(l2pagetable, undstack.physical-physical_start,
	    undstack.physical); 
	map_entry(l2pagetable, kernelstack.physical - physical_start,
	    kernelstack.physical); 
	map_entry(l2pagetable, kernelstack.physical + NBPG - physical_start,
	    kernelstack.physical + NBPG); 

	/* Now we fill in the L2 pagetable for the VRAM */

	/*
	 * Current architectures mean that the VRAM is always in 1 continuous
	 * bank.
	 * This means that we can just map the 2 meg that the VRAM would occupy.
	 * In theory we don't need a page table for VRAM, we could section map
	 * it but we would need the page tables if DRAM was in use.
	 */

	l2pagetable = kernel_pt_table[KERNEL_PT_VMEM] - physical_start;

	for (logical = 0; logical < videodram_size; logical += NBPG) {
		map_entry(l2pagetable, logical, vdrambase + logical);
		map_entry(l2pagetable, logical + videodram_size, vdrambase + logical);
	}

	/* Map entries in the page table used to map PDE's */

	l2pagetable = kernel_pt_table[KERNEL_PT_PDE] - physical_start;
	map_entry(l2pagetable, 0x0000000,
	    kernel_pt_table[KERNEL_PT_PAGEDIR]);
	map_entry(l2pagetable, 0x0001000,
	    kernel_pt_table[KERNEL_PT_PAGEDIR] + 0x1000);
	map_entry(l2pagetable, 0x0002000,
	    kernel_pt_table[KERNEL_PT_PAGEDIR] + 0x2000);
	map_entry(l2pagetable, 0x0003000,
	    kernel_pt_table[KERNEL_PT_PAGEDIR] + 0x3000);

	/*
	 * Map entries in the page table used to map PTE's
	 * Basically every kernel page table gets mapped here
	 */

	l2pagetable = kernel_pt_table[KERNEL_PT_PTE] - physical_start;
	map_entry(l2pagetable, (KERNEL_BASE >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_KERNEL]);
	map_entry(l2pagetable, (PAGE_DIRS_BASE >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_PDE]);
	map_entry(l2pagetable, (PROCESS_PAGE_TBLS_BASE >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_PTE]);
	map_entry(l2pagetable, (VMEM_VBASE >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_VMEM]);
	map_entry(l2pagetable, (0x00000000 >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_SYS]);
	map_entry(l2pagetable, ((KERNEL_VM_BASE + 0x00000000) >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_VMDATA0]);
	map_entry(l2pagetable, ((KERNEL_VM_BASE + 0x00400000) >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_VMDATA1]);
	map_entry(l2pagetable, ((KERNEL_VM_BASE + 0x00800000) >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_VMDATA2]);
	map_entry(l2pagetable, ((KERNEL_VM_BASE + 0x00c00000) >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_VMDATA3]);
	map_entry(l2pagetable, ((KERNEL_VM_BASE + 0x01000000) >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_VMDATA4]);
	map_entry(l2pagetable, ((KERNEL_VM_BASE + 0x01400000) >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_VMDATA5]);
	map_entry(l2pagetable, ((KERNEL_VM_BASE + 0x01800000) >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_VMDATA6]);
	map_entry(l2pagetable, ((KERNEL_VM_BASE + 0x01c00000) >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_VMDATA7]);

	map_entry(l2pagetable, (0xf5000000 >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_PAGEDIR] + 0x0000);
	map_entry(l2pagetable, (0xf5400000 >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_PAGEDIR] + 0x1000);
	map_entry(l2pagetable, (0xf5800000 >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_PAGEDIR] + 0x2000);
	map_entry(l2pagetable, (0xf5c00000 >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_PAGEDIR] + 0x3000);

	/*
	 * Map the system page in the kernel page table for the bottom 1Meg
	 * of the virtual memory map.
	 */
	l2pagetable = kernel_pt_table[KERNEL_PT_SYS] - physical_start;
	map_entry(l2pagetable, 0x0000000, systempage.physical);

	/* Now we construct the L1 pagetable */

	l1pagetable = kernel_pt_table[KERNEL_PT_PAGEDIR] - physical_start;

	/* Map the VIDC20, IOMD, COMBO and podules */

	/* Map the VIDC20 */

	map_section(l1pagetable, VIDC_BASE, VIDC_HW_BASE, 0);

	/* Map the IOMD (and SLOW and MEDIUM simple podules) */

	map_section(l1pagetable, IOMD_BASE, IOMD_HW_BASE, 0);

	/* Map the COMBO (and module space) */

	map_section(l1pagetable, IO_BASE, IO_HW_BASE, 0);

	/* Map the L2 pages tables in the L1 page table */

	map_pagetable(l1pagetable, 0x00000000,
	    kernel_pt_table[KERNEL_PT_SYS]);
	map_pagetable(l1pagetable, KERNEL_BASE,
	    kernel_pt_table[KERNEL_PT_KERNEL]);
	map_pagetable(l1pagetable, KERNEL_VM_BASE + 0x00000000,
	    kernel_pt_table[KERNEL_PT_VMDATA0]);
	map_pagetable(l1pagetable, KERNEL_VM_BASE + 0x00400000,
	    kernel_pt_table[KERNEL_PT_VMDATA1]);
	map_pagetable(l1pagetable, KERNEL_VM_BASE + 0x00800000,
	    kernel_pt_table[KERNEL_PT_VMDATA2]);
	map_pagetable(l1pagetable, KERNEL_VM_BASE + 0x00c00000,
	    kernel_pt_table[KERNEL_PT_VMDATA3]);
	map_pagetable(l1pagetable, KERNEL_VM_BASE + 0x01000000,
	    kernel_pt_table[KERNEL_PT_VMDATA4]);
	map_pagetable(l1pagetable, KERNEL_VM_BASE + 0x01400000,
	    kernel_pt_table[KERNEL_PT_VMDATA5]);
	map_pagetable(l1pagetable, KERNEL_VM_BASE + 0x01800000,
	    kernel_pt_table[KERNEL_PT_VMDATA6]);
	map_pagetable(l1pagetable, KERNEL_VM_BASE + 0x01c00000,
	    kernel_pt_table[KERNEL_PT_VMDATA7]);
	map_pagetable(l1pagetable, PAGE_DIRS_BASE,
	    kernel_pt_table[KERNEL_PT_PDE]);
	map_pagetable(l1pagetable, PROCESS_PAGE_TBLS_BASE,
	    kernel_pt_table[KERNEL_PT_PTE]);
	map_pagetable(l1pagetable, VMEM_VBASE,
	    kernel_pt_table[KERNEL_PT_VMEM]);

#ifdef PROM_DEBUG
	/* Bit more debugging info */
	printf("page tables look like this ...\n");
	printf("V0x00000000 - %08x\n", ReadWord(l1pagetable + 0x0000));
	printf("V0x03200000 - %08x\n", ReadWord(l1pagetable + 0x00c8));
	printf("V0x03500000 - %08x\n", ReadWord(l1pagetable + 0x00d4));
	printf("V0xf0000000 - %08x\n", ReadWord(l1pagetable + 0x3c00));
	printf("V0xf1000000 - %08x\n", ReadWord(l1pagetable + 0x3c40));
	printf("V0xf2000000 - %08x\n", ReadWord(l1pagetable + 0x3c80));
	printf("V0xf3000000 - %08x\n", ReadWord(l1pagetable + 0x3cc0));
	printf("V0xf3300000 - %08x\n", ReadWord(l1pagetable + 0x3ccc));
	printf("V0xf4000000 - %08x\n", ReadWord(l1pagetable + 0x3d00));
	printf("V0xf6000000 - %08x\n", ReadWord(l1pagetable + 0x3d80));
	printf("V0xefc00000 - %08x\n", ReadWord(l1pagetable + 0x3bf8));
	printf("V0xef800000 - %08x\n", ReadWord(l1pagetable + 0x3bfc));
promcngetc();
#endif

	/*
	 * Now we have the real page tables in place so we can switch to them.
	 * Once this is done we will be running with the REAL kernel page tables.
	 */

	/*
	 * The last thing we must do is copy the kernel down to the new memory.
	 * This copies all our kernel data structures and variables as well
	 * which is why it is left to the last moment.
	 */

#if 0
	bcopy((char *)KERNEL_BASE, (char *)0x00000000, kerneldatasize);
#endif

	cpu_domains(DOMAIN_CLIENT);

	/*
	 * When we get here, the ROM is still running, we need to
	 * turn all the interrupts off before switching TTB.
	 */
	irq_init();
	IRQdisable;

	setleds(LEDOFF);	/* turns off LEDs */

	/* Switch tables */
	setttb(kernel_pt_table[KERNEL_PT_PAGEDIR]);

	setleds(LEDALL);
	consinit();

	setleds(LEDOFF);

	/* Right set up the vectors at the bottom of page 0 */

	bcopy(page0, (char *)0x00000000, page0_end - page0);

	/*
	 * Pages were allocated during the secondary bootstrap for the
	 * stacks for different CPU modes.
	 * We must now set the r13 registers in the different CPU modes to
	 * point to these stacks.
	 * Since the ARM stacks use STMFD etc. we must set r13 to the top end
	 * of the stack memory.
	 */

#ifdef DIAGNOSTIC
	printf("FIQ stack V%08x P%08x\n", (u_int) fiqstack.virtual,
	    (u_int) fiqstack.physical);
	printf("IRQ stack V%08x P%08x\n", (u_int) irqstack.virtual,
	    (u_int) irqstack.physical);
	printf("ABT stack V%08x P%08x\n", (u_int) abtstack.virtual,
	    (u_int) abtstack.physical);
	printf("UND stack V%08x P%08x\n", (u_int) undstack.virtual,
	    (u_int) undstack.physical);
#endif

	printf("init subsystems: stacks ");

	set_stackptr(PSR_FIQ32_MODE, fiqstack.virtual + NBPG);
	set_stackptr(PSR_IRQ32_MODE, irqstack.virtual + NBPG);
	set_stackptr(PSR_ABT32_MODE, abtstack.virtual + NBPG);
	set_stackptr(PSR_UND32_MODE, undstack.virtual + NBPG);

#ifdef PMAP_DEBUG
	if (pmap_debug_level >= 0)
		printf("kstack V%08x P%08x\n", (int) kernelstack.virtual,
		    (int) kernelstack.physical);
#endif

	/*
	 * Well we should set a data abort handler.
	 * Once things get going this will change as we will need a proper handler.
	 * Until then we will use a handler that just panics but tells us
	 * why.
	 * Initialisation of the vectors will just panic on a data abort.
	 * This just fills in a slighly better one.
	 */

	printf("vectors ");
	data_abort_handler_address = (u_int)data_abort_handler;
	prefetch_abort_handler_address = (u_int)prefetch_abort_handler;
	undefined_handler_address = (u_int)undefinedinstruction_bounce;

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
	 * The kernel page directory is mapped to 0xf3000000
	 * The page tables are mapped to 0xefc00000
	 * The IOMD is mapped to 0xf6000000
	 * The VIDC is mapped to 0xf6100000
	 */

	/* Initialise the undefined instruction handlers */

	printf("undefined ");
	undefined_init();

	/* Boot strap pmap telling it where the kernel page table is */

	printf("pmap ");

	pmap_bootstrap(PAGE_DIRS_BASE);

	/* Setup the IRQ system */

	printf("irq ");
	irq_init();
	printf("done.\n");

#ifdef DDB
	printf("ddb: ");
	db_machine_init();
	ddb_init();

	if (boothowto & RB_KDB)
		Debugger();
#endif

	/* We return the new stack pointer address */
	return(kernelstack.virtual + USPACE_SVC_STACK_TOP);
}

void
process_kernel_args()
{
	parse_mi_bootargs(bootargs);
	parse_rc7500_bootargs(bootargs);
}

void
parse_rc7500_bootargs(args)
	char *args;
{
	int integer;

	videodram_size = 0x100000;
	if (get_bootconf_option(args, "m", BOOTOPT_TYPE_INT, &integer)) {
		if (integer >= 2 || integer <= 4)
			videodram_size *= integer;
	}
}

void
setleds(led)
	int led;
{
	outb(LEDPORT, ~led & 0xff);
}

/* End of rc7500_machdep.c */
