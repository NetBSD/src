/* $NetBSD: meson_wdt.c,v 1.2 2021/01/27 03:10:18 thorpej Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: meson_wdt.c,v 1.2 2021/01/27 03:10:18 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/wdog.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/fdt/fdtvar.h>

#define	CBUS_REG(x)		((x) << 2)

#define	WATCHDOG_TC_REG		CBUS_REG(0)
#define	 WATCHDOG_TC_CPUS	__BITS(27,24)
#define	 WATCHDOG_TC_ENABLE	__BIT(19)
#define	 WATCHDOG_TC_TCNT	__BITS(15,0)

#define	WATCHDOG_RESET_REG	CBUS_REG(1)
#define	 WATCHDOG_RESET_COUNT	__BITS(15,0)

#define	WATCHDOG_PERIOD_DEFAULT		8
#define	WATCHDOG_PERIOD_MAX		8
#define	WATCHDOG_TICKS_PER_SEC		7812

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "amlogic,meson8b-wdt" },
	DEVICE_COMPAT_EOL
};

struct meson_wdt_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct sysmon_wdog	sc_wdog;
};

#define	WDT_READ(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WDT_WRITE(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
meson_wdt_setmode(struct sysmon_wdog *smw)
{
	struct meson_wdt_softc * const sc = smw->smw_cookie;
	uint32_t val;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		WDT_WRITE(sc, WATCHDOG_RESET_REG, 0);
		val = WDT_READ(sc, WATCHDOG_TC_REG);
		val &= ~WATCHDOG_TC_ENABLE;
		WDT_WRITE(sc, WATCHDOG_TC_REG, val);
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

	const u_int tcnt = sc->sc_wdog.smw_period * WATCHDOG_TICKS_PER_SEC;
	WDT_WRITE(sc, WATCHDOG_RESET_REG, 0);
	WDT_WRITE(sc, WATCHDOG_TC_REG, WATCHDOG_TC_CPUS | WATCHDOG_TC_ENABLE |
	    __SHIFTIN(tcnt, WATCHDOG_TC_TCNT));

	return 0;
}

static int
meson_wdt_tickle(struct sysmon_wdog *smw)
{
	struct meson_wdt_softc * const sc = smw->smw_cookie;

	WDT_WRITE(sc, WATCHDOG_RESET_REG, 0);

	return 0;
}

static int
meson_wdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
meson_wdt_attach(device_t parent, device_t self, void *aux)
{
	struct meson_wdt_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	uint32_t val;

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

	/* Disable watchdog */
	WDT_WRITE(sc, WATCHDOG_RESET_REG, 0);
	val = WDT_READ(sc, WATCHDOG_TC_REG);
	val &= ~WATCHDOG_TC_ENABLE;
	WDT_WRITE(sc, WATCHDOG_TC_REG, val);

	/* Register watchdog */
	sc->sc_wdog.smw_name = "EE-watchdog";
	sc->sc_wdog.smw_setmode = meson_wdt_setmode;
	sc->sc_wdog.smw_tickle = meson_wdt_tickle;
	sc->sc_wdog.smw_period = WATCHDOG_PERIOD_DEFAULT;
	sc->sc_wdog.smw_cookie = sc;
	sysmon_wdog_register(&sc->sc_wdog);
}

CFATTACH_DECL_NEW(meson_wdt, sizeof(struct meson_wdt_softc),
    meson_wdt_match, meson_wdt_attach, NULL, NULL);
