/* $NetBSD: timekeeper.c,v 1.10.12.3 2017/12/03 11:36:23 jdolecek Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: timekeeper.c,v 1.10.12.3 2017/12/03 11:36:23 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <machine/cpu.h>

#include <dev/clock_subr.h>
#include <luna68k/dev/timekeeper.h>
#include <machine/autoconf.h>

#include "ioconf.h"

#define	YEAR0	1970	/* year offset */

struct timekeeper_softc {
	device_t sc_dev;
	void *sc_clock, *sc_nvram;
	int sc_nvramsize;
	uint8_t sc_image[2040];
	struct todr_chip_handle sc_todr;
};

static int  clock_match(device_t, cfdata_t, void *);
static void clock_attach(device_t, device_t, void *);
static void dsclock_init(struct timekeeper_softc *);

CFATTACH_DECL_NEW(clock, sizeof (struct timekeeper_softc),
    clock_match, clock_attach, NULL, NULL);

static int mkclock_get(todr_chip_handle_t, struct clock_ymdhms *);
static int mkclock_set(todr_chip_handle_t, struct clock_ymdhms *);
static int dsclock_get(todr_chip_handle_t, struct clock_ymdhms *);
static int dsclock_set(todr_chip_handle_t, struct clock_ymdhms *);

static int
clock_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, clock_cd.cd_name))
		return 0;
	return 1;
}

static void
clock_attach(device_t parent, device_t self, void *aux)
{
	struct timekeeper_softc *sc = device_private(self);
	struct mainbus_attach_args *ma = aux;

	sc->sc_dev = self;

	switch (machtype) {
	default:
	case LUNA_I:	/* Mostek MK48T02 */
		sc->sc_clock = (void *)(ma->ma_addr + 2040);
		sc->sc_nvram = (void *)ma->ma_addr;
		sc->sc_nvramsize = 2040;
		sc->sc_todr.todr_gettime_ymdhms = mkclock_get;
		sc->sc_todr.todr_settime_ymdhms = mkclock_set;
		sc->sc_todr.cookie = sc;
		aprint_normal(": mk48t02\n");
		break;
	case LUNA_II: /* Dallas DS1287A */
		sc->sc_clock = (void *)ma->ma_addr;
		sc->sc_nvram = (void *)(ma->ma_addr + MC_NREGS);
		sc->sc_nvramsize = 50;
		sc->sc_todr.todr_gettime_ymdhms = dsclock_get;
		sc->sc_todr.todr_settime_ymdhms = dsclock_set;
		sc->sc_todr.cookie = sc;
		dsclock_init(sc);
		aprint_normal(": ds1287a\n");
		break;
	}
	todr_attach(&sc->sc_todr);
	memcpy(sc->sc_image, sc->sc_nvram, sc->sc_nvramsize);
}

/*
 * Get the time of day, based on the clock's value and/or the base value.
 */
static int
mkclock_get(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	struct timekeeper_softc *sc = (void *)tch->cookie;
	volatile uint8_t *chiptime = (void *)sc->sc_clock;
	int s;

	s = splclock();
	chiptime[MK_CSR] |= MK_CSR_READ;	/* enable read (stop time) */
	dt->dt_sec = bcdtobin(chiptime[MK_SEC]);
	dt->dt_min = bcdtobin(chiptime[MK_MIN]);
	dt->dt_hour = bcdtobin(chiptime[MK_HOUR]);
	dt->dt_wday = bcdtobin(chiptime[MK_DOW]);
	dt->dt_day = bcdtobin(chiptime[MK_DOM]);
	dt->dt_mon = bcdtobin(chiptime[MK_MONTH]);
	dt->dt_year = bcdtobin(chiptime[MK_YEAR]) + YEAR0;
	chiptime[MK_CSR] &= ~MK_CSR_READ;	/* time wears on */
	splx(s);
	return 0;
}

/*
 * Reset the TODR based on the time value.
 */
static int
mkclock_set(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	struct timekeeper_softc *sc = (void *)tch->cookie;
	volatile uint8_t *chiptime = (void *)sc->sc_clock;
	volatile uint8_t *stamp = (uint8_t *)sc->sc_nvram + 0x10;
	int s;

	s = splclock();
	chiptime[MK_CSR] |= MK_CSR_WRITE;	/* enable write */
	chiptime[MK_SEC] = bintobcd(dt->dt_sec);
	chiptime[MK_MIN] = bintobcd(dt->dt_min);
	chiptime[MK_HOUR] = bintobcd(dt->dt_hour);
	chiptime[MK_DOW] = bintobcd(dt->dt_wday);
	chiptime[MK_DOM] = bintobcd(dt->dt_day);
	chiptime[MK_MONTH] = bintobcd(dt->dt_mon);
	chiptime[MK_YEAR] = bintobcd(dt->dt_year - YEAR0);
	chiptime[MK_CSR] &= ~MK_CSR_WRITE;	/* load them up */
	splx(s);

	stamp[0] = 'R'; stamp[1] = 'T'; stamp[2] = 'C'; stamp[3] = '\0';
	return 0;
}

static void
dsclock_init(struct timekeeper_softc *sc)
{
	volatile uint8_t *chiptime = (void *)sc->sc_clock;

	/*
	 * It looks the firmware ROM doesn't initialize DS1287 at all
	 * even after the chip is replaced, so explicitly initialize
	 * control registers here.
	 */
	chiptime = (void *)sc->sc_clock;

	/* No DSE, 24HR, BINARY */
	chiptime[MC_REGB] =
	    (chiptime[MC_REGB] & ~MC_REGB_DSE) |
	    (MC_REGB_24HR | MC_REGB_BINARY);

	/* make sure to start integrated clock OSC */
	chiptime[MC_REGA] =
	    (chiptime[MC_REGA] & ~MC_REGA_DVMASK) | MC_BASE_32_KHz;
}

/*
 * Get the time of day, based on the clock's value and/or the base value.
 */
static int
dsclock_get(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	struct timekeeper_softc *sc = (void *)tch->cookie;
	volatile uint8_t *chiptime = (void *)sc->sc_clock;
	int s;

	s = splclock();
	/* update in progress; spin loop */
	while (chiptime[MC_REGA] & MC_REGA_UIP)
		continue;
	dt->dt_sec = chiptime[MC_SEC];
	dt->dt_min = chiptime[MC_MIN];
	dt->dt_hour = chiptime[MC_HOUR];
	dt->dt_wday = chiptime[MC_DOW];
	dt->dt_day = chiptime[MC_DOM];
	dt->dt_mon = chiptime[MC_MONTH];
	dt->dt_year = chiptime[MC_YEAR] + YEAR0;
	splx(s);
	return 0;
}

/*
 * Reset the TODR based on the time value.
 */
static int
dsclock_set(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	struct timekeeper_softc *sc = (void *)tch->cookie;
	volatile uint8_t *chiptime = (void *)sc->sc_clock;
	int s;

	s = splclock();
	chiptime[MC_REGB] |= MC_REGB_SET;	/* enable write */
	chiptime[MC_SEC] = dt->dt_sec;
	chiptime[MC_MIN] = dt->dt_min;
	chiptime[MC_HOUR] = dt->dt_hour;
	chiptime[MC_DOW] = dt->dt_wday;
	chiptime[MC_DOM] = dt->dt_day;
	chiptime[MC_MONTH] = dt->dt_mon;
	chiptime[MC_YEAR] = dt->dt_year - YEAR0;
	chiptime[MC_REGB] &= ~MC_REGB_SET;	/* load them up */
	splx(s);
	return 0;
}
