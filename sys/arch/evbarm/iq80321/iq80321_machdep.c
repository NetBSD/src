/*	$NetBSD: iq80321_machdep.c,v 1.14 2003/04/18 12:01:32 scw Exp $	*/

/*
 * Copyright (c) 2001, 2002, 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe and Steve C. Woodford for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1997,1998 Mark Brinicombe.
 * Copyright (c) 1997,1998 Causality Limited.
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Machine dependant functions for kernel setup for Intel IQ80321 evaluation
 * boards using RedBoot firmware.
 */

#include "opt_ddb.h"
#include "opt_pmap_debug.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/exec.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/termios.h>

#include <dev/cons.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <machine/bootconfig.h>
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/frame.h>

#include <arm/arm32/machdep.h>

#include <arm/xscale/i80321reg.h>
#include <arm/xscale/i80321var.h>

#include <evbarm/iq80321/iq80321reg.h>
#include <evbarm/iq80321/iq80321var.h>
#include <evbarm/iq80321/obiovar.h>

#include <evbarm/evbarm/initarmvar.h>

#include "opt_ipkdb.h"

/*
 * Address to call from cpu_reset() to reset the machine.
 * This is machine architecture dependant as it varies depending
 * on where the ROM appears when you turn the MMU off.
 *
 * XXX Not actally used on IQ80321 -- clean up the generic
 * ARM code.
 */

u_int cpu_reset_address = 0x00000000;

BootConfig bootconfig;		/* Boot config storage */
char *boot_args = NULL;
char *boot_file = NULL;

/* Prototypes */

void	consinit(void);

#include "com.h"
#if NCOM > 0
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

/*
 * Define the default console speed for the board.  This is generally
 * what the firmware provided with the board defaults to.
 */
#ifndef CONSPEED
#define CONSPEED B115200
#endif /* ! CONSPEED */

#ifndef CONUNIT
#define	CONUNIT	0
#endif

#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

int comcnspeed = CONSPEED;
int comcnmode = CONMODE;
int comcnunit = CONUNIT;

/*
 * void cpu_reboot(int howto, char *bootstr)
 *
 * Reboots the system
 *
 * Deal with any syncing, unmounting, dumping and shutdown hooks,
 * then reset the CPU.
 */
void
cpu_reboot(int howto, char *bootstr)
{
#ifdef DIAGNOSTIC
	/* info */
	printf("boot: howto=%08x curproc=%p\n", howto, curproc);
#endif

	/*
	 * If we are still cold then hit the air brakes
	 * and crash to earth fast
	 */
	if (cold) {
		doshutdownhooks();
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
		printf("rebooting...\n");
		goto reset;
	}

	/* Disable console buffering */

	/*
	 * If RB_NOSYNC was not specified sync the discs.
	 * Note: Unless cold is set to 1 here, syslogd will die during the
	 * unmount.  It looks like syslogd is getting woken up only to find
	 * that it cannot page part of the binary in as the filesystem has
	 * been unmounted.
	 */
	if (!(howto & RB_NOSYNC))
		bootsync();

	/* Say NO to interrupts */
	splhigh();

	/* Do a dump if requested. */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		dumpsys();
	
	/* Run any shutdown hooks */
	doshutdownhooks();

	/* Make sure IRQ's are disabled */
	IRQdisable;

	if (howto & RB_HALT) {
		iq80321_7seg('.', '.');
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
	}

	printf("rebooting...\n\r");
 reset:
	/*
	 * Make really really sure that all interrupts are disabled,
	 * and poke the Internal Bus and Peripheral Bus reset lines.
	 */
	(void) disable_interrupts(I32_bit|F32_bit);
	*(__volatile uint32_t *)(IQ80321_80321_VBASE + VERDE_ATU_BASE +
	    ATU_PCSR) = PCSR_RIB | PCSR_RPB;

	/* ...and if that didn't work, just croak. */
	printf("RESET FAILED!\n");
	for (;;);
}

/*
 * Mapping table for core on-board devices.
 */
static const struct initarm_iospace iq80321_ioconf[] = {
	{
		IQ80321_OBIO_BASE,
		IQ80321_OBIO_BASE,
		IQ80321_OBIO_SIZE,
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE
	},
	{
		IQ80321_IOW_VBASE,
		VERDE_OUT_XLATE_IO_WIN0_BASE,
		VERDE_OUT_XLATE_IO_WIN_SIZE,
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE
	},
	{
		IQ80321_80321_VBASE,
		VERDE_PMMR_BASE,
		VERDE_PMMR_SIZE,
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE
	}
};
#define	IQ80321_NIO (sizeof(iq80321_ioconf)/sizeof(struct initarm_iospace))

static struct initarm_config iq80321_bootconf = {
	&bootconfig,
	0,		      /* PA which maps to KERNEL_BASE (see initarm()) */
	ARM_VECTORS_HIGH,	/* Where we'd like the vector page */
	IQ80321_IOPXS_VBASE,	/* KVA of fixed onboard devices */
	IQ80321_IOPXS_VSIZE,	/* Size of the above mapping (5MB) */
	IQ80321_NIO,		/* # of fixed I/O mappings */
	iq80321_ioconf		/* List of fixed I/O mappings */
};


static void
iq80321_hardclock_hook(void)
{
	static int snakefreq;

	if ((snakefreq++ & 15) == 0)
		iq80321_7seg_snake();
}

/*
 * u_int initarm(...)
 *
 * Initial entry point on startup. This gets called before main() is
 * entered.
 * It should be responsible for setting up everything that must be
 * in place when main is called.
 * This includes
 *   Taking a copy of the boot configuration structure.
 *   Initialising the physical console so characters can be printed.
 *   Setting up page tables for the kernel
 *   Relocating the kernel to the bottom of physical memory
 */
u_int
initarm(void *arg)
{
	extern vaddr_t xscale_cache_clean_addr;
	paddr_t memstart;
	psize_t memsize;
	vaddr_t kstack;

	/*
	 * Clear out the 7-segment display.  Whee, the first visual
	 * indication that we're running kernel code.
	 */
	iq80321_7seg(' ', ' ');

	/* Calibrate the delay loop. */
	i80321_calibrate_delay();
	i80321_hardclock_hook = iq80321_hardclock_hook;

	/*
	 * Since we map the on-board devices VA==PA, and the kernel
	 * is running VA==PA, it's possible for us to initialize
	 * the console now.
	 */
	consinit();

	/* Talk to the user */
	printf("\nNetBSD/evbarm (IQ80321) booting ...\n");

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())
		panic("cpu not recognized!");

	/*
	 * We are currently running with the MMU enabled and the
	 * entire address space mapped VA==PA, except for the
	 * first 64M of RAM is also double-mapped at 0xc0000000.
	 * There is an L1 page table at 0xa0004000.
	 */

	/*
	 * Fetch the SDRAM start/size from the i80321 SDRAM configration
	 * registers.
	 */
	i80321_sdram_bounds(&obio_bs_tag, VERDE_PMMR_BASE + VERDE_MCU_BASE,
	    &memstart, &memsize);

	printf("initarm: Configuring system ...\n");

	/* Fake bootconfig structure for the benefit of pmap.c */
	/* XXX must make the memory description h/w independant */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = memstart;
	bootconfig.dram[0].pages = memsize / PAGE_SIZE;
	iq80321_bootconf.ic_kernel_base_pa = memstart;

	/*
	 * Give the XScale global cache clean code an appropriately
	 * sized chunk of unmapped VA space starting at 0xff000000
	 * (our device mappings end before this address).
	 */
	xscale_cache_clean_addr = 0xff000000U;

	/*
	 * Now bootstrap the VM system
	 */
	kstack = initarm_common(&iq80321_bootconf);

	/* Setup the IRQ system */
	i80321_intr_init();

#ifdef IPKDB
	/* Initialise ipkdb */
	ipkdb_init();
	if (boothowto & RB_KDB)
		ipkdb_connect(0);
#endif

#ifdef DDB
	db_machine_init();

	/* Firmware doesn't load symbols. */
	ddb_init(0, NULL, NULL);

	if (boothowto & RB_KDB)
		Debugger();
#endif

	/* We return the new stack pointer address */
	return (kstack);
}

void
consinit(void)
{
	static const bus_addr_t comcnaddrs[] = {
		IQ80321_UART1,		/* com0 */
	};
	static int consinit_called;

	if (consinit_called != 0)
		return;

	consinit_called = 1;

#if NCOM > 0
	if (comcnattach(&obio_bs_tag, comcnaddrs[comcnunit], comcnspeed,
	    COM_FREQ, comcnmode))
		panic("can't init serial console @%lx", comcnaddrs[comcnunit]);
#else
	panic("serial console @%lx not configured", comcnaddrs[comcnunit]);
#endif
}
