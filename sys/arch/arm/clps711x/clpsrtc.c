/*      $NetBSD: clpsrtc.c,v 1.1 2013/04/28 11:57:13 kiyohara Exp $      */
/*
 * Copyright (c) 2013 KIYOHARA Takashi
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
__KERNEL_RCSID(0, "$NetBSD: clpsrtc.c,v 1.1 2013/04/28 11:57:13 kiyohara Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <dev/clock_subr.h>

#include <arm/clps711x/clps711xreg.h>
#include <arm/clps711x/clpssocvar.h>

#include "locators.h"

struct clpsrtc_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	struct todr_chip_handle sc_todr;
};

static int clpsrtc_match(device_t, cfdata_t, void *);
static void clpsrtc_attach(device_t, device_t, void *);

/* todr(9) interfaces */
static int clpsrtc_todr_gettime(todr_chip_handle_t, struct timeval *);
static int clpsrtc_todr_settime(todr_chip_handle_t, struct timeval *);

CFATTACH_DECL_NEW(clpsrtc, sizeof(struct clpsrtc_softc),
    clpsrtc_match, clpsrtc_attach, NULL, NULL);


/* ARGSUSED */
static int
clpsrtc_match(device_t parent, cfdata_t match, void *aux)
{

	return 1;
}

/* ARGSUSED */
static void
clpsrtc_attach(device_t parent, device_t self, void *aux)
{
	struct clpsrtc_softc *sc = device_private(self);
	struct clpssoc_attach_args *aa = aux;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_iot;
	sc->sc_ioh = *aa->aa_ioh;

	/* XXXX: reset to compare registers */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, PS711X_RTCMR, 0);

	sc->sc_todr.cookie = sc;
	sc->sc_todr.todr_gettime = clpsrtc_todr_gettime;
	sc->sc_todr.todr_settime = clpsrtc_todr_settime;
	sc->sc_todr.todr_setwen = NULL;
	todr_attach(&sc->sc_todr);
}

static int
clpsrtc_todr_gettime(todr_chip_handle_t ch, struct timeval *tv)
{
	struct clpsrtc_softc *sc = ch->cookie;

	tv->tv_sec = bus_space_read_4(sc->sc_iot, sc->sc_ioh, PS711X_RTCDR);
	tv->tv_usec = 0;
	return 0;
}

static int
clpsrtc_todr_settime(todr_chip_handle_t ch, struct timeval *tv)
{
	struct clpsrtc_softc *sc = ch->cookie;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, PS711X_RTCDR, tv->tv_sec);
	return 0;
}
