/* $NetBSD: sunxi_wdt.c,v 1.1.4.2 2017/08/28 17:51:32 skrll Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sunxi_wdt.c,v 1.1.4.2 2017/08/28 17:51:32 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/wdog.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/fdt/fdtvar.h>

#define	SUNXI_WDT_PERIOD_DEFAULT	16

#define	WDT_IRQ_EN_REG		0x00
#define	 WDT_IRQ_EN_EN		__BIT(0)
#define	WDT_IRQ_STA_REG		0x04
#define	 WDT_IRQ_STA_PEND	__BIT(0)
#define	WDT_CTRL_REG		0x10
#define	 WDT_CTRL_KEY_FIELD	__BITS(12,1)
#define	  WDT_CTRL_KEY_FIELD_V	0xa57
#define	 WDT_CTRL_RSTART	__BIT(0)
#define	WDT_CFG_REG		0x14
#define	 WDT_CFG_CONFIG		__BITS(1,0)
#define	  WDT_CFG_CONFIG_SYS	1
#define	  WDT_CFG_CONFIG_IRQ	2
#define	WDT_MODE_REG		0x18
#define	 WDT_MODE_INTV		__BITS(7,4)
#define	 WDT_MODE_EN		__BIT(0)

static const int sun6i_periods[] = {
	500, 1000, 2000, 3000,
	4000, 5000, 6000, 8000,
	10000, 12000, 14000, 16000,
	-1
};

static const struct of_compat_data compat_data[] = {
	{ "allwinner,sun6i-a31-wdt",	(uintptr_t)&sun6i_periods },
	{ NULL }
};

struct sunxi_wdt_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;

	const int *sc_periods;

	struct sysmon_wdog sc_smw;
};

#define WDT_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define WDT_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
sunxi_wdt_map_period(struct sunxi_wdt_softc *sc, u_int period,
    u_int *aperiod)
{
	const int *p = sc->sc_periods;
	int i;

	if (period == 0)
		return -1;

	for (i = 0; *p != -1; i++, p++)
		if (*p >= period * 1000) {
			*aperiod = *p / 1000;
			return i;
		}

	return -1;
}

static int
sunxi_wdt_setmode(struct sysmon_wdog *smw)
{
	struct sunxi_wdt_softc * const sc = smw->smw_cookie;
	uint32_t cfg, mode;
	int intv;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		WDT_WRITE(sc, WDT_MODE_REG, 0);
	} else {
		if (smw->smw_period == WDOG_PERIOD_DEFAULT)
			smw->smw_period = SUNXI_WDT_PERIOD_DEFAULT;
		intv = sunxi_wdt_map_period(sc, smw->smw_period,
		    &sc->sc_smw.smw_period);
		if (intv == -1)
			return EINVAL;

		cfg = __SHIFTIN(WDT_CFG_CONFIG_SYS, WDT_CFG_CONFIG);
		mode = WDT_MODE_EN | __SHIFTIN(intv, WDT_MODE_INTV);

		WDT_WRITE(sc, WDT_CFG_REG, cfg);
		WDT_WRITE(sc, WDT_MODE_REG, mode);
	}

	return 0;
}

static int
sunxi_wdt_tickle(struct sysmon_wdog *smw)
{
	struct sunxi_wdt_softc * const sc = smw->smw_cookie;
	const uint32_t ctrl = WDT_CTRL_RSTART |
	    __SHIFTIN(WDT_CTRL_KEY_FIELD_V, WDT_CTRL_KEY_FIELD);

	WDT_WRITE(sc, WDT_CTRL_REG, ctrl);

	return 0;
}

static int
sunxi_wdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
sunxi_wdt_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_wdt_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_periods = (void *)of_search_compatible(phandle, compat_data)->data;

	aprint_naive("\n");
	aprint_normal(": Watchdog\n");

	WDT_WRITE(sc, WDT_IRQ_EN_REG, 0);

	sc->sc_smw.smw_name = device_xname(self);
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = sunxi_wdt_setmode;
	sc->sc_smw.smw_tickle = sunxi_wdt_tickle;
	sc->sc_smw.smw_period = SUNXI_WDT_PERIOD_DEFAULT;

	aprint_normal_dev(self,
	    "default watchdog period is %u seconds\n",
	    sc->sc_smw.smw_period);

	if (sysmon_wdog_register(&sc->sc_smw) != 0) {
		aprint_error_dev(self,
		    "couldn't register with sysmon\n");
	}
}

CFATTACH_DECL_NEW(sunxi_wdt, sizeof(struct sunxi_wdt_softc),
	sunxi_wdt_match, sunxi_wdt_attach, NULL, NULL);
