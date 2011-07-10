/* $NetBSD: ar5315.c,v 1.9 2011/07/10 06:24:18 matt Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: ar5315.c,v 1.9 2011/07/10 06:24:18 matt Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_memsize.h"

#define __INTR_PRIVATE

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
#include <mips/atheros/include/platform.h>
#include <mips/atheros/include/arbusvar.h>

#include <mips/locore.h>

/* helper macro for accessing system registers without bus space */
#define	REGVAL(x)	*((volatile uint32_t *)(MIPS_PHYS_TO_KSEG1((x))))
#define	GETSYSREG(x)	REGVAL((x) + AR5315_SYSREG_BASE)
#define	PUTSYSREG(x,v)	(REGVAL((x) + AR5315_SYSREG_BASE)) = (v)
#define	GETPCIREG(x)	REGVAL((x) + AR5315_PCI_BASE)
#define	PUTPCIREG(x,v)	(REGVAL((x) + AR5315_PCI_BASE)) = (v)
#define	GETSDRAMREG(x)	REGVAL((x) + AR5315_SDRAMCTL_BASE)

static uint32_t
ar5315_get_memsize(void)
{
#ifndef	MEMSIZE
	uint32_t	memsize = 0;
	uint32_t	memcfg, cw, rw, dw;

	/*
	 * Determine the memory size.  We query the board info.
	 */
	memcfg = GETSDRAMREG(AR5315_SDRAMCTL_MEM_CFG);
	cw = __SHIFTOUT(memcfg, AR5315_MEM_CFG_COL_WIDTH);
	cw += 1;
	rw = __SHIFTOUT(memcfg, AR5315_MEM_CFG_ROW_WIDTH);
	rw += 1;

	/* XXX: according to redboot, this could be wrong if DDR SDRAM */
	dw = __SHIFTOUT(memcfg, AR5315_MEM_CFG_DATA_WIDTH);
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

static void
ar5315_wdog_reload(uint32_t period)
{

	if (period == 0) {
		PUTSYSREG(AR5315_SYSREG_WDOG_CTL, AR5315_WDOG_CTL_IGNORE);
		PUTSYSREG(AR5315_SYSREG_WDOG_TIMER, 0);
	} else {
		PUTSYSREG(AR5315_SYSREG_WDOG_TIMER, period);
		PUTSYSREG(AR5315_SYSREG_WDOG_CTL, AR5315_WDOG_CTL_RESET);
	}
}

static void
ar5315_bus_init(void)
{
	/*
	 * Set CCA of KSEG0 access to 3 (actually any value other than
	 * 2 & 7 means that KSEG0 accesses are cached but 3 is standard
	 * value for writeback caching).
	 */
	mips3_cp0_config_write((mips3_cp0_config_read() & -8) | 3);

	PUTSYSREG(AR5315_SYSREG_AHB_ERR0, AR5315_AHB_ERROR_DET);
	GETSYSREG(AR5315_SYSREG_AHB_ERR1);
}

static void
ar5315_get_freqs(struct arfreqs *freqs) 
{
	static const uint8_t pll_divide_table[] = {
		2, 3, 4, 6, 3,
		/*
		 * these entries are bogus, but it avoids a possible
		 * bad table dereference
		 */
		1, 1, 1
	};
	static const uint8_t pre_divide_table[] = {
		1, 2, 4, 5
	};

	const uint32_t pllc = GETSYSREG(AR5315_SYSREG_PLLC_CTL);

	const uint32_t refdiv = pre_divide_table[AR5315_PLLC_REF_DIV(pllc)];
	const uint32_t fbdiv = AR5315_PLLC_FB_DIV(pllc);
	const uint32_t div2 = (AR5315_PLLC_DIV_2(pllc) + 1) * 2; /* results in 2 or 4 */

	freqs->freq_ref = 40000000;

	/* 40MHz reference clk, reference and feedback dividers */
	freqs->freq_pll = (freqs->freq_ref / refdiv) * div2 * fbdiv;

	const uint32_t pllout[4] = {
	    /* CLKM select */
	    [0] = freqs->freq_pll / pll_divide_table[AR5315_PLLC_CLKM(pllc)],
	    [1] = freqs->freq_pll / pll_divide_table[AR5315_PLLC_CLKM(pllc)],

	    /* CLKC select */
	    [2] = freqs->freq_pll / pll_divide_table[AR5315_PLLC_CLKC(pllc)],

	    /* ref_clk select */
	    [3] = freqs->freq_ref, /* use original reference clock */
	};

	const uint32_t amba_clkctl = GETSYSREG(AR5315_SYSREG_AMBACLK);
	uint32_t ambadiv = AR5315_CLOCKCTL_DIV(amba_clkctl);
	ambadiv = ambadiv ? (ambadiv * 2) : 1;
	freqs->freq_bus = pllout[AR5315_CLOCKCTL_SELECT(amba_clkctl)] / ambadiv;

	const uint32_t cpu_clkctl = GETSYSREG(AR5315_SYSREG_CPUCLK);
	uint32_t cpudiv = AR5315_CLOCKCTL_DIV(cpu_clkctl);
	cpudiv = cpudiv ? (cpudiv * 2) : 1;
	freqs->freq_cpu = pllout[AR5315_CLOCKCTL_SELECT(cpu_clkctl)] / cpudiv;

	freqs->freq_mem = 0;
}

static void
addprop_data(device_t dev, const char *name, const uint8_t *data,
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
addprop_integer(device_t dev, const char *name, uint32_t val)
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

static void
ar5315_device_register(struct device *dev, void *aux) 
{
	const struct arbus_attach_args * const aa = aux;
	const struct ar531x_boarddata * const info = atheros_get_board_info();

	if (device_is_a(dev, "com")) {
		addprop_integer(dev, "frequency", atheros_get_bus_freq());
	}

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

		addprop_data(dev, "mac-address", enet, ETHER_ADDR_LEN);
	}

	if (device_is_a(dev, "ath")) {
		const uint8_t *enet;

		if (aa->aa_addr == AR5315_WLAN_BASE)
			enet = info->wlan0Mac;
		else
			return;

		addprop_data(dev, "mac-address", enet, ETHER_ADDR_LEN);

		addprop_integer(dev, "wmac-rev",
		    GETSYSREG(AR5315_SYSREG_SREV));
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

static int
ar5315_enable_device(const struct atheros_device *adv)
{
	if (adv->adv_addr == AR5315_WLAN_BASE) {
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

static void
ar5315_intr_init(void)
{
	atheros_intr_init();
}

static void
ar5315_reset(void)
{
	PUTSYSREG(AR5315_SYSREG_COLDRESET,
	    AR5315_COLD_AHB | AR5315_COLD_APB | AR5315_COLD_CPU);
}

const static struct atheros_device ar5315_devices[] = {
	{
		.adv_name = "com",
		.adv_addr = AR5315_UART_BASE,
		.adv_size = 0x1000,
		.adv_cirq = AR5315_CPU_IRQ_MISC,
		.adv_mirq = AR5315_MISC_IRQ_UART,
	}, {
		.adv_name = "ae",
		.adv_addr = AR5315_ENET_BASE,
		.adv_size = 0x100000,
		.adv_cirq = AR5315_CPU_IRQ_ENET,
		.adv_mirq = -1,
	}, {
		.adv_name = "ath",
		.adv_addr = AR5315_WLAN_BASE,
		.adv_size = 0x100000,
		.adv_cirq = AR5315_CPU_IRQ_WLAN,
		.adv_mirq = -1,
	}, {
		.adv_name = "arspi",
		.adv_addr = AR5315_SPI_BASE,
		.adv_size = 0x10,
		.adv_cirq = AR5315_CPU_IRQ_MISC,
		.adv_mirq = AR5315_MISC_IRQ_SPI,
	}, {
		.adv_name = NULL
	}
};

static const struct ipl_sr_map ar5315_ipl_sr_map = {
    .sr_bits = {
	[IPL_NONE] =		0,
	[IPL_SOFTCLOCK] =	MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTBIO] =		MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET] =		MIPS_SOFT_INT_MASK,
	[IPL_SOFTSERIAL] =	MIPS_SOFT_INT_MASK,
	[IPL_VM] =		MIPS_SOFT_INT_MASK | MIPS_INT_MASK_0
				    | MIPS_INT_MASK_1 | MIPS_INT_MASK_2,
	[IPL_SCHED] =		MIPS_INT_MASK,
	[IPL_DDB] =		MIPS_INT_MASK,
	[IPL_HIGH] =		MIPS_INT_MASK,
    },
};

static const char * const ar5315_cpu_intrnames[] = {
	"int 0 (misc)",
	"int 1 (wlan)",
	"int 2 (enet)",
};

static const char * const ar5315_misc_intrnames[] = {
	"misc 0 (uart)",
	"misc 1 (i2c)",
	"misc 2 (spi)",
	"misc 3 (ahb error)",
	"misc 4 (apb error)",
	"misc 5 (timer)",
	"misc 6 (gpio)",
	"misc 7 (watchdog)",
	"misc 8 (ir)"
};

const struct atheros_platformsw ar5315_platformsw = {
	.apsw_intrsw = &atheros_intrsw,
	.apsw_intr_init = ar5315_intr_init,
	.apsw_cpu_intrnames = ar5315_cpu_intrnames,
	.apsw_misc_intrnames = ar5315_misc_intrnames,
	.apsw_cpu_nintrs = __arraycount(ar5315_cpu_intrnames),
	.apsw_misc_nintrs = __arraycount(ar5315_misc_intrnames),
	.apsw_cpuirq_misc = AR5315_CPU_IRQ_MISC,
	.apsw_ipl_sr_map = &ar5315_ipl_sr_map,

	.apsw_revision_id_addr = AR5315_SYSREG_BASE + AR5315_SYSREG_SREV,
	.apsw_uart0_base = AR5315_UART_BASE,
	.apsw_misc_intstat = AR5315_SYSREG_BASE + AR5315_SYSREG_MISC_INTSTAT,
	.apsw_misc_intmask = AR5315_SYSREG_BASE + AR5315_SYSREG_MISC_INTMASK,

	/*
	 * CPU specific routines.
	 */
	.apsw_get_memsize = ar5315_get_memsize,
	.apsw_wdog_reload = ar5315_wdog_reload,
	.apsw_bus_init = ar5315_bus_init,
	.apsw_reset = ar5315_reset,

	.apsw_get_freqs = ar5315_get_freqs,
	.apsw_device_register = ar5315_device_register,
	.apsw_enable_device = ar5315_enable_device,
	.apsw_devices = ar5315_devices,
};
