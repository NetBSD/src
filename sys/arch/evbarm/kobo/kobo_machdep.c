/*	$NetBSD: kobo_machdep.c,v 1.2.6.3 2017/12/03 11:36:05 jdolecek Exp $	*/

/*
 * Copyright (c) 2002, 2003, 2005, 2010  Genetec Corporation.
 * All rights reserved.
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
 * Machine dependent functions for kernel setup for RAKUTEN Kobo.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kobo_machdep.c,v 1.2.6.3 2017/12/03 11:36:05 jdolecek Exp $");

#include "opt_evbarm_boardtype.h"
#include "opt_arm_debug.h"
#include "opt_cputypes.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_md.h"
#include "opt_com.h"
#include "imxuart.h"
#include "opt_imxuart.h"
#include "opt_imx.h"
#include "opt_machdep.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <sys/termios.h>
#include <sys/bus.h>

#include <machine/db_machdep.h>
#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include <machine/bootconfig.h>

#include <arm/arm32/machdep.h>

#include <arm/imx/imx51reg.h>
#include <arm/imx/imx51var.h>
#include <arm/imx/imxuartreg.h>
#include <arm/imx/imxuartvar.h>
#include <arm/imx/imx50_iomuxreg.h>
#include <arm/imx/imxgpiovar.h>

#include <evbarm/kobo/kobo.h>
#include <evbarm/kobo/kobo_reg.h>

/* Kernel text starts 1MB in from the bottom of the kernel address space. */
#define	KERNEL_TEXT_BASE	(KERNEL_BASE + 0x00100000)

BootConfig bootconfig;		/* Boot config storage */
static char bootargs[MAX_BOOT_STRING];
char *boot_args = NULL;

extern char KERNEL_BASE_phys[];

/*
 * Macros to translate between physical and virtual for a subset of the
 * kernel address space.  *Not* for general use.
 */
#define KERNEL_BASE_PHYS ((paddr_t)KERNEL_BASE_phys)

void consinit(void);

#ifdef KGDB
void	kgdb_port_init(void);
#endif

static void init_clocks(void);
static void setup_ioports(void);

#ifndef CONSPEED
#define CONSPEED B115200	/* What RedBoot uses */
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

int comcnspeed = CONSPEED;
int comcnmode = CONMODE;

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

#define _A(a)   ((a) & ~L1_S_OFFSET)
#define _S(s)   (((s) + L1_S_SIZE - 1) & ~(L1_S_SIZE-1))

const struct pmap_devmap kobo_devmap[] = {
	{
		/* for UART2, IOMUXC */
		.pd_va = _A(KOBO_IO_VBASE0),
		.pd_pa = _A(KOBO_IO_PBASE0),
		.pd_size = _S(L1_S_SIZE * 4),
		.pd_prot = VM_PROT_READ|VM_PROT_WRITE,
		.pd_cache = PTE_NOCACHE
	},
	{0}
};

#undef	_A
#undef	_S

#ifndef MEMSTART
#define MEMSTART	0x70000000
#endif
#ifndef MEMSIZE
#define MEMSIZE		256
#endif

static void
init_clocks(void)
{
	cortex_pmc_ccnt_init();
}

struct iomux_setup {
	/* iomux registers are 32-bit wide, but upper 16 bits are not
	 * used. */
	uint16_t	reg;
	uint16_t	val;
};

#define	IOMUX_M(padname, mux)		\
	IOMUX_DATA(__CONCAT(IOMUXC_SW_MUX_CTL_PAD_,padname), mux)

#define	IOMUX_P(padname, pad)		\
	IOMUX_DATA(__CONCAT(IOMUXC_SW_PAD_CTL_PAD_,padname), pad)

#define	IOMUX_MP(padname, mux, pad)	\
	IOMUX_M(padname, mux), \
	IOMUX_P(padname, pad)

#define	IOMUX_DATA(offset, value)	\
	{				\
		.reg = (offset),	\
		.val = (value),		\
	}

const struct iomux_setup iomux_setup_data[] = {
#define	HYS	PAD_CTL_HYS
#define	ODE	PAD_CTL_ODE
#define	DSEHIGH	PAD_CTL_DSE_HIGH
#define	DSEMID	PAD_CTL_DSE_MID
#define	DSELOW	PAD_CTL_DSE_LOW
#define	DSEMAX	PAD_CTL_DSE_MAX
#define	SRE	PAD_CTL_SRE
#define	KEEPER	PAD_CTL_KEEPER
#define	PULL	PAD_CTL_PULL
#define	PU_22K	PAD_CTL_PUS_22K_PU
#define	PU_47K	PAD_CTL_PUS_47K_PU
#define	PU_100K	PAD_CTL_PUS_100K_PU
#define	PD_100K	PAD_CTL_PUS_100K_PD
#define	HVE	PAD_CTL_HVE	/* Low output voltage */

#define	ALT0	IOMUX_CONFIG_ALT0
#define	ALT1	IOMUX_CONFIG_ALT1
#define	ALT2	IOMUX_CONFIG_ALT2
#define	ALT3	IOMUX_CONFIG_ALT3
#define	ALT4	IOMUX_CONFIG_ALT4
#define	ALT5	IOMUX_CONFIG_ALT5
#define	ALT6	IOMUX_CONFIG_ALT6
#define	ALT7	IOMUX_CONFIG_ALT7
#define	SION	IOMUX_CONFIG_SION

	/* I2C1 */
	IOMUX_MP(I2C1_SCL, ALT0 | SION, PULL | PU_100K | HYS | ODE | DSEHIGH),
	IOMUX_MP(I2C1_SDA, ALT0 | SION, PULL | PU_100K | HYS | ODE | DSEHIGH),

	/* I2C2 */
	IOMUX_MP(I2C2_SCL, ALT0 | SION, PULL | PU_100K | HYS | ODE | DSEHIGH),
	IOMUX_MP(I2C2_SDA, ALT0 | SION, PULL | PU_100K | HYS | ODE | DSEHIGH),

	/* I2C3 */
	IOMUX_MP(I2C3_SCL, ALT0 | SION, PULL | PU_100K | HYS | ODE | DSEHIGH),
	IOMUX_MP(I2C3_SDA, ALT0 | SION, PULL | PU_100K | HYS | ODE | DSEHIGH),

	/* UART2 */
	IOMUX_MP(UART2_RXD, ALT0, HYS | PULL | DSEHIGH | SRE),
	IOMUX_MP(UART2_TXD, ALT0, HYS | PULL | DSEHIGH | SRE),

	/* SD1 */
	IOMUX_MP(SD1_CMD, ALT0, HVE | PU_22K | DSEMAX | SRE),
	IOMUX_MP(SD1_CLK, ALT0, HVE | PU_22K | DSEMAX | SRE),
	IOMUX_MP(SD1_D0, ALT0, HVE | PU_22K | DSEMAX | SRE),
	IOMUX_MP(SD1_D1, ALT0, HVE | PU_22K | DSEMAX | SRE),
	IOMUX_MP(SD1_D2, ALT0, HVE | PU_22K | DSEMAX | SRE),
	IOMUX_MP(SD1_D3, ALT0, HVE | PU_22K | DSEMAX | SRE),
//	IOMUX_MP(SD1_CD, ALT0, HVE | PU_22K | DSEMAX | SRE),

	/* SD2 */
	IOMUX_MP(SD2_CMD, ALT0, HVE | PU_22K | DSEMAX | SRE),
	IOMUX_MP(SD2_CLK, ALT0, HVE | PU_22K | DSEMAX | SRE),
	IOMUX_MP(SD2_D0, ALT0, HVE | PU_22K | DSEMAX | SRE),
	IOMUX_MP(SD2_D1, ALT0, HVE | PU_22K | DSEMAX | SRE),
	IOMUX_MP(SD2_D2, ALT0, HVE | PU_22K | DSEMAX | SRE),
	IOMUX_MP(SD2_D3, ALT0, HVE | PU_22K | DSEMAX | SRE),
	IOMUX_MP(SD2_CD, ALT0, HVE | PU_22K | DSEMAX | SRE),

	IOMUX_DATA(IOMUXC_ESDHC2_IPP_CARD_DET_SELECT_INPUT, INPUT_DAISY_1),

	/* SD3 */
	IOMUX_MP(SD3_CMD, ALT0, HVE | PU_22K | DSEMAX | SRE),
	IOMUX_MP(SD3_CLK, ALT0, HVE | PU_22K | DSEMAX | SRE),
	IOMUX_MP(SD3_D0, ALT0, HVE | PU_22K | DSEMAX | SRE),
	IOMUX_MP(SD3_D1, ALT0, HVE | PU_22K | DSEMAX | SRE),
	IOMUX_MP(SD3_D2, ALT0, HVE | PU_22K | DSEMAX | SRE),
	IOMUX_MP(SD3_D3, ALT0, HVE | PU_22K | DSEMAX | SRE),
//	IOMUX_MP(SD3_CD, ALT0, HVE | PU_22K | DSEMAX | SRE),

	/* OTG */
	IOMUX_M(PWM2, ALT2),
//	IOMUX_MP(PWM1, ALT2, HYS | KEEPER | DSEHIGH),

	/* EPDC */
	IOMUX_M(EPDC_D0, ALT0),
	IOMUX_M(EPDC_D1, ALT0),
	IOMUX_M(EPDC_D2, ALT0),
	IOMUX_M(EPDC_D3, ALT0),
	IOMUX_M(EPDC_D4, ALT0),
	IOMUX_M(EPDC_D5, ALT0),
	IOMUX_M(EPDC_D6, ALT0),
	IOMUX_M(EPDC_D7, ALT0),
	IOMUX_M(EPDC_GDCLK, ALT0),
	IOMUX_M(EPDC_GDSP, ALT0),
	IOMUX_M(EPDC_GDOE, ALT0),
	IOMUX_M(EPDC_GDRL, ALT0),
	IOMUX_M(EPDC_SDCLK, ALT0),
	IOMUX_M(EPDC_SDOE, ALT0),
	IOMUX_M(EPDC_SDLE, ALT0),
	IOMUX_M(EPDC_BDR0, ALT0),
	IOMUX_M(EPDC_BDR1, ALT0),
	IOMUX_M(EPDC_SDCE0, ALT0),

	IOMUX_M(EPDC_PWRSTAT, ALT1),	/* GPIO3[28] */
	IOMUX_M(EPDC_PWRCTRL0, ALT1),	/* GPIO3[29] */
	IOMUX_M(EPDC_VCOM0, ALT1),	/* GPIO4[21] */
	IOMUX_M(UART4_TXD, ALT1),	/* GPIO6[16] */
	IOMUX_M(UART4_RXD, ALT1),	/* GPIO6[17] */

//	IOMUX_M(PWM2, ALT1),		/* GPIO6[25] */

#undef	ODE
#undef	HYS
#undef	SRE
#undef	PULL
#undef	KEEPER
#undef	PU_22K
#undef	PU_47K
#undef	PU_100K
#undef	PD_100K
#undef	HVE
#undef	DSEMAX
#undef	DSEHIGH
#undef	DSEMID
#undef	DSELOW

#undef	ALT0
#undef	ALT1
#undef	ALT2
#undef	ALT3
#undef	ALT4
#undef	ALT5
#undef	ALT6
#undef	ALT7
#undef	SION
};

static void
setup_ioports(void)
{
	int i;
	const struct iomux_setup *p;

	for (i=0; i < __arraycount(iomux_setup_data); ++i) {
		p = iomux_setup_data + i;

		ioreg_write(KOBO_IOMUXC_VBASE + p->reg,
			    p->val);
	}
}

#ifdef	CONSDEVNAME
const char consdevname[] = CONSDEVNAME;

#ifndef	CONMODE
#define	CONMODE	((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
#ifndef	CONSPEED
#define	CONSPEED	115200
#endif

int consmode = CONMODE;
int consrate = CONSPEED;

#endif	/* CONSDEVNAME */

#ifndef	IMXUART_FREQ
#define	IMXUART_FREQ	24000000
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
 */
u_int
initarm(void *arg)
{
	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())		// starts PMC counter
		panic("cpu not recognized!");

	/* map some peripheral registers */
	pmap_devmap_bootstrap((vaddr_t)armreg_ttbr_read() & -L1_TABLE_SIZE,
	    kobo_devmap);

	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);

	/* Register devmap for devices we mapped in start */
	pmap_devmap_register(kobo_devmap);
	setup_ioports();

	consinit();

	init_clocks();

#ifdef KGDB
	kgdb_port_init();
#endif

	/* Talk to the user */
	printf("\nNetBSD/evbarm (" ___STRING(EVBARM_BOARDTYPE) ") booting ...\n");

#ifdef BOOT_ARGS
	char mi_bootargs[] = BOOT_ARGS;
	parse_mi_bootargs(mi_bootargs);
#endif
	bootargs[0] = '\0';

#ifdef VERBOSE_INIT_ARM
	printf("initarm: Configuring system");
	printf(", CLIDR=%010o CTR=%#x",
	    armreg_clidr_read(), armreg_ctr_read());
	printf("\n");
#endif

#ifdef VERBOSE_INIT_ARM
	printf("initarm: Configuring system ...\n");
#endif
	/*
	 * Set up the variables that define the availability of physical
	 * memory.
	 */

	/* Fake bootconfig structure for the benefit of pmap.c. */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = MEMSTART;
	bootconfig.dram[0].pages = (MEMSIZE * 1024 * 1024) / PAGE_SIZE;

	psize_t ram_size = bootconfig.dram[0].pages * PAGE_SIZE;

#ifdef __HAVE_MM_MD_DIRECT_MAPPED_PHYS
	const bool mapallmem_p = true;
	if (ram_size > (KERNEL_VM_BASE - KERNEL_BASE)) {
		printf("%s: dropping RAM size from %luMB to %uMB\n",
		    __func__, (unsigned long) (ram_size >> 20),
		    (KERNEL_VM_BASE - KERNEL_BASE) >> 20);
		ram_size = KERNEL_VM_BASE - KERNEL_BASE;
	}
	KASSERT(ram_size <= KERNEL_VM_BASE - KERNEL_BASE);
#else
	const bool mapallmem_p = false;
#endif

	arm32_bootmem_init(bootconfig.dram[0].address, ram_size,
	    KERNEL_BASE_PHYS);
	arm32_kernel_vm_init(KERNEL_VM_BASE, ARM_VECTORS_HIGH, 0,
	    kobo_devmap, mapallmem_p);

#ifdef BOOTHOWTO
	boothowto |= BOOTHOWTO;
#endif

	return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE, NULL, 0);
}

void
consinit(void)
{
	static int consinit_called = 0;

	if (consinit_called)
		return;

	consinit_called = 1;

#ifdef	CONSDEVNAME

#if NIMXUART > 0
	imxuart_set_frequency(IMXUART_FREQ, 2);
#endif

#if (NIMXUART > 0) && defined(IMXUARTCONSOLE)
	if (strcmp(consdevname, "imxuart") == 0) {
		paddr_t consaddr;
#ifdef	CONADDR
		consaddr = CONADDR;
#else
		consaddr = IMX51_UART2_BASE;
#endif
		imxuart_cnattach(&armv7_generic_bs_tag, consaddr, consrate, consmode);
		return;
	}
#endif

#endif

#if (NWSDISPLAY > 0) && defined(IMXEPDCCONSOLE)
#if NUKBD > 0
	ukbd_cnattach();
#endif
	{
		extern void kobo_cnattach(void);
		kobo_cnattach();
	}
#endif
}

#ifdef KGDB
#ifndef KGDB_DEVNAME
#define KGDB_DEVNAME "imxuart"
#endif
#ifndef KGDB_DEVMODE
#define KGDB_DEVMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

const char kgdb_devname[20] = KGDB_DEVNAME;
int kgdb_mode = KGDB_DEVMODE;
int kgdb_addr = KGDB_DEVADDR;
extern int kgdb_rate;	/* defined in kgdb_stub.c */

void
kgdb_port_init(void)
{
#if (NIMXUART > 0)
	if (strcmp(kgdb_devname, "imxuart") == 0) {
		imxuart_kgdb_attach(&armv7_generic_bs_tag, kgdb_addr,
		kgdb_rate, kgdb_mode);
	    return;
	}

#endif
}
#endif
