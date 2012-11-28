/*	$NetBSD: beagle_machdep.c,v 1.13.10.1 2012/11/28 22:50:04 matt Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: beagle_machdep.c,v 1.13.10.1 2012/11/28 22:50:04 matt Exp $");

#include "opt_machdep.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_ipkdb.h"
#include "opt_md.h"
#include "opt_com.h"
#include "opt_omap.h"
#include "prcm.h"

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

#include <arm/omap/omap_com.h>
#include <arm/omap/omap_var.h>
#include <arm/omap/omap_wdtvar.h>
#include <arm/omap/omap2_prcm.h>

#include <evbarm/include/autoconf.h>
#include <evbarm/beagle/beagle.h>

#include "prcm.h"
#include "omapwdt32k.h"

BootConfig bootconfig;		/* Boot config storage */
char *boot_args = NULL;
char *boot_file = NULL;

u_int uboot_args[4] = { 0 };	/* filled in by beagle_start.S (not in bss) */

/* Same things, but for the free (unused by the kernel) memory. */

extern char KERNEL_BASE_phys[];
extern char _end[];

/*
 * Macros to translate between physical and virtual for a subset of the
 * kernel address space.  *Not* for general use.
 */
#define KERNEL_BASE_PHYS ((paddr_t)KERNEL_BASE_phys)

/* Prototypes */

void consinit(void);
#ifdef KGDB
static void kgdb_port_init(void);
#endif

static void init_clocks(void);
static void beagle_device_register(device_t, void *);
static void beagle_reset(void);
#if defined(OMAP_3530) || defined(TI_DM37XX)
static void omap3_cpu_clk(void);
#endif
#if defined(OMAP_4430)
static void omap4_cpu_clk(void);
#endif

bs_protos(bs_notimpl);

#include "com.h"
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
		/*
		 * Map the first 1MB of L4 Core area
		 * this gets us the ICU, I2C, USB, GPT[10-11], MMC, McSPI
		 * UART[12], clock manager, sDMA, ...
		 */
		.pd_va = _A(OMAP_L4_CORE_VBASE),
		.pd_pa = _A(OMAP_L4_CORE_BASE),
		.pd_size = _S(OMAP_L4_CORE_SIZE),
		.pd_prot = VM_PROT_READ|VM_PROT_WRITE,
		.pd_cache = PTE_NOCACHE
	},
	{
		/*
		 * Map the all 1MB of the L4 Core area
		 * this gets us the console UART3, GPT[2-9], WDT1, 
		 * and GPIO[2-6].
		 */
		.pd_va = _A(OMAP_L4_PERIPHERAL_VBASE),
		.pd_pa = _A(OMAP_L4_PERIPHERAL_BASE),
		.pd_size = _S(OMAP_L4_PERIPHERAL_SIZE),
		.pd_prot = VM_PROT_READ|VM_PROT_WRITE,
		.pd_cache = PTE_NOCACHE
	},
#if defined(OMAP_L4_WAKEUP_BASE) && defined(OMAP_L4_WAKEUP_VBASE)
	{
		/*
		 * Map all 256KB of the L4 Wakeup area
		 * this gets us GPIO1, WDT2, GPT1, 32K and power/reset regs
		 */
		.pd_va = _A(OMAP_L4_WAKEUP_VBASE),
		.pd_pa = _A(OMAP_L4_WAKEUP_BASE),
		.pd_size = _S(OMAP_L4_WAKEUP_SIZE),
		.pd_prot = VM_PROT_READ|VM_PROT_WRITE,
		.pd_cache = PTE_NOCACHE
	},
#endif
#ifdef OMAP_L4_FAST_BASE
	{
		/*
		 * Map all of the L4 Fast area
		 * this gets us GPIO1, WDT2, GPT1, 32K and power/reset regs
		 */
		.pd_va = _A(OMAP_L4_FAST_VBASE),
		.pd_pa = _A(OMAP_L4_FAST_BASE),
		.pd_size = _S(OMAP_L4_FAST_SIZE),
		.pd_prot = VM_PROT_READ|VM_PROT_WRITE,
		.pd_cache = PTE_NOCACHE
	},
#endif
#ifdef OMAP_L4_ABE_BASE
	{
		/*
		 * Map all of the L4 Fast area
		 * this gets us GPIO1, WDT2, GPT1, 32K and power/reset regs
		 */
		.pd_va = _A(OMAP_L4_ABE_VBASE),
		.pd_pa = _A(OMAP_L4_ABE_BASE),
		.pd_size = _S(OMAP_L4_ABE_SIZE),
		.pd_prot = VM_PROT_READ|VM_PROT_WRITE,
		.pd_cache = PTE_NOCACHE
	},
#endif
	{0}
};

#undef	_A
#undef	_S

#ifdef DDB
static void
beagle_db_trap(int where)
{
#if NOMAPWDT32K > 0
	static int oldwatchdogstate;

	if (where) {
		oldwatchdogstate = omapwdt32k_enable(0);
	} else {
		omapwdt32k_enable(oldwatchdogstate);
	}
#endif
}
#endif

void beagle_putchar(char c);
void
beagle_putchar(char c)
{
	unsigned char *com0addr = (char *)CONSADDR_VA;
	int timo = 150000;

	while ((com0addr[5 * 4] & 0x20) == 0)
		if (--timo == 0)
			break;

	com0addr[0] = c;

	while ((com0addr[5 * 4] & 0x20) == 0)
		if (--timo == 0)
			break;
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
#if 1
	beagle_putchar('d');
#endif
	/*
	 * When we enter here, we are using a temporary first level
	 * translation table with section entries in it to cover the OBIO
	 * peripherals and SDRAM.  The temporary first level translation table
	 * is at the end of SDRAM.
	 */
#if defined(OMAP_3530) || defined(TI_DM37XX)
	omap3_cpu_clk();		// find our CPU speed.
#endif
#if defined(OMAP_4430)
	omap4_cpu_clk();		// find our CPU speed.
#endif
	/* Heads up ... Setup the CPU / MMU / TLB functions. */
	if (set_cpufuncs())
		panic("cpu not recognized!");

	init_clocks();

	/* The console is going to try to map things.  Give pmap a devmap. */
	pmap_devmap_register(devmap);
	consinit();
#if 1
	beagle_putchar('h');
#endif
	printf("uboot arg = %#x, %#x, %#x, %#x\n",
	    uboot_args[0], uboot_args[1], uboot_args[2], uboot_args[3]);
#ifdef KGDB
	kgdb_port_init();
#endif

	cpu_reset_address = beagle_reset;

#ifdef VERBOSE_INIT_ARM
	/* Talk to the user */
	printf("\nNetBSD/evbarm (beagle) booting ...\n");
#endif

#ifdef BOOT_ARGS
	char mi_bootargs[] = BOOT_ARGS;
	parse_mi_bootargs(mi_bootargs);
#endif

#ifdef VERBOSE_INIT_ARM
	printf("initarm: Configuring system ...\n");
#endif

	/*
	 * Set up the variables that define the availability of physical
	 * memory.
	 */
#define	MEMSIZE_BYTES 	(MEMSIZE * 1024 * 1024)

	/* Fake bootconfig structure for the benefit of pmap.c. */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = KERNEL_BASE_PHYS & -0x400000;
	bootconfig.dram[0].pages = MEMSIZE_BYTES;

	arm32_bootmem_init(bootconfig.dram[0].address, MEMSIZE_BYTES,
	    KERNEL_BASE_PHYS);
	arm32_kernel_vm_init(KERNEL_VM_BASE, ARM_VECTORS_HIGH, 0, devmap, false);

	/* we've a specific device_register routine */
	evbarm_device_register = beagle_device_register;

	db_trap_callback = beagle_db_trap;

	return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE, NULL, 0);
}

static void
init_clocks(void)
{
#ifdef NOTYET
	static volatile uint32_t * const clksel_reg = (volatile uint32_t *) (OMAP3530_L4_WAKEUP_VBASE + OMAP2_CM_BASE + OMAP2_CM_CLKSEL_MPU - OMAP3530_L4_WAKEUP_BASE);
	uint32_t v;
	beagle_putchar('E');
	v = *clksel_reg;
	beagle_putchar('F');
	if (v != OMAP3530_CM_CLKSEL_MPU_FULLSPEED) {
		printf("Changed CPU speed from half (%d) ", v);
		*clksel_reg = OMAP3530_CM_CLKSEL_MPU_FULLSPEED;
		printf("to full speed.\n");
	}
	beagle_putchar('G');
#endif
}

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

void
consinit(void)
{
	bus_space_handle_t bh;
	static int consinit_called = 0;

	if (consinit_called != 0)
		return;

	consinit_called = 1;

	beagle_putchar('e');

	if (bus_space_map(&omap_a4x_bs_tag, consaddr, OMAP_COM_SIZE, 0, &bh))
		panic("Serial console can not be mapped.");

	if (comcnattach(&omap_a4x_bs_tag, consaddr, conspeed,
			OMAP_COM_FREQ, COM_TYPE_NORMAL, conmode))
		panic("Serial console can not be initialized.");

	bus_space_unmap(&omap_a4x_bs_tag, bh, OMAP_COM_SIZE);

	beagle_putchar('f');
	beagle_putchar('g');
}

void
beagle_reset(void)
{
#if defined(OMAP_4430)
	*(volatile uint32_t *)(OMAP_L4_CORE_VBASE + (OMAP_L4_WAKEUP_BASE - OMAP_L4_CORE_BASE) + OMAP4_PRM_RSTCTRL) = OMAP4_PRM_RSTCTRL_WARM;
#else
#if NPRCM > 0
	prcm_cold_reset();
#endif
#if NOMAPWDT32K > 0
	omapwdt32k_reboot();
#endif
#endif
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
	if (bus_space_map(&omap_a4x_bs_tag, comkgdbaddr, OMAP_COM_SIZE, 0, &bh))
		panic("kgdb port can not be mapped.");

	if (com_kgdb_attach(&omap_a4x_bs_tag, comkgdbaddr, comkgdbspeed,
			OMAP_COM_FREQ, COM_TYPE_NORMAL, comkgdbmode))
		panic("KGDB uart can not be initialized.");

	bus_space_unmap(&omap_a4x_bs_tag, bh, OMAP_COM_SIZE);
}
#endif

#if defined(OMAP_3530) || defined(TI_DM37XX)
void
omap3_cpu_clk(void)
{
	const vaddr_t prm_base = OMAP2_PRM_BASE - OMAP_L4_CORE_BASE + OMAP_L4_CORE_VBASE;
	const uint32_t prm_clksel = *(volatile uint32_t *)(prm_base + PLL_MOD + OMAP3_PRM_CLKSEL);
	static const uint32_t prm_clksel_freqs[] = OMAP3_PRM_CLKSEL_FREQS;
	const uint32_t sys_clk = prm_clksel_freqs[__SHIFTOUT(prm_clksel, OMAP3_PRM_CLKSEL_CLKIN)];
	const vaddr_t cm_base = OMAP2_CM_BASE - OMAP_L4_CORE_BASE + OMAP_L4_CORE_VBASE;
	const uint32_t dpll1 = *(volatile uint32_t *)(cm_base + OMAP3_CM_CLKSEL1_PLL_MPU);
	const uint32_t dpll2 = *(volatile uint32_t *)(cm_base + OMAP3_CM_CLKSEL2_PLL_MPU);
	const uint32_t m = __SHIFTOUT(dpll1, OMAP3_CM_CLKSEL1_PLL_MPU_DPLL_MULT);
	const uint32_t n = __SHIFTOUT(dpll1, OMAP3_CM_CLKSEL1_PLL_MPU_DPLL_DIV);
	const uint32_t m2 = __SHIFTOUT(dpll2, OMAP3_CM_CLKSEL2_PLL_MPU_DPLL_CLKOUT_DIV);

	/*
	 * MPU_CLK supplies ARM_FCLK which is twice the CPU frequency.
	 */
	curcpu()->ci_data.cpu_cc_freq = ((sys_clk * m) / ((n + 1) * m2 * 2)) * OMAP3_PRM_CLKSEL_MULT;
}
#endif /* OMAP_3530 || TI_DM37XX */

#if defined(OMAP_4430)
void
omap4_cpu_clk(void)
{
	const vaddr_t prm_base = OMAP2_PRM_BASE - OMAP_L4_CORE_BASE + OMAP_L4_CORE_VBASE;
	const vaddr_t cm_base = OMAP2_CM_BASE - OMAP_L4_CORE_BASE + OMAP_L4_CORE_VBASE;
	static const uint32_t cm_clksel_freqs[] = OMAP4_CM_CLKSEL_FREQS;
	const uint32_t prm_clksel = *(volatile uint32_t *)(prm_base + OMAP4_CM_SYS_CLKSEL);
	const uint32_t sys_clk = cm_clksel_freqs[__SHIFTOUT(prm_clksel, OMAP4_CM_SYS_CLKSEL_CLKIN)];
	const uint32_t dpll1 = *(volatile uint32_t *)(cm_base + OMAP4_CM_CLKSEL_DPLL_MPU);
	const uint32_t dpll2 = *(volatile uint32_t *)(cm_base + OMAP4_CM_DIV_M2_DPLL_MPU);
	const uint32_t m = __SHIFTOUT(dpll1, OMAP4_CM_CLKSEL_DPLL_MPU_DPLL_MULT);
	const uint32_t n = __SHIFTOUT(dpll1, OMAP4_CM_CLKSEL_DPLL_MPU_DPLL_DIV);
	const uint32_t m2 = __SHIFTOUT(dpll2, OMAP4_CM_DIV_M2_DPLL_MPU_DPLL_CLKOUT_DIV);

	/*
	 * MPU_CLK supplies ARM_FCLK which is twice the CPU frequency.
	 */
	curcpu()->ci_data.cpu_cc_freq = ((sys_clk * 2 * m) / ((n + 1) * m2)) * OMAP4_CM_CLKSEL_MULT / 2;
	printf("%s: %"PRIu64": sys_clk=%u m=%u n=%u (%u) m2=%u mult=%u\n",
	    __func__, curcpu()->ci_data.cpu_cc_freq,
	    sys_clk, m, n, n+1, m2, OMAP4_CM_CLKSEL_MULT);
}
#endif /* OMAP_4400 */

void
beagle_device_register(device_t self, void *aux)
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
		mb->mb_iot = &omap_bs_tag;
		return;
	}
 
	/*
	 * We need to tell the A9 Global/Watchdog Timer
	 * what frequency it runs at.
	 */
	if (device_is_a(self, "a9tmr") || device_is_a(self, "a9wdt")) {
		/*
		 * This clock always runs at (arm_clk div 2) and only goes
		 * to timers that are part of the A9 MP core subsystem.
		 */
                prop_dictionary_set_uint32(dict, "frequency",
		    curcpu()->ci_data.cpu_cc_freq / 2);
		return;
	}	
}
