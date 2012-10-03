/*	$NetBSD: rpi_machdep.c,v 1.10 2012/10/03 13:06:06 skrll Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
__KERNEL_RCSID(0, "$NetBSD: rpi_machdep.c,v 1.10 2012/10/03 13:06:06 skrll Exp $");

#include "opt_evbarm_boardtype.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/reboot.h>
#include <sys/termios.h>
#include <sys/bus.h>

#include <dev/cons.h>

#include <uvm/uvm_extern.h>

#include <arm/arm32/machdep.h>

#include <machine/vmparam.h>
#include <machine/bootconfig.h>
#include <machine/pmap.h>

#include <arm/broadcom/bcm2835var.h>
#include <arm/broadcom/bcm2835_pmvar.h>

#include <evbarm/rpi/rpi.h>

#include "plcom.h"
#if (NPLCOM > 0)
#include <evbarm/dev/plcomreg.h>
#include <evbarm/dev/plcomvar.h>
#endif

#include "ksyms.h"


BootConfig bootconfig;		/* Boot config storage */
static char bootargs[MAX_BOOT_STRING];
char *boot_args = NULL;

/*
 * Macros to translate between physical and virtual for a subset of the
 * kernel address space.  *Not* for general use.
 */
#define KERNEL_BASE_PHYS (paddr_t)0
#define KERN_PHYSTOV(pa) \
	((vaddr_t)((paddr_t)pa - KERNEL_BASE_PHYS + KERNEL_BASE))

#define	PLCONADDR 0x20201000

#ifndef CONSDEVNAME
#define CONSDEVNAME "plcom"
#endif

#ifndef PLCONSPEED
#define PLCONSPEED B115200
#endif
#ifndef PLCONMODE
#define PLCONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
#ifndef PLCOMCNUNIT
#define PLCOMCNUNIT -1
#endif

#if (NPLCOM > 0)
static const bus_addr_t consaddr = (bus_addr_t)PLCONADDR;

int plcomcnspeed = PLCONSPEED;
int plcomcnmode = PLCONMODE;
#endif

#include "opt_kgdb.h"
#ifdef KGDB
#include <sys/kgdb.h>
#endif

/* Smallest amount of RAM start.elf could give us. */
#define RPI_MINIMUM_ARM_RAM_SPLIT (128U * 1024 * 1024)

static inline
pd_entry_t *
read_ttb(void)
{
	long ttb;

	__asm volatile("mrc   p15, 0, %0, c2, c0, 0" : "=r" (ttb));

	return (pd_entry_t *)(ttb & ~((1<<14)-1));
}

/*
 * Static device mappings. These peripheral registers are mapped at
 * fixed virtual addresses very early in initarm() so that we can use
 * them while booting the kernel, and stay at the same address
 * throughout whole kernel's life time.
 *
 * We use this table twice; once with bootstrap page table, and once
 * with kernel's page table which we build up in initarm().
 *
 * Since we map these registers into the bootstrap page table using
 * pmap_devmap_bootstrap() which calls pmap_map_chunk(), we map
 * registers segment-aligned and segment-rounded in order to avoid
 * using the 2nd page tables.
 */

#define _A(a)	((a) & ~L1_S_OFFSET)
#define _S(s)	(((s) + L1_S_SIZE - 1) & ~(L1_S_SIZE-1))

static const struct pmap_devmap rpi_devmap[] = {
	{
		_A(RPI_KERNEL_IO_VBASE),	/* 0xf2000000 */
		_A(RPI_KERNEL_IO_PBASE),	/* 0x20000000 */
		_S(RPI_KERNEL_IO_VSIZE),	/* 16Mb */
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE,
	},
	{ 0, 0, 0, 0, 0 }
};

#undef  _A
#undef  _S

#define LINUX_ARM_MACHTYPE_BCM2708 3138

#define LINUX_ATAG_NONE		0x00000000
struct linux_atag_header {
	uint32_t size;
	uint32_t tag;
} __packed __aligned(4); 

#define LINUX_ATAG_MEM		0x54410002
struct linux_atag_mem {
	uint32_t size;
	uint32_t start;
} __packed __aligned(4);

#define LINUX_ATAG_CMDLINE	0x54410009
struct linux_atag_cmdline {
	char cmdline[1];
} __packed __aligned(4);

struct linux_atag {
	struct linux_atag_header	hdr;
	union {
		struct linux_atag_mem		mem;
		struct linux_atag_cmdline	cmdline;
	} u;
} __packed __aligned(4);

static void
parse_linux_atags(void *atag_base)
{
	struct linux_atag *atp;

	for (atp = atag_base;
	    atp->hdr.size >= sizeof(struct linux_atag_header)/sizeof(uint32_t);
	    atp = (void *)((uintptr_t)atp + sizeof(uint32_t) * atp->hdr.size)) {
		printf("atag: size %08x tag %08x\n", atp->hdr.size, atp->hdr.tag);
		if (atp->hdr.tag == LINUX_ATAG_MEM) {
			if (bootconfig.dramblocks > 1) {
				printf("Ignoring RAM block 0x%08x-0x%08x\n",
				    atp->u.mem.start, atp->u.mem.start +
				    atp->u.mem.size - 1);
				continue;
			}
			KASSERT(atp->u.mem.start == 0);
			bootconfig.dram[bootconfig.dramblocks].address = 0x0;
			bootconfig.dram[bootconfig.dramblocks].pages =
			    atp->u.mem.size / PAGE_SIZE;
			++bootconfig.dramblocks;
		}

		if (atp->hdr.tag == LINUX_ATAG_CMDLINE) {
			strncpy(bootargs, atp->u.cmdline.cmdline,
			    sizeof(bootargs));
		}
	}
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
 */
u_int
initarm(void *arg)
{

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())
		panic("cpu not recognized!");

	/* map some peripheral registers */
	pmap_devmap_bootstrap((vaddr_t)read_ttb(), rpi_devmap);

	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);

	consinit();

	/* Talk to the user */
#define BDSTR(s)	_BDSTR(s)
#define _BDSTR(s)	#s
	printf("\nNetBSD/evbarm (" BDSTR(EVBARM_BOARDTYPE) ") booting ...\n");

	bootargs[0] = '\0';

#ifdef VERBOSE_INIT_ARM
	printf("initarm: Configuring system ...\n");
#endif

	extern const uint32_t rpi_boot_regs[4];
	if (rpi_boot_regs[0] == 0 &&
	    rpi_boot_regs[1] == LINUX_ARM_MACHTYPE_BCM2708) {
		parse_linux_atags((void *)KERN_PHYSTOV(rpi_boot_regs[2]));
	} else {
		bootconfig.dramblocks = 1;

		bootconfig.dram[0].address = 0x0;
		bootconfig.dram[0].pages = 
		    RPI_MINIMUM_ARM_RAM_SPLIT / PAGE_SIZE;
	}
	arm32_bootmem_init(bootconfig.dram[0].address,
	    bootconfig.dram[0].pages * PAGE_SIZE, bootconfig.dram[0].address);
	arm32_kernel_vm_init(KERNEL_VM_BASE, ARM_VECTORS_HIGH, 0, rpi_devmap,
	    false);

	cpu_reset_address = bcm2835_system_reset;

#ifdef VERBOSE_INIT_ARM
	printf("done.\n");
#endif

#ifdef __HAVE_MEMORY_DISK__
	md_root_setconf(memory_disk, sizeof memory_disk);
#endif

	boot_args = bootargs;
	parse_mi_bootargs(boot_args);

#ifdef BOOTHOWTO
	boothowto |= BOOTHOWTO;
#endif

	return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE, NULL, 0);
}

void
consinit(void)
{
	static int consinit_called = 0;
#if (NPLCOM > 0 && defined(PLCONSOLE))
	static struct plcom_instance rpi_pi = {
		.pi_type = PLCOM_TYPE_PL011,
		.pi_flags = PLC_FLAG_32BIT_ACCESS,
		.pi_iot = &bcm2835_bs_tag,
		.pi_size = BCM2835_UART0_SIZE
	};
#endif
	if (consinit_called != 0)
		return;

	consinit_called = 1;

#if (NPLCOM > 0 && defined(PLCONSOLE))
	/*
	 * Initialise the diagnostic serial console
	 * This allows a means of generating output during initarm().
	 */
	rpi_pi.pi_iobase = consaddr;
	
	plcomcnattach(&rpi_pi, plcomcnspeed, BCM2835_UART0_CLK,
	    plcomcnmode, PLCOMCNUNIT);

#endif
}

