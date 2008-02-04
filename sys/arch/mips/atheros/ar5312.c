/* $NetBSD: ar5312.c,v 1.3.4.4 2008/02/04 09:22:11 yamt Exp $ */

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

#include "opt_memsize.h"
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/buf.h>

#include <mips/cache.h>
#include <mips/locore.h>
#include <mips/cpuregs.h>

#include <sys/socket.h>		/* these three just to get ETHER_ADDR_LEN(!) */
#include <net/if.h>
#include <net/if_ether.h>

#include <prop/proplib.h>

#include <mips/atheros/include/ar5312reg.h>
#include <mips/atheros/include/ar531xvar.h>
#include <mips/atheros/include/arbusvar.h>
#include "com.h"

uint32_t
ar531x_memsize(void)
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
	bank0 = (memcfg & AR5312_MEM_CFG1_BANK0_MASK) >>
	    AR5312_MEM_CFG1_BANK0_SHIFT;
	bank1 = (memcfg & AR5312_MEM_CFG1_BANK1_MASK) >>
	    AR5312_MEM_CFG1_BANK1_SHIFT;

	memsize = (bank0 ? (1 << (bank0 + 1)) : 0) +
	    (bank1 ? (1 << (bank1 + 1)) : 0);
	memsize <<= 20;
#endif

	return (memsize);
}

void
ar531x_wdog(uint32_t period)
{

	if (period == 0) {
		PUTSYSREG(AR5312_SYSREG_WDOG_CTL, AR5312_WDOG_CTL_IGNORE);
		PUTSYSREG(AR5312_SYSREG_WDOG_TIMER, 0);
	} else {
		PUTSYSREG(AR5312_SYSREG_WDOG_TIMER, period);
		PUTSYSREG(AR5312_SYSREG_WDOG_CTL, AR5312_WDOG_CTL_RESET);
	}
}

const char *
ar531x_cpuname(void)
{
	uint32_t	revision;

	revision = GETSYSREG(AR5312_SYSREG_REVISION);
	switch (AR5312_REVISION_MAJOR(revision)) {
	case AR5312_REVISION_MAJ_AR5311:
		return ("Atheros AR5311");
	case AR5312_REVISION_MAJ_AR5312:
		return ("Atheros AR5312");
	case AR5312_REVISION_MAJ_AR2313:
		return ("Atheros AR2313");
	case AR5312_REVISION_MAJ_AR5315:
		return ("Atheros AR5315");
	default:
		return ("Atheros AR531X");
	}
}

void
ar531x_businit(void)

{
	/*
	 * Clear previous AHB errors
	 */
	GETSYSREG(AR5312_SYSREG_AHBPERR);
	GETSYSREG(AR5312_SYSREG_AHBDMAE);
}

uint32_t
ar531x_cpu_freq(void)
{
	static uint32_t	cpufreq;
	uint32_t	wisoc = GETSYSREG(AR5312_SYSREG_REVISION);

	uint32_t	predivmask;
	uint32_t	predivshift;
	uint32_t	multmask;
	uint32_t	multshift;
	uint32_t	doublermask;
	uint32_t	divisor;
	uint32_t	multiplier;
	uint32_t	clockctl;

	const int	predivide_table[4] = { 1, 2, 4, 5 };

	/* XXX: in theory we might be able to get clock from bootrom */

	/*
	 * This logic looks at the clock control register and
	 * determines the actual CPU frequency.  These parts lack any
	 * kind of real-time clock on them, but the cpu clocks should
	 * be very accurate -- WiFi requires usec resolution timers.
	 */

	if (cpufreq) {
		return cpufreq;
	}

	if (AR5312_REVISION_MAJOR(wisoc) == AR5312_REVISION_MAJ_AR2313) {
		predivmask = AR2313_CLOCKCTL_PREDIVIDE_MASK;
		predivshift = AR2313_CLOCKCTL_PREDIVIDE_SHIFT;
		multmask = AR2313_CLOCKCTL_MULTIPLIER_MASK;
		multshift = AR2313_CLOCKCTL_MULTIPLIER_SHIFT;
		doublermask = AR2313_CLOCKCTL_DOUBLER_MASK;
	} else {
		predivmask = AR5312_CLOCKCTL_PREDIVIDE_MASK;
		predivshift = AR5312_CLOCKCTL_PREDIVIDE_SHIFT;
		multmask = AR5312_CLOCKCTL_MULTIPLIER_MASK;
		multshift = AR5312_CLOCKCTL_MULTIPLIER_SHIFT;
		doublermask = AR5312_CLOCKCTL_DOUBLER_MASK;
	}

	/*
	 * Note that the source clock involved here is a 40MHz.
	 */

	clockctl = GETSYSREG(AR5312_SYSREG_CLOCKCTL);
	divisor = predivide_table[(clockctl & predivmask) >> predivshift];
	multiplier = (clockctl & multmask) >> multshift;

	if (clockctl & doublermask)
		multiplier <<= 1;

	cpufreq = (40000000 / divisor) * multiplier;

	return (cpufreq);
}

uint32_t
ar531x_bus_freq(void)
{
	return (ar531x_cpu_freq() / 4);
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

		if (aa->aa_addr == AR5312_ENET0_BASE)
			enet = info->enet0Mac;
		else if (aa->aa_addr == AR5312_ENET1_BASE)
			enet = info->enet1Mac;
		else
			return;

		addprop_data(dev, "mac-addr", enet, ETHER_ADDR_LEN);
	}

	if (device_is_a(dev, "ath")) {
		const uint8_t *enet;

		if (aa->aa_addr == AR5312_WLAN0_BASE)
			enet = info->wlan0Mac;
		else if (aa->aa_addr == AR5312_WLAN1_BASE)
			enet = info->wlan1Mac;
		else
			return;

		addprop_data(dev, "mac-addr", enet, ETHER_ADDR_LEN);

		addprop_integer(dev, "wmac-rev",
		    AR5312_REVISION_WMAC(GETSYSREG(AR5312_SYSREG_REVISION)));

	}

	if (device_is_a(dev, "com")) {
		addprop_integer(dev, "frequency", ar531x_cpu_freq() / 4);
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

int
ar531x_enable_device(const struct ar531x_device *dev)
{
	const struct ar531x_boarddata *info;

	info = ar531x_board_info();
	if (dev->mask && ((dev->mask & info->config) == 0)) {
		return -1;
	}
	if (dev->reset) {
		/* put device into reset */
		PUTSYSREG(AR5312_SYSREG_RESETCTL,
		    GETSYSREG(AR5312_SYSREG_RESETCTL) | dev->reset);

		delay(15000);	/* XXX: tsleep? */

		/* take it out of reset */
		PUTSYSREG(AR5312_SYSREG_RESETCTL,
		    GETSYSREG(AR5312_SYSREG_RESETCTL) & ~dev->reset);

		delay(25);
	}
	if (dev->enable) {
		PUTSYSREG(AR5312_SYSREG_ENABLE,
		    GETSYSREG(AR5312_SYSREG_ENABLE) | dev->enable);
	}
	return 0;
}

const struct ar531x_device *
ar531x_get_devices(void)
{
	static const struct ar531x_device devices[] = {
		{
			"ae",
			AR5312_ENET0_BASE, 0x100000,
			AR5312_IRQ_ENET0, -1,
			AR5312_BOARD_CONFIG_ENET0,
			AR5312_RESET_ENET0 | AR5312_RESET_PHY0,
			AR5312_ENABLE_ENET0
		},
		{
			"ae",
			AR5312_ENET1_BASE, 0x100000,
			AR5312_IRQ_ENET1, -1,
			AR5312_BOARD_CONFIG_ENET1,
			AR5312_RESET_ENET1 | AR5312_RESET_PHY1,
			AR5312_ENABLE_ENET1
		},
		{
			"com",
			AR5312_UART0_BASE, 0x1000,
			AR5312_IRQ_MISC, AR5312_MISC_IRQ_UART0,
			AR5312_BOARD_CONFIG_UART0,
			0,
			0,
		},
		{
			"com",
			AR5312_UART1_BASE, 0x1000,
			-1, -1,
			AR5312_BOARD_CONFIG_UART1,
			0,
			0,
		},
		{
			"ath",
			AR5312_WLAN0_BASE, 0x100000,
			AR5312_IRQ_WLAN0, -1,
			AR5312_BOARD_CONFIG_WLAN0,
			AR5312_RESET_WLAN0 |
			AR5312_RESET_WARM_WLAN0_MAC |
			AR5312_RESET_WARM_WLAN0_BB,
			AR5312_ENABLE_WLAN0
		},
		{
			"ath",
			AR5312_WLAN1_BASE, 0x100000,
			AR5312_IRQ_WLAN1, -1,
			AR5312_BOARD_CONFIG_WLAN1,
			AR5312_RESET_WLAN1 |
			AR5312_RESET_WARM_WLAN1_MAC |
			AR5312_RESET_WARM_WLAN1_BB,
			AR5312_ENABLE_WLAN1
		},
		{
			"athflash",
			AR5312_FLASH_BASE, 0,
			-1, -1,
			0,
			0,
			0,
		},
		{
			"argpio", 0x1000,
			AR5312_GPIO_BASE,
			AR5312_IRQ_MISC, AR5312_MISC_IRQ_GPIO,
			0,
			0,
			0
		},
		{ NULL }
	};

	return devices;
}

