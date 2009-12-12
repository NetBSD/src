/*	$NetBSD: eprtc.c,v 1.3 2009/12/12 14:44:08 tsutsui Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: eprtc.c,v 1.3 2009/12/12 14:44:08 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <dev/clock_subr.h>
#include <machine/bus.h>
#include <arm/ep93xx/ep93xxvar.h> 
#include <arm/ep93xx/epsocvar.h> 
#include <arm/ep93xx/eprtcreg.h> 

struct eprtc_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	struct todr_chip_handle	sc_todr;
};

static int eprtc_match(struct device *, struct cfdata *, void *);
static void eprtc_attach(struct device *, struct device *, void *);

CFATTACH_DECL(eprtc, sizeof(struct eprtc_softc),
	      eprtc_match, eprtc_attach, NULL, NULL);

static int eprtc_gettime(struct todr_chip_handle *, struct timeval *);
static int eprtc_settime(struct todr_chip_handle *, struct timeval *);

static int
eprtc_match(struct device *parent, struct cfdata *match, void *aux)
{
	return 1;
}

static void
eprtc_attach(struct device *parent, struct device *self, void *aux)
{
	struct eprtc_softc *sc = (struct eprtc_softc*)self;
	struct epsoc_attach_args *sa = aux;

	printf("\n");
	sc->sc_iot = sa->sa_iot;

	if (bus_space_map(sa->sa_iot, sa->sa_addr,
			  sa->sa_size, 0, &sc->sc_ioh)){
		printf("%s: Cannot map registers", self->dv_xname);
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
	struct eprtc_softc *sc = ch->cookie;;

	tv->tv_sec = bus_space_read_4(sc->sc_iot, sc->sc_ioh, EP93XX_RTC_Data);
	tv->tv_usec = 0;
	return 0;
}

static int
eprtc_settime(struct todr_chip_handle *ch, struct timeval *tv)
{
	struct eprtc_softc *sc = ch->cookie;;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, EP93XX_RTC_Load, tv->tv_sec);
	return 0;
}
