/*      $NetBSD: wmrtc.c,v 1.1 2013/04/28 12:11:26 kiyohara Exp $      */
/*
 * Copyright (c) 2012 KIYOHARA Takashi
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wmrtc.c,v 1.1 2013/04/28 12:11:26 kiyohara Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <dev/clock_subr.h>

#include <epoc32/windermere/windermerereg.h>
#include <epoc32/windermere/windermerevar.h>

#include "locators.h"

#define RTC_SIZE	0x100

struct wmrtc_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	struct todr_chip_handle sc_todr;
};

static int wmrtc_match(device_t, cfdata_t, void *);
static void wmrtc_attach(device_t, device_t, void *);

/* todr(9) interfaces */
static int wmrtc_todr_gettime(todr_chip_handle_t, struct timeval *);
static int wmrtc_todr_settime(todr_chip_handle_t, struct timeval *);

CFATTACH_DECL_NEW(wmrtc, sizeof(struct wmrtc_softc),
    wmrtc_match, wmrtc_attach, NULL, NULL);


/* ARGSUSED */
static int
wmrtc_match(device_t parent, cfdata_t match, void *aux)
{
	struct windermere_attach_args *aa = aux;

	/* Wildcard not accept */
	if (aa->aa_offset == WINDERMERECF_OFFSET_DEFAULT)
		return 0;

	aa->aa_size = RTC_SIZE;
	return 1;
}

/* ARGSUSED */
static void
wmrtc_attach(device_t parent, device_t self, void *aux)
{
	struct wmrtc_softc *sc = device_private(self);
	struct windermere_attach_args *aa = aux;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_dev = self;
	if (windermere_bus_space_subregion(aa->aa_iot, *aa->aa_ioh,
				aa->aa_offset, aa->aa_size, &sc->sc_ioh) != 0) {
		aprint_error_dev(self, "can't map registers\n");
		return;
	}
	sc->sc_iot = aa->aa_iot;

	/* XXXX: reset to compare registers */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, RTC_MRL, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, RTC_MRH, 0);

	sc->sc_todr.cookie = sc;
	sc->sc_todr.todr_gettime = wmrtc_todr_gettime;
	sc->sc_todr.todr_settime = wmrtc_todr_settime;
	sc->sc_todr.todr_setwen = NULL;
	todr_attach(&sc->sc_todr);
}

static int
wmrtc_todr_gettime(todr_chip_handle_t ch, struct timeval *tv)
{
	struct wmrtc_softc *sc = ch->cookie;

	tv->tv_sec = bus_space_read_4(sc->sc_iot, sc->sc_ioh, RTC_DRL) & 0xffff;
	tv->tv_sec |= (bus_space_read_4(sc->sc_iot, sc->sc_ioh, RTC_DRH) << 16);
	tv->tv_usec = 0;
	return 0;
}

static int
wmrtc_todr_settime(todr_chip_handle_t ch, struct timeval *tv)
{
	struct wmrtc_softc *sc = ch->cookie;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, RTC_DRL, tv->tv_sec & 0xffff);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, RTC_DRH,
	    (tv->tv_sec >> 16) & 0xffff);
	return 0;
}
