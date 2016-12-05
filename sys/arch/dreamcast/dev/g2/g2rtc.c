/* $NetBSD: g2rtc.c,v 1.7.2.1 2016/12/05 10:54:51 skrll Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: g2rtc.c,v 1.7.2.1 2016/12/05 10:54:51 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <dev/clock_subr.h>

#include <dreamcast/dev/g2/g2busvar.h>


#define G2RTC_REG_BASE	0x00710000
#define G2RTC_REG_SIZE	12

/* Offset by 20 years, 5 of them are leap */
#define G2RTC_OFFSET	(20 * SECS_PER_COMMON_YEAR + 5 * SECS_PER_DAY)

struct g2rtc_softc {
	device_t sc_dev;

	bus_space_tag_t sc_bt;
	bus_space_handle_t sc_bh;
	struct todr_chip_handle sc_tch;
};

/* autoconf glue */
static int g2rtc_match(device_t, cfdata_t, void *);
static void g2rtc_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(g2rtc, sizeof(struct g2rtc_softc),
    g2rtc_match, g2rtc_attach, NULL, NULL);


/* todr(9) methods */
static int g2rtc_todr_gettime(todr_chip_handle_t, struct timeval *);
static int g2rtc_todr_settime(todr_chip_handle_t, struct timeval *);

static inline uint32_t g2rtc_read(bus_space_tag_t, bus_space_handle_t);


static int
g2rtc_match(device_t parent, cfdata_t cf, void *aux)
{
	static int g2rtc_matched = 0;

	if (g2rtc_matched)
		return 0;

	g2rtc_matched = 1;
	return 1;
}


static void
g2rtc_attach(device_t parent, device_t self, void *aux)
{
	struct g2rtc_softc *sc = device_private(self);
	struct g2bus_attach_args *ga = aux;
	todr_chip_handle_t tch;

	sc->sc_dev = self;
	sc->sc_bt = ga->ga_memt;
	if (bus_space_map(sc->sc_bt, G2RTC_REG_BASE, G2RTC_REG_SIZE, 0,
	    &sc->sc_bh) != 0) {
		printf(": unable to map registers\n");
		return;
	}
	printf(": time-of-day clock\n");

	tch = &sc->sc_tch;
	tch->cookie = sc;
	tch->todr_gettime = g2rtc_todr_gettime;
	tch->todr_settime = g2rtc_todr_settime;
	todr_attach(tch);
}


static inline uint32_t
g2rtc_read(bus_space_tag_t bt, bus_space_handle_t bh)
{
	
	return ((bus_space_read_4(bt, bh, 0) & 0xffff) << 16)
	    | (bus_space_read_4(bt, bh, 4) & 0xffff);
}


/*
 * Get time-of-day and convert to `struct timeval'.
 * Return 0 on success; an error number otherwise.
 */
static int
g2rtc_todr_gettime(todr_chip_handle_t handle, struct timeval *tv)
{
	struct g2rtc_softc *sc = handle->cookie;
	uint32_t new, old;
	int i;

	for (old = 0;;) {
		for (i = 0; i < 3; i++) {
			new = g2rtc_read(sc->sc_bt, sc->sc_bh);
			if (new != old)
				break;
		}
		if (i < 3)
			old = new;
		else
			break;
	}

	tv->tv_sec = new - G2RTC_OFFSET;
	tv->tv_usec = 0;

	return 0;
}


/*
 * Set the time-of-day clock based on the value of the `struct timeval' arg.
 * Return 0 on success; an error number otherwise.
 */
static int
g2rtc_todr_settime(todr_chip_handle_t handle, struct timeval *tv)
{
	struct g2rtc_softc *sc = handle->cookie;
	uint32_t secs;
	int i, retry;

	secs = (uint32_t)tv->tv_sec + G2RTC_OFFSET;

	for (retry = 0; retry < 5; retry++) {

		/* Don't change the order */
		bus_space_write_4(sc->sc_bt, sc->sc_bh, 8, 1);
		bus_space_write_4(sc->sc_bt, sc->sc_bh, 4, secs & 0xffff);
		bus_space_write_4(sc->sc_bt, sc->sc_bh, 0, secs >> 16);

		/* verify */
		for (i = 0; i < 3; i++)
			if (g2rtc_read(sc->sc_bt, sc->sc_bh) == secs)
				return 0; /* ok */
	}

	return EIO;
}
