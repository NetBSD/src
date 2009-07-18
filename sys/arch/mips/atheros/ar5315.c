/* $NetBSD: ar5315.c,v 1.5.10.1 2009/07/18 14:52:54 yamt Exp $ */

/*
 * Copyright (c) 2006 Urbana-Champaign Independent Media Center.
 * Copyright (c) 2006 Garrett D'Amore.
 * All rights reserved.
 *
 * Portions of this code were written by Garrett D'Amore for the
 * Champaign-Urbana Community Wireless Network Project.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgements:
 *      This product includes software developed by the Urbana-Champaign
 *      Independent Media Center.
 *	This product includes software developed by Garrett D'Amore.
 * 4. Urbana-Champaign Independent Media Center's name and Garrett
 *    D'Amore's name may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER AND GARRETT D'AMORE ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER OR GARRETT D'AMORE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/*
 * This file includes a bunch of implementation specific bits for
 * AR5315, which differs these from other members of the AR531X
 * family.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ar5315.c,v 1.5.10.1 2009/07/18 14:52:54 yamt Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"

#include "opt_memsize.h"
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/device.h>

#include <mips/cache.h>
#include <mips/locore.h>
#include <mips/cpuregs.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <prop/proplib.h>

#include <ah_soc.h>	/* XXX really doesn't belong in hal */

#include <mips/atheros/include/ar5315reg.h>
#include <mips/atheros/include/ar531xvar.h>
#include <mips/atheros/include/arbusvar.h>

#include <machine/locore.h>

/* helper macro for accessing system registers without bus space */
#define	REGVAL(x)	*((volatile uint32_t *)(MIPS_PHYS_TO_KSEG1((x))))
#define	GETSYSREG(x)	REGVAL((x) + AR5315_SYSREG_BASE)
#define	PUTSYSREG(x,v)	(REGVAL((x) + AR5315_SYSREG_BASE)) = (v)
#define	GETPCIREG(x)	REGVAL((x) + AR5315_PCI_BASE)
#define	PUTPCIREG(x,v)	(REGVAL((x) + AR5315_PCI_BASE)) = (v)
#define	GETSDRAMREG(x)	REGVAL((x) + AR5315_SDRAMCTL_BASE)

uint32_t
ar531x_memsize(void)
{
#ifndef	MEMSIZE
	uint32_t	memsize = 0;
	uint32_t	memcfg, cw, rw, dw;

	/*
	 * Determine the memory size.  We query the board info.
	 */
	memcfg = GETSDRAMREG(AR5315_SDRAMCTL_MEM_CFG);
	cw = (memcfg & AR5315_MEM_CFG_COL_WIDTH_MASK) >>
	    AR5315_MEM_CFG_COL_WIDTH_SHIFT;
	cw += 1;
	rw = (memcfg & AR5315_MEM_CFG_ROW_WIDTH_MASK) >>
	    AR5315_MEM_CFG_ROW_WIDTH_SHIFT;
	rw += 1;

	/* XXX: according to redboot, this could be wrong if DDR SDRAM */
	dw = (memcfg & AR5315_MEM_CFG_DATA_WIDTH_MASK) >>
	    AR5315_MEM_CFG_DATA_WIDTH_SHIFT;
	dw += 1;
	dw *= 8;	/* bits */

	/* not too sure about this math, but it _seems_ to add up */
	memsize = (1 << cw) * (1 << rw) * dw;
#if 0
	printf("SDRAM_MEM_CFG =%x, cw=%d rw=%d dw=%d xmemsize=%d\n", memcfg,
	    cw, rw, dw, memsize);
#endif

	return (memsize);
#else
	/* compile time value forced */
	return MEMSIZE;
#endif
}

const char *
ar531x_cpuname(void)
{
	uint16_t	rev = GETSYSREG(AR5315_SYSREG_SREV);
	switch (rev) {
	case 0x52:	/* AP30 */
	case 0x57:	/* AP31 */
		return "Atheros AR5312";
	case 0x58:	/* AP43 */
		return "Atheros AR2313";
	case 0x86:	/* AP51-Light */
	case 0x87:	/* AP51-Full */
		return "Atheros AR2315";
	case 0x91:	/* AP61 */
		return "Atheros AR2317";
	}
	return ("Atheros AR531X");
}

void
ar531x_wdog(uint32_t period)
{

	if (period == 0) {
		PUTSYSREG(AR5315_SYSREG_WDOG_CTL, AR5315_WDOG_CTL_IGNORE);
		PUTSYSREG(AR5315_SYSREG_WDOG_TIMER, 0);
	} else {
		PUTSYSREG(AR5315_SYSREG_WDOG_TIMER, period);
		PUTSYSREG(AR5315_SYSREG_WDOG_CTL, AR5315_WDOG_CTL_RESET);
	}
}

void
ar531x_businit(void)
{
	/*
	 * XXX: clear COP0 config bits 0 and 1 -- Linux sets KSEG0 to either
	 * 0 or 4.  Why does it do this?  It is implementation defined...
	 */
	mips3_cp0_config_write(mips3_cp0_config_read() & ~0x3);

	PUTSYSREG(AR5315_SYSREG_AHB_ERR0, AR5315_AHB_ERROR_DET);
	GETSYSREG(AR5315_SYSREG_AHB_ERR1);
}

static uint32_t
get_freq(uint32_t clkreg) 
{
	uint32_t	freq = 0;
	uint32_t	clkctl, pllc, pllout, refdiv, fbdiv, div2, cpudiv;

	static const int pll_divide_table[] = {
		2, 3, 4, 6, 3,
		/*
		 * these entries are bogus, but it avoids a possible
		 * bad table dereference
		 */
		1, 1, 1
	};
	static const int pre_divide_table[] = {
		1, 2, 4, 5
	};

	if (freq)
		return freq;

	pllc = GETSYSREG(AR5315_SYSREG_PLLC_CTL);
	clkctl = GETSYSREG(clkreg);

	refdiv = pre_divide_table[AR5315_PLLC_REF_DIV(pllc)];
	fbdiv = AR5315_PLLC_FB_DIV(pllc);
	div2 = (AR5315_PLLC_DIV_2(pllc) + 1) * 2;	/* results in 2 or 4 */

	cpudiv = AR5315_CLOCKCTL_DIV(clkctl);
	cpudiv = cpudiv ? (cpudiv * 2) : 1;

	/* 40MHz reference clk, reference and feedback dividers */
	pllout = (40000000 / refdiv) * div2 * fbdiv;

	switch (AR5315_CLOCKCTL_SELECT(clkctl)) {
	case 0:
	case 1:
		/* CLKM select */
		pllout /= pll_divide_table[AR5315_PLLC_CLKM(pllc)];
		break;
	case 2:
		/* CLKC select */
		pllout /= pll_divide_table[AR5315_PLLC_CLKC(pllc)];
		break;
	default:
		/* ref_clk select */
		pllout = 40000000;	/* use original reference clock */
		break;
	}

	freq = pllout/(cpudiv); 

	return (freq);
}

uint32_t
ar531x_cpu_freq(void)
{
	static uint32_t	freq = 0;
	if (freq == 0)
		freq = get_freq(AR5315_SYSREG_CPUCLK);
	return (freq);
}

uint32_t
ar531x_bus_freq(void)
{
	static uint32_t	freq = 0;
	if (freq == 0)
		freq = get_freq(AR5315_SYSREG_AMBACLK);
	return (freq);
}

static void
addprop_data(struct device *dev, const char *name, const uint8_t *data,
    int len)
{
	prop_data_t	pd;
	pd = prop_data_create_data(data, len);
	KASSERT(pd != NULL);
	if (prop_dictionary_set(device_properties(dev), name, pd) == false) {
		printf("WARNING: unable to set %s property for %s\n",
		    name, device_xname(dev));
	}
	prop_object_release(pd);
}

static void
addprop_integer(struct device *dev, const char *name, uint32_t val)
{
	prop_number_t	pn;
	pn = prop_number_create_integer(val);
	KASSERT(pn != NULL);
	if (prop_dictionary_set(device_properties(dev), name, pn) == false) {
		printf("WARNING: unable to set %s property for %s",
		    name, device_xname(dev));
	}
	prop_object_release(pn);
}

void
ar531x_device_register(struct device *dev, void *aux) 
{
	struct arbus_attach_args *aa = aux;
	const struct ar531x_boarddata *info;

	info = ar531x_board_info();
	if (info == NULL) {
		/* nothing known about this board! */
		return;
	}

	/*
	 * We don't ever know the boot device.  But that's because the
	 * firmware only loads from the network.
	 */

	/* Fetch the MAC addresses. */
	if (device_is_a(dev, "ae")) {
		const uint8_t *enet;

		if (aa->aa_addr == AR5315_ENET_BASE)
			enet = info->enet0Mac;
		else
			return;

		addprop_data(dev, "mac-addr", enet, ETHER_ADDR_LEN);
	}

	if (device_is_a(dev, "ath")) {
		const uint8_t *enet;

		if (aa->aa_addr == AR5315_WLAN_BASE)
			enet = info->wlan0Mac;
		else
			return;

		addprop_data(dev, "mac-addr", enet, ETHER_ADDR_LEN);

		addprop_integer(dev, "wmac-rev",
		    GETSYSREG(AR5315_SYSREG_SREV));
	}

	if (device_is_a(dev, "com")) {
		addprop_integer(dev, "frequency", ar531x_bus_freq());
	}

	if (device_is_a(dev, "argpio")) {
		if (info->config & BD_RSTFACTORY) {
			addprop_integer(dev, "reset-pin",
			    info->resetConfigGpio);
		}
		if (info->config & BD_SYSLED) {
			addprop_integer(dev, "sysled-pin",
			    info->sysLedGpio);
		}
	}
}

const struct ar531x_device *
ar531x_get_devices(void)
{
	const static struct ar531x_device devices[] = {
		{
			"com",
			AR5315_UART_BASE, 0x1000,
			AR5315_CPU_IRQ_MISC, AR5315_MISC_IRQ_UART,
			0, 0, 0
		},
		{
			"ae",
			AR5315_ENET_BASE, 0x100000,
			AR5315_CPU_IRQ_ENET, -1,
			0, 0, 0
		},
		{
			"ath",
			AR5315_WLAN_BASE, 0x100000,
			AR5315_CPU_IRQ_WLAN, -1,
			0, 0, 0
		},
		{
			"arspi",
			AR5315_SPI_BASE, 0x10,
			AR5315_CPU_IRQ_MISC, AR5315_MISC_IRQ_SPI,
			0, 0, 0
		},
		{ NULL }
	};

	return devices;
}

int
ar531x_enable_device(const struct ar531x_device *dev)
{
	if (dev->addr == AR5315_WLAN_BASE) {
		/* enable arbitration for wlan */
		PUTSYSREG(AR5315_SYSREG_AHB_ARB_CTL,
		    GETSYSREG(AR5315_SYSREG_AHB_ARB_CTL) | AR5315_ARB_WLAN);

		/* set WLAN for big endian */
		PUTSYSREG(AR5315_SYSREG_ENDIAN,
		    GETSYSREG(AR5315_SYSREG_ENDIAN) | AR5315_ENDIAN_WLAN);

		/* wake up the mac */
		PUTPCIREG(AR5315_PCI_MAC_SCR,
		    (GETPCIREG(AR5315_PCI_MAC_SCR) & ~PCI_MAC_SCR_SLM_MASK) |
		    PCI_MAC_SCR_SLM_FWAKE);

		/* wait for it to wake up */
		while (GETPCIREG(AR5315_PCI_MAC_PCICFG) &
		    PCI_MAC_PCICFG_SPWR_DN);
	}
	return 0;
}
