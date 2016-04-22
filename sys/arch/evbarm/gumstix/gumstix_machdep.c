/*	$NetBSD: gumstix_machdep.c,v 1.50.4.1 2016/04/22 15:44:09 skrll Exp $ */
/*
 * Copyright (C) 2005, 2006, 2007  WIDE Project and SOUM Corporation.
 * All rights reserved.
 *
 * Written by Takashi Kiyohara and Susumu Miki for WIDE Project and SOUM
 * Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the name of SOUM Corporation
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT and SOUM CORPORATION ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT AND SOUM CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 2002, 2003, 2004, 2005  Genetec Corporation.
 * All rights reserved.
 *
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
 * Machine dependent functions for kernel setup for Genetec G4250EBX
 * evaluation board.
 *
 * Based on iq80310_machhdep.c
 */
/*
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
 * Machine dependent functions for kernel setup for Intel IQ80310 evaluation
 * boards using RedBoot firmware.
 */

#include "opt_evbarm_boardtype.h"
#include "opt_cputypes.h"
#include "opt_gumstix.h"
#ifdef OVERO
#include "opt_omap.h"
#include "prcm.h"
#endif
#include "opt_kgdb.h"
#include "opt_pmap_debug.h"
#include "opt_com.h"

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/exec.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/termios.h>
#include <sys/bus.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/bootconfig.h>
#include <arm/locore.h>

#include <arm/arm32/machdep.h>
#ifdef OVERO
#include <arm/omap/omap2_gpmcreg.h>
#include <arm/omap/omap2_obiovar.h>
#include <arm/omap/omap2_prcm.h>
#include <arm/omap/omap2_reg.h>
#include <arm/omap/omap_var.h>
#include <arm/omap/omap_com.h>
#endif
#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>
#include <evbarm/gumstix/gumstixreg.h>
#include <evbarm/gumstix/gumstixvar.h>

#include <dev/cons.h>

#ifdef KGDB
#include <sys/kgdb.h>
#endif

/*
 * The range 0xc1000000 - 0xcfffffff is available for kernel VM space
 * Core-logic registers and I/O mappings occupy 0xfd000000 - 0xffffffff
 */
#ifndef KERNEL_VM_BASE
#define	KERNEL_VM_BASE		0xc1000000
#endif
#define KERNEL_VM_SIZE		0x0f000000

BootConfig bootconfig;		/* Boot config storage */
static char bootargs[MAX_BOOT_STRING];
const size_t bootargs_len = sizeof(bootargs) - 1;	/* without nul */
char *boot_args = NULL;

uint32_t system_serial_high;
uint32_t system_serial_low;

/* Prototypes */
#if defined(GUMSTIX)
static void	read_system_serial(void);
#elif defined(OVERO)
static void	overo_reset(void);
static void	find_cpu_clock(void);
#endif
static void	process_kernel_args(int, char *[]);
static void	process_kernel_args_liner(char *);
#ifdef KGDB
static void	kgdb_port_init(void);
#endif
static void	gumstix_device_register(device_t, void *);

bs_protos(bs_notimpl);

#include "com.h"
#if NCOM > 0
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

#if defined(CPU_XSCALE_PXA250) || defined(CPU_XSCALE_PXA270)
#include "lcd.h"
#endif

#ifndef CONSPEED
#define CONSPEED B115200	/* It's a setting of the default of u-boot */
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

int comcnspeed = CONSPEED;
int comcnmode = CONMODE;

#ifdef GUMSTIX_NETBSD_ARGS_CONSOLE
static char console[16];
#endif

extern void gxio_config_pin(void);
extern void gxio_config_expansion(char *);


static inline pd_entry_t *
read_ttb(void)
{
	long ttb;

	__asm volatile("mrc	p15, 0, %0, c2, c0, 0" : "=r" (ttb));

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

#define	_A(a)	((a) & ~L1_S_OFFSET)
#define	_S(s)	(((s) + L1_S_SIZE - 1) & ~(L1_S_SIZE-1))

static const struct pmap_devmap gumstix_devmap[] = {
#if defined(GUMSTIX)
	{
		GUMSTIX_GPIO_VBASE,
		_A(PXA2X0_GPIO_BASE),
		_S(PXA250_GPIO_SIZE),
		VM_PROT_READ | VM_PROT_WRITE,
		PTE_NOCACHE,
	},
	{
		GUMSTIX_CLKMAN_VBASE,
		_A(PXA2X0_CLKMAN_BASE),
		_S(PXA2X0_CLKMAN_SIZE),
		VM_PROT_READ | VM_PROT_WRITE,
		PTE_NOCACHE,
	},
	{
		GUMSTIX_INTCTL_VBASE,
		_A(PXA2X0_INTCTL_BASE),
		_S(PXA2X0_INTCTL_SIZE),
		VM_PROT_READ | VM_PROT_WRITE,
		PTE_NOCACHE,
	},
	{
		GUMSTIX_FFUART_VBASE,
		_A(PXA2X0_FFUART_BASE),
		_S(4 * COM_NPORTS),
		VM_PROT_READ | VM_PROT_WRITE,
		PTE_NOCACHE,
	},
	{
		GUMSTIX_STUART_VBASE,
		_A(PXA2X0_STUART_BASE),
		_S(4 * COM_NPORTS),
		VM_PROT_READ | VM_PROT_WRITE,
		PTE_NOCACHE,
	},
	{
		GUMSTIX_BTUART_VBASE,
		_A(PXA2X0_BTUART_BASE),
		_S(4 * COM_NPORTS),
		VM_PROT_READ | VM_PROT_WRITE,
		PTE_NOCACHE,
	},
	{
		GUMSTIX_HWUART_VBASE,
		_A(PXA2X0_HWUART_BASE),
		_S(4 * COM_NPORTS),
		VM_PROT_READ | VM_PROT_WRITE,
		PTE_NOCACHE,
	},
	{
		GUMSTIX_LCDC_VBASE,
		_A(PXA2X0_LCDC_BASE),
		_S(4 * COM_NPORTS),
		VM_PROT_READ | VM_PROT_WRITE,
		PTE_NOCACHE,
	},
#elif defined(OVERO)
	{
		OVERO_L4_CORE_VBASE,
		_A(OMAP3530_L4_CORE_BASE),
		_S(L1_S_SIZE),		/* No need 16MB.  Use only first 1MB */
		VM_PROT_READ | VM_PROT_WRITE,
		PTE_NOCACHE
	},
	{
		OVERO_L4_PERIPHERAL_VBASE,
		_A(OMAP3530_L4_PERIPHERAL_BASE),
		_S(OMAP3530_L4_PERIPHERAL_SIZE),
		VM_PROT_READ | VM_PROT_WRITE,
		PTE_NOCACHE
	},
	{
		OVERO_GPMC_VBASE,
		_A(GPMC_BASE),
		_S(GPMC_SIZE),
		VM_PROT_READ | VM_PROT_WRITE,
		PTE_NOCACHE
	},
#endif
	{ 0, 0, 0, 0, 0 }
};

#undef	_A
#undef	_S


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
	extern char KERNEL_BASE_phys[];
	extern uint32_t *u_boot_args[];
	extern uint32_t ram_size;
	enum { r0 = 0, r1 = 1, r2 = 2, r3 = 3 }; /* args from u-boot */

	/*
	 * We mapped PA == VA in gumstix_start.S.
	 * Also mapped SDRAM to KERNEL_BASE first 64Mbyte only with cachable.
	 *
	 * Gumstix (basix, connex, verdex, verdex-pro):
	 * Physical Address Range     Description
	 * -----------------------    ----------------------------------
	 * 0x00000000 - 0x00ffffff    flash Memory   (16MB or 4MB)
	 * 0x40000000 - 0x480fffff    Processor Registers
	 * 0xa0000000 - 0xa3ffffff    SDRAM Bank 0 (64MB or 128MB)
	 * 0xc0000000 - 0xc3ffffff    KERNEL_BASE
	 *
	 * Overo:
	 * Physical Address Range     Description
	 * -----------------------    ----------------------------------
	 * 0x80000000 - 0x8fffffff    SDRAM Bank 0 (256MB or 512MB)
	 * 0x80000000 - 0x83ffffff    KERNEL_BASE
	 */

#if defined(GUMSTIX)
	cpu_reset_address = NULL;
#elif defined(OVERO)
	cpu_reset_address = overo_reset;

	find_cpu_clock();	// find our CPU speed.
#endif

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())
		panic("cpu not recognized!");

	/* map some peripheral registers at static I/O area */
	pmap_devmap_bootstrap((vaddr_t)read_ttb(), gumstix_devmap);

#if defined(CPU_XSCALE_PXA250) || defined(CPU_XSCALE_PXA270)
	/* start 32.768kHz OSC */
	ioreg_write(GUMSTIX_CLKMAN_VBASE + CLKMAN_OSCC, OSCC_OON);

	/* Get ready for splfoo() */
	pxa2x0_intr_bootstrap(GUMSTIX_INTCTL_VBASE);

	/* setup GPIO for {FF,ST,HW}UART. */
	pxa2x0_gpio_bootstrap(GUMSTIX_GPIO_VBASE);

	pxa2x0_clkman_bootstrap(GUMSTIX_CLKMAN_VBASE);
#elif defined(CPU_CORTEX)
	cortex_pmc_ccnt_init();
#endif

	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);

	/* configure GPIOs. */
	gxio_config_pin();


#ifndef GUMSTIX_NETBSD_ARGS_CONSOLE
	consinit();
#endif
#ifdef KGDB
	kgdb_port_init();
#endif

	/*
	 * Examine the boot args string for options we need to know about
	 * now.
	 */
#if defined(GUMSTIX)
#define SDRAM_START	0xa0000000UL
#elif defined(OVERO)
#define SDRAM_START	0x80000000UL
#endif
	if (((uint32_t)u_boot_args[r0] & 0xf0000000) != SDRAM_START)
		/* Maybe r0 is 'argc'.  We are booted by command 'go'. */
		process_kernel_args((int)u_boot_args[r0],
		    (char **)u_boot_args[r1]);
	else
		/*
		 * Maybe r3 is 'boot args string' of 'bootm'.  This string is
		 * linely.
		 */
		process_kernel_args_liner((char *)u_boot_args[r3]);
#ifdef GUMSTIX_NETBSD_ARGS_CONSOLE
	consinit();
#endif

	/* Talk to the user */
#define BDSTR(s)	_BDSTR(s)
#define _BDSTR(s)	#s
	printf("\nNetBSD/evbarm (" BDSTR(EVBARM_BOARDTYPE) ") booting ...\n");

	/* Read system serial */
#if defined(GUMSTIX)
	read_system_serial();
#endif

#ifdef VERBOSE_INIT_ARM
	printf("initarm: Configuring system ...\n");
#endif

	/* Fake bootconfig structure for the benefit of pmap.c */
	/* XXX must make the memory description h/w independent */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = SDRAM_START;
	bootconfig.dram[0].pages = ram_size / PAGE_SIZE;

	KASSERT(ram_size <= KERNEL_VM_BASE - KERNEL_BASE);

	arm32_bootmem_init(bootconfig.dram[0].address, ram_size,
	    (uintptr_t) KERNEL_BASE_phys);
	arm32_kernel_vm_init(KERNEL_VM_BASE,
#if defined(CPU_XSCALE_PXA250) || defined(CPU_XSCALE_PXA270)
	    ARM_VECTORS_LOW,
#elif defined(CPU_CORTEXA8)
	    ARM_VECTORS_HIGH,
#endif
	    0, gumstix_devmap, true);

	evbarm_device_register = gumstix_device_register;

	return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE, NULL, 0);
}

#if defined(GUMSTIX)
static void
read_system_serial(void)
{
#define GUMSTIX_SYSTEM_SERIAL_ADDR	0
#define GUMSTIX_SYSTEM_SERIAL_SIZE	8
#define FLASH_OFFSET_INTEL_PROTECTION	0x81
#define FLASH_OFFSET_USER_PROTECTION	0x85
#define FLASH_CMD_READ_ID		0x90
#define FLASH_CMD_RESET			0xff
	int i;
	char system_serial[GUMSTIX_SYSTEM_SERIAL_SIZE], *src;
	char x;

	src = (char *)(FLASH_OFFSET_USER_PROTECTION * 2 /*word*/);
	*(volatile uint16_t *)0 = FLASH_CMD_READ_ID;
	memcpy(system_serial,
	    src + GUMSTIX_SYSTEM_SERIAL_ADDR, sizeof (system_serial));
	*(volatile uint16_t *)0 = FLASH_CMD_RESET;

	for (i = 1, x = system_serial[0]; i < sizeof (system_serial); i++)
		x &= system_serial[i];
	if (x == 0xff) {
		src = (char *)(FLASH_OFFSET_INTEL_PROTECTION * 2 /*word*/);
		*(volatile uint16_t *)0 = FLASH_CMD_READ_ID;
		memcpy(system_serial,
		    src + GUMSTIX_SYSTEM_SERIAL_ADDR, sizeof (system_serial));
		*(volatile uint16_t *)0 = FLASH_CMD_RESET;

		/*
		 * XXXX: Don't need ???
		 * gumstix_serial_hash(system_serial);
		 */
	}
	system_serial_high = system_serial[0] << 24 | system_serial[1] << 16 |
	    system_serial[2] << 8 | system_serial[3];
	system_serial_low = system_serial[4] << 24 | system_serial[5] << 16 |
	    system_serial[6] << 8 | system_serial[7];

	printf("system serial: 0x");
	for (i = 0; i < sizeof (system_serial); i++)
		printf("%02x", system_serial[i]);
	printf("\n");
}

#elif defined(OVERO)

static void
overo_reset(void)
{

#if NPRCM > 0
	prcm_cold_reset();
#endif
}

static void
find_cpu_clock(void)
{
	const vaddr_t prm_base = OMAP2_PRM_BASE;
	const vaddr_t cm_base = OMAP2_CM_BASE;
	const uint32_t prm_clksel =
	    *(volatile uint32_t *)(prm_base + PLL_MOD + OMAP3_PRM_CLKSEL);
	static const uint32_t prm_clksel_freqs[] = OMAP3_PRM_CLKSEL_FREQS;
	const uint32_t sys_clk =
	    prm_clksel_freqs[__SHIFTOUT(prm_clksel, OMAP3_PRM_CLKSEL_CLKIN)];
	const uint32_t dpll1 =
	    *(volatile uint32_t *)(cm_base + OMAP3_CM_CLKSEL1_PLL_MPU);
	const uint32_t dpll2 =
	    *(volatile uint32_t *)(cm_base + OMAP3_CM_CLKSEL2_PLL_MPU);
	const uint32_t m =
	    __SHIFTOUT(dpll1, OMAP3_CM_CLKSEL1_PLL_MPU_DPLL_MULT);
	const uint32_t n = __SHIFTOUT(dpll1, OMAP3_CM_CLKSEL1_PLL_MPU_DPLL_DIV);
	const uint32_t m2 =
	    __SHIFTOUT(dpll2, OMAP3_CM_CLKSEL2_PLL_MPU_DPLL_CLKOUT_DIV);

	/*
	 * MPU_CLK supplies ARM_FCLK which is twice the CPU frequency.
	 */
	curcpu()->ci_data.cpu_cc_freq =
	    ((sys_clk * m) / ((n + 1) * m2 * 2)) * OMAP3_PRM_CLKSEL_MULT;
	omap_sys_clk = sys_clk * OMAP3_PRM_CLKSEL_MULT;
}
#endif

#ifdef GUMSTIX_NETBSD_ARGS_BUSHEADER
static const char busheader_name[] = "busheader=";
#endif
#if defined(GUMSTIX_NETBSD_ARGS_BUSHEADER) || \
    defined(GUMSTIX_NETBSD_ARGS_EXPANSION)
static const char expansion_name[] = "expansion=";
#endif
#ifdef GUMSTIX_NETBSD_ARGS_CONSOLE
static const char console_name[] = "console=";
#endif
static void
process_kernel_args(int argc, char *argv[])
{
	int gxio_configured = 0, i, j;

	boothowto = 0;

	for (i = 1, j = 0; i < argc; i++) {
#ifdef GUMSTIX_NETBSD_ARGS_BUSHEADER
		if (!strncmp(argv[i], busheader_name, strlen(busheader_name))) {
			/* Configure for GPIOs of busheader side */
			gxio_config_expansion(argv[i] + strlen(busheader_name));
			gxio_configured = 1;
			continue;
		}
#endif
#if defined(GUMSTIX_NETBSD_ARGS_BUSHEADER) || \
    defined(GUMSTIX_NETBSD_ARGS_EXPANSION)
		if (!strncmp(argv[i], expansion_name, strlen(expansion_name))) {
			/* Configure expansion */
			gxio_config_expansion(argv[i] + strlen(expansion_name));
			gxio_configured = 1;
			continue;
		}
#endif
#ifdef GUMSTIX_NETBSD_ARGS_CONSOLE
		if (!strncmp(argv[i], console_name, strlen(console_name))) {
			strncpy(console, argv[i] + strlen(console_name),
			    sizeof(console));
			consinit();
		}
#endif
		if (j == bootargs_len) {
			*(bootargs + j) = '\0';
			continue;
		}
		if (j != 0)
			*(bootargs + j++) = ' ';
		strncpy(bootargs + j, argv[i], bootargs_len - j);
		bootargs[bootargs_len] = '\0';
		j += strlen(argv[i]);
	}
	boot_args = bootargs;

	parse_mi_bootargs(boot_args);

	if (!gxio_configured)
		gxio_config_expansion(NULL);
}

static void
process_kernel_args_liner(char *args)
{
	int i = 0;
	char *p = NULL;

	boothowto = 0;

	strncpy(bootargs, args, sizeof(bootargs));
#if defined(GUMSTIX_NETBSD_ARGS_BUSHEADER) || \
    defined(GUMSTIX_NETBSD_ARGS_EXPANSION)
	{
		char *q;

		if ((p = strstr(bootargs, expansion_name)))
			q = p + strlen(expansion_name);
#ifdef GUMSTIX_NETBSD_ARGS_BUSHEADER
		else if ((p = strstr(bootargs, busheader_name)))
			q = p + strlen(busheader_name);
#endif
		if (p) {
			char expansion[256], c;

			i = 0;
			do {
				c = *(q + i);
				if (c == ' ')
					c = '\0';
				expansion[i++] = c;
			} while (c != '\0' && i < sizeof(expansion));
			gxio_config_expansion(expansion);
			strcpy(p, q + i);
		}
	}
#endif
	if (p == NULL)
		gxio_config_expansion(NULL);
#ifdef GUMSTIX_NETBSD_ARGS_CONSOLE
	p = strstr(bootargs, console_name);
	if (p != NULL) {
		char c;

		i = 0;
		do {
			c = *(p + strlen(console_name) + i);
			if (c == ' ')
				c = '\0';
			console[i++] = c;
		} while (c != '\0' && i < sizeof(console));
		consinit();
		strcpy(p, p + strlen(console_name) + i);
	}
#endif
	boot_args = bootargs;

	parse_mi_bootargs(boot_args);
}

#ifdef KGDB
#ifndef KGDB_DEVNAME
#define KGDB_DEVNAME	"ffuart"
#endif
const char kgdb_devname[] = KGDB_DEVNAME;

#ifndef KGDB_DEVRATE
#define KGDB_DEVRATE	CONSPEED
#endif
int kgdb_devrate = KGDB_DEVRATE;

#if (NCOM > 0)
#ifndef KGDB_DEVMODE
#define KGDB_DEVMODE	CONMODE
#endif
int comkgdbmode = KGDB_DEVMODE;
#endif /* NCOM */

#endif /* KGDB */


void
consinit(void)
{
	static int consinit_called = 0;

	if (consinit_called != 0)
		return;

	consinit_called = 1;

#if NCOM > 0

#ifdef GUMSTIX_NETBSD_ARGS_CONSOLE
	/* Maybe passed Linux's bootargs 'console=ttyS?,<speed>...' */
	if (strncmp(console, "ttyS", 4) == 0 && console[5] == ',') {
		int i;

		comcnspeed = 0;
		for (i = 6; i < strlen(console) && isdigit(console[i]); i++)
			comcnspeed = comcnspeed * 10 + (console[i] - '0');
	}
#endif

#if defined(GUMSTIX)

#ifdef FFUARTCONSOLE
#ifdef KGDB
	if (strcmp(kgdb_devname, "ffuart") == 0){
		/* port is reserved for kgdb */
	} else
#endif
#if defined(GUMSTIX_NETBSD_ARGS_CONSOLE)
	if (console[0] == '\0' || strcasecmp(console, "ffuart") == 0 ||
	    strncmp(console, "ttyS0,", 6) == 0)
#endif
	{
		int rv;

		rv = comcnattach(&pxa2x0_a4x_bs_tag, PXA2X0_FFUART_BASE,
		    comcnspeed, PXA2X0_COM_FREQ, COM_TYPE_PXA2x0, comcnmode);
		if (rv == 0) {
			pxa2x0_clkman_config(CKEN_FFUART, 1);
			return;
		}
	}
#endif /* FFUARTCONSOLE */

#ifdef STUARTCONSOLE
#ifdef KGDB
	if (strcmp(kgdb_devname, "stuart") == 0) {
		/* port is reserved for kgdb */
	} else
#endif
#if defined(GUMSTIX_NETBSD_ARGS_CONSOLE)
	if (console[0] == '\0' || strcasecmp(console, "stuart") == 0)
#endif
	{
		int rv;

		rv = comcnattach(&pxa2x0_a4x_bs_tag, PXA2X0_STUART_BASE,
		    comcnspeed, PXA2X0_COM_FREQ, COM_TYPE_PXA2x0, comcnmode);
		if (rv == 0) {
			pxa2x0_clkman_config(CKEN_STUART, 1);
			return;
		}
	}
#endif /* STUARTCONSOLE */

#ifdef BTUARTCONSOLE
#ifdef KGDB
	if (strcmp(kgdb_devname, "btuart") == 0) {
		/* port is reserved for kgdb */
	} else
#endif
#if defined(GUMSTIX_NETBSD_ARGS_CONSOLE)
	if (console[0] == '\0' || strcasecmp(console, "btuart") == 0)
#endif
	{
		int rv;

		rv = comcnattach(&pxa2x0_a4x_bs_tag, PXA2X0_BTUART_BASE,
		    comcnspeed, PXA2X0_COM_FREQ, COM_TYPE_PXA2x0, comcnmode);
		if (rv == 0) {
			pxa2x0_clkman_config(CKEN_BTUART, 1);
			return;
		}
	}
#endif /* BTUARTCONSOLE */

#ifdef HWUARTCONSOLE
#ifdef KGDB
	if (strcmp(kgdb_devname, "hwuart") == 0) {
		/* port is reserved for kgdb */
	} else
#endif
#if defined(GUMSTIX_NETBSD_ARGS_CONSOLE)
	if (console[0] == '\0' || strcasecmp(console, "hwuart") == 0)
#endif
	{
		rv = comcnattach(&pxa2x0_a4x_bs_tag, PXA2X0_HWUART_BASE,
		    comcnspeed, PXA2X0_COM_FREQ, COM_TYPE_PXA2x0, comcnmode);
		if (rv == 0) {
			pxa2x0_clkman_config(CKEN_HWUART, 1);
			return;
		}
	}
#endif /* HWUARTCONSOLE */

#elif defined(OVERO)

	if (comcnattach(&omap_a4x_bs_tag, 0x49020000, comcnspeed,
	    OMAP_COM_FREQ, COM_TYPE_NORMAL, comcnmode) == 0)
		return;

#endif /* GUMSTIX or OVERO */

#endif /* NCOM */

#if NLCD > 0
#if defined(GUMSTIX_NETBSD_ARGS_CONSOLE)
	if (console[0] == '\0' || strcasecmp(console, "lcd") == 0)
#endif
	{
		gxlcd_cnattach();
	}
#endif
}

#ifdef KGDB
static void
kgdb_port_init(void)
{
#if (NCOM > 0) && defined(COM_PXA2X0)
	paddr_t paddr = 0;
	int cken = 0;

	if (0 == strcmp(kgdb_devname, "ffuart")) {
		paddr = PXA2X0_FFUART_BASE;
		cken = CKEN_FFUART;
	} else if (0 == strcmp(kgdb_devname, "stuart")) {
		paddr = PXA2X0_STUART_BASE;
		cken = CKEN_STUART;
	} else if (0 == strcmp(kgdb_devname, "btuart")) {
		paddr = PXA2X0_BTUART_BASE;
		cken = CKEN_BTUART;
	} else if (0 == strcmp(kgdb_devname, "hwuart")) {
		paddr = PXA2X0_HWUART_BASE;
		cken = CKEN_HWUART;
	}

	if (paddr &&
	    0 == com_kgdb_attach(&pxa2x0_a4x_bs_tag, paddr,
		kgdb_devrate, PXA2X0_COM_FREQ, COM_TYPE_PXA2x0, comkgdbmode)) {

		pxa2x0_clkman_config(cken, 1);
	}

#endif
}
#endif

static void
gumstix_device_register(device_t dev, void *aux)
{
	prop_dictionary_t dict = device_properties(dev);

	if (device_is_a(dev, "ehci")) {
		prop_dictionary_set_uint16(dict, "nports", 2);
		prop_dictionary_set_bool(dict, "phy-reset", true);
		prop_dictionary_set_cstring(dict, "port0-mode", "none");
		prop_dictionary_set_int16(dict, "port0-gpio", -1);
		prop_dictionary_set_cstring(dict, "port1-mode", "phy");
		prop_dictionary_set_int16(dict, "port1-gpio", 183);
		prop_dictionary_set_bool(dict, "port1-gpioval", true);
		prop_dictionary_set_uint16(dict, "dpll5-m", 443);
		prop_dictionary_set_uint16(dict, "dpll5-n", 11);
		prop_dictionary_set_uint16(dict, "dpll5-m2", 4);
	}
	if (device_is_a(dev, "ohci")) {
		if (prop_dictionary_set_bool(dict,
		    "Ganged-power-mask-on-port1", 1) == false) {
			printf("WARNING: unable to set power-mask for port1"
			    " property for %s\n", device_xname(dev));
		}
		if (prop_dictionary_set_bool(dict,
		    "Ganged-power-mask-on-port2", 1) == false) {
			printf("WARNING: unable to set power-mask for port2"
			    " property for %s\n", device_xname(dev));
		}
		if (prop_dictionary_set_bool(dict,
		    "Ganged-power-mask-on-port3", 1) == false) {
			printf("WARNING: unable to set power-mask for port3"
			    " property for %s\n", device_xname(dev));
		}
	}
	if (device_is_a(dev, "omapmputmr")) {
#ifdef OVERO
		struct obio_attach_args *obio = aux;
		int en;

		switch (obio->obio_addr) {
		case 0x49032000:	/* GPTIMER2 */
		case 0x49034000:	/* GPTIMER3 */
		case 0x49036000:	/* GPTIMER4 */
		case 0x49038000:	/* GPTIMER5 */
		case 0x4903a000:	/* GPTIMER6 */
		case 0x4903c000:	/* GPTIMER7 */
		case 0x4903e000:	/* GPTIMER8 */
		case 0x49040000:	/* GPTIMER9 */
			/* Ensure enable PRCM.CM_[FI]CLKEN_PER[3:10]. */
			en = 1 << (((obio->obio_addr >> 13) & 0x3f) - 0x16);
			ioreg_write(OVERO_L4_CORE_VBASE + 0x5000,
			    ioreg_read(OVERO_L4_CORE_VBASE + 0x5000) | en);
			ioreg_write(OVERO_L4_CORE_VBASE + 0x5010,
			    ioreg_read(OVERO_L4_CORE_VBASE + 0x5010) | en);
			break;
		}
#endif
	}
}
