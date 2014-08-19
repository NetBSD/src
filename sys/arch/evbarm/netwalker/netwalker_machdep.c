/*	$NetBSD: netwalker_machdep.c,v 1.9.2.2 2014/08/20 00:02:55 tls Exp $	*/

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
 * Machine dependent functions for kernel setup for Sharp Netwalker.
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
__KERNEL_RCSID(0, "$NetBSD: netwalker_machdep.c,v 1.9.2.2 2014/08/20 00:02:55 tls Exp $");

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
#include "opt_imx51_ipuv3.h"
#include "wsdisplay.h"
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
#include <arm/imx/imxgpioreg.h>
#include <arm/imx/imxwdogreg.h>
#include <arm/imx/imxuartreg.h>
#include <arm/imx/imxuartvar.h>
#include <arm/imx/imx51_iomuxreg.h>

#include <evbarm/netwalker/netwalker_reg.h>
#include <evbarm/netwalker/netwalker.h>

#include "ukbd.h"
#if (NUKBD > 0)
#include <dev/usb/ukbdvar.h>
#endif

/* Kernel text starts 1MB in from the bottom of the kernel address space. */
#define	KERNEL_TEXT_BASE	(KERNEL_BASE + 0x00100000)

BootConfig bootconfig;		/* Boot config storage */
static char bootargs[MAX_BOOT_STRING];
char *boot_args = NULL;

extern char KERNEL_BASE_phys[];

extern int cpu_do_powersave;

/*
 * Macros to translate between physical and virtual for a subset of the
 * kernel address space.  *Not* for general use.
 */
#define KERNEL_BASE_PHYS ((paddr_t)KERNEL_BASE_phys)


/* Prototypes */

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
 * fixed virtual addresses very early in netwalker_start() so that we
 * can use them while booting the kernel, and stay at the same address
 * throughout whole kernel's life time.
 *
 * We use this table twice; once with bootstrap page table, and once
 * with kernel's page table which we build up in initarm().
 */

#define _A(a)   ((a) & ~L1_S_OFFSET)
#define _S(s)   (((s) + L1_S_SIZE - 1) & ~(L1_S_SIZE-1))

static const struct pmap_devmap netwalker_devmap[] = {
	{
		/* for UART1, IOMUXC */
		.pd_va = _A(NETWALKER_IO_VBASE0),
		.pd_pa = _A(NETWALKER_IO_PBASE0),
		.pd_size = _S(L1_S_SIZE * 4),
		.pd_prot = VM_PROT_READ|VM_PROT_WRITE,
		.pd_cache = PTE_NOCACHE
	},
	{0}
};

#undef	_A
#undef	_S

#ifndef MEMSTART
#define MEMSTART	0x90000000
#endif
#ifndef MEMSIZE
#define MEMSIZE		512
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
	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())
		panic("cpu not recognized!");

	/* map some peripheral registers */
	pmap_devmap_bootstrap((vaddr_t)armreg_ttbr_read() & -L1_TABLE_SIZE,
	    netwalker_devmap);

	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);

	/* Register devmap for devices we mapped in start */
	pmap_devmap_register(netwalker_devmap);
	setup_ioports();

	consinit();

#ifdef	NO_POWERSAVE
	cpu_do_powersave=0;
#endif

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

#if defined(VERBOSE_INIT_ARM) || 1
	printf("initarm: Configuring system");
	printf(", CLIDR=%010o CTR=%#x",
	    armreg_clidr_read(), armreg_ctr_read());
	printf("\n");
#endif
	/*
	 * Ok we have the following memory map
	 *
	 * Physical Address Range     Description
	 * -----------------------    ----------------------------------
	 *
	 * 0x90000000 - 0xAFFFFFFF    DDR SDRAM (512MByte)
	 *
	 * The initarm() has the responsibility for creating the kernel
	 * page tables.
	 * It must also set up various memory pointers that are used
	 * by pmap etc.
	 */

#ifdef VERBOSE_INIT_ARM
	printf("initarm: Configuring system ...\n");
#endif
	/* Fake bootconfig structure for the benefit of pmap.c */
	/* XXX must make the memory description h/w independent */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = MEMSTART;
	bootconfig.dram[0].pages = (MEMSIZE * 1024 * 1024) / PAGE_SIZE;

	psize_t ram_size = bootconfig.dram[0].pages * PAGE_SIZE;

#ifdef __HAVE_MM_MD_DIRECT_MAPPED_PHYS
	if (ram_size > KERNEL_VM_BASE - KERNEL_BASE) {
		printf("%s: dropping RAM size from %luMB to %uMB\n",
		    __func__, (unsigned long) (ram_size >> 20),
		    (KERNEL_VM_BASE - KERNEL_BASE) >> 20);
		ram_size = KERNEL_VM_BASE - KERNEL_BASE;
	}
#endif

	arm32_bootmem_init(bootconfig.dram[0].address, ram_size,
	    KERNEL_BASE_PHYS);

#ifdef __HAVE_MM_MD_DIRECT_MAPPED_PHYS
	const bool mapallmem_p = true;
	KASSERT(ram_size <= KERNEL_VM_BASE - KERNEL_BASE);
#else
	const bool mapallmem_p = false;
#endif

	arm32_kernel_vm_init(KERNEL_VM_BASE, ARM_VECTORS_HIGH, 0,
	    netwalker_devmap, mapallmem_p);

	/* disable power down counter in watch dog,
	   This must be done within 16 seconds of start-up. */
	ioreg16_write(NETWALKER_WDOG_VBASE + IMX_WDOG_WMCR, 0);

#ifdef BOOTHOWTO
	boothowto |= BOOTHOWTO;
#endif

#ifdef VERBOSE_INIT_ARM
	printf("initarm done.\n");
#endif
	return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE, NULL, 0);
}


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


/*
 * set same values to IOMUX registers as linux kernel does
 */
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

	/* left button */
	IOMUX_MP(EIM_EB2, ALT1, HYS),
	/* right button */
	IOMUX_MP(EIM_EB3, ALT1, HYS),

	/* UART1 */
	IOMUX_MP(UART1_RXD, ALT0, HYS | PULL | DSEHIGH | SRE),
	IOMUX_MP(UART1_TXD, ALT0, HYS | PULL | DSEHIGH | SRE),
	IOMUX_MP(UART1_RTS, ALT0, HYS | PULL | DSEHIGH),
	IOMUX_MP(UART1_CTS, ALT0, HYS | PULL | DSEHIGH),

	/* LCD Display */
	IOMUX_M(DI1_PIN2, ALT0),
	IOMUX_M(DI1_PIN3, ALT0),

	IOMUX_DATA(IOMUXC_SW_PAD_CTL_GRP_DISP1_PKE0, PAD_CTL_PKE),
#if 0
	IOMUX_MP(DISP1_DAT0, ALT0, SRE | DSEMAX | PULL),
	IOMUX_MP(DISP1_DAT1, ALT0, SRE | DSEMAX | PULL),
	IOMUX_MP(DISP1_DAT2, ALT0, SRE | DSEMAX | PULL),
	IOMUX_MP(DISP1_DAT3, ALT0, SRE | DSEMAX | PULL),
	IOMUX_MP(DISP1_DAT4, ALT0, SRE | DSEMAX | PULL),
	IOMUX_MP(DISP1_DAT5, ALT0, SRE | DSEMAX | PULL),
#endif
	IOMUX_M(DISP1_DAT6, ALT0),
	IOMUX_M(DISP1_DAT7, ALT0),
	IOMUX_M(DISP1_DAT8, ALT0),
	IOMUX_M(DISP1_DAT9, ALT0),
	IOMUX_M(DISP1_DAT10, ALT0),
	IOMUX_M(DISP1_DAT11, ALT0),
	IOMUX_M(DISP1_DAT12, ALT0),
	IOMUX_M(DISP1_DAT13, ALT0),
	IOMUX_M(DISP1_DAT14, ALT0),
	IOMUX_M(DISP1_DAT15, ALT0),
	IOMUX_M(DISP1_DAT16, ALT0),
	IOMUX_M(DISP1_DAT17, ALT0),
	IOMUX_M(DISP1_DAT18, ALT0),
	IOMUX_M(DISP1_DAT19, ALT0),
	IOMUX_M(DISP1_DAT20, ALT0),
	IOMUX_M(DISP1_DAT21, ALT0),
	IOMUX_M(DISP1_DAT22, ALT0),
	IOMUX_M(DISP1_DAT23, ALT0),

	IOMUX_MP(DI1_D0_CS, ALT4, KEEPER | DSEHIGH | SRE), /* GPIO3_3 */
	IOMUX_DATA(IOMUXC_GPIO3_IPP_IND_G_IN_3_SELECT_INPUT, INPUT_DAISY_0),
	IOMUX_MP(CSI2_D12, ALT3, KEEPER | DSEHIGH | SRE), /* GPIO4_9 */
	IOMUX_MP(CSI2_D13, ALT3, KEEPER | DSEHIGH | SRE),
#if 1
	IOMUX_MP(GPIO1_2, ALT1, DSEHIGH | ODE),	/* LCD backlight by PWM */
#else
	IOMUX_MP(GPIO1_2, ALT0, DSEHIGH | ODE),	/* LCD backlight by GPIO */
#endif
	IOMUX_MP(EIM_A19, ALT1, SRE | DSEHIGH),
	/* XXX VGA pins */
	IOMUX_M(DI_GP4, ALT4),
	IOMUX_M(GPIO1_8, SION | ALT0),

	IOMUX_MP(GPIO1_8, SION | ALT0, HYS | DSEMID | PU_100K),
	/* I2C1 */
	IOMUX_MP(EIM_D16, SION | ALT4, HYS | ODE | DSEHIGH | SRE),
	IOMUX_MP(EIM_D19, SION | ALT4, SRE),	/* SCL */
	IOMUX_MP(EIM_A19, ALT1, SRE | DSEHIGH), /* GPIO2_13 */

#if 0
	IOMUX_MP(EIM_A23, ALT1, 0),
#else
	IOMUX_M(EIM_A23, ALT1),	/* GPIO2_17 */
#endif

	/* BT */
	IOMUX_M(EIM_D20, ALT1),	/* GPIO2_4 BT host wakeup */
	IOMUX_M(EIM_D22, ALT1),	/* GPIO2_6 BT RESET */
	IOMUX_M(EIM_D23, ALT1),	/* GPIO2_7 BT wakeup */

	/* UART3 */
	IOMUX_MP(EIM_D24, ALT3, KEEPER | PU_100K | DSEHIGH | SRE),
	IOMUX_MP(EIM_D25, ALT3, KEEPER | PU_100K | DSEHIGH | SRE), /* CTS */
	IOMUX_MP(EIM_D26, ALT3, KEEPER | PU_100K | DSEHIGH | SRE), /* TXD */
	IOMUX_MP(EIM_D27, ALT3, KEEPER | PU_100K | DSEHIGH | SRE), /* RTS */
	IOMUX_M(NANDF_D15, ALT3),	/* GPIO3_25 */
	IOMUX_MP(NANDF_D14, ALT3, HYS | PULL | PU_100K ),	/* GPIO3_26 */

	/* OJ6SH-T25 */
	IOMUX_M(CSI1_D9, ALT3),			/* GPIO3_13 */
	IOMUX_M(CSI1_VSYNC, ALT3),		/* GPIO3_14 */
	IOMUX_M(CSI1_HSYNC, ALT3),		/* GPIO3_15 */

	/* audio pins */
	IOMUX_MP(AUD3_BB_TXD, ALT0, DSEHIGH | PU_100K | SRE),
		/* XXX: linux code:
		   (PAD_CTL_SRE_FAST	     | PAD_CTL_DRV_HIGH |
		   PAD_CTL_100K_PU	     | PAD_CTL_HYS_NONE |
		   PAD_CTL_DDR_INPUT_CMOS | PAD_CTL_DRV_VOT_LOW), */

	IOMUX_MP(AUD3_BB_RXD, ALT0, KEEPER | DSEHIGH | SRE),
	IOMUX_MP(AUD3_BB_CK, ALT0, KEEPER | DSEHIGH | SRE),
	IOMUX_MP(AUD3_BB_FS, ALT0, KEEPER | DSEHIGH | SRE),

	/* headphone detect */
	IOMUX_MP(NANDF_D14, ALT3, HYS | PULL | PU_100K),
	IOMUX_MP(CSPI1_RDY, ALT3, SRE | DSEHIGH),
	/* XXX more audio pins ? */

	/* CSPI */
	IOMUX_MP(CSPI1_MOSI, ALT0, HYS | PULL | PD_100K | DSEHIGH | SRE),
	IOMUX_MP(CSPI1_MISO, ALT0, HYS | PULL | PD_100K | DSEHIGH | SRE),
	IOMUX_MP(CSPI1_SCLK, ALT0, HYS | PULL | PD_100K | DSEHIGH | SRE),

	/* SPI CS */
	IOMUX_MP(CSPI1_SS0, ALT3, HYS | KEEPER | DSEHIGH | SRE), /* GPIO4[24] */
	IOMUX_MP(CSPI1_SS1, ALT3, HYS | KEEPER | DSEHIGH | SRE), /* GPIO4[25] */
	IOMUX_MP(DI1_PIN11, ALT4, HYS | PULL | DSEHIGH | SRE),   /* GPIO3[0] */

	/* 26M Osc */
	IOMUX_MP(DI1_PIN12, ALT4, KEEPER | DSEHIGH | SRE), /* GPIO3_1 */

	/* I2C */
	IOMUX_MP(KEY_COL4, SION | ALT3, SRE),
	IOMUX_DATA(IOMUXC_I2C2_IPP_SCL_IN_SELECT_INPUT, INPUT_DAISY_1),
	IOMUX_MP(KEY_COL5, SION | ALT3, HYS | ODE | DSEHIGH | SRE),
	IOMUX_DATA(IOMUXC_I2C2_IPP_SDA_IN_SELECT_INPUT, INPUT_DAISY_1),
	IOMUX_DATA(IOMUXC_UART3_IPP_UART_RTS_B_SELECT_INPUT, INPUT_DAISY_3),

	/* NAND */
	IOMUX_MP(NANDF_WE_B, ALT0, HVE | DSEHIGH | PULL | PU_47K),
	IOMUX_MP(NANDF_RE_B, ALT0, HVE | DSEHIGH | PULL | PU_47K),
	IOMUX_MP(NANDF_ALE, ALT0, HVE | DSEHIGH | KEEPER),
	IOMUX_MP(NANDF_CLE, ALT0, HVE | DSEHIGH | KEEPER),
	IOMUX_MP(NANDF_WP_B, ALT0, HVE | DSEHIGH | PULL | PU_100K),
	IOMUX_MP(NANDF_RB0, ALT0, HVE | DSELOW | PULL | PU_100K),
	IOMUX_MP(NANDF_RB1, ALT0, HVE | DSELOW | PULL | PU_100K),
	IOMUX_MP(NANDF_D7, ALT0, HVE | DSEHIGH | KEEPER | PU_100K),
	IOMUX_MP(NANDF_D6, ALT0, HVE | DSEHIGH | KEEPER | PU_100K),
	IOMUX_MP(NANDF_D5, ALT0, HVE | DSEHIGH | KEEPER | PU_100K),
	IOMUX_MP(NANDF_D4, ALT0, HVE | DSEHIGH | KEEPER | PU_100K),
	IOMUX_MP(NANDF_D3, ALT0, HVE | DSEHIGH | KEEPER | PU_100K),
	IOMUX_MP(NANDF_D2, ALT0, HVE | DSEHIGH | KEEPER | PU_100K),
	IOMUX_MP(NANDF_D1, ALT0, HVE | DSEHIGH | KEEPER | PU_100K),
	IOMUX_MP(NANDF_D0, ALT0, HVE | DSEHIGH | KEEPER | PU_100K),

	/* Batttery pins */
	IOMUX_MP(NANDF_D13, ALT3, HYS | DSEHIGH),
	IOMUX_MP(NANDF_D12, ALT3, HYS | DSEHIGH),
#if 0
	IOMUX_MP(NANDF_D11, ALT3, HYS | DSEHIGH),
#endif
	IOMUX_MP(NANDF_D10, ALT3, HYS | DSEHIGH),

	/* SD1 */
	IOMUX_MP(SD1_CMD, SION | ALT0, DSEHIGH | SRE),
	IOMUX_MP(SD1_CLK, SION | ALT0, KEEPER | PU_47K | DSEHIGH),
	IOMUX_MP(SD1_DATA0, ALT0, DSEHIGH | SRE),
	IOMUX_MP(SD1_DATA1, ALT0, DSEHIGH | SRE),
	IOMUX_MP(SD1_DATA2, ALT0, DSEHIGH | SRE),
	IOMUX_MP(SD1_DATA3, ALT0, DSEHIGH | SRE),
	IOMUX_MP(GPIO1_0, SION | ALT0, HYS | PU_100K),

	/* SD2 */
	IOMUX_P(SD2_CMD, HVE | PU_22K | DSEMAX | SRE),
	IOMUX_P(SD2_CLK, HVE | PU_22K | DSEMAX | SRE),
	IOMUX_P(SD2_DATA0, HVE | PU_22K | DSEMAX | SRE),
	IOMUX_P(SD2_DATA1, HVE | PU_22K | DSEMAX | SRE),
	IOMUX_P(SD2_DATA2, HVE | PU_22K | DSEMAX | SRE),
	IOMUX_P(SD2_DATA3, HVE | PU_22K | DSEMAX | SRE),

	/* USB */
	IOMUX_MP(USBH1_CLK, ALT0, HYS | KEEPER | DSEHIGH | SRE),
	IOMUX_MP(USBH1_DIR, ALT0, HYS | KEEPER | DSEHIGH | SRE),
	IOMUX_MP(USBH1_STP, ALT0, HYS | KEEPER | DSEHIGH | SRE),
	IOMUX_MP(USBH1_NXT, ALT0, HYS | KEEPER | PU_100K | DSEHIGH | SRE),
	IOMUX_MP(USBH1_DATA0, ALT0, HYS | KEEPER | DSEHIGH | SRE),
	IOMUX_MP(USBH1_DATA1, ALT0, HYS | KEEPER | DSEHIGH | SRE),
	IOMUX_MP(USBH1_DATA2, ALT0, HYS | KEEPER | DSEHIGH | SRE),
	IOMUX_MP(USBH1_DATA3, ALT0, HYS | KEEPER | DSEHIGH | SRE),
	IOMUX_MP(USBH1_DATA4, ALT0, HYS | KEEPER | DSEHIGH | SRE),
	IOMUX_MP(USBH1_DATA5, ALT0, HYS | KEEPER | DSEHIGH | SRE),
	IOMUX_MP(USBH1_DATA6, ALT0, HYS | KEEPER | DSEHIGH | SRE),
	IOMUX_MP(USBH1_DATA7, ALT0, HYS | KEEPER | DSEHIGH | SRE),
	IOMUX_MP(EIM_D17, ALT1, KEEPER | DSEHIGH | SRE),
	IOMUX_MP(EIM_D21, ALT1, KEEPER | DSEHIGH | SRE),
	IOMUX_P(GPIO1_7, /*ALT0,*/ DSEHIGH | SRE),	/* USB Hub reset */

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

	/* Initialize all IOMUX registers */
	for (i=0; i < __arraycount(iomux_setup_data); ++i) {
		p = iomux_setup_data + i;

		ioreg_write(NETWALKER_IOMUXC_VBASE + p->reg,
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
#define	IMXUART_FREQ	66500000
#endif

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
		consaddr = IMX51_UART1_BASE;
#endif
		imxuart_cons_attach(&imx_bs_tag, consaddr, consrate, consmode);
	    return;
	}
#endif

#endif

#if (NWSDISPLAY > 0) && defined(IMXIPUCONSOLE)
#if NUKBD > 0
	ukbd_cnattach();
#endif
	{
		extern void netwalker_cnattach(void);
		netwalker_cnattach();
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
		imxuart_kgdb_attach(&imx_bs_tag, kgdb_addr,
		kgdb_rate, kgdb_mode);
	    return;
	}

#endif
}
#endif


