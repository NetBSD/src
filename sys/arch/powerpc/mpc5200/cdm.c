/*	$NetBSD: cdm.c,v 1.1 2026/06/27 13:28:34 rkujawa Exp $	*/

/*-
 * Copyright (c) 2008, 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa and Robert Swindells.
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

/*
 * Driver for the MPC5200B Clock Distribution Module (CDM).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cdm.c,v 1.1 2026/06/27 13:28:34 rkujawa Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <dev/ofw/openfirm.h>

#include <machine/autoconf.h>

#include <powerpc/mpc5200/obiovar.h>
#include <powerpc/mpc5200/mpc5200reg.h>
#include <powerpc/mpc5200/cdmvar.h>

/*
 * External reference oscillator on the SYS_XTAL_IN pin, in Hz.
 */
#define CDM_SYS_XTAL_IN		33333333	/* XXX: should be configurable? */

/* CDM register offsets from the block base (MBAR + 0x0200). */
#define CDM_JTAG_ID		0x00	/* JTAG identification (ro)	*/
#define CDM_PORCFG		0x04	/* power-on reset config (ro)	*/
#define CDM_BREADCRUMB		0x08	/* firmware scratch (never reset) */
#define CDM_CFG			0x0c	/* clock configuration		*/
#define CDM_FRAC_DIV		0x10	/* 48MHz fractional divider	*/
#define CDM_CLK_ENABLE		0x14	/* per-module clock enables	*/

#define CDM_REG_SIZE		0x38	/* register window if OF omits a size */

/*
 * Register fields use the manual's big-endian bit numbering (MSB = bit 0);
 * CDM_BIT() converts a manual bit number to NetBSD __BIT() (LSB = bit 0).
 */
#define CDM_BIT(n)		__BIT(31 - (n))

/* CDM Power-On Reset Configuration Register (CDM_PORCFG). */
#define PORCFG_SYS_PLL_BYPASS	CDM_BIT(15)	/* PLL bypassed, xtal direct	*/
#define PORCFG_SYS_PLL_CFG_1	CDM_BIT(24)	/* doubles VCO only (no clk effect) */
#define PORCFG_SYS_PLL_CFG_0	CDM_BIT(25)	/* 0: fsys=16x, 1: fsys=12x xtal	*/
#define PORCFG_XLB_CLK_SEL	CDM_BIT(26)	/* 0: XLB=fsys/4, 1: XLB=fsys/8	*/
#define PORCFG_PPC_PLL_CFG	__BITS(0, 4)	/* e300 core APLL config [0:4]	*/

/* CDM Configuration Register (CDM_CFG). */
#define CFG_XLB_CLK_SEL		CDM_BIT(15)	/* read-only mirror of PORCFG	*/
#define CFG_IPB_CLK_SEL		CDM_BIT(23)	/* 0: IPB=XLB, 1: IPB=XLB/2	*/
#define CFG_PCI_CLK_SEL		__BITS(0, 1)	/* PCI clock select [1:0]	*/

#define PCI_CLK_SEL_IPB		0		/* PCI = IPB			*/
#define PCI_CLK_SEL_IPB_DIV2	1		/* PCI = IPB / 2		*/
/* values 2 and 3 both select PCI = XLB / 4 */

/*
 * Computed clock tree, shared with the on-demand accessors.
 */
static struct cdm_clocks {
	bool		cc_valid;
	uint32_t	cc_core;
	uint32_t	cc_xlb;
	uint32_t	cc_ipb;
	uint32_t	cc_pci;
} cdm_clocks;

static int	cdm_match(device_t, cfdata_t, void *);
static void	cdm_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cdm, sizeof(struct cdm_softc),
    cdm_match, cdm_attach, NULL, NULL);

static const uint8_t cdm_core_ratio_x2[32] = {
	[0x03] =  2,	/* 1x  - xlb clocks core directly		*/
	[0x04] =  4,	/* 2x   */	[0x05] =  4,	/* 2x   */
	[0x06] =  5,	/* 2.5x */	[0x07] =  9,	/* 4.5x */
	[0x08] =  6,	/* 3x   */	[0x09] = 11,	/* 5.5x */
	[0x0a] =  8,	/* 4x   */	[0x0b] = 10,	/* 5x   */
	[0x0d] = 12,	/* 6x   */	[0x0e] =  7,	/* 3.5x */
	[0x10] =  6,	/* 3x   */	[0x11] =  5,	/* 2.5x */
	[0x12] = 13,	/* 6.5x */	[0x13] =  2,	/* 1x direct */
	[0x14] = 14,	/* 7x   */	[0x16] = 15,	/* 7.5x */
	[0x1c] = 16,	/* 8x   */
};

/*
 * Reconstruct the clock tree from the two strap registers and cache it.
 */
static void
cdm_compute(uint32_t porcfg, uint32_t cfg)
{
	uint32_t fsystem, xlb, ipb, pci, core;
	uint32_t ppc_cfg, ratio_x2;

	if (porcfg & PORCFG_SYS_PLL_BYPASS)
		fsystem = CDM_SYS_XTAL_IN;	/* PLL bypassed, no multiply */
	else
		fsystem = CDM_SYS_XTAL_IN *
		    ((porcfg & PORCFG_SYS_PLL_CFG_0) ? 12 : 16);

	xlb = fsystem / ((porcfg & PORCFG_XLB_CLK_SEL) ? 8 : 4);
	ipb = xlb / ((cfg & CFG_IPB_CLK_SEL) ? 2 : 1);

	switch (__SHIFTOUT(cfg, CFG_PCI_CLK_SEL)) {
	case PCI_CLK_SEL_IPB:
		pci = ipb;
		break;
	case PCI_CLK_SEL_IPB_DIV2:
		pci = ipb / 2;
		break;
	default:			/* 2, 3: PCI = XLB / 4 */
		pci = xlb / 4;
		break;
	}

	ppc_cfg = __SHIFTOUT(porcfg, PORCFG_PPC_PLL_CFG);
	ratio_x2 = cdm_core_ratio_x2[ppc_cfg];
	if (ratio_x2 != 0)
		core = (uint32_t)(((uint64_t)xlb * ratio_x2) / 2);
	else
		core = 0;		/* reserved/unclocked strap */

	cdm_clocks.cc_core = core;
	cdm_clocks.cc_xlb = xlb;
	cdm_clocks.cc_ipb = ipb;
	cdm_clocks.cc_pci = pci;
	cdm_clocks.cc_valid = true;
}

/*
 * Ensure the clock tree is populated.
 */
static void
cdm_ensure_valid(void)
{
	volatile uint32_t *cdm;
	uint32_t porcfg, cfg;

	if (cdm_clocks.cc_valid)
		return;

	cdm = (volatile uint32_t *)(uintptr_t)
	    (MPC5200_MBAR_DEFAULT + MPC5200_REG_CDM);
	porcfg = cdm[CDM_PORCFG / sizeof(*cdm)];
	cfg = cdm[CDM_CFG / sizeof(*cdm)];

	cdm_compute(porcfg, cfg);

	/*
	 * Guard against an implausible read
	 */
	if (cdm_clocks.cc_ipb < 16000000 || cdm_clocks.cc_ipb > 200000000) {
		cdm_clocks.cc_core = MPC5200_CORE_FREQ_DEFAULT;
		cdm_clocks.cc_xlb = MPC5200_XLB_FREQ_DEFAULT;
		cdm_clocks.cc_ipb = MPC5200_IPB_FREQ_DEFAULT;
		cdm_clocks.cc_pci = MPC5200_PCI_FREQ_DEFAULT;
	}
}

uint32_t
mpc5200_cdm_get_core_freq(void)
{
	cdm_ensure_valid();
	return cdm_clocks.cc_core != 0 ?
	    cdm_clocks.cc_core : MPC5200_CORE_FREQ_DEFAULT;
}

uint32_t
mpc5200_cdm_get_xlb_freq(void)
{
	cdm_ensure_valid();
	return cdm_clocks.cc_xlb;
}

uint32_t
mpc5200_cdm_get_ipb_freq(void)
{
	cdm_ensure_valid();
	return cdm_clocks.cc_ipb;
}

uint32_t
mpc5200_cdm_get_pci_freq(void)
{
	cdm_ensure_valid();
	return cdm_clocks.cc_pci;
}

static int
cdm_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args *oba = aux;
	char compat[32];
	int len;

	if (strcmp(oba->obio_name, "cdm") == 0)
		return 1;

	len = OF_getprop(oba->obio_node, "compatible", compat, sizeof(compat));
	if (len > 0 &&
	    (strcmp(compat, "mpc5200-cdm") == 0 ||
	     strcmp(compat, "mpc5200b-cdm") == 0))
		return 1;

	return 0;
}

static void
cdm_attach(device_t parent, device_t self, void *aux)
{
	struct cdm_softc *sc = device_private(self);
	struct obio_attach_args *oba = aux;
	bus_size_t size;
	uint32_t porcfg, cfg;

	sc->sc_dev = self;
	sc->sc_iot = oba->obio_bst;

	size = oba->obio_size != 0 ? oba->obio_size : CDM_REG_SIZE;
	if (bus_space_map(sc->sc_iot, oba->obio_addr, size, 0,
	    &sc->sc_ioh) != 0) {
		aprint_error(": can't map registers\n");
		return;
	}

	porcfg = bus_space_read_4(sc->sc_iot, sc->sc_ioh, CDM_PORCFG);
	cfg = bus_space_read_4(sc->sc_iot, sc->sc_ioh, CDM_CFG);
	cdm_compute(porcfg, cfg);

	aprint_normal(": core %u.%u MHz, XLB %u.%u MHz, IPB %u.%u MHz, "
	    "PCI %u.%u MHz\n",
	    cdm_clocks.cc_core / 1000000, (cdm_clocks.cc_core / 100000) % 10,
	    cdm_clocks.cc_xlb / 1000000, (cdm_clocks.cc_xlb / 100000) % 10,
	    cdm_clocks.cc_ipb / 1000000, (cdm_clocks.cc_ipb / 100000) % 10,
	    cdm_clocks.cc_pci / 1000000, (cdm_clocks.cc_pci / 100000) % 10);
}
