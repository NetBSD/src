/* $NetBSD: ar9344.c,v 1.4.10.1 2014/08/10 06:54:02 tls Exp $ */

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
 * AR9344, which differs these from other members of the AR9344
 * family.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ar9344.c,v 1.4.10.1 2014/08/10 06:54:02 tls Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_memsize.h"

#define __INTR_PRIVATE

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <mips/locore.h>

#include <mips/atheros/include/ar9344reg.h>
#include <mips/atheros/include/platform.h>
#include <mips/atheros/include/arbusvar.h>

static uint32_t
ar9344_get_memsize(void)
{
#ifndef	MEMSIZE
	uint32_t	memsize = 64*1024*1024;

	uint32_t	memcfg = GETDDRREG(AR9344_DDR_RD_DATA_THIS_CYCLE);

	/*
	 * 32-bit means twice the memory.
	 */
	if (memcfg == 0xff)
		memsize <<= 1;

	return memsize;
#else
	/* compile time value forced */
	return MEMSIZE;
#endif
}

static void
ar9344_wdog_reload(uint32_t period)
{

	if (period == 0) {
		PUTRESETREG(ARCHIP_RESET_WDOG_CTL, ARCHIP_WDOG_CTL_IGNORE);
		PUTRESETREG(ARCHIP_RESET_WDOG_TIMER, 0);
	} else {
		PUTRESETREG(ARCHIP_RESET_WDOG_TIMER, period);
		PUTRESETREG(ARCHIP_RESET_WDOG_CTL, ARCHIP_WDOG_CTL_RESET);
	}
}

static void
ar9344_bus_init(void)
{
#if 0
	PUTRESET(AR9344_RESET_AHB_ERR0, AR9344_AHB_ERROR_DET);
	GETRESET(AR9344_RESET_AHB_ERR1);
#endif
}

static void
ar9344_reset(void)
{
	PUTRESETREG(AR9344_RESET_RESETCTL, ARCHIP_RESETCTL_FULL_CHIP_RESET);
}


static void
ar9344_get_freqs(struct arfreqs *freqs)
{
	uint32_t out_div, ref_div, nint, post_div;
	uint32_t pll;
	uint32_t ref_clk;
	//uint32_t nfrac;

	if (GETRESETREG(AR9344_RESET_BOOTSTRAP) & AR9344_BOOTSTRAP_REF_CLK_40) {
		ref_clk = 40 * 1000000;
	} else {
		ref_clk = 25 * 1000000;
	}

	freqs->freq_ref = ref_clk;

	/*
	 * Let's figure out the CPU PLL frequency.
	 */
	pll = GETPLLREG(ARCHIP_PLL_CPU_PLL_CONFIG);
	out_div = __SHIFTOUT(pll, AR9344_CPU_PLL_CONFIG_OUTDIV);
	ref_div = __SHIFTOUT(pll, AR9344_CPU_PLL_CONFIG_REFDIV);
	nint = __SHIFTOUT(pll, AR9344_CPU_PLL_CONFIG_NINT);
	//nfrac = __SHIFTOUT(pll, AR9344_CPU_PLL_CONFIG_NFRAC);

	const uint32_t cpu_pll_freq = (nint * ref_clk / ref_div) >> out_div;

	/*
	 * Now figure out the DDR PLL frequency.
	 */
	pll = GETPLLREG(ARCHIP_PLL_DDR_PLL_CONFIG);
	out_div = __SHIFTOUT(pll, AR9344_DDR_PLL_CONFIG_OUTDIV);
	ref_div = __SHIFTOUT(pll, AR9344_DDR_PLL_CONFIG_REFDIV);
	nint = __SHIFTOUT(pll, AR9344_DDR_PLL_CONFIG_NINT);
	//nfrac = __SHIFTOUT(pll, AR9344_DDR_PLL_CONFIG_NFRAC);

	const uint32_t ddr_pll_freq = (nint * ref_clk / ref_div) >> out_div;

	/*
	 * Now we find out the various frequencies...
	 */
	uint32_t clk_ctl = GETPLLREG(ARCHIP_PLL_CPU_DDR_CLOCK_CONTROL);
	post_div = __SHIFTOUT(clk_ctl,
	    AR9344_CPU_DDR_CLOCK_CONTROL_AHB_POST_DIV);
	if (clk_ctl & AR9344_CPU_DDR_CLOCK_CONTROL_AHBCLK_FROM_DDRPLL) {
		freqs->freq_bus = ddr_pll_freq / (post_div + 1);
	} else {
		freqs->freq_bus = cpu_pll_freq / (post_div + 1);
	}

	post_div = __SHIFTOUT(clk_ctl,
	    AR9344_CPU_DDR_CLOCK_CONTROL_CPU_POST_DIV);
	if (clk_ctl & AR9344_CPU_DDR_CLOCK_CONTROL_CPUCLK_FROM_CPUPLL) {
		freqs->freq_cpu = cpu_pll_freq / (post_div + 1);
		freqs->freq_pll = cpu_pll_freq;
	} else {
		freqs->freq_cpu = ddr_pll_freq / (post_div + 1);
		freqs->freq_pll = ddr_pll_freq;
	}

	post_div = __SHIFTOUT(clk_ctl,
	    AR9344_CPU_DDR_CLOCK_CONTROL_DDR_POST_DIV);
	if (clk_ctl & AR9344_CPU_DDR_CLOCK_CONTROL_DDRCLK_FROM_DDRPLL) {
		freqs->freq_mem = ddr_pll_freq / (post_div + 1);
	} else {
		freqs->freq_mem = cpu_pll_freq / (post_div + 1);
	}

	/*
	 * Console is off the reference clock, not the bus clock.
	 */
	freqs->freq_uart = freqs->freq_ref;
}

#if 0
static void
addprop_data(device_t dev, const char *name, const uint8_t *data,
    int len)
{
	prop_data_t	pd;
	pd = prop_data_create_data(data, len);
	KASSERT(pd != NULL);
	if (prop_dictionary_set(device_properties(dev), name, pd) == FALSE) {
		printf("WARNING: unable to set %s property for %s\n",
		    name, device_xname(dev));
	}
	prop_object_release(pd);
}
#endif

static void
addprop_integer(device_t dev, const char *name, uint32_t val)
{
	prop_number_t	pn;
	pn = prop_number_create_integer(val);
	KASSERT(pn != NULL);
	if (prop_dictionary_set(device_properties(dev), name, pn) == FALSE) {
		printf("WARNING: unable to set %s property for %s",
		    name, device_xname(dev));
	}
	prop_object_release(pn);
}

static void
ar9344_device_register(device_t dev, void *aux) 
{

	if (device_is_a(dev, "com")
	    && device_is_a(device_parent(dev), "arbus")) {
		addprop_integer(dev, "frequency", atheros_get_bus_freq());
		return;
	}

#if 0
	const struct arbus_attach_args * const aa = aux;
	const struct ar9344_boarddata *info;
	info = ar9344_board_info();
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
		uint8_t enaddr[ETHER_ADDR_LEN];

		memcpy(enaddr, info->enet0Mac, ETHER_ADDR_LEN);
		if (aa->aa_addr == AR9344_GMAC0_BASE) {
			;
		} else if (aa->aa_addr == AR9344_GMAC1_BASE) {
			enaddr[5] ^= 1;
		} else
			return;

		addprop_data(dev, "mac-addr", enaddr, ETHER_ADDR_LEN);
	}

#if 0
	if (device_is_a(dev, "ath")) {
		const uint8_t *enet;

		if (aa->aa_addr == AR9344_WLAN_BASE)
			enet = info->wlan0Mac;
		else
			return;

		addprop_data(dev, "mac-addr", enet, ETHER_ADDR_LEN);

		addprop_integer(dev, "wmac-rev",
		    GETRESET(AR9344_RESET_SREV));
	}
#endif

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
#endif
}

static int
ar9344_enable_device(const struct atheros_device *adv)
{
	if (adv->adv_reset) {
		/* put device into reset */
		PUTRESETREG(AR9344_RESET_RESETCTL,
		    GETRESETREG(AR9344_RESET_RESETCTL) | adv->adv_reset);

		delay(15000);	/* XXX: tsleep? */

		/* take it out of reset */
		PUTRESETREG(AR9344_RESET_RESETCTL,
		    GETRESETREG(AR9344_RESET_RESETCTL) & ~adv->adv_reset);

		delay(25);
	}
	if (adv->adv_enable)
		panic("%s: %s: enable not supported!", __func__, adv->adv_name);

	return 0;
}

static void
ar9344_intr_init(void)
{
	atheros_intr_init();
}

static const char * const ar9344_cpu_intrnames[] = {
	[AR9344_CPU_IRQ_PCIERC] =		"irq 0 (pcie rc)",
	[ARCHIP_CPU_IRQ_USB] =			"irq 1 (usb)",
	[ARCHIP_CPU_IRQ_GMAC0] =		"irq 2 (gmac0)",
	[ARCHIP_CPU_IRQ_GMAC1] =		"irq 3 (gmac1)",
	[ARCHIP_CPU_IRQ_MISC] =			"irq 4 (misc)",
	[ARCHIP_CPU_IRQ_TIMER] =		"irq 5 (timer)",
#if 0
	[AR9344_CPU_IRQ_PCIEEP_HSTDMA] =	"irq 6 (pcieep)",
#endif
};

static const char * const ar9344_misc_intrnames[] = {
	[AR9344_MISC_IRQ_TIMER] =		"irq 0 (timer1)",
	[AR9344_MISC_IRQ_ERROR] =		"irq 1 (error)",
	[AR9344_MISC_IRQ_GPIO] =		"irq 2 (gpio)",
	[AR9344_MISC_IRQ_UART0] =		"irq 3 (uart0)",
	[AR9344_MISC_IRQ_WDOG] =		"irq 4 (wdog)",
	[AR9344_MISC_IRQ_PC] =	       		"irq 5 (pc)",
	[AR9344_MISC_IRQ_UART1] =		"irq 6 (uart1)",
	[AR9344_MISC_IRQ_MBOX] =		"irq 7 (mbox)",
	[AR9344_MISC_IRQ_TIMER2] =		"irq 8 (timer2)",
	[AR9344_MISC_IRQ_TIMER3] =		"irq 9 (timer3)",
	[AR9344_MISC_IRQ_TIMER4] =		"irq 10 (timer4)",
	[AR9344_MISC_IRQ_DDR_PERF] =		"irq 11 (ddr_perf)",
	[AR9344_MISC_IRQ_SW_MAC] =		"irq 12 (sw_mac)",
	[AR9344_MISC_IRQ_LUTS_AGER] =		"irq 13 (lut_ager)",
	[AR9344_MISC_IRQ_CHKSUM_ACC] =		"irq 15 (chksum_acc)",
	[AR9344_MISC_IRQ_DDR_SF_ENTRY] =	"irq 16 (ddr_sf_entry)",
	[AR9344_MISC_IRQ_DDR_SF_EXIT] =		"irq 17 (ddr_sf_exit)",
	[AR9344_MISC_IRQ_DDR_ACT_IN_SF] =	"irq 18 (ddr_act_in_sf)",
	[AR9344_MISC_IRQ_SLIC] =		"irq 19 (slic)",
	[AR9344_MISC_IRQ_WOW] =			"irq 20 (wow)",
	[AR9344_MISC_IRQ_NANDF] =		"irq 21 (nandf)",
};

#if 0
static const char * const ar9344_misc2_intrnames[] = {
	[AR9344_WMAC_IRQ_WMAC_MISC_INT] =	"irq 0 (wmac misc)",
	[AR9344_WMAC_IRQ_WMAC_TX_INT] =		"irq 1 (wmac tx)",
	[AR9344_WMAC_IRQ_WMAC_RXLP_INT] =	"irq 2 (wmac rxlp)",
	[AR9344_WMAC_IRQ_WMAC_RXHP_INT] =	"irq 3 (wmac rxhp)",
	[AR9344_WMAC_IRQ_PCIE_RC_INT] =		"irq 4 (pcie rc int)",
	[AR9344_WMAC_IRQ_PCIE_RC_INT0] =	"irq 5 (pcie rc int 0)",
	[AR9344_WMAC_IRQ_PCIE_RC_INT1] =	"irq 6 (pcie rc int 1)",
	[AR9344_WMAC_IRQ_PCIE_RC_INT2] =	"irq 7 (pcie rc int 2)",
	[AR9344_WMAC_IRQ_PCIE_RC_INT3] =	"irq 8 (pcie rc int 3)",
};
#endif

static const struct ipl_sr_map ar9344_ipl_sr_map = {
    .sr_bits = {
	[IPL_NONE] =		0,
	[IPL_SOFTCLOCK] = 	MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTBIO] = 	MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET] = 	MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTSERIAL] = 	MIPS_SOFT_INT_MASK_0,
	[IPL_VM] =		MIPS_SOFT_INT_MASK |
				MIPS_INT_MASK_0 |	/* PCIE RC */
				MIPS_INT_MASK_1	|	/* USB */
				MIPS_INT_MASK_2 |	/* GMAC0 */
				MIPS_INT_MASK_3 |	/* GMAC1 */
				MIPS_INT_MASK_4,	/* MISC */
	[IPL_SCHED] =		MIPS_INT_MASK,		/* EVERYTHING */
	[IPL_DDB] =		MIPS_INT_MASK,		/* EVERYTHING */
	[IPL_HIGH] =		MIPS_INT_MASK,		/* EVERYTHING */
     },
};

static const struct atheros_device ar9344_devices[] = {
	{
		.adv_name = "com",
		.adv_addr = AR9344_UART0_BASE,
		.adv_size = 0x1000,
		.adv_cirq = ARCHIP_CPU_IRQ_MISC,
		.adv_mirq = AR9344_MISC_IRQ_UART0,
	}, {
		.adv_name = "ehci",
		.adv_addr = AR9344_USB_BASE + 0x100,
		.adv_size = 0x1000,
		.adv_cirq = ARCHIP_CPU_IRQ_USB,
		.adv_mirq = -1,
		.adv_reset = AR9344_RESETCTL_USB_PHY_SUSPEND_OVERRIDE
		    | ARCHIP_RESETCTL_USB_PHY_RESET
		    | ARCHIP_RESETCTL_USB_HOST_RESET,
	}, {
		.adv_name = "age",
		.adv_addr = AR9344_GMAC0_BASE,
		.adv_size = 0x1000,
		.adv_cirq = ARCHIP_CPU_IRQ_GMAC0,
		.adv_mirq = -1,
	}, {
		.adv_name = "age",
		.adv_addr = AR9344_GMAC1_BASE,
		.adv_size = 0x1000,
		.adv_cirq = ARCHIP_CPU_IRQ_GMAC1,
		.adv_mirq = -1,
	}, {
		.adv_name = "arpcie",
		.adv_addr = AR9344_PCIE_RC_BASE,
		.adv_size = 0x1000,
		.adv_cirq = AR9344_CPU_IRQ_PCIERC,
		.adv_mirq = -1,
	},
#if 0
	{
		.adv_name = "ath",
		.adv_addr = AR9344_WLAN_BASE,
		.adv_size = 0x100000,
		.adv_cirq = AR9344_CPU_IRQ_WLAN,
		.adv_mirq = -1,
	}, {
		.adv_name = "arspi",
		.adv_addr = AR9344_SPI_BASE,
		.adv_size = 0x20,
		.adv_cirq = AR9344_CPU_IRQ_MISC,
		.adv_mirq = AR9344_MISC_IRQ_SPI,
	},
#endif
	{
		.adv_name = NULL
	}
};

const struct atheros_platformsw ar9344_platformsw = {
	.apsw_intrsw = &atheros_intrsw,
	.apsw_intr_init = ar9344_intr_init,
	.apsw_cpu_intrnames = ar9344_cpu_intrnames,
	.apsw_misc_intrnames = ar9344_misc_intrnames,
	.apsw_cpu_nintrs = __arraycount(ar9344_cpu_intrnames),
	.apsw_misc_nintrs = __arraycount(ar9344_misc_intrnames),
	.apsw_cpuirq_misc = ARCHIP_CPU_IRQ_MISC,
	.apsw_ipl_sr_map = &ar9344_ipl_sr_map,

	.apsw_revision_id_addr = ARCHIP_RESET_BASE + ARCHIP_RESET_REVISION,
	.apsw_uart0_base = AR9344_UART0_BASE,
	.apsw_misc_intstat = ARCHIP_RESET_BASE + ARCHIP_RESET_MISC_INTSTAT,
	.apsw_misc_intmask = ARCHIP_RESET_BASE + ARCHIP_RESET_MISC_INTMASK,

	/*
	 * CPU specific routines.
	 */
	.apsw_get_memsize = ar9344_get_memsize,
	.apsw_wdog_reload = ar9344_wdog_reload,
	.apsw_bus_init = ar9344_bus_init,
	.apsw_reset = ar9344_reset,

	.apsw_get_freqs = ar9344_get_freqs,
	.apsw_device_register = ar9344_device_register,
	.apsw_enable_device = ar9344_enable_device,
	.apsw_devices = ar9344_devices,
};
