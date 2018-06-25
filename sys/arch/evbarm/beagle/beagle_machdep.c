/*	$NetBSD: beagle_machdep.c,v 1.68.14.1 2018/06/25 07:25:40 pgoyette Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: beagle_machdep.c,v 1.68.14.1 2018/06/25 07:25:40 pgoyette Exp $");

#include "opt_machdep.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_ipkdb.h"
#include "opt_md.h"
#include "opt_com.h"
#include "opt_omap.h"

#include "com.h"
#include "omapwdt32k.h"
#include "prcm.h"
#include "sdhc.h"
#include "ukbd.h"
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

#include <arm/omap/omap_com.h>
#include <arm/omap/omap_var.h>
#include <arm/omap/omap_wdtvar.h>
#include <arm/omap/omap2_prcm.h>
#include <arm/omap/omap2_gpio.h>
#ifdef TI_AM335X
# if NPRCM == 0
#  error no prcm device configured.
# endif
# include <arm/omap/am335x_prcm.h>
# include <arm/omap/tifbvar.h>
# if NSDHC > 0
#  include <arm/omap/omap2_obiovar.h>
#  include <arm/omap/omap3_sdmmcreg.h>
# endif
#endif

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
#include <evbarm/beagle/beagle.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/ddcreg.h>

#include <dev/usb/ukbdvar.h>

BootConfig bootconfig;		/* Boot config storage */
static char bootargs[MAX_BOOT_STRING];
char *boot_args = NULL;
char *boot_file = NULL;

static uint8_t beagle_edid[128];	/* EDID storage */

u_int uboot_args[4] = { 0 };	/* filled in by beagle_start.S (not in bss) */

/* Same things, but for the free (unused by the kernel) memory. */

extern char KERNEL_BASE_phys[];
extern char _end[];

#if NCOM > 0
int use_fb_console = false;
#else
int use_fb_console = true;
#endif

#ifdef CPU_CORTEXA15
uint32_t omap5_cnt_frq;
#endif

/*
 * Macros to translate between physical and virtual for a subset of the
 * kernel address space.  *Not* for general use.
 */
#define KERNEL_BASE_PHYS ((paddr_t)KERNEL_BASE_phys)
#define	OMAP_L4_CORE_VOFFSET	(OMAP_L4_CORE_VBASE - OMAP_L4_CORE_BASE)

/* Prototypes */

void consinit(void);
#ifdef KGDB
static void kgdb_port_init(void);
#endif

static void init_clocks(void);
static void beagle_device_register(device_t, void *);
static void beagle_reset(void);
#if defined(OMAP_3XXX) || defined(TI_DM37XX)
static void omap3_cpu_clk(void);
#endif
#if defined(OMAP_4XXX) || defined(OMAP_5XXX)
static void omap4_cpu_clk(void);
#endif
#if defined(OMAP_4XXX) || defined(OMAP_5XXX) || defined(TI_AM335X)
static psize_t emif_memprobe(void);
#endif

#if defined(OMAP_3XXX)
static psize_t omap3_memprobe(void);
#endif

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
#ifdef OMAP_EMIF1_BASE
	{
		/*
		 * Map all of the L4 EMIF1 area
		 */
		.pd_va = _A(OMAP_EMIF1_VBASE),
		.pd_pa = _A(OMAP_EMIF1_BASE),
		.pd_size = _S(OMAP_EMIF1_SIZE),
		.pd_prot = VM_PROT_READ|VM_PROT_WRITE,
		.pd_cache = PTE_NOCACHE
	},
#endif
#ifdef OMAP_EMIF2_BASE
	{
		/*
		 * Map all of the L4 EMIF2 area
		 */
		.pd_va = _A(OMAP_EMIF2_VBASE),
		.pd_pa = _A(OMAP_EMIF2_BASE),
		.pd_size = _S(OMAP_EMIF2_SIZE),
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
#ifdef OMAP_SDRC_BASE
	{
		/*
		 * Map SDRAM Controller (SDRC) registers
		 */
		.pd_va = _A(OMAP_SDRC_VBASE),
		.pd_pa = _A(OMAP_SDRC_BASE),
		.pd_size = _S(OMAP_SDRC_SIZE),
		.pd_prot = VM_PROT_READ|VM_PROT_WRITE,
		.pd_cache = PTE_NOCACHE,
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

#ifdef VERBOSE_INIT_ARM
void beagle_putchar(char c);
void
beagle_putchar(char c)
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
#else
#define beagle_putchar(c)	((void)0)
#endif

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

#if 1
	beagle_putchar('d');
#endif

	/*
	 * When we enter here, we are using a temporary first level
	 * translation table with section entries in it to cover the OBIO
	 * peripherals and SDRAM.  The temporary first level translation table
	 * is at the end of SDRAM.
	 */
#if defined(OMAP_3XXX) || defined(TI_DM37XX)
	omap3_cpu_clk();		// find our CPU speed.
#endif
#if defined(OMAP_4XXX) || defined(OMAP_5XXX)
	omap4_cpu_clk();		// find our CPU speed.
#endif
#if defined(TI_AM335X)
	prcm_bootstrap(OMAP2_CM_BASE + OMAP_L4_CORE_VOFFSET);
	// find our reference clock.
	am335x_sys_clk(TI_AM335X_CTLMOD_BASE + OMAP_L4_CORE_VOFFSET);
	am335x_cpu_clk();		// find our CPU speed.
#endif
	/* Heads up ... Setup the CPU / MMU / TLB functions. */
	if (set_cpufuncs())
		panic("cpu not recognized!");

	init_clocks();

	/* The console is going to try to map things.  Give pmap a devmap. */
	pmap_devmap_register(devmap);

	if (get_bootconf_option(bootargs, "console",
		    BOOTOPT_TYPE_STRING, &ptr) && strncmp(ptr, "fb", 2) == 0) {
		use_fb_console = true;
	}
	consinit();

#ifdef CPU_CORTEXA15
#ifdef MULTIPROCESSOR
	arm_cpu_max = 1 + __SHIFTOUT(armreg_l2ctrl_read(), L2CTRL_NUMCPU);
#endif
#endif
#if defined(OMAP_4XXX)
#if NARML2CC > 0
	/*
	 * Probe the PL310 L2CC
	 */
	const bus_space_handle_t pl310_bh = OMAP4_L2CC_BASE
	    + OMAP_L4_PERIPHERAL_VBASE - OMAP_L4_PERIPHERAL_BASE;
	arml2cc_init(&omap_bs_tag, pl310_bh, 0);
	beagle_putchar('l');
#endif
#ifdef MULTIPROCESSOR
	const bus_space_handle_t scu_bh = OMAP4_SCU_BASE
	    + OMAP_L4_PERIPHERAL_VBASE - OMAP_L4_PERIPHERAL_BASE;
	uint32_t scu_cfg = bus_space_read_4(&omap_bs_tag, scu_bh, SCU_CFG);
	arm_cpu_max = 1 + (scu_cfg & SCU_CFG_CPUMAX);
	beagle_putchar('s');
#endif
#endif /* OMAP_4XXX */
#if defined(TI_AM335X) && defined(VERBOSE_INIT_ARM)
	am335x_cpu_clk();		// find our CPU speed.
#endif
#if 1
	beagle_putchar('h');
#endif
	printf("\nuboot arg = %#x, %#x, %#x, %#x\n",
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

#if !defined(CPU_CORTEXA8)
	printf("initarm: cbar=%#x\n", armreg_cbar_read());
#endif

	/*
	 * Set up the variables that define the availability of physical
	 * memory.
	 */
#if defined(OMAP_3XXX)
	ram_size = omap3_memprobe();
#endif
#if defined(OMAP_4XXX) || defined(OMAP_5XXX) || defined(TI_AM335X)
	ram_size = emif_memprobe();
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
#else
	KASSERTMSG(ram_size > 0, "RAM size unknown and MEMSIZE undefined");
#endif

	/* Fake bootconfig structure for the benefit of pmap.c. */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = KERNEL_BASE_PHYS & -0x400000;
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

	beagle_putchar('e');

#if NCOM > 0
	if (bus_space_map(&omap_a4x_bs_tag, consaddr, OMAP_COM_SIZE, 0, &bh))
		panic("Serial console can not be mapped.");

	if (comcnattach(&omap_a4x_bs_tag, consaddr, conspeed,
			OMAP_COM_FREQ, COM_TYPE_NORMAL, conmode))
		panic("Serial console can not be initialized.");

	bus_space_unmap(&omap_a4x_bs_tag, bh, OMAP_COM_SIZE);
#endif

#if NUKBD > 0
	if (use_fb_console)
		ukbd_cnattach(); /* allow USB keyboard to become console */
#endif

	beagle_putchar('f');
	beagle_putchar('g');
}

void
beagle_reset(void)
{
#if defined(OMAP_4XXX)
	*(volatile uint32_t *)(OMAP_L4_CORE_VBASE + (OMAP_L4_WAKEUP_BASE - OMAP_L4_CORE_BASE) + OMAP4_PRM_RSTCTRL) = OMAP4_PRM_RSTCTRL_WARM;
#elif defined(OMAP_5XXX)
	*(volatile uint32_t *)(OMAP_L4_CORE_VBASE + (OMAP_L4_WAKEUP_BASE - OMAP_L4_CORE_BASE) + OMAP5_PRM_RSTCTRL) = OMAP4_PRM_RSTCTRL_COLD;
#elif defined(TI_AM335X)
	*(volatile uint32_t *)(OMAP_L4_CORE_VBASE + (OMAP2_CM_BASE - OMAP_L4_CORE_BASE) + AM335X_PRCM_PRM_DEVICE + PRM_RSTCTRL) = RST_GLOBAL_WARM_SW;
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

#if defined(OMAP_3XXX) || defined(TI_DM37XX)
void
omap3_cpu_clk(void)
{
	const vaddr_t prm_base = OMAP2_PRM_BASE + OMAP_L4_CORE_VOFFSET;
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
	omap_sys_clk = sys_clk * OMAP3_PRM_CLKSEL_MULT;
}
#endif /* OMAP_3XXX || TI_DM37XX */

#if defined(OMAP_4XXX) || defined(OMAP_5XXX)
void
omap4_cpu_clk(void)
{
	const vaddr_t prm_base = OMAP2_PRM_BASE + OMAP_L4_CORE_VOFFSET;
	const vaddr_t cm_base = OMAP2_CM_BASE + OMAP_L4_CORE_VOFFSET;
	static const uint32_t cm_clksel_freqs[] = OMAP4_CM_CLKSEL_FREQS;
	const uint32_t prm_clksel = *(volatile uint32_t *)(prm_base + OMAP4_CM_SYS_CLKSEL);
	const u_int clksel = __SHIFTOUT(prm_clksel, OMAP4_CM_SYS_CLKSEL_CLKIN);
	const uint32_t sys_clk = cm_clksel_freqs[clksel];
	const uint32_t dpll1 = *(volatile uint32_t *)(cm_base + OMAP4_CM_CLKSEL_DPLL_MPU);
	const uint32_t dpll2 = *(volatile uint32_t *)(cm_base + OMAP4_CM_DIV_M2_DPLL_MPU);
	const uint32_t m = __SHIFTOUT(dpll1, OMAP4_CM_CLKSEL_DPLL_MPU_DPLL_MULT);
	const uint32_t n = __SHIFTOUT(dpll1, OMAP4_CM_CLKSEL_DPLL_MPU_DPLL_DIV);
	const uint32_t m2 = __SHIFTOUT(dpll2, OMAP4_CM_DIV_M2_DPLL_MPU_DPLL_CLKOUT_DIV);

	/*
	 * MPU_CLK supplies ARM_FCLK which is twice the CPU frequency.
	 */
	curcpu()->ci_data.cpu_cc_freq = ((sys_clk * 2 * m) / ((n + 1) * m2)) * OMAP4_CM_CLKSEL_MULT / 2;
	omap_sys_clk = sys_clk * OMAP4_CM_CLKSEL_MULT;
	printf("%s: %"PRIu64": sys_clk=%u m=%u n=%u (%u) m2=%u mult=%u\n",
	    __func__, curcpu()->ci_data.cpu_cc_freq,
	    sys_clk, m, n, n+1, m2, OMAP4_CM_CLKSEL_MULT);

#if defined(CPU_CORTEXA15)
	if ((armreg_pfr1_read() & ARM_PFR1_GTIMER_MASK) != 0) {
		beagle_putchar('0');
		uint32_t voffset = OMAP_L4_PERIPHERAL_VBASE - OMAP_L4_PERIPHERAL_BASE;
		uint32_t frac1_reg = OMAP5_PRM_FRAC_INCREMENTER_NUMERATOR; 
		uint32_t frac2_reg = OMAP5_PRM_FRAC_INCREMENTER_DENUMERATOR_RELOAD; 
		uint32_t frac1 = *(volatile uint32_t *)(frac1_reg + voffset);
		beagle_putchar('1');
		uint32_t frac2 = *(volatile uint32_t *)(frac2_reg + voffset);
		beagle_putchar('2');
		uint32_t numer = __SHIFTOUT(frac1, PRM_FRAC_INCR_NUM_SYS_MODE);
		uint32_t denom = __SHIFTOUT(frac2, PRM_FRAC_INCR_DENUM_DENOMINATOR);
		uint32_t freq = (uint64_t)omap_sys_clk * numer / denom;
#if 1
		if (freq != OMAP5_GTIMER_FREQ) {
			static uint16_t numer_demon[8][2] = {
			    {         0,          0 },	/* not used */
			    {  26 *  64,  26 *  125 },	/* 12.0Mhz */
			    {   2 * 768,   2 * 1625 },	/* 13.0Mhz */
			    {         0,          0 },	/* 16.8Mhz (not used) */
			    { 130 *   8, 130 *   25 },	/* 19.2Mhz */
			    {   2 * 384,   2 * 1625 },	/* 26.0Mhz */
			    {   3 * 256,   3 * 1125 },	/* 27.0Mhz */
			    { 130 *   4, 130 *   25 },	/* 38.4Mhz */
			};
			if (numer_demon[clksel][0] != numer) {
				frac1 &= ~PRM_FRAC_INCR_NUM_SYS_MODE;
				frac1 |= numer_demon[clksel][0];
			}
			if (numer_demon[clksel][1] != denom) {
				frac2 &= ~PRM_FRAC_INCR_DENUM_DENOMINATOR;
				frac2 |= numer_demon[clksel][1];
			}
			*(volatile uint32_t *)(frac1_reg + voffset) = frac1;
			*(volatile uint32_t *)(frac2_reg + voffset) = frac2
			    | PRM_FRAC_INCR_DENUM_RELOAD;
			freq = OMAP5_GTIMER_FREQ;
		}
#endif
		beagle_putchar('3');
#if 0
		if (gtimer_freq != freq) {
			armreg_cnt_frq_write(freq);	// secure only
		}
#endif
		omap5_cnt_frq = freq;
		beagle_putchar('4');
	}
#endif
}
#endif /* OMAP_4XXX || OMAP_5XXX */


#if defined(OMAP_4XXX) || defined(OMAP_5XXX) || defined(TI_AM335X)
static inline uint32_t
emif_read_sdram_config(vaddr_t emif_base)
{
#ifdef CPU_CORTEXA15
	return 0x61851b32; // XXX until i figure out why deref emif_base dies
#else
	emif_base += EMIF_SDRAM_CONFIG;
	//printf("%s: sdram_config @ %#"PRIxVADDR" = ", __func__, emif_base);
	uint32_t v = *(const volatile uint32_t *)(emif_base);
	//printf("%#x\n", v);
	return v;
#endif
}

static psize_t 
emif_memprobe(void)
{
	uint32_t sdram_config = emif_read_sdram_config(OMAP_EMIF1_VBASE);
	psize_t memsize = 1L;
#if defined(TI_AM335X)
	/*
	 * The original bbone's u-boot misprograms the EMIF so correct it
	 * if we detect if it has the wrong value.
	 */
	if (sdram_config == 0x41805332)
		sdram_config -= __SHIFTIN(1, SDRAM_CONFIG_RSIZE);
#endif
#ifdef OMAP_EMIF2_VBASE
	/*
	 * OMAP4 and OMAP5 have two EMIFs so if the 2nd one is configured
	 * like the first, we have twice the memory.
	 */
	const uint32_t sdram_config2 = emif_read_sdram_config(OMAP_EMIF2_VBASE);
	if (sdram_config2 == sdram_config)
		memsize <<= 1;
#endif

	const u_int ebank = __SHIFTOUT(sdram_config, SDRAM_CONFIG_EBANK);
	const u_int ibank = __SHIFTOUT(sdram_config, SDRAM_CONFIG_IBANK);
	const u_int rsize = 9 + __SHIFTOUT(sdram_config, SDRAM_CONFIG_RSIZE);
	const u_int pagesize = 8 + __SHIFTOUT(sdram_config, SDRAM_CONFIG_PAGESIZE);
	const u_int width = 2 - __SHIFTOUT(sdram_config, SDRAM_CONFIG_WIDTH);
#ifdef TI_AM335X
	KASSERT(ebank == 0);	// No chip selects on Sitara
#endif
	memsize <<= (ebank + ibank + rsize + pagesize + width);
#ifdef VERBOSE_INIT_ARM
	printf("sdram_config = %#x, memsize = %uMB\n", sdram_config,
	    (u_int)(memsize >> 20));
#endif
	return memsize;
}
#endif

#if defined(OMAP_3XXX)
#define SDRC_MCFG(p)		(0x80 + (0x30 * (p)))
#define SDRC_MCFG_MEMSIZE(m)	((((m) & __BITS(8,17)) >> 8) * 2)
static psize_t 
omap3_memprobe(void)
{
	const vaddr_t gpmc_base = OMAP_SDRC_VBASE;
	const uint32_t mcfg0 = *(volatile uint32_t *)(gpmc_base + SDRC_MCFG(0));
	const uint32_t mcfg1 = *(volatile uint32_t *)(gpmc_base + SDRC_MCFG(1));

	printf("mcfg0 = %#x, size %lld\n", mcfg0, SDRC_MCFG_MEMSIZE(mcfg0));
	printf("mcfg1 = %#x, size %lld\n", mcfg1, SDRC_MCFG_MEMSIZE(mcfg1));

	return (SDRC_MCFG_MEMSIZE(mcfg0) + SDRC_MCFG_MEMSIZE(mcfg1)) * 1024 * 1024;
}
#endif

/*
 * EDID can be read from DVI-D (HDMI) port on BeagleBoard from
 * If EDID data is present, this function fills in the supplied edid_buf
 * and returns true. Otherwise, it returns false and the contents of the
 * buffer are undefined.
 */
static bool
beagle_read_edid(uint8_t *edid_buf, size_t edid_buflen)
{
#if defined(OMAP_3530)
	i2c_tag_t ic = NULL;
	uint8_t reg;
	int error;

	/* On Beagleboard, EDID is accessed using I2C2 ("omapiic2"). */
	extern i2c_tag_t omap3_i2c_get_tag(device_t);
	ic = omap3_i2c_get_tag(device_find_by_xname("omapiic2"));

	if (ic == NULL)
		return false;

	iic_acquire_bus(ic, 0);
	for (reg = DDC_EDID_START; reg < edid_buflen; reg++) {
		error = iic_exec(ic, I2C_OP_READ_WITH_STOP, DDC_ADDR,
		    &reg, sizeof(reg), &edid_buf[reg], 1, 0);
		if (error)
			break;
	}
	iic_release_bus(ic, 0);

	return error == 0 ? true : false;
#else
	return false;
#endif
}

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
		 * bus space used for the armcore registers (which armperiph uses). 
		 */
		struct mainbus_attach_args * const mb = aux;
		mb->mb_iot = &omap_bs_tag;
		return;
	}
 
#ifdef CPU_CORTEXA9
	/*
	 * We need to tell the A9 Global/Watchdog Timer
	 * what frequency it runs at.
	 */
	if (device_is_a(self, "arma9tmr") || device_is_a(self, "a9wdt")) {
		/*
		 * This clock always runs at (arm_clk div 2) and only goes
		 * to timers that are part of the A9 MP core subsystem.
		 */
                prop_dictionary_set_uint32(dict, "frequency",
		    curcpu()->ci_data.cpu_cc_freq / 2);
		return;
	}
#endif

#ifdef CPU_CORTEXA15
	if (device_is_a(self, "armgtmr")) {
		/*
		 * The frequency of the generic timer was figured out when
		 * determined the cpu frequency.
		 */
                prop_dictionary_set_uint32(dict, "frequency", omap5_cnt_frq);
	}
#endif

	if (device_is_a(self, "ehci")) {
#if defined(OMAP_3530)
		/* XXX Beagleboard specific port configuration */
		prop_dictionary_set_uint16(dict, "nports", 3);
		prop_dictionary_set_cstring(dict, "port0-mode", "none");
		prop_dictionary_set_cstring(dict, "port1-mode", "phy");
		prop_dictionary_set_cstring(dict, "port2-mode", "none");
		prop_dictionary_set_bool(dict, "phy-reset", true);
		prop_dictionary_set_int16(dict, "port0-gpio", -1);
		prop_dictionary_set_int16(dict, "port1-gpio", 147);
		prop_dictionary_set_bool(dict, "port1-gpioval", true);
		prop_dictionary_set_int16(dict, "port2-gpio", -1);
		prop_dictionary_set_uint16(dict, "dpll5-m", 443);
		prop_dictionary_set_uint16(dict, "dpll5-n", 11);
		prop_dictionary_set_uint16(dict, "dpll5-m2", 4);
#endif
#if defined(TI_DM37XX)
		/* XXX Beagleboard specific port configuration */
		prop_dictionary_set_uint16(dict, "nports", 3);
		prop_dictionary_set_cstring(dict, "port0-mode", "none");
		prop_dictionary_set_cstring(dict, "port1-mode", "phy");
		prop_dictionary_set_cstring(dict, "port2-mode", "none");
		prop_dictionary_set_bool(dict, "phy-reset", true);
		prop_dictionary_set_int16(dict, "port0-gpio", -1);
		prop_dictionary_set_int16(dict, "port1-gpio", 56);
		prop_dictionary_set_bool(dict, "port1-gpioval", true);
		prop_dictionary_set_int16(dict, "port2-gpio", -1);
#if 0
		prop_dictionary_set_uint16(dict, "dpll5-m", 443);
		prop_dictionary_set_uint16(dict, "dpll5-n", 11);
		prop_dictionary_set_uint16(dict, "dpll5-m2", 4);
#endif
#endif
#if defined(OMAP_4430)
		prop_dictionary_set_uint16(dict, "nports", 2);
		prop_dictionary_set_bool(dict, "phy-reset", false);
		prop_dictionary_set_cstring(dict, "port0-mode", "none");
		prop_dictionary_set_int16(dict, "port0-gpio", -1);
		prop_dictionary_set_cstring(dict, "port1-mode", "phy");
		prop_dictionary_set_int16(dict, "port1-gpio", 62);
		prop_dictionary_set_bool(dict, "port1-gpioval", true);
		omap2_gpio_ctl(1, GPIO_PIN_OUTPUT);
		omap2_gpio_write(1, 1);		// Power Hub
#endif
#if defined(OMAP_5430)
		prop_dictionary_set_uint16(dict, "nports", 3);
		prop_dictionary_set_cstring(dict, "port0-mode", "none");
		prop_dictionary_set_int16(dict, "port0-gpio", -1);
		prop_dictionary_set_cstring(dict, "port1-mode", "hsic");
		prop_dictionary_set_int16(dict, "port1-gpio", -1);
		prop_dictionary_set_cstring(dict, "port2-mode", "hsic");
		prop_dictionary_set_int16(dict, "port2-gpio", -1);
#endif
#if defined(OMAP_5430)
		bus_space_tag_t iot = &omap_bs_tag;
		bus_space_handle_t ioh;
		omap2_gpio_ctl(80, GPIO_PIN_OUTPUT);
		omap2_gpio_write(80, 0);
		prop_dictionary_set_uint16(dict, "nports", 1);
		prop_dictionary_set_cstring(dict, "port0-mode", "hsi");
#if 0
		prop_dictionary_set_bool(dict, "phy-reset", true);
		prop_dictionary_set_int16(dict, "port0-gpio", 80);
		prop_dictionary_set_bool(dict, "port0-gpioval", true);
#endif
		int rv = bus_space_map(iot, OMAP5_CM_CTL_WKUP_REF_CLK0_OUT_REF_CLK1_OUT, 4, 0, &ioh);
		KASSERT(rv == 0);
		uint32_t v = bus_space_read_4(iot, ioh, 0);
		v &= 0xffff;
		v |= __SHIFTIN(OMAP5_CM_CTL_WKUP_MUXMODE1_REF_CLK1_OUT,
		    OMAP5_CM_CTL_WKUP_MUXMODE1);
		bus_space_write_4(iot, ioh, 0, v);
		bus_space_unmap(iot, ioh, 4);

		omap2_gpio_write(80, 1);
#endif
		return;
	}

	if (device_is_a(self, "sdhc")) {
#if defined(OMAP_3430) || defined(OMAP_3530)
		prop_dictionary_set_uint32(dict, "clkmask", 0);
		prop_dictionary_set_bool(dict, "8bit", true);
#endif
#if defined(TI_AM335X)
		struct obio_attach_args * const obio = aux;
		if (obio->obio_addr == SDMMC2_BASE_TIAM335X)
			prop_dictionary_set_bool(dict, "8bit", true);
#endif
		return;
	}

	if (device_is_a(self, "omapfb")) {
		if (beagle_read_edid(beagle_edid, sizeof(beagle_edid))) {
			prop_dictionary_set(dict, "EDID",
			    prop_data_create_data(beagle_edid,
						  sizeof(beagle_edid)));
		}
		if (use_fb_console)
			prop_dictionary_set_bool(dict, "is_console", true);
		return;
	}
#if defined(TI_AM335X)
	if (device_is_a(self, "tifb")) {
		static const struct tifb_panel_info default_panel_info = {
			.panel_tft = 1,
			.panel_mono = false,
			.panel_bpp = 24,

			.panel_pxl_clk = 30000000,
			.panel_width = 800,
			.panel_height = 600,
			.panel_hfp = 0,
			.panel_hbp = 47,
			.panel_hsw = 47,
			.panel_vfp = 0,
			.panel_vbp = 10,
			.panel_vsw = 2,
			.panel_invert_hsync = 1,
			.panel_invert_vsync = 1,

			.panel_ac_bias = 255,
			.panel_ac_bias_intrpt = 0,
			.panel_dma_burst_sz = 16,
			.panel_fdd = 0x80,
			.panel_sync_edge = 0,
			.panel_sync_ctrl = 1,
			.panel_invert_pxl_clk = 0,
		};
		prop_data_t panel_info;

		panel_info = prop_data_create_data_nocopy(&default_panel_info,
		    sizeof(struct tifb_panel_info));
		KASSERT(panel_info != NULL);
		prop_dictionary_set(dict, "panel-info", panel_info);
		prop_object_release(panel_info);

		if (use_fb_console)
			prop_dictionary_set_bool(dict, "is_console", true);
		return;
	}
#endif
	if (device_is_a(self, "com")) {
		if (use_fb_console)
			prop_dictionary_set_bool(dict, "is_console", false);
	}
#if defined(TI_AM335X)
	if (device_is_a(self, "tps65217pmic")) {
		extern const char *mpu_supply;

		mpu_supply = "DCDC2";
	}
#endif
}
