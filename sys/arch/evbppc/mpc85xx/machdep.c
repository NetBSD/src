/*	$NetBSD: machdep.c,v 1.45 2022/03/03 06:28:04 riastradh Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.45 2022/03/03 06:28:04 riastradh Exp $");

#include "opt_altivec.h"
#include "opt_ddb.h"
#include "opt_mpc85xx.h"
#include "opt_multiprocessor.h"
#include "opt_pci.h"
#include "gpio.h"
#include "pci.h"

#define	DDRC_PRIVATE
#define	GLOBAL_PRIVATE
#define	L2CACHE_PRIVATE
#define _POWERPC_BUS_DMA_PRIVATE

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/intr.h>
#include <sys/msgbuf.h>
#include <sys/tty.h>
#include <sys/kcore.h>
#include <sys/bitops.h>
#include <sys/bus.h>
#include <sys/extent.h>
#include <sys/reboot.h>
#include <sys/module.h>

#include <uvm/uvm_extern.h>

#include <prop/proplib.h>

#include <dev/cons.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <net/if.h>
#include <net/if_media.h>
#include <dev/mii/miivar.h>

#include <powerpc/pcb.h>
#include <powerpc/spr.h>
#include <powerpc/booke/spr.h>

#include <powerpc/booke/cpuvar.h>
#include <powerpc/booke/e500reg.h>
#include <powerpc/booke/e500var.h>
#include <powerpc/booke/etsecreg.h>
#include <powerpc/booke/openpicreg.h>
#ifdef CADMUS
#include <evbppc/mpc85xx/cadmusreg.h>
#endif
#ifdef PIXIS
#include <evbppc/mpc85xx/pixisreg.h>
#endif

struct uboot_bdinfo {
	uint32_t bd_memstart;
	uint32_t bd_memsize;
	uint32_t bd_flashstart;
	uint32_t bd_flashsize;
/*10*/	uint32_t bd_flashoffset;
	uint32_t bd_sramstart;
	uint32_t bd_sramsize;
	uint32_t bd_immrbase;
/*20*/	uint32_t bd_bootflags;
	uint32_t bd_ipaddr;
	uint8_t bd_etheraddr[6];
	uint16_t bd_ethspeed;
/*30*/	uint32_t bd_intfreq;
	uint32_t bd_cpufreq;
	uint32_t bd_baudrate;
/*3c*/	uint8_t bd_etheraddr1[6];
/*42*/	uint8_t bd_etheraddr2[6];
/*48*/	uint8_t bd_etheraddr3[6];
/*4e*/	uint16_t bd_pad;
};

char root_string[16];

/*
 * booke kernels need to set module_machine to this for modules to work.
 */
char module_machine_booke[] = "powerpc-booke";

void	initppc(vaddr_t, vaddr_t, void *, void *, char *, char *);

#define	MEMREGIONS	4
phys_ram_seg_t physmemr[MEMREGIONS];		/* All memory */
phys_ram_seg_t availmemr[2*MEMREGIONS];		/* Available memory */
static u_int nmemr;

#ifndef CONSFREQ
# define CONSFREQ	-1            /* inherit from firmware */
#endif
#ifndef CONSPEED
# define CONSPEED	115200
#endif
#ifndef CONMODE
# define CONMODE	((TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8)
#endif
#ifndef CONSADDR
# define CONSADDR	DUART2_BASE
#endif

int		comcnfreq  = CONSFREQ;
int		comcnspeed = CONSPEED;
tcflag_t	comcnmode  = CONMODE;
bus_addr_t	comcnaddr  = (bus_addr_t)CONSADDR;

#if NPCI > 0
struct extent *pcimem_ex;
struct extent *pciio_ex;
#endif

struct powerpc_bus_space gur_bst = {
	.pbs_flags = _BUS_SPACE_BIG_ENDIAN|_BUS_SPACE_MEM_TYPE,
	.pbs_offset = GUR_BASE,
	.pbs_limit = GUR_SIZE,
};

struct powerpc_bus_space gur_le_bst = {
	.pbs_flags = _BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE,
	.pbs_offset = GUR_BASE,
	.pbs_limit = GUR_SIZE,
};

const bus_space_handle_t gur_bsh = (bus_space_handle_t)(uintptr_t)(GUR_BASE);

#if defined(SYS_CLK)
static uint64_t e500_sys_clk = SYS_CLK;
#endif
#ifdef CADMUS
static uint8_t cadmus_pci;
static uint8_t cadmus_csr;
#ifndef SYS_CLK
static uint64_t e500_sys_clk = 33333333; /* 33.333333Mhz */
#endif
#elif defined(PIXIS)
static const uint32_t pixis_spd_map[8] = {
    [PX_SPD_33MHZ] = 33333333,
    [PX_SPD_40MHZ] = 40000000,
    [PX_SPD_50MHZ] = 50000000,
    [PX_SPD_66MHZ] = 66666666,
    [PX_SPD_83MHZ] = 83333333,
    [PX_SPD_100MHZ] = 100000000,
    [PX_SPD_133MHZ] = 133333333,
    [PX_SPD_166MHZ] = 166666667,
};
static uint8_t pixis_spd;
#ifndef SYS_CLK
static uint64_t e500_sys_clk;
#endif
#elif !defined(SYS_CLK)
static uint64_t e500_sys_clk = 66666667; /* 66.666667Mhz */
#endif

static int e500_cngetc(dev_t);
static void e500_cnputc(dev_t, int);

static struct consdev e500_earlycons = {
	.cn_getc = e500_cngetc,
	.cn_putc = e500_cnputc,
	.cn_pollc = nullcnpollc,
};

/*
 * List of port-specific devices to attach to the processor local bus.
 */
static const struct cpunode_locators mpc8548_cpunode_locs[] = {
	{ "cpu", 0, 0, 0, 0, { 0 }, 0,	/* not a real device */
		{ 0xffff, SVR_MPC8572v1 >> 16, SVR_P2020v2 >> 16,
		  SVR_P1025v1 >> 16, SVR_P1023v1 >> 16 } },
#if defined(MPC8572) || defined(P2020) || defined(P1025) \
    || defined(P1023)
	{ "cpu", 0, 0, 1, 0, { 0 }, 0,	/* not a real device */
		{ SVR_MPC8572v1 >> 16, SVR_P2020v2 >> 16,
		  SVR_P1025v1 >> 16, SVR_P1023v1 >> 16 } },
	{ "cpu", 0, 0, 2, 0, { 0 }, 0,	/* not a real device */
		{ SVR_MPC8572v1 >> 16, SVR_P2020v2 >> 16,
		  SVR_P1025v1 >> 16, SVR_P1023v1 >> 16 } },
#endif
	{ "wdog" },	/* not a real device */
	{ "duart", DUART1_BASE, 2*DUART_SIZE, 0,
		1, { ISOURCE_DUART },
		1 + ilog2(DEVDISR_DUART) },
	{ "tsec", ETSEC1_BASE, ETSEC_SIZE, 1,
		3, { ISOURCE_ETSEC1_TX, ISOURCE_ETSEC1_RX, ISOURCE_ETSEC1_ERR },
		1 + ilog2(DEVDISR_TSEC1),
		{ 0xffff, SVR_P1025v1 >> 16, SVR_P1023v1 >> 16 } },
#if defined(P1025)
	{ "mdio", ETSEC1_BASE, ETSEC_SIZE, 1,
		0, { },
		1 + ilog2(DEVDISR_TSEC1),
		{ SVR_P1025v1 >> 16 } },
	{ "tsec", ETSEC1_G0_BASE, ETSEC_SIZE, 1,
		3, { ISOURCE_ETSEC1_TX, ISOURCE_ETSEC1_RX, ISOURCE_ETSEC1_ERR },
		1 + ilog2(DEVDISR_TSEC1),
		{ SVR_P1025v1 >> 16 } },
#if 0
	{ "tsec", ETSEC1_G1_BASE, ETSEC_SIZE, 1,
		3, { ISOURCE_ETSEC1_G1_TX, ISOURCE_ETSEC1_G1_RX,
		     ISOURCE_ETSEC1_G1_ERR },
		1 + ilog2(DEVDISR_TSEC1),
		{ SVR_P1025v1 >> 16 } },
#endif
#endif
#if defined(MPC8548) || defined(MPC8555) || defined(MPC8572) \
    || defined(P2020)
	{ "tsec", ETSEC2_BASE, ETSEC_SIZE, 2,
		3, { ISOURCE_ETSEC2_TX, ISOURCE_ETSEC2_RX, ISOURCE_ETSEC2_ERR },
		1 + ilog2(DEVDISR_TSEC2),
		{ SVR_MPC8548v1 >> 16, SVR_MPC8555v1 >> 16,
		  SVR_MPC8572v1 >> 16, SVR_P2020v2 >> 16,
		  SVR_P1025v1 >> 16 } },
#endif
#if defined(P1025)
	{ "mdio", ETSEC2_BASE, ETSEC_SIZE, 2,
		0, { },
		1 + ilog2(DEVDISR_TSEC2),
		{ SVR_P1025v1 >> 16 } },
	{ "tsec", ETSEC2_G0_BASE, ETSEC_SIZE, 2,
		3, { ISOURCE_ETSEC2_TX, ISOURCE_ETSEC2_RX, ISOURCE_ETSEC2_ERR },
		1 + ilog2(DEVDISR_TSEC2),
		{ SVR_P1025v1 >> 16 } },
#if 0
	{ "tsec", ETSEC2_G1_BASE, ETSEC_SIZE, 5,
		3, { ISOURCE_ETSEC2_G1_TX, ISOURCE_ETSEC2_G1_RX,
		     ISOURCE_ETSEC2_G1_ERR },
		1 + ilog2(DEVDISR_TSEC2),
		{ SVR_P1025v1 >> 16 } },
#endif
#endif
#if defined(MPC8544) || defined(MPC8536)
	{ "tsec", ETSEC3_BASE, ETSEC_SIZE, 2,
		3, { ISOURCE_ETSEC3_TX, ISOURCE_ETSEC3_RX, ISOURCE_ETSEC3_ERR },
		1 + ilog2(DEVDISR_TSEC3),
		{ SVR_MPC8536v1 >> 16, SVR_MPC8544v1 >> 16 } },
#endif
#if defined(MPC8548) || defined(MPC8572) || defined(P2020)
	{ "tsec", ETSEC3_BASE, ETSEC_SIZE, 3,
		3, { ISOURCE_ETSEC3_TX, ISOURCE_ETSEC3_RX, ISOURCE_ETSEC3_ERR },
		1 + ilog2(DEVDISR_TSEC3),
		{ SVR_MPC8548v1 >> 16, SVR_MPC8572v1 >> 16,
		  SVR_P2020v2 >> 16 } },
#endif
#if defined(P1025)
	{ "mdio", ETSEC3_BASE, ETSEC_SIZE, 3,
		0, { },
		1 + ilog2(DEVDISR_TSEC3),
		{ SVR_P1025v1 >> 16 } },
	{ "tsec", ETSEC3_G0_BASE, ETSEC_SIZE, 3,
		3, { ISOURCE_ETSEC3_TX, ISOURCE_ETSEC3_RX, ISOURCE_ETSEC3_ERR },
		1 + ilog2(DEVDISR_TSEC3),
		{ SVR_P1025v1 >> 16 } },
#if 0
	{ "tsec", ETSEC3_G1_BASE, ETSEC_SIZE, 3,
		3, { ISOURCE_ETSEC3_G1_TX, ISOURCE_ETSEC3_G1_RX,
		     ISOURCE_ETSEC3_G1_ERR },
		1 + ilog2(DEVDISR_TSEC3),
		{ SVR_P1025v1 >> 16 } },
#endif
#endif
#if defined(MPC8548) || defined(MPC8572)
	{ "tsec", ETSEC4_BASE, ETSEC_SIZE, 4,
		3, { ISOURCE_ETSEC4_TX, ISOURCE_ETSEC4_RX, ISOURCE_ETSEC4_ERR },
		1 + ilog2(DEVDISR_TSEC4),
		{ SVR_MPC8548v1 >> 16, SVR_MPC8572v1 >> 16 } },
#endif
	{ "diic", I2C1_BASE, 2*I2C_SIZE, 0,
		1, { ISOURCE_I2C },
		1 + ilog2(DEVDISR_I2C) },
	/* MPC8572 doesn't have any GPIO */
	{ "gpio", GLOBAL_BASE, GLOBAL_SIZE, 0, 
		1, { ISOURCE_GPIO },
		0, 
		{ 0xffff, SVR_MPC8572v1 >> 16 } },
	{ "ddrc", DDRC1_BASE, DDRC_SIZE, 0,
		1, { ISOURCE_DDR },
		1 + ilog2(DEVDISR_DDR_15),
		{ 0xffff, SVR_MPC8572v1 >> 16, SVR_MPC8536v1 >> 16 } },
#if defined(MPC8536)
	{ "ddrc", DDRC1_BASE, DDRC_SIZE, 0,
		1, { ISOURCE_DDR },
		1 + ilog2(DEVDISR_DDR_16),
		{ SVR_MPC8536v1 >> 16 } },
#endif
#if defined(MPC8572)
	{ "ddrc", DDRC1_BASE, DDRC_SIZE, 1,
		1, { ISOURCE_DDR },
		1 + ilog2(DEVDISR_DDR_15),
		{ SVR_MPC8572v1 >> 16 } },
	{ "ddrc", DDRC2_BASE, DDRC_SIZE, 2,
		1, { ISOURCE_DDR },
		1 + ilog2(DEVDISR_DDR2_14),
		{ SVR_MPC8572v1 >> 16 } },
#endif
	{ "lbc", LBC_BASE, LBC_SIZE, 0,
		1, { ISOURCE_LBC },
		1 + ilog2(DEVDISR_LBC) },
#if defined(MPC8544) || defined(MPC8536)
	{ "pcie", PCIE1_BASE, PCI_SIZE, 1,
		1, { ISOURCE_PCIEX },
		1 + ilog2(DEVDISR_PCIE),
		{ SVR_MPC8536v1 >> 16, SVR_MPC8544v1 >> 16 } },
	{ "pcie", PCIE2_MPC8544_BASE, PCI_SIZE, 2,
		1, { ISOURCE_PCIEX2 },
		1 + ilog2(DEVDISR_PCIE2),
		{ SVR_MPC8536v1 >> 16, SVR_MPC8544v1 >> 16 } },
	{ "pcie", PCIE3_MPC8544_BASE, PCI_SIZE, 3,
		1, { ISOURCE_PCIEX3 },
		1 + ilog2(DEVDISR_PCIE3),
		{ SVR_MPC8536v1 >> 16, SVR_MPC8544v1 >> 16 } },
	{ "pci", PCIX1_MPC8544_BASE, PCI_SIZE, 0,
		1, { ISOURCE_PCI1 },
		1 + ilog2(DEVDISR_PCI1),
		{ SVR_MPC8536v1 >> 16, SVR_MPC8544v1 >> 16 } },
#endif
#ifdef MPC8548
	{ "pcie", PCIE1_BASE, PCI_SIZE, 0,
		1, { ISOURCE_PCIEX },
		1 + ilog2(DEVDISR_PCIE),
		{ SVR_MPC8548v1 >> 16 }, },
	{ "pci", PCIX1_MPC8548_BASE, PCI_SIZE, 1,
		1, { ISOURCE_PCI1 },
		1 + ilog2(DEVDISR_PCI1),
		{ SVR_MPC8548v1 >> 16 }, },
	{ "pci", PCIX2_MPC8548_BASE, PCI_SIZE, 2,
		1, { ISOURCE_PCI2 },
		1 + ilog2(DEVDISR_PCI2),
		{ SVR_MPC8548v1 >> 16 }, },
#endif
#if defined(MPC8572) || defined(P1025) || defined(P2020) \
    || defined(P1023)
	{ "pcie", PCIE1_BASE, PCI_SIZE, 1,
		1, { ISOURCE_PCIEX },
		1 + ilog2(DEVDISR_PCIE),
		{ SVR_MPC8572v1 >> 16, SVR_P2020v2 >> 16,
		  SVR_P1025v1 >> 16, SVR_P1023v1 >> 16 } },
	{ "pcie", PCIE2_MPC8572_BASE, PCI_SIZE, 2,
		1, { ISOURCE_PCIEX2 },
		1 + ilog2(DEVDISR_PCIE2),
		{ SVR_MPC8572v1 >> 16, SVR_P2020v2 >> 16,
		  SVR_P1025v1 >> 16, SVR_P1023v1 >> 16 } },
#endif
#if defined(MPC8572) || defined(P2020) || defined(_P1023)
	{ "pcie", PCIE3_MPC8572_BASE, PCI_SIZE, 3,
		1, { ISOURCE_PCIEX3_MPC8572 },
		1 + ilog2(DEVDISR_PCIE3),
		{ SVR_MPC8572v1 >> 16, SVR_P2020v2 >> 16,
		  SVR_P1023v1 >> 16 } },
#endif
#if defined(MPC8536) || defined(P1025) || defined(P2020) \
    || defined(P1023)
	{ "ehci", USB1_BASE, USB_SIZE, 1,
		1, { ISOURCE_USB1 },
		1 + ilog2(DEVDISR_USB1),
		{ SVR_MPC8536v1 >> 16, SVR_P2020v2 >> 16,
		  SVR_P1025v1 >> 16, SVR_P1023v1 >> 16 } },
#endif
#ifdef MPC8536
	{ "ehci", USB2_BASE, USB_SIZE, 2,
		1, { ISOURCE_USB2 },
		1 + ilog2(DEVDISR_USB2),
		{ SVR_MPC8536v1 >> 16 }, },
	{ "ehci", USB3_BASE, USB_SIZE, 3,
		1, { ISOURCE_USB3 },
		1 + ilog2(DEVDISR_USB3),
		{ SVR_MPC8536v1 >> 16 }, },
	{ "sata", SATA1_BASE, SATA_SIZE, 1,
		1, { ISOURCE_SATA1 },
		1 + ilog2(DEVDISR_SATA1),
		{ SVR_MPC8536v1 >> 16 }, },
	{ "sata", SATA2_BASE, SATA_SIZE, 2,
		1, { ISOURCE_SATA2 },
		1 + ilog2(DEVDISR_SATA2),
		{ SVR_MPC8536v1 >> 16 }, },
	{ "spi", SPI_BASE, SPI_SIZE, 0,
		1, { ISOURCE_SPI },
		1 + ilog2(DEVDISR_SPI_15),
		{ SVR_MPC8536v1 >> 16 }, },
	{ "sdhc", ESDHC_BASE, ESDHC_SIZE, 0,
		1, { ISOURCE_ESDHC },
		1 + ilog2(DEVDISR_ESDHC_12),
		{ SVR_MPC8536v1 >> 16 }, },
#endif
#if defined(P1025) || defined(P2020) || defined(P1023)
	{ "spi", SPI_BASE, SPI_SIZE, 0,
		1, { ISOURCE_SPI },
		1 + ilog2(DEVDISR_SPI_28),
		{ SVR_P2020v2 >> 16, SVR_P1025v1 >> 16,
		  SVR_P1023v1 >> 16 }, },
#endif
#if defined(P1025) || defined(P2020)
	{ "sdhc", ESDHC_BASE, ESDHC_SIZE, 0,
		1, { ISOURCE_ESDHC },
		1 + ilog2(DEVDISR_ESDHC_10),
		{ SVR_P2020v2 >> 16, SVR_P1025v1 >> 16 }, },
#endif
	//{ "sec", RNG_BASE, RNG_SIZE, 0, 0, },
	{ NULL }
};

static int
e500_cngetc(dev_t dv)
{
	volatile uint8_t * const com0addr = (void *)(GUR_BASE+CONSADDR);

        if ((com0addr[com_lsr] & LSR_RXRDY) == 0)
		return -1;

	return com0addr[com_data] & 0xff;
}

static void
e500_cnputc(dev_t dv, int c)
{               
	volatile uint8_t * const com0addr = (void *)(GUR_BASE+CONSADDR);
	int timo = 150000;
		
	while ((com0addr[com_lsr] & LSR_TXRDY) == 0 && --timo > 0)
		;

	com0addr[com_data] = c;
	__asm("mbar");
			
	while ((com0addr[com_lsr] & LSR_TSRE) == 0 && --timo > 0)
		;
}

static void *
gur_tlb_mapiodev(paddr_t pa, psize_t len, bool prefetchable)
{
	if (prefetchable)
		return NULL;
	if (pa < gur_bst.pbs_offset)
		return NULL;
	if (pa + len > gur_bst.pbs_offset + gur_bst.pbs_limit)
		return NULL;
	return (void *)pa;
}

static void *(* const early_tlb_mapiodev)(paddr_t, psize_t, bool) = gur_tlb_mapiodev;

static void
e500_cpu_reset(void)
{
	__asm volatile("sync");
	cpu_write_4(GLOBAL_BASE + RSTCR, HRESET_REQ);
	__asm volatile("msync;isync");
}

static psize_t
memprobe(vaddr_t endkernel)
{
	phys_ram_seg_t *mr;
	paddr_t boot_page = cpu_read_4(GUR_BPTR);
	printf(" bptr=%"PRIxPADDR, boot_page);
	if (boot_page & BPTR_EN) {
		/*
		 * shift it to an address
		 */
		boot_page = (boot_page & BPTR_BOOT_PAGE) << PAGE_SHIFT;
	} else {
		boot_page = ~(paddr_t)0;
	}

	/*
	 * First we need to find out how much physical memory we have.
	 * We could let our bootloader tell us, but it's almost as easy
	 * to ask the DDR memory controller.
	 */
	mr = physmemr;
	for (u_int i = 0; i < 4; i++) {
		uint32_t v = cpu_read_4(DDRC1_BASE + CS_CONFIG(i));
		if (v & CS_CONFIG_EN) {
			v = cpu_read_4(DDRC1_BASE + CS_BNDS(i));
			if (v == 0)
				continue;
			mr->start = BNDS_SA_GET(v);
			mr->size  = BNDS_SIZE_GET(v);
#ifdef MEMSIZE
			if (mr->start >= MEMSIZE)
				continue;
			if (mr->start + mr->size > MEMSIZE)
				mr->size = MEMSIZE - mr->start;
#endif
#if 0
			printf(" [%zd]={%#"PRIx64"@%#"PRIx64"}",
			    mr - physmemr, mr->size, mr->start);
#endif
			mr++;
		}
	}

	if (mr == physmemr)
		panic("no memory configured!");

	/*
	 * Sort memory regions from low to high and coalesce adjacent regions
	 */
	u_int cnt = mr - physmemr;
	if (cnt > 1) {
		for (u_int i = 0; i < cnt - 1; i++) {
			for (u_int j = i + 1; j < cnt; j++) {
				if (physmemr[j].start < physmemr[i].start) {
					phys_ram_seg_t tmp = physmemr[i];
					physmemr[i] = physmemr[j];
					physmemr[j] = tmp;
				}
			}
		}
		mr = physmemr;
		for (u_int i = 0; i + 1 < cnt; i++, mr++) {
			if (mr->start + mr->size == mr[1].start) {
				mr->size += mr[1].size;
				for (u_int j = 1; i + j + 1 < cnt; j++)
					mr[j] = mr[j+1];
				cnt--;
			}
		}
	} else if (cnt == 0) {
		panic("%s: no memory found", __func__);
	}

	/*
	 * Copy physical memory to available memory.
	 */
	memcpy(availmemr, physmemr, cnt * sizeof(physmemr[0]));

	/*
	 * Adjust available memory to skip kernel at start of memory.
	 */
	availmemr[0].size -= endkernel - availmemr[0].start;
	availmemr[0].start = endkernel;

	mr = availmemr;
	for (u_int i = 0; i < cnt; i++, mr++) {
		/*
		 * U-boot reserves a boot-page on multi-core chips.
		 * We need to make sure that we never disturb it.
		 */
		const paddr_t mr_end = mr->start + mr->size;
		if (mr_end > boot_page && boot_page >= mr->start) {
			/*
			 * Normally u-boot will put in at the end
			 * of memory.  But in case it doesn't, deal
			 * with all possibilities.
			 */
			if (boot_page + PAGE_SIZE == mr_end) {
				mr->size -= PAGE_SIZE;
			} else if (boot_page == mr->start) {
				mr->start += PAGE_SIZE;
				mr->size -= PAGE_SIZE;
			} else {
				mr->size = boot_page - mr->start;
				mr++;
				for (u_int j = cnt; j > i + 1; j--) {
					availmemr[j] = availmemr[j-1];
				}
				cnt++;
				mr->start = boot_page + PAGE_SIZE;
				mr->size = mr_end - mr->start;
			}
			break;
		}
	}

	/*
	 * Steal pages at the end of memory for the kernel message buffer.
	 */
	mr = availmemr + cnt - 1;
	KASSERT(mr->size >= round_page(MSGBUFSIZE));
	mr->size -= round_page(MSGBUFSIZE);
	msgbuf_paddr = (uintptr_t)(mr->start + mr->size);

	/*
	 * Calculate physmem.
	 */
	for (u_int i = 0; i < cnt; i++)
		physmem += atop(physmemr[i].size);

	nmemr = cnt;
	return physmemr[cnt-1].start + physmemr[cnt-1].size;
}

void
consinit(void)
{
	static bool attached = false;

	if (attached)
		return;
	attached = true;

	if (comcnfreq == -1) {
		const uint32_t porpplsr = cpu_read_4(GLOBAL_BASE + PORPLLSR);
		const uint32_t plat_ratio = PLAT_RATIO_GET(porpplsr);
		comcnfreq = e500_sys_clk * plat_ratio;
		printf(" comcnfreq=%u", comcnfreq);
	}

	comcnattach(&gur_bst, comcnaddr, comcnspeed, comcnfreq,
	    COM_TYPE_NORMAL, comcnmode);
}

void
cpu_probe_cache(void)
{
	struct cpu_info * const ci = curcpu();
	const uint32_t l1cfg0 = mfspr(SPR_L1CFG0);
	const int dcache_assoc = L1CFG_CNWAY_GET(l1cfg0);

	ci->ci_ci.dcache_size = L1CFG_CSIZE_GET(l1cfg0);
	ci->ci_ci.dcache_line_size = 32 << L1CFG_CBSIZE_GET(l1cfg0);

	if (L1CFG_CARCH_GET(l1cfg0) == L1CFG_CARCH_HARVARD) {
		const uint32_t l1cfg1 = mfspr(SPR_L1CFG1);

		ci->ci_ci.icache_size = L1CFG_CSIZE_GET(l1cfg1);
		ci->ci_ci.icache_line_size = 32 << L1CFG_CBSIZE_GET(l1cfg1);
	} else {
		ci->ci_ci.icache_size = ci->ci_ci.dcache_size;
		ci->ci_ci.icache_line_size = ci->ci_ci.dcache_line_size;
	}

	/*
	 * Possibly recolor.
	 */
	uvm_page_recolor(atop(curcpu()->ci_ci.dcache_size / dcache_assoc));

#ifdef DEBUG
	uint32_t l1csr0 = mfspr(SPR_L1CSR0);
	if ((L1CSR_CE & l1csr0) == 0)
		printf(" DC=off");

	uint32_t l1csr1 = mfspr(SPR_L1CSR1);
	if ((L1CSR_CE & l1csr1) == 0)
		printf(" IC=off");
#endif
}

static uint16_t
getsvr(void)
{
	uint16_t svr = mfspr(SPR_SVR) >> 16;

	svr &= ~0x8;		/* clear security bit */
	switch (svr) {
	case SVR_MPC8543v1 >> 16:	return SVR_MPC8548v1 >> 16;
	case SVR_MPC8541v1 >> 16:	return SVR_MPC8555v1 >> 16;
	case SVR_P2010v2 >> 16:		return SVR_P2020v2 >> 16;
	case SVR_P1016v1 >> 16:		return SVR_P1025v1 >> 16;
	case SVR_P1017v1 >> 16:		return SVR_P1023v1 >> 16;
	default:			return svr;
	}
}

static const char *
socname(uint32_t svr)
{
	svr &= ~0x80000;	/* clear security bit */
	switch (svr >> 8) {
	case SVR_MPC8533 >> 8: return "MPC8533";
	case SVR_MPC8536v1 >> 8: return "MPC8536";
	case SVR_MPC8541v1 >> 8: return "MPC8541";
	case SVR_MPC8543v2 >> 8: return "MPC8543";
	case SVR_MPC8544v1 >> 8: return "MPC8544";
	case SVR_MPC8545v2 >> 8: return "MPC8545";
	case SVR_MPC8547v2 >> 8: return "MPC8547";
	case SVR_MPC8548v2 >> 8: return "MPC8548";
	case SVR_MPC8555v1 >> 8: return "MPC8555";
	case SVR_MPC8568v1 >> 8: return "MPC8568";
	case SVR_MPC8567v1 >> 8: return "MPC8567";
	case SVR_MPC8572v1 >> 8: return "MPC8572";
	case SVR_P2020v2 >> 8: return "P2020";
	case SVR_P2010v2 >> 8: return "P2010";
	case SVR_P1016v1 >> 8: return "P1016";
	case SVR_P1017v1 >> 8: return "P1017";
	case SVR_P1023v1 >> 8: return "P1023";
	case SVR_P1025v1 >> 8: return "P1025";
	default:
		panic("%s: unknown SVR %#x", __func__, svr);
	}
}

static void
e500_tlb_print(device_t self, const char *name, uint32_t tlbcfg)
{
	static const char units[16] = "KKKKKMMMMMGGGGGT";

	const uint32_t minsize = 1U << (2 * TLBCFG_MINSIZE(tlbcfg));
	const uint32_t assoc = TLBCFG_ASSOC(tlbcfg);
	const u_int maxsize_log4k = TLBCFG_MAXSIZE(tlbcfg);
	const uint64_t maxsize = 1ULL << (2 * maxsize_log4k % 10);
	const uint32_t nentries = TLBCFG_NENTRY(tlbcfg);

	aprint_normal_dev(self, "%s:", name);

	aprint_normal(" %u", nentries);
	if (TLBCFG_AVAIL_P(tlbcfg)) {
		aprint_normal(" variable-size (%uKB..%"PRIu64"%cB)",
		    minsize, maxsize, units[maxsize_log4k]);
	} else {
		aprint_normal(" fixed-size (%uKB)", minsize);
	}
	if (assoc == 0 || assoc == nentries)
		aprint_normal(" fully");
	else
		aprint_normal(" %u-way set", assoc);
	aprint_normal(" associative entries\n");
}

static void
cpu_print_info(struct cpu_info *ci)
{
	uint64_t freq = board_info_get_number("processor-frequency");
	device_t self = ci->ci_dev;

	char freqbuf[10];
	if (freq >= 999500000) {
		const uint32_t freq32 = (freq + 500000) / 10000000;
		snprintf(freqbuf, sizeof(freqbuf), "%u.%02u GHz",
		    freq32 / 100, freq32 % 100);
	} else {
		const uint32_t freq32 = (freq + 500000) / 1000000;
		snprintf(freqbuf, sizeof(freqbuf), "%u MHz", freq32);
	}

	const uint32_t pvr = mfpvr();
	const uint32_t svr = mfspr(SPR_SVR);
	const uint32_t pir = mfspr(SPR_PIR);

	aprint_normal_dev(self, "%s %s%s %u.%u with an e500%s %u.%u core, "
	   "ID %u%s\n",
	   freqbuf, socname(svr), (SVR_SECURITY_P(svr) ? "E" : ""),
	   (svr >> 4) & 15, svr & 15,
	   (pvr >> 16) == PVR_MPCe500v2 ? "v2" : "",
	   (pvr >> 4) & 15, pvr & 15,
	   pir, (pir == 0 ? " (Primary)" : ""));

	const uint32_t l1cfg0 = mfspr(SPR_L1CFG0);
	aprint_normal_dev(self,
	    "%uKB/%uB %u-way L1 %s cache\n",
	    L1CFG_CSIZE_GET(l1cfg0) >> 10,
	    32 << L1CFG_CBSIZE_GET(l1cfg0),
	    L1CFG_CNWAY_GET(l1cfg0),
	    L1CFG_CARCH_GET(l1cfg0) == L1CFG_CARCH_HARVARD
		? "data" : "unified");
	    
	if (L1CFG_CARCH_GET(l1cfg0) == L1CFG_CARCH_HARVARD) {
		const uint32_t l1cfg1 = mfspr(SPR_L1CFG1);
		aprint_normal_dev(self,
		    "%uKB/%uB %u-way L1 %s cache\n",
		    L1CFG_CSIZE_GET(l1cfg1) >> 10,
		    32 << L1CFG_CBSIZE_GET(l1cfg1),
		    L1CFG_CNWAY_GET(l1cfg1),
		    "instruction");
	}

	const uint32_t mmucfg = mfspr(SPR_MMUCFG);
	aprint_normal_dev(self,
	    "%u TLBs, %u concurrent %u-bit PIDs (%u total)\n",
	    MMUCFG_NTLBS_GET(mmucfg) + 1,
	    MMUCFG_NPIDS_GET(mmucfg),
	    MMUCFG_PIDSIZE_GET(mmucfg) + 1,
	    1 << (MMUCFG_PIDSIZE_GET(mmucfg) + 1));

	e500_tlb_print(self, "tlb0", mfspr(SPR_TLB0CFG));
	e500_tlb_print(self, "tlb1", mfspr(SPR_TLB1CFG));
}

#ifdef MULTIPROCESSOR
static void
e500_cpu_spinup(device_t self, struct cpu_info *ci)
{
	uintptr_t spinup_table_addr = board_info_get_number("mp-spin-up-table");
	struct pglist splist;

	if (spinup_table_addr == 0) {
		aprint_error_dev(self, "hatch failed (no spin-up table)");
		return;
	}

	struct uboot_spinup_entry * const e = (void *)spinup_table_addr;
	volatile struct cpu_hatch_data * const h = &cpu_hatch_data;
	const size_t id = cpu_index(ci);
	kcpuset_t * const hatchlings = cpuset_info.cpus_hatched;

	if (h->hatch_sp == 0) {
		int error = uvm_pglistalloc(PAGE_SIZE, PAGE_SIZE,
		    64*1024*1024, PAGE_SIZE, 0, &splist, 1, 1);
		if (error) {
			aprint_error_dev(self,
			    "unable to allocate hatch stack\n");
			return;
		}
		h->hatch_sp = VM_PAGE_TO_PHYS(TAILQ_FIRST(&splist))
		    + PAGE_SIZE - CALLFRAMELEN;
        }


	for (size_t i = 1; e[i].entry_pir != 0; i++) {
		printf("%s: cpu%u: entry#%zu(%p): pir=%u\n",
		    __func__, ci->ci_cpuid, i, &e[i], e[i].entry_pir);
		if (e[i].entry_pir == ci->ci_cpuid) {

			ci->ci_curlwp = ci->ci_data.cpu_idlelwp;
			ci->ci_curpcb = lwp_getpcb(ci->ci_curlwp);
			ci->ci_curpm = pmap_kernel();
			ci->ci_lasttb = cpu_info[0].ci_lasttb;
			ci->ci_data.cpu_cc_freq =
			    cpu_info[0].ci_data.cpu_cc_freq;

			h->hatch_self = self;
			h->hatch_ci = ci;
			h->hatch_running = -1;
			h->hatch_pir = e[i].entry_pir;
			h->hatch_hid0 = mfspr(SPR_HID0);
			u_int tlbidx;
		        e500_tlb_lookup_xtlb(0, &tlbidx);
		        h->hatch_tlbidx = tlbidx;
			KASSERT(h->hatch_sp != 0);
			/*
			 * Get new timebase.  We don't want to deal with
			 * timebase crossing a 32-bit boundary so make sure
			 * that we have enough headroom to do the timebase
			 * synchronization. 
			 */
#define	TBSYNC_SLOP	2000
			uint32_t tbl;
			uint32_t tbu;
			do {
				tbu = mfspr(SPR_RTBU);
				tbl = mfspr(SPR_RTBL) + TBSYNC_SLOP;
			} while (tbl < TBSYNC_SLOP);
			
			h->hatch_tbu = tbu;
			h->hatch_tbl = tbl;
			__asm("sync;isync");
			dcache_wbinv((vaddr_t)h, sizeof(*h));

			/*
			 * And here we go...
			 */
			e[i].entry_addr_lower =
			    (uint32_t)e500_spinup_trampoline;
			dcache_wbinv((vaddr_t)&e[i], sizeof(e[i]));
			__asm __volatile("sync;isync");
			__insn_barrier();

			for (u_int timo = 0; timo++ < 10000; ) {
				dcache_inv((vaddr_t)&e[i], sizeof(e[i]));
				if (e[i].entry_addr_lower == 3) {
#if 0
					printf(
					    "%s: cpu%u started in %u spins\n",
					    __func__, cpu_index(ci), timo);
#endif
					break;
				}
			}
			for (u_int timo = 0; timo++ < 10000; ) {
				dcache_inv((vaddr_t)h, sizeof(*h));
				if (h->hatch_running == 0) {
#if 0
					printf(
					    "%s: cpu%u cracked in %u spins: (running=%d)\n",
					    __func__, cpu_index(ci),
					    timo, h->hatch_running);
#endif
					break;
				}
			}
			if (h->hatch_running == -1) {
				aprint_error_dev(self,
				    "hatch failed (timeout): running=%d"
				    ", entry=%#x\n",
				    h->hatch_running, e[i].entry_addr_lower);
				goto out;
			}

			/*
			 * First then we do is to synchronize timebases.
			 * TBSYNC_SLOP*16 should be more than enough
			 * instructions.
			 */
			while (tbl != mftbl())
				continue;
			h->hatch_running = 1;
			dcache_wbinv((vaddr_t)h, sizeof(*h));
			__asm("sync;isync");
			__insn_barrier();
			printf("%s: cpu%u set to running\n",
			    __func__, cpu_index(ci));

			for (u_int timo = 10000; timo-- > 0; ) {
				dcache_inv((vaddr_t)h, sizeof(*h));
				if (h->hatch_running > 1)
					break;
			}
			if (h->hatch_running == 1) {
				printf(
				    "%s: tb sync failed: offset from %"PRId64"=%"PRId64" (running=%d)\n",
				    __func__,
				    ((int64_t)tbu << 32) + tbl,
				    (int64_t)
					(((uint64_t)h->hatch_tbu << 32)
					+ (uint64_t)h->hatch_tbl),
				    h->hatch_running);
				goto out;
			}
			printf(
			    "%s: tb synced: offset=%"PRId64" (running=%d)\n",
			    __func__,
			    (int64_t)
				(((uint64_t)h->hatch_tbu << 32)
				+ (uint64_t)h->hatch_tbl),
			    h->hatch_running);
			/*
			 * Now we wait for the hatching to complete.  30ms
			 * should be long enough.
			 */
			for (u_int timo = 30000; timo-- > 0; ) {
				if (kcpuset_isset(hatchlings, id)) {
					aprint_normal_dev(self,
					    "hatch successful (%u spins, "
					    "timebase adjusted by %"PRId64")\n",
					    30000 - timo,
					    (int64_t)
						(((uint64_t)h->hatch_tbu << 32)
						+ (uint64_t)h->hatch_tbl));
					goto out;
				}
				DELAY(1);
			}

			aprint_error_dev(self,
			    "hatch failed (timeout): running=%u\n",
			    h->hatch_running);
			goto out;
		}
	}

	aprint_error_dev(self, "hatch failed (no spin-up entry for PIR %u)",
	    ci->ci_cpuid);
out:
	if (h->hatch_sp == 0)
		uvm_pglistfree(&splist);
}
#endif

void
e500_cpu_hatch(struct cpu_info *ci)
{
	mtmsr(mfmsr() | PSL_CE | PSL_ME | PSL_DE);

	/*
	 * Make sure interrupts are blocked.
	 */
	cpu_write_4(OPENPIC_BASE + OPENPIC_CTPR, 15);	/* IPL_HIGH */

	/* Set the MAS4 defaults */
	mtspr(SPR_MAS4, MAS4_TSIZED_4KB | MAS4_MD);
	tlb_invalidate_all();

	intr_cpu_hatch(ci);

	cpu_probe_cache();
	cpu_print_info(ci);

/*
 */
}

static void
e500_cpu_attach(device_t self, u_int instance)
{
	struct cpu_info * const ci = &cpu_info[instance - (instance > 0)];

	if (instance > 1) {
#if defined(MULTIPROCESSOR)
		ci->ci_idepth = -1;
		device_set_private(self, ci);

		ci->ci_cpuid = instance - (instance > 0);
		ci->ci_dev = self;
		ci->ci_tlb_info = cpu_info[0].ci_tlb_info;

		mi_cpu_attach(ci);

		intr_cpu_attach(ci);
		cpu_evcnt_attach(ci);

		e500_cpu_spinup(self, ci);
		return;
#else
		aprint_error_dev(self, "disabled (uniprocessor kernel)\n");
		return;
#endif
	}

	device_set_private(self, ci);

	ci->ci_cpuid = instance - (instance > 0);
	ci->ci_dev = self;

	intr_cpu_attach(ci);
	cpu_evcnt_attach(ci);

	KASSERT(ci == curcpu());
	intr_cpu_hatch(ci);

	cpu_print_info(ci);
}

void
e500_ipi_halt(void)
{
#ifdef MULTIPROCESSOR
	struct cpuset_info * const csi = &cpuset_info;
	const cpuid_t index = cpu_index(curcpu());

	printf("cpu%lu: shutting down\n", index);
	kcpuset_set(csi->cpus_halted, index);
#endif
	register_t msr, hid0;

	msr = wrtee(0);

	hid0 = mfspr(SPR_HID0);
	hid0 = (hid0 & ~(HID0_TBEN|HID0_NAP|HID0_SLEEP)) | HID0_DOZE;
	mtspr(SPR_HID0, hid0);

	msr = (msr & ~(PSL_EE|PSL_CE|PSL_ME)) | PSL_WE;
	mtmsr(msr);
	for (;;);	/* loop forever */
}


static void
calltozero(void)
{
	panic("call to 0 from %p", __builtin_return_address(0));
}

#if !defined(ROUTERBOOT)
static void
parse_cmdline(char *cp)
{
	int ourhowto = 0;
	char c;
	bool opt = false;
	for (; (c = *cp) != '\0'; cp++) {
		if (c == '-') {	
			opt = true;
			continue;
		}
		if (c == ' ') {
			opt = false;
			continue;
		}
		if (opt) {
			switch (c) {
			case 'a': ourhowto |= RB_ASKNAME; break;
			case 'd': ourhowto |= AB_DEBUG; break;
			case 'q': ourhowto |= AB_QUIET; break;
			case 's': ourhowto |= RB_SINGLE; break;
			case 'v': ourhowto |= AB_VERBOSE; break;
			}
			continue;
		}
		strlcpy(root_string, cp, sizeof(root_string));
		break;
	}
	if (ourhowto) {
		boothowto |= ourhowto;
		printf(" boothowto=%#x(%#x)", boothowto, ourhowto);
	}
	if (root_string[0])
		printf(" root=%s", root_string);
}
#endif	/* !ROUTERBOOT */

void
initppc(vaddr_t startkernel, vaddr_t endkernel,
	void *a0, void *a1, char *a2, char *a3)
{
	struct cpu_info * const ci = curcpu();
	struct cpu_softc * const cpu = ci->ci_softc;

	cn_tab = &e500_earlycons;
	printf(" initppc(%#"PRIxVADDR", %#"PRIxVADDR", %p, %p, %p, %p)<enter>",
	    startkernel, endkernel, a0, a1, a2, a3);

#if !defined(ROUTERBOOT)
	if (a2[0] != '\0')
		printf(" consdev=<%s>", a2);
	if (a3[0] != '\0') {
		printf(" cmdline=<%s>", a3);
		parse_cmdline(a3);
	}
#endif	/* !ROUTERBOOT */

	/*
	 * Make sure we don't enter NAP or SLEEP if PSL_POW (MSR[WE]) is set.
	 * DOZE is ok.
	 */
	const register_t hid0 = mfspr(SPR_HID0);
	mtspr(SPR_HID0,
	    (hid0 & ~(HID0_NAP | HID0_SLEEP)) | HID0_TBEN | HID0_EMCP | HID0_DOZE);
#ifdef CADMUS
	/*
	 * Need to cache this from cadmus since we need to unmap cadmus since
	 * it falls in the middle of kernel address space.
	 */
	cadmus_pci = ((uint8_t *)0xf8004000)[CM_PCI];
	cadmus_csr = ((uint8_t *)0xf8004000)[CM_CSR];
	((uint8_t *)0xf8004000)[CM_CSR] |= CM_RST_PHYRST;
	printf(" cadmus_pci=%#x", cadmus_pci);
	printf(" cadmus_csr=%#x", cadmus_csr);
	((uint8_t *)0xf8004000)[CM_CSR] = 0;
	if ((cadmus_pci & CM_PCI_PSPEED) == CM_PCI_PSPEED_66) {
		e500_sys_clk *= 2;
	}
#endif
#ifdef PIXIS
	pixis_spd = ((uint8_t *)PX_BASE)[PX_SPD];
	printf(" pixis_spd=%#x sysclk=%"PRIuMAX,
	    pixis_spd, PX_SPD_SYSCLK_GET(pixis_spd));
#ifndef SYS_CLK
	e500_sys_clk = pixis_spd_map[PX_SPD_SYSCLK_GET(pixis_spd)];
#else
	printf(" pixis_sysclk=%u", pixis_spd_map[PX_SPD_SYSCLK_GET(pixis_spd)]);
#endif
#endif
	printf(" porpllsr=0x%08x",
	    *(uint32_t *)(GUR_BASE + GLOBAL_BASE + PORPLLSR));
	printf(" sys_clk=%"PRIu64, e500_sys_clk);

	/*
	 * Make sure arguments are page aligned.
	 */
	startkernel = trunc_page(startkernel);
	endkernel = round_page(endkernel);

	/*
	 * Initialize the bus space tag used to access the 85xx general
	 * utility registers.  It doesn't need to be extent protected.
	 * We know the GUR is mapped via a TLB1 entry so we add a limited
	 * mapiodev which allows mappings in GUR space.
	 */
	CTASSERT(offsetof(struct tlb_md_io_ops, md_tlb_mapiodev) == 0);
	cpu_md_ops.md_tlb_io_ops = (const void *)&early_tlb_mapiodev;
	bus_space_init(&gur_bst, NULL, NULL, 0);
	bus_space_init(&gur_le_bst, NULL, NULL, 0);
	cpu->cpu_bst = &gur_bst;
	cpu->cpu_le_bst = &gur_le_bst;
	cpu->cpu_bsh = gur_bsh;

	/*
	 * Attach the console early, really early.
	 */
	consinit();

	/*
	 * Reset the PIC to a known state.
	 */
	cpu_write_4(OPENPIC_BASE + OPENPIC_GCR, GCR_RST);
	while (cpu_read_4(OPENPIC_BASE + OPENPIC_GCR) & GCR_RST)
		;
#if 0
	cpu_write_4(OPENPIC_BASE + OPENPIC_CTPR, 15);	/* IPL_HIGH */
#endif
	printf(" openpic-reset(ctpr=%u)",
	    cpu_read_4(OPENPIC_BASE + OPENPIC_CTPR));

	/*
	 * fill in with an absolute branch to a routine that will panic.
	 */
	*(volatile int *)0 = 0x48000002 | (int) calltozero;

	/*
	 * Get the cache sizes.
	 */
	cpu_probe_cache();
		printf(" cache(DC=%uKB/%u,IC=%uKB/%u)",
		    ci->ci_ci.dcache_size >> 10,
		    ci->ci_ci.dcache_line_size,
		    ci->ci_ci.icache_size >> 10,
		    ci->ci_ci.icache_line_size);

	/*
	 * Now find out how much memory is attached
	 */
	pmemsize = memprobe(endkernel);
	cpu->cpu_highmem = pmemsize;
		printf(" memprobe=%zuMB", (size_t) (pmemsize >> 20));

	/*
	 * Now we need cleanout the TLB of stuff that we don't need.
	 */
	e500_tlb_init(endkernel, pmemsize);
		printf(" e500_tlbinit(%#lx,%zuMB)",
		    endkernel, (size_t) (pmemsize >> 20));

	/*
	 *
	 */
	printf(" hid0=%#lx/%#jx", hid0, (uintmax_t)mfspr(SPR_HID0));
	printf(" hid1=%#jx", (uintmax_t)mfspr(SPR_HID1));
	printf(" pordevsr=%#x", cpu_read_4(GLOBAL_BASE + PORDEVSR));
	printf(" devdisr=%#x", cpu_read_4(GLOBAL_BASE + DEVDISR));

	mtmsr(mfmsr() | PSL_CE | PSL_ME | PSL_DE);

	/*
	 * Initialize the message buffer.
	 */
	initmsgbuf((void *)msgbuf_paddr, round_page(MSGBUFSIZE));
	printf(" msgbuf=%p", (void *)msgbuf_paddr);

	/*
	 * Initialize exception vectors and interrupts
	 */
	exception_init(&e500_intrsw);

	printf(" exception_init=%p", &e500_intrsw);

	mtspr(SPR_TCR, TCR_WIE | mfspr(SPR_TCR));

	uvm_md_init();

	/*
	 * Initialize the pmap.
	 */
	endkernel = pmap_bootstrap(startkernel, endkernel, availmemr, nmemr);

	/*
	 * Let's take all the indirect calls via our stubs and patch 
	 * them to be direct calls.
	 */
	cpu_fixup_stubs();

	/*
	 * As a debug measure we can change the TLB entry that maps all of
	 * memory to one that encompasses the 64KB with the kernel vectors.
	 * All other pages will be soft faulted into the TLB as needed.
	 */
	e500_tlb_minimize(endkernel);

	/*
	 * Set some more MD helpers
	 */
	cpu_md_ops.md_cpunode_locs = mpc8548_cpunode_locs;
	cpu_md_ops.md_device_register = e500_device_register;
	cpu_md_ops.md_cpu_attach = e500_cpu_attach;
	cpu_md_ops.md_cpu_reset = e500_cpu_reset;
#if NGPIO > 0
	cpu_md_ops.md_cpunode_attach = pq3gpio_attach;
#endif

	printf(" initppc done!\n");

	/*
	 * Look for the Book-E modules in the right place.
	 */
	module_machine = module_machine_booke;
}

#ifdef MPC8548
static const char * const mpc8548cds_extirq_names[] = {
	[0] = "pci inta",
	[1] = "pci intb",
	[2] = "pci intc",
	[3] = "pci intd",
	[4] = "irq4",
	[5] = "gige phy",
	[6] = "atm phy",
	[7] = "cpld",
	[8] = "irq8",
	[9] = "nvram",
	[10] = "debug",
	[11] = "pci2 inta",
};
#endif

#ifndef MPC8548
static const char * const mpc85xx_extirq_names[] = {
	[0] = "extirq 0",
	[1] = "extirq 1",
	[2] = "extirq 2",
	[3] = "extirq 3",
	[4] = "extirq 4",
	[5] = "extirq 5",
	[6] = "extirq 6",
	[7] = "extirq 7",
	[8] = "extirq 8",
	[9] = "extirq 9",
	[10] = "extirq 10",
	[11] = "extirq 11",
};
#endif

static void
mpc85xx_extirq_setup(void)
{
#ifdef MPC8548
	const char * const * names = mpc8548cds_extirq_names;
	const size_t n = __arraycount(mpc8548cds_extirq_names);
#else
	const char * const * names = mpc85xx_extirq_names;
	const size_t n = __arraycount(mpc85xx_extirq_names);
#endif
	prop_array_t extirqs = prop_array_create_with_capacity(n);
	for (u_int i = 0; i < n; i++) {
		prop_string_t ps = prop_string_create_cstring_nocopy(names[i]);
		prop_array_set(extirqs, i, ps);
		prop_object_release(ps);
	}
	board_info_add_object("external-irqs", extirqs);
	prop_object_release(extirqs);
}

static void
mpc85xx_pci_setup(const char *name, uint32_t intmask, int ist, int inta, ...)
{
	prop_dictionary_t pci_intmap = prop_dictionary_create();
	KASSERT(pci_intmap != NULL);
	prop_number_t mask = prop_number_create_unsigned_integer(intmask);
	KASSERT(mask != NULL);
	prop_dictionary_set(pci_intmap, "interrupt-mask", mask);
	prop_object_release(mask);
	prop_number_t pn_ist = prop_number_create_unsigned_integer(ist);
	KASSERT(pn_ist != NULL);
	prop_number_t pn_intr = prop_number_create_unsigned_integer(inta);
	KASSERT(pn_intr != NULL);
	prop_dictionary_t entry = prop_dictionary_create();
	KASSERT(entry != NULL);
	prop_dictionary_set(entry, "interrupt", pn_intr);
	prop_dictionary_set(entry, "type", pn_ist);
	prop_dictionary_set(pci_intmap, "000000", entry);
	prop_object_release(pn_intr);
	prop_object_release(entry);
	va_list ap;
	va_start(ap, inta);
	u_int intrinc = __LOWEST_SET_BIT(intmask);
	for (u_int i = 0; i < intmask; i += intrinc) {
		char prop_name[12];
		snprintf(prop_name, sizeof(prop_name), "%06x", i + intrinc);
		entry = prop_dictionary_create();
		KASSERT(entry != NULL);
		pn_intr = prop_number_create_unsigned_integer(va_arg(ap, u_int));
		KASSERT(pn_intr != NULL);
		prop_dictionary_set(entry, "interrupt", pn_intr);
		prop_dictionary_set(entry, "type", pn_ist);
		prop_dictionary_set(pci_intmap, prop_name, entry);
		prop_object_release(pn_intr);
		prop_object_release(entry);
	}
	va_end(ap);
	prop_object_release(pn_ist);
	board_info_add_object(name, pci_intmap);
	prop_object_release(pci_intmap);
}

void
cpu_startup(void)
{
	struct cpu_info * const ci = curcpu();
	const uint16_t svr = getsvr();

	powersave = 0;	/* we can do it but turn it on by default */

	booke_cpu_startup(socname(mfspr(SPR_SVR)));

	uint32_t v = cpu_read_4(GLOBAL_BASE + PORPLLSR);
	uint32_t plat_ratio = PLAT_RATIO_GET(v);
	uint32_t e500_ratio = E500_RATIO_GET(v);

	uint64_t ccb_freq = e500_sys_clk * plat_ratio;
	uint64_t cpu_freq = ccb_freq * e500_ratio / 2;

	ci->ci_khz = (cpu_freq + 500) / 1000;
	cpu_timebase = ci->ci_data.cpu_cc_freq = ccb_freq / 8;

	board_info_add_number("my-id", svr);
	board_info_add_bool("pq3");
	board_info_add_number("mem-size", pmemsize);
	const uint32_t l2ctl = cpu_read_4(L2CACHE_BASE + L2CTL);
	uint32_t l2siz = L2CTL_L2SIZ_GET(l2ctl);
	uint32_t l2banks = l2siz >> 16;
#ifdef MPC85555
	if (svr == (MPC8555v1 >> 16)) {
		l2siz >>= 1;
		l2banks >>= 1;
	}
#endif
	paddr_t boot_page = cpu_read_4(GUR_BPTR);
	if (boot_page & BPTR_EN) {
		bool found = false;
		boot_page = (boot_page & BPTR_BOOT_PAGE) << PAGE_SHIFT;
		for (const uint32_t *dp = (void *)(boot_page + PAGE_SIZE - 4),
		     * const bp = (void *)boot_page;
		     bp <= dp; dp--) {
			if (*dp == boot_page) {
				uintptr_t spinup_table_addr = (uintptr_t)++dp;
				spinup_table_addr =
				    roundup2(spinup_table_addr, 32);
				board_info_add_number("mp-boot-page",
				    boot_page);
				board_info_add_number("mp-spin-up-table", 
				    spinup_table_addr);
				printf("Found MP boot page @ %#"PRIxPADDR". "
				    "Spin-up table @ %#"PRIxPTR"\n",
				    boot_page, spinup_table_addr);
				found = true;
				break;
			}
		}
		if (!found) {
			printf("Found MP boot page @ %#"PRIxPADDR
			    " with missing U-boot signature!\n", boot_page);
			board_info_add_number("mp-spin-up-table", 0);
		}
	}
	board_info_add_number("l2-cache-size", l2siz);
	board_info_add_number("l2-cache-line-size", 32);
	board_info_add_number("l2-cache-banks", l2banks);
	board_info_add_number("l2-cache-ways", 8);

	board_info_add_number("processor-frequency", cpu_freq);
	board_info_add_number("bus-frequency", ccb_freq);
	board_info_add_number("pci-frequency", e500_sys_clk);
	board_info_add_number("timebase-frequency", ccb_freq / 8);

#ifdef CADMUS
	const uint8_t phy_base = CM_CSR_EPHY_GET(cadmus_csr) << 2;
	board_info_add_number("tsec1-phy-addr", phy_base + 0);
	board_info_add_number("tsec2-phy-addr", phy_base + 1);
	board_info_add_number("tsec3-phy-addr", phy_base + 2);
	board_info_add_number("tsec4-phy-addr", phy_base + 3);
#else
	board_info_add_number("tsec1-phy-addr", MII_PHY_ANY);
	board_info_add_number("tsec2-phy-addr", MII_PHY_ANY);
	board_info_add_number("tsec3-phy-addr", MII_PHY_ANY);
	board_info_add_number("tsec4-phy-addr", MII_PHY_ANY);
#endif

	uint64_t macstnaddr =
	    ((uint64_t)le32toh(cpu_read_4(ETSEC1_BASE + MACSTNADDR1)) << 16)
	    | ((uint64_t)le32toh(cpu_read_4(ETSEC1_BASE + MACSTNADDR2)) << 48);
	board_info_add_data("tsec-mac-addr-base", &macstnaddr, 6);

#if NPCI > 0 && defined(PCI_MEMBASE)
	pcimem_ex = extent_create("pcimem",
	    PCI_MEMBASE, PCI_MEMBASE + 4*PCI_MEMSIZE,
	    NULL, 0, EX_WAITOK);
#endif
#if NPCI > 0 && defined(PCI_IOBASE)
	pciio_ex = extent_create("pciio",
	    PCI_IOBASE, PCI_IOBASE + 4*PCI_IOSIZE,
	    NULL, 0, EX_WAITOK);
#endif
	mpc85xx_extirq_setup();
	/*
	 * PCI-Express virtual wire interrupts on combined with
	 * External IRQ0/1/2/3.
	 */
	switch (svr) {
#if defined(MPC8548)
	case SVR_MPC8548v1 >> 16:
		mpc85xx_pci_setup("pcie0-interrupt-map", 0x001800,
		    IST_LEVEL, 0, 1, 2, 3);
		break;
#endif
#if defined(MPC8544) || defined(MPC8572) || defined(MPC8536) \
    || defined(P1025) || defined(P2020) || defined(P1023)
	case SVR_MPC8536v1 >> 16:
	case SVR_MPC8544v1 >> 16:
	case SVR_MPC8572v1 >> 16:
	case SVR_P1016v1 >> 16:
	case SVR_P1017v1 >> 16:
	case SVR_P1023v1 >> 16:
	case SVR_P2010v2 >> 16:
	case SVR_P2020v2 >> 16:
		mpc85xx_pci_setup("pcie3-interrupt-map", 0x001800, IST_LEVEL,
		    8, 9, 10, 11);
		/* FALLTHROUGH */
	case SVR_P1025v1 >> 16:
		mpc85xx_pci_setup("pcie2-interrupt-map", 0x001800, IST_LEVEL,
		    4, 5, 6, 7);
		mpc85xx_pci_setup("pcie1-interrupt-map", 0x001800, IST_LEVEL,
		    0, 1, 2, 3);
		break;
#endif
	}
	switch (svr) {
#if defined(MPC8536)
	case SVR_MPC8536v1 >> 16:
		mpc85xx_pci_setup("pci0-interrupt-map", 0x001800, IST_LEVEL,
		    1, 2, 3, 4);
		break;
#endif
#if defined(MPC8544)
	case SVR_MPC8544v1 >> 16:
		mpc85xx_pci_setup("pci0-interrupt-map", 0x001800, IST_LEVEL,
		    0, 1, 2, 3);
		break;
#endif
#if defined(MPC8548)
	case SVR_MPC8548v1 >> 16:
		mpc85xx_pci_setup("pci1-interrupt-map", 0x001800, IST_LEVEL,
		    0, 1, 2, 3);
		mpc85xx_pci_setup("pci2-interrupt-map", 0x001800, IST_LEVEL,
		    11, 1, 2, 3);
		break;
#endif
	}
}
