/*	$NetBSD: epled.c,v 1.1.10.1 2006/04/22 11:37:17 simonb Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: epled.c,v 1.1.10.1 2006/04/22 11:37:17 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <machine/bus.h>
#include <arm/ep93xx/epgpiovar.h> 
#include <arm/ep93xx/epledvar.h> 

struct epled_softc {
	struct device		sc_dev;
	int			sc_port;
	int			sc_green;
	int			sc_red;
	struct epgpio_softc	*sc_gpio;
};

static int epled_match(struct device *, struct cfdata *, void *);
static void epled_attach(struct device *, struct device *, void *);

CFATTACH_DECL(epled, sizeof(struct epled_softc),
	      epled_match, epled_attach, NULL, NULL);

static struct epled_softc *the_epled_sc = 0;

int
epled_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return 1;
}

void
epled_attach(struct device *parent, struct device *self, void *aux)
{       
	struct epled_softc *sc = (struct epled_softc *)self;
	struct epgpio_attach_args *ga = aux;

	sc->sc_port = ga->ga_port;
	sc->sc_green = ga->ga_bit1;
	sc->sc_red = ga->ga_bit2;
	sc->sc_gpio = (struct epgpio_softc *)parent;
	printf("\n");

	if (!the_epled_sc)
		the_epled_sc = sc;
#ifdef DIAGNOSTIC
	else
		printf("%s is already configured\n", sc->sc_dev.dv_xname);
#endif

	epgpio_out(sc->sc_gpio, sc->sc_port, sc->sc_green);
	epgpio_out(sc->sc_gpio, sc->sc_port, sc->sc_red);
}

int
epled_red_on(void)
{
	struct epled_softc *sc = the_epled_sc;

#ifdef DIAGNOSTIC
	if (!sc) {
		printf("epled not configured\n");
		return (ENXIO);
	}
#endif
	epgpio_set(sc->sc_gpio, sc->sc_port, sc->sc_red);
	return 0;
}

int
epled_red_off(void)
{
	struct epled_softc *sc = the_epled_sc;

#ifdef DIAGNOSTIC
	if (!sc) {
		printf("epled not configured\n");
		return (ENXIO);
	}
#endif
	epgpio_clear(sc->sc_gpio, sc->sc_port, sc->sc_red);
	return 0;
}

int
epled_green_on(void)
{
	struct epled_softc *sc = the_epled_sc;

#ifdef DIAGNOSTIC
	if (!sc) {
		printf("epled not configured\n");
		return (ENXIO);
	}
#endif
	epgpio_set(sc->sc_gpio, sc->sc_port, sc->sc_green);
	return 0;
}

int
epled_green_off(void)
{
	struct epled_softc *sc = the_epled_sc;

#ifdef DIAGNOSTIC
	if (!sc) {
		printf("epled not configured\n");
		return (ENXIO);
	}
#endif
	epgpio_clear(sc->sc_gpio, sc->sc_port, sc->sc_green);
	return 0;
}
