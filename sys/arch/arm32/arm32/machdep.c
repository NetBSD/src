/*	$NetBSD: machdep.c,v 1.20 1997/03/27 21:01:26 thorpej Exp $	*/

/*
 * Copyright (c) 1994-1996 Mark Brinicombe.
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
#include <sys/sysctl.h>
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
#include <machine/iomd.h>
#include <machine/io.h>
#include <machine/irqhandler.h>
#include <machine/katelib.h>
#include <machine/pte.h>
#include <machine/vidc.h>
#include <machine/vconsole.h>
#include <machine/undefined.h>
#include <machine/rtc.h>

#include "ipkdb.h"
#include "hydrabus.h"

#ifdef RC7500
#include <arm32/dev/rc7500_prom.h>
#endif

/* Describe different actions to take when boot() is called */

#define ACTION_HALT   0x01	/* Halt and boot */
#define ACTION_REBOOT 0x02	/* Halt and request RiscBSD reboot */
#define ACTION_KSHELL 0x04	/* Call kshell */
#define ACTION_DUMP   0x08	/* Dump the system to the dump dev if requested */

#define HALT_ACTION	ACTION_HALT | ACTION_KSHELL | ACTION_DUMP	/* boot(RB_HALT) */
#define REBOOT_ACTION	ACTION_REBOOT | ACTION_DUMP			/* boot(0) */
#define PANIC_ACTION	ACTION_HALT | ACTION_DUMP			/* panic() */

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
int max_processes;
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
#if NHYDRABUS > 0
pv_addr_t hydrascratch;
#endif

pt_entry_t kernel_pt_table[15];

/* the following is used externally (sysctl_hw) */
char machine[] = "arm32";	/* cpu "architecture" */

char *boot_args;

extern pt_entry_t msgbufpte;
int	msgbufmapped;
vm_offset_t msgbufphys;

extern u_int data_abort_handler_address;
extern u_int prefetch_abort_handler_address;
extern u_int undefined_handler_address;

extern int pmap_debug_level;

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

/*
 * Declare these as initialized data so we can patch them.
 */
int	nswbuf = 0;
#ifdef	NBUF
int	nbuf = NBUF;
#else
int	nbuf = 0;
#endif
#ifdef	BUFPAGES
int	bufpages = BUFPAGES;
#else
int	bufpages = 0;
#endif

int cold = 1;

/* Prototypes */

void physconputchar		__P((char));
void physcon_display_base	__P((u_int addr));
void consinit			__P((void));

void map_section	__P((vm_offset_t pt, vm_offset_t va, vm_offset_t pa));
void map_pagetable	__P((vm_offset_t pt, vm_offset_t va, vm_offset_t pa));
void map_entry		__P((vm_offset_t pt, vm_offset_t va, vm_offset_t pa));
void map_entry_ro	__P((vm_offset_t pt, vm_offset_t va, vm_offset_t pa));

void pmap_bootstrap		__P((vm_offset_t kernel_l1pt));
void process_kernel_args	__P((void));
u_long strtoul			__P((const char *s, char **ptr, int base));
caddr_t allocsys		__P((caddr_t v));
void data_abort_handler		__P((trapframe_t *frame));
void prefetch_abort_handler	__P((trapframe_t *frame));
void undefinedinstruction_bounce	__P((trapframe_t *frame));
void zero_page_readonly		__P((void));
void zero_page_readwrite	__P((void));
extern void set_boot_devs	__P((void));
extern void configure		__P((void));
extern void init_fpe_state	__P((struct proc *p));
extern int	savectx		__P((struct pcb *pcb));
extern void dump_spl_masks	__P((void));
extern pt_entry_t *pmap_pte	__P((pmap_t pmap, vm_offset_t va));
extern void db_machine_init	__P((void));
extern void console_flush	__P((void));
extern void vidcconsole_reinit	__P((void));
extern int vidcconsole_blank	__P((struct vconsole *vc, int type));

extern void pmap_debug	__P((int level));
extern void dumpsys	__P((void));
extern void hydrastop	__P((void));

/*
 * Debug function just to park the CPU
 *
 * This should be updated to power down an ARM7500
 */

void
halt()
{
	while (1);
}


/*
 * void cpu_reboot(int howto, char *bootstr)
 *
 * Reboots the system
 *
 * This gets called when a reboot is request by the user or after a panic.
 * Call boot0() will reboot the machine. For the moment we will try and be
 * clever and return to the booting environment. This may work if we
 * have be booted with the Kate boot loader as long as we have not messed
 * the system up to much. Until we have our own memory management running
 * this should work. The only use of being able to return (to RISC OS)
 * is so I don't have to wait while the machine reboots.
 */

/* NOTE: These variables will be removed, well some of them */

extern u_int spl_mask;
extern u_int current_mask;
extern u_int arm700bugcount;

extern int userret_count0;
extern int userret_count1;

void
cpu_reboot(howto, bootstr)
	int howto;
	char *bootstr;
{
	int loop;
	int action;

#ifdef DIAGNOSTIC
	/* Info */

	if (curproc == NULL)
		printf("curproc = 0 - must have been in cpu_idle()\n");
/*	if (curpcb)
		printf("curpcb=%08x pcb_sp=%08x pcb_und_sp=%08x\n", curpcb, curpcb->pcb_sp, curpcb->pcb_und_sp);*/
#endif

	printf("userret_count0=%d\n", userret_count0);
	printf("userret_count1=%d\n", userret_count1);

#if NHYDRABUS > 0
	/*
	 * If we are halting the master then we should halt the slaves :-)
	 * otherwise it can get a bit disconcerting to have 4 other
	 * processor still tearing away doing things.
	 */

	hydrastop();
#endif

#ifdef DIAGNOSTIC
	/* info */

	printf("boot: howto=%08x %08x curproc=%08x\n", howto, spl_mask, (u_int)curproc);

	printf("current_mask=%08x spl_mask=%08x\n", current_mask, spl_mask);
	printf("ipl_bio=%08x ipl_net=%08x ipl_tty=%08x ipl_clock=%08x ipl_imp=%08x\n",
            irqmasks[IPL_BIO], irqmasks[IPL_NET], irqmasks[IPL_TTY],
            irqmasks[IPL_CLOCK], irqmasks[IPL_IMP]);

	dump_spl_masks();

	/* Did we encounter the ARM700 bug we discovered ? */

	if (arm700bugcount > 0)
		printf("ARM700 PREFETCH/SWI bug count = %d\n", arm700bugcount);
#endif

	/* If we are still cold then hit the air brakes and crash to earth fast */

	if (cold) {
		doshutdownhooks();
		printf("Halted while still in the ICE age.\n");
		printf("Hit a key to reboot\n");
		cngetc();
		boot0();
	}

	/* Disable console buffering */

	cnpollc(1);

	/*
	 * Depending on how we got here and with what intructions, choose
	 * the actions to take. (See the actions defined above)
	 */
 
	if (panicstr)
		action = PANIC_ACTION;
	else if (howto & RB_HALT)
		action = HALT_ACTION;
	else
		action = REBOOT_ACTION;

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

	/* If we need to do a dump, do it */

	if ((howto & RB_DUMP) && (action & ACTION_DUMP)) {
		dumpsys();
	}
	
#ifdef KSHELL
	cold = 0;

	/* Now enter our crude debug shell if required. Soon to be replaced with DDB */

	if (action & ACTION_KSHELL)
		shell();
#else
	if (action & ACTION_KSHELL) {
		printf("Halted.\n");
		printf("Hit a key to reboot ");
		cngetc();
	}
#endif

	/* Auto reboot overload protection */

	/*
	 * This code stops the kernel entering an endless loop of reboot - panic
	 * cycles. This will only effect kernels that have been configured to
	 * reboot on a panic and will have the effect of stopping further reboots
	 * after it has rebooted 16 times after panics and clean halt or reboot
	 * will reset the counter.
	 */

	/*
	 * Have we done 16 reboots in a row ? If so halt rather than reboot
	 * since 16 panics in a row without 1 clean halt means something is
	 * seriously wrong
	 */

	if (cmos_read(RTC_ADDR_REBOOTCNT) > 16)
		action = (action & ~ACTION_REBOOT) | ACTION_HALT;

	/*
	 * If we are rebooting on a panic then up the reboot count otherwise reset
	 * This will thus be reset if the kernel changes the boot action from
	 * reboot to halt due to too any reboots.
	 */
 
	if ((action & ACTION_REBOOT) && panicstr)
		cmos_write(RTC_ADDR_REBOOTCNT,
		   cmos_read(RTC_ADDR_REBOOTCNT) + 1);
	else
		cmos_write(RTC_ADDR_REBOOTCNT, 0);

	/*
	 * If we need a RiscBSD reboot, request it but setting a bit in the CMOS RAM
	 * This can be detected by the RiscBSD boot loader during a RISC OS boot
	 * No other way to do this as RISC OS is in ROM.
	 */

	if (action & ACTION_REBOOT)
		cmos_write(RTC_ADDR_BOOTOPTS,
		    cmos_read(RTC_ADDR_BOOTOPTS) | 0x02);

	/* Run any shutdown hooks */

	printf("Running shutdown hooks ...\n");
	doshutdownhooks();

	/* Make sure IRQ's are disabled */

	IRQdisable;

	/* Tell the user we are booting */

	printf("boot...");

	/* Give the user time to read the last couple of lines of text. */

	for (loop = 5; loop > 0; --loop) {
		printf("%d..", loop);
		delay(500000);
	}

	boot0();
}


/* Sync the discs and unmount the filesystems */

void
bootsync(void)
{
	static int bootsyncdone = 0;

	if (bootsyncdone) return;

	bootsyncdone = 1;

	/* Make sure we can still manage to do things */

	if (GetCPSR() & I32_bit) {
		/*
		 * If we get then boot has been called without RB_NOSYNC and interrupts were
		 * disabled. This means the boot() call did not come from a user process e.g.
		 * shutdown, but must have come from somewhere in the kernel.
		 */

		IRQenable;
		printf("Warning IRQ's disabled during boot()\n");
	}

	/*
	 * Ok we set cold so that syslogd does not get rescheduled
	 * during the shutdown.
	 */

	cold = 1;	/* no sleeping etc. */
	vfs_shutdown();
	cold = 0;	/* sleeping etc. */
}


/*
 * Estimated loop for n microseconds
 */

/* Need to re-write this to use the timers */

/* One day soon I will actually do this */

void
delay(n)
	u_int n;
{
	u_int i;

	while (--n > 0) {
		if (cputype == ID_SA110)
			for (i = 50; --i;);
		else
			for (i = 8; --i;);
	}
}


/* A few functions that are used to help construct the page tables
 * during the bootstrap process.
 */

/*__inline*/ void
map_section(pagetable, va, pa)
	vm_offset_t pagetable;
	vm_offset_t va;
	vm_offset_t pa; 
{
	if ((va & 0xfffff) != 0)
		panic("initarm: Cannot allocate 1MB section on non 1MB boundry\n");

	((u_int *)pagetable)[(va >> 20)] = L1_SEC((pa & PD_MASK));
}


/*__inline*/ void
map_pagetable(pagetable, va, pa)
	vm_offset_t pagetable;
	vm_offset_t va;
	vm_offset_t pa;
{
	if ((pa & 0xc00) != 0)
		panic("pagetables should be group allocated on pageboundry");
	((u_int *)pagetable)[(va >> 20) + 0] = L1_PTE((pa & PG_FRAME) + 0x000);
	((u_int *)pagetable)[(va >> 20) + 1] = L1_PTE((pa & PG_FRAME) + 0x400);
	((u_int *)pagetable)[(va >> 20) + 2] = L1_PTE((pa & PG_FRAME) + 0x800);
	((u_int *)pagetable)[(va >> 20) + 3] = L1_PTE((pa & PG_FRAME) + 0xc00);
}


__inline void
map_entry(pagetable, va, pa)
	vm_offset_t pagetable;
	vm_offset_t va;
	vm_offset_t pa;
{
	WriteWord(pagetable + ((va >> 10) & 0x00000ffc),
	    L2_PTE((pa & PG_FRAME), AP_KRW));
}


__inline void
map_entry_nc(pagetable, va, pa)
	vm_offset_t pagetable;
	vm_offset_t va;
	vm_offset_t pa;
{
	WriteWord(pagetable + ((va >> 10) & 0x00000ffc),
	    L2_PTE_NC_NB((pa & PG_FRAME), AP_KRW));
}


__inline void
map_entry_ro(pagetable, va, pa)
	vm_offset_t pagetable;
	vm_offset_t va;
	vm_offset_t pa;
{
	WriteWord(pagetable + ((va >> 10) & 0x00000ffc),
	    L2_PTE((pa & PG_FRAME), AP_KR));
}


#ifndef RC7500
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
	u_int physical;
	u_int kerneldatasize;
	u_int l1pagetable;
	u_int l2pagetable;
	extern char page0[], page0_end[];
	struct exec *kernexec = (struct exec *)KERNEL_BASE;
	int id;

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
		 * Update the videomemory structure to reflect the mapping changes
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

		for (logical = 0; logical < bootconfig.display_size; logical += NBPG) {
			map_entry(l2pagetable + 0x1000, logical,
			    bootconfig.display_phys + logical);
		}

		/*
		 * Update the videomemory structure to reflect the mapping changes
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
 
	for (logical = 0; logical < 0x200000; logical += NBPG) {
		map_entry(l2pagetable + 0x2000, logical,
		    bootconfig.dram[0].address + logical + NBPG);
	}
#else
	for (logical = 0; logical < 0x200000; logical += NBPG) {
		map_entry(l2pagetable + 0x2000, logical,
		    bootconfig.dram[0].address + logical);
	}
#endif	/* NHYDRABUS */

	/*
	 * Now we construct the L1 pagetable. This only needs the minimum to
	 * keep us going until we can contruct the proper kernel L1 page table.
	 */

	map_section(l1pagetable, VIDC_BASE,  VIDC_HW_BASE);
	map_section(l1pagetable, IOMD_BASE,  IOMD_HW_BASE);

	map_pagetable(l1pagetable, 0x00000000,
	    bootconfig.scratchphysicalbase + 0x2000);
	map_pagetable(l1pagetable, KERNEL_BASE,
	    bootconfig.scratchphysicalbase + 0x3000);
	map_pagetable(l1pagetable, VMEM_VBASE,
	    bootconfig.scratchphysicalbase + 0x1000);

	/* Print some debugging info */

/*
	printf("page tables look like this ...\n");
	printf("V0x00000000 - %08x\n", ReadWord(l1pagetable + 0x0000));
	printf("V0x03500000 - %08x\n", ReadWord(l1pagetable + 0x00d4));
	printf("V0x00200000 - %08x\n", ReadWord(l1pagetable + 0x0080));
	printf("V0xf4000000 - %08x\n", ReadWord(l1pagetable + 0x3d00));
	printf("V0xf0000000 - %08x\n", ReadWord(l1pagetable + 0x3c00));
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

	/* XXX - Is this really needed ? - no as the setttb() function cleans the caches */
	cpu_cache_syncI();

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
	 * Since we have mapped the VRAM up into kernel space we must now update the
	 * the bootconfig and display structures by hand.
	 */

	if (bootconfig.vram[0].pages != 0) {
		bootconfig.display_start = VMEM_VBASE;
		physcon_display_base(VMEM_VBASE);
	}

	if (bootconfig.vram[0].pages != 0)
		printf("done.\n");

	id = ReadByte(IOMD_ID0) | (ReadByte(IOMD_ID1) << 8);
	switch (id) {
	case ARM7500_IOC_ID:
#ifndef CPU_ARM7500
		panic("Encountered ARM7500 IOMD but no ARM7500 kernel support");
#endif
		break;
	case RPC600_IOMD_ID:
#ifdef CPU_ARM7500
		panic("Encountered ARM6/7 IOMD and ARM7500 kernel support");
#endif
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

	bzero(0x00000000, 0x200000);	/* XXX */

	/* Set up the variables that define the availablilty of physcial memory */

	physical_start = bootconfig.dram[0].address;
	physical_freestart = physical_start;
	physical_end = bootconfig.dram[bootconfig.dramblocks - 1].address
	    + bootconfig.dram[bootconfig.dramblocks - 1].pages * NBPG;
	physical_freeend = physical_end;
	physical_memoryblock = 0;
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

	physical_freeend -= PD_SIZE * max_processes;
	free_pages -= 4 * max_processes;
	pagetables_start = physical_freeend;

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

/*
 * The Simtec Hydra board needs a 2MB aligned page for bootstrapping.
 * Simplest thing is to nick the bottom page of physical memory.
 */

#if NHYDRABUS > 0
	hydrascratch.physical = physical_start;
	physical_start += NBPG;
	--free_pages;
#endif

	physical = physical_start + kerneldatasize;
/*	printf("physical=%08x next_phys=%08x\n", physical, pmap_next_phys_page(physical - NBPG));*/
	loop1 = 1;
	kernel_pt_table[0] = 0;
	for (loop = 0; loop < NUM_KERNEL_PTS; ++loop) {
		if ((physical & (PD_SIZE-1)) == 0 && kernel_pt_table[0] == 0) {
			kernel_pt_table[KERNEL_PT_PAGEDIR] = physical;
			bzero((char *)physical - physical_start, PD_SIZE);
			physical += PD_SIZE; 
		} else {
			kernel_pt_table[loop1] = physical;
			bzero((char *)physical - physical_start, PT_SIZE);
			physical += PT_SIZE;
			++loop1;
		}
	}

	/* This should never be able to happen but better confirm that. */

	if ((kernel_pt_table[0] & (PD_SIZE-1)) != 0)
		panic("initarm: Failed to align the kernel page directory\n");

	/* Update the address of the first free page of physical memory */

	physical_freestart = physical;
	free_pages -= (physical - physical_start) / NBPG;

	/* Allocate a page for the system page mapped to 0x00000000 */

	systempage.physical = physical_freestart;
	physical_freestart += NBPG;
	--free_pages;
	bzero((char *)systempage.physical - physical_start, NBPG);

	/* Allocate another 3 pages for the stacks in different CPU modes. */

	irqstack.physical = physical_freestart;
	physical_freestart += NBPG;
	abtstack.physical = physical_freestart;
	physical_freestart += NBPG;
	undstack.physical = physical_freestart;
#if NIPKDB > 0
	/* Use a bigger UND32 stack when running with ipkdb */

	physical_freestart += 2*NBPG;
	bzero((char *)irqstack.physical - physical_start, 4*NBPG);
	free_pages -= 4;
#else
	physical_freestart += NBPG;
	bzero((char *)irqstack.physical - physical_start, 3*NBPG);
	free_pages -= 3;
#endif
	irqstack.virtual = KERNEL_BASE + irqstack.physical-physical_start;
	abtstack.virtual = KERNEL_BASE + abtstack.physical-physical_start;
	undstack.virtual = KERNEL_BASE + undstack.physical-physical_start;

	kernelstack.physical = physical_freestart;
	physical_freestart += UPAGES * NBPG;
	bzero((char *)kernelstack.physical - physical_start, UPAGES * NBPG);
	free_pages -= UPAGES;

	kernelstack.virtual = KERNEL_BASE + kernelstack.physical
	    - physical_start;

	msgbufphys = physical_freestart;
	physical_freestart += round_page(sizeof(struct msgbuf));
	free_pages -= round_page(sizeof(struct msgbuf)) / NBPG;

	/* Ok we have allocated physical pages for the primary kernel page tables */

	/* Now we fill in the L2 pagetable for the kernel code/data */

	l2pagetable = kernel_pt_table[KERNEL_PT_KERNEL] - physical_start;

	if (N_GETMAGIC(kernexec[0]) == ZMAGIC) {
/*		printf("[ktext read-only] ");
		printf("[%08x %08x %08x] \n", (u_int)kerneldatasize, (u_int)kernexec->a_text,
		    (u_int)(kernexec->a_text+kernexec->a_data+kernexec->a_bss));*/
		for (logical = 0; logical < 0x00/*kernexec->a_text*/;
		    logical += NBPG)
			map_entry_ro(l2pagetable, logical, physical_start
			    + logical);
		for (; logical < kerneldatasize; logical += NBPG)
			map_entry(l2pagetable, logical, physical_start
			    + logical);
	} else
		for (logical = 0; logical < kerneldatasize; logical += NBPG)
			map_entry(l2pagetable, logical, physical_start
			    + logical);

	/* Map the stack pages */

	map_entry(l2pagetable, irqstack.physical-physical_start,
	    irqstack.physical);
	map_entry(l2pagetable, abtstack.physical-physical_start,
	    abtstack.physical); 
	map_entry(l2pagetable, undstack.physical-physical_start,
	    undstack.physical); 
#if NIPKDB > 0
	/* Use a bigger UND32 stack when running with ipkdb */

	map_entry(l2pagetable, NBPG+undstack.physical-physical_start,
	    NBPG+undstack.physical); 
#endif
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

	if (videodram_size > 0) {
		for (logical = 0; logical < videomemory.vidm_size; logical += NBPG) {
			map_entry(l2pagetable, logical, videomemory.vidm_pbase
			    + logical);
			map_entry(l2pagetable, logical + videomemory.vidm_size,
			    videomemory.vidm_pbase + logical);
		}
	} else {
		for (logical = 0; logical < 0x200000; logical += NBPG) {
			map_entry(l2pagetable, logical, bootconfig.vram[0].address
			    + logical);
			map_entry(l2pagetable, logical + 0x200000,
			    bootconfig.vram[0].address + logical);
		}
	}

	/* Map entries in the page table used to map PDE's */

	l2pagetable = kernel_pt_table[KERNEL_PT_PDE] - physical_start;
	map_entry_nc(l2pagetable, 0x0000000,
	    kernel_pt_table[KERNEL_PT_PAGEDIR]);
	map_entry_nc(l2pagetable, 0x0001000,
	    kernel_pt_table[KERNEL_PT_PAGEDIR] + 0x1000);
	map_entry_nc(l2pagetable, 0x0002000,
	    kernel_pt_table[KERNEL_PT_PAGEDIR] + 0x2000);
	map_entry_nc(l2pagetable, 0x0003000,
	    kernel_pt_table[KERNEL_PT_PAGEDIR] + 0x3000);

	/*
	 * Map entries in the page table used to map PTE's
	 * Basically every kernel page table gets mapped here
	 */

	l2pagetable = kernel_pt_table[KERNEL_PT_PTE] - physical_start;
	map_entry_nc(l2pagetable, (KERNEL_BASE >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_KERNEL]);
	map_entry_nc(l2pagetable, (PAGE_DIRS_BASE >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_PDE]);
	map_entry_nc(l2pagetable, (PROCESS_PAGE_TBLS_BASE >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_PTE]);
	map_entry_nc(l2pagetable, (VMEM_VBASE >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_VMEM]);
	map_entry_nc(l2pagetable, (0x00000000 >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_SYS]);
	map_entry_nc(l2pagetable, ((KERNEL_VM_BASE + 0x00000000) >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_VMDATA0]);
	map_entry_nc(l2pagetable, ((KERNEL_VM_BASE + 0x00400000) >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_VMDATA1]);
	map_entry_nc(l2pagetable, ((KERNEL_VM_BASE + 0x00800000) >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_VMDATA2]);
	map_entry_nc(l2pagetable, ((KERNEL_VM_BASE + 0x00c00000) >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_VMDATA3]);
	map_entry_nc(l2pagetable, ((KERNEL_VM_BASE + 0x01000000) >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_VMDATA4]);
	map_entry_nc(l2pagetable, ((KERNEL_VM_BASE + 0x01400000) >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_VMDATA5]);
	map_entry_nc(l2pagetable, ((KERNEL_VM_BASE + 0x01800000) >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_VMDATA6]);
	map_entry_nc(l2pagetable, ((KERNEL_VM_BASE + 0x01c00000) >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_VMDATA7]);

	map_entry_nc(l2pagetable, (0xf5000000 >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_PAGEDIR] + 0x0000);
	map_entry_nc(l2pagetable, (0xf5400000 >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_PAGEDIR] + 0x1000);
	map_entry_nc(l2pagetable, (0xf5800000 >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_PAGEDIR] + 0x2000);
	map_entry_nc(l2pagetable, (0xf5c00000 >> (PGSHIFT-2)),
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

	map_section(l1pagetable, VIDC_BASE, VIDC_HW_BASE);

	/* Map the IOMD (and SLOW and MEDIUM simple podules) */

	map_section(l1pagetable, IOMD_BASE, IOMD_HW_BASE);

	/* Map the COMBO (and module space) */

	map_section(l1pagetable, IO_BASE, IO_HW_BASE);

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

	/* Bit more debugging info */

/*	printf("page tables look like this ...\n");
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
*/
/*	printf("V0xefc00000 - %08x\n", ReadWord(l1pagetable + 0x3bf8));
	printf("V0xef800000 - %08x\n", ReadWord(l1pagetable + 0x3bfc));*/

/*
 * Now we have the real page tables in place so we can switch to them.
 * Once this is done we will be running with the REAL kernel page tables.
 */

/*
 * The last thing we must do is copy the kernel down to the new memory.
 * This copies all our kernel data structures and variables as well
 * which is why it is left to the last moment.
 */

	if (bootconfig.vram[0].pages != 0)
		printf("mapping ... ");

	bcopy((char *)KERNEL_BASE, (char *)0x00000000, kerneldatasize);

	/* XXX - Is this really needed ? - no as the setttb() function cleans the caches */
	cpu_cache_syncI();

	/* Switch tables */

	setttb(kernel_pt_table[KERNEL_PT_PAGEDIR]);

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

	bcopy(page0, (char *)0x00000000, page0_end - page0);

	/* We have modified a text page so sync the icache */
	cpu_cache_syncI();

/*
 * Pages were allocated during the secondary bootstrap for the
 * stacks for different CPU modes.
 * We must now set the r13 registers in the different CPU modes to
 * point to these stacks.
 * Since the ARM stacks use STMFD etc. we must set r13 to the top end
 * of the stack memory.
 */

#if defined(DIAGNOSTIC) && 0
	printf("IRQ stack V%08x P%08x\n", (u_int) irqstack.virtual,
	    (u_int) irqstack.physical);
	printf("ABT stack V%08x P%08x\n", (u_int) abtstack.virtual,
	    (u_int) abtstack.physical);
	printf("UND stack V%08x P%08x\n", (u_int) undstack.virtual,
	    (u_int) undstack.physical);
#endif

	printf("init subsystems: stacks ");
	console_flush();

	set_stackptr(PSR_IRQ32_MODE, irqstack.virtual + NBPG);
	set_stackptr(PSR_ABT32_MODE, abtstack.virtual + NBPG);
#if NIPKDB > 0
	/* Use a bigger UND32 stack when running with ipkdb */
	set_stackptr(PSR_UND32_MODE, undstack.virtual + 2*NBPG);
#else
	set_stackptr(PSR_UND32_MODE, undstack.virtual + NBPG);
#endif
	if (pmap_debug_level >= 0)
		printf("kstack V%08x P%08x\n", (int) kernelstack.virtual,
		    (int) kernelstack.physical);

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
	console_flush();

	/* XXX - Is this really needed */
	cpu_cache_syncI();

/* Diagnostic stuff. while writing the boot code */

/*	for (loop = 0x0; loop < 0x1000; ++loop) {
		if (ReadWord(PAGE_DIRS_BASE + loop * 4) != 0)
			printf("Pagetable for V%08x = %08x\n", loop << 20,
			    ReadWord(0xf2000000 + loop * 4));
	}*/
 
/* Diagnostic stuff. while writing the boot code */

/*	for (loop = 0x0; loop < 0x400; ++loop) {
		if (ReadWord(kernel_pt_table[KERNEL_PT_PTE] + loop * 4) != 0)
			printf("Pagetable for V%08x P%08x = %08x\n",
			    loop << 22, kernel_pt_table[KERNEL_PT_PTE]+loop*4,
			    ReadWord(kernel_pt_table[KERNEL_PT_PTE]+loop * 4));
	}*/

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
	console_flush();

	/* XXX - Is this really needed */
	cpu_cache_syncI();

	/* Boot strap pmap telling it where the kernel page table is */

	printf("pmap ");
	pmap_bootstrap(PAGE_DIRS_BASE);
	console_flush();

	/* XXX - Is this really needed */
	cpu_cache_syncI();

	/* Setup the IRQ system */

	printf("irq ");
	console_flush();
	irq_init();
	printf("done.\n");

	/* Initialise ipkdb */

#if NIPKDB > 0
	ipkdb_init();
	if (boothowto & RB_KDB)
		ipkdb_connect(0);
#endif

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

#else	/* RC7500 */

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
	struct exec *kernexec = (struct exec *)KERNEL_BASE;

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
	boot_args = "root=/dev/md0a swapsize=0";
#else
	if (strcmp(prom_id->bootdev, "fd") == 0) {
		boot_args = "root=/dev/fd0a swapsize=0";
	} else {
		strcpy(bootstring, "root=/dev/");
		strcat(bootstring, prom_id->bootdev);
		if (((prom_id->bootdevnum >> B_UNITSHIFT) & B_UNITMASK) == 0)
			strcat(bootstring, "0a swap=/dev/wd0b");
		else
			strcat(bootstring, "1a swap=/dev/wd1b");
		boot_args = bootstring;
	}
#endif

	strcpy(bootargs, prom_id->bootargs);

	/*
	 * XXX FIX ME.
	 * Each level 1 page table would take up 16KB.
	 * So, we only allow 64 level 1 page tables for now.
	 */
	max_processes = 64;

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

	physical_freeend -= PD_SIZE * max_processes;
	free_pages -= 4 * max_processes;
	pagetables_start = physical_freeend;

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
	physical_freestart += round_page(sizeof(struct msgbuf));
	free_pages -= round_page(sizeof(struct msgbuf)) / NBPG;

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
		printf("physical start=%08x physical freestart=%08x hydra phys=%08x\n", physical_start, physical_freestart, hydrascratch.physical);
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

	map_section(l1pagetable, VIDC_BASE, VIDC_HW_BASE);

	/* Map the IOMD (and SLOW and MEDIUM simple podules) */

	map_section(l1pagetable, IOMD_BASE, IOMD_HW_BASE);

	/* Map the COMBO (and module space) */

	map_section(l1pagetable, IO_BASE, IO_HW_BASE);

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

	if (pmap_debug_level >= 0)
		printf("kstack V%08x P%08x\n", (int) kernelstack.virtual,
		    (int) kernelstack.physical);

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
#endif	/* !RC7500 */


/*
 * void cpu_startup(void)
 *
 * Machine dependant startup code. 
 *
 */

void
cpu_startup()
{
	int loop;
	vm_offset_t minaddr;
	vm_offset_t maxaddr;
	caddr_t sysbase;
	caddr_t size;
	vm_size_t bufsize;
	int base, residual;

	/* Set the cpu control register */

	cpu_ctrl = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_32BP_ENABLE
		   | CPU_CONTROL_32BD_ENABLE | CPU_CONTROL_SYST_ENABLE;
                                        
	if (cpu_cache & 1)
		cpu_ctrl |= CPU_CONTROL_IDC_ENABLE;
	if (cpu_cache & 2)
		cpu_ctrl |= CPU_CONTROL_WBUF_ENABLE;

	if (!(cpu_cache & 4))
		cpu_ctrl |= CPU_CONTROL_CPCLK;

#ifdef CPU_LATE_ABORT
	cpu_ctrl |= CPU_CONTROL_LABT_ENABLE;
#endif

	if (cputype == ID_SA110) {
		cpu_ctrl = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_32BP_ENABLE
			   | CPU_CONTROL_32BD_ENABLE | CPU_CONTROL_SYST_ENABLE;
		if (cpu_cache & 1)
			cpu_ctrl |= (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE);
		if (cpu_cache & 2)
			cpu_ctrl |= CPU_CONTROL_WBUF_ENABLE;
		if (cpu_cache & 8)
			cpu_ctrl |= CPU_CONTROL_IC_ENABLE;
		if (cpu_cache & 16)
			cpu_ctrl |= CPU_CONTROL_DC_ENABLE;
	}

	/* Clear out the cache */

	cpu_cache_purgeID();
    
	cpu_control(cpu_ctrl);

	/* All domains MUST be clients, permissions are VERY important */

	cpu_domains(DOMAIN_CLIENT);

	/* Lock down zero page */

	zero_page_readonly();

	/*
	 * Initialize error message buffer (at end of core).
	 */

	/* msgbufphys was setup during the secondary boot strap */

	for (loop = 0; loop < btoc(sizeof(struct msgbuf)); ++loop)
		pmap_enter(pmap_kernel(),
		    (vm_offset_t)((caddr_t)msgbufp + loop * NBPG),
		    msgbufphys + loop * NBPG, VM_PROT_ALL, TRUE);

	msgbufmapped = 1;

	/*
	 * Identify ourselves for the msgbuf (everything printed earlier will
	 * not be buffered).
	 */
 
	printf(version);

	printf("screen: %d x %d x %d\n", bootconfig.width + 1, bootconfig.height + 1, bootconfig.framerate);

	if (cmos_read(RTC_ADDR_REBOOTCNT) > 0)
		printf("Warning: REBOOTCNT = %d\n", cmos_read(RTC_ADDR_REBOOTCNT));

	printf("real mem = %d (%d pages)\n", arm_page_to_byte(physmem), physmem);

	/*
	 * Find out how much space we need, allocate it,
	 * and then give everything true virtual addresses.
	 */
                                    
	size = allocsys((caddr_t)0);
	sysbase = (caddr_t)kmem_alloc(kernel_map, round_page(size));
	if (sysbase == 0)
		panic("cpu_startup: no room for system tables %d bytes required", (u_int)size);
	if ((caddr_t)((allocsys(sysbase) - sysbase)) != size)
		panic("cpu_startup: system table size inconsistency");

   	/*
	 * Now allocate buffers proper.  They are different than the above
	 * in that they usually occupy more virtual memory than physical.
	 */


	bufsize = MAXBSIZE * nbuf;
	printf("cpu_startup: buffer VM size = %d\n", bufsize);
	buffer_map = kmem_suballoc(kernel_map, (vm_offset_t *)&buffers,
				   &maxaddr, bufsize, TRUE);
	minaddr = (vm_offset_t)buffers;
	if (vm_map_find(buffer_map, vm_object_allocate(bufsize),
	    (vm_offset_t)0, &minaddr, bufsize, FALSE) != KERN_SUCCESS)
		panic("startup: cannot allocate buffers");

	if ((bufpages / nbuf) >= btoc(MAXBSIZE)) {
		/* don't want to alloc more physical mem than needed */
		bufpages = btoc(MAXBSIZE) * nbuf;
	}

	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (loop = 0; loop < nbuf; ++loop) {
		vm_size_t curbufsize;
		vm_offset_t curbuf;

	/*
	 * First <residual> buffers get (base+1) physical pages
	 * allocated for them.  The rest get (base) physical pages.
	 *
	 * The rest of each buffer occupies virtual space,
	 * but has no physical memory allocated for it.
	 */

		curbuf = (vm_offset_t)buffers + loop * MAXBSIZE;
		curbufsize = CLBYTES * (loop < residual ? base+1 : base);
		vm_map_pageable(buffer_map, curbuf, curbuf+curbufsize, FALSE);
		vm_map_simplify(buffer_map, curbuf);
	}

	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */

	exec_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
				 16*NCARGS, TRUE);

	/*
	 * Allocate a submap for physio
	 */

	phys_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
				 VM_PHYS_SIZE, TRUE);

	/*
	 * Finally, allocate mbuf cluster submap.
	 */

	mb_map = kmem_suballoc(kernel_map, (vm_offset_t *)&mbutl, &maxaddr,
			       VM_MBUF_SIZE, FALSE);

	/*
	 * Initialise callouts
	 */

	callfree = callout;

	for (loop = 1; loop < ncallout; ++loop)
		callout[loop - 1].c_next = &callout[loop];

	printf("avail mem = %d (%d pages)\n", (int)ptoa(cnt.v_free_count),
	    (int)ptoa(cnt.v_free_count) / NBPG);
	printf("using %d buffers containing %d bytes of memory\n",
	    nbuf, bufpages * CLBYTES);

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */

	bufinit();

	proc0paddr = (struct user *)kernelstack.virtual;
	proc0.p_addr = proc0paddr;

	curpcb = &proc0.p_addr->u_pcb;
	curpcb->pcb_flags = 0;
	curpcb->pcb_und_sp = (u_int)proc0.p_addr + USPACE_UNDEF_STACK_TOP;
	curpcb->pcb_sp = (u_int)proc0.p_addr + USPACE_SVC_STACK_TOP;
	curpcb->pcb_pagedir = (pd_entry_t *)pmap_extract(kernel_pmap,
	    (vm_offset_t)(kernel_pmap)->pm_pdir);
	    
	proc0.p_md.md_regs = (struct trapframe *)curpcb->pcb_sp - 1;

	/*
	 * Configure the hardware
	 */
 
	configure();

	dump_spl_masks();

	cold = 0;	/* We are warm now ... */
}


/*
 * Allocate space for system data structures.  We are given
 * a starting virtual address and we return a final virtual
 * address; along the way we set each data structure pointer.
 *
 * We call allocsys() with 0 to find out how much space we want,
 * allocate that much and fill it with zeroes, and then call
 * allocsys() again with the correct base virtual address.
 */

caddr_t
allocsys(v)
	register caddr_t v;
{

#define valloc(name, type, num) \
	(caddr_t)(name) = (type *)v; \
	v = (caddr_t)((name) + (num));

	valloc(callout, struct callout, ncallout);
	valloc(swapmap, struct map, nswapmap = maxproc * 2);

#ifdef SYSVSHM
	valloc(shmsegs, struct shmid_ds, shminfo.shmmni);
#endif
#ifdef SYSVSEM
	valloc(sema, struct semid_ds, seminfo.semmni);
	valloc(sem, struct sem, seminfo.semmns);
	/* This is pretty disgusting! */
	valloc(semu, int, (seminfo.semmnu * seminfo.semusz) / sizeof(int));
#endif
#ifdef SYSVMSG
	valloc(msgpool, char, msginfo.msgmax);
	valloc(msgmaps, struct msgmap, msginfo.msgseg);
	valloc(msghdrs, struct msg, msginfo.msgtql);
	valloc(msqids, struct msqid_ds, msginfo.msgmni);
#endif
                                                                         
	/*
	 * Determine how many buffers to allocate.  We use 10% of the
	 * first 2MB of memory, and 5% of the rest, with a minimum of 16
	 * buffers.  We allocate 1/2 as many swap buffer headers as file
	 * i/o buffers.
	 */

	if (bufpages == 0)
		if (physmem < arm_byte_to_page(2 * 1024 * 1024))
			bufpages = physmem / (10 * CLSIZE);
		else
			bufpages = (arm_byte_to_page(2 * 1024 * 1024)
			         + physmem) / (20 * CLSIZE);

	/*
	 * XXX stopgap measure to prevent wasting too much KVM on
	 * the sparsely filled buffer cache.
	 */
	if (bufpages * MAXBSIZE > VM_MAX_KERNEL_BUF)
		bufpages = VM_MAX_KERNEL_BUF / MAXBSIZE;

#ifdef DIAGNOSTIC
	if (bufpages == 0)
		panic("bufpages = 0\n");
#endif

	if (nbuf == 0) {
		nbuf = bufpages;
		if (nbuf < 16)
			nbuf = 16;
	}

	/*
	 * XXX stopgap measure to prevent wasting too much KVM on
	 * the sparsely filled buffer cache.
	 */
	if (nbuf * MAXBSIZE > VM_MAX_KERNEL_BUF)
		nbuf = VM_MAX_KERNEL_BUF / MAXBSIZE;

	if (nswbuf == 0) {
		nswbuf = (nbuf / 2) & ~1;       /* force even */
		if (nswbuf > 256)
			nswbuf = 256;           /* sanity */
	}

	valloc(swbuf, struct buf, nswbuf);
	valloc(buf, struct buf, nbuf);

	return(v);
}


#ifndef RC7500
int wdresethack = 1;

void
process_kernel_args()
{
	char *ptr;
	char *args;

	/* Ok now we will check the arguments for interesting parameters. */

	args = (char *)bootconfig.argvirtualbase;
	max_processes = 64;
	boothowto = 0;
	cpu_cache = 0x03;
	debug_flags = 0;
	videodram_size = 0;
    
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

	boot_args = NULL;

	if (*args != 0) {
		boot_args = args;

		if (strstr(args, "nocache"))
			cpu_cache &= ~1;

		if (strstr(args, "nowritebuf"))
			cpu_cache &= ~2;

		if (strstr(args, "fpaclk2"))
			cpu_cache |= 4;

		if (strstr(args, "icache"))
			cpu_cache |= 8;

		if (strstr(args, "dcache"))
			cpu_cache |= 16;

		ptr = strstr(args, "maxproc=");
		if (ptr) {
			max_processes = (int)strtoul(ptr + 8, NULL, 10);
			if (max_processes < 16)
				max_processes = 16;
			/* Limit is PDSIZE * (maxprocess+1) <= 4MB */
			if (max_processes > 255)
				max_processes = 255;
			if (bootconfig.vram[0].pages != 0)
				printf("Maximum \"in memory\" processes = %d\n",
				    max_processes);
		}
		ptr = strstr(args, "memorydisc=");
		if (!ptr)
			ptr = strstr(args, "memorydisk=");
		if (ptr) {
			memory_disc_size = (u_int)strtoul(ptr + 11, NULL, 10);
			memory_disc_size *= 1024;
			if (memory_disc_size < 32*1024)
				memory_disc_size = 32*1024;
			if (memory_disc_size > 2048*1024)
				memory_disc_size = 2048*1024;
		}
		ptr = strstr(args, "videodram=");
		if (ptr) {
			videodram_size = (u_int)strtoul(ptr + 10, NULL, 10);
			/* Round to 4K page */
			videodram_size *= 1024;
			videodram_size = round_page(videodram_size);
			if (videodram_size > 1024*1024)
				videodram_size = 1024*1024;
			if (bootconfig.vram[0].pages != 0)
				printf("Video DRAM = %d\n", videodram_size);
		}
		if (strstr(args, "single"))
			boothowto |= RB_SINGLE;
		if (strstr(args, "kdb"))
			boothowto |= RB_KDB;
		ptr = strstr(args, "pmapdebug=");
		if (ptr) {
			pmap_debug_level = (int)strtoul(ptr + 10, NULL, 10);
			pmap_debug(pmap_debug_level);
			debug_flags |= 0x01;
		}
		ptr = strstr(args, "nbuf=");
		if (ptr)
			bufpages = (int)strtoul(ptr + 5, NULL, 10);
		if (strstr(args, "termdebug"))
			debug_flags |= 0x02;
		if (strstr(args, "nowdreset"))
			wdresethack = 0;
		if (strstr(args, "notermcls"))
			debug_flags |= 0x04;
	}
}

#else

int wdresethack = 0;

void
process_kernel_args()
{
	register char *p, *q;
	int i;
	q = bootargs;
	videodram_size = 0x100000;
	while ((p = strchr(q, '-')) != NULL) {
		p++;
		switch (*p) {
			case 's':
				boothowto |= RB_SINGLE;
				break;
			case 'a':
				boothowto |= RB_ASKNAME;
				break;
			case 'k':
				boothowto |= RB_KDB;
				break;
			case 'm':
				p++;
				i = *p - '0';
				if (i >= 2 && i <= 4) {
					videodram_size *= i;
				}
				break;
			default:
				break;
		}
		q = p;
	}
}

void
setleds(led)
	int led;
{
	outb(LEDPORT, ~led & 0xff);
}
#endif	/* !RC7500 */

/*
 * Clear registers on exec
 */

void
setregs(p, pack, stack, retval)
	struct proc *p;
	struct exec_package *pack;
	u_long stack;
	register_t *retval;
{
	register struct trapframe *tf;

	if (pmap_debug_level >= -1)
		printf("setregs: ip=%08x sp=%08x proc=%08x\n",
		    (u_int) pack->ep_entry, (u_int) stack, (u_int) p);

	tf = p->p_md.md_regs;

	if (pmap_debug_level >= -1)
		printf("mdregs=%08x pc=%08x lr=%08x sp=%08x\n",
		    (u_int) tf, tf->tf_pc, tf->tf_usr_lr, tf->tf_usr_sp);

	tf->tf_r11 = 0;				/* bottom of the fp chain */
	tf->tf_r12 = stack;			/* needed by crt0.c */
	tf->tf_pc = pack->ep_entry;
	tf->tf_usr_lr = pack->ep_entry;
	tf->tf_svc_lr = 0x77777777;		/* Something we can see */
	tf->tf_usr_sp = stack;
	tf->tf_r10 = 0xaa55aa55;		/* Something we can see */
	tf->tf_spsr = PSR_USR32_MODE;

	p->p_addr->u_pcb.pcb_flags = 0;

	retval[1] = 0;
}


/*
 * Modify the current mapping for zero page to make it read only
 *
 * This routine is only used until things start forking. Then new
 * system pages are mapped read only in pmap_enter().
 */

void
zero_page_readonly()
{
	WriteWord(0xefc00000, L2_PTE((systempage.physical & PG_FRAME), AP_KR));
	tlb_flush();
}


/*
 * Modify the current mapping for zero page to make it read/write
 *
 * This routine is only used until things start forking. Then system
 * pages belonging to user processes are never made writable.
 */

void
zero_page_readwrite()
{
	WriteWord(0xefc00000, L2_PTE((systempage.physical & PG_FRAME), AP_KRW));
	tlb_flush();
}


/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow sigcode stored in u. to call routine, followed by kcall
 * to sigreturn routine below.  After sigreturn resets the signal mask, the stack, and the
 * frame pointer, it returns to the user specified pc.
 */

void
sendsig(catcher, sig, mask, code)
	sig_t catcher;
	int sig;
	int mask;
	u_long code;
{
	struct proc *p = curproc;
	struct trapframe *tf;
	struct sigframe *fp, frame;
	struct sigacts *psp = p->p_sigacts;
	int oonstack;
	extern char sigcode[], esigcode[];

	if (pmap_debug_level >= 0)
		printf("Sendsig: sig=%d mask=%08x catcher=%08x code=%08x\n",
		    sig, mask, (u_int)catcher, (u_int)code);

	tf = p->p_md.md_regs;
	oonstack = psp->ps_sigstk.ss_flags & SA_ONSTACK;

	/*
	 * Allocate space for the signal handler context.
	 */

	if ((psp->ps_flags & SAS_ALTSTACK) && !oonstack &&
	    (psp->ps_sigonstack & sigmask(sig))) {
		fp = (struct sigframe *)(psp->ps_sigstk.ss_sp +
		    psp->ps_sigstk.ss_size - sizeof(struct sigframe));
		psp->ps_sigstk.ss_flags |= SA_ONSTACK;
	} else {
		fp = (struct sigframe *)tf->tf_usr_sp - 1;
	}

	/* 
	 * Build the argument list for the signal handler.
	 */

	frame.sf_signum = sig;
	frame.sf_code = code;
	frame.sf_scp = &fp->sf_sc;
	frame.sf_handler = catcher;

	/*
	 * Build the signal context to be used by sigreturn.
	 */

	frame.sf_sc.sc_onstack = oonstack;
	frame.sf_sc.sc_mask   = mask;
	frame.sf_sc.sc_r0     = tf->tf_r0;
	frame.sf_sc.sc_r1     = tf->tf_r1;
	frame.sf_sc.sc_r2     = tf->tf_r2;
	frame.sf_sc.sc_r3     = tf->tf_r3;
	frame.sf_sc.sc_r4     = tf->tf_r4;
	frame.sf_sc.sc_r5     = tf->tf_r5;
	frame.sf_sc.sc_r6     = tf->tf_r6;
	frame.sf_sc.sc_r7     = tf->tf_r7;
	frame.sf_sc.sc_r8     = tf->tf_r8;
	frame.sf_sc.sc_r9     = tf->tf_r9;
	frame.sf_sc.sc_r10    = tf->tf_r10;
	frame.sf_sc.sc_r11    = tf->tf_r11;
	frame.sf_sc.sc_r12    = tf->tf_r12;
	frame.sf_sc.sc_usr_sp = tf->tf_usr_sp;
	frame.sf_sc.sc_usr_lr = tf->tf_usr_lr;
	frame.sf_sc.sc_svc_lr = tf->tf_svc_lr;
	frame.sf_sc.sc_pc     = tf->tf_pc;
	frame.sf_sc.sc_spsr   = tf->tf_spsr;

	if (copyout(&frame, fp, sizeof(frame)) != 0) {
	/*
	 * Process has trashed its stack; give it an illegal
	 * instruction to halt it in its tracks.
	 */

		sigexit(p, SIGILL);
		/* NOTREACHED */
	}

	/*
	 * Build context to run handler in.
	 */

	tf->tf_r0 = frame.sf_signum;
	tf->tf_r1 = frame.sf_code;
	tf->tf_r2 = (u_int)frame.sf_scp;
	tf->tf_r3 = (u_int)frame.sf_handler;
	tf->tf_usr_sp = (int)fp;
	tf->tf_pc = (int)(((char *)PS_STRINGS) - (esigcode - sigcode));

	/* XXX - should just be a data purge and icache flush */
	cpu_cache_syncI();

	if (pmap_debug_level >= 0)
		printf("Sendsig: sig=%d pc=%08x\n", sig, tf->tf_pc);
}


/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to previous pc and psl as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * psr to gain improper privileges or to cause
 * a machine fault.
 */

int
sys_sigreturn(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_sigreturn_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap = v;
	struct sigcontext *scp, context;
/*	register struct sigframe *fp;*/
	register struct trapframe *tf;

	if (pmap_debug_level >= 0)
		printf("sigreturn: context=%08x\n", (int)SCARG(uap, sigcntxp));

	tf = p->p_md.md_regs;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */

	scp = SCARG(uap, sigcntxp);

	if (copyin((caddr_t)scp, &context, sizeof(*scp)) != 0)
		return (EFAULT);

	/*
	 * Check for security violations.
	 */

	/* Make sure the processor mode has not been tampered with */

	if ((context.sc_spsr & PSR_MODE) != PSR_USR32_MODE)
		return(EINVAL);

	if (context.sc_onstack & 01)
		p->p_sigacts->ps_sigstk.ss_flags |= SA_ONSTACK;
	else
		p->p_sigacts->ps_sigstk.ss_flags &= ~SA_ONSTACK;
	p->p_sigmask = context.sc_mask & ~sigcantmask;

	/*
	 * Restore signal context.
	 */

	tf->tf_r0    = context.sc_r0;
	tf->tf_r1    = context.sc_r1;
	tf->tf_r2    = context.sc_r2;
	tf->tf_r3    = context.sc_r3;
	tf->tf_r4    = context.sc_r4;
	tf->tf_r5    = context.sc_r5;
	tf->tf_r6    = context.sc_r6;
	tf->tf_r7    = context.sc_r7;
	tf->tf_r8    = context.sc_r8;
	tf->tf_r9    = context.sc_r9;
	tf->tf_r10   = context.sc_r10;
	tf->tf_r11   = context.sc_r11;
	tf->tf_r12   = context.sc_r12;
	tf->tf_usr_sp = context.sc_usr_sp;
	tf->tf_usr_lr = context.sc_usr_lr;
	tf->tf_svc_lr = context.sc_svc_lr;
	tf->tf_pc    = context.sc_pc;
	tf->tf_spsr  = context.sc_spsr;

#ifdef VALIDATE_TRAPFRAME
	validate_trapframe(tf, 6);
#endif
	
	return (EJUSTRETURN);
}

/*
 * machine dependent system variables.
 */

int
cpu_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}


#ifndef CPU_ARM7500
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
			map_entry_nc(l2pagetable, logical, bootconfig.vram[0].address
			    + logical);
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
	}
	else {
		printf("Enabling caching and buffering of VRAM\n");
		for (logical = 0; logical < 0x200000; logical += NBPG) {
			map_entry(l2pagetable, logical, bootconfig.vram[0].address
			    + logical);
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

#endif	/* !ARM7500 */

/* End of machdep.c */
