/* $NetBSD: mesongx_wdt.c,v 1.3 2022/09/28 10:23:37 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: mesongx_wdt.c,v 1.3 2022/09/28 10:23:37 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/wdog.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/fdt/fdtvar.h>

#define	CBUS_REG(x)		((x) << 2)

#define	WATCHDOG_CNTL		CBUS_REG(0)
#define	 CNTL_CLK_DIV_EN		__BIT(25)
#define	 CNTL_CLK_EN			__BIT(24)
#define	 CNTL_SYS_RESET_N_EN		__BIT(21)
#define	 CNTL_WATCHDOG_EN		__BIT(18)
#define	 CNTL_CLK_DIV_TCNT		__BITS(17,0)
#define	WATCHDOG_CNTL1		CBUS_REG(1)
#define	WATCHDOG_TCNT		CBUS_REG(2)
#define	WATCHDOG_RESET		CBUS_REG(3)

#define	WATCHDOG_PERIOD_DEFAULT		8
#define	WATCHDOG_PERIOD_MAX		8

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "amlogic,meson-gx-wdt" },
	{ .compat = "amlogic,meson-gxbb-wdt" },
	DEVICE_COMPAT_EOL
};

struct mesongx_wdt_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct sysmon_wdog	sc_wdog;
	u_int			sc_rate;
};

#define	WDT_READ(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WDT_WRITE(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
mesongx_wdt_setmode(struct sysmon_wdog *smw)
{
	struct mesongx_wdt_softc * const sc = smw->smw_cookie;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		WDT_WRITE(sc, WATCHDOG_CNTL, 0);
		return 0;
	}

	if (smw->smw_period == WDOG_PERIOD_DEFAULT) {
		sc->sc_wdog.smw_period = WATCHDOG_PERIOD_DEFAULT;
	} else if (smw->smw_period == 0 ||
		   smw->smw_period > WATCHDOG_PERIOD_MAX) {
		return EINVAL;
	} else {
		sc->sc_wdog.smw_period = smw->smw_period;
	}

	const u_int tcnt = sc->sc_rate / 1000;

	WDT_WRITE(sc, WATCHDOG_CNTL, 0);
	WDT_WRITE(sc, WATCHDOG_RESET, 0);
	WDT_WRITE(sc, WATCHDOG_TCNT, sc->sc_wdog.smw_period * 1000);
	WDT_WRITE(sc, WATCHDOG_CNTL,
	    __SHIFTIN(tcnt, CNTL_CLK_DIV_TCNT) |
	    CNTL_CLK_DIV_EN | CNTL_CLK_EN |
	    CNTL_SYS_RESET_N_EN | CNTL_WATCHDOG_EN);

	return 0;
}

static int
mesongx_wdt_tickle(struct sysmon_wdog *smw)
{
	struct mesongx_wdt_softc * const sc = smw->smw_cookie;

	WDT_WRITE(sc, WATCHDOG_RESET, 0);

	return 0;
}

static int
mesongx_wdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
mesongx_wdt_attach(device_t parent, device_t self, void *aux)
{
	struct mesongx_wdt_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct clk *clk;
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

	aprint_naive("\n");
	aprint_normal(": EE-watchdog\n");

	clk = fdtbus_clock_get_index(phandle, 0);
	if (clk != NULL)
		sc->sc_rate = clk_get_rate(clk);
	else {
		aprint_error_dev(self, "WARNING: couldn't get xtal clock, assuming 24 MHz\n");
		sc->sc_rate = 24000000;
	}

	/* Disable watchdog */
	WDT_WRITE(sc, WATCHDOG_CNTL, 0);

	/* Register watchdog */
	sc->sc_wdog.smw_name = "EE-watchdog";
	sc->sc_wdog.smw_setmode = mesongx_wdt_setmode;
	sc->sc_wdog.smw_tickle = mesongx_wdt_tickle;
	sc->sc_wdog.smw_period = WATCHDOG_PERIOD_DEFAULT;
	sc->sc_wdog.smw_cookie = sc;
	sysmon_wdog_register(&sc->sc_wdog);
}

CFATTACH_DECL_NEW(mesongx_wdt, sizeof(struct mesongx_wdt_softc),
    mesongx_wdt_match, mesongx_wdt_attach, NULL, NULL);
