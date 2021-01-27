/* $NetBSD: ti_wdt.c,v 1.2 2021/01/27 03:10:20 thorpej Exp $ */

/*-
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: ti_wdt.c,v 1.2 2021/01/27 03:10:20 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/wdog.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/fdt/fdtvar.h>

#include <arm/ti/ti_prcm.h>

#define	WDT_WDSC		0x10
#define	 WDSC_SOFTRESET		__BIT(1)
#define	WDT_WDST		0x14
#define	WDT_WISR		0x18
#define	WDT_WIER		0x1c
#define	WDT_WCLR		0x24
#define	 WCLR_PRE		__BIT(5)
#define	 WCLR_PTV		__BITS(4,2)
#define	WDT_WCRR		0x28
#define	WDT_WLDR		0x2c
#define	WDT_WTGR		0x30
#define	WDT_WWPS		0x34
#define	 WWPS_W_PEND_WDLY	__BIT(5)
#define	 WWPS_W_PEND_WSPR	__BIT(4)
#define	 WWPS_W_PEND_WTGR	__BIT(3)
#define	 WWPS_W_PEND_WLDR	__BIT(2)
#define	 WWPS_W_PEND_WCRR	__BIT(1)
#define	 WWPS_W_PEND_WCLR	__BIT(0)
#define	 WWPS_W_PEND_MASK	__BITS(5,0)
#define	WDT_WDLY		0x44
#define	WDT_WSPR		0x48
#define	WDT_WIRQSTATRAW		0x54
#define	WDT_WIRQSTAT		0x58
#define	WDT_WIRQENSET		0x5c
#define	WDT_WIRQENCLR		0x60
#define	 WIRQ_EVENT_DLY		__BIT(1)
#define	 WIRQ_EVENT_OVF		__BIT(0)

#define	WATCHDOG_PERIOD_DEFAULT		10

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,omap3-wdt" },
	DEVICE_COMPAT_EOL
};

struct ti_wdt_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct sysmon_wdog	sc_wdog;
	u_int			sc_rate;
};

#define	RD4(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WR4(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static void
ti_wdt_sync(struct ti_wdt_softc *sc, uint32_t mask)
{
	uint32_t val;
	int retry;

	for (retry = 10000; retry > 0; retry--) {
		val = RD4(sc, WDT_WWPS);
		if ((val & mask) == 0)
			return;
	}

	aprint_error_dev(sc->sc_dev,
	    "reg sync timeout, mask=%#x, wwps=%#x\n", mask, val);
}

static void
ti_wdt_sync_all(struct ti_wdt_softc *sc)
{
	ti_wdt_sync(sc, WWPS_W_PEND_MASK);
}

static int
ti_wdt_reset(struct ti_wdt_softc *sc)
{
	uint32_t val;
	int retry;

	val = RD4(sc, WDT_WDSC);
	val |= WDSC_SOFTRESET;
	WR4(sc, WDT_WDSC, val);
	for (retry = 10000; retry > 0; retry--) {
		val = RD4(sc, WDT_WDSC);
		if ((val & WDSC_SOFTRESET) == 0)
			return 0;
		delay(10);
	}

	return EIO;
}

static void
ti_wdt_stop(struct ti_wdt_softc *sc)
{
	WR4(sc, WDT_WSPR, 0xaaaa);
	ti_wdt_sync(sc, WWPS_W_PEND_WSPR);
	WR4(sc, WDT_WSPR, 0x5555);
	ti_wdt_sync(sc, WWPS_W_PEND_WSPR);
}

static void
ti_wdt_start(struct ti_wdt_softc *sc)
{
	WR4(sc, WDT_WSPR, 0xbbbb);
	ti_wdt_sync(sc, WWPS_W_PEND_WSPR);
	WR4(sc, WDT_WSPR, 0x4444);
	ti_wdt_sync(sc, WWPS_W_PEND_WSPR);
}

static int
ti_wdt_setmode(struct sysmon_wdog *smw)
{
	struct ti_wdt_softc * const sc = smw->smw_cookie;
	uint32_t counter_val;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		ti_wdt_stop(sc);
		return 0;
	}

	if (smw->smw_period == WDOG_PERIOD_DEFAULT)
		sc->sc_wdog.smw_period = WATCHDOG_PERIOD_DEFAULT;
	else
		sc->sc_wdog.smw_period = smw->smw_period;

	if (sc->sc_wdog.smw_period == 0)
		counter_val = ~0u;
	else
		counter_val = ~(sc->sc_wdog.smw_period * sc->sc_rate / 2);

	ti_wdt_stop(sc);
	ti_wdt_sync_all(sc);

	WR4(sc, WDT_WCLR, WCLR_PRE | __SHIFTIN(1, WCLR_PTV));
	WR4(sc, WDT_WLDR, counter_val);
	WR4(sc, WDT_WCRR, counter_val);

	ti_wdt_sync_all(sc);

	ti_wdt_start(sc);

	return 0;
}

static int
ti_wdt_tickle(struct sysmon_wdog *smw)
{
	struct ti_wdt_softc * const sc = smw->smw_cookie;
	uint32_t val;

	ti_wdt_sync_all(sc);
	val = RD4(sc, WDT_WTGR);
	WR4(sc, WDT_WTGR, ~val);

	return 0;
}

static int
ti_wdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
ti_wdt_attach(device_t parent, device_t self, void *aux)
{
	struct ti_wdt_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct clk *clk;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	clk = ti_prcm_get_hwmod(phandle, 0);
	if (clk == NULL || clk_enable(clk) != 0) {
		aprint_error(": couldn't enable hwmod\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_rate = clk_get_rate(clk);

	aprint_naive("\n");
	aprint_normal(": WATCHDOG\n");

	/* Software reset */
	if (ti_wdt_reset(sc) != 0) {
		aprint_error_dev(self, "software reset timeout\n");
		return;
	}

	/* Stop the watchdog */
	ti_wdt_stop(sc);

	/* Register watchdog */
	sc->sc_wdog.smw_name = device_xname(self);
	sc->sc_wdog.smw_setmode = ti_wdt_setmode;
	sc->sc_wdog.smw_tickle = ti_wdt_tickle;
	sc->sc_wdog.smw_period = WATCHDOG_PERIOD_DEFAULT;
	sc->sc_wdog.smw_cookie = sc;
	sysmon_wdog_register(&sc->sc_wdog);
}

CFATTACH_DECL_NEW(ti_wdt, sizeof(struct ti_wdt_softc),
    ti_wdt_match, ti_wdt_attach, NULL, NULL);
