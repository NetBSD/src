/*	$NetBSD: rtc.c,v 1.19 2019/11/10 21:16:32 chs Exp $ */

/*
 * Copyright (c) 2001 Valeriy E. Ushakov
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * `rtc' is a DS1287A (== MC146818A) time-of-day clock at EBus.
 * In Krups it's not used to store idprom so this driver doesn't
 * support it.  Don't know about other ms-IIep systems.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rtc.c,v 1.19 2019/11/10 21:16:32 chs Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#ifdef GPROF
#include <sys/gmon.h>
#endif

#include <sys/bus.h>
#include <machine/autoconf.h>

#include <dev/clock_subr.h>
#include <dev/ic/mc146818reg.h>

#include <dev/ebus/ebusreg.h>
#include <dev/ebus/ebusvar.h>

struct rtc_ebus_softc {
	bus_space_tag_t		sc_bt;	/* parent bus tag */
	bus_space_handle_t	sc_bh;	/* handle for registers */
};

static int	rtcmatch_ebus(device_t, cfdata_t, void *);
static void	rtcattach_ebus(device_t, device_t, void *);

CFATTACH_DECL_NEW(rtc_ebus, sizeof(struct rtc_ebus_softc),
    rtcmatch_ebus, rtcattach_ebus, NULL, NULL);

/* XXX: global TOD clock handle (sparc/clock.c) */
extern todr_chip_handle_t todr_handle;

/* todr(9) methods */
static int rtc_gettime(todr_chip_handle_t, struct timeval *);
static int rtc_settime(todr_chip_handle_t, struct timeval *);

int rtc_auto_century_adjust = 1; /* XXX: do we ever want not to? */


/*
 * MD read/write functions declared in mc146818reg.h
 */
#define	RTC_ADDR	0
#define	RTC_DATA	1

u_int
mc146818_read(void *cookie, u_int reg)
{
	struct rtc_ebus_softc *sc = cookie;

	bus_space_write_1(sc->sc_bt, sc->sc_bh, RTC_ADDR, reg);
	return (bus_space_read_1(sc->sc_bt, sc->sc_bh, RTC_DATA));
}

void
mc146818_write(void *cookie, u_int reg, u_int datum)
{
	struct rtc_ebus_softc *sc = cookie;

	bus_space_write_1(sc->sc_bt, sc->sc_bh, RTC_ADDR, reg);
	bus_space_write_1(sc->sc_bt, sc->sc_bh, RTC_DATA, datum);
}


static int
rtcmatch_ebus(device_t parent, cfdata_t cf, void *aux)
{
	struct ebus_attach_args *ea = aux;

	return (strcmp(cf->cf_name, ea->ea_name) == 0);
}

static void
rtcattach_ebus(device_t parent, device_t self, void *aux)
{
	struct rtc_ebus_softc *sc = device_private(self);
	struct ebus_attach_args *ea = aux;
	todr_chip_handle_t handle;

	sc->sc_bt = ea->ea_bustag;
	if (bus_space_map(sc->sc_bt, EBUS_ADDR_FROM_REG(&ea->ea_reg[0]),
			  ea->ea_reg[0].size, 0, &sc->sc_bh) != 0)
	{
		printf(": unable to map registers\n");
		return;
	}

	/* XXX: no "model" property in Krups */
	printf(": time-of-day clock\n");

	/*
	 * Turn interrupts off (clear MC_REGB_?IE bits), just in case
	 * (although they shouldn't be wired to an interrupt
	 * controller on sparcs).
	 */
	mc146818_write(sc, MC_REGB, MC_REGB_BINARY | MC_REGB_24HR);

	/* setup our todr_handle */
	handle = malloc(ALIGN(sizeof(struct todr_chip_handle)),
			M_DEVBUF, M_WAITOK);
	handle->cookie = sc;
	handle->bus_cookie = NULL; /* unused */
	handle->todr_gettime = rtc_gettime;
	handle->todr_settime = rtc_settime;
	handle->todr_setwen = NULL; /* not necessary, no idprom to protect */

	todr_attach(handle);
}


/*
 * Get time-of-day and convert to a `struct timeval'
 * Return 0 on success; an error number otherwise.
 */
static int
rtc_gettime(todr_chip_handle_t handle, struct timeval *tv)
{
	struct rtc_ebus_softc *sc = handle->cookie;
	struct clock_ymdhms dt;
	u_int year;

	/* update in progress; spin loop */
	while (mc146818_read(sc, MC_REGA) & MC_REGA_UIP)
		continue;

	/* stop updates (XXX: do we need that???) */
	mc146818_write(sc, MC_REGB,
		       (mc146818_read(sc, MC_REGB) | MC_REGB_SET));

	/* read time */
	dt.dt_sec  = mc146818_read(sc, MC_SEC);
	dt.dt_min  = mc146818_read(sc, MC_MIN);
	dt.dt_hour = mc146818_read(sc, MC_HOUR);
	dt.dt_day  = mc146818_read(sc, MC_DOM);
	dt.dt_mon  = mc146818_read(sc, MC_MONTH);
	year       = mc146818_read(sc, MC_YEAR);

	/* reenable updates */
	mc146818_write(sc, MC_REGB,
		       (mc146818_read(sc, MC_REGB) & ~MC_REGB_SET));

	/* year in the century 0..99: adjust to AD */
	year += 1900;
	if (year < POSIX_BASE_YEAR && rtc_auto_century_adjust != 0)
		year += 100;
	dt.dt_year = year;

	/* simple sanity checks */
	if (dt.dt_mon > 12 || dt.dt_day > 31
	    || dt.dt_hour >= 24 || dt.dt_min >= 60 || dt.dt_sec >= 60)
		return (ERANGE);

	tv->tv_sec = clock_ymdhms_to_secs(&dt);
	tv->tv_usec = 0;
	return (0);
}

/*
 * Set the time-of-day clock based on the value of the `struct timeval' arg.
 * Return 0 on success; an error number otherwise.
 */
static int
rtc_settime(todr_chip_handle_t handle, struct timeval *tv)
{
	struct rtc_ebus_softc *sc = handle->cookie;
	struct clock_ymdhms dt;
	u_int year;

	clock_secs_to_ymdhms(tv->tv_sec, &dt);

	year = dt.dt_year - 1900;
	if (year >= 100 && rtc_auto_century_adjust != 0)
		year -= 100;

	/* stop updates */
	mc146818_write(sc, MC_REGB,
		       (mc146818_read(sc, MC_REGB) | MC_REGB_SET));

	mc146818_write(sc, MC_SEC,   dt.dt_sec);
	mc146818_write(sc, MC_MIN,   dt.dt_min);
	mc146818_write(sc, MC_HOUR,  dt.dt_hour);
	mc146818_write(sc, MC_DOW,   dt.dt_wday + 1);
	mc146818_write(sc, MC_DOM,   dt.dt_day);
	mc146818_write(sc, MC_MONTH, dt.dt_mon);
	mc146818_write(sc, MC_YEAR,  year);

	/* reenable updates */
	mc146818_write(sc, MC_REGB,
		       (mc146818_read(sc, MC_REGB) & ~MC_REGB_SET));
	return (0);
}
