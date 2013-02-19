/*	$NetBSD: integrator_machdep.c,v 1.73 2013/02/19 10:57:10 skrll Exp $	*/

/*
 * Copyright (c) 2001,2002 ARM Ltd
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
 * 3. The name of the company may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ARM LTD ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ARM LTD
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
 * Machine dependent functions for kernel setup for integrator board
 *
 * Created      : 24/11/97
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: integrator_machdep.c,v 1.73 2013/02/19 10:57:10 skrll Exp $");

#include "opt_ddb.h"
#include "opt_pmap_debug.h"

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

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <machine/bootconfig.h>
#include <sys/bus.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/intr.h>
#include <arm/undefined.h>

#include <arm/arm32/machdep.h>

#include <evbarm/integrator/integrator_boot.h>

#include "pci.h"
#include "ksyms.h"

void ifpga_reset(void) __attribute__((noreturn));

/*
 * The range 0xc1000000 - 0xccffffff is available for kernel VM space
 * Core-logic registers and I/O mappings occupy 0xfd000000 - 0xffffffff
 */
#define KERNEL_VM_BASE		(KERNEL_BASE + 0x01000000)
#define KERNEL_VM_SIZE		0x0C000000

BootConfig bootconfig;		/* Boot config storage */
char *boot_args = NULL;
char *boot_file = NULL;

/* Prototypes */

static void	integrator_sdram_bounds	(paddr_t *, psize_t *);

void	consinit(void);

/* A load of console goo. */
#include "vga.h"
#if NVGA > 0
#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>
#endif

#include "pckbc.h"
#if NPCKBC > 0
#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>
#endif

#include "com.h"
#if NCOM > 0
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#ifndef CONCOMADDR
#define CONCOMADDR 0x3f8
#endif
#endif

/*
 * Define the default console speed for the board.  This is generally
 * what the firmware provided with the board defaults to.
 */
#ifndef CONSPEED
#define CONSPEED B115200
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

int comcnspeed = CONSPEED;
int comcnmode = CONMODE;

#include "plcom.h"
#if (NPLCOM > 0)
#include <evbarm/dev/plcomreg.h>
#include <evbarm/dev/plcomvar.h>

#include <evbarm/ifpga/ifpgamem.h>
#include <evbarm/ifpga/ifpgareg.h>
#include <evbarm/ifpga/ifpgavar.h>
#endif

#ifndef CONSDEVNAME
#define CONSDEVNAME "plcom"
#endif

#ifndef PLCONSPEED
#define PLCONSPEED B38400
#endif
#ifndef PLCONMODE
#define PLCONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
#ifndef PLCOMCNUNIT
#define PLCOMCNUNIT -1
#endif

int plcomcnspeed = PLCONSPEED;
int plcomcnmode = PLCONMODE;

#if 0
extern struct consdev kcomcons;
static void kcomcnputc(dev_t, int);
#endif

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

	/*
	 * If we are still cold then hit the air brakes
	 * and crash to earth fast
	 */
	if (cold) {
		doshutdownhooks();
		pmf_system_shutdown(boothowto);
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
		printf("rebooting...\n");
		ifpga_reset();
		/*NOTREACHED*/
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

	pmf_system_shutdown(boothowto);

	/* Make sure IRQ's are disabled */
	IRQdisable;

	if (howto & RB_HALT) {
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
	}

	printf("rebooting...\n");
	ifpga_reset();
	/*NOTREACHED*/
}

/* Statically mapped devices. */
static const struct pmap_devmap integrator_devmap[] = {
#if NPLCOM > 0 && defined(PLCONSOLE)
	{
		UART0_BOOT_BASE,
		IFPGA_IO_BASE + IFPGA_UART0,
		1024 * 1024,
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE
	},

	{
		UART1_BOOT_BASE,
		IFPGA_IO_BASE + IFPGA_UART1,
		1024 * 1024,
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE
	},
#endif
#if NPCI > 0
	{
		IFPGA_PCI_IO_VBASE,
		IFPGA_PCI_IO_BASE,
		IFPGA_PCI_IO_VSIZE,
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE
	},

	{
		IFPGA_PCI_CONF_VBASE,
		IFPGA_PCI_CONF_BASE,
		IFPGA_PCI_CONF_VSIZE,
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE
	},
#endif

	{
		0,
		0,
		0,
		0,
		0
	}
};

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
	extern int KERNEL_BASE_phys[];
	paddr_t memstart;
	psize_t memsize;

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())
		panic("cpu not recognized!");

	/* map some peripheral registers */

	pmap_devmap_bootstrap((vaddr_t)armreg_ttbr_read() & -L1_TABLE_SIZE,
		integrator_devmap);

	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);

	consinit();

	/* Talk to the user */
#define BDSTR(s)        _BDSTR(s)
#define _BDSTR(s)       #s
	printf("\nNetBSD/evbarm (" BDSTR(EVBARM_BOARDTYPE) ") booting ...\n");

	/*
	 * Fetch the SDRAM start/size from the CM configuration registers.
	 */
	integrator_sdram_bounds(&memstart, &memsize);

#if defined(INTEGRATOR_CP)
	/*
	 * XXX QEMU reports SDRAM starting at 0x100000, but presents a flat
	 * physical memory model. Set memstart to 0x0, so arm32_bootmem_init
	 * doesn't get fooled later.
	 */
	memstart = 0;
#endif

#ifdef VERBOSE_INIT_ARM
	printf("initarm: Configuring system ...\n");
#endif

	/* Fake bootconfig structure for the benefit of pmap.c */
	/* XXX must make the memory description h/w independent */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = memstart;
	bootconfig.dram[0].pages = memsize / PAGE_SIZE;
	bootconfig.dram[0].flags = BOOT_DRAM_CAN_DMA | BOOT_DRAM_PREFER;

	arm32_bootmem_init(bootconfig.dram[0].address,
		bootconfig.dram[0].pages * PAGE_SIZE, (unsigned int) KERNEL_BASE_phys);

	arm32_kernel_vm_init(KERNEL_VM_BASE, ARM_VECTORS_LOW, 0, integrator_devmap,
		false);

#ifdef VERBOSE_INIT_ARM
	printf("done.\n");
#endif

	return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE, NULL, 0);
}

void
consinit(void)
{
	static int consinit_called = 0;
#if 0
	char *console = CONSDEVNAME;
#endif

	if (consinit_called != 0)
		return;

	consinit_called = 1;

#if NPLCOM > 0 && defined(PLCONSOLE)
	if (PLCOMCNUNIT == 0) {
		extern struct bus_space ifpga_common_bs_tag;
		static struct plcom_instance ifpga_pi1 = {
#if defined(INTEGRATOR_CP)
			.pi_type = PLCOM_TYPE_PL011,
#else
			.pi_type = PLCOM_TYPE_PL010,
#endif
			.pi_iot = &ifpga_common_bs_tag,
			.pi_size = IFPGA_UART_SIZE,
			.pi_iobase = IFPGA_UART0
		};

		if (plcomcnattach(&ifpga_pi1, plcomcnspeed, IFPGA_UART_CLK,
		      plcomcnmode, PLCOMCNUNIT))
			panic("can't init serial console");
		return;
	} else if (PLCOMCNUNIT == 1) {
		extern struct bus_space ifpga_common_bs_tag;
		static struct plcom_instance ifpga_pi1 = {
#if defined(INTEGRATOR_CP)
			.pi_type = PLCOM_TYPE_PL011,
#else
			.pi_type = PLCOM_TYPE_PL010,
#endif
			.pi_iot = &ifpga_common_bs_tag,
			.pi_size = IFPGA_UART_SIZE,
			.pi_iobase = IFPGA_UART1
		};

		if (plcomcnattach(&ifpga_pi1, plcomcnspeed, IFPGA_UART_CLK,
		      plcomcnmode, PLCOMCNUNIT))
			panic("can't init serial console");
		return;
	}
#endif
#if (NCOM > 0)
	if (comcnattach(&isa_io_bs_tag, CONCOMADDR, comcnspeed,
	    COM_FREQ, COM_TYPE_NORMAL, comcnmode))
		panic("can't init serial console @%x", CONCOMADDR);
	return;
#endif
	panic("No serial console configured");
}

static void
integrator_sdram_bounds(paddr_t *memstart, psize_t *memsize)
{
	volatile unsigned long *cm_sdram 
	    = (volatile unsigned long *)0x10000020;
	volatile unsigned long *cm_stat
	    = (volatile unsigned long *)0x10000010;

	*memstart = *cm_stat & 0x00ff0000;

	/*
	 * Although the SSRAM overlaps the SDRAM, we can use the wrap-around
	 * to access the entire bank.
	 */
	switch ((*cm_sdram >> 2) & 0x7)
	{
	case 0:
		*memsize = 16 * 1024 * 1024;
		break;
	case 1:
		*memsize = 32 * 1024 * 1024;
		break;
	case 2:
		*memsize = 64 * 1024 * 1024;
		break;
	case 3:
		*memsize = 128 * 1024 * 1024;
		break;
	case 4:
		/* With 256M of memory there is no wrap-around.  */
		*memsize = 256 * 1024 * 1024 - *memstart;
		break;
	default:
		printf("CM_SDRAM retuns unknown value, using 16M\n");
		*memsize = 16 * 1024 * 1024;
		break;
	}
}
