/*	$NetBSD: pl030_rtc.c,v 1.6 2003/07/15 00:24:59 lukem Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: pl030_rtc.c,v 1.6 2003/07/15 00:24:59 lukem Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/device.h>

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
};

static int  plrtc_probe  (struct device *, struct cfdata *, void *);
static void plrtc_attach (struct device *, struct device *, void *);

CFATTACH_DECL(plrtc, sizeof(struct plrtc_softc),
    plrtc_probe, plrtc_attach, NULL, NULL);

/* Remember our handle, since it isn't passed in by inittodr and
   resettodr.  */
static struct plrtc_softc *plrtc_sc;

static int timeset = 0;

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

	plrtc_sc = sc;

	printf("\n");
}

void
inittodr(time_t base)
{
	time_t rtc_time;
	struct plrtc_softc *sc = plrtc_sc;

	if (sc == NULL)
		panic("RTC not attached");

	/* Default to the suggested time, but replace that with one from the
	   RTC if it seems more sensible.  */
	rtc_time = bus_space_read_4(sc->sc_iot, sc->sc_ioh, IFPGA_RTC_DR);
	
	time.tv_usec = 0;
	time.tv_sec = rtc_time;

	timeset = 1;

	if (base > rtc_time) {
		printf("inittodr: rtc value suspect, ignoring it.\n");
		time.tv_usec = 0;
		time.tv_sec = base;
		return;
	}
}

void
resettodr()
{
	struct plrtc_softc *sc = plrtc_sc;

	/* The time hasn't been set yet, so avoid writing a dubious value
	   to the RTC.  */
	if (!timeset)
		return;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, IFPGA_RTC_LR, time.tv_sec);
}
