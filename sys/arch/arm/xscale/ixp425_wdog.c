/*	$NetBSD: ixp425_wdog.c,v 1.1.56.1 2008/05/16 02:22:04 yamt Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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

#include "opt_ddb.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ixp425_wdog.c,v 1.1.56.1 2008/05/16 02:22:04 yamt Exp $");

#include <sys/systm.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/wdog.h>

#include <dev/clock_subr.h>
#include <dev/sysmon/sysmonvar.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <arm/xscale/ixp425reg.h>
#include <arm/xscale/ixp425var.h>
#include <arm/xscale/ixp425_sipvar.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

struct ixpdog_softc {
	struct device sc_dev;
	struct sysmon_wdog sc_smw;
	bus_space_tag_t sc_bust;
	bus_space_handle_t sc_bush;
	uint32_t sc_preset;
	int sc_armed;
};

#define	IXPDOG_DEFAULT_PERIOD	8
#ifndef	IXP425_CLOCK_FREQ
#define	IXPDOG_COUNTS_PER_SEC	66666600
#else
#define	IXPDOG_COUNTS_PER_SEC	IXP425_CLOCK_FREQ
#endif

static int ixpdog_match(struct device *, struct cfdata *, void *);
static void ixpdog_attach(struct device *, struct device *, void *);

CFATTACH_DECL(ixpdog, sizeof(struct ixpdog_softc),
    ixpdog_match, ixpdog_attach, NULL, NULL);

static void ixpdog_control(struct ixpdog_softc *, int);
static int ixpdog_tickle(struct sysmon_wdog *);
static int ixpdog_setmode(struct sysmon_wdog *);

#ifdef DDB
static struct ixpdog_softc *ixpdog_softc;
static void ixpdog_ddb_trap(int);
#endif


static int
ixpdog_match(struct device *parent, struct cfdata *match, void *arg)
{
	struct ixpsip_attach_args *sa = arg;

	return (sa->sa_addr == IXP425_OST_WDOG_HWBASE);
}

static void
ixpdog_attach(struct device *parent, struct device *self, void *arg)
{
	struct ixpdog_softc *sc = (struct ixpdog_softc *)self;
	struct ixpsip_attach_args *sa = arg;

	aprint_naive("\n");
	aprint_normal(": Watchdog Timer\n");

	sc->sc_bust = sa->sa_iot;

	if (bus_space_map(sc->sc_bust, sa->sa_addr, IXP425_OST_WDOG_SIZE, 0,
	    &sc->sc_bush)) {
		aprint_error("%s: Failed to map watchdog registers\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_preset = IXPDOG_COUNTS_PER_SEC * hz * IXPDOG_DEFAULT_PERIOD;
	sc->sc_armed = 0;
	ixpdog_control(sc, 0);

#ifdef DDB
	ixpdog_softc = sc;
	db_trap_callback = ixpdog_ddb_trap;
#endif

	sc->sc_smw.smw_name = sc->sc_dev.dv_xname;
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = ixpdog_setmode;
	sc->sc_smw.smw_tickle = ixpdog_tickle;
	sc->sc_smw.smw_period = IXPDOG_DEFAULT_PERIOD;

	if (sysmon_wdog_register(&sc->sc_smw) != 0)
		aprint_error("%s: unable to register watchdog with sysmon\n",
		    sc->sc_dev.dv_xname);
}

static void
ixpdog_control(struct ixpdog_softc *sc, int enable)
{
	uint32_t reg;
	int s;

	s = disable_interrupts(I32_bit);
	bus_space_write_2(sc->sc_bust, sc->sc_bush, IXP425_OST_WDOG_KEY,
	    OST_WDOG_KEY_MAJICK);
	reg = bus_space_read_4(sc->sc_bust, sc->sc_bush, IXP425_OST_WDOG_ENAB);

	if (!enable)
		reg &= ~(OST_WDOG_ENAB_CNT_ENA | OST_WDOG_ENAB_RST_ENA);
	else {
		bus_space_write_4(sc->sc_bust, sc->sc_bush, IXP425_OST_WDOG,
		    sc->sc_preset);
		reg |= OST_WDOG_ENAB_CNT_ENA | OST_WDOG_ENAB_RST_ENA;
	}

	bus_space_write_4(sc->sc_bust, sc->sc_bush, IXP425_OST_WDOG_ENAB, reg);
	bus_space_write_2(sc->sc_bust, sc->sc_bush, IXP425_OST_WDOG_KEY,
	    ~OST_WDOG_KEY_MAJICK);
	restore_interrupts(s);
}

static int
ixpdog_tickle(struct sysmon_wdog *smw)
{
	struct ixpdog_softc *sc = smw->smw_cookie;
	int s;

	s = disable_interrupts(I32_bit);
	bus_space_write_2(sc->sc_bust, sc->sc_bush, IXP425_OST_WDOG_KEY,
	    OST_WDOG_KEY_MAJICK);
	bus_space_write_4(sc->sc_bust, sc->sc_bush, IXP425_OST_WDOG,
	    sc->sc_preset);
	bus_space_write_2(sc->sc_bust, sc->sc_bush, IXP425_OST_WDOG_KEY,
	    ~OST_WDOG_KEY_MAJICK);
	restore_interrupts(s);

	return (0);
}

static int
ixpdog_setmode(struct sysmon_wdog *smw)
{
	struct ixpdog_softc *sc = smw->smw_cookie;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED)
		sc->sc_armed = 0;
	else {
		if (smw->smw_period == WDOG_PERIOD_DEFAULT)
			smw->smw_period = IXPDOG_DEFAULT_PERIOD;
		sc->sc_preset = IXPDOG_COUNTS_PER_SEC * hz * smw->smw_period;
		if (sc->sc_preset < (IXPDOG_COUNTS_PER_SEC * hz) ||
		    sc->sc_preset < smw->smw_period)
			return (EOPNOTSUPP);

		sc->sc_armed = 1;
	}

	ixpdog_control(sc, sc->sc_armed);

	return (0);
}

#ifdef DDB
static void
ixpdog_ddb_trap(int enter)
{
	struct ixpdog_softc *sc;

	if ((sc = ixpdog_softc) == NULL)
		return;

	sc->sc_armed += enter ? (-1) : 1;

	if ((enter && sc->sc_armed == 0) || (!enter && sc->sc_armed == 1))
		ixpdog_control(sc, sc->sc_armed);
}
#endif
