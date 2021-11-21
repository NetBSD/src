/*	$NetBSD: eprtc.c,v 1.7 2021/11/21 08:25:26 skrll Exp $	*/

/*
 * Copyright (c) 2005 HAMAJIMA Katsuomi. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: eprtc.c,v 1.7 2021/11/21 08:25:26 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <dev/clock_subr.h>
#include <sys/bus.h>
#include <arm/ep93xx/ep93xxvar.h>
#include <arm/ep93xx/epsocvar.h>
#include <arm/ep93xx/eprtcreg.h>

struct eprtc_softc {
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	struct todr_chip_handle	sc_todr;
};

static int eprtc_match(device_t, cfdata_t, void *);
static void eprtc_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(eprtc, sizeof(struct eprtc_softc),
	      eprtc_match, eprtc_attach, NULL, NULL);

static int eprtc_gettime(struct todr_chip_handle *, struct timeval *);
static int eprtc_settime(struct todr_chip_handle *, struct timeval *);

static int
eprtc_match(device_t parent, cfdata_t match, void *aux)
{
	return 1;
}

static void
eprtc_attach(device_t parent, device_t self, void *aux)
{
	struct eprtc_softc *sc = device_private(self);
	struct epsoc_attach_args *sa = aux;

	printf("\n");
	sc->sc_iot = sa->sa_iot;

	if (bus_space_map(sa->sa_iot, sa->sa_addr,
			  sa->sa_size, 0, &sc->sc_ioh)) {
		printf("%s: Cannot map registers", device_xname(self));
		return;
	}

	sc->sc_todr.cookie = sc;
	sc->sc_todr.todr_gettime = eprtc_gettime;
	sc->sc_todr.todr_settime = eprtc_settime;
	sc->sc_todr.todr_setwen = NULL;
	todr_attach(&sc->sc_todr);
}

static int
eprtc_gettime(struct todr_chip_handle *ch, struct timeval *tv)
{
	struct eprtc_softc *sc = ch->cookie;

	tv->tv_sec = bus_space_read_4(sc->sc_iot, sc->sc_ioh, EP93XX_RTC_Data);
	tv->tv_usec = 0;
	return 0;
}

static int
eprtc_settime(struct todr_chip_handle *ch, struct timeval *tv)
{
	struct eprtc_softc *sc = ch->cookie;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, EP93XX_RTC_Load, tv->tv_sec);
	return 0;
}
