/* $NetBSD: ar5312.c,v 1.9 2012/10/27 17:18:01 chs Exp $ */

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
 * AR5312, which differents these from other members of the AR5315
 * family.
 */
#include "opt_ddb.h"
#include "opt_kgdb.h"
#define __INTR_PRIVATE

#include "opt_memsize.h"
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/buf.h>

#include <mips/cache.h>
#include <mips/locore.h>
#include <mips/cpuregs.h>

#include <sys/socket.h>		/* these three just to get ETHER_ADDR_LEN(!) */
#include <net/if.h>
#include <net/if_ether.h>

#include <prop/proplib.h>

#include <ah_soc.h>

#include <mips/atheros/include/platform.h>
#include <mips/atheros/include/arbusvar.h>
#include <mips/atheros/include/ar5312reg.h>
#include "com.h"

static uint32_t
ar5312_get_memsize(void)
{
	uint32_t memsize;
	uint32_t memcfg, bank0, bank1;

	/*
	 * Determine the memory size as established by system
	 * firmware.
	 *
	 * NB: we allow compile time override
	 */
#if defined(MEMSIZE)
	memsize = MEMSIZE;
#else
	memcfg = GETSDRAMREG(AR5312_SDRAMCTL_MEM_CFG1);
	bank0 = __SHIFTOUT(memcfg, AR5312_MEM_CFG1_BANK0);
	bank1 = __SHIFTOUT(memcfg, AR5312_MEM_CFG1_BANK1);

	memsize = (bank0 ? (1 << (bank0 + 1)) : 0) +
	    (bank1 ? (1 << (bank1 + 1)) : 0);
	memsize <<= 20;
#endif

	return (memsize);
}

static void
ar5312_wdog_reload(uint32_t period)
{

	if (period == 0) {
		PUTSYSREG(AR5312_SYSREG_WDOG_CTL, AR5312_WDOG_CTL_IGNORE);
		PUTSYSREG(AR5312_SYSREG_WDOG_TIMER, 0);
	} else {
		PUTSYSREG(AR5312_SYSREG_WDOG_TIMER, period);
		PUTSYSREG(AR5312_SYSREG_WDOG_CTL, AR5312_WDOG_CTL_RESET);
	}
}

static void
ar5312_bus_init(void)
{
	/*
	 * Clear previous AHB errors
	 */
	GETSYSREG(AR5312_SYSREG_AHBPERR);
	GETSYSREG(AR5312_SYSREG_AHBDMAE);
}

static void
ar5312_reset(void)
{
	PUTSYSREG(AR5312_SYSREG_RESETCTL, AR5312_RESET_SYSTEM);
}

static void
ar5312_get_freqs(struct arfreqs *freqs)
{
	const uint32_t	wisoc = GETSYSREG(AR5312_SYSREG_REVISION);

	uint32_t	predivisor;
	uint32_t	multiplier;

	/*
	 * This logic looks at the clock control register and
	 * determines the actual CPU frequency.  These parts lack any
	 * kind of real-time clock on them, but the cpu clocks should
	 * be very accurate -- WiFi requires usec resolution timers.
	 */

	const uint32_t clockctl = GETSYSREG(AR5312_SYSREG_CLOCKCTL);

	if (AR5312_REVISION_MAJOR(wisoc) == AR5312_REVISION_MAJ_AR2313) {
		predivisor = __SHIFTOUT(clockctl, AR2313_CLOCKCTL_PREDIVIDE);
		multiplier = __SHIFTOUT(clockctl, AR2313_CLOCKCTL_MULTIPLIER);
	} else {
		predivisor = __SHIFTOUT(clockctl, AR5312_CLOCKCTL_PREDIVIDE);
		multiplier = __SHIFTOUT(clockctl, AR5312_CLOCKCTL_MULTIPLIER);
		if (clockctl & AR5312_CLOCKCTL_DOUBLER)
			multiplier <<= 1;
	}

	/*
	 * Note that the source clock involved here is a 40MHz.
	 */

	const uint32_t divisor = (0x5421 >> (predivisor * 4)) & 15;

	const uint32_t cpufreq = (40000000 / divisor) * multiplier;

	freqs->freq_cpu = cpufreq;
	freqs->freq_bus = cpufreq / 4;
	freqs->freq_mem = 0;
	freqs->freq_ref = 40000000;
	freqs->freq_pll = 40000000;
}


static void
addprop_data(device_t dev, const char *name, const uint8_t *data, int len)
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
ar5312_device_register(device_t dev, void *aux) 
{
	const struct arbus_attach_args * const aa = aux;

	if (device_is_a(dev, "com")) {
		addprop_integer(dev, "frequency", atheros_get_bus_freq());
	}

	const struct ar531x_boarddata * const info = atheros_get_board_info();
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

		if (aa->aa_addr == AR5312_ENET0_BASE)
			enet = info->enet0Mac;
		else if (aa->aa_addr == AR5312_ENET1_BASE)
			enet = info->enet1Mac;
		else
			return;

		addprop_data(dev, "mac-address", enet, ETHER_ADDR_LEN);
	}

	if (device_is_a(dev, "ath")) {
		const uint8_t *enet;

		if (aa->aa_addr == AR5312_WLAN0_BASE)
			enet = info->wlan0Mac;
		else if (aa->aa_addr == AR5312_WLAN1_BASE)
			enet = info->wlan1Mac;
		else
			return;

		addprop_data(dev, "mac-address", enet, ETHER_ADDR_LEN);

		addprop_integer(dev, "wmac-rev",
		    AR5312_REVISION_WMAC(GETSYSREG(AR5312_SYSREG_REVISION)));

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
ar5312_enable_device(const struct atheros_device *adv)
{
	const struct ar531x_boarddata * const info = atheros_get_board_info();

	if (info != NULL
	    && adv->adv_mask && ((adv->adv_mask & info->config) == 0)) {
		return -1;
	}
	if (adv->adv_reset) {
		/* put device into reset */
		PUTSYSREG(AR5312_SYSREG_RESETCTL,
		    GETSYSREG(AR5312_SYSREG_RESETCTL) | adv->adv_reset);

		delay(15000);	/* XXX: tsleep? */

		/* take it out of reset */
		PUTSYSREG(AR5312_SYSREG_RESETCTL,
		    GETSYSREG(AR5312_SYSREG_RESETCTL) & ~adv->adv_reset);

		delay(25);
	}
	if (adv->adv_enable) {
		PUTSYSREG(AR5312_SYSREG_ENABLE,
		    GETSYSREG(AR5312_SYSREG_ENABLE) | adv->adv_enable);
	}
	return 0;
}

static void
ar5312_intr_init(void)
{
	atheros_intr_init();
}

static const struct atheros_device ar5312_devices[] = {
	{
		.adv_name = "ae",
		.adv_addr = AR5312_ENET0_BASE,
		.adv_size = 0x100000,
		.adv_cirq = AR5312_IRQ_ENET0,
		.adv_mirq = -1,
		.adv_mask = AR5312_BOARD_CONFIG_ENET0,
		.adv_reset = AR5312_RESET_ENET0 | AR5312_RESET_PHY0,
		.adv_enable = AR5312_ENABLE_ENET0
	}, {
		.adv_name = "ae",
		.adv_addr = AR5312_ENET1_BASE,
		.adv_size = 0x100000,
		.adv_cirq = AR5312_IRQ_ENET1,
		.adv_mirq = -1,
		.adv_mask = AR5312_BOARD_CONFIG_ENET1,
		.adv_reset = AR5312_RESET_ENET1 | AR5312_RESET_PHY1,
		.adv_enable = AR5312_ENABLE_ENET1
	}, {
		.adv_name = "com",
		.adv_addr = AR5312_UART0_BASE,
		.adv_size = 0x1000,
		.adv_cirq = AR5312_IRQ_MISC,
		.adv_mirq = AR5312_MISC_IRQ_UART0,
		.adv_mask = AR5312_BOARD_CONFIG_UART0,
	}, {
		.adv_name = "com",
		.adv_addr = AR5312_UART1_BASE,
		.adv_size = 0x1000,
		.adv_cirq = -1,
		.adv_mirq = -1,
		.adv_mask = AR5312_BOARD_CONFIG_UART1,
	}, {
		.adv_name = "ath",
		.adv_addr = AR5312_WLAN0_BASE,
		.adv_size = 0x100000,
		.adv_cirq = AR5312_IRQ_WLAN0,
		.adv_mirq = -1,
		.adv_mask = AR5312_BOARD_CONFIG_WLAN0,
		.adv_reset = AR5312_RESET_WLAN0 | AR5312_RESET_WARM_WLAN0_MAC
		    | AR5312_RESET_WARM_WLAN0_BB,
		.adv_enable = AR5312_ENABLE_WLAN0
	}, {
		.adv_name = "ath",
		.adv_addr = AR5312_WLAN1_BASE,
		.adv_size = 0x100000,
		.adv_cirq = AR5312_IRQ_WLAN1,
		.adv_mirq = -1,
		.adv_mask = AR5312_BOARD_CONFIG_WLAN1,
		.adv_reset = AR5312_RESET_WLAN1 | AR5312_RESET_WARM_WLAN1_MAC
		    | AR5312_RESET_WARM_WLAN1_BB,
		.adv_enable = AR5312_ENABLE_WLAN1
	}, {
		.adv_name = "athflash",
		.adv_addr = AR5312_FLASH_BASE,
		.adv_size = 0,
		.adv_cirq = -1,
		.adv_mirq = -1,
	}, {
		.adv_name = "argpio",
		.adv_addr = AR5312_GPIO_BASE,
		.adv_size = 0x1000,
		.adv_cirq = AR5312_IRQ_MISC,
		.adv_mirq = AR5312_MISC_IRQ_GPIO,
	}, {
		.adv_name = NULL
	}
};

static const struct ipl_sr_map ar5312_ipl_sr_map = {
    .sr_bits = {
	[IPL_NONE]		= 0,
	[IPL_SOFTCLOCK]		= MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTBIO]		= MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET]		= MIPS_SOFT_INT_MASK,
	[IPL_SOFTSERIAL]	= MIPS_SOFT_INT_MASK,
	[IPL_VM]		= MIPS_SOFT_INT_MASK | MIPS_INT_MASK_0
				    | MIPS_INT_MASK_1 | MIPS_INT_MASK_2
				    | MIPS_INT_MASK_3,
	[IPL_SCHED]		= MIPS_INT_MASK,
	[IPL_DDB]		= MIPS_INT_MASK,
	[IPL_HIGH]		= MIPS_INT_MASK,
    },
};

static const char * const ar5312_cpu_intrnames[] = {
	"int 0 (wlan0)",
	"int 1 (enet0)",
	"int 2 (enet1)",
	"int 3 (wlan1)",
	"int 4 (misc)",
	"int 5 (timer)",
};

static const char * const ar5312_misc_intrnames[] = {
	"misc 0 (timer)",
	"misc 1 (AHBproc error)",
	"misc 2 (AHBdma error)",
	"misc 3 (gpio)",
	"misc 4 (uart)",
	"misc 5 (uart dma)",
	"misc 6 (watchdog)"
};


const struct atheros_platformsw ar5312_platformsw = {
	.apsw_intrsw = &atheros_intrsw,
	.apsw_intr_init = ar5312_intr_init,
	.apsw_cpu_intrnames = ar5312_cpu_intrnames,
	.apsw_misc_intrnames = ar5312_misc_intrnames,
	.apsw_cpu_nintrs = __arraycount(ar5312_cpu_intrnames),
	.apsw_misc_nintrs = __arraycount(ar5312_misc_intrnames),
	.apsw_cpuirq_misc = AR5312_IRQ_MISC,
	.apsw_ipl_sr_map = &ar5312_ipl_sr_map,

	.apsw_revision_id_addr = AR5312_SYSREG_BASE + AR5312_SYSREG_REVISION,
	.apsw_uart0_base = AR5312_UART0_BASE,
	.apsw_misc_intstat = AR5312_SYSREG_BASE + AR5312_SYSREG_MISC_INTSTAT,
	.apsw_misc_intmask = AR5312_SYSREG_BASE + AR5312_SYSREG_MISC_INTMASK,

	/*
	 * CPU specific routines.
	 */
	.apsw_get_memsize = ar5312_get_memsize,
	.apsw_wdog_reload = ar5312_wdog_reload,
	.apsw_bus_init = ar5312_bus_init,
	.apsw_reset = ar5312_reset,

	.apsw_get_freqs = ar5312_get_freqs,
	.apsw_device_register = ar5312_device_register,
	.apsw_enable_device = ar5312_enable_device,
	.apsw_devices = ar5312_devices,
};
