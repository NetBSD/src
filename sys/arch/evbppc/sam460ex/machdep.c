/*	$NetBSD: machdep.c,v 1.4 2026/06/18 21:23:01 rkujawa Exp $	*/

/*
 * Copyright (c) 2012, 2014, 2024, 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
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
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Machine-dependent bootstrap for the ACube Sam460ex (AMCC 460EX).
 *
 * Modeled on evbppc/walnut/machdep.c and evbppc/obs405/obs600_machdep.c.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.4 2026/06/18 21:23:01 rkujawa Exp $");

#include "opt_ddb.h"
#include "opt_sam460ex.h"

#include <sys/param.h>
#include <sys/boot_flag.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/reboot.h>
#include <sys/sysctl.h>
#include <sys/systm.h>

#include <dev/cons.h>

#include <prop/proplib.h>

#include <machine/sam460ex.h>

#include <powerpc/spr.h>
#include <powerpc/ibm4xx/spr.h>

#include <powerpc/ibm4xx/amcc460ex.h>
#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/dcr4xx.h>
#include <powerpc/ibm4xx/tlb.h>

#include <powerpc/ibm4xx/pci_machdep.h>
#include <dev/pci/pciconf.h>
#include <dev/pci/pcivar.h>

#include "com.h"
#include "ukbd.h"
#if NUKBD > 0
#include <dev/usb/ukbdvar.h>
#endif
#if (NCOM > 0)
#include <sys/termios.h>
#include <powerpc/ibm4xx/dev/comopbvar.h>
#include <dev/ic/comreg.h>

#ifndef CONADDR
#define CONADDR		AMCC460EX_UART0_BASE
#endif
#ifndef CONSPEED
#define CONSPEED	B115200
#endif
#ifndef CONMODE
			/* 8N1 */
#define CONMODE		((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8)
#endif
#endif	/* NCOM */

#ifndef SAM460EX_MEMSIZE
#define SAM460EX_MEMSIZE	(512 * 1024 * 1024)
#endif
#ifndef SAM460EX_CPU_FREQ
#define SAM460EX_CPU_FREQ	(1150 * 1000 * 1000)
#endif
#ifndef SAM460EX_OPB_FREQ
#define SAM460EX_OPB_FREQ	(115 * 1000 * 1000)
#endif

#define	TLB_PG_SIZE 	(16 * 1024 * 1024)

/* Boot loader handoff, for later FDT parsing */
paddr_t sam460ex_fdt_pa;
uint32_t sam460ex_epapr_magic;

void initppc(vaddr_t, vaddr_t, paddr_t, uint32_t);

/*
 * Polled early console on UART0.
 * VA 0xef600300 covered by the locore.
 */
static void
earlycons_putc(dev_t dev, int c)
{
	volatile uint8_t *uart = (volatile uint8_t *)AMCC460EX_UART0_BASE;

	while ((uart[5] & 0x20) == 0)	/* LSR.THRE */
		;
	uart[0] = c;
}

static int
earlycons_getc(dev_t dev)
{
	volatile uint8_t *uart = (volatile uint8_t *)AMCC460EX_UART0_BASE;

	while ((uart[5] & 0x01) == 0)	/* LSR.DR */
		;
	return uart[0];
}

static struct consdev earlycons = {
	.cn_putc = earlycons_putc,
	.cn_getc = earlycons_getc,
	.cn_pollc = nullcnpollc,
	.cn_dev = NODEV,
	.cn_pri = CN_INTERNAL,
};

void
initppc(vaddr_t startkernel, vaddr_t endkernel, paddr_t fdt_pa,
    uint32_t magic)
{
	u_int memsize;
	vaddr_t va;

	cn_tab = &earlycons;

	sam460ex_fdt_pa = fdt_pa;
	sam460ex_epapr_magic = magic;

	/* Disable all external interrupts */
	mtdcr(DCR_UIC0_BASE + DCR_UIC_ER, 0);
	mtdcr(DCR_UIC1_BASE + DCR_UIC_ER, 0);
	mtdcr(DCR_UIC2_BASE + DCR_UIC_ER, 0);
	mtdcr(DCR_UIC3_BASE + DCR_UIC_ER, 0);

	memsize = SAM460EX_MEMSIZE;
#ifdef SAM460EX_FDT
	if (sam460ex_fdt_parse(fdt_pa)) {
		if (sam460ex_fdt_info.fi_memsize != 0)
			memsize = sam460ex_fdt_info.fi_memsize;
	} else
		printf("sam460ex: no valid FDT at %#lx, using defaults\n",
		    (u_long)fdt_pa);
	/* Record the value actually used so cpu_startup() agrees. */
	sam460ex_fdt_info.fi_memsize = memsize;
#endif

	/* Slots 0/1 hold the TS=0 identity entries pinned by locore */
	ppc44x_tlb_boot_reserved(2);

	/*
	 * locore TS=0 RAM identity entry covers the first 256MB
	 */
	for (paddr_t pa = 0x10000000; pa < memsize; pa += 0x10000000)
		ppc44x_tlb_reserve_ts0(pa);

	/* Linear map kernel memory (TS=1, KERNEL_PID) */
	for (va = 0; va < endkernel; va += TLB_PG_SIZE)
		ppc4xx_tlb_reserve(va, va, TLB_PG_SIZE, TLB_EX);

	/*
	 * Map the on-chip peripherals
	 */
	ppc44x_tlb_reserve((uint64_t)AMCC460EX_OPB_PA_HIGH << 32 | 0xef000000,
	    0xef000000, TLB_PG_SIZE, TLB_I | TLB_G);

	/*
	 * PCIX host bridge windows
	 */
	for (va = 0; va < AMCC460EX_PCIX0_MEM_SIZE; va += TLB_PG_SIZE)
		ppc44x_tlb_reserve(
		    (uint64_t)AMCC460EX_PCIX0_MEM_PLBA_H << 32 |
		      (AMCC460EX_PCIX0_MEM_BASE + va),
		    SAM460EX_PCIMEM_VA + va, TLB_PG_SIZE, TLB_I | TLB_G);
	/* Prefetchable window (POM1): pin the radeonfb framebuffer aperture */
	for (va = 0; va < AMCC460EX_PCIX0_PMEM_MAP; va += TLB_PG_SIZE)
		ppc44x_tlb_reserve(
		    (uint64_t)AMCC460EX_PCIX0_PMEM_PLBA_H << 32 |
		      (AMCC460EX_PCIX0_PMEM_BASE + va),
		    SAM460EX_PCIPREFMEM_VA + va, TLB_PG_SIZE, TLB_I | TLB_G);
	ppc44x_tlb_reserve((uint64_t)AMCC460EX_PCIX0_IO_PA_HIGH << 32 |
	    AMCC460EX_PCIX0_IO_PLBA,
	    SAM460EX_PCIIO_VA, TLB_PG_SIZE, TLB_I | TLB_G);
	ppc44x_tlb_reserve((uint64_t)AMCC460EX_PCIX0_CFG_PA_HIGH << 32 |
	    (AMCC460EX_PCIX0_CFG_PLBA & ~(TLB_PG_SIZE - 1)),
	    SAM460EX_PCICFG_VA, TLB_PG_SIZE, TLB_I | TLB_G);

	/*
	 * PCIe root complex windows
	 */
	ppc44x_tlb_reserve((uint64_t)AMCC460EX_PCIE_CFG_PA_HIGH << 32 |
	    AMCC460EX_PCIE0_CFG_PLBA,
	    SAM460EX_PCIE0CFG_VA, TLB_PG_SIZE, TLB_I | TLB_G);
	ppc44x_tlb_reserve((uint64_t)AMCC460EX_PCIE_CFG_PA_HIGH << 32 |
	    AMCC460EX_PCIE1_CFG_PLBA,
	    SAM460EX_PCIE1CFG_VA, TLB_PG_SIZE, TLB_I | TLB_G);
	ppc44x_tlb_reserve((uint64_t)AMCC460EX_PCIE_MEM_PA_HIGH << 32 |
	    AMCC460EX_PCIE0_MEM_PLBA,
	    SAM460EX_PCIE0MEM_VA, TLB_PG_SIZE, TLB_I | TLB_G);
	ppc44x_tlb_reserve((uint64_t)AMCC460EX_PCIE_MEM_PA_HIGH << 32 |
	    AMCC460EX_PCIE1_MEM_PLBA,
	    SAM460EX_PCIE1MEM_VA, TLB_PG_SIZE, TLB_I | TLB_G);

	/*
	 * AHB peripherals (USB OTG/OHCI/EHCI) behind the PLB-AHB
	 * bridge.
	 */
	ppc44x_tlb_reserve((uint64_t)AMCC460EX_AHB_PA_HIGH << 32 |
	    AMCC460EX_AHB_BASE,
	    SAM460EX_AHB_VA, TLB_PG_SIZE, TLB_I | TLB_G);

	mtspr(SPR_TCR, 0);	/* disable all timers */

	ibm40x_memsize_init(memsize, startkernel);
	ibm4xx_init(startkernel, endkernel, pic_ext_intr);

#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

/*
 * UART input clock for the console and com devices.
 */
uint32_t
sam460ex_com_freq(void)
{

#ifdef SAM460EX_FDT
	if (sam460ex_fdt_info.fi_uart_freq != 0)
		return sam460ex_fdt_info.fi_uart_freq;
#endif
	return AMCC460EX_COM_FREQ;
}

void
consinit(void)
{

#if (NCOM > 0)
	com_opb_cnattach(sam460ex_com_freq(), CONADDR, CONSPEED, CONMODE);
#endif
}

/*
 * Parse /chosen/bootargs.
 */
static char bootspec_buf[64];

/* Console target from the "console=" bootarg (see machine/sam460ex.h). */
enum sam460ex_console sam460ex_console = SAM460EX_CONS_COM;
int sam460ex_console_pci_bdf[3] = { -1, -1, -1 };

/*
 * Parse an optional ":bus:dev:func" suffix for "console=pci"
 */
static void
parse_pci_bdf(const char *s)
{
	char *ep;
	int i;

	for (i = 0; i < 3 && *s == ':'; i++) {
		sam460ex_console_pci_bdf[i] = (int)strtoul(s + 1, &ep, 0);
		if (ep == s + 1) {		/* no digits consumed */
			sam460ex_console_pci_bdf[i] = -1;
			return;
		}
		s = ep;
	}
}

static void
parse_bootargs(const char *args)
{
	const char *cp = args;

#define	BA_DELIM(c)	((c) == ' ' || (c) == '"')
	while (*cp != '\0') {
		if (BA_DELIM(*cp)) {
			cp++;
			continue;
		}
		if (*cp == '-') {
			for (cp++; *cp != '\0' && !BA_DELIM(*cp); cp++)
				BOOT_FLAG(*cp, boothowto);
		} else if (strncmp(cp, "root=", 5) == 0) {
			char *bp = bootspec_buf;

			for (cp += 5; *cp != '\0' && !BA_DELIM(*cp) &&
			    bp < &bootspec_buf[sizeof(bootspec_buf) - 1]; )
				*bp++ = *cp++;
			*bp = '\0';
			if (bootspec_buf[0] != '\0') {
				bootspec = bootspec_buf;
				booted_method = "bootargs/root";
			}
		} else if (strncmp(cp, "console=", 8) == 0) {
			char cbuf[24];
			char *bp = cbuf;

			for (cp += 8; *cp != '\0' && !BA_DELIM(*cp) &&
			    bp < &cbuf[sizeof(cbuf) - 1]; )
				*bp++ = *cp++;
			*bp = '\0';

			if (strcmp(cbuf, "com0") == 0 ||
			    strcmp(cbuf, "serial") == 0)
				sam460ex_console = SAM460EX_CONS_COM;
			else if (strcmp(cbuf, "sm502") == 0 ||
			    strcmp(cbuf, "fb") == 0)
				sam460ex_console = SAM460EX_CONS_SM502;
			else if (strncmp(cbuf, "pci", 3) == 0) {
				sam460ex_console = SAM460EX_CONS_PCI;
				parse_pci_bdf(&cbuf[3]);
			}
		} else {
			while (*cp != '\0' && !BA_DELIM(*cp))
				cp++;
		}
	}
#undef BA_DELIM
}

/*
 * Sanitize EHCI state before the kernel takes over the USB host controller. 
 */
static void
sam460ex_usb_host_init(void)
{
	volatile uint32_t *gpio;
	uint32_t v, srst;
	const uint32_t pin = 0x00008000;	/* GPIO16 = bit 15 in OR/TCR */

	/* AHB-to-PLB bridge config: 460EX errata for concurrent USB/SATA. */
	v = mfsdr(DCR_SDR0_AHB_CFG);
	v |= SDR0_AHB_CFG_A2P_INCR4;
	v &= ~SDR0_AHB_CFG_A2P_PROT2;
	mtsdr(DCR_SDR0_AHB_CFG, v);

	/* USB 2.0 host wrapper config (Sam460ex value). */
	v = mfsdr(DCR_SDR0_USB2HOST_CFG);
	v &= ~0x0000ff00;
	v |= 0x00004400;
	mtsdr(DCR_SDR0_USB2HOST_CFG, v);

	gpio = ppc4xx_tlb_mapiodev(AMCC460EX_GPIO0_BASE, 0x40);

	/*
	 * Reset and re-sync the USB 2.0 host and its external ULPI PHY 
	 */
	srst = mfsdr(DCR_SDR0_SRST1);
	mtsdr(DCR_SDR0_SRST1, srst | SDR0_SRST1_USBHOST);

	if (gpio != NULL) {
		/* GPIO16 -> GPIO mode, output low */
		gpio[0x04 / 4] &= ~pin;			/* TCR: tristate */
		gpio[0x0c / 4] &= ~0xc0000000;		/* OSRH: select GPIO */
		gpio[0x00 / 4] &= ~pin;			/* OR: drive low */
		gpio[0x04 / 4] |= pin;			/* TCR: drive output */
	}

	delay(500 * 1000);

	if (gpio != NULL) {
		/* GPIO16 -> ALT1 (USB2HostStop), output high */
		gpio[0x04 / 4] &= ~pin;
		gpio[0x0c / 4] = (gpio[0x0c / 4] & ~0xc0000000) | 0x40000000;
		gpio[0x00 / 4] |= pin;
		gpio[0x04 / 4] |= pin;
	}

	mtsdr(DCR_SDR0_SRST1, srst & ~SDR0_SRST1_USBHOST);
	delay(200 * 1000);
}

/*
 * Machine dependent startup code.
 */
void
cpu_startup(void)
{
	prop_number_t pn;
	uint32_t cpu_freq = SAM460EX_CPU_FREQ;
	uint32_t opb_freq = SAM460EX_OPB_FREQ;
	uint32_t memsize = SAM460EX_MEMSIZE;

	ibm4xx_cpu_startup("ACube Sam460ex (AMCC 460EX)");

	/* re-init the on-chip USB host so it doesn't depend on firmware state */
	sam460ex_usb_host_init();

#ifdef SAM460EX_FDT
	/*
	 * The timebase runs at the CPU clock; "processor-frequency"
	 * feeds delay() and the DEC reload value.
	 */
	if (sam460ex_fdt_info.fi_timebase_freq != 0)
		cpu_freq = sam460ex_fdt_info.fi_timebase_freq;
	else if (sam460ex_fdt_info.fi_cpu_freq != 0)
		cpu_freq = sam460ex_fdt_info.fi_cpu_freq;
	if (sam460ex_fdt_info.fi_opb_freq != 0)
		opb_freq = sam460ex_fdt_info.fi_opb_freq;
	if (sam460ex_fdt_info.fi_memsize != 0)
		memsize = sam460ex_fdt_info.fi_memsize;
	printf("sam460ex: fdt cpu %u Hz, opb %u Hz, uart %u Hz, mem %u MB\n",
	    cpu_freq, opb_freq, sam460ex_fdt_info.fi_uart_freq,
	    memsize / (1024 * 1024));
	if (sam460ex_fdt_info.fi_bootargs != NULL) {
		printf("bootargs: %s\n", sam460ex_fdt_info.fi_bootargs);
		parse_bootargs(sam460ex_fdt_info.fi_bootargs);
	}
#endif

#if NUKBD > 0
	/* Glass consoles (SM502 or a PCI display) take keyboard input via USB. */
	if (sam460ex_console == SAM460EX_CONS_SM502 ||
	    sam460ex_console == SAM460EX_CONS_PCI) {
		ukbd_cnattach();
	}
#endif

	/*
	 * Set up the board props
	 */
	board_info_init();

	pn = prop_number_create_integer(cpu_freq);
	KASSERT(pn != NULL);
	if (prop_dictionary_set(board_properties, "processor-frequency",
	    pn) == false)
		panic("setting processor-frequency");
	prop_object_release(pn);

	pn = prop_number_create_integer(opb_freq);
	KASSERT(pn != NULL);
	if (prop_dictionary_set(board_properties, "opb-frequency", pn) ==
	    false)
		panic("setting opb-frequency");
	prop_object_release(pn);

	pn = prop_number_create_integer(memsize);
	KASSERT(pn != NULL);
	if (prop_dictionary_set(board_properties, "mem-size", pn) == false)
		panic("setting mem-size");
	prop_object_release(pn);

#ifdef SAM460EX_FDT
	/*
	 * EMAC MAC addresses from the device tree filled by U-Boot
	 */
	for (int i = 0; i < SAM460EX_NEMAC; i++) {
		char propname[16];
		prop_data_t pd;

		if (!sam460ex_fdt_info.fi_enaddr_valid[i])
			continue;
		snprintf(propname, sizeof(propname), "emac%d-mac-addr", i);
		pd = prop_data_create_data_nocopy(
		    sam460ex_fdt_info.fi_enaddr[i], 6);
		KASSERT(pd != NULL);
		if (prop_dictionary_set(board_properties, propname, pd) ==
		    false)
			panic("setting %s", propname);
		prop_object_release(pd);

		snprintf(propname, sizeof(propname), "emac%d-mii-phy", i);
		pn = prop_number_create_integer(i);
		KASSERT(pn != NULL);
		if (prop_dictionary_set(board_properties, propname, pn) ==
		    false)
			panic("setting %s", propname);
		prop_object_release(pn);
	}
#endif

	calc_delayconst();

	/*
	 * Now that we have VM, malloc is OK
	 */
	bus_space_mallocok();
	fake_mapiodev = 0;
}

/*
 * PCI interrupt routing and slot policy for the on-chip PLB-PCIX
 * bridge (see powerpc/ibm4xx/pci/pcix.c).
 *
 * XXX: On the Sam460ex every PCI intr pin is wired to UIC1 bit 0?
 */
int
ibm4xx_pci_bus_maxdevs(void *v, int busno)
{
	return 16;
}

/*
 * Board interrupt routing for the PCI-X slot. Unlike the AMCC Canyonlands
 * design (where the SoC's external IRQ2 / UIC1 bit 0 is the PCI INT), the
 * Sam460ex wire-ORs all PCI INTx through its FPGA onto UIC1 bit 3 = irq 35
 */
#define	SAM460EX_PCI_INTR_IRQ	32	/* TEMP, see above; correct value is 35? */

int
ibm4xx_pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	*ihp = SAM460EX_PCI_INTR_IRQ;
	return 0;
}

void
ibm4xx_pci_conf_interrupt(void *v, int bus, int dev, int pin, int swiz,
    int *iline)
{
	*iline = SAM460EX_PCI_INTR_IRQ;
}

#define	UIC_DUMP(n, base)						\
	blen += snprintf(buf + blen, sizeof(buf) - blen,		\
	    "uic%d SR=%08x MSR=%08x ER=%08x PR=%08x TR=%08x\n", (n),	\
	    (unsigned int)mfdcr((base) + DCR_UIC_SR),			\
	    (unsigned int)mfdcr((base) + DCR_UIC_MSR),			\
	    (unsigned int)mfdcr((base) + DCR_UIC_ER),			\
	    (unsigned int)mfdcr((base) + DCR_UIC_PR),			\
	    (unsigned int)mfdcr((base) + DCR_UIC_TR))

static int
sysctl_machdep_uicregs(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	char buf[400];
	int blen = 0;

	UIC_DUMP(0, DCR_UIC0_BASE);
	UIC_DUMP(1, DCR_UIC1_BASE);
	UIC_DUMP(2, DCR_UIC2_BASE);
	UIC_DUMP(3, DCR_UIC3_BASE);

	node = *rnode;
	node.sysctl_data = buf;
	node.sysctl_size = strlen(buf) + 1;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}
#undef UIC_DUMP

SYSCTL_SETUP(sysctl_machdep_uicregs_setup, "sam460ex UIC register dump")
{

	sysctl_createv(clog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT | CTLFLAG_READONLY,
	    CTLTYPE_STRING, "uicregs",
	    SYSCTL_DESCR("UIC0-3 SR/MSR/ER/PR/TR (interrupt debug)"),
	    sysctl_machdep_uicregs, 0, NULL, 0,
	    CTL_MACHDEP, CTL_CREATE, CTL_EOL);
}

