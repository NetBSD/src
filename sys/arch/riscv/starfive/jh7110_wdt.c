/* $NetBSD: jh7110_wdt.c,v 1.1 2026/08/25 20:46:17 riz Exp $ */

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jeffrey C. Rizzo
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: jh7110_wdt.c,v 1.1 2026/08/25 20:46:17 riz Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/wdog.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/fdt/fdtvar.h>

/*
 * The JH7110 WDT appears to be a clone of the ARM SP805 watchdog module.
 * 
 * Register definitions and operating principles were taken from the
 * ARM Watchdog Module (SP805) Technical Reference Manual, retrieved from
 * https://community.arm.com/cfs-file/__key/communityserver-discussions-components-files/468/DDI0270-_2800_4_2900_-2-_2800_1_2900_.pdf
 *
 */

#define JH7110_WDT_PERIOD_DEFAULT	15

#define JH7110_WDT_LOAD		0x00
#define JH7110_WDT_VALUE	0x04
#define JH7110_WDT_CONTROL	0x08
#define  JH7110_WDT_CONTROL_INTEN		__BIT(0)
#define  JH7110_WDT_CONTROL_RESEN		__BIT(1)

#define JH7110_WDT_INTCLR	0x0c
#define JH7110_WDT_MIS		0x14

#define JH7110_WDT_LOCK		0xc00
#define JH7110_WDT_UNLOCK_KEY	0x1ACCE551

#define JH7110_WDT_PERIPHID0	0xfe0
#define JH7110_WDT_PERIPHID1	0xfe4
#define JH7110_WDT_PERIPHID2	0xfe8
#define JH7110_WDT_PERIPHID3	0xfec
#define JH7110_WDT_PCELLID0	0xff0
#define JH7110_WDT_PCELLID1	0xff4
#define JH7110_WDT_PCELLID2	0xff8
#define JH7110_WDT_PCELLID3	0xffc

enum {
	JH7110_WDT_CLK_APB = 0,
	JH7110_WDT_CLK_CORE = 1,
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "starfive,jh7110-wdt" },
	DEVICE_COMPAT_EOL
};

struct jh7110_wdt_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;

	struct sysmon_wdog sc_smw;
	u_int sc_rate;
};

static inline uint32_t
jh7110_wdt_read(struct jh7110_wdt_softc *sc, bus_size_t reg)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, reg);
}

static inline void
jh7110_wdt_write(struct jh7110_wdt_softc *sc, bus_size_t reg, uint32_t val)
{
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, reg, val);
}

static void
jh7110_wdt_stop(struct jh7110_wdt_softc *sc)
{
	jh7110_wdt_write(sc, JH7110_WDT_LOCK, JH7110_WDT_UNLOCK_KEY);

	uint32_t val = jh7110_wdt_read(sc, JH7110_WDT_CONTROL);
	jh7110_wdt_write(sc, JH7110_WDT_CONTROL, val & ~JH7110_WDT_CONTROL_INTEN);

	jh7110_wdt_write(sc, JH7110_WDT_LOCK, 1);
}

static void
jh7110_wdt_start(struct jh7110_wdt_softc *sc, uint32_t count)
{
	jh7110_wdt_write(sc, JH7110_WDT_LOCK, JH7110_WDT_UNLOCK_KEY);

	uint32_t val = jh7110_wdt_read(sc, JH7110_WDT_CONTROL);
	jh7110_wdt_write(sc, JH7110_WDT_CONTROL, val & ~JH7110_WDT_CONTROL_INTEN);
	val = jh7110_wdt_read(sc, JH7110_WDT_CONTROL);
	jh7110_wdt_write(sc, JH7110_WDT_CONTROL, val | JH7110_WDT_CONTROL_RESEN);
	jh7110_wdt_write(sc, JH7110_WDT_INTCLR, 1);
	jh7110_wdt_write(sc, JH7110_WDT_LOAD, count);
	val = jh7110_wdt_read(sc, JH7110_WDT_CONTROL);
	jh7110_wdt_write(sc, JH7110_WDT_CONTROL, val | JH7110_WDT_CONTROL_INTEN);

	jh7110_wdt_write(sc, JH7110_WDT_LOCK, 1);
}

static int
jh7110_wdt_setmode(struct sysmon_wdog *smw)
{
	struct jh7110_wdt_softc * const sc = smw->smw_cookie;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		jh7110_wdt_stop(sc);
		return 0;
	}

	if (smw->smw_period == WDOG_PERIOD_DEFAULT)
		sc->sc_smw.smw_period = JH7110_WDT_PERIOD_DEFAULT;
	else
		sc->sc_smw.smw_period = smw->smw_period;

	if (sc->sc_smw.smw_period == 0) {
		jh7110_wdt_stop(sc);
	} else {
		/* wdt counts down twice before resetting, so halve rate */
		uint32_t counter = sc->sc_smw.smw_period * sc->sc_rate / 2;
		jh7110_wdt_start(sc, (uint32_t)counter);
	}

	return 0;
}

static int
jh7110_wdt_tickle(struct sysmon_wdog *smw)
{
	struct jh7110_wdt_softc * const sc = smw->smw_cookie;

	jh7110_wdt_write(sc, JH7110_WDT_LOCK, JH7110_WDT_UNLOCK_KEY);

	jh7110_wdt_write(sc, JH7110_WDT_INTCLR, 1);

	jh7110_wdt_write(sc, JH7110_WDT_LOCK, 1);
	return 0;
}

static int
jh7110_wdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
jh7110_wdt_attach(device_t parent, device_t self, void *aux)
{
	struct jh7110_wdt_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#" PRIxBUSADDR ": %d", addr,
		    error);
		return;
	}

	struct {
		const char *jwc_name;
		u_int jwc_rate;
	} jh7110_wdt_clk[] = {
		[JH7110_WDT_CLK_APB] = {
			.jwc_name = "apb",
		},
		[JH7110_WDT_CLK_CORE] = {
			.jwc_name = "core",
		},
	};
	for (size_t i = 0; i < __arraycount(jh7110_wdt_clk); i++) {
		const char *cr = jh7110_wdt_clk[i].jwc_name;

		struct clk *clk = fdtbus_clock_get(phandle, cr);
		if (clk == NULL) {
			aprint_error(": couldn't get clock '%s'\n", cr);
			return;
		}
		error = clk_enable(clk);
		if (error) {
			aprint_error(": couldn't enable clock '%s'\n", cr);
			return;
		}
		u_int rate = clk_get_rate(clk);
		if (rate == 0) {
			aprint_error(": couldn't get rate for clock '%s'\n", cr);
			return;
		}
		jh7110_wdt_clk[i].jwc_rate = rate;
	}

	/* de-assert resets */
	struct fdtbus_reset *rst;
	for (u_int n = 0; (rst = fdtbus_reset_get_index(phandle, n)) != NULL; n++) {
		if (fdtbus_reset_deassert(rst) != 0) {
			aprint_error(": couldn't de-assert reset #%d\n", n);
			return;
		}
	}

	sc->sc_rate = jh7110_wdt_clk[JH7110_WDT_CLK_CORE].jwc_rate;
	
	aprint_naive("\n");
	aprint_normal(": watchdog\n");

	sc->sc_smw.smw_name = device_xname(self);
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = jh7110_wdt_setmode;
	sc->sc_smw.smw_tickle = jh7110_wdt_tickle;
	sc->sc_smw.smw_period = JH7110_WDT_PERIOD_DEFAULT;
	sysmon_wdog_register(&sc->sc_smw);
}

CFATTACH_DECL_NEW(jh7110_wdt, sizeof(struct jh7110_wdt_softc),
	jh7110_wdt_match, jh7110_wdt_attach, NULL, NULL);
