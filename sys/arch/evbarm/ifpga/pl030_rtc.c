/*	$NetBSD: pl030_rtc.c,v 1.7.24.1 2007/03/04 12:15:38 bouyer Exp $ */

/*
 * Copyright (c) 2001 ARM Ltd
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
 * 3. The name of the company may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Include header files */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pl030_rtc.c,v 1.7.24.1 2007/03/04 12:15:38 bouyer Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/device.h>

#include <dev/clock_subr.h>

#include <arm/cpufunc.h>
#include <machine/intr.h>
#include <evbarm/ifpga/ifpgavar.h>
#include <evbarm/ifpga/ifpgamem.h>
#include <evbarm/ifpga/ifpgareg.h>

#define PL030_RTC_SIZE	0x14

struct plrtc_softc {
	struct device		    sc_dev;
	bus_space_tag_t		    sc_iot;
	bus_space_handle_t	    sc_ioh;
	struct todr_chip_handle     sc_todr;
};

static int  plrtc_probe  (struct device *, struct cfdata *, void *);
static void plrtc_attach (struct device *, struct device *, void *);

CFATTACH_DECL(plrtc, sizeof(struct plrtc_softc),
    plrtc_probe, plrtc_attach, NULL, NULL);

static int
plrtc_gettime(todr_chip_handle_t todr, volatile struct timeval *tv)
{
	struct plrtc_softc *sc;

 	sc = (struct plrtc_softc *)todr->cookie;
	tv->tv_sec = bus_space_read_4(sc->sc_iot, sc->sc_ioh, IFPGA_RTC_DR);
	/* initialize tv_usec? */
	return 0;
}

static int
plrtc_settime(todr_chip_handle_t todr, volatile struct timeval *tv)
{
	struct plrtc_softc *sc;

 	sc = (struct plrtc_softc *)todr->cookie;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, IFPGA_RTC_LR, tv->tv_sec);
	return 0;
}

static int
plrtc_probe(struct device *parent, struct cfdata *cf, void *aux)
{
	return 1;
}

static void
plrtc_attach(struct device *parent, struct device *self, void *aux)
{
	struct ifpga_attach_args *ifa = aux;
	struct plrtc_softc *sc = (struct plrtc_softc *)self;

	sc->sc_iot = ifa->ifa_iot;
	if (bus_space_map(ifa->ifa_iot, ifa->ifa_addr, PL030_RTC_SIZE, 0,
	    &sc->sc_ioh)) {
		printf("%s: unable to map device\n", sc->sc_dev.dv_xname);
		return;
	}

	memset(&sc->sc_todr, 0, sizeof(struct todr_chip_handle));
	sc->sc_todr.cookie = sc;
	sc->sc_todr.todr_gettime = plrtc_gettime;
	sc->sc_todr.todr_settime = plrtc_settime;

	todr_attach( &sc->sc_todr );

	printf("\n");
}
