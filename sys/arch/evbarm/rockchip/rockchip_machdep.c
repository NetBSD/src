/*	$NetBSD: rockchip_machdep.c,v 1.4 2014/12/27 02:40:23 jmcneill Exp $ */

/*
 * Machine dependent functions for kernel setup for TI OSK5912 board.
 * Based on lubbock_machdep.c which in turn was based on iq80310_machhdep.c
 *
 * Copyright (c) 2002, 2003, 2005  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec Corporation may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *
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
 * Copyright (c) 2007 Microsoft
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
 *	This product includes software developed by Microsoft
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTERS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rockchip_machdep.c,v 1.4 2014/12/27 02:40:23 jmcneill Exp $");

#include "opt_machdep.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_ipkdb.h"
#include "opt_md.h"
#include "opt_com.h"
#include "opt_rockchip.h"
#include "opt_arm_debug.h"

#include "com.h"
#if 0
#include "prcm.h"
#include "sdhc.h"
#include "ukbd.h"
#endif
#include "arml2cc.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/exec.h>
#include <sys/kernel.h>
#include <sys/ksyms.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/termios.h>
#include <sys/gpio.h>

#include <uvm/uvm_extern.h>

#include <sys/conf.h>
#include <dev/cons.h>
#include <dev/md.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include <machine/bootconfig.h>
#include <arm/armreg.h>
#include <arm/undefined.h>

#include <arm/arm32/machdep.h>
#include <arm/mainbus/mainbus.h>

#include <dev/ic/ns16550reg.h>
#include <dev/ic/comreg.h>

#include <arm/rockchip/rockchip_reg.h>
#include <arm/rockchip/rockchip_crureg.h>
#include <arm/rockchip/rockchip_var.h>

#ifdef CPU_CORTEXA9
#include <arm/cortex/pl310_reg.h>
#include <arm/cortex/scu_reg.h>

#include <arm/cortex/a9tmr_var.h>
#include <arm/cortex/pl310_var.h>
#endif

#if defined(CPU_CORTEXA7) || defined(CPU_CORTEXA15)
#include <arm/cortex/gtmr_var.h>
#endif

#include <evbarm/include/autoconf.h>
#include <evbarm/rockchip/platform.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/ddcreg.h>

#include <dev/usb/ukbdvar.h>

BootConfig bootconfig;		/* Boot config storage */
static char bootargs[MAX_BOOT_STRING];
char *boot_args = NULL;
char *boot_file = NULL;
#if 0
static uint8_t rockchip_edid[128];	/* EDID storage */
#endif
u_int uboot_args[4] = { 0 };	/* filled in by rockchip_start.S (not in bss) */

/* Same things, but for the free (unused by the kernel) memory. */

extern char KERNEL_BASE_phys[];
extern char _end[];

#if NCOM > 0
int use_fb_console = false;
#else
int use_fb_console = true;
#endif

/*
 * Macros to translate between physical and virtual for a subset of the
 * kernel address space.  *Not* for general use.
 */
#define KERNEL_BASE_PHYS ((paddr_t)KERNEL_BASE_phys)
#define	ROCKCHIP_CORE0_VOFFSET	(ROCKCHIP_CORE0_VBASE - ROCKCHIP_CORE0_BASE)
#define	ROCKCHIP_CORE1_VOFFSET	(ROCKCHIP_CORE1_VBASE - ROCKCHIP_CORE1_BASE)
/* Prototypes */

void consinit(void);
#ifdef KGDB
static void kgdb_port_init(void);
#endif

static void init_clocks(void);
static void rockchip_device_register(device_t, void *);
static void rockchip_reset(void);

bs_protos(bs_notimpl);

#if NCOM > 0
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

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

#define	_A(a)	((a) & ~L1_S_OFFSET)
#define	_S(s)	(((s) + L1_S_SIZE - 1) & ~(L1_S_SIZE-1))

static const struct pmap_devmap devmap[] = {
	{
		.pd_va = _A(ROCKCHIP_CORE0_VBASE),
		.pd_pa = _A(ROCKCHIP_CORE0_BASE),
		.pd_size = _S(ROCKCHIP_CORE0_SIZE),
		.pd_prot = VM_PROT_READ|VM_PROT_WRITE,
		.pd_cache = PTE_NOCACHE
	},
	{
		.pd_va = _A(ROCKCHIP_CORE1_VBASE),
		.pd_pa = _A(ROCKCHIP_CORE1_BASE),
		.pd_size = _S(ROCKCHIP_CORE1_SIZE),
		.pd_prot = VM_PROT_READ|VM_PROT_WRITE,
		.pd_cache = PTE_NOCACHE
	},
	{0}
};

#undef	_A
#undef	_S

#ifdef DDB
static void
rockchip_db_trap(int where)
{
	/* NOT YET */
}
#endif

void rockchip_putchar(char c);
void
rockchip_putchar(char c)
{
#if NCOM > 0
	volatile uint32_t *com0addr = (volatile uint32_t *)CONSADDR_VA;
	int timo = 150000;

	while ((com0addr[com_lsr] & LSR_TXRDY) == 0) {
		if (--timo == 0)
			break;
	}

	com0addr[com_data] = c;

	while ((com0addr[com_lsr] & LSR_TXRDY) == 0) {
		if (--timo == 0)
			break;
	}
#endif
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
	psize_t ram_size = 0;
	char *ptr;
	*(volatile int *)CONSADDR_VA  = 0x40;	/* output '@' */
#if 1
	rockchip_putchar('d');
#endif

	pmap_devmap_register(devmap);
	rockchip_bootstrap();

	/* Heads up ... Setup the CPU / MMU / TLB functions. */
	if (set_cpufuncs())
		panic("cpu not recognized!");

	curcpu()->ci_data.cpu_cc_freq = rockchip_cpu_get_rate();

	init_clocks();

	consinit();
#ifdef CPU_CORTEXA15
#ifdef MULTIPROCESSOR
	arm_cpu_max = 1 + __SHIFTOUT(armreg_l2ctrl_read(), L2CTRL_NUMCPU);
#endif
#endif

#if NARML2CC > 0
        /*
         * Probe the PL310 L2CC
         */
	printf("probe the PL310 L2CC\n");
        const bus_space_handle_t pl310_bh =
            ROCKCHIP_CORE0_VBASE + ROCKCHIP_PL310_OFFSET;
        arml2cc_init(&rockchip_a4x_bs_tag, pl310_bh, 0);
        rockchip_putchar('l');
#endif

	printf("\nuboot arg = %#x, %#x, %#x, %#x\n",
	    uboot_args[0], uboot_args[1], uboot_args[2], uboot_args[3]);


#ifdef KGDB
	kgdb_port_init();
#endif

	cpu_reset_address = rockchip_reset;

#ifdef VERBOSE_INIT_ARM
	/* Talk to the user */
	printf("\nNetBSD/evbarm (rockchip) booting ...\n");
#endif

#ifdef BOOT_ARGS
	char mi_bootargs[] = BOOT_ARGS;
	parse_mi_bootargs(mi_bootargs);
#endif

#ifdef VERBOSE_INIT_ARM
	printf("initarm: Configuring system ...\n");
#endif

#if !defined(CPU_CORTEXA8)
	printf("initarm: cbar=%#x\n", armreg_cbar_read());
	printf("KERNEL_BASE=0x%x, KERNEL_VM_BASE=0x%x, KERNEL_VM_BASE - KERNEL_BASE=0x%x, KERNEL_BASE_VOFFSET=0x%x\n",
		KERNEL_BASE, KERNEL_VM_BASE, KERNEL_VM_BASE - KERNEL_BASE, KERNEL_BASE_VOFFSET);
#endif

#ifdef __HAVE_MM_MD_DIRECT_MAPPED_PHYS
	if (ram_size > KERNEL_VM_BASE - KERNEL_BASE) {
		printf("%s: dropping RAM size from %luMB to %uMB\n",
		    __func__, (unsigned long) (ram_size >> 20),     
		    (KERNEL_VM_BASE - KERNEL_BASE) >> 20);
		ram_size = KERNEL_VM_BASE - KERNEL_BASE;
	}
#endif

	/*
	 * If MEMSIZE specified less than what we really have, limit ourselves
	 * to that.
	 */
#ifdef MEMSIZE
	if (ram_size == 0 || ram_size > (unsigned)MEMSIZE * 1024 * 1024)
		ram_size = (unsigned)MEMSIZE * 1024 * 1024;
	printf("ram_size = 0x%x\n", (int)ram_size);
#else
	KASSERTMSG(ram_size > 0, "RAM size unknown and MEMSIZE undefined");
#endif

	/* Fake bootconfig structure for the benefit of pmap.c. */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = 0x60000000; /* SDRAM PHY addr */
	bootconfig.dram[0].pages = ram_size / PAGE_SIZE;

#ifdef __HAVE_MM_MD_DIRECT_MAPPED_PHYS
	const bool mapallmem_p = true;
	KASSERT(ram_size <= KERNEL_VM_BASE - KERNEL_BASE);
#else
	const bool mapallmem_p = false;
#endif
	KASSERT((armreg_pfr1_read() & ARM_PFR1_SEC_MASK) != 0);

	arm32_bootmem_init(bootconfig.dram[0].address, ram_size,
	    KERNEL_BASE_PHYS);
	arm32_kernel_vm_init(KERNEL_VM_BASE, ARM_VECTORS_LOW, 0, devmap,
	    mapallmem_p);

#ifdef __HAVE_MM_MD_DIRECT_MAPPED_PHYS
	/* "bootargs" env variable is passed as 4th argument to kernel */
	if (uboot_args[3] - 0x80000000 < ram_size) {
		strlcpy(bootargs, (char *)uboot_args[3], sizeof(bootargs));
	}
#endif
	boot_args = bootargs;
	parse_mi_bootargs(boot_args);

	/* we've a specific device_register routine */
	evbarm_device_register = rockchip_device_register;

	db_trap_callback = rockchip_db_trap;

	if (get_bootconf_option(boot_args, "console",
		    BOOTOPT_TYPE_STRING, &ptr) && strncmp(ptr, "fb", 2) == 0) {
		use_fb_console = true;
	}
	
	return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE, NULL, 0);

}

static void
init_clocks(void)
{
	/* NOT YET */
}

#if NCOM > 0
#ifndef CONSADDR
#error Specify the address of the console UART with the CONSADDR option.
#endif
#ifndef CONSPEED
#define CONSPEED 115200
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

static const bus_addr_t consaddr = CONSADDR;
static const int conspeed = CONSPEED;
static const int conmode = CONMODE;
#endif

void
consinit(void)
{
#if NCOM > 0
	bus_space_handle_t bh;
#endif
	static int consinit_called = 0;

	if (consinit_called != 0)
		return;

	consinit_called = 1;

	rockchip_putchar('e');

#if NCOM > 0
	if (bus_space_map(&rockchip_a4x_bs_tag, consaddr, ROCKCHIP_UART_SIZE, 0, &bh))
		panic("Serial console can not be mapped.");

	if (comcnattach(&rockchip_a4x_bs_tag, consaddr, conspeed,
			ROCKCHIP_UART_FREQ, COM_TYPE_NORMAL, conmode))
		panic("Serial console can not be initialized.");

	bus_space_unmap(&rockchip_a4x_bs_tag, bh, ROCKCHIP_UART_SIZE);
#endif


#if NUKBD > 0
	ukbd_cnattach();	/* allow USB keyboard to become console */
#endif

	rockchip_putchar('f');
	rockchip_putchar('g');
}

void
rockchip_reset(void)
{
	bus_space_tag_t bst = &rockchip_bs_tag;
	bus_space_handle_t bsh;

	bus_space_subregion(bst, rockchip_core1_bsh,
	    ROCKCHIP_CRU_OFFSET, ROCKCHIP_CRU_SIZE, &bsh);

	bus_space_write_4(bst, bsh, CRU_GLB_SRST_FST_REG,
	    CRU_GLB_SRST_FST_MAGIC);
}

#ifdef KGDB
#ifndef KGDB_DEVADDR
#error Specify the address of the kgdb UART with the KGDB_DEVADDR option.
#endif
#ifndef KGDB_DEVRATE
#define KGDB_DEVRATE 115200
#endif

#ifndef KGDB_DEVMODE
#define KGDB_DEVMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
static const vaddr_t comkgdbaddr = KGDB_DEVADDR;
static const int comkgdbspeed = KGDB_DEVRATE;
static const int comkgdbmode = KGDB_DEVMODE;

void
static kgdb_port_init(void)
{
	static int kgdbsinit_called = 0;

	if (kgdbsinit_called != 0)
		return;

	kgdbsinit_called = 1;

	bus_space_handle_t bh;
	if (bus_space_map(&rockchip_a4x_bs_tag, comkgdbaddr, ROCKCHIP_COM_SIZE, 0, &bh))
		panic("kgdb port can not be mapped.");

	if (com_kgdb_attach(&rockchip_a4x_bs_tag, comkgdbaddr, comkgdbspeed,
			ROCKCHIP_COM_FREQ, COM_TYPE_NORMAL, comkgdbmode))
		panic("KGDB uart can not be initialized.");

	bus_space_unmap(&rockchip_a4x_bs_tag, bh, ROCKCHIP_COM_SIZE);
}
#endif

void
rockchip_device_register(device_t self, void *aux)
{
	prop_dictionary_t dict = device_properties(self);

	if (device_is_a(self, "armperiph")
	    && device_is_a(device_parent(self), "mainbus")) {
		/*
		 * XXX KLUDGE ALERT XXX
		 * The iot mainbus supplies is completely wrong since it scales
		 * addresses by 2.  The simpliest remedy is to replace with our
		 * bus space used for the armcore regisers (which armperiph uses). 
		 */
		struct mainbus_attach_args * const mb = aux;
		mb->mb_iot = &rockchip_bs_tag;
		return;
	}

#ifdef CPU_CORTEXA9 
	/*
	 * We need to tell the A9 Global/Watchdog Timer
	 * what frequency it runs at.
	 */
	if (device_is_a(self, "a9tmr") || device_is_a(self, "a9wdt")) {
		/*
		 * This clock always runs at (arm_clk div 2) and only goes
		 * to timers that are part of the A9 MP core subsystem.
		 */
                prop_dictionary_set_uint32(dict, "frequency", 148500000); /* 148.5MHz */
		return;
	}

	if (device_is_a(self, "arml2cc")) {
                prop_dictionary_set_uint32(dict, "offset", 0xffffc000); /* -0x4000 */
		return;
	}
#endif
}
