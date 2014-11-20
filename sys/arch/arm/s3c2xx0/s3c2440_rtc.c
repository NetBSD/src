/*--
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Fleischer <paul@xpg.dk>
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
#include <sys/types.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/time.h>

#include <dev/clock_subr.h>

#include <arm/s3c2xx0/s3c24x0var.h>
#include <arm/s3c2xx0/s3c2440var.h>
#include <arm/s3c2xx0/s3c2440reg.h>

#ifdef SSRTC_DEBUG
#define DPRINTF(s) do { printf s; } while (/*CONSTCOND*/0)
#else
#define DPRINTF(s) do {} while (/*CONSTCOND*/0)
#endif

/* As the RTC keeps track of Leap years, we need to use the right zero-year */
#define SSRTC_YEAR_ZERO 2000

struct ssrtc_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	struct todr_chip_handle	sc_todr;
};

static int  ssrtc_match(device_t, cfdata_t, void *);
static void ssrtc_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ssrtc, sizeof(struct ssrtc_softc),
  ssrtc_match, ssrtc_attach, NULL, NULL);

static int ssrtc_todr_gettime(struct todr_chip_handle *, struct timeval *);
static int ssrtc_todr_settime(struct todr_chip_handle *, struct timeval *);

static int
ssrtc_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
ssrtc_attach(device_t parent, device_t self, void *aux)
{
	struct ssrtc_softc *sc = device_private(self);
	struct s3c2xx0_attach_args *sa = aux;

	sc->sc_dev = self;
	sc->sc_iot = sa->sa_iot;

	if (bus_space_map(sc->sc_iot, S3C2440_RTC_BASE,
			  S3C2440_RTC_SIZE, 0, &sc->sc_ioh)) {
		aprint_error(": failed to map registers");
		return;
	}
	aprint_normal(": RTC \n");

	sc->sc_todr.cookie = sc;
	sc->sc_todr.todr_gettime = ssrtc_todr_gettime;
	sc->sc_todr.todr_settime = ssrtc_todr_settime;
	sc->sc_todr.todr_setwen = NULL;

	todr_attach(&sc->sc_todr);
}

static int
ssrtc_todr_gettime(struct todr_chip_handle *h, struct timeval *tv)
{
	struct ssrtc_softc *sc = h->cookie;
	struct clock_ymdhms dt;
	uint8_t reg;

	reg = bus_space_read_1(sc->sc_iot, sc->sc_ioh, RTC_BCDSEC);
	DPRINTF(("BCDSEC: %02X\n", reg));
	dt.dt_sec = bcdtobin(reg);

	reg = bus_space_read_1(sc->sc_iot, sc->sc_ioh, RTC_BCDMIN);
	DPRINTF(("BCDMIN: %02X\n", reg));
	dt.dt_min = bcdtobin(reg);

	reg = bus_space_read_1(sc->sc_iot, sc->sc_ioh, RTC_BCDHOUR);
	DPRINTF(("BCDHOUR: %02X\n", reg));
	dt.dt_hour = bcdtobin(reg);

	reg = bus_space_read_1(sc->sc_iot, sc->sc_ioh, RTC_BCDDATE);
	DPRINTF(("BCDDATE: %02X\n", reg));
	dt.dt_day = bcdtobin(reg);

	reg = bus_space_read_1(sc->sc_iot, sc->sc_ioh, RTC_BCDDAY);
	DPRINTF(("BCDDAY: %02X\n", reg));
	dt.dt_wday = bcdtobin(reg);

	reg = bus_space_read_1(sc->sc_iot, sc->sc_ioh, RTC_BCDMON);
	DPRINTF(("BCDMON: %02X\n", reg));
	dt.dt_mon = bcdtobin(reg);

	reg = bus_space_read_1(sc->sc_iot, sc->sc_ioh, RTC_BCDYEAR);
	DPRINTF(("BCDYEAR: %02X\n", reg));
	dt.dt_year = SSRTC_YEAR_ZERO + bcdtobin(reg);

	DPRINTF(("Seconds: %d\n", dt.dt_sec));
	DPRINTF(("Minutes: %d\n", dt.dt_min));
	DPRINTF(("Hour: %d\n", dt.dt_hour));
	DPRINTF(("Mon: %d\n", dt.dt_mon));
	DPRINTF(("Date: %d\n", dt.dt_day));
	DPRINTF(("Day: %d\n", dt.dt_wday));
	DPRINTF(("Year: %d\n", dt.dt_year));

	tv->tv_sec = clock_ymdhms_to_secs(&dt);
	tv->tv_usec = 0;

	return 0;
}

static int
ssrtc_todr_settime(struct todr_chip_handle *h, struct timeval *tv)
{
	struct ssrtc_softc *sc = h->cookie;
	struct clock_ymdhms dt;
	uint8_t reg;

	clock_secs_to_ymdhms(tv->tv_sec, &dt);

	DPRINTF(("ssrtc_todr_settime"));

	/* Set RTCEN */
	reg = bus_space_read_1(sc->sc_iot, sc->sc_ioh, RTC_RTCCON);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, RTC_RTCCON, reg | RTCCON_RTCEN);

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, RTC_BCDSEC, bintobcd(dt.dt_sec));
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, RTC_BCDMIN, bintobcd(dt.dt_min));
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, RTC_BCDHOUR, bintobcd(dt.dt_hour));
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, RTC_BCDDATE, bintobcd(dt.dt_day));
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, RTC_BCDDAY, bintobcd(dt.dt_wday));
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, RTC_BCDMON, bintobcd(dt.dt_mon));
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, RTC_BCDYEAR, bintobcd(dt.dt_year-SSRTC_YEAR_ZERO));

	/* Clear RTCEN */
	reg = bus_space_read_1(sc->sc_iot, sc->sc_ioh, RTC_RTCCON);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, RTC_RTCCON, reg & ~RTCCON_RTCEN);
	return 0;
}
